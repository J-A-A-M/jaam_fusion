"""Tests for the fusion flashing station's logic - run with `pytest tools/factory`.

The chip_id derivation, the partition table reading and the "result before the restart" rule are the parts that fail
silently in the field; the rest is the unit flow (reserve, confirm, release only on a known failure)."""

import io
import json
import struct
import sys
import time
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent))
import factory_flash as ff  # noqa: E402

UNIT = {"id": "u1", "order_number": "1234", "unit_no": 1, "units_total": 2, "hw_version": "JAAM3.3",
        "customer": "Шевченко Т.", "status": "reserved"}
PENDING = [
    {"order_number": "1234", "hw_version": "JAAM3.3", "pending": 2, "total": 2, "customer": "Шевченко Т.",
     "paid_at": "2026-11-26T10:00:00Z"},
    {"order_number": "1300", "hw_version": "JAAM2", "pending": 1, "total": 1, "customer": None,
     "paid_at": "2026-11-26T12:30:00Z"},
]


# --- chip, esptool output, partitions ---


def test_chip_id_matches_the_firmwares_format():
    # firmware: sprintf("%04x%04x", (uint32_t)(efuse >> 32), (uint32_t)efuse), efuse = MAC packed little-endian
    assert ff.chip_id_from_mac("24:0a:c4:12:34:56") == "563412C40A24"
    assert ff.chip_id_from_mac("DC:DA:0C:2A:B1:F0") == "F0B12A0CDADC"


def test_chip_id_shorter_than_12_digits_is_refused():
    # b3 == 0: "%04x" of the low half prints 6 digits, the registry takes 12
    with pytest.raises(ff.StationError, match="register this board by hand"):
        ff.chip_id_from_mac("24:0a:c4:00:34:56")


@pytest.mark.parametrize("bad", ["", "24:0a:c4:12:34", "24-0a-c4-12-34-56", "zz:0a:c4:12:34:56"])
def test_chip_id_rejects_garbage(bad):
    with pytest.raises(ValueError):
        ff.chip_id_from_mac(bad)


def test_esptool_info_v5_and_v4_output():
    v5 = "Chip type:          ESP32-D0WD-V3 (revision v3.1)\nMAC:                DC:DA:0C:2A:B1:F0\n"
    assert ff.parse_esptool_info(v5) == ("dc:da:0c:2a:b1:f0", "ESP32-D0WD-V3 (revision v3.1)")
    v4 = "Chip is ESP32-D0WDQ6 (revision 1)\nMAC: 24:0a:c4:12:34:56\n"
    assert ff.parse_esptool_info(v4)[0] == "24:0a:c4:12:34:56"


@pytest.mark.parametrize("chip", ["ESP32-S3 (QFN56)", "ESP32-C3", "ESP32-S2"])
def test_other_families_are_refused(chip):
    with pytest.raises(ff.StationError, match="classic ESP32"):
        ff.parse_esptool_info(f"Chip type: {chip}\nMAC: 24:0a:c4:12:34:56\n")


def test_esptool_info_missing():
    with pytest.raises(ff.StationError):
        ff.parse_esptool_info("A fatal error occurred: Failed to connect")


def table_bytes(entries):
    out = b""
    for label, offset, size in entries:
        out += struct.pack("<2sBBII16sI", b"\xaa\x50", 1, 2, offset, size, label.encode(), 0)
    return out + b"\xeb\xeb" + b"\xff" * 30  # the MD5 entry ends the table


def test_partition_table_is_read_by_label():
    data = table_bytes([("nvs", 0x9000, 0x5000), ("otadata", 0xE000, 0x2000), ("app0", 0x10000, 0x1E0000)])
    assert ff.parse_partitions(data) == {"nvs": (0x9000, 0x5000), "otadata": (0xE000, 0x2000),
                                         "app0": (0x10000, 0x1E0000)}


def make_build(tmp_path, entries=(("nvs", 0x9000, 0x5000), ("otadata", 0xE000, 0x2000), ("app0", 0x10000, 0x1E0000))):
    tmp_path.mkdir(parents=True, exist_ok=True)
    (tmp_path / "bootloader.bin").write_bytes(b"b")
    (tmp_path / "firmware.bin").write_bytes(b"f")
    (tmp_path / "partitions.bin").write_bytes(table_bytes(entries))
    return tmp_path


def test_build_images_needs_all_files_and_the_partitions(tmp_path):
    with pytest.raises(ff.StationError, match="build the test firmware first"):
        ff.build_images(tmp_path)
    images = ff.build_images(make_build(tmp_path))
    assert images["table"]["nvs"] == (0x9000, 0x5000) and images["files"]["firmware"].name == "firmware.bin"
    with pytest.raises(ff.StationError, match="'app0' is missing"):
        ff.build_images(make_build(tmp_path, (("nvs", 0x9000, 0x5000), ("otadata", 0xE000, 0x2000))))


# --- NVS image and release tag ---


def test_nvs_rows_hold_what_test_mode_reads():
    rows = ff.nvs_rows("1234", "Factory", "pw", "https://update.jaam.net.ua/5.1.3", "dev-ws.jaam.net.ua")
    assert rows[1] == ["factory", "namespace", "", ""]
    assert {r[0]: r[3] for r in rows[2:]} == {"order": "1234", "ssid": "Factory", "pass": "pw",
                                              "fw_url": "https://update.jaam.net.ua/5.1.3",
                                              "wshost": "dev-ws.jaam.net.ua"}


def test_nvs_order_must_be_numeric():
    with pytest.raises(ff.StationError, match="not numeric"):
        ff.nvs_rows("A-12", "s", "p", "u", "h")


def test_generated_nvs_image_has_the_keys_and_no_csv_left(tmp_path):
    pytest.importorskip("esp_idf_nvs_partition_gen")
    out = tmp_path / "nvs.bin"
    ff.generate_nvs(ff.nvs_rows("1234", "Factory", "pw", "https://u/5.1.3", "ws.jaam.net.ua"), out, 0x5000)
    data = out.read_bytes()
    assert len(data) == 0x5000 and b"factory" in data and b"1234" in data and b"Factory" in data
    assert not out.with_suffix(".csv").exists(), "the CSV holds the WiFi password"


def test_latest_release_tag():
    class Resp:
        def __init__(self, body):
            self.body = body

        def __enter__(self):
            return self

        def __exit__(self, *a):
            return False

        def read(self):
            return self.body

    assert ff.latest_release_tag(lambda req, timeout: Resp(b'{"tag_name": "5.1.3"}')) == "5.1.3"
    with pytest.raises(ff.StationError, match="unexpected release tag"):
        ff.latest_release_tag(lambda req, timeout: Resp(b'{"tag_name": "../evil"}'))
    with pytest.raises(ff.StationError):
        ff.latest_release_tag(lambda req, timeout: Resp(b"not json"))


# --- the FT protocol ---


def feed_all(*lines):
    state = ff.FtRun()
    for line in lines:
        state.feed(line)
    return state


def test_ft_run_happy_path_needs_both_lines():
    state = feed_all("FT:BOOT JAAM3", "FT:CHIP 563412c40a24", "FT:ORDER 1234 nvs", "FT:PHASE TEST", "FT:READY")
    assert (state.platform, state.chip_id, state.order, state.order_source, state.ready) == (
        "JAAM3", "563412C40A24", "1234", "nvs", True)
    assert not state.finished
    state.feed("FT:OTA PROGRESS 50")
    state.feed("FT:OTA OK")
    assert not state.finished, "the OTA alone is not enough: the cleanup line comes next"
    state.feed("FT:CLEANUP OK")
    assert state.finished and state.ota_ok and state.cleanup_ok is True


def test_ft_run_error_and_failed_cleanup():
    state = feed_all("FT:OTA ERROR -11 HTTP error: connection refused")
    assert state.finished and state.ota_error == "-11 HTTP error: connection refused"
    state = feed_all("FT:OTA OK", "FT:CLEANUP FAIL")
    assert state.finished and state.cleanup_ok is False
    assert not feed_all("[UPDATE] 10%", "garbage").finished


class FakeSerialModule:
    """A serial port that serves scripted chunks; `SerialException` is raised for the first `fail_opens` opens."""

    class SerialException(Exception):
        pass

    def __init__(self, chunks, fail_opens=0):
        self.chunks, self.fail_opens, self.writes = list(chunks), fail_opens, []
        module = self

        class Serial:
            def __init__(self, port, baud, timeout):
                if module.fail_opens:
                    module.fail_opens -= 1
                    raise FakeSerialModule.SerialException("no port")

            def write(self, data):
                module.writes.append(data)

            def read(self, n):
                return module.chunks.pop(0) if module.chunks else b""

            def close(self):
                pass

        self.Serial = Serial


class Operator(ff.OperatorInput):
    def __init__(self, reject=False):
        super().__init__(interactive=False)
        self.reject = reject

    def rejected(self):
        return self.reject


def lines(*ls):
    return ("\n".join(ls) + "\n").encode()


def test_follow_reads_the_result_reopens_the_port_and_pings(capsys):
    serial = FakeSerialModule([lines("FT:BOOT JAAM3", "FT:CHIP 563412C40A24", "FT:ORDER 1234 nvs", "FT:READY"),
                               lines("[UPDATE] 50%", "FT:OTA OK", "FT:CLEANUP OK")], fail_opens=2)
    result = ff.follow_ota("/dev/x", "563412C40A24", "1234", Operator(), timeout_s=10, serial_module=serial)
    assert result.ota_ok and result.cleanup_ok and serial.writes and serial.writes[0] == b"?\n"
    out = capsys.readouterr().out
    assert "Пройдіть тест" in out and "FT:OTA OK" in out


@pytest.mark.parametrize("chunk,match", [
    (lines("FT:CHIP AAAAAAAAAAAA"), "derived 563412C40A24"),
    (lines("FT:ORDER 1234 build"), "did not read the NVS settings"),
    (lines("FT:ORDER 999 nvs"), "has order 999"),
])
def test_follow_stops_on_a_mismatch(chunk, match):
    with pytest.raises(ff.StationError, match=match):
        ff.follow_ota("/dev/x", "563412C40A24", "1234", Operator(), timeout_s=5,
                      serial_module=FakeSerialModule([chunk]))


def test_follow_operator_rejection_and_timeout():
    with pytest.raises(ff.OperatorRejected):
        ff.follow_ota("/dev/x", "563412C40A24", "1234", Operator(reject=True), timeout_s=5,
                      serial_module=FakeSerialModule([]))
    with pytest.raises(ff.StationError, match="no OTA result"):
        ff.follow_ota("/dev/x", "563412C40A24", "1234", Operator(), timeout_s=0,
                      serial_module=FakeSerialModule([]))


def test_operator_input_f_means_defective():
    op = ff.OperatorInput(interactive=True, source=io.StringIO("hello\nF\n"))
    for _ in range(50):
        if op.rejected():
            break
        time.sleep(0.02)
    else:
        pytest.fail("f was not seen")


# --- orders and units ---


def test_choose_order_returns_the_entry_so_the_type_decides_the_build(capsys):
    assert ff.choose_order(PENDING, prompt=lambda q: "", interactive=True)["order_number"] == "1234"
    answers = iter(["9", "2"])
    picked = ff.choose_order(PENDING, prompt=lambda q: next(answers), interactive=True)
    assert (picked["order_number"], picked["hw_version"]) == ("1300", "JAAM2")
    assert "вільно 2 з 2" in capsys.readouterr().out
    with pytest.raises(ff.StationError, match="немає оплачених"):
        ff.choose_order([], interactive=True)
    with pytest.raises(ff.StationError, match="--order-number"):
        ff.choose_order(PENDING, interactive=False)


def test_pending_asks_for_every_supported_type(monkeypatch):
    seen = {}
    monkeypatch.setattr(ff, "api_request", lambda url, token, method, path, body=None: seen.update(path=path) or [])
    ff.fetch_pending("https://dev-admin.jaam.net.ua", "jaam_t")
    assert seen["path"] == "/api/factory/pending?hw_version=JAAM3.3&hw_version=JAAM2&hw_version=JAAM1"


class Api:
    def __init__(self, monkeypatch, fail=()):
        self.calls, self.fail = [], set(fail)
        monkeypatch.setattr(ff, "api_request", self.request)
        monkeypatch.setattr(ff, "resolve_token", lambda stage: "jaam_test")
        monkeypatch.setattr(ff.time, "sleep", lambda s: None)

    def request(self, admin_url, token, method, path, body=None):
        self.calls.append((method, path.split("?")[0], body))
        if path.split("?")[0] in self.fail:
            raise ff.StationError("admin panel refused (409): no")
        if path.startswith("/api/factory/pending"):
            return PENDING
        if path == "/api/factory/register":
            return {"chip_id": body["chip_id"], "created": True, "unit": UNIT}
        return {"chip_id": body["chip_id"], "unit": UNIT}

    def paths(self):
        return [c[1] for c in self.calls]


def run_fake(tmp_path, *extra, stage="dev"):
    return ff.main(["--stage", stage, "run", "--fake-chip", "deadbeef0001", "--reports-dir", str(tmp_path), *extra])


def report_of(tmp_path):
    [path] = tmp_path.glob("*.json")
    return json.loads(path.read_text(encoding="utf-8"))


def test_fake_chip_is_refused_on_prod_and_for_garbage(monkeypatch, tmp_path, capsys):
    api = Api(monkeypatch)
    assert run_fake(tmp_path, "--order-number", "1234", "--hw-version", "JAAM3.3", stage="prod") == 2
    assert "only with --stage dev" in capsys.readouterr().err
    assert ff.main(["--stage", "dev", "run", "--fake-chip", "nope", "--no-order", "--hw-version", "JAAM2"]) == 2
    assert api.calls == []


def test_fake_run_registers_completes_and_reports(monkeypatch, tmp_path):
    api = Api(monkeypatch)
    assert run_fake(tmp_path, "--order-number", "1234", "--hw-version", "JAAM3.3") == 0
    assert api.paths() == ["/api/factory/register", "/api/factory/complete"]
    assert api.calls[0][2] == {"chip_id": "DEADBEEF0001", "hw_version": "JAAM3.3", "order_number": "1234"}
    rec = report_of(tmp_path)
    assert (rec["unit_state"], rec["build"], rec["passed"]) == ("flashed", "test_mode_jaam3", True)


def test_picking_from_the_list_takes_the_type_and_the_build_from_the_order(monkeypatch, tmp_path):
    api = Api(monkeypatch)
    monkeypatch.setattr(ff, "choose_order", lambda pending, **kw: pending[1])
    assert run_fake(tmp_path) == 0
    assert api.paths()[0] == "/api/factory/pending"
    assert api.calls[1][2]["hw_version"] == "JAAM2" and report_of(tmp_path)["build"] == "test_mode_jaam2"


def test_a_type_without_a_build_is_refused_before_anything_happens(monkeypatch, tmp_path, capsys):
    api = Api(monkeypatch)
    assert run_fake(tmp_path, "--order-number", "1234", "--hw-version", "MATRIX") == 2
    assert api.calls == [] and "one of JAAM3.3, JAAM2, JAAM1" in capsys.readouterr().err


def test_known_failures_release_the_unit(monkeypatch, tmp_path):
    api = Api(monkeypatch)
    assert run_fake(tmp_path, "--order-number", "1234", "--hw-version", "JAAM3.3", "--fake-fail") == 1
    assert api.paths() == ["/api/factory/register", "/api/factory/release"]
    assert report_of(tmp_path)["unit_state"] == "released"


def test_release_the_panel_did_not_confirm_is_release_failed(monkeypatch, tmp_path, capsys):
    api = Api(monkeypatch, fail={"/api/factory/release"})
    assert run_fake(tmp_path, "--order-number", "1234", "--hw-version", "JAAM3.3", "--fake-fail") == 1
    assert api.paths().count("/api/factory/release") == 3 and report_of(tmp_path)["unit_state"] == "release_failed"
    assert "не підтверджено" in capsys.readouterr().err


def test_declined_confirmation_releases(monkeypatch, tmp_path):
    api = Api(monkeypatch)
    monkeypatch.setattr(ff, "confirm", lambda q, **kw: False)
    assert run_fake(tmp_path, "--order-number", "1234", "--hw-version", "JAAM3.3") == 1
    assert api.paths() == ["/api/factory/register", "/api/factory/release"]
    assert report_of(tmp_path)["failure"] == "declined"


def test_unknown_result_leaves_the_unit_reserved(monkeypatch, tmp_path, capsys):
    api = Api(monkeypatch)
    monkeypatch.setattr(ff, "_flash_and_wait", lambda *a, **k: (_ for _ in ()).throw(ff.StationError("no OTA result")))
    assert run_fake(tmp_path, "--order-number", "1234", "--hw-version", "JAAM3.3") == 2
    assert api.paths() == ["/api/factory/register"], "no release on an unknown result"
    assert report_of(tmp_path)["unit_state"] == "reserved_unknown"
    assert "НЕ повернуто" in capsys.readouterr().err


def test_no_order_touches_no_server(monkeypatch, tmp_path, capsys):
    api = Api(monkeypatch)
    assert run_fake(tmp_path, "--no-order", "--hw-version", "JAAM1") == 0
    assert api.calls == [] and report_of(tmp_path)["unit_state"] == "none"
    assert run_fake(tmp_path, "--no-order") == 2
    assert run_fake(tmp_path, "--no-order", "--order-number", "1", "--hw-version", "JAAM1") == 2


# --- the real flow with the hardware mocked away ---


class Hw:
    """esptool, the build and the serial follow-up replaced; records what would be written to the board."""

    def __init__(self, monkeypatch, tmp_path, follow):
        self.cmds, self.nvs = [], None
        build = make_build(tmp_path / "build")
        (tmp_path / "boot_app0.bin").write_bytes(b"x")
        monkeypatch.setattr(ff, "find_port", lambda: "/dev/ttyUSB0")
        monkeypatch.setattr(ff, "esptool", self.esptool)
        monkeypatch.setattr(ff, "find_boot_app0", lambda explicit: tmp_path / "boot_app0.bin")
        monkeypatch.setattr(ff, "latest_release_tag", lambda: "5.1.3")
        monkeypatch.setattr(ff, "generate_nvs", lambda rows, out, size: setattr(self, "nvs", (rows, size)) or out.write_bytes(b"n"))
        monkeypatch.setattr(ff, "follow_ota", follow)
        monkeypatch.setattr(ff, "OperatorInput", lambda *a, **k: None)
        self.build_dir = build

    def esptool(self, port, *args, capture=False):
        self.cmds.append(args)
        return "Chip type: ESP32-D0WD-V3\nMAC: 24:0a:c4:12:34:56\n" if args[0] == "read-mac" else ""


def run_real(tmp_path, *extra):
    return ff.main(["--stage", "dev", "run", "--reports-dir", str(tmp_path / "reports"), "--wifi-ssid", "Factory",
                    "--wifi-pass", "pw", "--order-number", "1234", "--hw-version", "JAAM3.3", "--test-build-dir",
                    str(tmp_path / "build"), *extra])


def ok_result(*a, **k):
    return feed_all("FT:OTA OK", "FT:CLEANUP OK")


def test_real_flow_flashes_at_the_table_offsets_and_completes(monkeypatch, tmp_path):
    api = Api(monkeypatch)
    hw = Hw(monkeypatch, tmp_path, ok_result)
    assert run_real(tmp_path) == 0
    assert api.paths() == ["/api/factory/register", "/api/factory/complete"]
    flash = next(c for c in hw.cmds if c[0] == "write-flash")
    assert list(flash[1::2]) == ["0x1000", "0x8000", "0x9000", "0xe000", "0x10000"]
    rows, size = hw.nvs
    assert size == 0x5000 and {r[0]: r[3] for r in rows[2:]}["wshost"] == "dev-ws.jaam.net.ua"
    assert {r[0]: r[3] for r in rows[2:]}["fw_url"] == "https://update.jaam.net.ua/5.1.3"
    assert ("erase-flash",) in hw.cmds and report_of(tmp_path / "reports")["unit_state"] == "flashed"


def test_real_flow_ota_error_releases(monkeypatch, tmp_path):
    api = Api(monkeypatch)
    Hw(monkeypatch, tmp_path, lambda *a, **k: feed_all("FT:OTA ERROR 3 no_wifi_config"))
    assert run_real(tmp_path) == 1
    assert api.paths() == ["/api/factory/register", "/api/factory/release"]
    assert report_of(tmp_path / "reports")["failure"].startswith("OTA:")


def test_real_flow_failed_cleanup_is_not_complete_and_not_released(monkeypatch, tmp_path, capsys):
    api = Api(monkeypatch)
    Hw(monkeypatch, tmp_path, lambda *a, **k: feed_all("FT:OTA OK", "FT:CLEANUP FAIL"))
    assert run_real(tmp_path) == 2
    assert api.paths() == ["/api/factory/register"]
    assert report_of(tmp_path / "reports")["unit_state"] == "reserved_unknown"


def test_real_flow_operator_rejection_releases(monkeypatch, tmp_path):
    api = Api(monkeypatch)

    def rejected(*a, **k):
        raise ff.OperatorRejected("defective")

    Hw(monkeypatch, tmp_path, rejected)
    assert run_real(tmp_path) == 1
    assert api.paths() == ["/api/factory/register", "/api/factory/release"]


def test_refused_registration_leaves_the_board_untouched(monkeypatch, tmp_path):
    Api(monkeypatch, fail={"/api/factory/register"})
    hw = Hw(monkeypatch, tmp_path, ok_result)
    assert run_real(tmp_path) == 2
    assert not any(c[0] in ("erase-flash", "write-flash") for c in hw.cmds)


def test_unsupported_board_family_stops_before_the_server(monkeypatch, tmp_path):
    api = Api(monkeypatch)
    hw = Hw(monkeypatch, tmp_path, ok_result)
    hw.esptool = lambda port, *args, capture=False: "Chip type: ESP32-S3\nMAC: 24:0a:c4:12:34:56\n"
    monkeypatch.setattr(ff, "esptool", hw.esptool)
    assert run_real(tmp_path) == 2 and api.calls == []


# --- configuration ---


def test_stage_is_mandatory(monkeypatch, capsys):
    monkeypatch.delenv("FACTORY_STAGE", raising=False)
    monkeypatch.setattr(ff, "load_env_file", lambda *a, **k: False)  # a real .env must not bring the stage back
    with pytest.raises(SystemExit):
        ff.main(["run", "--no-order", "--hw-version", "JAAM2"])


def test_token_is_per_stage_and_validated():
    assert ff.resolve_token("dev", env={"JAAM_FACTORY_TOKEN_DEV": "jaam_abc"}) == "jaam_abc"
    with pytest.raises(ff.StationError, match="JAAM_FACTORY_TOKEN_PROD"):
        ff.resolve_token("prod", env={"JAAM_FACTORY_TOKEN_DEV": "jaam_abc"}, interactive=False)
    with pytest.raises(ff.StationError, match="expected jaam_"):
        ff.resolve_token("dev", env={"JAAM_FACTORY_TOKEN_DEV": "nope"})


def test_env_file_never_overrides_the_environment(tmp_path):
    f = tmp_path / ".env"
    f.write_text("# c\nFACTORY_STAGE=dev\nexport FACTORY_WIFI_SSID='My Net'\nEMPTY=\n", encoding="utf-8")
    env = {"FACTORY_STAGE": "prod"}
    assert ff.load_env_file(f, env) and env == {"FACTORY_STAGE": "prod", "FACTORY_WIFI_SSID": "My Net"}
    f.write_text("garbage", encoding="utf-8")
    with pytest.raises(ff.StationError):
        ff.load_env_file(f, {})


def test_late_operator_rejection_is_ignored_once_the_ota_started():
    serial = FakeSerialModule([lines("FT:BOOT JAAM3", "FT:CHIP 563412C40A24", "FT:ORDER 1234 nvs", "FT:READY", "FT:OTA START"),
                               lines("FT:OTA OK", "FT:CLEANUP OK")])
    class LateF(Operator):
        calls = 0

        def rejected(self):  # the operator types "f" only after the first read, when the OTA already started
            self.calls += 1
            return self.calls > 1

    result = ff.follow_ota("/dev/x", "563412C40A24", "1234", LateF(), timeout_s=10, serial_module=serial)
    assert result.ota_ok and result.ota_started


def test_progress_line_counts_as_ota_started_when_start_was_lost():
    class LateF(Operator):
        calls = 0

        def rejected(self):
            self.calls += 1
            return self.calls > 1

    serial = FakeSerialModule([lines("FT:BOOT JAAM3", "FT:CHIP 563412C40A24", "FT:ORDER 1234 nvs", "FT:READY"),
                               lines("FT:OTA PROGRESS 10", "FT:OTA OK", "FT:CLEANUP OK")])
    result = ff.follow_ota("/dev/x", "563412C40A24", "1234", LateF(), timeout_s=10, serial_module=serial)
    assert result.ota_ok and result.ota_started


def test_f_is_checked_after_the_read_so_a_start_already_on_the_wire_wins():
    serial = FakeSerialModule([lines("FT:BOOT JAAM3", "FT:CHIP 563412C40A24", "FT:ORDER 1234 nvs", "FT:OTA START"),
                               lines("FT:OTA OK", "FT:CLEANUP OK")])
    result = ff.follow_ota("/dev/x", "563412C40A24", "1234", Operator(reject=True), timeout_s=10, serial_module=serial)
    assert result.ota_ok  # "f" typed before the first read does not release a board that is already updating


def test_wrong_test_build_platform_stops_the_run():
    serial = FakeSerialModule([lines("FT:BOOT JAAM1", "FT:CHIP 563412C40A24")])
    with pytest.raises(ff.StationError, match="needs JAAM3"):
        ff.follow_ota("/dev/x", "563412C40A24", "1234", Operator(), timeout_s=5, serial_module=serial,
                      expected_platform="JAAM3")


def test_ota_error_waits_briefly_for_the_cleanup_line():
    serial = FakeSerialModule([lines("FT:BOOT JAAM3", "FT:CHIP 563412C40A24", "FT:OTA ERROR 3 no_wifi"),
                               lines("FT:CLEANUP OK")])
    result = ff.follow_ota("/dev/x", "563412C40A24", "1234", Operator(), timeout_s=10, serial_module=serial)
    assert result.ota_error and result.cleanup_ok is True


def test_missing_explicit_boot_app0_is_refused_before_the_erase(tmp_path):
    with pytest.raises(ff.StationError, match="is not a file"):
        ff.find_boot_app0(str(tmp_path / "missing.bin"))
