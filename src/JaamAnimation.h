#pragma once
#include "JaamLed.h" 
#include "JaamConfig.h"
#include "JaamLogs.h"
#include "JaamGlobals.h"

// Зовнішні змінні для стріпок
extern Adafruit_NeoPixel* strip_main;
extern Adafruit_NeoPixel* strip_bg;
extern Adafruit_NeoPixel* strip_service;

// Зовнішня змінна для індексу домашнього району
extern uint8_t homeDistrict;

// Структура для параметрів анімації
struct AnimationParams {
    enum class Type {
        FADE,
        BLINK,
        BLEND_BLINK,
        PULSE  // Новий тип анімації
    };

    Adafruit_NeoPixel* strip;
    Type type;
    int* positions;
    int posCount;
    uint32_t color;
    uint32_t initialColor;  // Додаємо збереження початкового кольору
    uint32_t period;
    uint8_t cycles;
    uint8_t startBrightness;
    uint8_t endBrightness;
    bool isActive;
    uint32_t startTime;  // Додаємо час початку для кожної анімації
};

// Функція для змішування кольорів
uint32_t blendColors(uint32_t color1, uint32_t color2, float factor) {
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

class AnimationManager {
    private:
        static const int MAX_ANIMATIONS = 50;
        AnimationParams* animations[MAX_ANIMATIONS];
        SemaphoreHandle_t animMutex;
        int activeCount;
        JaamSettings* settings;  // Додаємо посилання на налаштування

        // Структура для відстеження активних анімацій
        struct ActiveAnimation {
            Adafruit_NeoPixel* strip;
            int ledIndex;
            int animationIndex;
        };
        ActiveAnimation activeAnimations[MAX_ANIMATIONS];
        int activeAnimationsCount;

    public:
        AnimationManager() : activeCount(0), activeAnimationsCount(0), settings(nullptr) {
            animMutex = xSemaphoreCreateMutex();
            for (int i = 0; i < MAX_ANIMATIONS; i++) {
                animations[i] = nullptr;
                activeAnimations[i].strip = nullptr;
                activeAnimations[i].ledIndex = -1;
                activeAnimations[i].animationIndex = -1;
            }
        }

        void setSettings(JaamSettings* settings) {
            this->settings = settings;
        }

        bool createAnimation(AnimationParams::Type type, 
                            Adafruit_NeoPixel* strip,
                            const int* positions, 
                            int posCount,
                            uint32_t color,
                            uint32_t period = 1000,
                            uint8_t cycles = 5,
                            uint8_t startBrightness = 50,
                            uint8_t endBrightness = 150) {
            
            if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
                // Перевіряємо чи є вже анімація на цьому LED цієї стріпки
                for (int i = 0; i < activeAnimationsCount; i++) {
                    if (activeAnimations[i].strip == strip && 
                        activeAnimations[i].ledIndex == positions[0]) {
                        // Примусово завершуємо попередню анімацію
                        cleanupAnimation(animations[activeAnimations[i].animationIndex], 
                                      activeAnimations[i].animationIndex);
                        break;
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

                    // Зберігаємо початковий колір
                    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                        animations[slot]->initialColor = strip->getPixelColor(positions[0]);
                        xSemaphoreGive(stripMutex);
                    }

                    // Додаємо в список активних анімацій
                    activeAnimations[activeAnimationsCount].strip = strip;
                    activeAnimations[activeAnimationsCount].ledIndex = positions[0];
                    activeAnimations[activeAnimationsCount].animationIndex = slot;
                    activeAnimationsCount++;

                    activeCount++;
                    xSemaphoreGive(animMutex);
                    return true;
                }
                xSemaphoreGive(animMutex);
            }
            return false;
        }

        void update() {
            if (xSemaphoreTake(animMutex, portMAX_DELAY) == pdTRUE) {
                for (int i = 0; i < MAX_ANIMATIONS; i++) {
                    if (animations[i] != nullptr && animations[i]->isActive) {
                        updateAnimation(animations[i], i);
                    }
                }
                xSemaphoreGive(animMutex);
            }
        }

        void clearAllAnimations() {
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

        void logActiveAnimations() {
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
                        }

                        LOG.printf("Animation %d: strip=%s, LED=%d, type=%s, color=0x%06X, period=%u, cycles=%u\n",
                                 i, stripName, anim->positions[0], typeName, anim->color, anim->period, anim->cycles);
                    }
                }
                xSemaphoreGive(animMutex);
            }
        }

    private:
        void updateAnimation(AnimationParams* anim, int index) {
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
            }
        }

        void updateFadeAnimation(AnimationParams* anim, float elapsed) {
            float phase = elapsed - floor(elapsed);
            float factor = 0.5 * (1 - cos(2 * PI * phase));
            
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

        void updateBlinkAnimation(AnimationParams* anim, float elapsed) {
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

        void updateBlendBlinkAnimation(AnimationParams* anim, float elapsed) {
            float phase = elapsed - floor(elapsed);
            // Використовуємо синусоїду для плавного переходу
            float factor = 0.5 * (1 + sin(2 * PI * phase));
            
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

        void updatePulseAnimation(AnimationParams* anim, float elapsed) {
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

        void updateRainbowAnimation(AnimationParams* anim, float elapsed) {
            // TODO: Реалізувати радужну анімацію
        }

        void cleanupAnimation(AnimationParams* anim, int index) {
            if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
                for (int i = 0; i < anim->posCount; ++i) {
                    // Визначаємо дефолтний колір для стріпки
                    uint32_t defaultColor;
                    if (anim->strip == strip_main) {
                        defaultColor = DefaultColors::MAIN_STRIP;
                        // Перевіряємо чи це LED домашнього району
                        if (anim->positions[i] == homeDistrict) {
                            // Встановлюємо спеціальну яскравість для домашнього району
                            uint8_t homeBrightness = brightnessAbsolute(settings->getInt(BRIGHTNESS_HOME_DISTRICT));
                            uint8_t r = ((defaultColor >> 16) & 0xFF) * homeBrightness / 255;
                            uint8_t g = ((defaultColor >> 8) & 0xFF) * homeBrightness / 255;
                            uint8_t b = (defaultColor & 0xFF) * homeBrightness / 255;
                            LOG.printf("Setting home district brightness after animation: raw=%d, converted=%d, color=0x%02X%02X%02X\n", 
                                     settings->getInt(BRIGHTNESS_HOME_DISTRICT), homeBrightness, r, g, b);
                            anim->strip->setPixelColor(anim->positions[i], r, g, b);
                            continue;
                        }
                    } else if (anim->strip == strip_bg) {
                        defaultColor = DefaultColors::BG_STRIP;
                    } else {
                        defaultColor = DefaultColors::SERVICE_STRIP;
                    }
                    // Встановлюємо дефолтний колір
                    anim->strip->setPixelColor(anim->positions[i], defaultColor);
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
};


