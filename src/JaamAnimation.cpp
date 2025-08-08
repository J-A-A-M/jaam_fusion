#include "JaamAnimation.h"
#include "JaamConfig.h"
#include "JaamUtils.h"

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

AnimationManager::AnimationManager() : activeCount(0), activeAnimationsCount(0), settings(nullptr), synchronizedMode(false) {
    animMutex = xSemaphoreCreateMutex();
    globalTimesMutex = xSemaphoreCreateMutex();
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        animations[i] = nullptr;
        activeAnimations[i].strip = nullptr;
        activeAnimations[i].ledIndex = -1;
        activeAnimations[i].animationIndex = -1;
    }
}

void AnimationManager::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

// Методи синхронізації анімацій
void AnimationManager::setSynchronizedMode(bool enabled) {
    bool wasEnabled = synchronizedMode;
    synchronizedMode = enabled;
    
    // Якщо синхронізацію вмикають - скидаємо всі глобальні часи
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
            LOG.printf("[ANIMATION] Invalid animation type %d, returning current time\n", animationType);
            return currentTime;
        }
        
        if (xSemaphoreTake(globalTimesMutex, portMAX_DELAY) == pdTRUE) {
            if (!globalTimesInitialized[animationType]) {
                globalStartTimes[animationType] = currentTime;
                globalTimesInitialized[animationType] = true;
                LOG.printf("[ANIMATION] Initialized global time for type %d (%s): %u\n", 
                          animationType, 
                          ANIMATION_TYPES[animationType].name,
                          globalStartTimes[animationType]);
            }
            
            uint32_t returnTime = globalStartTimes[animationType];
            xSemaphoreGive(globalTimesMutex);
            return returnTime;
        }
    }
    // Асинхронний режим або якщо не вдалося захопити мютекс - повертаємо поточний час
    return currentTime;
}

void AnimationManager::checkAndResetGlobalTime(uint16_t animationType) {
    if (!synchronizedMode || animationType >= ANIMATION_TYPES_COUNT) return;
    
    // Перевіряємо чи є активні анімації цього типу
    bool hasActiveAnimations = false;
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (animations[i] != nullptr && animations[i]->isActive && animations[i]->type == animationType) {
            hasActiveAnimations = true;
            break;
        }
    }
    
    // Якщо немає активних анімацій цього типу - скидаємо глобальний час
    if (!hasActiveAnimations) {
        if (xSemaphoreTake(globalTimesMutex, portMAX_DELAY) == pdTRUE) {
            if (globalTimesInitialized[animationType]) {
                globalTimesInitialized[animationType] = false;
                globalStartTimes[animationType] = 0;
                LOG.printf("[ANIMATION] Reset global time for type %d (%s)\n", 
                          animationType, ANIMATION_TYPES[animationType].name);
            }
            xSemaphoreGive(globalTimesMutex);
        }
    }
}

bool AnimationManager::createAnimation(uint16_t type, 
                                    Adafruit_NeoPixel* strip,
                                    int* positions, 
                                    int posCount,
                                    uint32_t color,
                                    uint32_t initialColor,
                                    uint32_t period,
                                    uint32_t cycles,
                                    uint8_t startBrightness,
                                    uint8_t endBrightness,
                                    uint16_t region_id,
                                    int bit)
{
    if (strip == nullptr) {
        LOG.printf("[ANIMATION] ERROR: Strip is nullptr\n");
        return false;
    }
    
    // Отримуємо startTime до захоплення animMutex щоб уникнути deadlock
    uint32_t startTime = getStartTime(type);
    
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        // Для кожного LED перевіряємо пріоритет існуючих анімацій
        for (int posIdx = 0; posIdx < posCount; ++posIdx) {
            int ledPos = positions[posIdx];
            for (int i = 0; i < activeAnimationsCount; ++i) {
                if (activeAnimations[i].strip == strip &&
                    activeAnimations[i].ledIndex == ledPos) {
                    int animIdx = activeAnimations[i].animationIndex;
                    if (animations[animIdx] != nullptr) {
                        int existingBit = animations[animIdx]->bit;
                        
                        const char* typeName = (type < ANIMATION_TYPES_COUNT) ? ANIMATION_TYPES[type].name : "unknown";
                        const char* stripName = getStripName(strip);
                        // Перевіряємо пріоритет: якщо існуюча анімація має вищий або рівний пріоритет або це відбій тривоги
                        if (!hasHigherPriority(bit, existingBit) && bit!= -1) {
                            
                            LOG.printf("[ANIMATION] REJECTED strip=%s, type=%s, region=%d, led=%d: existing animation has higher priority (bit %d vs %d)\n", 
                                      stripName, typeName, region_id, ledPos, existingBit, bit);
                            xSemaphoreGive(animMutex);
                            return false;
                        } else {
                            LOG.printf("[ANIMATION] REPLACING strip=%s, type=%s, region=%d, led=%d: existing animation bit %d replaced with %d\n", 
                                      stripName, typeName, region_id, ledPos, existingBit, bit);
                        }
                        
                        // Якщо нова анімація має вищий пріоритет - видаляємо стару
                        removeLedFromAnimation(animations[animIdx], ledPos, animIdx);
                        // Після видалення масив зсувається, тому треба зменшити i
                        i--;
                    }
                }
            }
        }

        if (activeCount >= MAX_ANIMATIONS) {
            LOG.printf("[ANIMATION] ERROR: Maximum animations reached (%d)\n", MAX_ANIMATIONS);
            xSemaphoreGive(animMutex);
            return false;
        }

        int slot = -1;
        for (int i = 0; i < MAX_ANIMATIONS; i++) {
            if (animations[i] == nullptr) {
                slot = i;
                break;
            }
        }

        if (slot != -1) {
            animations[slot] = new(std::nothrow) AnimationParams;
            if (animations[slot] == nullptr) {
                LOG.printf("[ANIMATION] ERROR: Failed to allocate memory for animation\n");
                xSemaphoreGive(animMutex);
                return false;
            }
            
            animations[slot]->positions = new(std::nothrow) int[posCount];
            if (animations[slot]->positions == nullptr) {
                LOG.printf("[ANIMATION] ERROR: Failed to allocate memory for positions array\n");
                delete animations[slot];
                animations[slot] = nullptr;
                xSemaphoreGive(animMutex);
                return false;
            }
            
            animations[slot]->type = type;
            animations[slot]->strip = strip;
            animations[slot]->posCount = posCount;
            animations[slot]->color = color;
            animations[slot]->initialColor = initialColor;
            animations[slot]->period = period;
            animations[slot]->cycles = cycles;
            animations[slot]->startBrightness = startBrightness;
            animations[slot]->endBrightness = endBrightness;
            animations[slot]->isActive = true;
            animations[slot]->startTime = startTime;                 // Використовуємо заздалегідь отриманий час
            animations[slot]->localStartTime = millis();             // Локальний час для тривалості
            animations[slot]->region_id = region_id;
            animations[slot]->bit = bit;
            
            memcpy(animations[slot]->positions, positions, posCount * sizeof(int));

            // LOG: Початок анімації
            const char* typeName = (type < ANIMATION_TYPES_COUNT) ? ANIMATION_TYPES[type].name : "unknown";
            const char* stripName = getStripName(strip);
            
            LOG.printf("[ANIMATION] START strip=%s, type=%s, region=%d, leds=", stripName, typeName, region_id);
            for (int i = 0; i < posCount; ++i) {
                LOG.printf("%d ", positions[i]);
            }
            LOG.printf(" period=%u, cycles=%u, startBrightness=%u, endBrightness=%u, bit=%d\n", period, cycles, startBrightness, endBrightness, bit);

            // Зберігаємо початковий колір для першого LED
            if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                if (animations[slot]->initialColor == 0x000000) {
                    animations[slot]->initialColor = strip->getPixelColor(positions[0]);
                }
                xSemaphoreGive(stripMutex);
            }

            // Додаємо в список активних анімацій для кожного LED
            for (int posIdx = 0; posIdx < posCount; ++posIdx) {
                if (activeAnimationsCount < MAX_ANIMATIONS) {
                    activeAnimations[activeAnimationsCount].strip = strip;
                    activeAnimations[activeAnimationsCount].ledIndex = positions[posIdx];
                    activeAnimations[activeAnimationsCount].animationIndex = slot;
                    activeAnimationsCount++;
                } else {
                    LOG.printf("[ANIMATION] WARNING: activeAnimations array full\n");
                    break;
                }
            }

            activeCount++;
            xSemaphoreGive(animMutex);
            return true;
        }
        xSemaphoreGive(animMutex);
    }
    return false;
}

void AnimationManager::update() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < MAX_ANIMATIONS; i++) {
            if (animations[i] != nullptr && animations[i]->isActive) {
                updateAnimation(animations[i], i);
            }
        }
        xSemaphoreGive(animMutex);
    }
    showAllStrips();
}

void AnimationManager::showAllStrips() {
    // Викликаємо show() лише один раз для кожної стрічки
    std::set<Adafruit_NeoPixel*> updatedStrips;
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (animations[i] != nullptr && animations[i]->isActive) {
            Adafruit_NeoPixel* strip = animations[i]->strip;
            if (strip && updatedStrips.find(strip) == updatedStrips.end()) {
                if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                    strip->show();
                    xSemaphoreGive(stripMutex);
                }
                updatedStrips.insert(strip);
            }
        }
    }
}

void AnimationManager::clearAllAnimations() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < MAX_ANIMATIONS; i++) {
            if (animations[i] != nullptr) {
                cleanupAnimation(animations[i], i);
            }
        }
        activeCount = 0;
        activeAnimationsCount = 0;
        xSemaphoreGive(animMutex);
    }
    
    // Скидаємо всі глобальні часи після очищення всіх анімацій
    resetAllGlobalTimes();
}

void AnimationManager::logActiveAnimations() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        LOG.printf("[ANIMATION] Active animations count: %d\n", activeCount);
        for (int i = 0; i < MAX_ANIMATIONS; i++) {
            if (animations[i] != nullptr && animations[i]->isActive) {
                AnimationParams* anim = animations[i];
                const char* stripName = getStripName(anim->strip);

                if (strcmp(stripName, "unknown") == 0) {
                    LOG.printf("[DEBUG] Animation %d: strip is nullptr\n", i);
                    continue;
                }

                const char* typeName = (anim->type < ANIMATION_TYPES_COUNT) ? ANIMATION_TYPES[anim->type].name : "unknown";

                LOG.printf("[DEBUG] Animation %d: strip=%s, LED=%d, region=%d, type=%s, startBrightness=%d, endBrightness=%d, period=%u, cycles=%u\n",
                         i, stripName, anim->positions, anim->region_id, typeName, anim->startBrightness, anim->endBrightness, anim->period, anim->cycles);
            }
        }
        xSemaphoreGive(animMutex);
    }
}

void AnimationManager::updateAnimation(AnimationParams* anim, int index) {
    uint32_t currentTime = millis();
    
    // Розраховуємо тривалість анімації від локального часу початку
    float localElapsed = (currentTime - anim->localStartTime) / float(anim->period);
    
    // Для синхронного режиму розраховуємо фазу від глобального часу
    float elapsed;
    if (synchronizedMode) {
        elapsed = (currentTime - anim->startTime) / float(anim->period);
    } else {
        elapsed = localElapsed;
    }

    // Логування раз на секунду
    // if (currentTime - anim->lastLogTime >= 1000) {
    //     LOG.printf("[ANIMATION PROGRESS] type=%s, region=%d, period=%u, cycles=%u, elapsed=%d\n", typeName, anim->region_id, anim->period, anim->cycles, (int)elapsed);
    //     anim->lastLogTime = currentTime;
    // }

    // Перевіряємо завершення анімації за локальним часом
    if (localElapsed >= anim->cycles) {
        anim->isActive = false;
        // LOG: Кінець анімації
        uint32_t duration = millis() - anim->localStartTime;
        const char* typeName = (anim->type < ANIMATION_TYPES_COUNT) ? ANIMATION_TYPES[anim->type].name : "unknown";
        const char* stripName = getStripName(anim->strip);
        
        LOG.printf("[ANIMATION] END strip=%s, type=%s, region=%d, leds=", stripName, typeName, anim->region_id);
        for (int i = 0; i < anim->posCount; ++i) {
            LOG.printf("%d ", anim->positions[i]);
        }
        LOG.printf(" period=%u, cycles=%u, duration=%u ms, localElapsed=%.2f\n", anim->period, anim->cycles, duration, localElapsed);
        cleanupAnimation(anim, index);
        return;
    }

    switch (anim->type) {
        case 0:
            updateFadeAnimation(anim, elapsed);
            break;
        case 1:
            updateBlinkAnimation(anim, elapsed);
            break;
        case 2:
            updateBlendFadeAnimation(anim, elapsed);
            break;
        case 3:
            updatePulseAnimation(anim, elapsed);
            break;
        case 4:
            updateOneWayBlendAnimation(anim, elapsed);
            break;
        case 5:
            updateRunningLightAnimation(anim, elapsed);
            break;
        case 6:
            updateSetBrightnessAnimation(anim, elapsed);
            break;
    }
}

void AnimationManager::updateSetBrightnessAnimation(AnimationParams* anim, float elapsed) {
    float phase = elapsed - floor(elapsed); // 0.0 to 1.0
    uint8_t currentBrightness = anim->startBrightness + 
                (anim->endBrightness - anim->startBrightness) * phase;

    // Якщо це останній цикл або майже кінець фази — явно ставимо endBrightness
    if (phase > 0.9f || (anim->cycles - elapsed) < 0.001f) {
        currentBrightness = anim->endBrightness;
    }

    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        LOG.printf("[ANIMATION] updateSetBrightnessAnimation: %d\n", led.brightnessMapped(currentBrightness));
        anim->strip->setBrightness(led.brightnessMapped(currentBrightness));
        for (int i = 0; i < anim->strip->numPixels(); i++) {
            uint32_t color = ledActualColor(anim->strip, i);
            anim->strip->setPixelColor(i, color);
        }
        xSemaphoreGive(stripMutex);
    }
}

void AnimationManager::updateFadeAnimation(AnimationParams* anim, float elapsed) {
    float phase = elapsed - floor(elapsed);
    float factor = 1.0f - (0.5 * (1 - cos(2 * PI * phase)));
    
    uint8_t scale = anim->startBrightness + 
                (anim->endBrightness - anim->startBrightness) * factor;

    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < anim->posCount; ++i) {
            int idx = anim->positions[i];
            uint32_t c = anim->color;
            uint8_t r = ((c >> 16) & 0xFF) * scale / 255;
            uint8_t g = ((c >>  8) & 0xFF) * scale / 255;
            uint8_t b = ( c        & 0xFF) * scale / 255;
            anim->strip->setPixelColor(idx, r, g, b);
        }
        xSemaphoreGive(stripMutex);
    }
}

void AnimationManager::updateBlinkAnimation(AnimationParams* anim, float elapsed) {
    float phase = elapsed - floor(elapsed);
    uint8_t brightness = (phase < 0.5) ? anim->endBrightness : anim->startBrightness;

    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < anim->posCount; ++i) {
            int idx = anim->positions[i];
            uint32_t c = anim->color;
            uint8_t r = ((c >> 16) & 0xFF) * brightness / 255;
            uint8_t g = ((c >>  8) & 0xFF) * brightness / 255;
            uint8_t b = ( c        & 0xFF) * brightness / 255;
            anim->strip->setPixelColor(idx, r, g, b);
        }
        xSemaphoreGive(stripMutex);
    }
}

void AnimationManager::updateBlendFadeAnimation(AnimationParams* anim, float elapsed) {
    float phase = elapsed - floor(elapsed);
    // Використовуємо синусоїду для плавного переходу
    //float factor = 0.5 * (1 + sin(2 * PI * phase));
    float factor = 0.5 * (1 - cos(2 * PI * phase));
    
    
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < anim->posCount; ++i) {
            int idx = anim->positions[i];
            // Змішуємо початковий колір з кольором анімації
            uint32_t blendedColor = blendColors(anim->color, anim->initialColor, factor);
            anim->strip->setPixelColor(idx, blendedColor);
        }
        xSemaphoreGive(stripMutex);
    }
}

void AnimationManager::updateOneWayBlendAnimation(AnimationParams* anim, float elapsed) {
    float phase = elapsed - floor(elapsed);
    float factor = phase;

    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < anim->posCount; ++i) {
            int idx = anim->positions[i];
            // Змішуємо початковий колір з кольором анімації в одному напрямку
            uint32_t blendedColor = blendColors(anim->initialColor, anim->color, factor);
            anim->strip->setPixelColor(idx, blendedColor);
        }
        xSemaphoreGive(stripMutex);
    }
}

void AnimationManager::updatePulseAnimation(AnimationParams* anim, float elapsed) {
    float phase = elapsed - floor(elapsed);
    float factor;
    
    // Розділяємо цикл на 4 фази:
    // 0.0-0.2: підйом до максимуму
    // 0.2-0.3: спуск до середини
    // 0.3-0.4: підйом до максимуму
    // 0.4-1.0: спуск до мінімуму
    if (phase < 0.2) {
        // Підйом до максимуму (0.0 -> 1.0)
        factor = sin(phase * 5 * PI);
    } else if (phase < 0.3) {
        // Спуск до середини (1.0 -> 0.5)
        factor = 1.0 - (phase - 0.2) * 5;
    } else if (phase < 0.4) {
        // Підйом до максимуму (0.5 -> 1.0)
        factor = 0.5 + sin((phase - 0.3) * 5 * PI) * 0.5;
    } else {
        // Спуск до мінімуму (1.0 -> 0.0)
        factor = 1.0 - (phase - 0.4) * 1.67;
    }
    
    uint8_t scale = anim->startBrightness + 
                (anim->endBrightness - anim->startBrightness) * factor;

    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < anim->posCount; ++i) {
            int idx = anim->positions[i];
            uint32_t c = anim->color;
            uint8_t r = ((c >> 16) & 0xFF) * scale / 255;
            uint8_t g = ((c >>  8) & 0xFF) * scale / 255;
            uint8_t b = ( c        & 0xFF) * scale / 255;
            anim->strip->setPixelColor(idx, r, g, b);
        }
        xSemaphoreGive(stripMutex);
    }
}

void AnimationManager::updateRunningLightAnimation(AnimationParams* anim, float elapsed) {
    // elapsed: 0.0 ... 1.0 ... cycles
    int totalLeds = anim->strip->numPixels();
    if (totalLeds == 0) return;

    // Розмір анімаційного вікна (9 ледів)
    const int windowSize = 9;
    
    // Поточна позиція початку вікна анімації
    int windowStart = int((elapsed - floor(elapsed)) * totalLeds);
    
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        // Спочатку встановлюємо всі леди в дефолтний колір
        for (int i = 0; i < totalLeds; ++i) {
            anim->strip->setPixelColor(i, anim->initialColor);
        }
        
        // Тепер застосовуємо анімаційне вікно
        for (int windowPos = 0; windowPos < windowSize; ++windowPos) {
            int ledIndex = (windowStart + windowPos) % totalLeds;
            
            uint32_t ledColor;
            uint8_t brightness = anim->endBrightness;
            
            if (windowPos == 0) {
                // LED 1: initialColor
                ledColor = anim->initialColor;
            } else if (windowPos >= 1 && windowPos <= 4) {
                // LEDs 2-5: поступовий перехід від initialColor до color
                float factor = (float)(windowPos - 1) / 3.0f; // 0.0 to 1.0
                ledColor = blendColors(anim->initialColor, anim->color, factor);
            } else if (windowPos == 4) {
                // LED 5: повністю color
                ledColor = anim->color;
            } else if (windowPos >= 5 && windowPos <= 8) {
                // LEDs 6-9: поступовий перехід від color до initialColor
                float factor = (float)(windowPos - 5) / 3.0f; // 0.0 to 1.0
                ledColor = blendColors(anim->color, anim->initialColor, factor);
            }
            
            // Застосовуємо яскравість
            uint8_t r = ((ledColor >> 16) & 0xFF) * brightness / 255;
            uint8_t g = ((ledColor >> 8) & 0xFF) * brightness / 255;
            uint8_t b = (ledColor & 0xFF) * brightness / 255;
            
            anim->strip->setPixelColor(ledIndex, r, g, b);
        }
        
        xSemaphoreGive(stripMutex);
    }
}



uint32_t AnimationManager::adaptColorBrightness(uint32_t color, uint8_t brightness) {
    uint8_t r = ((color >> 16)  & 0xFF) * brightness / 255;
    uint8_t g = ((color >> 8)   & 0xFF) * brightness / 255;
    uint8_t b = ( color         & 0xFF) * brightness / 255;
    color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    return color;
}

void AnimationManager::adaptAllAnimationColors() {
    // Захист від багатопотокового доступу, якщо потрібно
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        LOG.printf("[ANIMATION] Active animations count: %d\n", activeCount);
        for (int i = 0; i < MAX_ANIMATIONS; i++) {
            if (animations[i] != nullptr && animations[i]->isActive) {
                LOG.printf("[DEBUG] enter animation %d\n", i);
                AnimationParams* anim = animations[i];
                for (int k = 0; k < anim->posCount; ++k) {
                    int ledIdx = anim->positions[k];
                    // Адаптуємо колір для кожного LED
                    LOG.printf("[DEBUG] changed color for led %d\n", ledIdx);
                    anim->color = ledActualColor(anim->strip, ledIdx, false, anim->bit);
                }
            }
        }
        xSemaphoreGive(animMutex);
    }
}

void AnimationManager::adaptAllAnimationBrightness() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        uint8_t globalEnd = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_ANIMATION_END));
        for (int i = 0; i < MAX_ANIMATIONS; i++) {
            if (animations[i] != nullptr && animations[i]->isActive) {
                AnimationParams* anim = animations[i];
                int bit = anim->bit; // може бути -1 (clear)
                if (bit >= -1) {
                    std::pair<uint32_t, uint8_t> result = getActualColorAndBrightness(bit);
                    uint8_t newStart = result.second;
                    if (newStart != anim->startBrightness) {
                        LOG.printf("[ANIMATION] Adapt startBrightness anim %d: bit=%d %u->%u\n", i, bit, anim->startBrightness, newStart);
                        anim->startBrightness = newStart;
                    }
                }
                if (anim->endBrightness != globalEnd) {
                    LOG.printf("[ANIMATION] Adapt endBrightness anim %d: %u->%u\n", i, anim->endBrightness, globalEnd);
                    anim->endBrightness = globalEnd;
                }
            }
        }
        xSemaphoreGive(animMutex);
    }
}

std::pair<uint32_t, uint8_t> AnimationManager::getActualColorAndBrightness(int highest_bit) {
    uint32_t color = 0;
    uint8_t brightness = 0;
    
    // Перебираємо біти від найвищого до найнижчого
    for (int bit = highest_bit; bit >= -1; bit--) {
        bool is_enabled = false;
        
        // Перевіряємо чи дозволено показувати цей тип тривоги
        if (bit == -1) {
            is_enabled = true; // Відбій завжди показуємо
        } else if (bit == 0) {
            is_enabled = true; // Alert завжди показуємо
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

        // Якщо тип тривоги дозволено показувати - встановлюємо колір
        if (is_enabled) {
            if (bit == -1) {
                color = colorFromHex(settings->getString(COLOR_CLEAR));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_CLEAR));
            } else if (bit == 0) {
                color = colorFromHex(settings->getString(COLOR_ALERT));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_ALERT));
            } else if (bit == 5) {
                color = colorFromHex(settings->getString(COLOR_DRONES));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_DRONES));
            } else if (bit == 6) {
                color = colorFromHex(settings->getString(COLOR_MISSILES));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_MISSILES));
            } else if (bit == 7) {
                color = colorFromHex(settings->getString(COLOR_KABS));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_KABS));
            } else if (bit == 8) {
                color = colorFromHex(settings->getString(COLOR_BALLISTIC));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_BALLISTIC));
            } else if (bit == 9) {
                color = colorFromHex(settings->getString(COLOR_EXPLOSION));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_EXPLOSION));
            } else if (bit == 10) {
                color = colorFromHex(settings->getString(COLOR_RECON_DRONES));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_RECON_DRONES));
            }
            break; // Зупиняємося на першому дозволеному біті
        }
    }
    return std::make_pair(color, brightness);
}

uint32_t AnimationManager::stripActualColor(Adafruit_NeoPixel* strip, bool adapted) {
    uint32_t color;
    uint8_t brightness = 255;
    if (strip == strip_main) {
        LOG.printf("[COLOR] main strip color\n");
        color = DefaultColors::MAIN_STRIP;
    }
    if (strip == strip_bg) {
        if (settings->getInt(BG_LED_MODE) == 0) {
            LOG.printf("[COLOR] bg strip color HOME_DISTRICT\n");
            color = regionActualColor(settings->getInt(HOME_DISTRICT), false);
        } else {
            LOG.printf("[COLOR] bg strip color SELF\n");
            color = colorFromHex(settings->getString(COLOR_BG));
        }
        brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_BG));
    } 
    if (strip == strip_service) {
        LOG.printf("[COLOR] service strip color\n");
        color = DefaultColors::SERVICE_STRIP;
        brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_SERVICE));
    }
    if (adapted) {
        color = adaptColorBrightness(color, brightness);
    }
    return color;
}

uint32_t AnimationManager::regionActualColor(uint16_t region_id, bool adapted) {
    uint32_t color;
    bool alert = false;
    bool drones = false;
    bool missiles = false;
    bool kab = false;
    bool ballistic = false;
    bool explosion = false;

    uint8_t brightness = 0;
    int highest_bit = findHighestBitForRegion(region_id);
    
    if (highest_bit != -1) {
        std::pair<uint32_t, uint8_t> result = getActualColorAndBrightness(highest_bit);
        color = result.first;
        brightness = result.second;
    } else {
        color = colorFromHex(settings->getString(COLOR_CLEAR));
        brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_CLEAR));
        if (region_id == settings->getInt(HOME_DISTRICT)) {
            color = colorFromHex(settings->getString(COLOR_HOME_DISTRICT));
            brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_HOME_DISTRICT));
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
        auto regions = getRegionsForLed(position);
        int highest_bit = -1;

        if (bit != -1) {
            highest_bit = bit;
        } else {
            highest_bit = findHighestBitForLed(position);
        }
        if (highest_bit != -1) {
            std::pair<uint32_t, uint8_t> result = getActualColorAndBrightness(highest_bit);
            color = result.first;
            brightness = result.second;
        } else {
            
            color = colorFromHex(settings->getString(COLOR_CLEAR));
            brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_CLEAR));
            
            // Якщо немає тривог, перевіряємо чи є домашній район
            for (uint16_t region_id : regions) {
                if (region_id == settings->getInt(HOME_DISTRICT)) {
                    color = colorFromHex(settings->getString(COLOR_HOME_DISTRICT));
                    brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_HOME_DISTRICT));
                    break;
                }
            }
        }
    } 
    if (strip == strip_bg) {
        int highest_bit = -1;

        if (bit != -1) {
            highest_bit = bit;
        } else {
            highest_bit = findHighestBitForRegion(settings->getInt(HOME_DISTRICT));
        }
        
        if (highest_bit != -1 && settings->getInt(BG_LED_MODE) == 0) {
            // Якщо немає тривог, встановлюємо колір домашнього району
            std::pair<uint32_t, uint8_t> result = getActualColorAndBrightness(highest_bit);
            color = result.first;
        } else {
            color = colorFromHex(settings->getString(COLOR_BG));
        }
        brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_BG));
    } 
    if (strip == strip_service) {
        color = getServicePinColor(position);
        brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_SERVICE));
    }

    if (adapted) {
        color = adaptColorBrightness(color, brightness);
    }
    return color;
}

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

void AnimationManager::removeLedFromAnimation(AnimationParams* anim, int ledIdx, int animIndex) {
    // Видаляємо ledIdx з positions
    int newCount = 0;
    for (int i = 0; i < anim->posCount; ++i) {
        if (anim->positions[i] != ledIdx) {
            anim->positions[newCount++] = anim->positions[i];
        }
    }
    anim->posCount = newCount;

    // Видаляємо з activeAnimations
    for (int i = 0; i < activeAnimationsCount; ++i) {
        if (activeAnimations[i].animationIndex == animIndex &&
            activeAnimations[i].ledIndex == ledIdx) {
            for (int j = i; j < activeAnimationsCount - 1; ++j) {
                activeAnimations[j] = activeAnimations[j + 1];
            }
            activeAnimationsCount--;
            break;
        }
    }

    // Якщо не залишилось LED — видалити всю анімацію
    if (anim->posCount == 0) {
        cleanupAnimation(anim, animIndex);
    }
}

void AnimationManager::cleanupAnimation(AnimationParams* anim, int index) {
    if (anim == nullptr) return;
    String positionsStr = "";
    uint16_t animationType = anim->type;
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < anim->posCount; ++i) {
            // Визначаємо дефолтний колір для стрічки
            uint32_t color = ledActualColor(anim->strip, anim->positions[i]);
            anim->strip->setPixelColor(anim->positions[i], color);
            positionsStr += String(anim->positions[i]);
            if (i < anim->posCount - 1) positionsStr += " ";
        }
        anim->strip->show();
        xSemaphoreGive(stripMutex);
    }

    // Видаляємо з списку активних анімацій
    for (int i = 0; i < activeAnimationsCount; i++) {
        if (activeAnimations[i].animationIndex == index) {
            // Зміщуємо всі елементи після видаленого
            for (int j = i; j < activeAnimationsCount - 1; j++) {
                activeAnimations[j] = activeAnimations[j + 1];
            }
            activeAnimationsCount--;
            i--; // Decrement i to check the shifted element
        }
    }

    // Proper memory cleanup
    if (anim->positions != nullptr) {
        delete[] anim->positions;
        anim->positions = nullptr;
    }
    
    delete anim;
    animations[index] = nullptr;
    activeCount--;
    
    // Перевіряємо чи потрібно скинути глобальний час для цього типу
    checkAndResetGlobalTime(animationType);
    
    const char* stripName = getStripName(anim->strip);

    LOG.printf("[ANIMATION] Cleaned up animation slot %d, strip=%s, leds=%s, active count: %d\n", index, stripName, positionsStr, activeCount);
}

std::vector<FreeLedInfo> AnimationManager::getFreeLeds(Adafruit_NeoPixel* strip, uint32_t num_leds) {
    std::vector<FreeLedInfo> freeLedsResult;
    std::set<int> animatedLeds;

    if (strip == nullptr) {
        LOG.println("[LED] ERROR: Strip is nullptr in getFreeLeds");
        return freeLedsResult;
    }
    
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < MAX_ANIMATIONS; i++) {
            if (animations[i] != nullptr && animations[i]->isActive && animations[i]->strip == strip) {
                AnimationParams* anim = animations[i];
                for (int k = 0; k < anim->posCount; ++k) {
                    animatedLeds.insert(anim->positions[k]);
                }
            }
        }
        xSemaphoreGive(animMutex);
    }

    for (uint32_t j = 0; j < num_leds; ++j) {
        if (animatedLeds.find(j) == animatedLeds.end()) {
            freeLedsResult.push_back({(int)j});
        }
    }

    return freeLedsResult;
}

bool AnimationManager::isLedAnimated(Adafruit_NeoPixel* strip, int ledIdx) {
    for (int i = 0; i < MAX_ANIMATIONS; ++i) {
        if (animations[i] && animations[i]->isActive && animations[i]->strip == strip) {
            for (int j = 0; j < animations[i]->posCount; ++j) {
                if (animations[i]->positions[j] == ledIdx) {
                    return true;
                }
            }
        }
    }
    return false;
}

void AnimationManager::paintStripDefault(Adafruit_NeoPixel* strip) {
    if (strip == nullptr) return;
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        uint32_t defaultColor = stripActualColor(strip);
        LOG.println("[LED] paint default color: " + String(defaultColor, HEX));
        for (uint16_t i = 0; i < strip->numPixels(); ++i) {
            strip->setPixelColor(i, defaultColor);
        }
        strip->show();
        xSemaphoreGive(stripMutex);
    }
}