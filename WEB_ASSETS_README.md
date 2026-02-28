# Web Assets Compression

## Загальна інформація

Цей проект використовує систему pre-build компресії для веб-ресурсів (CSS та JavaScript), що дозволяє зменшити розмір файлів на **74%** перед їх вбудовуванням у firmware ESP32.

## Результати компресії

### Основні файли
- **styles.css**: 10,169 bytes → 2,072 bytes (**79.6%** reduction)
- **scripts.js**: 39,071 bytes → 9,749 bytes (**75.0%** reduction)

### Page-specific файли
- **map_editor.css**: 1,119 bytes → 437 bytes (**60.9%** reduction)
- **map_editor.js**: 2,379 bytes → 912 bytes (**61.7%** reduction)
- **bg_color_editor.css**: 1,712 bytes → 621 bytes (**63.7%** reduction)
- **bg_color_editor.js**: 3,113 bytes → 1,175 bytes (**62.3%** reduction)

### Загальна статистика
- **Разом**: 57,563 bytes → 14,966 bytes (**74.0%** reduction)
- **Економія**: 42,597 bytes

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
│   └── bg_color_editor.js       # JavaScript для редактора кольорів
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
- При першому завантаженні: повні 15.0KB
- При повторних візитах: 0 bytes (304 Not Modified)

### PROGMEM

Compressed arrays зберігаються в Flash пам'яті ESP32 (`PROGMEM`), що економить RAM.

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
- ✅ Розмір: **~15KB** (74% економії)
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
       "new_file": os.path.join(WEB_DIR, "new_file.txt"),  # Нове
   }
   ```
3. Створіть handler метод у JaamWeb.cpp (наприклад `handleNewFile()`)
4. Зареєструйте route у `begin()`: `server.on("/new-file", HTTP_GET, ...)`
5. Підключіть у HTML: `<link>` для CSS або `<script src="">` для JS
