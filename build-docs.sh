#!/bin/bash

# Скрипт для білду документації

echo "🔨 Building JAAM Fusion Documentation"
echo "====================================="
echo ""

# Перевірка наявності MkDocs
if ! command -v mkdocs &> /dev/null; then
    echo "❌ MkDocs не встановлено."
    echo "📦 Встановлення..."
    pip3 install -r requirements.txt
    echo ""
fi

# Білд документації
echo "📖 Збірка документації..."
mkdocs build --clean --strict

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Документація успішно зібрана!"
    echo "📁 Результат в директорії: site/"
    echo ""
    echo "Для локального перегляду запустіть: ./serve-docs.sh"
else
    echo ""
    echo "❌ Помилка при збірці документації"
    exit 1
fi
