#pragma once
#include "JaamLed.h" 
#include "JaamConfig.h"
#include "JaamLogs.h"
#include "JaamUtils.h"

// Зовнішні змінні для стрічок
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
        PULSE,
        ONE_WAY_BLEND
    };

    Adafruit_NeoPixel* strip;
    Type type;
    int* positions;
    int posCount;
    uint32_t color;
    uint32_t initialColor;
    uint32_t period;
    uint8_t cycles;
    uint8_t startBrightness;
    uint8_t endBrightness;
    bool isActive;
    uint32_t startTime;
    uint16_t region_id;
};

// Структура для інформації про вільний LED
struct FreeLedInfo {
    int ledIdx;
};

class AnimationManager {
    private:
        static const int MAX_ANIMATIONS = 50;
        AnimationParams* animations[MAX_ANIMATIONS];
        SemaphoreHandle_t animMutex;
        int activeCount;
        JaamSettings* settings;
        JaamLed led;
        u_int16_t num_leds_main;

        // Структура для відстеження активних анімацій
        struct ActiveAnimation {
            Adafruit_NeoPixel* strip;
            int ledIndex;
            int animationIndex;
        };
        ActiveAnimation activeAnimations[MAX_ANIMATIONS];
        int activeAnimationsCount;

        void updateAnimation(AnimationParams* anim, int index);
        void updateFadeAnimation(AnimationParams* anim, float elapsed);
        void updateBlinkAnimation(AnimationParams* anim, float elapsed);
        void updateBlendBlinkAnimation(AnimationParams* anim, float elapsed);
        void updatePulseAnimation(AnimationParams* anim, float elapsed);
        void updateRainbowAnimation(AnimationParams* anim, float elapsed);
        void updateOneWayBlendAnimation(AnimationParams* anim, float elapsed);
        void cleanupAnimation(AnimationParams* anim, int index);
        uint32_t blendColors(uint32_t color1, uint32_t color2, float factor);
        void removeLedFromAnimation(AnimationParams* anim, int ledIdx, int animIndex);
        uint32_t adaptColorBrightness(Adafruit_NeoPixel* strip, uint32_t color, uint8_t brightness);
        

    public:
        AnimationManager();
        void setSettings(JaamSettings* settings);
        bool createAnimation(AnimationParams::Type type, 
                           Adafruit_NeoPixel* strip,
                           const int* positions, 
                           int posCount,
                           uint32_t color,
                           uint32_t period = 1000,
                           uint8_t cycles = 5,
                           uint8_t startBrightness = 50,
                           uint8_t endBrightness = 150,
                           uint16_t region_id = 0);
        void update();
        void clearAllAnimations();
        void logActiveAnimations();
        uint32_t stripDefaultColor(Adafruit_NeoPixel* strip);
        uint32_t ledActualColor(Adafruit_NeoPixel* strip, uint16_t region_id, bool adapted = true);
        bool safeStripOperation(Adafruit_NeoPixel* strip, std::function<void(Adafruit_NeoPixel*)> operation);
        std::vector<FreeLedInfo> getFreeLeds(Adafruit_NeoPixel* strip, uint16_t num_leds);
        bool isLedAnimated(Adafruit_NeoPixel* strip, int ledIdx);
};


