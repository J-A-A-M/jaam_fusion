#!/usr/bin/env python3
"""
Тестовий WebSocket клієнт для JAAM Fusion API
Підтримує відправку команд та отримання подій
"""
import asyncio
import websockets
import json
import sys
import traceback


def parse_alert_flags(flags16):
    """Parse flags16 and return list of active alert types."""
    alert_names = {
        0: "Air",
        1: "Artillery",
        2: "Urban",
        3: "Chemical",
        4: "Nuclear",
        5: "Drones",
        6: "Missiles",
        7: "KAB",
        8: "Ballistic",
        9: "Explosion",
        10: "Recon Drones",
    }
    active_alerts = []
    for bit, name in alert_names.items():
        if flags16 & (1 << bit):
            active_alerts.append(name)
    return active_alerts if active_alerts else ["NO_ALERT"]


def format_uptime(seconds):
    """Format uptime from seconds to human-readable string."""
    if seconds is None:
        return "N/A"

    days = seconds // 86400
    hours = (seconds % 86400) // 3600
    minutes = (seconds % 3600) // 60
    secs = seconds % 60

    parts = []
    if days > 0:
        parts.append(f"{days}d")
    if hours > 0:
        parts.append(f"{hours}h")
    if minutes > 0:
        parts.append(f"{minutes}m")
    if secs > 0 or not parts:  # Show seconds if it's the only value or non-zero
        parts.append(f"{secs}s")

    return " ".join(parts)


async def send_command(websocket):
    """
    Інтерактивна відправка команд через WebSocket.
    Використовує websocket.send для надсилання команд.
    """
    print("\n" + "=" * 60)
    print("Available commands:")
    print("  1. Set map mode - mode_id: 0=OFF, 1=ALERT, 2=WEATHER, 3=FLAG, 4=RANDOM, 5=LAMP")
    print("  2. Set display mode - mode_id: 0=OFF, 1=CLOCK, 2=WEATHER, 3=TECH_INFO, 4=CLIMATE, 9=COMBINED")
    print("  3. Set lamp color and brightness")
    print("  4. Set home region")
    print("  5. Set night mode (on/off)")
    print("  q. Quit")
    print("=" * 60)

    while True:
        cmd = input("Enter command (1/2/3/4/5/q): ").strip()
        if cmd == "1":
            mode_id = input("Enter map mode id (0-5): ").strip()
            try:
                mode_id = int(mode_id)
            except ValueError:
                print("Invalid mode id")
                continue
            msg = {"type": "set_map_mode", "mode_id": mode_id}
            await websocket.send(json.dumps(msg))
            print(f"Sent: {msg}")
        elif cmd == "2":
            mode_id = input("Enter display mode id (0-9): ").strip()
            try:
                mode_id = int(mode_id)
            except ValueError:
                print("Invalid mode id")
                continue
            msg = {"type": "set_display_mode", "mode_id": mode_id}
            await websocket.send(json.dumps(msg))
            print(f"Sent: {msg}")
        elif cmd == "3":
            color = input("Enter lamp color (e.g. #FF0000): ").strip()
            brightness = input("Enter brightness (0-100): ").strip()
            try:
                brightness = int(brightness)
            except ValueError:
                print("Invalid brightness")
                continue
            msg = {"type": "set_lamp", "color": color, "brightness": brightness}
            await websocket.send(json.dumps(msg))
            print(f"Sent: {msg}")
        elif cmd == "4":
            region_id = input("Enter home region id: ").strip()
            try:
                region_id = int(region_id)
            except ValueError:
                print("Invalid region id")
                continue
            msg = {"type": "set_home_region", "region_id": region_id}
            await websocket.send(json.dumps(msg))
            print(f"Sent: {msg}")
        elif cmd == "5":
            state = input("Enter night mode state (on/off or true/false): ").strip().lower()
            if state in ["on", "true", "1"]:
                state_bool = True
            elif state in ["off", "false", "0"]:
                state_bool = False
            else:
                print("Invalid state. Use 'on', 'off', 'true', or 'false'")
                continue
            msg = {"type": "set_night_mode", "state": state_bool}
            await websocket.send(json.dumps(msg))
            print(f"Sent: {msg}")
        elif cmd.lower() == "q":
            print("Exiting command sender.")
            break
        else:
            print("Unknown command.")


async def receive_messages(websocket):
    """Отримання повідомлень від сервера."""
    try:
        while True:
            message = await websocket.recv()

            try:
                data = json.loads(message)
                msg_type = data.get("type")

                print(f"\n[{msg_type}]")
                print(json.dumps(data, indent=2, ensure_ascii=False))
                print("-" * 60)

                # Виводимо зручний summary
                if msg_type == "initial_state":
                    mode_names = {0: "OFF", 1: "ALERT", 2: "WEATHER", 3: "FLAG", 4: "RANDOM", 5: "LAMP"}
                    display_names = {0: "OFF", 1: "CLOCK", 2: "WEATHER", 3: "TECH_INFO", 4: "CLIMATE", 9: "COMBINED"}

                    map_mode_id = data.get("map_mode_id")
                    map_mode_name = mode_names.get(map_mode_id, f"Unknown({map_mode_id})")

                    display_mode_id = data.get("display_mode_id")
                    display_mode_name = display_names.get(display_mode_id, f"Unknown({display_mode_id})")

                    flags16 = data.get("home_alert_flags", 0)
                    active_alerts = parse_alert_flags(flags16)

                    print("✓ Initial state received")
                    print(f"  Device: {data.get('chip_id')} (FW: {data.get('fw_version')})")
                    if data.get("fw_latest"):
                        print(f"  Latest FW: {data.get('fw_latest')}")
                    print(f"  Name: {data.get('device_name', 'N/A')}")
                    print(f"  Map mode: {map_mode_name} (id={map_mode_id})")
                    print(f"  Display mode: {display_mode_name} (id={display_mode_id})")
                    print(f"  Home region: {data.get('home_region')}")
                    print(f"  Home alert: {', '.join(active_alerts)} (flags=0x{flags16:04X})")
                    print(f"  Home temp: {data.get('home_district_temp', 'N/A')}°C")
                    print(f"  Night mode: {'ON' if data.get('night_mode', False) else 'OFF'}")

                    # Supported features
                    supported_map = data.get("supported_map_modes", [])
                    supported_display = data.get("supported_display_modes", [])
                    supported_sensors = data.get("supported_sensors", [])
                    if supported_map:
                        print(f"  Supported map modes: {supported_map}")
                    if supported_display:
                        print(f"  Supported display modes: {supported_display}")
                    if supported_sensors:
                        print(f"  Supported sensors: {', '.join(supported_sensors)}")

                    # Sensor data
                    if "climate_temp" in data:
                        print(f"  Climate temp: {data.get('climate_temp')}°C")
                    if "climate_humidity" in data:
                        print(f"  Climate humidity: {data.get('climate_humidity')}%")
                    if "climate_pressure" in data:
                        print(f"  Climate pressure: {data.get('climate_pressure')} hPa")
                    if "light_level" in data:
                        print(f"  Light level: {data.get('light_level')} lx")

                    print(f"  Memory: {data.get('used_memory', 0) / 1024:.1f} KB")
                    print(
                        f"  Uptime: {format_uptime(data.get('uptime'))}, WiFi: {format_uptime(data.get('wifi_uptime'))}"
                    )
                    print(
                        f"  WiFi signal: {data.get('wifi_signal')} dBm, WebSocket: {'up' if data.get('websocket_status') else 'down'} ({format_uptime(data.get('websocket_uptime'))})"
                    )
                    print(f"  CPU temp: {data.get('cpu_temp', 'N/A')}°C")
                    lamp = data.get("lamp", {})
                    print(f"  Lamp: {lamp.get('color')} @ {lamp.get('brightness')}%")

                elif msg_type == "map_mode_change":
                    mode_names = {0: "OFF", 1: "ALERT", 2: "WEATHER", 3: "FLAG", 4: "RANDOM", 5: "LAMP"}
                    mode_id = data.get("map_mode_id")
                    mode_name = mode_names.get(mode_id, f"Unknown({mode_id})")
                    print(f"→ Map mode changed to: {mode_name} (id={mode_id})")

                elif msg_type == "display_mode_change":
                    display_names = {0: "OFF", 1: "CLOCK", 2: "WEATHER", 3: "TECH_INFO", 4: "CLIMATE", 9: "COMBINED"}
                    mode_id = data.get("display_mode_id")
                    mode_name = display_names.get(mode_id, f"Unknown({mode_id})")
                    print(f"→ Display mode changed to: {mode_name} (id={mode_id})")

                elif msg_type == "lamp_change":
                    lamp = data.get("lamp", {})
                    print(f"→ Lamp: {lamp.get('color')} @ {lamp.get('brightness')}%")

                elif msg_type == "home_region_change":
                    print(f"→ Home region: {data.get('home_region')}")

                elif msg_type == "alert_change":
                    print(f"→ Alert: region {data.get('region_id')}, type {data.get('alert_type')}")

                elif msg_type == "home_alert_change":
                    flags16 = data.get("home_alert_flags", 0)
                    active_alerts = parse_alert_flags(flags16)
                    print(f"→ Home alert changed to: {', '.join(active_alerts)} (flags=0x{flags16:04X})")

                elif msg_type == "night_mode_change":
                    state = data.get("night_mode", False)
                    print(f"→ Night mode changed to: {'ON' if state else 'OFF'}")

                elif msg_type == "system_info":
                    mem_kb = data.get("used_memory", 0) / 1024
                    print(f"→ System: {mem_kb:.1f} KB, {format_uptime(data.get('uptime'))} uptime")
                    print(f"  WiFi: {data.get('wifi_signal')} dBm, {format_uptime(data.get('wifi_uptime'))}")
                    print(
                        f"  WebSocket: {'up' if data.get('websocket_status') else 'down'}, {format_uptime(data.get('websocket_uptime'))}"
                    )
                    print(f"  CPU temp: {data.get('cpu_temp', 'N/A')}°C")

                elif msg_type == "device_name_change":
                    print(f"→ Device name changed to: {data.get('device_name')}")

                elif msg_type == "home_district_temp_change":
                    print(f"→ Home district temp changed to: {data.get('home_district_temp')}°C")

                elif msg_type == "climate_data_change":
                    parts = []
                    if "climate_temp" in data:
                        parts.append(f"temp={data.get('climate_temp')}°C")
                    if "climate_humidity" in data:
                        parts.append(f"humidity={data.get('climate_humidity')}%")
                    if "climate_pressure" in data:
                        parts.append(f"pressure={data.get('climate_pressure')} hPa")
                    print(f"→ Climate data: {', '.join(parts) if parts else 'N/A'}")

                elif msg_type == "light_level_change":
                    print(f"→ Light level changed to: {data.get('light_level')} lx")

                elif msg_type == "firmware_update":
                    print(f"→ New firmware available: {data.get('fw_latest')}")

                elif msg_type == "fw_update_progress":
                    print(f"→ Firmware update progress: {data.get('progress')}%")

            except json.JSONDecodeError as e:
                print(f"\n✗ Failed to parse JSON: {e}")

    except asyncio.CancelledError:
        print("\n✓ Receive task cancelled")
    except websockets.exceptions.ConnectionClosed:
        print("\n✗ Connection closed")


async def test_commands(websocket):
    """Тестування команд."""
    # Даємо час отримати initial state
    await asyncio.sleep(1)

    print("\n" + "=" * 60)
    print("Testing commands...")
    print("=" * 60)

    # Тест 1: Змінюємо режим мапи на LAMP (mode_id=5)
    print("\n→ Sending: set_map_mode to LAMP (mode_id=5)")
    await websocket.send(json.dumps({"type": "set_map_mode", "mode_id": 5}))
    await asyncio.sleep(2)

    # Тест 2: Змінюємо колір та яскравість лампи
    print("\n→ Sending: set_lamp to red at 75%")
    await websocket.send(json.dumps({"type": "set_lamp", "color": "#FF0000", "brightness": 75}))
    await asyncio.sleep(2)

    # Тест 3: Змінюємо домашній регіон
    print("\n→ Sending: set_home_region to 31 (Kyiv)")
    await websocket.send(json.dumps({"type": "set_home_region", "region_id": 31}))
    await asyncio.sleep(2)

    # Тест 4: Повертаємось до режиму ALERT (mode_id=1)
    print("\n→ Sending: set_map_mode to ALERT (mode_id=1)")
    await websocket.send(json.dumps({"type": "set_map_mode", "mode_id": 1}))

    print("\n" + "=" * 60)
    print("Commands test completed. Listening for events...")
    print("Press Ctrl+C to stop")
    print("=" * 60)


async def test_websocket_client(host="192.168.1.100", test_mode=False):
    """Підключення до WebSocket та отримання подій."""
    url = f"ws://{host}:81"

    print(f"Connecting to {url}...")
    print("-" * 60)

    try:
        async with websockets.connect(url) as websocket:
            print("✓ Connected to WebSocket")
            print("-" * 60)

            if test_mode:
                # Запускаємо паралельно отримання повідомлень та відправку тестових команд
                receiver = asyncio.create_task(receive_messages(websocket))
                tester = asyncio.create_task(test_commands(websocket))

                # Чекаємо завершення тестів
                await tester
                # Даємо ще трохи часу отримати відповіді
                await asyncio.sleep(2)
                # Скасовуємо receiver
                receiver.cancel()
                try:
                    await receiver
                except asyncio.CancelledError:
                    pass
            else:
                # Просто слухаємо події
                print("Listening for events... (Press Ctrl+C to stop)")
                print("-" * 60)
                await receive_messages(websocket)

    except KeyboardInterrupt:
        print("\n\nDisconnected by user")
    except Exception as e:
        print(f"\nError: {e}")
        traceback.print_exc()
        return 1

    return 0


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 test_ws_client.py <host> [--test]")
        print("  <host>: ESP32 IP address")
        print("  --test: Run automated command tests")
        sys.exit(1)

    host = sys.argv[1]
    test_mode = "--test" in sys.argv

    try:
        asyncio.run(test_websocket_client(host, test_mode))
    except KeyboardInterrupt:
        print("\n\n✓ Interrupted by user")
        sys.exit(0)
