# Веб-асети: пакування, кешування і стиснення

Цей документ описує, як веб-інтерфейс JAAM Fusion пакується у прошивку:

- які файли вважаються "статичними веб-асетами";
- як вони стискаються перед збіркою і потрапляють у `src/web_assets.h`;
- як `JaamWeb` віддає ці ресурси з gzip та кешуванням;
- які відповіді стискаються вже під час роботи пристрою.

## Де лежать вихідні файли

Вихідні (редаговані людиною) ресурси лежать у `web/`.

На момент написання, у прошивку вбудовуються такі файли:

- `web/styles.css`
- `web/scripts.js`
- `web/map_editor.css`
- `web/map_editor.js`
- `web/bg_color_editor.css`
- `web/bg_color_editor.js`
- `web/ui_schema_models.json`
- `web/ui_schema_sections.json`
- `web/controls.json`

Список визначений у `tools/compress_assets.py` у словнику `ASSETS`.

## Pre-build пайплайн (PlatformIO)

Pre-build скрипт підключений у `platformio.ini` через:

```ini
extra_scripts = pre:tools/pre_build.py
```

Далі `tools/pre_build.py` запускає послідовно:

1. `tools/compress_assets.py` — стискає web-асети і генерує `src/web_assets.h`.
2. `tools/convert_region_map.py` — окремий pre-build крок (не про веб-асети).

### Що саме генерується

`tools/compress_assets.py` створює `src/web_assets.h`, де для кожного файлу є:

- gzip-масив у `PROGMEM`, наприклад `styles_css_gz`;
- довжина, наприклад `styles_css_gz_len`;
- хеш версії, наприклад `styles_css_hash`.

Важливо:

- gzip робиться з `mtime=0`, щоб результат був детермінований;
- хеш — це `sha256` від **стиснутих** байтів, обрізаний до 16 символів;
- `src/web_assets.h` — згенерований файл, його не редагують вручну.

Під час збірки скрипт друкує статистику (оригінал/стиснуте/хеш).
Ці цифри є джерелом істини для розміру та “економії”.

## Як прошивка віддає статичні асети

Статичні ресурси віддаються вже у стисненому (gzip) вигляді.
`JaamWeb` встановлює:

- `Content-Encoding: gzip`
- `Cache-Control: public, max-age=86400`
- `ETag: "<hash>"`

І якщо браузер надсилає `If-None-Match` з тим самим ETag,
прошивка повертає `304 Not Modified`.

Типові маршрути (HTTP GET):

- `/styles.css`
- `/scripts.js`
- `/map-editor.css`
- `/map-editor.js`
- `/bg-color-editor.css`
- `/bg-color-editor.js`
- `/ui-schema/models`
- `/ui-schema/sections`
- `/ui-schema/controls`

### Версії через `?v=`

Окрім ETag, головна UI-сторінка `/` підключає CSS/JS з query-параметром версії:

- `/styles.css?v=<styles_css_hash>`
- `/scripts.js?v=<scripts_js_hash>`

Так само в HTML інжектиться `window.JAAM_HASHES`, і фронтенд використовує
`?v=` для `GET /ui-schema/models`, `GET /ui-schema/sections`,
`GET /ui-schema/controls`.

## Динамічні дані та стиснення “на льоту”

Частина відповідей генерується динамічно й стискається вже під час запиту.
Для цього використовується `LZPacker::compress()` (бібліотека `ESP32-targz`).

### JSON

`sendCompressedJson()`:

- стискає відповідь у gzip;
- віддає з `Content-Encoding: gzip`;
- якщо стиснення не вдалося — віддає не стиснуту відповідь.

Приклади ендпоінтів, які віддаються як динамічний JSON:

- `/ui-schema/dropdown_lists`
- `/ui-schema/controls/values`
- `/system-info`
- `/alerts-info`
- `/logs-info`
- `/map-data`
- `/bg-colors-data`

Для `/ui-schema/controls/values` додатково виставляється “anti-cache”:

- `Cache-Control: no-store, no-cache, must-revalidate, max-age=0`
- `Pragma: no-cache`
- `Expires: 0`

### HTML

Деякі HTML-сторінки теж стискаються на льоту через `sendCompressedHtml()`:

- `/`
- `/map-editor`
- `/bg-color-editor`

Для `/` також виставляється заборона кешування,
щоб гарантовано підхоплювались нові версії ресурсів.

## Дрібниці про іконки

HTML містить посилання на `favicon.png` і `apple-touch-icon.png`.
Станом на поточну реалізацію:

- `GET /favicon.png` відповідає `204`;
- маршруту для `apple-touch-icon.png` не зареєстровано.

Якщо потрібні реальні іконки, їх треба додати як ресурс
(аналогічно CSS/JS) і зареєструвати handler у `JaamWeb::begin()`.

## Як додати новий статичний файл

1. Додайте файл у `web/`.
2. Додайте його в `ASSETS` у `tools/compress_assets.py`.
3. Додайте handler у `src/JaamWeb.cpp` за зразком `handleCss()`:
   gzip-віддача, `ETag`, `Cache-Control`, `If-None-Match`.
4. Зареєструйте route у `JaamWeb::begin()` через `server.on()`.
5. Підключіть ресурс у HTML/JS.

## Troubleshooting

### Зміни у `web/` не видно в браузері

1. Переконайтесь, що збірка виконалась (`platformio run`).
2. Зробіть hard reload у браузері.
3. За потреби зробіть clean build:

```bash
platformio run -t clean && platformio run
```

### Помилка `web_assets.h not found`

Це означає, що pre-build не згенерував файл.
Перевірте, що в `platformio.ini` підключений `tools/pre_build.py`.

## Додаткові ресурси

- [Довідник по UI контролам](controls-guide.md)
- [Архітектура](architecture.md)
