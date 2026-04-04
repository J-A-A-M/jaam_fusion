# Документація JAAM Fusion

<!-- markdownlint-disable MD033 -->

<div class="ukraine-badge" markdown>
[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](http://stand-with-ukraine.pp.ua/)
[![Russian Warship](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/RussianWarship.svg)](http://stand-with-ukraine.pp.ua/)
</div>

Ласкаво просимо до документації **JAAM Fusion 5.x** — прошивки для ESP32.
Вона перетворює LED‑стрічку (або іншу адресну індикацію) на **інформаційну мапу
України**, яку можна налаштувати через веб‑інтерфейс.

## Що таке JAAM Fusion

JAAM Fusion — це прошивка для ESP32, яка відображає стан/події по регіонах
України (на LED‑мапі), а також може показувати дані на опціональному дисплеї.

- **Повітряні тривоги** (відображення по регіонах).
- **Погода** (температура по регіонах).
- **Режим настільної лампи** (суцільне світіння обраним кольором).

Підтримується опціональний **OLED‑дисплей** (I2C), який може показувати час,
погоду та сервісну інформацію залежно від обраного режиму дисплею.

!!! warning "Функції в розробці"
    Режими **"Радіація"** та **"Стан електромережі"** описані в документації,
    але у поточній версії прошивки можуть бути **недоступні**.

## Ключові можливості

### Кастомний LED‑маппінг

- Можна налаштувати відповідність LED → регіони через веб‑редактор.
- Маппінг зберігається в пам'яті пристрою та використовується після
    перезавантаження.
- Є ліміти на кількість LED (див. розділ про LED‑маппінг).

### Веб‑інтерфейс налаштувань

- Налаштування параметрів у браузері без додаткових застосунків.
- Окремі сторінки‑редактори для LED‑мапи та фонових кольорів.

### Інтеграція з Home Assistant

- Локальна інтеграція через вбудований API (вмикається у налаштуваннях).

## Швидкий старт

1. [Встановлення](getting-started/installation.md) через [FLASHER](https://flasher.jaam.net.ua/).
2. [Перша конфігурація](getting-started/first-config.md) (Wi‑Fi та базові налаштування).
3. [LED Mapping](hardware/led-mapping.md) — налаштування мапи під вашу конфігурацію.

## Ресурси проєкту

- [FLASHER](https://flasher.jaam.net.ua/) — прошивка прямо з браузера.
- [Портал даних](http://jaam.net.ua) — джерело інформації.
- [Телеграм канал проєкту](https://t.me/jaam_project)
- [Чат проєкту](https://t.me/jaam_discussions)
- [Багтрекер](https://github.com/J-A-A-M/jaam_fusion/issues)
- [GitHub репозиторій](https://github.com/J-A-A-M/jaam_fusion)

## Підтримка

Якщо у вас виникли питання або проблеми:

1. [FAQ](reference/faq.md)
2. [Troubleshooting](reference/troubleshooting.md)
3. Чат: https://t.me/jaam_discussions
4. Issues: https://github.com/J-A-A-M/jaam_fusion/issues

## Навігація по документації

<div class="grid cards" markdown>

-   :material-rocket-launch:{ .lg .middle } __Початок роботи__

    ---

    Швидкий старт, встановлення та перша конфігурація

    [:octicons-arrow-right-24: Перейти](getting-started/quickstart.md)

-   :material-led-on:{ .lg .middle } __Обладнання__

    ---

    LED Mapping, OLED Дисплей, сенсори та підтримувані плати

    [:octicons-arrow-right-24: Перейти](hardware/led-mapping.md)

-   :material-feature-search:{ .lg .middle } __Функціонал__

    ---

    Тривоги, погода, режим лампи (інші режими можуть бути в розробці)

    [:octicons-arrow-right-24: Перейти](features/air-alerts.md)

-   :material-code-braces:{ .lg .middle } __Для розробників__

    ---

    Архітектура, UI контроли, збірка проєкту

    [:octicons-arrow-right-24: Перейти](development/architecture.md)

</div>

---

<div style="text-align: center; margin-top: 2em;">
    <p><strong>JAAM Project</strong></p>
</div>

<!-- markdownlint-enable MD033 -->
