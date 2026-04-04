# Довідник по UI контролам

Цей документ описує **реальний** механізм UI-контролів у прошивці JAAM Fusion: що лежить у `web/`, які JSON ендпоінти віддає прошивка, як фронтенд підставляє поточні значення і як параметри зберігаються в налаштування.

## Зміст

- [Як працює UI schema](#як-працює-ui-schema)
- [Типи контролів і їх формат](#типи-контролів-і-їх-формат)
- [Покроково: додати або змінити контрол](#покроково-додати-або-змінити-контрол)
- [Dropdown списки](#dropdown-списки)
- [Умови видимості (visibility)](#умови-видимості-visibility)
- [Поширені помилки](#поширені-помилки)
- [Де почитати більше](#де-почитати-більше)

## Як працює UI schema {#як-працює-ui-schema}

Веб-інтерфейс рендериться динамічно на клієнті на основі набору JSON-ресурсів ("UI schema"):

- `GET /ui-schema/models` — моделі полів для кожного `type` (див. `web/ui_schema_models.json`).
- `GET /ui-schema/sections` — секції/вкладки UI (див. `web/ui_schema_sections.json`).
- `GET /ui-schema/dropdown_lists` — списки опцій для dropdown (генерується в `src/JaamWeb.cpp`).
- `GET /ui-schema/controls` — список контролів без поточних значень (джерело: `web/controls.json`).
- `GET /ui-schema/controls/values` — поточні значення параметрів у форматі `[[name, value], ...]` (генерується в `src/JaamWeb.cpp` з `ALL_PARAM_MAPPINGS`).

На клієнті (`web/scripts.js`) відбувається:

1. Паралельне завантаження ресурсів (schema).
2. Об'єднання `controls` зі `values` у `mergeControlsWithValues()`.
3. Рендер кожного контролу в DOM.

### Чому `controls.json` не містить `current`

У `web/controls.json` **немає** поля `current` (поточне значення). Його підставляє фронтенд, використовуючи модель з `web/ui_schema_models.json`:

- модель визначає порядок полів (включно з `current`),
- `mergeControlsWithValues()` знаходить `name`, шукає значення у `/ui-schema/controls/values`,
- і вставляє `current` у потрібну позицію.

Висновок: якщо контрол має `name` і модель містить `current`, то для нього **обов'язково** має існувати запис у `ALL_PARAM_MAPPINGS`, інакше рендер контролу зіб'ється.

## Типи контролів і їх формат {#типи-контролів-і-їх-формат}

Офіційне джерело формату — `web/ui_schema_models.json`. Нижче — практичний довідник: як контрол виглядає саме у `web/controls.json` (тобто **без** `current`).

### Параметричні контроли (мають `name`, потребують `ALL_PARAM_MAPPINGS`)

- `dropdown` — `controls.json`: `["dropdown", name, label, list, section, visibility]`; відправляє зміни на `POST /parameter`.
- `dropdown_confirm` — `controls.json`: `["dropdown_confirm", name, label, list, section, visibility]`; відправляє зміни на `POST /parameter` після підтвердження.
- `bool` — `controls.json`: `["bool", name, label, section, visibility]`; відправляє `value=1` або `value=0` на `POST /parameter`.
- `text` — `controls.json`: `["text", name, label, placeholder, section, visibility]`; відправляє на `POST /text`.
- `color` — `controls.json`: `["color", name, label, section, visibility]`; відправляє на `POST /color` значення `#RRGGBB`.
- `slider` — `controls.json`: `["slider", name, label, min, max, step, section, unit, visibility]`; відправляє на `POST /parameter` числове значення.

### Інформаційні контроли (не мають `current`, не потребують `ALL_PARAM_MAPPINGS`)

- `button` — `controls.json`: `["button", name, label, color, url, section, visibility]`; навігаційна кнопка (перехід на сторінку/редактор), нічого не зберігає.
- `label` — `controls.json`: `["label", text, section, visibility]`; просто заголовок/пояснювальний рядок.
- `info` — `controls.json`: `["info", text, color, icon, section, visibility]`; інфо-панель, `icon` — це `path` для SVG (Material Design Icons).

## Покроково: додати або змінити контрол {#покроково-додати-або-змінити-контрол}

### Крок 1. Додайте або змініть контрол у `web/controls.json`

Приклад (перемикач):

```json
[
    "bool",
    "api_enabled",
    "Home Assistant",
    "network",
    ""
]
```

Поля `section` і `visibility` мають відповідати секціям і правилам, описаним нижче.

### Крок 2. Додайте маппінг у `ALL_PARAM_MAPPINGS` (для параметричних контролів)

`ALL_PARAM_MAPPINGS` живе в `src/JaamWeb.cpp` і є джерелом істини для двох речей:

1. Які значення потраплять у `/ui-schema/controls/values`.
2. Які параметри приймають `POST /parameter`, `POST /text`, `POST /color`.

Формат:

```cpp
{"api_enabled", API_ENABLED, TYPE_BOOL},
```

де:

- перше поле (`"api_enabled"`) — це `name` з `controls.json`.
- друге поле (`API_ENABLED`) — це елемент `enum Type` з `src/JaamConfig.h`.
- третє поле (`TYPE_BOOL`) — тип даних для збереження/читання.

### Крок 3. Переконайтесь, що налаштування існує

Якщо ви додаєте **новий** параметр (а не використовуєте існуючий), його треба завести в типах налаштувань і в збереженні.

Мінімальний перелік місць, які зазвичай доводиться чіпати:

- `src/JaamConfig.h`: додати значення в `enum Type`.
- `src/JaamSettings.*`: додати підтримку збереження/дефолтів/валідації (залежить від типу).
- `src/JaamWeb.cpp`: додати в `ALL_PARAM_MAPPINGS`.

### Крок 4. Якщо це dropdown — додайте список у `/ui-schema/dropdown_lists`

Деталі в розділі [Dropdown списки](#dropdown-списки).

### Крок 5. Зберіть прошивку

`web/*.json` та інші веб-асети збираються у `src/web_assets.h` під час PlatformIO build (pre-script `tools/pre_build.py`).

Команда для перевірки:

```bash
platformio run
```

Не редагуйте `src/web_assets.h` вручну — це згенерований файл.

## Dropdown списки {#dropdown-списки}

Dropdown використовує `list`-ключ, який має існувати у відповіді `GET /ui-schema/dropdown_lists`.

### Як формується список

У `src/JaamWeb.cpp` є хелпер `appendOptionsList()`, який додає в JSON компактні елементи у форматі:

```text
[id, name, sub, disabled]
```

де:

- `id` — числовий ідентифікатор (те, що буде збережене як значення).
- `name` — текст, який показується користувачу.
- `sub` — `0/1` (в UI відображається як "-- name", якщо `1`).
- `disabled` — `0/1` (опція буде disabled).

### Джерело елементів: `SettingListItem`

У `src/JaamConfig.h` є структура:

```cpp
struct SettingListItem {
    int id;
    const char* name;
    bool ignore;
    bool sub;
    bool showDisabled;
};
```

Правила, які реально застосовуються при серіалізації списку:

- якщо `ignore == true` і `showDisabled == false` — елемент не потрапляє в список взагалі;
- якщо `showDisabled == true` — елемент потрапляє в список з `disabled = 1`.

### Як додати новий список

1. Додайте масив `SettingListItem` (зазвичай у `src/JaamConfig.cpp` + `extern` у `src/JaamConfig.h`).
2. У `JaamWeb::buildUiSchemaDropdownLists()` додайте заповнення JSON:

    ```cpp
    {
        JsonArray arr = dropdownLists["my_list"].to<JsonArray>();
        appendOptionsList(arr, MY_LIST, MY_LIST_COUNT);
    }
    ```

3. У `web/controls.json` використайте `"my_list"` у відповідному dropdown:

```json
["dropdown", "my_param", "Мій параметр", "my_list", "general", ""]
```

## Умови видимості (visibility) {#умови-видимості-visibility}

`visibility` — це рядок у контролі, який фронтенд читає з атрибута `data-visibility` і на його основі додає/прибирає CSS-клас `hidden`.

### Базовий синтаксис

Одна умова:

```text
field==N
field!=N
```

Важливі обмеження, які випливають з реалізації у `web/scripts.js`:

- підтримуються тільки числа (`N` має бути цілим; рядки на кшталт `true` не працюють);
- `field` шукається як `document.getElementById(field)`;
- для checkbox поточне значення — це `1` або `0`.

Приклади (валідні):

```json
"hardware==5"
"api_enabled==1"
"api_enabled!=1"
""
```

### Кілька умов через кому

Умови розділяються комою:

```json
"hardware==5,hardware==6"
"hardware!=0,hardware!=3"
"hardware==5,api_enabled==1"
```

Логіка (як реалізовано):

- умови групуються за `field` і оператором (`==` окремо від `!=`);
- всередині групи `field==...` діє **OR** (достатньо, щоб спрацювала будь-яка);
- всередині групи `field!=...` діє **AND** (мають виконатися всі);
- різні групи між собою комбінуються через **AND**.

## Поширені помилки {#поширені-помилки}

### Контрол має `name`, але немає маппінга

Симптоми:

- UI ламається (поле `current` не підставляється і позиції зсуваються), або контрол відображається некоректно.

Рішення:

- додайте запис у `ALL_PARAM_MAPPINGS` у `src/JaamWeb.cpp`.

### `name` не збігається між `controls.json` і `ALL_PARAM_MAPPINGS`

Симптоми:

- значення не підтягується у `current`, зміни не зберігаються.

Рішення:

- зробіть `name` ідентичним (включно з регістром і підкресленнями).

### Неправильний `ValueType`

Симптоми:

- сервер відхиляє значення як невалідне, або зберігає не те.

Підказка:

- `dropdown`/`slider` зазвичай працюють як `TYPE_INT`.
- `bool` — `TYPE_BOOL`.
- `color` і більшість `text` — `TYPE_STRING`.

### Visibility використовує `true/false` або рядки

Не працює, бо вирази парсяться регуляркою, яка приймає тільки `-?\d+`.

Правильно: `api_enabled==1`.

### Dropdown порожній

Причини:

- ключ `list` у контролі не існує в `/ui-schema/dropdown_lists`;
- список серіалізується, але всі елементи відфільтровані через `ignore`.

### `text` для числового параметра

`text` відправляє значення на `POST /text`. Для `TYPE_INT`/`TYPE_FLOAT` там використовується нестрогий парсинг (`toInt()`, `toFloat()`), тому краще для чисел використовувати `slider` або `dropdown`.

### Неправильний формат `color`

`POST /color` приймає тільки `#RRGGBB` і валідує кожний hex-символ.

## Де почитати більше {#де-почитати-більше}

- [Web Assets](web-assets.md) — як веб-асети пакуються та кешуються.
- [Архітектура](architecture.md) — огляд модулів і ендпоінтів прошивки.
- [Збірка](building.md) — як зібрати прошивку з PlatformIO.
- [Material Design Icons](https://pictogrammers.com/library/mdi/) — іконки для `info` контролів.
