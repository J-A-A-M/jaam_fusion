#!/usr/bin/env python3
"""
Тестовий WebSocket клієнт для JAAM Fusion API
Підтримує відправку команд та отримання подій
"""
import asyncio
import websockets
import json
import sys


async def send_command(websocket):
    """Інтерактивна відправка команд."""
    print("\n" + "=" * 60)
    print("Available commands:")
    print("  1. Set map mode (off/alert/weather/flag/lamp)")
    print("  2. Set lamp color and brightness")
    print("  3. Set home region")
    print("  q. Quit")
    print("=" * 60)

    # Read from stdin in non-blocking way would be complex
    # So we'll just provide examples


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
                    print(f"✓ Initial state received")
                    print(f"  Device: {data.get('chip_id')} (FW: {data.get('fw_version')})")
                    print(f"  Mode: {data.get('map_mode')} (ID: {data.get('map_mode_id')})")
                    print(f"  Region: {data.get('home_region')}")
                    lamp = data.get("lamp", {})
                    print(f"  Lamp: {lamp.get('color')} @ {lamp.get('brightness')}%")

                elif msg_type == "map_mode_change":
                    print(f"→ Map mode changed to: {data.get('map_mode')}")

                elif msg_type == "lamp_change":
                    lamp = data.get("lamp", {})
                    print(f"→ Lamp: {lamp.get('color')} @ {lamp.get('brightness')}%")

                elif msg_type == "home_region_change":
                    print(f"→ Home region: {data.get('home_region')}")

                elif msg_type == "alert_change":
                    print(f"→ Alert: region {data.get('region_id')}, type {data.get('alert_type')}")

                print("-" * 60)

            except json.JSONDecodeError as e:
                print(f"JSON Error: {e}")
                print(f"Data: {message}")

    except websockets.exceptions.ConnectionClosed:
        print("\n✗ Connection closed")


async def test_commands(websocket):
    """Тестування команд."""
    # Даємо час отримати initial state
    await asyncio.sleep(1)

    print("\n" + "=" * 60)
    print("Testing commands...")
    print("=" * 60)

    # Тест 1: Змінюємо режим мапи на LAMP
    print("\n→ Sending: set_map_mode to 'lamp'")
    await websocket.send(json.dumps({"type": "set_map_mode", "mode": "lamp"}))
    await asyncio.sleep(2)

    # Тест 2: Змінюємо колір та яскравість лампи
    print("\n→ Sending: set_lamp to red at 75%")
    await websocket.send(json.dumps({"type": "set_lamp", "color": "#FF0000", "brightness": 75}))
    await asyncio.sleep(2)

    # Тест 3: Змінюємо домашній регіон
    print("\n→ Sending: set_home_region to 31 (Kyiv)")
    await websocket.send(json.dumps({"type": "set_home_region", "region_id": 31}))
    await asyncio.sleep(2)

    # Тест 4: Повертаємось до режиму ALERT
    print("\n→ Sending: set_map_mode to 'alert'")
    await websocket.send(json.dumps({"type": "set_map_mode", "mode": "alert"}))

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
