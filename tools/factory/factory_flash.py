#!/usr/bin/env python3
"""Flashing station for jaam_fusion boards (JAAM3.3, JAAM2, JAAM1): register by order, test, ship.

One board at a time, over a USB cable. The station works with the paid orders of the store (OpenSpec
device-order-autoregistration), so the customer, the order number and the device type land in the admin panel's
registry on their own:

  1. read the chip, derive the chip_id the firmware reports (the same unpadded "%04x%04x" of the efuse MAC);
  2. pick the order: the station lists the paid orders with free units (GET /api/factory/pending, the oldest first),
     or take --order-number; the type of the order decides the test_mode build (JAAM3.3 -> jaam3, ...);
  3. POST /api/factory/register reserves a unit of the order for the chip and fills the registry; the operator
     confirms "order N, device i of n" before anything is erased;
  4. erase, flash the test_mode build (hardware test + NVS defaults + OTA, test_mode/test_mode.cpp) together with an
     NVS image "factory" holding the order number, the station's WiFi, the release URL and the data server of the stage;
  5. the operator walks through the test and long-presses BTN1; the board takes the NVS defaults, downloads the
     latest release over WiFi (OTA), wipes the "factory" namespace and restarts into the release firmware;
  6. the station reads "FT:OTA OK" and "FT:CLEANUP OK" from serial (they are printed before the restart, the release
     firmware does not answer afterwards) and confirms the unit (POST /api/factory/complete).

The unit goes back to the queue (POST /api/factory/release) only on a KNOWN failure: "FT:OTA ERROR" or an operator who
typed f. A timeout, a lost port or Ctrl+C leave it reserved (report unit_state reserved_unknown): the board may already
carry the release, so the operator decides on the admin page "Пристрої замовлень". A release the panel did not confirm
is retried three times, then reported as release_failed.

--no-order (needs --hw-version) flashes without any unit (prototypes); --fake-chip (dev only) runs just the server
half without a board.

Configuration: tools/factory/.env (copy .env.example; gitignored) or the environment; a variable already set wins:
  FACTORY_STAGE            prod | dev (or --stage; there is no default on purpose)
  JAAM_FACTORY_TOKEN_PROD  personal API token from admin.jaam.net.ua (Акаунт -> API-токени)
  JAAM_FACTORY_TOKEN_DEV   same, from dev-admin.jaam.net.ua (hidden prompt if unset; no --token flag on purpose)
  FACTORY_WIFI_SSID / FACTORY_WIFI_PASS  2.4GHz network the board uses for the OTA
"""

from __future__ import annotations

import argparse
import csv
import datetime
import getpass
import glob
import json
import os
import queue
import re
import struct
import subprocess
import sys
import tempfile
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
ENV_FILE = Path(__file__).with_name(".env")

STAGES = {
    "prod": {"admin": "https://admin.jaam.net.ua", "ws_host": "ws.jaam.net.ua"},
    "dev": {"admin": "https://dev-admin.jaam.net.ua", "ws_host": "dev-ws.jaam.net.ua"},
}
# Type of the order in the registry -> PlatformIO env of test_mode (platformio.ini). Types without a build are refused.
PLATFORMS = {"JAAM3.3": "test_mode_jaam3", "JAAM2": "test_mode_jaam2", "JAAM1": "test_mode_jaam1"}
# PLATFORM_NAME of each build (platformio.ini), what the board prints in FT:BOOT
PLATFORM_NAMES = {"JAAM3.3": "JAAM3", "JAAM2": "JAAM2", "JAAM1": "JAAM1"}
RELEASES_API = "https://api.github.com/repos/J-A-A-M/jaam_fusion/releases/latest"
FIRMWARE_URL = "https://update.jaam.net.ua/{version}"
OFFSET_BOOTLOADER = 0x1000  # classic ESP32; the other offsets come from the build's partitions.bin
OFFSET_PARTITIONS = 0x8000
BAUD = 460800
TEST_TIMEOUT_S = 15 * 60
FAKE_CHIP_RE = re.compile(r"^[0-9A-Fa-f]{12}$")
UNSUPPORTED_CHIP_RE = re.compile(r"^ESP32-(S2|S3|C2|C3|C5|C6|H2|P4)")


class StationError(RuntimeError):
    pass


class OperatorRejected(StationError):
    """The operator declared the board defective: a known failure, the unit goes back to the queue."""


# --------------------------------------------------------------------------------------------
# Chip, partitions, images


def chip_id_from_mac(mac: str) -> str:
    """What the firmware prints: ESP.getEfuseMac() packs the MAC little-endian into a uint64 and the firmware formats it
    "%04x%04x" (high 16 bits, low 32 bits, the second one NOT padded to 8 digits). Upper case here, as the registry
    stores it. A board whose low half starts with a zero nibble gets fewer than 12 digits: the admin panel cannot take
    it, so it is refused here instead of failing at the server."""
    if not re.fullmatch(r"(?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}", mac):
        raise ValueError(f"bad MAC: {mac!r}")
    value = int.from_bytes(bytes.fromhex(mac.replace(":", "")), "little")
    chip_id = f"{value >> 32:04X}{value & 0xFFFFFFFF:04X}"
    if len(chip_id) != 12:
        raise StationError(
            f"chip_id {chip_id} of this board has {len(chip_id)} digits (the firmware prints it unpadded): "
            "the registry takes 12, register this board by hand"
        )
    return chip_id


def parse_esptool_info(output: str) -> tuple[str, str]:
    mac = re.search(r"^MAC:\s+((?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2})", output, re.M)
    chip = re.search(r"^Chip type:\s+(.+)$", output, re.M) or re.search(r"Chip is (.+)", output)
    if not mac or not chip:
        raise StationError("could not read the chip and MAC from esptool:\n" + output.strip()[-400:])
    chip_type = chip.group(1).strip()
    if UNSUPPORTED_CHIP_RE.match(chip_type):
        raise StationError(f"{chip_type}: test_mode builds are for the classic ESP32 boards only")
    return mac.group(1).lower(), chip_type


def parse_partitions(data: bytes) -> dict[str, tuple[int, int]]:
    """label -> (offset, size) from a partitions.bin (32-byte entries, magic AA 50, ends with EB EB or 0xFF)."""
    table = {}
    for off in range(0, len(data) - 31, 32):
        magic, _type, _subtype, offset, size, label, _flags = struct.unpack_from("<2sBBII16sI", data, off)
        if magic != b"\xaa\x50":
            break
        table[label.rstrip(b"\x00").decode("ascii", "replace")] = (offset, size)
    return table


def build_images(build_dir: Path) -> dict:
    """Bootloader, partition table and app of a test_mode build, with the offsets read from its partition table."""
    files = {name: build_dir / f"{name}.bin" for name in ("bootloader", "partitions", "firmware")}
    missing = [str(p) for p in files.values() if not p.is_file()]
    if missing:
        raise StationError("build the test firmware first (pio run -e <test_mode env>): missing " + ", ".join(missing))
    table = parse_partitions(files["partitions"].read_bytes())
    for label in ("nvs", "otadata", "app0"):
        if label not in table:
            raise StationError(f"partition {label!r} is missing in {files['partitions']}")
    return {"files": files, "table": table}


def find_boot_app0(explicit: str | None) -> Path:
    if explicit:
        if not Path(explicit).is_file():  # before the erase: a missing file must not leave the board blank
            raise StationError(f"--boot-app0 {explicit} is not a file")
        return Path(explicit)
    home = Path.home()
    for pattern in (".platformio-pioarduino/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin",
                    ".platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"):
        hits = glob.glob(str(home / pattern))
        if hits:
            return Path(hits[0])
    raise StationError("boot_app0.bin not found - build the firmware once with PlatformIO or pass --boot-app0")


def nvs_rows(order: str, wifi_ssid: str, wifi_pass: str, firmware_url: str, ws_host: str) -> list[list[str]]:
    """The namespace "factory" test_mode reads (loadFactoryConfig) and wipes after the OTA."""
    if not order.isdigit():
        raise StationError(f"order number {order!r} is not numeric: test_mode builds the device id JAAM3-<order> from it")
    return [
        ["key", "type", "encoding", "value"],
        ["factory", "namespace", "", ""],
        ["order", "data", "string", order],
        ["ssid", "data", "string", wifi_ssid],
        ["pass", "data", "string", wifi_pass],
        ["fw_url", "data", "string", firmware_url],
        ["wshost", "data", "string", ws_host],
    ]


def generate_nvs(rows: list[list[str]], out_bin: Path, size: int) -> None:
    csv_path = out_bin.with_suffix(".csv")
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        csv.writer(f).writerows(rows)
    try:
        run([sys.executable, "-m", "esp_idf_nvs_partition_gen", "generate", str(csv_path), str(out_bin), hex(size)],
            capture=True)
    finally:
        csv_path.unlink(missing_ok=True)  # it holds the station's WiFi password in clear text


def latest_release_tag(opener=urllib.request.urlopen) -> str:
    req = urllib.request.Request(RELEASES_API, headers={"Accept": "application/vnd.github+json",
                                                        "User-Agent": "jaam-factory-station/1"})
    try:
        with opener(req, timeout=20) as resp:
            tag = json.loads(resp.read()).get("tag_name", "")
    except (urllib.error.URLError, OSError, ValueError) as e:
        raise StationError(f"could not read the latest release of jaam_fusion: {e}") from None
    if not re.fullmatch(r"[0-9][0-9A-Za-z._-]*", tag or ""):
        raise StationError(f"unexpected release tag {tag!r}")
    return tag


# --------------------------------------------------------------------------------------------
# Subprocesses and configuration


def run(cmd: list[str], *, capture: bool = False) -> str:
    proc = subprocess.run(cmd, text=True, capture_output=capture)
    if proc.returncode != 0:
        detail = (proc.stdout or "") + (proc.stderr or "") if capture else ""
        raise StationError(f"command failed ({proc.returncode}): {' '.join(cmd)}\n{detail}")
    return (proc.stdout or "") if capture else ""


def esptool(port: str, *args: str, capture: bool = False) -> str:
    return run([sys.executable, "-m", "esptool", "--chip", "esp32", "--port", port, "--baud", str(BAUD), *args],
               capture=capture)


def find_port() -> str:
    from serial.tools import list_ports

    ports = [p.device for p in list_ports.comports() if p.vid is not None]
    if len(ports) != 1:
        raise StationError(f"expected exactly one USB serial device, found {len(ports)}: {ports or '-'} - pass --port")
    return ports[0]


def load_env_file(path: Path = ENV_FILE, env=os.environ) -> bool:
    if not path.is_file():
        return False
    for n, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("export "):
            line = line[len("export "):]
        key, sep, value = line.partition("=")
        key, value = key.strip(), value.strip()
        if not sep or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", key):
            raise StationError(f"{path.name}:{n}: expected KEY=VALUE")
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        if value:
            env.setdefault(key, value)
    return True


def token_env_var(stage: str) -> str:
    return f"JAAM_FACTORY_TOKEN_{stage.upper()}"


def resolve_token(stage: str, env=os.environ, prompt=getpass.getpass, interactive: bool | None = None) -> str:
    token = env.get(token_env_var(stage), "").strip()
    if not token:
        if interactive is None:
            interactive = sys.stdin.isatty()
        if not interactive:
            raise StationError(f"no API token for {stage} - set {token_env_var(stage)}")
        token = prompt(f"API-токен {STAGES[stage]['admin']} (Акаунт -> API-токени): ").strip()
    if not token.startswith("jaam_"):
        raise StationError("that doesn't look like an admin panel API token (expected jaam_...)")
    return token


# --------------------------------------------------------------------------------------------
# Admin panel: orders and units


def api_request(admin_url: str, token: str, method: str, path: str, body: dict | None = None):
    """One call to the admin panel; HTTP errors become StationError with the panel's own explanation."""
    req = urllib.request.Request(
        admin_url.rstrip("/") + path,
        data=json.dumps(body).encode() if body is not None else None,
        # Own User-Agent: Cloudflare in front of the admin panel rejects urllib's default with 403 (error 1010).
        headers={"Content-Type": "application/json", "Authorization": f"Bearer {token}",
                 "User-Agent": "jaam-factory-station/1"},
        method=method,
    )
    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            return json.loads(resp.read())
    except urllib.error.HTTPError as e:
        try:
            detail = json.loads(e.read()).get("detail", "")
        except Exception:  # noqa: BLE001
            detail = ""
        raise StationError(f"admin panel refused ({e.code}): {detail}") from None
    except (urllib.error.URLError, OSError) as e:
        raise StationError(f"admin panel unreachable: {e}") from None


def fetch_pending(admin_url: str, token: str, hw_versions=tuple(PLATFORMS)) -> list[dict]:
    query = urllib.parse.urlencode([("hw_version", hw) for hw in hw_versions])
    return api_request(admin_url, token, "GET", f"/api/factory/pending?{query}")


def choose_order(pending: list[dict], prompt=input, interactive: bool | None = None) -> dict:
    """The operator picks the order from the list (the oldest paid first, preselected). Returns the whole entry: its
    type decides the build."""
    if not pending:
        raise StationError("немає оплачених замовлень з вільними пристроями (JAAM3.3, JAAM2, JAAM1) - нічого "
                           "прошивати (для прототипу: --no-order --hw-version ...)")
    print("\nОплачені замовлення з вільними пристроями (найстаріші першими):")
    for i, o in enumerate(pending, 1):
        print(f"  {i}. №{o['order_number']}  {o['hw_version']}  вільно {o['pending']} з {o['total']}  "
              f"{o.get('customer') or ''}  оплачено {str(o['paid_at'])[:16].replace('T', ' ')}")
    if interactive is None:
        interactive = sys.stdin.isatty()
    if not interactive:
        raise StationError("немає терміналу для вибору замовлення - вкажіть --order-number")
    while True:
        answer = prompt(f"Замовлення [1-{len(pending)}, Enter = 1]: ").strip() or "1"
        if answer.isdigit() and 1 <= int(answer) <= len(pending):
            return pending[int(answer) - 1]
        print("  Невірний вибір.")


def describe_unit(unit: dict) -> str:
    return (f"Замовлення №{unit['order_number']}, пристрій {unit['unit_no']} з {unit['units_total']}, "
            f"{unit['hw_version']}, {unit.get('customer') or 'клієнт без імені'}")


def confirm(question: str, prompt=input, interactive: bool | None = None) -> bool:
    if interactive is None:
        interactive = sys.stdin.isatty()
    if not interactive:
        return True
    return prompt(f"{question} [Y/n]: ").strip().lower() in ("", "y", "yes", "т", "так")


# Keep in step with jaam_touch/tools/factory/factory_flash.py: api_request, choose_order, confirm, unit_action,
# describe_unit, check_fake_chip are the same flow there (a fix in one belongs in the other).
def unit_action(admin_url: str, token: str, action: str, chip_id: str, sleep=time.sleep) -> bool:
    """complete / release with 3 tries: a call the panel did not confirm is not 'done'."""
    last = ""
    for attempt in range(3):
        try:
            api_request(admin_url, token, "POST", f"/api/factory/{action}", {"chip_id": chip_id})
            return True
        except StationError as e:
            last = str(e)
            if attempt < 2:
                sleep(2 * (attempt + 1))
    print(f"\nУВАГА: {action} для {chip_id} не підтверджено адмінкою ({last}). Одиниця лишається зарезервованою: "
          "виправте на сторінці «Пристрої замовлень».", file=sys.stderr)
    return False


# --------------------------------------------------------------------------------------------
# Serial: the FT protocol of test_mode


@dataclass
class FtRun:
    chip_id: str = ""
    order: str = ""
    order_source: str = ""
    platform: str = ""
    ready: bool = False
    ota_started: bool = False
    ota_ok: bool = False
    ota_error: str | None = None
    cleanup_ok: bool | None = None

    def feed(self, line: str) -> None:
        line = line.strip()
        if not line.startswith("FT:"):
            return
        kind, _, rest = line[3:].partition(" ")
        if kind == "BOOT":
            self.platform = rest
        elif kind == "CHIP":
            self.chip_id = rest.strip().upper()
        elif kind == "ORDER":
            self.order, _, self.order_source = rest.partition(" ")
        elif kind == "READY":
            self.ready = True
        elif kind == "OTA":
            state, _, detail = rest.partition(" ")
            if state in ("START", "PROGRESS"):  # a lost START line must not make a late "f" release the unit
                self.ota_started = True
            elif state == "OK":
                self.ota_ok = True
            elif state == "ERROR":
                self.ota_error = detail or "unknown"
        elif kind == "CLEANUP":
            self.cleanup_ok = rest.strip() == "OK"

    @property
    def finished(self) -> bool:
        return self.ota_error is not None or (self.ota_ok and self.cleanup_ok is not None)


class OperatorInput:
    """Lines typed by the operator while the station waits for the board: f + Enter = the board is defective."""

    def __init__(self, interactive: bool | None = None, source=None):
        self.lines: queue.Queue[str] = queue.Queue()
        if interactive is None:
            interactive = sys.stdin.isatty()
        if interactive:
            threading.Thread(target=self._read, args=(source or sys.stdin,), daemon=True).start()

    def _read(self, stream) -> None:
        for line in stream:
            self.lines.put(line.strip().lower())

    def rejected(self) -> bool:
        while not self.lines.empty():
            if self.lines.get_nowait() in ("f", "fail", "брак"):
                return True
        return False


def follow_ota(port: str, expected_chip_id: str, expected_order: str, operator: OperatorInput,
               timeout_s: int = TEST_TIMEOUT_S, serial_module=None, expected_platform: str | None = None) -> FtRun:
    """Reads the FT lines until the OTA result. The USB bridge may drop while the chip resets, so the port is reopened;
    the "?" every 2 s makes the firmware re-print what it printed before we attached. The result after the restart is
    gone (the release firmware does not answer), so the OK lines must be seen here."""
    if serial_module is None:
        import serial as serial_module

    state, deadline, err_deadline = FtRun(), time.monotonic() + timeout_s, None
    ser, buf, last_ping = None, b"", 0.0
    while time.monotonic() < deadline:
        if ser is None:
            try:
                ser = serial_module.Serial(port, 115200, timeout=0.2)
            except (serial_module.SerialException, OSError):
                time.sleep(0.5)
                continue
        try:
            if time.monotonic() - last_ping > 2:
                ser.write(b"?\n")
                last_ping = time.monotonic()
            buf += ser.read(512)
        except (serial_module.SerialException, OSError):
            ser.close()
            ser = None
            continue
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            line = raw.decode("utf-8", "replace").strip()
            if line.startswith(("FT:", "[TEST]", "[UPDATE]")):
                print("    " + line)
            was_ready = state.ready
            state.feed(line)
            if state.ready and not was_ready:
                print("\n>>> Пройдіть тест на платі (стрічки, датчики, кнопки). Усе гаразд - довго натисніть BTN1: "
                      "плата сама завантажить прошивку. Брак - введіть f і Enter.")
        # Checked after the read: a START line already on the wire must be seen before "f" can release the unit. Once
        # the board is downloading the release the line is dropped and the OTA result decides.
        if operator.rejected() and not state.ota_started:
            raise OperatorRejected("the operator declared the board defective")
        if expected_platform and state.platform and state.platform != expected_platform:
            raise StationError(f"the board runs the {state.platform} test build, the order needs {expected_platform}: "
                               "wrong --test-build-dir?")
        if state.chip_id and state.chip_id != expected_chip_id:
            raise StationError(f"the board reports chip_id {state.chip_id}, the station derived {expected_chip_id} "
                               "from the MAC: stop and check chip_id_from_mac()")
        if state.order_source == "build":
            raise StationError("test_mode did not read the NVS settings (FT:ORDER ... build): wrong NVS offset?")
        if state.order and state.order != expected_order:
            raise StationError(f"the board has order {state.order}, the station sent {expected_order}")
        if state.ota_error is not None and err_deadline is None:
            err_deadline = time.monotonic() + 5  # the board prints FT:CLEANUP right after the error
        if state.ota_ok and state.cleanup_ok is not None:
            break
        if state.ota_error is not None and (state.cleanup_ok is not None or time.monotonic() > err_deadline):
            break
    if ser is not None:
        ser.close()
    if not state.finished:
        raise StationError(f"no OTA result in {timeout_s // 60} min")
    return state


# --------------------------------------------------------------------------------------------
# Commands


def write_report(reports_dir: Path, record: dict) -> Path:
    reports_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    path = reports_dir / f"{stamp}_{record['chip_id']}.json"
    path.write_text(json.dumps(record, ensure_ascii=False, indent=2), encoding="utf-8")
    return path


def check_fake_chip(args: argparse.Namespace) -> str | None:
    """--fake-chip runs only the server half of the flow: dev stand only, refused on prod before any call."""
    if not args.fake_chip:
        return None
    if args.stage != "dev":
        raise StationError("--fake-chip is allowed only with --stage dev")
    if not FAKE_CHIP_RE.match(args.fake_chip):
        raise StationError("--fake-chip must be 12 hex digits, e.g. DEADBEEF0001")
    return args.fake_chip.upper()


def cmd_run(args: argparse.Namespace) -> int:
    fake_chip = check_fake_chip(args)
    if args.no_order and args.order_number:
        raise StationError("--no-order and --order-number exclude each other")
    if args.no_order and args.hw_version not in PLATFORMS:
        raise StationError(f"--no-order needs --hw-version, one of {', '.join(PLATFORMS)}")
    stage = STAGES[args.stage]
    admin_url = args.admin_url or stage["admin"]
    ssid = args.wifi_ssid or os.environ.get("FACTORY_WIFI_SSID")
    wifi_pass = args.wifi_pass if args.wifi_pass is not None else os.environ.get("FACTORY_WIFI_PASS", "")
    if not ssid and not fake_chip:
        raise StationError("no station WiFi - set FACTORY_WIFI_SSID / FACTORY_WIFI_PASS")

    token = None if args.no_order else resolve_token(args.stage)
    port = None
    if fake_chip:
        mac, chip_id = "(fake)", fake_chip
    else:
        port = args.port or find_port()
        mac, chip_type = parse_esptool_info(esptool(port, "read-mac", capture=True))
        chip_id = chip_id_from_mac(mac)
        print(f"=== {chip_type}, MAC {mac} -> chip_id {chip_id}")

    # The order and its type first: nothing is reserved, erased or flashed until the operator picked.
    if args.no_order:
        hw_version, order_number = args.hw_version, "0"
    else:
        if args.order_number:
            hw_version = args.hw_version
            if hw_version not in PLATFORMS:
                raise StationError(f"--order-number needs --hw-version (the type of the order), one of {', '.join(PLATFORMS)}")
            order_number = args.order_number
        else:
            picked = choose_order(fetch_pending(admin_url, token))
            hw_version, order_number = picked["hw_version"], picked["order_number"]
    env_name = PLATFORMS.get(hw_version)
    if env_name is None:
        raise StationError(f"no test_mode build for {hw_version}")
    if not order_number.isdigit():  # before the reservation: test_mode builds the device id from the digits
        raise StationError(f"order number {order_number!r} is not numeric: test_mode builds JAAM3-<order> from it")

    firmware_url = None
    images = boot_app0 = None
    if not fake_chip:
        build_dir = Path(args.test_build_dir or REPO_ROOT / ".pio" / "build" / env_name)
        images = build_images(build_dir)
        boot_app0 = find_boot_app0(args.boot_app0)
        firmware_url = args.firmware_url or FIRMWARE_URL.format(version=args.firmware_version or latest_release_tag())
    print(f"=== СЕРЕДОВИЩЕ: {args.stage.upper()}  (адмінка {admin_url}, сервер даних {stage['ws_host']}, "
          f"тип {hw_version}, збірка {env_name}, прошивка {firmware_url or '(fake)'})")

    unit = None
    if not args.no_order:
        # Before the erase: a refusal (409: no free unit, not paid, wrong type) must leave the board as it was.
        print(f"=== Резервую пристрій замовлення ({admin_url})")
        registered = api_request(admin_url, token, "POST", "/api/factory/register",
                                 {"chip_id": chip_id, "hw_version": hw_version, "order_number": order_number})
        unit = registered.get("unit")
        if unit is None:
            raise StationError("the admin panel did not return a unit - is it an old server?")

    record = {"chip_id": chip_id, "mac": mac, "stage": args.stage, "hw_version": hw_version, "order_number": order_number,
              "build": env_name, "firmware_url": firmware_url, "unit": unit, "unit_state": "reserved" if unit else "none"}
    try:
        return _flash_and_wait(args, stage, admin_url, token, fake_chip, chip_id, order_number, ssid, wifi_pass,
                               firmware_url, unit, record, images, boot_app0, port)
    except (StationError, KeyboardInterrupt) as e:
        if isinstance(e, OperatorRejected):
            return _reject(args, admin_url, token, chip_id, unit, record, "operator")
        # Result unknown (timeout, lost port, Ctrl+C, cleanup failed): the board may already carry the release, so the
        # unit stays reserved and the operator decides on the admin page - never an automatic release.
        if unit and record["unit_state"] == "reserved":
            record["unit_state"], record["passed"], record["error"] = "reserved_unknown", None, str(e)
            print(f"\nУВАГА: результат невідомий, одиницю {chip_id} НЕ повернуто в чергу. Перевірте плату й звільніть "
                  "одиницю на сторінці «Пристрої замовлень» в адмінці.", file=sys.stderr)
            write_report(Path(args.reports_dir), record)
        raise


def _reject(args, admin_url, token, chip_id, unit, record, why: str) -> int:
    """A known failure: the board is rejected, the unit goes back to the queue (once the panel confirmed it)."""
    if unit:
        ok = unit_action(admin_url, token, "release", chip_id)
        record["unit_state"] = "released" if ok else "release_failed"
    record.update({"passed": False, "failure": why,
                   "finished_at": datetime.datetime.now().astimezone().isoformat(timespec="seconds")})
    report = write_report(Path(args.reports_dir), record)
    print(f"\nБРАК: {chip_id} ({why}). Звіт: {report}")
    return 1


def _flash_and_wait(args, stage, admin_url, token, fake_chip, chip_id, order_number, ssid, wifi_pass, firmware_url,
                    unit, record, images, boot_app0, port) -> int:
    if unit:
        print(f"\n=== {describe_unit(unit)}")
        if not confirm("Прошивати цю плату для цього замовлення?"):
            return _reject(args, admin_url, token, chip_id, unit, record, "declined")

    if fake_chip:
        if args.fake_fail:
            return _reject(args, admin_url, token, chip_id, unit, record, "fake-fail")
    else:
        table = images["table"]
        nvs_offset, nvs_size = table["nvs"]
        with tempfile.TemporaryDirectory(prefix="jaam-factory-") as tmp:
            nvs_bin = Path(tmp) / "nvs.bin"
            # Built before the erase: a bad order number or a missing generator must not leave the board blank.
            generate_nvs(nvs_rows(order_number, ssid, wifi_pass, firmware_url, stage["ws_host"]), nvs_bin, nvs_size)
            if not args.no_erase:
                print("=== Стираю flash")
                esptool(port, "erase-flash")
            print("=== Заливаю тестову прошивку з налаштуваннями")
            f = images["files"]
            esptool(port, "write-flash",
                    hex(OFFSET_BOOTLOADER), str(f["bootloader"]),
                    hex(OFFSET_PARTITIONS), str(f["partitions"]),
                    hex(nvs_offset), str(nvs_bin),
                    hex(table["otadata"][0]), str(boot_app0),
                    hex(table["app0"][0]), str(f["firmware"]))
        print("=== Очікую тест і OTA")
        result = follow_ota(port, chip_id, order_number, OperatorInput(), expected_platform=PLATFORM_NAMES[record["hw_version"]])
        if result.ota_error is not None:
            if result.cleanup_ok is not True:
                print("\nУВАГА: після помилки OTA плата не підтвердила очищення (FT:CLEANUP): пароль Wi-Fi станції може "
                      "лишатися в NVS забракованої плати.", file=sys.stderr)
                record["cleanup"] = "unconfirmed"
            return _reject(args, admin_url, token, chip_id, unit, record, f"OTA: {result.ota_error}")
        if result.cleanup_ok is not True:
            raise StationError("the OTA went through but the board did not wipe the station's settings "
                               "(FT:CLEANUP FAIL): its WiFi password is still in NVS - check the board")
        record["ota"] = "ok"

    record.update({"passed": True, "finished_at": datetime.datetime.now().astimezone().isoformat(timespec="seconds")})
    if unit:
        record["unit_state"] = "flashed" if unit_action(admin_url, token, "complete", chip_id) else "complete_failed"
    report = write_report(Path(args.reports_dir), record)
    print(f"\nПРОЙДЕНО: {chip_id}, робочу прошивку завантажено. Звіт: {report}")
    return 0


def cmd_chip_id(args: argparse.Namespace) -> int:
    port = args.port or find_port()
    mac, _ = parse_esptool_info(esptool(port, "read-mac", capture=True))
    print(chip_id_from_mac(mac))
    return 0


def main(argv: list[str] | None = None) -> int:
    try:
        load_env_file()
    except StationError as e:
        print(f"\nПОМИЛКА: {e}", file=sys.stderr)
        return 2
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", help="serial port (default: the single connected USB serial device)")
    parser.add_argument("--stage", choices=sorted(STAGES), default=os.environ.get("FACTORY_STAGE"),
                        help="prod or dev (required, or $FACTORY_STAGE)")
    parser.add_argument("--boot-app0", help="path to boot_app0.bin (default: from the PlatformIO framework package)")
    sub = parser.add_subparsers(dest="command", required=True)

    p_run = sub.add_parser("run", help="register + test + OTA the release (the normal flow)")
    p_run.add_argument("--order-number", help="flash for this order (default: pick from the list of paid orders); "
                                              "needs --hw-version")
    p_run.add_argument("--hw-version", help="type of the order (JAAM3.3, JAAM2, JAAM1): with --order-number or --no-order")
    p_run.add_argument("--no-order", action="store_true", help="no order and no unit (prototype): needs --hw-version")
    p_run.add_argument("--firmware-version", help="release tag to OTA (default: the latest GitHub release)")
    p_run.add_argument("--firmware-url", help="full OTA URL instead of https://update.jaam.net.ua/<version>")
    p_run.add_argument("--no-erase", action="store_true", help="skip the full flash erase")
    p_run.add_argument("--admin-url", help="admin panel URL (default: per --stage)")
    p_run.add_argument("--wifi-ssid", help="station WiFi (default: $FACTORY_WIFI_SSID)")
    p_run.add_argument("--wifi-pass", help="station WiFi password (default: $FACTORY_WIFI_PASS)")
    p_run.add_argument("--test-build-dir", help="test_mode build dir (default: .pio/build/<env of the order's type>)")
    p_run.add_argument("--reports-dir", default=str(REPO_ROOT / "factory_reports"),
                       help="where per-device JSON reports go (default: ./factory_reports)")
    p_run.add_argument("--fake-chip", metavar="CHIP_ID",
                       help="dev only: run just the server half of the flow for this made-up chip_id, no board")
    p_run.add_argument("--fake-fail", action="store_true", help="with --fake-chip: pretend the board is defective")
    p_run.set_defaults(func=cmd_run)

    p_chip = sub.add_parser("chip-id", help="print the chip_id of the connected board")
    p_chip.set_defaults(func=cmd_chip_id)

    args = parser.parse_args(argv)
    if args.command == "run" and args.stage not in STAGES:
        parser.error("choose the stage explicitly: --stage {dev,prod} or FACTORY_STAGE")
    try:
        return args.func(args)
    except StationError as e:
        print(f"\nПОМИЛКА: {e}", file=sys.stderr)
        return 2
    except KeyboardInterrupt:
        print("\nПерервано.", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
