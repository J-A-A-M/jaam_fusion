#!/bin/bash

# Скрипт для локального запуску документації MkDocs

echo "🚀 JAAM Fusion Documentation Server"
echo "===================================="
echo ""

# Перевірка наявності Python
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 не знайдено. Будь ласка, встановіть Python 3."
    exit 1
fi

# Перевірка наявності pip
if ! command -v pip3 &> /dev/null; then
    echo "❌ pip3 не знайдено. Будь ласка, встановіть pip."
    exit 1
fi

# Встановлення залежностей якщо потрібно
if ! python3 -c "import mkdocs" 2>/dev/null; then
    echo "📦 Встановлення залежностей..."
    pip3 install -r requirements.txt
    echo ""
fi

# Запуск сервера
echo "🌐 Запуск локального сервера..."
echo "📖 Документація буде доступна за адресою: http://127.0.0.1:8000"
echo ""
echo "⌨️  Натисніть Ctrl+C для зупинки"
echo ""

mkdocs serve
