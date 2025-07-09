#include "JaamAnimation.h"
#include "JaamConfig.h"
#include "JaamUtils.h"


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
                                    int* positions, 
                                    int posCount,
                                    uint32_t color,
                                    uint32_t initialColor,
                                    uint32_t period,
                                    uint32_t cycles,
                                    uint8_t startBrightness,
                                    uint8_t endBrightness,
                                    uint16_t region_id)
{
    if (strip == nullptr) {
        LOG.printf("[ANIMATION] ERROR: Strip is nullptr\n");
        return false;
    }
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
            animations[slot]->startTime = millis();
            animations[slot]->region_id = region_id;
            
            memcpy(animations[slot]->positions, positions, posCount * sizeof(int));

            // LOG: Початок анімації
            const char* typeName = "unknown";
            switch (type) {
                case AnimationParams::Type::FADE: typeName = "FADE"; break;
                case AnimationParams::Type::BLINK: typeName = "BLINK"; break;
                case AnimationParams::Type::BLEND_FADE: typeName = "BLEND_FADE"; break;
                case AnimationParams::Type::PULSE: typeName = "PULSE"; break;
                case AnimationParams::Type::ONE_WAY_BLEND_FADE: typeName = "ONE_WAY_BLEND_FADE"; break;
                case AnimationParams::Type::RUNNING_LIGHT: typeName = "RUNNING_LIGHT"; break;
                case AnimationParams::Type::SET_BRIGHTNESS: typeName = "SET_BRIGHTNESS"; break;
            }
            LOG.printf("[ANIMATION] START type=%s, region=%d, leds=", typeName, region_id);
            for (int i = 0; i < posCount; ++i) {
                LOG.printf("%d ", positions[i]);
            }
            LOG.printf(" period=%u, cycles=%u, startBrightness=%u, endBrightness=%u\n", period, cycles, startBrightness, endBrightness);

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
}

void AnimationManager::logActiveAnimations() {
    if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
        LOG.printf("[ANIMATION] Active animations count: %d\n", activeCount);
        for (int i = 0; i < MAX_ANIMATIONS; i++) {
            if (animations[i] != nullptr && animations[i]->isActive) {
                AnimationParams* anim = animations[i];
                const char* stripName = "unknown";
                if (anim->strip == strip_main && strip_main != nullptr) {
                    stripName = "main";
                } else if (anim->strip == strip_bg && strip_bg != nullptr) {
                    stripName = "bg";
                } else if (anim->strip == strip_service && strip_service != nullptr) {
                    stripName = "service";
                }

                if (stripName == "unknown") {
                    LOG.printf("[DEBUG] Animation %d: strip is nullptr\n", i);
                    continue;
                }

                const char* typeName = "unknown";
                switch (anim->type) {
                    case AnimationParams::Type::FADE:
                        typeName = "FADE";
                        break;
                    case AnimationParams::Type::BLINK:
                        typeName = "BLINK";
                        break;
                    case AnimationParams::Type::BLEND_FADE:
                        typeName = "BLEND_FADE";
                        break;
                    case AnimationParams::Type::PULSE:
                        typeName = "PULSE";
                        break;
                    case AnimationParams::Type::ONE_WAY_BLEND_FADE:
                        typeName = "ONE_WAY_BLEND_FADE";
                        break;
                }

                LOG.printf("[DEBUG] Animation %d: strip=%s, LED=%d, region=%d, type=%s, startBrightness=%d, endBrightness=%d, period=%u, cycles=%u\n",
                         i, stripName, anim->positions, anim->region_id, typeName, anim->startBrightness, anim->endBrightness, anim->period, anim->cycles);
            }
        }
        xSemaphoreGive(animMutex);
    }
}

void AnimationManager::updateAnimation(AnimationParams* anim, int index) {
    uint32_t currentTime = millis();
    float elapsed = (currentTime - anim->startTime) / float(anim->period);
    

    // Логування раз на секунду
    // if (currentTime - anim->lastLogTime >= 1000) {
    //     LOG.printf("[ANIMATION PROGRESS] type=%s, region=%d, period=%u, cycles=%u, elapsed=%d\n", typeName, anim->region_id, anim->period, anim->cycles, (int)elapsed);
    //     anim->lastLogTime = currentTime;
    // }
    if (elapsed >= anim->cycles) {
        anim->isActive = false;
        // LOG: Кінець анімації
        uint32_t duration = millis() - anim->startTime;
        const char* typeName = "unknown";
        switch (anim->type) {
            case AnimationParams::Type::FADE: typeName = "FADE"; break;
            case AnimationParams::Type::BLINK: typeName = "BLINK"; break;
            case AnimationParams::Type::BLEND_FADE: typeName = "BLEND_FADE"; break;
            case AnimationParams::Type::PULSE: typeName = "PULSE"; break;
            case AnimationParams::Type::ONE_WAY_BLEND_FADE: typeName = "ONE_WAY_BLEND_FADE"; break;
            case AnimationParams::Type::RUNNING_LIGHT: typeName = "RUNNING_LIGHT"; break;
            case AnimationParams::Type::SET_BRIGHTNESS: typeName = "SET_BRIGHTNESS"; break;
        }
        
        LOG.printf("[ANIMATION] END type=%s, region=%d, leds=", typeName, anim->region_id);
        for (int i = 0; i < anim->posCount; ++i) {
            LOG.printf("%d ", anim->positions[i]);
        }
        LOG.printf(" period=%u, cycles=%u, duration=%u ms, elapsed=%u\n", anim->period, anim->cycles, duration, elapsed);
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
        case AnimationParams::Type::BLEND_FADE:
            updateBlendFadeAnimation(anim, elapsed);
            break;
        case AnimationParams::Type::PULSE:
            updatePulseAnimation(anim, elapsed);
            break;
        case AnimationParams::Type::ONE_WAY_BLEND_FADE:
            updateOneWayBlendAnimation(anim, elapsed);
            break;
        case AnimationParams::Type::RUNNING_LIGHT:
            updateRunningLightAnimation(anim, elapsed);
            break;
        case AnimationParams::Type::SET_BRIGHTNESS:
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
                    anim->color = ledActualColor(anim->strip, ledIdx, true);
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
    for (int bit = highest_bit; bit >= 0; bit--) {
        bool is_enabled = false;
        
        // Перевіряємо чи дозволено показувати цей тип тривоги
        if (bit == 0) {
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
        }

        // Якщо тип тривоги дозволено показувати - встановлюємо колір
        if (is_enabled) {
            if (bit == 0) {
                color = colorFromHex(settings->getString(COLOR_ALERT));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_ALERT));
            } else if (bit == 5) {
                color = colorFromHex(settings->getString(COLOR_DRONES));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_EXPLOSION));
            } else if (bit == 6) {
                color = colorFromHex(settings->getString(COLOR_MISSILES));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_EXPLOSION));
            } else if (bit == 7) {
                color = colorFromHex(settings->getString(COLOR_KABS));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_EXPLOSION));
            } else if (bit == 8) {
                color = colorFromHex(settings->getString(COLOR_BALLISTIC));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_EXPLOSION));
            } else if (bit == 9) {
                color = colorFromHex(settings->getString(COLOR_EXPLOSION));
                brightness = led.brightnessAbsolute(settings->getInt(BRIGHTNESS_EXPLOSION));
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

uint32_t AnimationManager::ledActualColor(Adafruit_NeoPixel* strip, uint16_t position, bool adapted) {
    uint32_t color = 0;
    uint8_t brightness = 0;

    if (strip == strip_main) {
        auto regions = getRegionsForLed(position);
        int highest_bit = findHighestBitForLed(position);
        
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
        int highest_bit = findHighestBitForRegion(settings->getInt(HOME_DISTRICT));
        
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
    char hexColor[8];
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < anim->posCount; ++i) {
            // Визначаємо дефолтний колір для стрічки
            uint32_t color = ledActualColor(anim->strip, anim->positions[i]);
            snprintf(hexColor, sizeof(hexColor), "#%06X", color);
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
    
    LOG.printf("[ANIMATION] Cleaned up animation slot %d, color %s, active count: %d\n", index, hexColor, activeCount);
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