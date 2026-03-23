
#!/bin/bash
TAG_NAME="5.0-b9"
FULL_VERSION="5.0-b9"
VERSION="5.0"


# Витягуємо версію (без -bX)
BASE_VERSION="${VERSION}"

echo "📋 Аналізуємо release notes з GitHub API..."
echo "   Поточна версія: ${FULL_VERSION}"
echo "   Base версія: ${BASE_VERSION}"

# 1️⃣ Отримуємо всі releases з GitHub API (сортовані за датою, новіші спочатку)
RELEASES=$(curl -s -H "Authorization: token ${GITHUB_TOKEN}" \
    "https://api.github.com/repos/J-A-A-M/jaam_fusion/releases?per_page=100&sort=created&direction=desc")

# 2️⃣ Знаходимо останній pre-release beta для поточної версії
PREVIOUS_BETA_TAG=$(echo "$RELEASES" | jq -r ".[] | select(.prerelease == true and .tag_name != null and (.tag_name | startswith(\"${BASE_VERSION}-b\"))) | .tag_name" | head -1)

# 3️⃣ Знаходимо останній stable або попередній beta (не поточної версії)
LAST_STABLE_TAG=$(echo "$RELEASES" | jq -r ".[] | select(.tag_name != null and .tag_name != \"${TAG_NAME}\" and (.prerelease == false or (.tag_name | startswith(\"${BASE_VERSION}-b\") | not))) | .tag_name" | head -1)

echo "   Попередній beta: ${PREVIOUS_BETA_TAG:-'(відсутній - це перший beta)'}"
echo "   Останній stable/попередній: ${LAST_STABLE_TAG:-'(відсутній - перша версія)'}"

# Витягуємо набір комітів
NEW_COMMITS=""

# Перший розділ: нові коміти від попереднього beta поточної версії
if [ -n "$PREVIOUS_BETA_TAG" ]; then
    echo "   📝 Витягуємо нові коміти від ${PREVIOUS_BETA_TAG}..."
    NEW_COMMITS=$(git log --pretty=format:"- %s (%an, %h)" ${PREVIOUS_BETA_TAG}..HEAD 2>/dev/null)
else
    echo "   📝 Це перший beta білд, витягуємо від попереднього релізу..."
    if [ -n "$LAST_STABLE_TAG" ]; then
    NEW_COMMITS=$(git log --pretty=format:"- %s (%an, %h)" ${LAST_STABLE_TAG}..HEAD 2>/dev/null)
    else
    NEW_COMMITS=$(git log --pretty=format:"- %s (%an, %h)" HEAD | head -50)
    fi
fi

# Другий розділ: Release notes з попередніх beta релізів (якщо вони є) до останнього stable з GitHub API
if [ -n "$PREVIOUS_BETA_TAG" ] && [ -n "$LAST_STABLE_TAG" ]; then
    echo "   📝 Витягуємо release notes з попередніх beta релізів (відносно ${LAST_STABLE_TAG})..."
    # Отримуємо всі beta теги новіші за LAST_STABLE_TAG
    BETA_TAGS=$(git tag -l "${BASE_VERSION}-b*" | while read tag; do
    if git merge-base --is-ancestor "${LAST_STABLE_TAG}" "$tag" 2>/dev/null; then
        echo "$tag"
    fi
    done | sort -V)

    # Витягуємо Release Notes з кожного beta тегу (обмежуємо до 10: 2 найстаріших + 8 найновіших)
    PREVIOUS_RELEASE_NOTES=""
    BETA_TAGS_ARRAY=($BETA_TAGS)
    BETA_COUNT=${#BETA_TAGS_ARRAY[@]}

    if [ "$BETA_COUNT" -le 10 ]; then
    SELECTED_TAGS=("${BETA_TAGS_ARRAY[@]}")
    else
    OLDEST_TWO=("${BETA_TAGS_ARRAY[@]:0:2}")
    NEWEST_EIGHT=("${BETA_TAGS_ARRAY[@]: -8}")
    SELECTED_TAGS=("${OLDEST_TWO[@]}" "${NEWEST_EIGHT[@]}")
    echo "   ⚠️ Знайдено ${BETA_COUNT} beta тегів, обмежуємо до 10 (2 найстаріших + 8 найновіших)"
    fi

    for beta_tag in "${SELECTED_TAGS[@]}"; do
    echo "   📖 Витягуємо Release Notes з ${beta_tag}..."
    NOTES=$(echo "$RELEASES" | jq -r --arg tag "$beta_tag" '.[] | select(.tag_name == $tag) | .body // empty')
    echo "   📝 Витягнуті нотатки (${#NOTES} символів)"
    if [ -n "$NOTES" ]; then
        # Якщо у $NOTES є частина, що починається з "## 📋 Зміни з попередніх beta версій" — обрізаємо її
        NOTES=$(echo "$NOTES" | sed '/## 📋 Зміни з попередніх beta версій/,$d')
        echo "   📝 Обрізані нотатки до ${#NOTES} символів"
        PREVIOUS_RELEASE_NOTES+=$'\n'"### ${beta_tag}"$'\n'"${NOTES}"$'\n'
    fi
    done
fi

echo ""
echo "✅ Аналіз releases та комітів завершено"

echo "📝 Генеруємо changelog за допомогою AI..."

# Викликаємо Gemini API
if [ -n "$GEMINI_API_KEY" ]; then
    # Створюємо промпт для AI з двома розділами
    PROMPT="Ти - AI асистент для створення changelog для IoT пристрою \"JAAM Fusion\" - карти повітряних тривог України на базі ESP32.

    Проаналізуй наступні коміти та створи професійний changelog українською мовою у форматі Markdown для beta релізу. Врахуй changelog уже опублікованих попередніх beta релізів, щоб не дублювати інформацію.

    ВАЖЛИВО: Ігноруй технічні коміти, які не впливають на функціональність прошивки:
    - Зміни в GitHub Actions (.github/workflows)
    - Зміни конфігурації білда (якщо вони не додають нові функції користувачам)
    - Оновлення документації проекту
    - Зміни в CI/CD
    - Рефакторинг без нових функцій

    Включай тільки зміни, які впливають на роботу пристрою або додають нові можливості користувачам.

    НОВІ КОМІТИ (від попереднього beta до поточного):
    ${NEW_COMMITS}

    РЕЛІЗ НОТИ З ПОПЕРЕДНІХ BETA РЕЛІЗІВ (від попереднього релізу до попереднього beta, вже опубліковані раніше):
    ${PREVIOUS_RELEASE_NOTES:-}

    Створи changelog з такою структурою:

    # 🧪 Beta Реліз ${FULL_VERSION}

    > ⚠️ **Увага**: Це бета-версія для тестування. Може містити нестабільні функції.

    ## 🆕 Що нового у версії ${FULL_VERSION}

    [Витягни зміни з НОВИХ КОМІТІВ. Згрупуй за категоріями:]
    - 🚀 Нові можливості
    - 🐛 Виправлення помилок
    - 🔧 Технічні покращення
    - ⚡ Оптимізація
    - 🎨 UI/UX покращення

    ## 📋 Зміни з попередніх beta версій (відносно версії ${LAST_STABLE_TAG})

    [Якщо РЕЛІЗ НОТИ З ПОПЕРЕДНІХ BETA РЕЛІЗІВ пусті - пропусти цей розділ. Інакше витягни ТІЛЬКИ КЛЮЧОВІ зміни з РЕЛІЗ НОТ З ПОПЕРЕДНІХ BETA РЕЛІЗІВ, щоб розділ був менш деталізованим. Згрупуй їх за категоріями:]
    - 🚀 Нові можливості
    - 🐛 Виправлення помилок
    - 🔧 Технічні покращення
    - ⚡ Оптимізація
    - 🎨 UI/UX покращення

    Вимоги до обох розділів:
    - Будь стислим та зрозумілим для кінцевих користувачів
    - Опиши зміни простою мовою, уникай технічного жаргону де можливо
    - Якщо коміт технічний (refactor, cleanup) - опиши користувацьку вигоду
    - Групуй схожі зміни разом
    - Підкресли найважливіші зміни
    - НЕ додавай секції про завантаження або встановлення
    - ⚠️ ВАЖЛИВО: Обмеження на довжину release notes становить 125 000 символів! Якщо текст близько до ліміту, скорочуй менш важливі деталі.

    ⚠️ ВАЖЛИВО про другий розділ: Робимо його менш деталізованим, ніж перший. Включай ТІЛЬКИ найважливіші та найпомітніші зміни, пропускай дрібниці та технічні деталі. Уникай дублікації змін, які вже описані раніше чи у першому розділі. Цей розділ має бути швидким оглядом.

    ⚠️ КРИТИЧНО: Це генерація для автоматичної публікації релізу. Виводи ТІЛЬКИ Markdown текст changelog без жодних додаткових текстів, коментарів чи пояснень!"

    # Створюємо JSON payload
    JSON_PAYLOAD=$(jq -n \
        --arg prompt "$PROMPT" \
        '{
        "contents": [{
            "parts": [{
            "text": $prompt
            }]
        }],
        "generationConfig": {
            "temperature": 0.7,
            "maxOutputTokens": 120000
        }
        }')

    RESPONSE=$(curl -s "https://generativelanguage.googleapis.com/v1beta/models/gemini-3-flash-preview:generateContent?key=${GEMINI_API_KEY}" \
        -H "Content-Type: application/json" \
        -d "$JSON_PAYLOAD")

    # Перевіряємо чи є помилка в відповіді
    ERROR=$(echo "$RESPONSE" | jq -r '.error.message // empty')
    if [ -n "$ERROR" ]; then
        echo "⚠️ Помилка API: $ERROR"
        echo "📄 Повна відповідь:"
        echo "$RESPONSE" | jq .
    fi

    # Витягуємо текст з відповіді
    CHANGELOG=$(echo "$RESPONSE" | jq -r '.candidates[0].content.parts[0].text // empty')

    if [ -z "$CHANGELOG" ]; then
        echo "⚠️ AI не згенерував changelog, використовуємо fallback"
        CHANGELOG="# 🧪 Beta Реліз ${FULL_VERSION}

        > ⚠️ **Увага**: Це бета-версія для тестування. Може містити нестабільні функції.

        ## 🆕 Що нового у версії ${FULL_VERSION}

        ${NEW_COMMITS}

        ## 📋 Зміни з попередніх beta версій (відносно версії ${LAST_STABLE_TAG})

        ${PREVIOUS_RELEASE_NOTES}"
    else
        echo "✅ AI згенерував changelog"
    fi
else
    echo "⚠️ GEMINI_API_KEY не знайдено, використовуємо стандартний changelog"
    CHANGELOG="# 🧪 Beta Реліз ${FULL_VERSION}

    > ⚠️ **Увага**: Це бета-версія для тестування. Може містити нестабільні функції.

    ## 🆕 Що нового у версії ${FULL_VERSION}

    ${NEW_COMMITS}"

    # Якщо є попередні реліз ноти, додаємо їх як другий розділ
    if [ -n "$PREVIOUS_RELEASE_NOTES" ]; then
    CHANGELOG+=$'\n\n'"## 📋 Зміни з попередніх beta версій (відносно версії ${LAST_STABLE_TAG})"$'\n'"${PREVIOUS_RELEASE_NOTES}"
    fi
fi

# Зберігаємо changelog
echo "$CHANGELOG" > CHANGELOG.md
cat CHANGELOG.md
