#include "JaamAnimation.h"
#include "JaamConfig.h"
#include "JaamUtils.h"

extern volatile bool isMapOff;
extern int getCurrentMapMode();

// Helper: HSV -> RGB (returns 0xRRGGBB)
static inline uint32_t hsvToRgb(float h, float s, float v) {
    // Нормалізація h (якщо h завжди в межах 0-360, ці рядки майже нічого не коштують)
    while (h < 0.0f) h += 360.0f;
    while (h >= 360.0f) h -= 360.0f;

    // Швидка перевірка: якщо насиченість 0, це просто відтінок сірого
    if (s <= 0.0f) {
        uint8_t val = (uint8_t)(v * 255.0f + 0.5f);
        return ((uint32_t)val << 16) | ((uint32_t)val << 8) | val;
    }

    float hh = h / 60.0f;
    int i = (int)hh;        // Отримуємо індекс сектора (0 до 5)
    float f = hh - i;       // Дробова частина всередині сектора

    float p = v * (1.0f - s);
    float q = v * (1.0f - (s * f));
    float t = v * (1.0f - (s * (1.0f - f)));

    float r1 = 0, g1 = 0, b1 = 0;

    switch (i) {
        case 0:  r1 = v; g1 = t; b1 = p; break;
        case 1:  r1 = q; g1 = v; b1 = p; break;
        case 2:  r1 = p; g1 = v; b1 = t; break;
        case 3:  r1 = p; g1 = q; b1 = v; break;
        case 4:  r1 = t; g1 = p; b1 = v; break;
        default: r1 = v; g1 = p; b1 = q; break; // випадок 5
    }

    // Замінюємо roundf на додавання 0.5 (стандартний трюк для додатних чисел)
    uint8_t R = (uint8_t)(r1 * 255.0f + 0.5f);
    uint8_t G = (uint8_t)(g1 * 255.0f + 0.5f);
    uint8_t B = (uint8_t)(b1 * 255.0f + 0.5f);

    return ((uint32_t)R << 16) | ((uint32_t)G << 8) | B;
}

// Helper: convert temperature (C) to color using violet->red gradient via HSV
static inline uint32_t colorFromTemperature(int tempC, int minC, int maxC) {
    if (maxC <= minC) {
        maxC = minC + 1;
    }
    if (tempC < minC) tempC = minC;
    if (tempC > maxC) tempC = maxC;
    float factor = (float)(tempC - minC) / (float)(maxC - minC); // 0..1
    // Hue: 270° (violet) -> 0° (red)
    float hue = (1.0f - factor) * 270.0f;
    return hsvToRgb(hue, 1.0f, 1.0f);
}

static inline void getWeatherTempBounds(JaamSettings* settings, int& minT, int& maxT) {
    if (settings->getBool(WEATHER_AUTO_BOUNDS) && weatherAutoBoundsValid) {
        minT = weatherAutoMinTemp;
        maxT = weatherAutoMaxTemp;
    } else {
        minT = settings->getInt(WEATHER_MIN_TEMP);
        maxT = settings->getInt(WEATHER_MAX_TEMP);
    }
}

// Ініціалізація статичних змінних для синхронізації
uint32_t AnimationManager::globalStartTimes[ANIMATION_TYPES_COUNT] = {0};
bool AnimationManager::globalTimesInitialized[ANIMATION_TYPES_COUNT] = {false};

// Трансформує "#RRGGBB" у uint32_t для setPixelColor
uint32_t AnimationManager::colorFromHex(const char* hex) {
    if (!hex || hex[0] != '#' || strlen(hex) != 7) {
        return 0;
    }
    char buf[3] = {0};
    buf[0] = hex[1]; buf[1] = hex[2];
    uint8_t r = strtoul(buf, nullptr, 16);
    buf[0] = hex[3]; buf[1] = hex[4];
    uint8_t g = strtoul(buf, nullptr, 16);
    buf[0] = hex[5]; buf[1] = hex[6];
    uint8_t b = strtoul(buf, nullptr, 16);
    uint32_t color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    return color;
}

// Функція для змішування кольорів
uint32_t AnimationManager::blendColors(uint32_t color1, uint32_t color2, float factor) {
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    uint8_t r = r1 + (r2 - r1) * factor;
    uint8_t g = g1 + (g2 - g1) * factor;
    uint8_t b = b1 + (b2 - b1) * factor;

    return (r << 16) | (g << 8) | b;
}

// Допоміжний метод для визначення назви стрічки
const char* AnimationManager::getStripName(Adafruit_NeoPixel* strip) {
    if (strip == strip_main && strip_main != nullptr) {
        return "main";
    } else if (strip == strip_bg && strip_bg != nullptr) {
        return "bg";
    } else if (strip == strip_service && strip_service != nullptr) {
        return "service";
    }
    return "unknown";
}

// Перевірка чи preview активний для поточного map mode
bool AnimationManager::isPreviewActiveForMode(const StripState& previewState, uint8_t mapMode) const {
    return previewActive                  // Preview глобально увімкнений
        && previewState.active            // Стан preview стрічки активний
        && previewState.period > 0        // Період валідний (захист від ділення на нуль)
        && previewState.mapMode == mapMode; // Preview для поточного режиму мапи
}

AnimationManager::AnimationManager() : settings(nullptr), synchronizedMode(false), previewActive(false), previewEndTime(0), previewEventType(-1), randomColorsMainInitialized(false), randomColorsBgInitialized(false) {
    animMutex = xSemaphoreCreateMutex();
    globalTimesMutex = xSemaphoreCreateMutex();
    memset(mainStates,    0, sizeof(mainStates));
    memset(serviceStates, 0, sizeof(serviceStates));
    memset(rcMainStates,  0, sizeof(rcMainStates));
    memset(&bgState,       0, sizeof(bgState));
    memset(&rcBgState,     0, sizeof(rcBgState));
    memset(&previewState,  0, sizeof(previewState));
    memset(&previewStateBg, 0, sizeof(previewStateBg));
    memset(&mainOverride,  0, sizeof(mainOverride));
}

void AnimationManager::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

// ──────────────────────────────────────────────────────
// Методи синхронізації анімацій
// ──────────────────────────────────────────────────────

void AnimationManager::setSynchronizedMode(bool enabled) {
    bool wasEnabled = synchronizedMode;
    synchronizedMode = enabled;
    if (enabled && !wasEnabled) {
        resetAllGlobalTimes();
    }
    LOG.printf("[ANIMATION] Synchronized mode %s\n", enabled ? "enabled" : "disabled");
}

bool AnimationManager::isSynchronizedMode() const {
    return synchronizedMode;
}

void AnimationManager::resetAllGlobalTimes() {
    if (xSemaphoreTake(globalTimesMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < ANIMATION_TYPES_COUNT; i++) {
            globalTimesInitialized[i] = false;
            globalStartTimes[i] = 0;
        }
        xSemaphoreGive(globalTimesMutex);
    }
    LOG.printf("[ANIMATION] All global start times reset\n");
}

uint32_t AnimationManager::getStartTime(uint16_t animationType) {
    uint32_t currentTime = millis();

    if (synchronizedMode) {
        if (animationType >= ANIMATION_TYPES_COUNT) {
            return currentTime;
        }

        if (xSemaphoreTake(globalTimesMutex, portMAX_DELAY) == pdTRUE) {
            if (!globalTimesInitialized[animationType]) {
                globalStartTimes[animationType] = currentTime;
                globalTimesInitialized[animationType] = true;
                LOG.printf("[ANIMATION] Initialized global time for type %d: %u\n",
                          animationType,
                          globalStartTimes[animationType]);
            }
            uint32_t returnTime = globalStartTimes[animationType];
            xSemaphoreGive(globalTimesMutex);
            return returnTime;
        }
    }
    return currentTime;
}

// Викликається всередині animMutex — ітерує масиви без захоплення animMutex
void AnimationManager::checkAndResetGlobalTime(uint16_t animationType) {
    if (!synchronizedMode || animationType >= ANIMATION_TYPES_COUNT) return;

    bool hasActive = false;

    int numMain = strip_main ? min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN) : MAX_LEDS_STRIP_MAIN;
    for (int i = 0; i < numMain && !hasActive; i++) {
        if (mainStates[i].active && mainStates[i].animType == animationType) hasActive = true;
    }

    int numSvc = strip_service ? min((int)strip_service->numPixels(), MAX_LEDS_STRIP_SERVICE) : MAX_LEDS_STRIP_SERVICE;
    for (int i = 0; i < numSvc && !hasActive; i++) {
        if (serviceStates[i].active && serviceStates[i].animType == animationType) hasActive = true;
    }

    if (!hasActive && bgState.active     && bgState.animType     == animationType) hasActive = true;
    if (!hasActive && mainOverride.active && mainOverride.animType == animationType) hasActive = true;

    if (!hasActive) {
        if (xSemaphoreTake(globalTimesMutex, portMAX_DELAY) == pdTRUE) {
            if (globalTimesInitialized[animationType]) {
                globalTimesInitialized[animationType] = false;
                globalStartTimes[animationType] = 0;
                LOG.printf("[ANIMATION] Reset global time for type %d\n",
                          animationType);
            }
            xSemaphoreGive(globalTimesMutex);
        }
    }
}

// ──────────────────────────────────────────────────────
// createAnimation — публічний API, сигнатура незмінна
// ──────────────────────────────────────────────────────

bool AnimationManager::createAnimation(uint16_t type,
                                       Adafruit_NeoPixel* strip,
                                       uint8_t map_mode,
                                       int* positions,
                                       int posCount,
                                       uint32_t color,
                                       uint32_t initialColor,
                                       uint32_t period,
                                       uint32_t cycles,
                                       uint8_t startBrightness,
                                       uint8_t endBrightness,
                                       uint16_t region_id,
                                       int bit,
                                       int initialBit)
{
    if (strip == nullptr) {
        LOG.printf("[ANIMATION] ERROR: Strip is nullptr\n");
        return false;
    }

    // Отримуємо startTime до захоплення animMutex щоб уникнути deadlock
    uint32_t startTime = getStartTime(type);
    uint32_t now = millis();

    const char* stripName = getStripName(strip);

    // ── RUNNING_LIGHT / SET_BRIGHTNESS — strip-level override ──
    if (type == AnimationTypes::RUNNING_LIGHT || type == AnimationTypes::SET_BRIGHTNESS) {
        if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
            mainOverride.strip      = strip;
            mainOverride.color      = color;
            mainOverride.adaptedInitColor  = initialColor;
            mainOverride.startTime  = startTime;
            mainOverride.localStart = now;
            mainOverride.period     = period;
            mainOverride.cycles     = cycles;
            mainOverride.animType   = type;
            mainOverride.startBr    = startBrightness;
            mainOverride.endBr      = endBrightness;
            mainOverride.bit        = (int8_t)bit;
            mainOverride.initialBit = (int8_t)initialBit;
            mainOverride.mapMode    = map_mode;
            mainOverride.active     = true;
            xSemaphoreGive(animMutex);
        }
        LOG.printf("[ANIMATION] START strip=%s, type=%d, period=%u, cycles=%u\n",
                   stripName, type, period, cycles);
        return true;
    }

    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {

        // ── strip_bg — один стан на всю стрічку ──
        if (strip == strip_bg) {
            if (!hasHigherPriority(bit, bgState.bit) && bit != -1 && bgState.active) {
                LOG.printf("[ANIMATION] REJECTED strip=%s, type=%d, region=%d: existing bit %d vs %d\n",
                           stripName, type, region_id, bgState.bit, bit);
                xSemaphoreGive(animMutex);
                return false;
            }
            uint32_t initColor = initialColor;
            if (initColor == 0 && strip->numPixels() > 0) {
                if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                    initColor = strip->getPixelColor(0);
                    xSemaphoreGive(stripMutex);
                }
            }
            bgState.strip      = strip;
            bgState.color      = color;
            bgState.adaptedInitColor  = initColor;
            bgState.startTime  = startTime;
            bgState.localStart = now;
            bgState.period     = period;
            bgState.cycles     = cycles;
            bgState.animType   = type;
            bgState.startBr    = startBrightness;
            bgState.endBr      = endBrightness;
            bgState.bit        = (int8_t)bit;
            bgState.initialBit = (int8_t)initialBit;
            bgState.mapMode    = map_mode;
            bgState.active     = true;
            LOG.printf("[ANIMATION] START strip=%s, type=%d, region=%d, period=%u, cycles=%u, bit=%d\n",
                       stripName, type, region_id, period, cycles, bit);
            xSemaphoreGive(animMutex);
            return true;
        }

        // ── strip_main / strip_service — per-LED стани ──
        LedState* stateArr = nullptr;
        int maxLeds = 0;
        if (strip == strip_main) {
            stateArr = mainStates;
            maxLeds  = MAX_LEDS_STRIP_MAIN;
        } else if (strip == strip_service) {
            stateArr = serviceStates;
            maxLeds  = MAX_LEDS_STRIP_SERVICE;
        } else {
            LOG.printf("[ANIMATION] ERROR: Unknown strip\n");
            xSemaphoreGive(animMutex);
            return false;
        }

        bool anyCreated = false;
        for (int posIdx = 0; posIdx < posCount; posIdx++) {
            int ledPos = positions[posIdx];
            if (ledPos < 0 || ledPos >= maxLeds) continue;

            LedState& s = stateArr[ledPos];

            if (s.active && !hasHigherPriority(bit, (int)s.bit) && bit != -1) {
                LOG.printf("[ANIMATION] REJECTED strip=%s, type=%d, region=%d, led=%d: existing bit %d vs %d\n",
                           stripName, type, region_id, ledPos, s.bit, bit);
                continue;
            }

            if (s.active) {
                LOG.printf("[ANIMATION] REPLACING strip=%s, type=%d, region=%d, led=%d: bit %d -> %d\n",
                           stripName, type, region_id, ledPos, s.bit, bit);
            }

            uint32_t initColor = initialColor;
            if (initColor == 0) {
                if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                    initColor = strip->getPixelColor(ledPos);
                    xSemaphoreGive(stripMutex);
                }
            }

            s.color      = color;
            s.adaptedInitColor  = initColor;
            s.startTime  = startTime;
            s.localStart = now;
            s.period     = period;
            s.cycles     = cycles;
            s.animType   = (uint8_t)type;
            s.startBr    = startBrightness;
            s.endBr      = endBrightness;
            s.bit        = (int8_t)bit;
            s.initialBit = (int8_t)initialBit;
            s.mapMode    = map_mode;
            s.active     = true;
            anyCreated   = true;

            LOG.printf("[ANIMATION] START strip=%s, type=%d, region=%d, led=%d, period=%u, cycles=%u, bit=%d\n",
                       stripName, type, region_id, ledPos, period, cycles, bit);
        }

        xSemaphoreGive(animMutex);
        return anyCreated;
    }
    return false;
}

// ──────────────────────────────────────────────────────
static float computePulseFactor(float phase) {
    float factor;
    if (phase < 0.2f) {
        factor = sinf(phase * 5.0f * PI);
    } else if (phase < 0.3f) {
        factor = 1.0f - (phase - 0.2f) * 5.0f;
    } else if (phase < 0.4f) {
        factor = 0.5f + sinf((phase - 0.3f) * 5.0f * PI) * 0.5f;
    } else {
        factor = 1.0f - (phase - 0.4f) * 1.67f;
    }
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    return factor;
}

// Обчислення кольору анімації (без звернень до стрічок)
// ──────────────────────────────────────────────────────

uint32_t AnimationManager::computeColorRaw(
        uint16_t animType, uint32_t color, uint32_t adaptedInitColor,
        uint8_t startBr, uint8_t endBr, float elapsed) {
    float phase = elapsed - floorf(elapsed);
    switch (animType) {
        case AnimationTypes::FADE: {
            float factor = 1.0f - (0.5f * (1.0f - cosf(2.0f * PI * phase)));
            float scale = startBr + (endBr - startBr) * factor;
            uint32_t c = color;
            return ((uint32_t)((float)((c >> 16) & 0xFF) * scale / 255.0f + 0.5f) << 16)
                 | ((uint32_t)((float)((c >>  8) & 0xFF) * scale / 255.0f + 0.5f) <<  8)
                 | ((uint32_t)((float)( c        & 0xFF) * scale / 255.0f + 0.5f));
        }
        case AnimationTypes::BLINK: {
            uint8_t br = (phase < 0.5f) ? endBr : startBr;
            uint32_t c = color;
            return ((uint32_t)((float)((c >> 16) & 0xFF) * br / 255.0f + 0.5f) << 16)
                 | ((uint32_t)((float)((c >>  8) & 0xFF) * br / 255.0f + 0.5f) <<  8)
                 | ((uint32_t)((float)( c        & 0xFF) * br / 255.0f + 0.5f));
        }
        case AnimationTypes::BLEND_FADE: {
            float factor = 0.5f * (1.0f - cosf(2.0f * PI * phase));
            uint32_t adaptedColor = adaptColorBrightness(color, startBr);
            return blendColors(adaptedColor, adaptedInitColor, factor);
        }
        case AnimationTypes::PULSE: {
            float factor = computePulseFactor(phase);
            float scale = startBr + (endBr - startBr) * factor;
            uint32_t c = color;
            return ((uint32_t)((float)((c >> 16) & 0xFF) * scale / 255.0f + 0.5f) << 16)
                 | ((uint32_t)((float)((c >>  8) & 0xFF) * scale / 255.0f + 0.5f) <<  8)
                 | ((uint32_t)((float)( c        & 0xFF) * scale / 255.0f + 0.5f));
        }
        case AnimationTypes::COLOR_PULSE: {
            float factor = computePulseFactor(phase);
            uint32_t adaptedColor = adaptColorBrightness(color, startBr);
            return blendColors(adaptedColor, adaptedInitColor, 1.0f - factor);
        }
        case AnimationTypes::COLOR_BLINK: {
            if (phase < 0.5f) {
                return adaptColorBrightness(color, startBr);
            } else {
                return adaptedInitColor;
            }
        }
        case AnimationTypes::ONE_WAY_BLEND_FADE: {
            uint32_t adaptedColor = adaptColorBrightness(color, startBr);
            return blendColors(adaptedInitColor, adaptedColor, phase);
        }
        case AnimationTypes::OFF:
        default:
            return adaptColorBrightness(color, startBr);
    }
}

// Рендер RUNNING_LIGHT по всій стрічці (викликати тримаючи stripMutex)
void AnimationManager::renderRunningLight(const StripState& s, float elapsed) {
    if (s.strip == nullptr) return;
    int totalLeds = s.strip->numPixels();
    if (totalLeds == 0) return;

    const int windowSize = 9;
    int windowStart = (int)((elapsed) * totalLeds);

    // Базовий колір для всіх LED
    for (int i = 0; i < totalLeds; i++) {
        s.strip->setPixelColor(i, s.adaptedInitColor);
    }

    // Анімаційне вікно
    for (int w = 0; w < windowSize; w++) {
        int idx = (windowStart + w) % totalLeds;
        uint32_t ledColor;
        if (w == 0) {
            ledColor = s.adaptedInitColor;
        } else if (w <= 4) {
            float factor = (float)(w - 1) / 3.0f;
            ledColor = blendColors(s.adaptedInitColor, s.color, factor);
        } else {
            float factor = (float)(w - 5) / 3.0f;
            ledColor = blendColors(s.color, s.adaptedInitColor, factor);
        }
        uint8_t br = s.endBr;
        uint8_t r = ((ledColor >> 16) & 0xFF) * br / 255;
        uint8_t g = ((ledColor >>  8) & 0xFF) * br / 255;
        uint8_t b = ( ledColor        & 0xFF) * br / 255;
        s.strip->setPixelColor(idx, r, g, b);
    }
}

// ──────────────────────────────────────────────────────
// Головний цикл оновлення
// ──────────────────────────────────────────────────────

void AnimationManager::update() {
    uint32_t now    = millis();
    bool     isOff  = isMapOff;
    int      mapMode = getCurrentMapMode();

    // ── Track RANDOM_COLORS mode changes and initialization ──
    static int lastMapMode = -1;
    if (mapMode != lastMapMode) {
        LOG.printf("[ANIMATION] Map mode changed from %d to %d\n", lastMapMode, mapMode);
        
        // If switching away from RANDOM_COLORS, reset and clear animations
        if (lastMapMode == MapModes::RANDOM_COLORS && mapMode != MapModes::RANDOM_COLORS) {
            LOG.printf("[ANIMATION] Exiting RANDOM_COLORS mode, clearing animations\n");
            resetRandomColorsFlags();
        }
        
        lastMapMode = mapMode;
    }
    
    // Initialize RANDOM_COLORS mode if active
    if (mapMode == MapModes::RANDOM_COLORS) {
        if (strip_main) {
            initRandomColorsMain();
        }
        if (strip_bg) {
            initRandomColorsBg();
        }
    }

    bool mainDirty    = false;
    bool bgDirty      = false;
    bool serviceDirty = false;

    if (xSemaphoreTake(animMutex, portMAX_DELAY) != pdTRUE) return;

    // ── RUNNING_LIGHT / SET_BRIGHTNESS override на strip_main ──
    if (mainOverride.active && mainOverride.strip != nullptr) {
        float localElapsed = (now - mainOverride.localStart) / float(mainOverride.period);
        if (localElapsed >= mainOverride.cycles) {
            mainOverride.active = false;
            checkAndResetGlobalTime(mainOverride.animType);
            // Відновлюємо idle-кольори
            if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                Adafruit_NeoPixel* s = mainOverride.strip;
                for (uint16_t i = 0; i < s->numPixels(); i++) {
                    s->setPixelColor(i, ledActualColor(s, i));
                }
                xSemaphoreGive(stripMutex);
            }
            mainDirty = true;
        } else if (!isOff && mainOverride.mapMode == mapMode) {
            float elapsed = synchronizedMode
                ? (now - mainOverride.startTime) / float(mainOverride.period)
                : localElapsed;
            if (mainOverride.animType == AnimationTypes::SET_BRIGHTNESS) {
                float phase = elapsed - floorf(elapsed);
                uint8_t br = mainOverride.startBr + (mainOverride.endBr - mainOverride.startBr) * phase;
                if (phase > 0.9f || (mainOverride.cycles - elapsed) < 0.001f) br = mainOverride.endBr;
                if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                    Adafruit_NeoPixel* s = mainOverride.strip;
                    for (uint16_t i = 0; i < s->numPixels(); i++) {
                        s->setPixelColor(i, ledActualColor(s, i));
                    }
                    xSemaphoreGive(stripMutex);
                }
            } else {
                if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                    renderRunningLight(mainOverride, elapsed);
                    xSemaphoreGive(stripMutex);
                }
            }
            mainDirty = true;
        }
    }

    // ── Per-LED strip_main ──
    if (strip_main != nullptr && !mainOverride.active && !isPreviewActiveForMode(previewState, mapMode)) {
        int numLeds = min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN);
        if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < numLeds; i++) {
                LedState& s = mainStates[i];
                if (!s.active) continue;

                float localElapsed = (now - s.localStart) / float(s.period);
                if (localElapsed >= s.cycles) {
                    s.active = false;
                    checkAndResetGlobalTime(s.animType);
                    strip_main->setPixelColor(i, ledActualColor(strip_main, i));
                    mainDirty = true;
                    continue;
                }
                if (isOff || s.mapMode != mapMode) continue;

                float elapsed = synchronizedMode
                    ? (now - s.startTime) / float(s.period)
                    : localElapsed;
                strip_main->setPixelColor(i, computeColorRaw(s.animType, s.color, s.adaptedInitColor,
                                                              s.startBr, s.endBr, elapsed));
                mainDirty = true;
            }
            xSemaphoreGive(stripMutex);
        }
    }

    // ── Random Colors mode: Per-LED strip_main (rcMainStates) ──
    if (strip_main != nullptr && mapMode == MapModes::RANDOM_COLORS && !mainOverride.active && !isPreviewActiveForMode(previewState, mapMode)) {
        if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < ADMIN_UNITS_COUNT; i++) {
                LedState& s = rcMainStates[i];
                if (!s.active) continue;
                uint16_t regionId = ADMIN_UNITS[i];

                float localElapsed = now < s.localStart ? 0.0f : (now - s.localStart) / float(s.period);
                const RegionLedMapMeta* meta = findRegionMeta(regionId);
                if (meta == nullptr) {
                    LOG.printf("[RANDOM_COLORS] WARNING: No meta found for region %u\n", regionId);
                    continue;
                }
                if (meta->led_count == 0) continue; // Немає LED для цього регіону, пропускаємо

                const uint16_t* leds = getRegionLeds(meta);
                if (localElapsed >= s.cycles) {
                    // Continuous cycling: генеруємо новий випадковий колір
                    uint8_t r = random(256);
                    uint8_t g = random(256);
                    uint8_t b = random(256);
                    uint32_t newColor = strip_main->Color(r, g, b);
                    
                    // Поточний колір стає початковим для нової анімації
                    uint32_t currentColor = strip_main->getPixelColor(leds[0]);  // Беремо колір першого LED регіону (припускаємо, що всі LED в регіоні однакові)
                    
                    uint16_t delay = random(RAND_COLOR_MOD_ANIM_INTERVAL); // Додаємо випадкову затримку перед наступним циклом для кожного регіону, щоб уникнути синхронності
                    // Перезапускаємо анімацію з новим кольором
                    s.adaptedInitColor = currentColor;
                    s.color = newColor;
                    s.localStart = now + delay;
                    s.startTime = now + delay;
                    // period, cycles, animType залишаються без змін
                    
                    // LOG.printf("[RANDOM_COLORS] Main LED %d: new cycle with color #%06X\n", i, newColor);
                    continue;
                }
                uint32_t computedColor = computeColorRaw(s.animType, s.color, s.adaptedInitColor,
                                                         s.startBr, s.endBr, localElapsed);
                for (uint8_t j = 0; j < meta->led_count; ++j) {
                    strip_main->setPixelColor(leds[j], computedColor);
                    mainDirty = true;
                }
            }
            xSemaphoreGive(stripMutex);
        }
    }


    // ── Preview має вищий пріоритет і перезаписує кольори mainStates ──
    if (strip_main != nullptr && isPreviewActiveForMode(previewState, mapMode)) {
        int numLeds = min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN);
        if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
            float localElapsed = (now - previewState.localStart) / float(previewState.period);
            if (localElapsed < previewState.cycles && !isOff) {
                float elapsed = synchronizedMode
                                    ? (now - previewState.startTime) / float(previewState.period)
                                    : localElapsed;
                uint32_t previewColor = computeColorRaw(previewState.animType, previewState.color, previewState.adaptedInitColor,
                                                         previewState.startBr, previewState.endBr, elapsed);
                for (int i = 0; i < numLeds; i++) {
                    strip_main->setPixelColor(i, previewColor);
                }
                mainDirty = true;
            }
            xSemaphoreGive(stripMutex);
        }
    }

    // ── strip_bg strip-level (bgState) ──
    if (strip_bg != nullptr && bgState.active && !isPreviewActiveForMode(previewStateBg, mapMode)) {
        float localElapsed = (now - bgState.localStart) / float(bgState.period);
        if (localElapsed >= bgState.cycles) {
            bgState.active = false;
            checkAndResetGlobalTime(bgState.animType);
            if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                uint32_t idleColor = stripActualColor(strip_bg);
                for (uint16_t i = 0; i < strip_bg->numPixels(); i++) {
                    strip_bg->setPixelColor(i, idleColor);
                }
                xSemaphoreGive(stripMutex);
            }
            bgDirty = true;
        } else if (!isOff && bgState.mapMode == mapMode) {
            float elapsed = synchronizedMode
                ? (now - bgState.startTime) / float(bgState.period)
                : localElapsed;
            uint32_t c = computeColorRaw(bgState.animType, bgState.color, bgState.adaptedInitColor,
                                          bgState.startBr, bgState.endBr, elapsed);
            if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                for (uint16_t i = 0; i < strip_bg->numPixels(); i++) {
                    strip_bg->setPixelColor(i, c);
                }
                xSemaphoreGive(stripMutex);
            }
            bgDirty = true;
        }
    }

    // ── Random Colors mode: strip-level strip_bg (rcBgState) ──
    if (strip_bg != nullptr && mapMode == MapModes::RANDOM_COLORS && rcBgState.active && !isPreviewActiveForMode(previewStateBg, mapMode)) {
        float localElapsed = now < rcBgState.localStart ? 0.0f : (now - rcBgState.localStart) / float(rcBgState.period);
        if (localElapsed >= rcBgState.cycles) {
            // Continuous cycling: генеруємо новий випадковий колір
            uint8_t r = random(256);
            uint8_t g = random(256);
            uint8_t b = random(256);
            uint32_t newColor = strip_bg->Color(r, g, b);
            
            // Поточний колір стає початковим для нової анімації
            uint32_t currentColor = 0;
            if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                currentColor = strip_bg->getPixelColor(0);  // Беремо колір першого LED
                xSemaphoreGive(stripMutex);
            }
            
            // Перезапускаємо анімацію з новим кольором
            rcBgState.color = newColor;
            rcBgState.adaptedInitColor = currentColor;
            rcBgState.localStart = now;
            rcBgState.startTime = now;
            // period, cycles, animType залишаються без змін
            
            LOG.printf("[RANDOM_COLORS] Bg strip: new cycle with color #%06X\n", newColor);
        } else if (!isOff) {
            uint32_t c = computeColorRaw(rcBgState.animType, rcBgState.color, rcBgState.adaptedInitColor,
                                         rcBgState.startBr, rcBgState.endBr, localElapsed);
            if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                for (uint16_t i = 0; i < strip_bg->numPixels(); i++) {
                    strip_bg->setPixelColor(i, c);
                }
                xSemaphoreGive(stripMutex);
            }
            bgDirty = true;
        }
    }
    
    // ── Preview для strip_bg має вищий пріоритет і перезаписує кольори bgState ──
    if (strip_bg != nullptr && isPreviewActiveForMode(previewStateBg, mapMode)) {
        int numLeds = min((int)strip_bg->numPixels(), MAX_LEDS_STRIP_BG);
        float localElapsed = (now - previewStateBg.localStart) / float(previewStateBg.period);
        if (localElapsed < previewStateBg.cycles && !isOff) {
            float elapsed = synchronizedMode
                ? (now - previewStateBg.startTime) / float(previewStateBg.period)
                : localElapsed;
            uint32_t previewColor = computeColorRaw(previewStateBg.animType, previewStateBg.color, previewStateBg.adaptedInitColor,
                                                     previewStateBg.startBr, previewStateBg.endBr, elapsed);
            if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                for (uint16_t i = 0; i < strip_bg->numPixels(); i++) {
                    strip_bg->setPixelColor(i, previewColor);
                }
                xSemaphoreGive(stripMutex);
            }
            bgDirty = true;
        }
    }

    // ── strip_service per-LED ──
    if (strip_service != nullptr) {
        int numLeds = min((int)strip_service->numPixels(), MAX_LEDS_STRIP_SERVICE);
        if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < numLeds; i++) {
                LedState& s = serviceStates[i];
                if (!s.active) continue;

                float localElapsed = (now - s.localStart) / float(s.period);
                if (localElapsed >= s.cycles) {
                    s.active = false;
                    checkAndResetGlobalTime(s.animType);
                    strip_service->setPixelColor(i, ledActualColor(strip_service, i));
                    serviceDirty = true;
                    continue;
                }
                if (isOff || s.mapMode != mapMode) continue;

                float elapsed = synchronizedMode
                    ? (now - s.startTime) / float(s.period)
                    : localElapsed;
                strip_service->setPixelColor(i, computeColorRaw(s.animType, s.color, s.adaptedInitColor,
                                                                  s.startBr, s.endBr, elapsed));
                serviceDirty = true;
            }
            xSemaphoreGive(stripMutex);
        }
    }

    xSemaphoreGive(animMutex);

    // Перевірка завершення попереднього перегляду (після звільнення animMutex!)
    if (previewActive && now >= previewEndTime) {
        stopPreview();
    }

    // Відправляємо на стрічки тільки ті що були змінені
    if (mainDirty && strip_main != nullptr) {
        if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
            strip_main->show();
            xSemaphoreGive(stripMutex);
        }
    }
    if (bgDirty && strip_bg != nullptr) {
        if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
            strip_bg->show();
            xSemaphoreGive(stripMutex);
        }
    }
    if (serviceDirty && strip_service != nullptr) {
        if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
            strip_service->show();
            xSemaphoreGive(stripMutex);
        }
    }
}

void AnimationManager::clearAllAnimations() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        memset(mainStates,    0, sizeof(mainStates));
        memset(serviceStates, 0, sizeof(serviceStates));
        bgState.active       = false;
        mainOverride.active  = false;
        previewState.active  = false;
        previewStateBg.active = false;
        previewActive        = false;
        previewEndTime       = 0;
        previewEventType     = -1;
        xSemaphoreGive(animMutex);
    }
    resetAllGlobalTimes();
}

void AnimationManager::logActiveAnimations() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        int count = 0;
        int numMain = strip_main ? min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN) : 0;
        for (int i = 0; i < numMain; i++) if (mainStates[i].active) count++;
        for (int i = 0; i < ADMIN_UNITS_COUNT; i++) if (rcMainStates[i].active) count++;
        int numSvc = strip_service ? min((int)strip_service->numPixels(), MAX_LEDS_STRIP_SERVICE) : 0;
        for (int i = 0; i < numSvc; i++) if (serviceStates[i].active) count++;
        if (bgState.active)      count++;
        if (rcBgState.active)    count++;
        if (mainOverride.active) count++;

        LOG.printf("[ANIMATION] Active animations count: %d\n", count);

        for (int i = 0; i < numMain; i++) {
            if (!mainStates[i].active) continue;
            const LedState& s = mainStates[i];
            LOG.printf("[DEBUG] main LED %d: type=%d, bit=%d, period=%u, cycles=%u, startBr=%u, endBr=%u\n",
                       i, s.animType, s.bit, s.period, s.cycles, s.startBr, s.endBr);
        }
        for (int i = 0; i < ADMIN_UNITS_COUNT; i++) {
            if (!rcMainStates[i].active) continue;
            const LedState& s = rcMainStates[i];
            const char* typeName = (s.animType < ANIMATION_TYPES_COUNT) ? ANIMATION_TYPES[s.animType].name : "unknown";
            LOG.printf("[DEBUG] RC main LED %d: type=%s, bit=%d, period=%u, cycles=%u, startBr=%u, endBr=%u\n",
                       i, typeName, s.bit, s.period, s.cycles, s.startBr, s.endBr);
        }
        if (bgState.active) {
            LOG.printf("[DEBUG] bg: type=%d, bit=%d, period=%u, cycles=%u\n",
                       bgState.animType, bgState.bit, bgState.period, bgState.cycles);
        }
        if (rcBgState.active) {
            const char* typeName = (rcBgState.animType < ANIMATION_TYPES_COUNT) ? ANIMATION_TYPES[rcBgState.animType].name : "unknown";
            LOG.printf("[DEBUG] RC bg: type=%s, bit=%d, period=%u, cycles=%u\n",
                       typeName, rcBgState.bit, rcBgState.period, rcBgState.cycles);
        }
        if (mainOverride.active) {
            LOG.printf("[DEBUG] mainOverride: type=%d, period=%u, cycles=%u\n",
                       mainOverride.animType, mainOverride.period, mainOverride.cycles);
        }
        xSemaphoreGive(animMutex);
    }
}

// ──────────────────────────────────────────────────────
// Адаптація параметрів активних анімацій
// ──────────────────────────────────────────────────────

uint32_t AnimationManager::adaptColorBrightness(uint32_t color, uint8_t brightness) {
    uint8_t r = ((color >> 16)  & 0xFF) * brightness / 255;
    uint8_t g = ((color >> 8)   & 0xFF) * brightness / 255;
    uint8_t b = ( color         & 0xFF) * brightness / 255;
    color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    return color;
}

void AnimationManager::adaptAllAnimationColors() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        int numMain = strip_main ? min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN) : 0;
        for (int i = 0; i < numMain; i++) {
            if (mainStates[i].active) {
                mainStates[i].color = ledActualColor(strip_main, i, false, mainStates[i].bit);
                mainStates[i].adaptedInitColor = ledActualColor(strip_main, i, true, mainStates[i].initialBit);
            }
        }
        if (bgState.active) {
            bgState.color = ledActualColor(strip_bg, 0, false, bgState.bit);
            bgState.adaptedInitColor = ledActualColor(strip_bg, 0, true, bgState.initialBit);
        }
        xSemaphoreGive(animMutex);
    }
}

void AnimationManager::adaptAllAnimationBrightness() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        uint8_t globalEnd = led.brightnessRelative(settings->getInt(BRIGHTNESS_ANIMATION_END));
        int numMain = strip_main ? min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN) : 0;

        for (int i = 0; i < numMain; i++) {
            LedState& s = mainStates[i];
            if (!s.active) continue;
            if (s.bit >= -1) {
                std::pair<uint32_t, uint8_t> result = getActualColorAndBrightness(s.bit);
                uint8_t newStart = result.second;
                if (newStart != s.startBr) {
                    LOG.printf("[ANIMATION] Adapt startBrightness led %d: bit=%d %u->%u\n", i, s.bit, s.startBr, newStart);
                    s.startBr = newStart;
                }
            }
            if (s.endBr != globalEnd) {
                LOG.printf("[ANIMATION] Adapt endBrightness led %d: %u->%u\n", i, s.endBr, globalEnd);
                s.endBr = globalEnd;
            }
        }
        if (bgState.active) {
            if (bgState.bit >= -1) {
                std::pair<uint32_t, uint8_t> result = getActualColorAndBrightness(bgState.bit);
                bgState.startBr = result.second;
            }
            bgState.endBr = globalEnd;
        }
        if (randomColorsMainInitialized) {
            uint8_t relativeBrightness = led.brightnessRelative(100);
            for (int i = 0; i < ADMIN_UNITS_COUNT; i++) {
                LedState& s = rcMainStates[i];
                if (!s.active) continue;
                s.startBr = relativeBrightness;
                s.endBr = relativeBrightness;
            }
        }
        if (randomColorsBgInitialized and rcBgState.active) {
            uint8_t relativeBrightness = led.brightnessRelative(100);
            rcBgState.startBr = relativeBrightness;
            rcBgState.endBr = relativeBrightness;
        }
        xSemaphoreGive(animMutex);
    }
}

void AnimationManager::adaptAllAnimationPeriod() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        int numMain = strip_main ? min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN) : 0;

        auto recalc = [&](int8_t bit, uint32_t& period, uint32_t& cycles) {
            uint32_t newPeriod   = period;
            uint32_t totalTimeMs = 0;
            switch (bit) {
                case -1: newPeriod = settings->getInt(ANIMATION_ALERT_OFF_CYCLE_TIME);  totalTimeMs = settings->getInt(ALERT_OFF_TIME)       * 1000UL; break;
                case  0: newPeriod = settings->getInt(ANIMATION_ALERT_ON_CYCLE_TIME);   totalTimeMs = settings->getInt(ALERT_ON_TIME)        * 1000UL; break;
                case  5: newPeriod = settings->getInt(ANIMATION_DRONE_CYCLE_TIME);      totalTimeMs = settings->getInt(DRONE_TIME)           * 1000UL; break;
                case  6: newPeriod = settings->getInt(ANIMATION_MISSILE_CYCLE_TIME);    totalTimeMs = settings->getInt(MISSILE_TIME)         * 1000UL; break;
                case  7: newPeriod = settings->getInt(ANIMATION_KAB_CYCLE_TIME);        totalTimeMs = settings->getInt(KAB_TIME)             * 1000UL; break;
                case  8: newPeriod = settings->getInt(ANIMATION_BALLISTIC_CYCLE_TIME);  totalTimeMs = settings->getInt(BALLISTIC_TIME)       * 1000UL; break;
                case  9: newPeriod = settings->getInt(ANIMATION_EXPLOSION_CYCLE_TIME);  totalTimeMs = settings->getInt(EXPLOSION_TIME)       * 1000UL; break;
                case 10: newPeriod = settings->getInt(ANIMATION_RECON_DRONE_CYCLE_TIME);totalTimeMs = settings->getInt(RECON_DRONE_TIME)     * 1000UL; break;
                default: break;
            }
            if (newPeriod > 0 && totalTimeMs > 0) {
                uint32_t newCycles = totalTimeMs / newPeriod;
                if (newCycles == 0) newCycles = 1;
                if (newPeriod != period) { period = newPeriod; }
                if (newCycles != cycles) { cycles = newCycles; }
            }
        };

        for (int i = 0; i < numMain; i++) {
            if (mainStates[i].active) {
                recalc(mainStates[i].bit, mainStates[i].period, mainStates[i].cycles);
            }
        }
        if (bgState.active) {
            recalc(bgState.bit, bgState.period, bgState.cycles);
        }
        xSemaphoreGive(animMutex);
    }
}

void AnimationManager::adaptAllAnimationType() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        int numMain = strip_main ? min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN) : 0;

        auto newType = [&](int8_t bit) -> uint16_t {
            switch (bit) {
                case -1: return settings->getInt(ANIMATION_ALERT_OFF_TYPE);
                case  0: return settings->getInt(ANIMATION_ALERT_ON_TYPE);
                case  5: return settings->getInt(ANIMATION_DRONE_TYPE);
                case  6: return settings->getInt(ANIMATION_MISSILE_TYPE);
                case  7: return settings->getInt(ANIMATION_KAB_TYPE);
                case  8: return settings->getInt(ANIMATION_BALLISTIC_TYPE);
                case  9: return settings->getInt(ANIMATION_EXPLOSION_TYPE);
                case 10: return settings->getInt(ANIMATION_RECON_DRONE_TYPE);
                default: return 0xFF; // sentinel: no change
            }
        };

        for (int i = 0; i < numMain; i++) {
            LedState& s = mainStates[i];
            if (!s.active) continue;
            uint16_t t = newType(s.bit);
            if (t != 0xFF && t != s.animType) {
                LOG.printf("[ANIMATION] Adapt type led %d: bit=%d %u->%u\n", i, s.bit, s.animType, t);
                s.animType = (uint8_t)t;
            }
        }
        if (bgState.active) {
            uint16_t t = newType(bgState.bit);
            if (t != 0xFF && t != bgState.animType) {
                bgState.animType = (uint16_t)t;
            }
        }
        xSemaphoreGive(animMutex);
    }
}

// ──────────────────────────────────────────────────────
// Допоміжні методи для кольорів (незмінні)
// ──────────────────────────────────────────────────────

std::pair<uint32_t, uint8_t> AnimationManager::getActualColorAndBrightness(int highest_bit) {
    uint32_t color = 0;
    uint8_t brightness = 0;

    for (int bit = highest_bit; bit >= -1; bit--) {
        bool is_enabled = false;

        if (bit == -1) {
            is_enabled = true;
        } else if (bit == 0) {
            is_enabled = true;
        } else if (bit == 5) {
            is_enabled = settings->getBool(ENABLE_DRONES);
        } else if (bit == 6) {
            is_enabled = settings->getBool(ENABLE_MISSILES);
        } else if (bit == 7) {
            is_enabled = settings->getBool(ENABLE_KABS);
        } else if (bit == 8) {
            is_enabled = settings->getBool(ENABLE_BALLISTIC);
        } else if (bit == 9) {
            is_enabled = settings->getBool(ENABLE_EXPLOSIONS);
        } else if (bit == 10) {
            is_enabled = settings->getBool(ENABLE_RECON_DRONES);
        }

        if (is_enabled) {
            if (bit == -1) {
                color = colorFromHex(settings->getString(COLOR_CLEAR));
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_CLEAR));
            } else if (bit == 0) {
                color = colorFromHex(settings->getString(COLOR_ALERT));
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_ALERT));
            } else if (bit == 5) {
                color = colorFromHex(settings->getString(COLOR_DRONES));
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_DRONES));
            } else if (bit == 6) {
                color = colorFromHex(settings->getString(COLOR_MISSILES));
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_MISSILES));
            } else if (bit == 7) {
                color = colorFromHex(settings->getString(COLOR_KABS));
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_KABS));
            } else if (bit == 8) {
                color = colorFromHex(settings->getString(COLOR_BALLISTIC));
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_BALLISTIC));
            } else if (bit == 9) {
                color = colorFromHex(settings->getString(COLOR_EXPLOSION));
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_EXPLOSION));
            } else if (bit == 10) {
                color = colorFromHex(settings->getString(COLOR_RECON_DRONES));
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_RECON_DRONES));
            }
            break;
        }
    }
    return std::make_pair(color, brightness);
}

uint32_t AnimationManager::stripActualColor(Adafruit_NeoPixel* strip, bool adapted) {
    uint32_t color;
    uint8_t brightness = 0;
    if (strip == strip_main) {
        LOG.printf("[COLOR] main strip color\n");
        color = DefaultColors::MAIN_STRIP;
    }
    if (strip == strip_bg) {
        // Handle RANDOM_COLORS mode first, regardless of BG_LED_MODE
        if (!isMapOff && getCurrentMapMode() == MapModes::RANDOM_COLORS) {
            // rcBgState has already rendered with brightness applied
            color = strip_bg->getPixelColor(0); // Already brightness-adjusted by rcBgState
            brightness = led.brightnessRelative(100);
            LOG.printf("[COLOR] bg strip color RANDOM_COLORS (already brightness-adjusted)\n");
        } else if (settings->getInt(BG_LED_MODE) == BgLedModes::HOME_REGION) {
            if (isMapOff) {
                color = DefaultColors::OFF;
                brightness = 0;
            } else {
                int currentMapMode = getCurrentMapMode();
                switch (currentMapMode) {
                    case MapModes::OFF:
                        color = DefaultColors::OFF;
                        brightness = 0;
                        break;
                    case MapModes::ALERT:
                        color = regionActualColor(settings->getInt(HOME_DISTRICT), false);
                        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_BG));
                        break;
                    case MapModes::WEATHER: {
                        uint16_t home = settings->getInt(HOME_DISTRICT);
                        auto it = temperatureMap.find(home);
                        if (it != temperatureMap.end()) {
                            int t = decodeTemperature(it->second);
                            int minT, maxT;
                            getWeatherTempBounds(settings, minT, maxT);
                            color = colorFromTemperature(t, minT, maxT);
                        } else {
                            color = colorFromHex(settings->getString(COLOR_BG));
                        }
                        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_BG));
                        break;
                    }
                    case MapModes::FLAG:
                        color = DefaultColors::FLAG_BLUE;
                        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_BG));
                        break;
                    case MapModes::LAMP:
                        color = colorFromHex(settings->getString(COLOR_LAMP));
                        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_LAMP));
                        break;
                }
            }
            LOG.printf("[COLOR] bg strip color HOME_DISTRICT\n");
        } else {
            LOG.printf("[COLOR] bg strip color SELF\n");
            color = colorFromHex(settings->getString(COLOR_BG));
            brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_BG));
        }
    }
    if (strip == strip_service) {
        LOG.printf("[COLOR] service strip color\n");
        switch (getCurrentMapMode()) {
            case MapModes::OFF: 
                color = DefaultColors::OFF;
                brightness = 0;
                break;
            default:
                color = DefaultColors::SERVICE_STRIP;
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_SERVICE));
                break;
        }
    }
    if (adapted) {
        color = adaptColorBrightness(color, brightness);
    }
    return color;
}

uint32_t AnimationManager::regionActualColor(uint16_t region_id, bool adapted) {
    uint32_t color;
    uint8_t brightness = 0;
    int highest_bit = findHighestBitForRegionFlat(region_id);

    if (highest_bit != -1) {
        std::pair<uint32_t, uint8_t> result = getActualColorAndBrightness(highest_bit);
        color = result.first;
        brightness = result.second;
    } else {
        color = colorFromHex(settings->getString(COLOR_CLEAR));
        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_CLEAR));
        if (region_id == settings->getInt(HOME_DISTRICT)) {
            color = colorFromHex(settings->getString(COLOR_HOME_DISTRICT));
            brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_HOME_DISTRICT));
        }
    }
    if (adapted) {
        color = adaptColorBrightness(color, brightness);
    }
    return color;
}

uint32_t AnimationManager::ledActualColor(Adafruit_NeoPixel* strip, uint16_t position, bool adapted, int bit) {
    uint32_t color = 0;
    uint8_t brightness = 0;

    if (strip == strip_main) {
        // Один раз отримуємо регіони для цього LED — повторно використовуємо нижче
        uint16_t region_buf[16];
        int region_count = getRegionsForLedStatic(position, region_buf, 16);

        // У режимі кастомного маппінгу світлодіоди, не призначені жодному регіону, вимкнені
        bool unmappedCustom = (settings->getInt(HARDWARE) == HARDWARE::CUSTOM_MAPPING)
                              && (region_count == 0);

        if (isMapOff || unmappedCustom) {
            color = DefaultColors::OFF;
            brightness = 0;
        } else {
            switch (getCurrentMapMode()) {
                case MapModes::OFF:
                    color = DefaultColors::OFF;
                    brightness = 0;
                    break;
                case MapModes::ALERT: {
                    int highest_bit = -1;
                    if (bit != -1) {
                        highest_bit = bit;
                    } else {
                        // ledBitCache відображає поточний стан alertsFlat (O(1), без heap)
                        highest_bit = (position < MAX_LEDS_STRIP_MAIN)
                                      ? (int)ledBitCache[position]
                                      : findHighestBitForLedFlat(position);
                    }
                    if (highest_bit != -1) {
                        std::pair<uint32_t, uint8_t> result = getActualColorAndBrightness(highest_bit);
                        color = result.first;
                        brightness = result.second;
                    } else {
                        // Немає тривоги — перевіряємо домашній регіон (region_buf уже заповнений вище)
                        color = colorFromHex(settings->getString(COLOR_CLEAR));
                        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_CLEAR));
                        for (int r = 0; r < region_count; ++r) {
                            if (region_buf[r] == (uint16_t)settings->getInt(HOME_DISTRICT)) {
                                color = colorFromHex(settings->getString(COLOR_HOME_DISTRICT));
                                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_HOME_DISTRICT));
                                break;
                            }
                        }
                    }
                    break;
                }
                case MapModes::WEATHER: {
                    auto regions = getRegionsForLed(position);
                    long sum = 0;
                    int cnt = 0;
                    for (uint16_t region_id : regions) {
                        auto it = temperatureMap.find(region_id);
                        if (it != temperatureMap.end()) {
                            int t = decodeTemperature(it->second);
                            sum += t;
                            cnt++;
                        }
                    }
                    if (cnt > 0) {
                        int avg = (int)(sum / cnt);
                        int minT, maxT;
                        getWeatherTempBounds(settings, minT, maxT);
                        color = colorFromTemperature(avg, minT, maxT);
                    } else {
                        color = colorFromHex(settings->getString(COLOR_CLEAR));
                    }
                    brightness = led.brightnessRelative(100);
                    break;
                }
                case MapModes::FLAG: {
                    auto regions = getRegionsForLed(position);
                    if (regions.empty()) {
                        color = DefaultColors::FLAG_BLUE;
                    } else {
                        int blueVotes = 0, yellowVotes = 0;
                        for (uint16_t region_id : regions) {
                            uint32_t flagColor = getFlagColorForRegion(region_id);
                            if (flagColor == DefaultColors::FLAG_BLUE) blueVotes++;
                            else yellowVotes++;
                        }
                        color = (yellowVotes > blueVotes) ? DefaultColors::FLAG_YELLOW : DefaultColors::FLAG_BLUE;
                    }
                    brightness = led.brightnessRelative(100);
                    break;
                }
                case MapModes::LAMP: {
                    color = colorFromHex(settings->getString(COLOR_LAMP));
                    brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_LAMP));
                    break;
                }
                case MapModes::RANDOM_COLORS: {
                    color = strip->getPixelColor(position); // Отримуємо поточний колір без додаткового brightness
                    brightness = led.brightnessRelative(100);
                    break;
                }
            }
        }
    }
    if (strip == strip_bg) {
        if (isMapOff) {
            color = DefaultColors::OFF;
            brightness = 0;
        } else {
            if (settings->getInt(BG_LED_MODE) == BgLedModes::COLOR_MAP) {
                color = getBgLedColor(position);
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_BG));
            } else {
                switch (getCurrentMapMode()) {
                    case MapModes::OFF:
                        color = DefaultColors::OFF;
                        brightness = 0;
                        break;
                    case MapModes::ALERT: {
                        int highest_bit = -1;
                        if (bit != -1) {
                            highest_bit = bit;
                        } else {
                            highest_bit = findHighestBitForRegionFlat(settings->getInt(HOME_DISTRICT));
                        }
                        if (settings->getInt(BG_LED_MODE) == BgLedModes::HOME_REGION) {
                            std::pair<uint32_t, uint8_t> result = getActualColorAndBrightness(highest_bit);
                            color = result.first;
                        } else {
                            color = colorFromHex(settings->getString(COLOR_BG));
                        }
                        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_BG));
                        break;
                    }
                    case MapModes::WEATHER: {
                        uint16_t home = settings->getInt(HOME_DISTRICT);
                        auto it = temperatureMap.find(home);
                        if (it != temperatureMap.end()) {
                            int t = decodeTemperature(it->second);
                            int minT, maxT;
                            getWeatherTempBounds(settings, minT, maxT);
                            color = colorFromTemperature(t, minT, maxT);
                        } else {
                            color = colorFromHex(settings->getString(COLOR_BG));
                        }
                        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_BG));
                        break;
                    }
                    case MapModes::FLAG:
                        color = DefaultColors::FLAG_BLUE;
                        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_BG));
                        break;
                    case MapModes::LAMP:
                        color = colorFromHex(settings->getString(COLOR_LAMP));
                        brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_LAMP));
                        break;
                }
            }
        }
    }
    if (strip == strip_service) {
        switch (getCurrentMapMode()) {
            case MapModes::OFF: 
                color = DefaultColors::OFF;
                brightness = 0;
                break;
            default:
                color = getServicePinColor(position);
                brightness = led.brightnessRelative(settings->getInt(BRIGHTNESS_SERVICE));
                break;
        }
    }

    if (adapted) {
        color = adaptColorBrightness(color, brightness);
    }
    return color;
}

// ──────────────────────────────────────────────────────
// Утиліти (незмінні)
// ──────────────────────────────────────────────────────

bool AnimationManager::safeStripOperation(Adafruit_NeoPixel* strip, std::function<void(Adafruit_NeoPixel*)> operation) {
    if (strip == nullptr) {
        return false;
    }
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        operation(strip);
        xSemaphoreGive(stripMutex);
        return true;
    }
    return false;
}

std::vector<FreeLedInfo> AnimationManager::getFreeLeds(Adafruit_NeoPixel* strip, uint32_t num_leds) {
    std::vector<FreeLedInfo> result;

    if (strip == nullptr) {
        LOG.printf("[LED] ERROR: Strip is nullptr in getFreeLeds\n");
        return result;
    }

    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        if (strip == strip_main) {
            int n = min((int)num_leds, MAX_LEDS_STRIP_MAIN);
            for (int i = 0; i < n; i++) {
                if (!mainStates[i].active) result.push_back({i});
            }
        } else if (strip == strip_service) {
            int n = min((int)num_leds, MAX_LEDS_STRIP_SERVICE);
            for (int i = 0; i < n; i++) {
                if (!serviceStates[i].active) result.push_back({i});
            }
        } else {
            // Unknown strip — all LEDs are free
            for (uint32_t i = 0; i < num_leds; i++) result.push_back({(int)i});
        }
        xSemaphoreGive(animMutex);
    }

    return result;
}

void AnimationManager::paintStripDefault(Adafruit_NeoPixel* strip) {
    if (strip == nullptr) return;
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        uint32_t defaultColor = stripActualColor(strip);
        LOG.printf("[LED] paint default color: %08X\n", defaultColor);
        for (uint16_t i = 0; i < strip->numPixels(); ++i) {
            strip->setPixelColor(i, defaultColor);
        }
        strip->show();
        xSemaphoreGive(stripMutex);
    }
}

// ──────────────────────────────────────────────────────
// Попередній перегляд анімацій
// ──────────────────────────────────────────────────────

void AnimationManager::startPreview(int8_t eventType, uint16_t animType, uint32_t color, uint32_t period, uint8_t brightness) {
    if (!settings) return;
    
    // Перевірка чи увімкнено попередній перегляд
    if (!settings->getBool(ENABLE_ANIMATION_PREVIEW)) {
        return;
    }
    
    if (strip_main == nullptr) {
        LOG.printf("[PREVIEW] strip_main is null, skipping preview\n");
        return;
    }
    
    LOG.printf("[PREVIEW] Starting preview: eventType=%d, animType=%d, color=0x%06X, period=%u, brightness=%u\n", 
               eventType, animType, color, period, brightness);
    
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        // Якщо вже є активний preview - просто оновлюємо параметри без відновлення кольорів
        // Це запобігає "моргання" map mode при швидкій зміні параметрів
        
        // Захист від ділення на нуль
        if (period == 0) {
            period = 1;
            LOG.printf("[PREVIEW] WARNING: period was 0, using default 1\n");
        }
        
        // Налаштування попереднього перегляду
        previewActive = true;
        previewEndTime = millis() + 5000;  // 5 секунд
        previewEventType = eventType;
        
        // Створюємо анімацію на всій стрічці
        uint32_t now = millis();
        uint32_t cycles = (5000 + period - 1) / period;  // Кількість циклів за 5 секунд
        uint8_t globalStart = led.brightnessRelative(brightness);
        uint8_t globalEnd = led.brightnessRelative(settings->getInt(BRIGHTNESS_ANIMATION_END));
        uint8_t mapMode = getCurrentMapMode();
        
        // Початковий колір залежить від типу події:
        // - Для ALERT: перехід від clear до alert
        // - Для NO_ALERT: перехід від alert назад до clear  
        // - Для інших загроз: перехід від alert до більш небезпечного стану
        uint32_t initColor;
        if (eventType == AlertModes::ALERT) {
            initColor = colorFromHex(settings->getString(COLOR_CLEAR));
        } else {
            // NO_ALERT та інші загрози починаються з alert кольору
            initColor = colorFromHex(settings->getString(COLOR_ALERT));
        }
        uint32_t adaptedInitColor = adaptColorBrightness(initColor, globalStart);
        
        // Отримуємо поточний час старту для синхронізації
        uint32_t startTime = synchronizedMode ? getStartTime(animType) : now;
        
        previewState.strip = strip_main;
        previewState.color = color;
        previewState.adaptedInitColor = adaptedInitColor;
        previewState.startTime = startTime;
        previewState.localStart = now;
        previewState.period = period;
        previewState.cycles = cycles;
        previewState.animType = animType;
        previewState.startBr = globalStart;
        previewState.endBr = globalEnd;
        previewState.bit = 100;  // Високий пріоритет для попереднього перегляду
        previewState.mapMode = mapMode;
        previewState.active = true;
        
        // Налаштовуємо preview для bg стрічки, якщо вона є
        if (strip_bg != nullptr) {
            previewStateBg.strip = strip_bg;
            previewStateBg.color = color;
            previewStateBg.adaptedInitColor = adaptedInitColor;
            previewStateBg.startTime = startTime;
            previewStateBg.localStart = now;
            previewStateBg.period = period;
            previewStateBg.cycles = cycles;
            previewStateBg.animType = animType;
            previewStateBg.startBr = globalStart;
            previewStateBg.endBr = globalEnd;
            previewStateBg.bit = 100;
            previewStateBg.mapMode = mapMode;
            previewStateBg.active = true;
        }
        
        xSemaphoreGive(animMutex);
        
        LOG.printf("[PREVIEW] Preview started on all LEDs for 5 seconds\n");
    }
}

void AnimationManager::stopPreview() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        // Перевіряємо previewActive вже після захоплення мутексу
        if (!previewActive) {
            xSemaphoreGive(animMutex);
            return;
        }
        
        LOG.printf("[PREVIEW] Stopping preview\n");
        
        previewActive = false;
        previewEndTime = 0;
        previewEventType = -1;
        previewState.active = false;
        previewStateBg.active = false;
        
        xSemaphoreGive(animMutex);
    }
    
    // Відновлюємо кольори (захоплюємо stripMutex лише один раз)
    if (strip_main && xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        int numLeds = min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN);
        for (int i = 0; i < numLeds; i++) {
            strip_main->setPixelColor(i, ledActualColor(strip_main, i));
        }
        strip_main->show();
        xSemaphoreGive(stripMutex);
    }
    
    // Відновлюємо кольори для bg стрічки, якщо вона є
    // Використовуємо ledActualColor для кожного пікселя, щоб коректно відновити
    // per-pixel кольори для BgLedModes::COLOR_MAP
    if (strip_bg && xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        int currentMapMode = getCurrentMapMode();
        
        // Special case for RANDOM_COLORS: restore from rcBgState animation frame
        if (currentMapMode == MapModes::RANDOM_COLORS && rcBgState.active) {
            uint32_t now = millis();
            float localElapsed = now < rcBgState.localStart ? 0.0f : (now - rcBgState.localStart) / float(rcBgState.period);
            uint32_t c = computeColorRaw(rcBgState.animType, rcBgState.color, rcBgState.adaptedInitColor,
                                         rcBgState.startBr, rcBgState.endBr, localElapsed);
            for (uint16_t i = 0; i < strip_bg->numPixels(); i++) {
                strip_bg->setPixelColor(i, c);
            }
            LOG.printf("[PREVIEW] Restored RANDOM_COLORS bg strip from rcBgState\n");
        } else {
            // Regular restore using ledActualColor for each pixel (COLOR_MAP support)
            for (uint16_t i = 0; i < strip_bg->numPixels(); i++) {
                strip_bg->setPixelColor(i, ledActualColor(strip_bg, i));
            }
        }
        strip_bg->show();
        xSemaphoreGive(stripMutex);
    }
}

bool AnimationManager::isPreviewActive() const {
    return previewActive;
}

// ──────────────────────────────────────────────────────
// Random Colors Mode Methods
// ──────────────────────────────────────────────────────

void AnimationManager::initRandomColorsMain() {
    if (!strip_main || randomColorsMainInitialized) {
        return;
    }

    // int numLeds = min((int)strip_main->numPixels(), MAX_LEDS_STRIP_MAIN);
    // if (numLeds == 0) {
    //     return;
    // }
    uint32_t now = millis();

    LOG.printf("[RANDOM_COLORS] Initializing main strip, now=%d\n", now);


    uint8_t brightness = led.brightnessRelative(100);


    // Запускаємо анімації з затримкою між кожним LED
    // Записуємо безпосередньо в rcMainStates[] без використання createAnimation()
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        for (int idx = 0; idx < ADMIN_UNITS_COUNT; idx++) {
            uint16_t regionId = ADMIN_UNITS[idx];
            
            // Перевіряємо чи існує LED map для цього регіону
            const RegionLedMapMeta* meta = findRegionMeta(regionId);
            if (meta == nullptr || meta->led_count == 0) {
                // Пропускаємо регіони без LED map
                continue;
            }

            // Генеруємо початковий випадковий колір
            uint8_t r = random(256);
            uint8_t g = random(256);
            uint8_t b = random(256);
            uint32_t initialColor = adaptColorBrightness(strip_main->Color(r, g, b), brightness);

            
            // Генеруємо випадковий колір
            r = random(256);
            g = random(256);
            b = random(256);
            uint32_t randomColor = strip_main->Color(r, g, b);

            uint16_t delay = random(RAND_COLOR_MOD_ANIM_INTERVAL); // Додаткова випадкова затримка до 2 секунд для кожного LED

            // Налаштовуємо стан анімації ONE_WAY_BLEND_FADE в rcMainStates[]
            LedState& s = rcMainStates[idx];
            s.color = randomColor;
            s.adaptedInitColor = initialColor;
            s.startTime = now + delay;  // Стартуємо з додатковою випадковою затримкою
            s.localStart = now + delay;
            s.period = RAND_COLOR_MOD_ANIM_INTERVAL;
            s.cycles = 1;
            s.animType = AnimationTypes::ONE_WAY_BLEND_FADE;
            s.startBr = brightness;
            s.endBr = brightness;
            s.bit = -1;       // no priority
            s.initialBit = -1;
            s.mapMode = MapModes::RANDOM_COLORS;
            s.active = true;
        }
        xSemaphoreGive(animMutex);
    }

    randomColorsMainInitialized = true;
    LOG.printf("[RANDOM_COLORS] Main strip initialized\n");
}

void AnimationManager::initRandomColorsBg() {
    if (!strip_bg || randomColorsBgInitialized) {
        return;
    }

    int numLeds = min((int)strip_bg->numPixels(), MAX_LEDS_STRIP_BG);
    if (numLeds == 0) {
        return;
    }

    LOG.printf("[RANDOM_COLORS] Initializing bg strip as single entity (strip-level)\n");

    // Генеруємо випадковий колір для всієї стрічки
    uint8_t r = random(256);
    uint8_t g = random(256);
    uint8_t b = random(256);
    uint32_t randomColor = strip_bg->Color(r, g, b);

    // Отримуємо поточний колір першого LED як початковий (вся стрічка буде однакова)
    uint32_t initialColor = 0;
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        initialColor = strip_bg->getPixelColor(0);
        xSemaphoreGive(stripMutex);
    }
    uint32_t now = millis();
    uint8_t brightness = led.brightnessRelative(100);
    
    // Записуємо безпосередньо в rcBgState замість використання createAnimation()
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        rcBgState.strip = strip_bg;
        rcBgState.color = randomColor;
        rcBgState.adaptedInitColor = initialColor;
        rcBgState.startTime = now;  // Стартуємо без додаткової випадкової затримки
        rcBgState.localStart = now;
        rcBgState.period = RAND_COLOR_MOD_ANIM_INTERVAL;
        rcBgState.cycles = 1;
        rcBgState.animType = AnimationTypes::ONE_WAY_BLEND_FADE;
        rcBgState.startBr = brightness;
        rcBgState.endBr = brightness;
        rcBgState.bit = -1;       // no priority
        rcBgState.initialBit = -1;
        rcBgState.mapMode = MapModes::RANDOM_COLORS;
        rcBgState.active = true;
        xSemaphoreGive(animMutex);
    }

    randomColorsBgInitialized = true;
    LOG.printf("[RANDOM_COLORS] Bg strip initialized as strip-level animation\n");
}

void AnimationManager::resetRandomColorsFlags() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        randomColorsMainInitialized = false;
        randomColorsBgInitialized = false;
        memset(rcMainStates,  0, sizeof(rcMainStates));
        rcBgState.active     = false;
        xSemaphoreGive(animMutex);
    }
    LOG.printf("[RANDOM_COLORS] Flags reset\n");
}
