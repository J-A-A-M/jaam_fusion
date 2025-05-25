#include "JaamAnimation.h"
#include "JaamConfig.h"
#include "JaamUtils.h"


extern std::map<uint16_t, bool> airAlertsMap;
extern std::map<uint16_t, bool> dronesAlertsMap;

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

AnimationManager::AnimationManager() : activeCount(0), activeAnimationsCount(0), settings(nullptr) {
    animMutex = xSemaphoreCreateMutex();
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

bool AnimationManager::createAnimation(AnimationParams::Type type, 
                                    Adafruit_NeoPixel* strip,
                                    const int* positions, 
                                    int posCount,
                                    uint32_t color,
                                    uint32_t period,
                                    uint8_t cycles,
                                    uint8_t startBrightness,
                                    uint8_t endBrightness,
                                    uint16_t region_id)
{
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        // Для кожного LED видаляємо його зі старої анімації (якщо є)
        for (int posIdx = 0; posIdx < posCount; ++posIdx) {
            int ledPos = positions[posIdx];
            for (int i = 0; i < activeAnimationsCount; ++i) {
                if (activeAnimations[i].strip == strip &&
                    activeAnimations[i].ledIndex == ledPos) {
                    int animIdx = activeAnimations[i].animationIndex;
                    if (animations[animIdx] != nullptr) {
                        removeLedFromAnimation(animations[animIdx], ledPos, animIdx);
                        // Після видалення масив зсувається, тому треба зменшити i
                        i--;
                    }
                }
            }
        }

        if (activeCount >= MAX_ANIMATIONS) {
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
            animations[slot] = new AnimationParams;
            animations[slot]->type = type;
            animations[slot]->strip = strip;
            animations[slot]->posCount = posCount;
            animations[slot]->color = color;
            animations[slot]->period = period;
            animations[slot]->cycles = cycles;
            animations[slot]->startBrightness = startBrightness;
            animations[slot]->endBrightness = endBrightness;
            animations[slot]->isActive = true;
            animations[slot]->startTime = millis();
            animations[slot]->positions = new int[posCount];
            memcpy(animations[slot]->positions, positions, posCount * sizeof(int));
            animations[slot]->region_id = region_id;

            // Зберігаємо початковий колір для першого LED
            if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                animations[slot]->initialColor = strip->getPixelColor(positions[0]);
                xSemaphoreGive(stripMutex);
            }

            // Додаємо в список активних анімацій для кожного LED
            for (int posIdx = 0; posIdx < posCount; ++posIdx) {
                activeAnimations[activeAnimationsCount].strip = strip;
                activeAnimations[activeAnimationsCount].ledIndex = positions[posIdx];
                activeAnimations[activeAnimationsCount].animationIndex = slot;
                activeAnimationsCount++;
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
}

void AnimationManager::logActiveAnimations() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        LOG.printf("Active animations count: %d\n", activeCount);
        for (int i = 0; i < MAX_ANIMATIONS; i++) {
            if (animations[i] != nullptr && animations[i]->isActive) {
                AnimationParams* anim = animations[i];
                const char* stripName = "unknown";
                if (anim->strip == strip_main) {
                    stripName = "main";
                } else if (anim->strip == strip_bg) {
                    stripName = "bg";
                } else if (anim->strip == strip_service) {
                    stripName = "service";
                }

                const char* typeName = "unknown";
                switch (anim->type) {
                    case AnimationParams::Type::FADE:
                        typeName = "FADE";
                        break;
                    case AnimationParams::Type::BLINK:
                        typeName = "BLINK";
                        break;
                    case AnimationParams::Type::BLEND_BLINK:
                        typeName = "BLEND_BLINK";
                        break;
                    case AnimationParams::Type::PULSE:
                        typeName = "PULSE";
                        break;
                    case AnimationParams::Type::ONE_WAY_BLEND:
                        typeName = "ONE_WAY_BLEND";
                        break;
                }

                LOG.printf("Animation %d: strip=%s, LED=%d, region=%d, type=%s, startBrightness=%d, endBrightness=%d, period=%u, cycles=%u\n",
                         i, stripName, anim->positions, anim->region_id, typeName, anim->startBrightness, anim->endBrightness, anim->period, anim->cycles);
            }
        }
        xSemaphoreGive(animMutex);
    }
}

void AnimationManager::updateAnimation(AnimationParams* anim, int index) {
    uint32_t currentTime = millis();
    float elapsed = (currentTime - anim->startTime) / float(anim->period);
    
    if (elapsed >= anim->cycles) {
        anim->isActive = false;
        cleanupAnimation(anim, index);
        return;
    }

    switch (anim->type) {
        case AnimationParams::Type::FADE:
            updateFadeAnimation(anim, elapsed);
            break;
        case AnimationParams::Type::BLINK:
            updateBlinkAnimation(anim, elapsed);
            break;
        case AnimationParams::Type::BLEND_BLINK:
            updateBlendBlinkAnimation(anim, elapsed);
            break;
        case AnimationParams::Type::PULSE:
            updatePulseAnimation(anim, elapsed);
            break;
        case AnimationParams::Type::ONE_WAY_BLEND:
            updateOneWayBlendAnimation(anim, elapsed);
            break;
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
        anim->strip->show();
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
        anim->strip->show();
        xSemaphoreGive(stripMutex);
    }
}

void AnimationManager::updateBlendBlinkAnimation(AnimationParams* anim, float elapsed) {
    float phase = elapsed - floor(elapsed);
    // Використовуємо синусоїду для плавного переходу
    //float factor = 0.5 * (1 + sin(2 * PI * phase));
    float factor = 0.5 * (1 - cos(2 * PI * phase));
    
    
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < anim->posCount; ++i) {
            int idx = anim->positions[i];
            // Змішуємо початковий колір з кольором анімації
            uint32_t blendedColor = blendColors(anim->initialColor, anim->color, factor);
            anim->strip->setPixelColor(idx, blendedColor);
        }
        anim->strip->show();
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
        anim->strip->show();
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
        anim->strip->show();
        xSemaphoreGive(stripMutex);
    }
}



uint32_t AnimationManager::stripDefaultColor(Adafruit_NeoPixel* strip) {
    uint32_t color;
    if (strip == strip_main) {
        color = DefaultColors::MAIN_STRIP;
    } else if (strip == strip_bg) {
        color = DefaultColors::BG_STRIP;
    } else if (strip == strip_service) {
        color = DefaultColors::SERVICE_STRIP;
    }
    return color;
}

uint32_t AnimationManager::adaptColorBrightness(Adafruit_NeoPixel* strip, uint32_t color, uint8_t brightness) {
    uint8_t r = ((color >> 16)  & 0xFF) * brightness / 255;
    uint8_t g = ((color >> 8)   & 0xFF) * brightness / 255;
    uint8_t b = ( color         & 0xFF) * brightness / 255;
    color = strip->Color(r, g, b); 
    return color;
}

uint32_t AnimationManager::ledActualColor(Adafruit_NeoPixel* strip, uint16_t position, bool adapted) {
    uint32_t color;
    if (strip == strip_main) {
        auto regions = getRegionsForLed(position);
        bool alert = false;
        bool drones = false;
        uint8_t brightness = 0;
        for (uint16_t region_id : regions) {
            auto it = airAlertsMap.find(region_id);
            if (it != airAlertsMap.end() && it->second) {
                alert = true;
                break; // Достатньо одного true
            }
        }
        for (uint16_t region_id : regions) {
            auto it = dronesAlertsMap.find(region_id);
            if (it != dronesAlertsMap.end() && it->second) {
                drones = true;
                break; // Достатньо одного true
            }
        }
        if (alert) {
            color = strip->Color(255, 0, 0); // Червоний
            brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_ALERT));
            if (drones) {
                color = strip->Color(255, 0, 255);
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_EXPLOSION));
            }
        } else {
            color = strip->Color(0, 255, 0); // Зелений
            brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_CLEAR));
            // Якщо є домашній район — окремий brightness
            for (uint16_t region_id : regions) {
                if (region_id == homeDistrict) {
                    brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_HOME_DISTRICT));
                    break;
                }
            }
        }
        if (adapted) {
            color = adaptColorBrightness(strip, color, brightness);
        }
    } else if (strip == strip_bg) {
        color = DefaultColors::BG_STRIP;
    } else if (strip == strip_service) {
        color = DefaultColors::SERVICE_STRIP;
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
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < anim->posCount; ++i) {
            // Визначаємо дефолтний колір для стрічки
            uint32_t color = ledActualColor(anim->strip, anim->positions[i]);
            anim->strip->setPixelColor(anim->positions[i], color);
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
            break;
        }
    }

    delete[] anim->positions;
    delete anim;
    animations[index] = nullptr;
    activeCount--;
}

std::vector<FreeLedInfo> AnimationManager::getFreeLeds(Adafruit_NeoPixel* strip, uint16_t num_leds) {
    std::vector<FreeLedInfo> freeLedsResult;
    std::set<int> animatedLeds;

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

    for (uint16_t j = 0; j < num_leds; ++j) {
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