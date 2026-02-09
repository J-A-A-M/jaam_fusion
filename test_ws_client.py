#!/usr/bin/env python3
"""
Тестовий WebSocket клієнт для JAAM Fusion API
Підтримує відправку команд та отримання подій
"""
import asyncio
import websockets
import json
import sys


def parse_alert_flags(flags16):
    """Parse flags16 and return list of active alert types."""
    alert_names = {
        0: 'Air',
        1: 'Artillery',
        2: 'Urban',
        3: 'Chemical',
        4: 'Nuclear',
        5: 'Drones',
        6: 'Missiles',
        7: 'KAB',
        8: 'Ballistic',
        9: 'Explosion',
        10: 'Recon Drones'
    }
    active_alerts = []
    for bit, name in alert_names.items():
        if flags16 & (1 << bit):
            active_alerts.append(name)
    return active_alerts if active_alerts else ['NO_ALERT']



async def send_command(websocket):
    """
    Інтерактивна відправка команд через WebSocket.
    Використовує websocket.send для надсилання команд.
    """
    print("\n" + "=" * 60)
    print("Available commands:")
    print("  1. Set map mode - mode_id: 0=OFF, 1=ALERT, 2=WEATHER, 3=FLAG, 4=LAMP")
    print("  2. Set lamp color and brightness")
    print("  3. Set home region")
    print("  q. Quit")
    print("=" * 60)

    while True:
        cmd = input("Enter command (1/2/3/q): ").strip()
        if cmd == "1":
            mode_id = input("Enter map mode id (0-4): ").strip()
            try:
                mode_id = int(mode_id)
            except ValueError:
                print("Invalid mode id")
                continue
            msg = {"type": "set_map_mode", "mode_id": mode_id}
            await websocket.send(json.dumps(msg))
            print(f"Sent: {msg}")
        elif cmd == "2":
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
        elif cmd == "3":
            region_id = input("Enter home region id: ").strip()
            try:
                region_id = int(region_id)
            except ValueError:
                print("Invalid region id")
                continue
            msg = {"type": "set_home_region", "region_id": region_id}
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
                    mode_names = {0: "OFF", 1: "ALERT", 2: "WEATHER", 3: "FLAG", 4: "LAMP"}
                    mode_id = data.get("map_mode_id")
                    mode_name = mode_names.get(mode_id, f"Unknown({mode_id})")

                    flags16 = data.get("home_alert_flags", 0)
                    active_alerts = parse_alert_flags(flags16)

                    print("✓ Initial state received")
                    print(f"  Device: {data.get('chip_id')} (FW: {data.get('fw_version')})")
                    print(f"  Name: {data.get('device_name', 'N/A')}")
                    print(f"  Map mode: {mode_name} (id={mode_id})")
                    print(f"  Home region: {data.get('home_region')}")
                    print(f"  Home alert: {", ".join(active_alerts)} (flags=0x{flags16:04X})")
                    print(f"  Home temp: {data.get('home_district_temp', 'N/A')}°C")
                    print(f"  Memory: {data.get('used_memory', 0) / 1024:.1f} KB")
                    print(f"  Uptime: {data.get('uptime')} min, WiFi: {data.get('wifi_uptime')} min")
                    print(
                        f"  WiFi signal: {data.get('wifi_signal')} dBm, WebSocket: {'up' if data.get('websocket_status') else 'down'} ({data.get('websocket_uptime')} min)"
                    )
                    print(f"  CPU temp: {data.get('cpu_temp', 'N/A')}°C")
                    lamp = data.get("lamp", {})
                    print(f"  Lamp: {lamp.get('color')} @ {lamp.get('brightness')}%")

                elif msg_type == "map_mode_change":
                    mode_names = {0: "OFF", 1: "ALERT", 2: "WEATHER", 3: "FLAG", 4: "LAMP"}
                    mode_id = data.get("map_mode_id")
                    mode_name = mode_names.get(mode_id, f"Unknown({mode_id})")
                    print(f"→ Map mode changed to: {mode_name} (id={mode_id})")

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
                    print(f"→ Home alert changed to: {", ".join(active_alerts)} (flags=0x{flags16:04X})")

                elif msg_type == "system_info":
                    mem_kb = data.get("used_memory", 0) / 1024
                    print(f"→ System: {mem_kb:.1f} KB, {data.get('uptime')} min uptime")
                    print(f"  WiFi: {data.get('wifi_signal')} dBm, {data.get('wifi_uptime')} min")
                    print(
                        f"  WebSocket: {'up' if data.get('websocket_status') else 'down'}, {data.get('websocket_uptime')} min"
                    )
                    print(f"  CPU temp: {data.get('cpu_temp', 'N/A')}°C")

                elif msg_type == "device_name_change":
                    print(f"→ Device name changed to: {data.get('device_name')}")

                elif msg_type == "home_district_temp_change":
                    print(f"→ Home district temp changed to: {data.get('home_district_temp')}°C")

            except json.JSONDecodeError as e:
                print(f"\n✗ Failed to parse JSON: {e}")

    except websockets.exceptions.ConnectionClosed:
        print("\n✗ Connection closed")


async def test_commands(websocket):
    """Тестування команд."""
    # Даємо час отримати initial state
    await asyncio.sleep(1)

    print("\n" + "=" * 60)
    print("Testing commands...")
    print("=" * 60)

    # Тест 1: Змінюємо режим мапи на LAMP (mode_id=4)
    print("\n→ Sending: set_map_mode to LAMP (mode_id=4)")
    await websocket.send(json.dumps({"type": "set_map_mode", "mode_id": 4}))
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

    asyncio.run(test_websocket_client(host, test_mode))
