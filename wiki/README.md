# Документація JAAM Fusion

Цей каталог містить вихідні файли документації для проєкту JAAM Fusion.

## Структура

```
docs/
├── index.md                    # Головна сторінка
├── getting-started/            # Початок роботи
│   ├── quickstart.md          # Швидкий старт
│   ├── installation.md        # Встановлення
│   └── first-config.md        # Перша конфігурація
├── hardware/                   # Обладнання
│   ├── led-mapping.md         # LED Mapping
│   ├── oled-display.md        # OLED Display
│   ├── sensors.md             # Сенсори
│   └── boards.md              # Підтримувані плати
├── features/                   # Функціонал
├── api/                        # API
├── web-interface/              # Веб-інтерфейс
├── development/                # Для розробників
├── reference/                  # Довідник
└── stylesheets/
    └── custom.css             # Кастомні стилі
```

## Локальна розробка

### Встановлення

```bash
# Встановити залежності
pip install -r requirements.txt
```

### Запуск локального сервера

```bash
mkdocs serve
```

Відкрийте http://127.0.0.1:8000 в браузері.

### Білд документації

```bash
mkdocs build
```

Результат буде в папці `site/`.

## Деплой

Документація автоматично деплоїться на GitHub Pages через GitHub Actions при будь-яких змінах в папці `docs/` або файлу `mkdocs.yml`.

URL: https://j-a-a-m.github.io/jaam_fusion/

## Правила написання

1. **Мова**: Документація пишеться українською мовою
2. **Markdown**: Використовуйте розширений синтаксис Material for MkDocs
3. **Структура**: Кожна сторінка має мати заголовок першого рівня (`#`)
4. **Посилання**: Використовуйте відносні посилання між сторінками
5. **Зображення**: Зберігайте в папці `docs/assets/images/`

## Корисні розширення

- **Admonitions**: `!!! note`, `!!! warning`, `!!! tip`
- **Code blocks**: З підсвічуванням синтаксису
- **Tabs**: Для організації контенту
- **Mermaid**: Для діаграм

Приклади: https://squidfunk.github.io/mkdocs-material/reference/

## Контакти

- Telegram: https://t.me/jaam_discussions
- GitHub: https://github.com/J-A-A-M/jaam_fusion
