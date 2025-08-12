#pragma once
#include "JaamLed.h" 
#include "JaamConfig.h"
#include "JaamLogs.h"
#include "JaamUtils.h"

// Зовнішні змінні для стрічок
extern Adafruit_NeoPixel* strip_main;
extern Adafruit_NeoPixel* strip_bg;
extern Adafruit_NeoPixel* strip_service;

// Структура для параметрів анімації
struct AnimationParams {
    Adafruit_NeoPixel* strip;
    uint8_t mapMode;
    uint16_t type;
    int* positions;
    int posCount;
    uint32_t color;
    uint32_t initialColor;
    uint32_t period;
    uint32_t cycles;
    uint8_t startBrightness;
    uint8_t endBrightness;
    bool isActive;
    uint32_t startTime;        // Глобальний час для синхронізації фази
    uint32_t localStartTime;   // Локальний час початку для розрахунку тривалості
    uint16_t region_id;
    uint32_t lastLogTime = 0;
    int bit;
};

// Структура для інформації про вільний LED
struct FreeLedInfo {
    int ledIdx;
};

class AnimationManager {
    private:
        static const int MAX_ANIMATIONS = 280;     
        SemaphoreHandle_t animMutex;
        SemaphoreHandle_t globalTimesMutex;  // Окремий мютекс для синхронізації global start times
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
        
        // Синхронізація анімацій
        bool synchronizedMode;
        static uint32_t globalStartTimes[ANIMATION_TYPES_COUNT];
        static bool globalTimesInitialized[ANIMATION_TYPES_COUNT];
        
        uint32_t getStartTime(uint16_t animationType);
        void checkAndResetGlobalTime(uint16_t animationType);

        void updateAnimation(AnimationParams* anim, int index);
        void updateFadeAnimation(AnimationParams* anim, float elapsed);
        void updateBlinkAnimation(AnimationParams* anim, float elapsed);
        void updateBlendFadeAnimation(AnimationParams* anim, float elapsed);
        void updatePulseAnimation(AnimationParams* anim, float elapsed);
        void updateRainbowAnimation(AnimationParams* anim, float elapsed);
        void updateOneWayBlendAnimation(AnimationParams* anim, float elapsed);
        void updateRunningLightAnimation(AnimationParams* anim, float elapsed);
        void updateSetBrightnessAnimation(AnimationParams* anim, float elapsed);
        void cleanupAnimation(AnimationParams* anim, int index);
        void removeLedFromAnimation(AnimationParams* anim, int ledIdx, int animIndex);  
        uint32_t blendColors(uint32_t color1, uint32_t color2, float factor);
        std::pair<uint32_t, uint8_t> getActualColorAndBrightness(int highest_bit);
        const char* getStripName(Adafruit_NeoPixel* strip);

    public:
        AnimationManager();
        AnimationParams* animations[MAX_ANIMATIONS];
        void setSettings(JaamSettings* settings);
        bool createAnimation(uint16_t type, 
                           Adafruit_NeoPixel* strip,
                           uint8_t map_mode,
                           int* positions, 
                           int posCount,
                           uint32_t color,
                           uint32_t initialColor = 0x000000,
                           uint32_t period = 1000,
                           uint32_t cycles = 5,
                           uint8_t startBrightness = 50,
                           uint8_t endBrightness = 255,
                           uint16_t region_id = 0,
                           int bit = 0);
        void update();
        void clearAllAnimations();
        void logActiveAnimations();
        std::vector<FreeLedInfo> getFreeLeds(Adafruit_NeoPixel* strip, uint32_t num_leds);
        bool safeStripOperation(Adafruit_NeoPixel* strip, std::function<void(Adafruit_NeoPixel*)> operation);
        bool isLedAnimated(Adafruit_NeoPixel* strip, int ledIdx);
        void paintStripDefault(Adafruit_NeoPixel* strip);
        void adaptAllAnimationColors();
        void adaptAllAnimationBrightness();
        void adaptAllAnimationPeriod();
        void adaptAllAnimationType();
        uint32_t colorFromHex(const char* hex);
        uint32_t stripActualColor(Adafruit_NeoPixel* strip, bool adapted = true);
        uint32_t ledActualColor(Adafruit_NeoPixel* strip, uint16_t position, bool adapted = true, int bit = -1);
        uint32_t regionActualColor(uint16_t region_id, bool adapted = true);
        uint32_t adaptColorBrightness(uint32_t color, uint8_t brightness);
        void showAllStrips();
        
        // Методи синхронізації
        void setSynchronizedMode(bool enabled);
        bool isSynchronizedMode() const;
        void resetAllGlobalTimes();
};


