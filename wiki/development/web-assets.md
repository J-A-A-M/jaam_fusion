# Web Assets Compression

## Загальна інформація

Цей проект використовує систему pre-build компресії для веб-ресурсів (CSS та JavaScript), що дозволяє зменшити розмір файлів на **74%** перед їх вбудовуванням у firmware ESP32.

## Результати компресії

### Основні файли
- **styles.css**: 15,048 bytes → 2,839 bytes (**81.1%** reduction)
- **scripts.js**: 55,817 bytes → 13,867 bytes (**75.2%** reduction)

### Page-specific файли
- **map_editor.css**: 1,236 bytes → 490 bytes (**60.4%** reduction)
- **map_editor.js**: 2,456 bytes → 952 bytes (**61.2%** reduction)
- **bg_color_editor.css**: 1,712 bytes → 621 bytes (**63.7%** reduction)
- **bg_color_editor.js**: 4,010 bytes → 1,463 bytes (**63.5%** reduction)

### UI Schema (JSON)
- **ui_schema_models.json**: 705 bytes → 207 bytes (**70.6%** reduction)
- **ui_schema_sections.json**: 789 bytes → 379 bytes (**52.0%** reduction)
- **controls.json**: 41,752 bytes → 5,221 bytes (**87.5%** reduction)

### Загальна статистика
- **Разом**: 123,525 bytes → 26,039 bytes (**78.9%** reduction)
- **Економія**: 97,486 bytes

**Примітка**: Окрім статичних файлів, всі динамічні JSON та HTML відповіді також стискаються в реальному часі (див. розділ "Динамічне стиснення відповідей").

## Архітектура

### Структура файлів

```text
jaam_fusion/
├── web/                          # Вихідні веб-ресурси
│   ├── styles.css               # Основні CSS стилі
│   ├── scripts.js               # Основний JavaScript код
│   ├── map_editor.css           # CSS для редактора карти
│   ├── map_editor.js            # JavaScript для редактора карти
│   ├── bg_color_editor.css      # CSS для редактора кольорів
│   ├── bg_color_editor.js       # JavaScript для редактора кольорів
│   ├── ui_schema_models.json    # UI моделі (статичні)
│   ├── ui_schema_sections.json  # UI секції (статичні)
│   └── controls.json            # UI контроли без значень (статичні)
├── tools/
│   └── compress_assets.py       # PlatformIO pre-build script
└── src/
    ├── web_assets.h             # Auto-generated (GZIP compressed arrays)
    └── JaamWeb.cpp              # Використовує compressed assets
```

### Як це працює

1. **Вихідні файли**: CSS і JS зберігаються у папці `web/` як звичайні text файли
2. **Pre-build компресія**: Перед кожною збіркою PlatformIO запускає `tools/compress_assets.py`
3. **Генерація header**: Скрипт створює `src/web_assets.h` з GZIP-compressed binary arrays
4. **Вбудовування**: JaamWeb.cpp включає web_assets.h та відправляє compressed дані з `Content-Encoding: gzip`
5. **Браузер**: Автоматично розпаковує GZIP дані

## Використання

### Редагування веб-ресурсів

Просто редагуйте файли у папці `web/`:
- `web/styles.css` - основні CSS стилі
- `web/scripts.js` - основний JavaScript код
- `web/map_editor.css` - стилі редактора карти
- `web/map_editor.js` - логіка редактора карти
- `web/bg_color_editor.css` - стилі редактора кольорів
- `web/bg_color_editor.js` - логіка редактора кольорів
- `web/ui_schema_models.json` - моделі UI елементів (статичні)
- `web/ui_schema_sections.json` - секції налаштувань (статичні)
- `web/controls.json` - структура UI контролів без значень (статичні)

### Збірка проекту

```bash
# Автоматична компресія та збірка
platformio run

# Або для environment specific
platformio run -e firmware
```

Компресія відбувається автоматично перед кожною збіркою завдяки `extra_scripts = pre:tools/compress_assets.py` у `platformio.ini`.

## Технічні деталі

### Content-Encoding: gzip

Сервер відправляє заголовок `Content-Encoding: gzip`, що дозволяє браузеру автоматично розпаковувати дані. Це стандартний HTTP механізм.

### ETag для кешування

Кожен скомпонований файл має унікальний SHA256 hash, який використовується як ETag для HTTP кешування:
- При першому завантаженні: повні 15.1KB
- При повторних візитах: 0 bytes (304 Not Modified)

### PROGMEM

Compressed arrays зберігаються в Flash пам'яті ESP32 (`PROGMEM`), що економить RAM.

### Розділення статичних контролів і динамічних значень

Для максимальної оптимізації, UI контроли розділено на дві частини:

**Статична частина** (`/ui-schema/controls`):
- Файл `web/controls.json` містить **тільки структуру** контролів (без поля `current`)
- Pre-compressed при збірці (87.5% reduction)
- Кешується браузером через ETag (завантажується один раз)
- Розмір: 41,752 bytes → 5,221 bytes

**Динамічна частина** (`/ui-schema/controls/values`):
- Endpoint повертає масив значень `{values: [["name1", value1], ["name2", value2], ...]}`
- Стискається динамічно при кожному запиті
- Завантажується щоразу (актуальні значення з пам'яті пристрою)
- Об'єднується з контролями на клієнті через `mergeControlsWithValues()`

**Переваги**:
- ✅ **Економія трафіку**: Велика структура (41KB) завантажується тільки один раз
- ✅ **Швидке оновлення**: Значення (кілька KB) завантажуються швидко
- ✅ **Кешування**: 304 Not Modified для статичної частини
- ✅ **Паралельне завантаження**: Обидва запити через `Promise.all()`

## Динамічне стиснення відповідей

Окрім pre-compressed статичних файлів, сервер також стискає динамічні відповіді в реальному часі:

### JSON API Endpoints

**Більшість JSON API endpoints** стискаються динамічно через `ESP32-targz` бібліотеку:

- `/ui-schema/dropdown_lists` - списки для dropdown елементів
- `/ui-schema/controls/values` - динамічні значення для контролів (стискається ESP32-targz, має anti-cache headers)
- `/system-info` - системна інформація
- `/alerts-info` - дані про тривоги
- `/logs-info` - логи системи
- `/map-data` - дані для редактора мапи
- `/bg-colors-data` - дані кольорів фону

**Виняток**: `/ui-schema/controls` - pre-compressed при збірці та завантажується з `web_assets.h` (НЕ використовує ESP32-targz)

**Функція**: `sendCompressedJson()` використовує `LZPacker::compress()` для автоматичного gzip стиснення.

**Anti-caching**: `/ui-schema/controls/values` має заголовки `Cache-Control: no-store, no-cache, must-revalidate, max-age=0` для гарантії актуальних значень.

### HTML Pages

Динамічно згенеровані HTML сторінки також стискаються:

- `/` - головна UI сторінка
- `/map-editor` - редактор мапи
- `/bg-color-editor` - редактор кольорів фону

**Функція**: `sendCompressedHtml()` стискає великі HTML рядки перед відправкою.

### Переваги динамічного стиснення

- ✅ **Економія трафіку**: 60-80% зменшення розміру JSON/HTML відповідей
- ✅ **Швидше завантаження**: менше часу на передачу даних через WiFi
- ✅ **Менше навантаження**: на WiFi радіо та точку доступу
- ✅ **Автоматичне**: браузер автоматично розпаковує `Content-Encoding: gzip`
- ✅ **Fallback**: при помилці стиснення відправляється нестиснута версія

### Технічні деталі

**Бібліотека**: `tobozo/ESP32-targz@1.3.1`

**Метод стиснення**: `LZPacker::compress()` - високоефективний gzip компресор

**Приклад логування**:
```
[WEB] Compressed JSON: 3245 → 892 bytes (72.5% reduction)
[WEB] Compressed HTML: 8921 → 2134 bytes (76.1% reduction)
```

**Управління пам'яттю**: Стиснуті дані автоматично звільняються після відправки через `free(compressedBytes)`.

---

## Додаткові ресурси

- **[UI Контроли](controls-guide.md)** - Детальний довідник з додавання нових UI контролів
- [Material Design Icons](https://pictogrammers.com/library/mdi/) - Іконки для info контролів
- `web/ui_schema_models.json` - Визначення типів контролів
- `web/ui_schema_sections.json` - Секції для групування контролів

## Порівняння з попередніми підходами

### До змін
- ❌ CSS/JS embedded в R"HTML(...)" raw strings всередині методів
- ❌ Інлайн стилі та скрипти у кожному page handler
- ❌ Дублювання коду (saveBtn стилі у кожному редакторі)
- ❌ Розмір: ~58KB без компресії

### Після змін
- ✅ CSS/JS у окремих файлах (легше редагувати та підтримувати)
- ✅ GZIP compression під час build (без навантаження на ESP32)
- ✅ Pre-compressed binary arrays у PROGMEM
- ✅ Розмір статичних assets: **~16KB** (73.7% економії)
- ✅ Динамічне стиснення JSON/HTML через ESP32-targz (60-80% економії)
- ✅ Hash-based ETag для ефективного кешування
- ✅ Переиспользування коду між сторінками

## Підтримка браузерів

Всі сучасні браузери підтримують `Content-Encoding: gzip`:
- Chrome/Edge (всі версії)
- Firefox (всі версії)
- Safari (всі версії)
- Opera (всі версії)

## Troubleshooting

### Файл web_assets.h не генерується

Переконайтеся що PlatformIO правильно налаштований:
```bash
# Перевірте platformio.ini
grep extra_scripts platformio.ini
# Повинно бути: extra_scripts = pre:tools/compress_assets.py
```

### Помилка компіляції "web_assets.h not found"

Повністю перебудуйте проект:
```bash
platformio run -t clean && platformio run
```

### Зміни в CSS/JS не застосовуються

1. Очистіть браузерний кеш (Ctrl+Shift+R або Cmd+Shift+R)
2. Перебудуйте проект: `platformio run -t clean && platformio run`
3. Перевірте що змінився ETag hash у консолі браузера

## Розробка

Якщо потрібно додати нові веб-ресурси:

1. Створіть файл у папці `web/`
2. Додайте його в `ASSETS` dictionary у `tools/compress_assets.py`:
   ```python
   ASSETS = {
       "styles_css": os.path.join(WEB_DIR, "styles.css"),
       "scripts_js": os.path.join(WEB_DIR, "scripts.js"),
       "map_editor_css": os.path.join(WEB_DIR, "map_editor.css"),
       "map_editor_js": os.path.join(WEB_DIR, "map_editor.js"),
       "bg_color_editor_css": os.path.join(WEB_DIR, "bg_color_editor.css"),
       "bg_color_editor_js": os.path.join(WEB_DIR, "bg_color_editor.js"),
       "ui_schema_models_json": os.path.join(WEB_DIR, "ui_schema_models.json"),
       "ui_schema_sections_json": os.path.join(WEB_DIR, "ui_schema_sections.json"),
       "new_file": os.path.join(WEB_DIR, "new_file.txt"),  # Нове
   }
   ```
3. Створіть handler метод у JaamWeb.cpp (наприклад `handleNewFile()`)
4. Зареєструйте route у `begin()`: `server.on("/new-file", HTTP_GET, ...)`
5. Підключіть у HTML: `<link>` для CSS або `<script src="">` для JS
