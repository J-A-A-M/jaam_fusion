# Довідник з додавання UI контролів

## Зміст
- [Архітектура системи контролів](#архітектура-системи-контролів)
- [Типи контролів](#типи-контролів)
- [Покрокова інструкція](#покрокова-інструкція)
- [Деталі по типах контролів](#деталі-по-типах-контролів)
- [Приклади](#приклади)
- [Поширені помилки](#поширені-помилки)

---

## Архітектура системи контролів

Починаючи з версії 5.0, система UI контролів використовує **розділену архітектуру**:

### Статична частина (структура)
- **Файл**: `web/controls.json`
- **Зміст**: Структура контролів **без поля `current`** (значення)
- **Розмір**: 41,752 → 5,221 bytes (87.5% компресія)
- **Кешування**: ETag-based, завантажується один раз
- **Endpoint**: `/ui-schema/controls`

### Динамічна частина (значення)
- **Генерується**: Метод `buildUiSchemaControlsValues()` в `JaamWeb.cpp`
- **Зміст**: Масив `{values: [["name", value], ...]}`
- **Endpoint**: `/ui-schema/controls/values`
- **Завантаження**: Щоразу при оновленні сторінки

### Об'єднання на клієнті
- **Метод**: `mergeControlsWithValues()` в `scripts.js`
- **Паралельне завантаження**: `Promise.all()` для обох частин
- **Індексація**: Враховує поле `type` на позиції 0

---

## Типи контролів

Всі типи контролів визначені в `web/ui_schema_models.json`:

| Тип | Призначення | Поля |
|-----|-------------|------|
| `dropdown` | Випадаючий список | `[type, name, label, list, section, visibility]` |
| `dropdown_confirm` | Випадаючий список з підтвердженням | `[type, name, label, list, section, visibility]` |
| `bool` | Перемикач (так/ні) | `[type, name, label, section, visibility]` |
| `text` | Текстове поле | `[type, name, label, placeholder, section, visibility]` |
| `color` | Вибір кольору | `[type, name, label, section, visibility]` |
| `slider` | Повзунок | `[type, name, label, min, max, step, section, visibility]` |
| `button` | Кнопка-посилання | `[type, name, label, color, url, section, visibility]` |
| `label` | Текстова мітка | `[type, label, section, visibility]` |
| `info` | Інформаційне повідомлення | `[type, text, color, icon, section, visibility]` |

**Важливо**: В `controls.json` перше поле завжди `type`, але в моделях це поле **пропускається** (воно універсальне для всіх).

---

## Покрокова інструкція

### 1. Додати контрол у `web/controls.json`

**Приклад**: Додаємо новий bool контрол для увімкнення звуку

```json
{
    "controls": [
        // ... існуючі контроли
        [
            "bool",
            "sound_enabled",
            "Увімкнути звук",
            "general",
            ""
        ]
    ]
}
```

**Структура масиву**: `[type, name, label, section, visibility]`

- `type`: Тип контролу (з таблиці вище)
- `name`: Унікальна назва параметра (використовується в C++ коді)
- `label`: Текст мітки для користувача
- `section`: Назва секції (з `ui_schema_sections.json`)
- `visibility`: Умова видимості (порожній рядок = завжди видимий)

### 2. Додати ParamMapping у `src/JaamWeb.cpp`

Знайдіть масив `ALL_PARAM_MAPPINGS` (лінія ~62-254) і додайте новий запис:

```cpp
static const ParamMapping ALL_PARAM_MAPPINGS[] = {
    // ... існуючі параметри
    
    // Sound settings
    {"sound_enabled", SOUND_ENABLED, TYPE_BOOL},
    
    // ... інші параметри
};
```

**Структура**: `{name, settingType, valueType}`

- `name`: **Точна назва** з controls.json
- `settingType`: Enum з `JaamSettings.h` (тип налаштування)
- `valueType`: Тип даних (`TYPE_INT`, `TYPE_BOOL`, `TYPE_FLOAT`, `TYPE_STRING`, `TYPE_SPECIAL`)

### 3. Переконайтесь що тип існує в `JaamSettings.h`

Якщо `SOUND_ENABLED` ще не існує, додайте його в enum `Type`:

```cpp
enum Type : uint8_t {
    // ... існуючі типи
    SOUND_ENABLED = 123,  // Унікальний ID
    // ... інші типи
};
```

### 4. Додати валідацію (опціонально)

Якщо потрібна спеціальна валідація, додайте її в `JaamSettings.cpp`:

```cpp
bool JaamSettings::saveBool(Type type, bool value) {
    switch(type) {
        case SOUND_ENABLED:
            // Спеціальна логіка, якщо потрібна
            return save(type, value ? 1 : 0);
        // ... інші типи
    }
}
```

### 5. Якщо це dropdown - додати список

Для контролів типу `dropdown` потрібно додати список опцій у метод `buildUiSchemaDropdownLists()`:

```cpp
void JaamWeb::buildUiSchemaDropdownLists(JsonDocument& doc) {
    JsonObject dropdownLists = doc["dropdown_lists"].to<JsonObject>();
    
    // ... існуючі списки
    
    {
        JsonArray arr = dropdownLists["sound_modes"].to<JsonArray>();
        appendOptionsList(arr, SOUND_MODES, SOUND_MODES_COUNT);
    }
}
```

Та визначити масив опцій (зазвичай у `JaamConfig.h`):

```cpp
const Option SOUND_MODES[] = {
    {"off", "Вимкнено", 0},
    {"melody", "Мелодія", 0},
    {"beep", "Сигнал", 0}
};
const int SOUND_MODES_COUNT = sizeof(SOUND_MODES) / sizeof(SOUND_MODES[0]);
```

### 6. Компіляція

Після внесення змін:

```bash
platformio run
```

Скрипт `tools/compress_assets.py` автоматично:
1. Стиснув `controls.json` → `ui_schema_controls_json_gz[]`
2. Згенерує `src/web_assets.h`
3. Створить новий hash для кешування

---

## Деталі по типах контролів

### Dropdown (випадаючий список)

**Структура в controls.json**:
```json
["dropdown", "hardware", "Режим прошивки", "hardware", "general", ""]
```

**Поля**: `[type, name, label, list, section, visibility]`

- `list`: Назва списку (з `buildUiSchemaDropdownLists`)

**ParamMapping**:
```cpp
{"hardware", HARDWARE, TYPE_INT}
```

**Dropdown список**:
```cpp
{
    JsonArray arr = dropdownLists["hardware"].to<JsonArray>();
    appendOptionsList(arr, HARDWARE_OPTIONS, HARDWARE_OPTIONS_COUNT);
}
```

**Масив опцій**:
```cpp
const Option HARDWARE_OPTIONS[] = {
    {"0", "Немає підсвітки (тільки дисплей)", 0},
    {"1", "Кільце LED 12", 0},
    {"2", "Кільце LED 16", 0}
};
const int HARDWARE_OPTIONS_COUNT = 3;
```

---

### Bool (перемикач)

**Структура в controls.json**:
```json
["bool", "min_of_silence", "Хвилина мовчання", "general", ""]
```

**Поля**: `[type, name, label, section, visibility]`

**ParamMapping**:
```cpp
{"min_of_silence", MIN_OF_SILENCE, TYPE_BOOL}
```

**Збереження**: Автоматично через `handleParameter()` → `settings->saveBool()`

---

### Text (текстове поле)

**Структура в controls.json**:
```json
["text", "device_name", "Назва пристрою", "JAAM", "general", ""]
```

**Поля**: `[type, name, label, placeholder, section, visibility]`

- `placeholder`: Підказка при порожньому полі

**ParamMapping**:
```cpp
{"device_name", DEVICE_NAME, TYPE_STRING}
```

**Збереження**: Через `handleTextParameter()` → `settings->saveString()`

---

### Color (вибір кольору)

**Структура в controls.json**:
```json
["color", "color_kabs", "Колір КАБС", "brightness", "hardware==5"]
```

**Поля**: `[type, name, label, section, visibility]`

**ParamMapping**:
```cpp
{"color_kabs", COLOR_KABS, TYPE_STRING}
```

**Формат значення**: `#RRGGBB` (7 символів з решіткою)

**Валідація**: Автоматична в `handleColorParameter()`
- Перевіряє формат `#RRGGBB`
- Валідує hex символи

---

### Slider (повзунок)

**Структура в controls.json**:
```json
["slider", "display_brightness", "Яскравість дисплею", "0", "100", "1", "brightness", ""]
```

**Поля**: `[type, name, label, min, max, step, section, visibility]`

- `min`: Мінімальне значення
- `max`: Максимальне значення
- `step`: Крок зміни

**ParamMapping**:
```cpp
{"display_brightness", DISPLAY_BRIGHTNESS, TYPE_INT}
```

**Збереження**: Автоматично через `handleParameter()` → `settings->saveInt()`

---

### Button (кнопка-посилання)

**Структура в controls.json**:
```json
["button", "map_editor", "Редактор мапи", "#007bff", "/map-editor", "general", "hardware==5"]
```

**Поля**: `[type, name, label, color, url, section, visibility]`

- `color`: Колір кнопки (hex або Bootstrap колір)
- `url`: URL для переходу

**ParamMapping**: Не потрібен (тільки UI елемент)

---

### Label (текстова мітка)

**Структура в controls.json**:
```json
["label", "Налаштування яскравості", "brightness", ""]
```

**Поля**: `[type, label, section, visibility]`

**ParamMapping**: Не потрібен (тільки UI елемент)

---

### Info (інформаційне повідомлення)

**Структура в controls.json**:
```json
[
    "info",
    "Оберіть режим прошивки відповідно до вашої версії пристрою",
    "#007bff",
    "M13,9H11V7H13M13,17H11V11H13M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z",
    "general",
    ""
]
```

**Поля**: `[type, text, color, icon, section, visibility]`

- `text`: Текст повідомлення
- `color`: Колір іконки
- `icon`: SVG path для Material Design Icons

**ParamMapping**: Не потрібен (тільки UI елемент)

---

## Приклади

### Приклад 1: Додати bool контрол

**Мета**: Додати перемикач для увімкнення/вимкнення логування

**1. controls.json**:
```json
[
    "bool",
    "logging_enabled",
    "Увімкнути логування",
    "general",
    ""
]
```

**2. JaamWeb.cpp** (ALL_PARAM_MAPPINGS):
```cpp
{"logging_enabled", LOGGING_ENABLED, TYPE_BOOL},
```

**3. JaamSettings.h** (enum Type):
```cpp
LOGGING_ENABLED = 150,
```

**Готово!** Решта обробляється автоматично.

---

### Приклад 2: Додати dropdown контрол

**Мета**: Додати вибір рівня логування

**1. controls.json**:
```json
[
    "dropdown",
    "log_level",
    "Рівень логування",
    "log_levels",
    "general",
    "logging_enabled==true"
]
```

**2. JaamWeb.cpp** (ALL_PARAM_MAPPINGS):
```cpp
{"log_level", LOG_LEVEL, TYPE_INT},
```

**3. JaamConfig.h** (масив опцій):
```cpp
const Option LOG_LEVELS[] = {
    {"0", "ERROR", 0},
    {"1", "WARN", 0},
    {"2", "INFO", 0},
    {"3", "DEBUG", 0}
};
const int LOG_LEVELS_COUNT = 4;
```

**4. JaamWeb.cpp** (buildUiSchemaDropdownLists):
```cpp
{
    JsonArray arr = dropdownLists["log_levels"].to<JsonArray>();
    appendOptionsList(arr, LOG_LEVELS, LOG_LEVELS_COUNT);
}
```

**5. JaamSettings.h** (enum Type):
```cpp
LOG_LEVEL = 151,
```

---

### Приклад 3: Додати slider контрол

**Мета**: Додати повзунок для вибору часу затримки

**1. controls.json**:
```json
[
    "slider",
    "animation_delay",
    "Затримка анімації (мс)",
    "0",
    "5000",
    "100",
    "animations",
    ""
]
```

**2. JaamWeb.cpp** (ALL_PARAM_MAPPINGS):
```cpp
{"animation_delay", ANIMATION_DELAY, TYPE_INT},
```

**3. JaamSettings.h** (enum Type):
```cpp
ANIMATION_DELAY = 152,
```

---

### Приклад 4: Додати text контрол

**Мета**: Додати поле для NTP хоста

**1. controls.json**:
```json
[
    "text",
    "ntp_host",
    "NTP сервер",
    "time.google.com",
    "network",
    ""
]
```

**2. JaamWeb.cpp** (ALL_PARAM_MAPPINGS):
```cpp
{"ntp_host", NTP_HOST, TYPE_STRING},
```

**3. JaamSettings.h** (enum Type):
```cpp
NTP_HOST = 153,
```

---

### Приклад 5: Додати color контрол

**Мета**: Додати вибір кольору для тривоги

**1. controls.json**:
```json
[
    "color",
    "alert_color",
    "Колір тривоги",
    "animations",
    ""
]
```

**2. JaamWeb.cpp** (ALL_PARAM_MAPPINGS):
```cpp
{"alert_color", ALERT_COLOR, TYPE_STRING},
```

**3. JaamSettings.h** (enum Type):
```cpp
ALERT_COLOR = 154,
```

**Примітка**: Колір зберігається як рядок у форматі `#RRGGBB`

---

## Умови видимості (visibility)

Поле `visibility` дозволяє показувати/приховувати контроли залежно від інших значень.

### Синтаксис

```
параметр оператор значення
```

**Оператори**:
- `==` - дорівнює
- `!=` - не дорівнює

**Приклади**:

```json
"hardware==5"          // Показати тільки якщо hardware дорівнює 5
"hardware!=0"          // Показати якщо hardware НЕ дорівнює 0
"logging_enabled==true" // Показати якщо logging увімкнено
""                     // Завжди показувати (порожній рядок)
```

### Складні умови

Для складних умов використовуйте кілька контролів:

```json
// Показати якщо (hardware == 5) АБО (hardware == 6)
// Спосіб: створити два контроли з різними visibility
["slider", "led_count", "Кількість LED", "0", "100", "1", "hardware", "hardware==5"],
["slider", "led_count", "Кількість LED", "0", "100", "1", "hardware", "hardware==6"]
```

**Примітка**: AND логіка не підтримується. Для складних умов потрібно модифікувати `scripts.js`.

---

## Секції (sections)

Контроли групуються по секціях, визначених у `web/ui_schema_sections.json`:

```json
{
  "sections": [
    {"name": "general", "label": "Загальні"},
    {"name": "network", "label": "Мережа"},
    {"name": "brightness", "label": "Яскравість"},
    {"name": "animations", "label": "Анімації"}
  ]
}
```

Щоб додати нову секцію:

**1. ui_schema_sections.json**:
```json
{"name": "security", "label": "Безпека"}
```

**2. Використовувати в controls.json**:
```json
["bool", "require_password", "Вимагати пароль", "security", ""]
```

---

## Поширені помилки

### 1. Назва в controls.json не збігається з ParamMapping

**Помилка**:
```json
// controls.json
["bool", "sound_enable", ...]
```

```cpp
// JaamWeb.cpp
{"sound_enabled", SOUND_ENABLED, TYPE_BOOL}  // ❌ Різні назви!
```

**Рішення**: Переконайтесь що `name` **точно збігається**:
```cpp
{"sound_enable", SOUND_ENABLED, TYPE_BOOL}  // ✅ Однакові назви
```

---

### 2. Забули додати ParamMapping

**Симптоми**:
- Контрол відображається, але значення не завантажується
- Помилка в консолі: "Unknown parameter"

**Рішення**: Додайте запис до `ALL_PARAM_MAPPINGS[]`

---

### 3. Неправильний ValueType

**Помилка**:
```cpp
{"display_brightness", DISPLAY_BRIGHTNESS, TYPE_STRING}  // ❌ Це число!
```

**Рішення**: Використовуйте правильний тип:
```cpp
{"display_brightness", DISPLAY_BRIGHTNESS, TYPE_INT}  // ✅
```

**Відповідність типів**:
- int параметри → `TYPE_INT`
- bool параметри → `TYPE_BOOL`
- float параметри → `TYPE_FLOAT`
- string параметри → `TYPE_STRING`
- спеціальні випадки → `TYPE_SPECIAL`

---

### 4. Забули додати dropdown список

**Симптоми**:
- Dropdown контрол порожній
- Немає опцій для вибору

**Рішення**:
1. Створити масив опцій у `JaamConfig.h`
2. Додати обробку в `buildUiSchemaDropdownLists()`

---

### 5. Неправильна структура масиву в controls.json

**Помилка**:
```json
["bool", "sound_enabled", "Увімкнути звук"]  // ❌ Бракує section і visibility!
```

**Рішення**: Використовуйте правильну кількість полів згідно з моделлю:
```json
["bool", "sound_enabled", "Увімкнути звук", "general", ""]  // ✅
```

**Перевірте ui_schema_models.json** для точної структури кожного типу.

---

### 6. Не перекомпілювали після зміни controls.json

**Симптоми**:
- Зміни не відображаються в браузері
- Старі контроли все ще присутні

**Рішення**:
```bash
platformio run
```

Процес компіляції автоматично:
1. Запустить `tools/compress_assets.py`
2. Стиснув `web/controls.json`
3. Згенерує новий `src/web_assets.h`
4. Створить новий hash для кешування

---

### 7. Браузер кешував старий controls.json

**Симптоми**:
- Файл перекомпільований, але зміни не відображаються
- Старий вміст в браузері

**Рішення**:
1. Оновіть сторінку з очищенням кешу: `Cmd+Shift+R` (Mac) або `Ctrl+Shift+R` (Windows/Linux)
2. Або відкрийте DevTools → Application → Clear storage → Clear site data

**Примітка**: Hash-based ETag автоматично інвалідує кеш при зміні файлу.

---

### 8. Неправильна видимість умови

**Помилка**:
```json
"visibility": "hardware=5"  // ❌ Один знак =
```

**Рішення**: Використовуйте подвійний знак:
```json
"visibility": "hardware==5"  // ✅
```

---

## Контрольний чеклист

При додаванні нового контролу:

- [ ] Додано запис у `web/controls.json` з **правильною структурою** для типу
- [ ] Додано `ParamMapping` у `ALL_PARAM_MAPPINGS[]` в `JaamWeb.cpp`
- [ ] Назва параметра **точно збігається** в обох місцях
- [ ] `ValueType` відповідає типу даних
- [ ] Додано `Type` enum у `JaamSettings.h` (якщо потрібно)
- [ ] Для dropdown: створено масив опцій і додано в `buildUiSchemaDropdownLists()`
- [ ] Секція існує в `ui_schema_sections.json`
- [ ] Умова видимості правильна (якщо використовується)
- [ ] Виконано `platformio run` для компіляції
- [ ] Протестовано в браузері з очищенням кешу

---

## Додаткові ресурси

- [WEB_ASSETS_README.md](WEB_ASSETS_README.md) - Інформація про компресію та кешування
- [Material Design Icons](https://pictogrammers.com/library/mdi/) - Іконки для info контролів
- `web/scripts.js` - Клієнтська логіка обробки контролів
- `src/JaamSettings.cpp` - Валідація та збереження налаштувань

---

## Структура проекту (контроли)

```
jaam_fusion/
├── web/
│   ├── controls.json              # Структура контролів (БЕЗ значень)
│   ├── ui_schema_models.json      # Визначення типів контролів
│   └── ui_schema_sections.json    # Секції для групування
├── src/
│   ├── JaamWeb.cpp                # HTTP endpoints та обробка
│   │   ├── ALL_PARAM_MAPPINGS[]   # Маппінг параметрів (лінія ~62-254)
│   │   ├── buildUiSchemaDropdownLists() # Генерація dropdown списків
│   │   ├── buildUiSchemaControlsValues() # Генерація значень
│   │   ├── handleParameter()      # Обробка int/bool/float параметрів
│   │   ├── handleTextParameter()  # Обробка text параметрів
│   │   └── handleColorParameter() # Обробка color параметрів
│   ├── JaamSettings.h             # enum Type (типи налаштувань)
│   ├── JaamSettings.cpp           # Валідація та збереження
│   └── JaamConfig.h               # Константи та масиви опцій
└── tools/
    └── compress_assets.py         # Pre-build компресія controls.json
```

---

**Версія документу**: 1.0  
**Дата оновлення**: 5 березня 2026  
**Сумісність**: JAAM Fusion v5.0+
