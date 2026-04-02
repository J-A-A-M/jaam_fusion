# JAAM Fusion Documentation

<div class="ukraine-badge" markdown>
[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](http://stand-with-ukraine.pp.ua/)
[![Russian Warship](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/RussianWarship.svg)](http://stand-with-ukraine.pp.ua/)
</div>

Ласкаво просимо до документації **JAAM Fusion 5.x** - прошивки для ESP32, що дозволяє за допомогою розміщених на мапі України адресних світлодіодів відображати важливу інформацію в реальному часі.

## 🎯 Що таке JAAM Fusion?

JAAM Fusion - це прошивка для ESP32, яка перетворює звичайну LED-стрічку на інформаційну мапу України. Вона відображає:

- 🚨 **Повітряні тривоги** в реальному часі
- 🌡️ **Погоду** в регіонах України
- ☢️ **Радіаційне забруднення**
- ⚡ **Стан електромережі**
- 🇺🇦 **Візуальні зображення** (прапор України та інші)
- 💡 **Режим настільної лампи**

Крім цього, підтримується окремий **OLED-дисплей**, який відображає поточний час, погоду, радіацію, стан електромережі, глобальні сповіщення та сервісні повідомлення.

## 🌟 Ключові особливості

### Кастомний маппінг LED
**Революційна можливість** налаштування будь-якої апаратної конфігурації світлодіодів!

- **Повна свобода конфігурації**: Підтримка будь-якої кількості та розташування LED
- **Редактор карти LED**: Зручний web-інтерфейс для прив'язки LED до регіонів
- **Гнучкість**: Від простої стрічки до складних матричних конфігурацій
- **Збереження конфігурації**: Маппінг зберігається в пам'яті та відновлюється після перезавантаження

### Сучасна архітектура
- **Модульна структура коду**: Кожен компонент системи винесено в окремий клас
- **Асинхронна обробка**: Використання ArduinoAsync та FreeRTOS
- **Оптимізація пам'яті**: Покращене управління heap memory
- **WebSocket протокол**: Real-time оновлення через власний сервер

## 🚀 Швидкий старт

1. **[Завантажте прошивку](getting-started/installation.md)** через [FLASHER](https://flasher.jaam.net.ua/)
2. **[Налаштуйте WiFi](getting-started/first-config.md)** через веб-інтерфейс
3. **[Налаштуйте LED-карту](hardware/led-mapping.md)** відповідно до вашої конфігурації
4. Насолоджуйтесь!

## 📚 Ресурси проєкту

- [FLASHER](https://flasher.jaam.net.ua/) - прошивка прямо з браузера!
- [ПОРТАЛ ДАНИХ](http://jaam.net.ua) - джерело інформації
- [Телеграм канал проєкту](https://t.me/jaam_project)
- [Чат проєкту](https://t.me/jaam_discussions)
- [Багтрекер](https://github.com/J-A-A-M/jaam_fusion/issues)
- [GitHub репозиторій](https://github.com/J-A-A-M/jaam_fusion)

## 🛠️ Підтримка

Якщо у вас виникли питання або проблеми:

1. Перевірте розділ [FAQ](reference/faq.md)
2. Зверніться до [Troubleshooting](reference/troubleshooting.md)
3. Задайте питання в [чаті проєкту](https://t.me/jaam_discussions)
4. Створіть [issue на GitHub](https://github.com/J-A-A-M/jaam_fusion/issues)

## 📖 Навігація по документації

<div class="grid cards" markdown>

-   :material-rocket-launch:{ .lg .middle } __Початок роботи__

    ---

    Швидкий старт, встановлення та перша конфігурація

    [:octicons-arrow-right-24: Перейти](getting-started/quickstart.md)

-   :material-led-on:{ .lg .middle } __Обладнання__

    ---

    LED Mapping, OLED Display, сенсори та підтримувані плати

    [:octicons-arrow-right-24: Перейти](hardware/led-mapping.md)

-   :material-feature-search:{ .lg .middle } __Функціонал__

    ---

    Тривоги, погода, радіація, електромережа

    [:octicons-arrow-right-24: Перейти](features/air-alerts.md)

-   :material-code-braces:{ .lg .middle } __Для розробників__

    ---

    Архітектура, UI контроли, збірка проєкту

    [:octicons-arrow-right-24: Перейти](development/architecture.md)

</div>

---

<div style="text-align: center; margin-top: 2em;">
    <p><strong>Зроблено з ❤️ до України</strong></p>
    <p>© 2024-2026 JAAM Project</p>
</div>
