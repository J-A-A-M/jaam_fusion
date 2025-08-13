#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import time
import requests
from typing import Dict, Tuple, Optional


def get_coordinates_from_nominatim(location_name: str, country: str = "Ukraine") -> Optional[Tuple[float, float]]:
    """
    Отримує координати з Nominatim API для заданої локації.

    Args:
        location_name: Назва локації
        country: Країна (за замовчуванням "Ukraine")

    Returns:
        Кортеж (latitude, longitude) або None якщо не знайдено
    """
    try:
        # URL для Nominatim API
        url = "https://nominatim.openstreetmap.org/search"

        # Параметри запиту
        params = {"q": f"{location_name}, {country}", "format": "json", "limit": 1, "addressdetails": 1}

        # Заголовки для ввічливого використання API
        headers = {"User-Agent": "JAAM Fusion Location Mapper/1.0"}

        print(f"  🌍 Запитую координати через API для: {location_name}")

        response = requests.get(url, params=params, headers=headers, timeout=10)
        response.raise_for_status()

        data = response.json()

        if data and len(data) > 0:
            lat = float(data[0]["lat"])
            lon = float(data[0]["lon"])
            print(f"  ✅ Знайдено: {lat:.6f}, {lon:.6f}")
            return (lat, lon)
        else:
            print(f"  ❌ Не знайдено в API")
            return None

    except requests.exceptions.RequestException as e:
        print(f"  🔥 Помилка мережі для '{location_name}': {e}")
        return None
    except (KeyError, ValueError, TypeError) as e:
        print(f"  🔥 Помилка обробки даних для '{location_name}': {e}")
        return None
    except Exception as e:
        print(f"  🔥 Невідома помилка для '{location_name}': {e}")
        return None


def get_coordinates(location_name: str) -> Tuple[float, float]:
    """
    Отримує координати для локації через API.

    Args:
        location_name: Назва локації

    Returns:
        (latitude, longitude)
    """
    # Спеціальний випадок для "Вся Україна"
    if location_name == "Вся Україна":
        print(f"  📍 Використовую центр України")
        return (49.0, 32.0)  # Центр України

    # Використовуємо API для отримання координат
    coords = get_coordinates_from_nominatim(location_name)
    if coords:
        return coords

    # Якщо API не спрацював, повертаємо координати центру України
    print(f"  ⚠️  Не вдалося знайти координати для '{location_name}', використовую центр України")
    return (49.0, 32.0)


def format_json_data(data: dict) -> str:
    """
    Форматує дані JSON з правильним вирівнюванням.

    Args:
        data: Словник з даними для форматування

    Returns:
        Відформатований JSON рядок
    """
    # Знаходимо максимальну довжину ключа для вирівнювання
    max_key_length = max(len(key) for key in data.keys())

    # Знаходимо максимальну довжину назви для вирівнювання
    max_name_length = max(len(value.get("name", "")) for value in data.values())

    # Формуємо вихідний рядок
    output_lines = ["{"]

    items = list(data.items())
    for i, (key, value) in enumerate(items):
        # Вирівнюємо ключ
        padded_key = f'"{key}"'.ljust(max_key_length + 2)

        # Вирівнюємо назву
        name = value.get("name", "")
        padded_name = f'"{name}"'.ljust(max_name_length + 2)

        # Форматуємо числові значення з вирівнюванням
        region_id = str(value.get("regionId", 0)).rjust(4)
        legacy_id = str(value.get("legacyId", 0)).rjust(2)
        state_id = str(value.get("stateId", 0)).rjust(4)

        # Додаємо кому після кожного елемента, крім останнього
        comma = "," if i < len(items) - 1 else ""

        # Формуємо location частину якщо є
        if "location" in value:
            location = value["location"]
            lat = location.get("lat", 0)
            lon = location.get("lon", 0)
            # Форматуємо координати з вирівнюванням
            lat_str = f"{lat:>12}"
            lon_str = f"{lon:>12}"
            location_str = f', "location": {{"lat": {lat_str}, "lon": {lon_str}}}'
        else:
            location_str = ""

        # Додаємо рядок для цього елемента з вирівнюванням
        line = f'  {padded_key} : {{ "name": {padded_name}, "regionId": {region_id}, "legacyId": {legacy_id}, "stateId": {state_id}{location_str} }}{comma}'
        output_lines.append(line)

    output_lines.append("}")

    return "\n".join(output_lines)
    """
    Отримує координати для локації.

    Args:
        location_name: Назва локації

    Returns:
        (latitude, longitude)
    """

    # Якщо не знайдено у статичному мапінгу, використовуємо API
    coords = get_coordinates_from_nominatim(location_name)
    if coords:
        return coords

    # Якщо API не спрацював, повертаємо координати центру України
    print(f"Не вдалося знайти координати для '{location_name}', використовую координати центру України")
    return (49.0, 32.0)


def add_coordinates_to_json(input_file: str, output_file: str):
    """
    Додає географічні координати до кожного запису у JSON файлі.

    Args:
        input_file: Шлях до вхідного JSON файлу
        output_file: Шлях до вихідного JSON файлу
    """
    print(f"Читаю файл: {input_file}")

    with open(input_file, "r", encoding="utf-8") as f:
        data = json.load(f)

    total_locations = len(data)
    processed = 0

    print(f"Обробляю {total_locations} локацій...")

    for key, location_data in data.items():
        location_name = location_data.get("name", "")

        print(f"[{processed + 1}/{total_locations}] Обробляю: {location_name}")

        # Отримуємо координати
        lat, lon = get_coordinates(location_name)

        # Додаємо координати до даних (використовуємо lat/lon замість latitude/longitude)
        location_data["location"] = {"lat": lat, "lon": lon}

        processed += 1

        # Затримка між запитами до API для ввічливого використання (крім "Вся Україна")
        if location_name != "Вся Україна":
            print(f"  ⏳ Чекаю 1 секунду перед наступним запитом...")
            time.sleep(1.0)  # 1 секунда - рекомендована затримка для Nominatim API

    print(f"Зберігаю результат у файл: {output_file}")
    print("Форматую JSON з правильним вирівнюванням...")

    # Використовуємо нашу функцію форматування замість стандартного json.dump
    formatted_json = format_json_data(data)

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(formatted_json)

    print("Готово!")


if __name__ == "__main__":
    input_file = "gen_data.json"
    output_file = "gen_data_with_locations.json"

    add_coordinates_to_json(input_file, output_file)
