#pragma once
#include "JaamLed.h"
#include "JaamConfig.h"
#include "JaamLogs.h"
#include "JaamUtils.h"

// Зовнішні змінні для стрічок
extern Adafruit_NeoPixel* strip_main;
extern Adafruit_NeoPixel* strip_bg;
extern Adafruit_NeoPixel* strip_service;

// Стан одного LED (indexed by position in mainStates / serviceStates)
struct LedState {
    uint32_t color;
    uint32_t adaptedInitColor;
    uint32_t startTime;    // глобальний час для синхронізації фази
    uint32_t localStart;   // локальний час для розрахунку тривалості
    uint32_t period;
    uint32_t cycles;
    uint8_t  animType;
    uint8_t  startBr;
    uint8_t  endBr;
    int8_t   bit;
    int8_t   initialBit;
    uint8_t  mapMode;
    bool     active;
};

// Стан стрічки-в-цілому (bg або RUNNING_LIGHT/SET_BRIGHTNESS override на strip_main)
struct StripState {
    Adafruit_NeoPixel* strip;
    uint32_t color;
    uint32_t adaptedInitColor;
    uint32_t startTime;
    uint32_t localStart;
    uint32_t period;
    uint32_t cycles;
    uint16_t animType;
    uint8_t  startBr;
    uint8_t  endBr;
    int8_t   bit;
    int8_t   initialBit;
    uint8_t  mapMode;
    bool     active;
};

// Структура для інформації про вільний LED
struct FreeLedInfo {
    int ledIdx;
};

class AnimationManager {
    private:
        SemaphoreHandle_t animMutex;
        SemaphoreHandle_t globalTimesMutex;
        JaamSettings* settings;
        JaamLed led;

        // Синхронізація анімацій
        bool synchronizedMode;
        static uint32_t globalStartTimes[ANIMATION_TYPES_COUNT];
        static bool globalTimesInitialized[ANIMATION_TYPES_COUNT];

        uint32_t getStartTime(uint16_t animationType);
        void checkAndResetGlobalTime(uint16_t animationType);

        // Стани LED-ів
        LedState   mainStates[MAX_LEDS_STRIP_MAIN];
        LedState   serviceStates[MAX_LEDS_STRIP_SERVICE];

        // Стани стрічок-в-цілому
        StripState bgState;
        StripState mainOverride;   // RUNNING_LIGHT / SET_BRIGHTNESS
        StripState previewState;   // Попередній перегляд анімацій (main strip)
        StripState previewStateBg; // Попередній перегляд анімацій (bg strip)

        // Попередній перегляд анімацій
        bool previewActive;
        uint32_t previewEndTime;
        int8_t previewEventType;

        // Обчислення кольору за elapsed для per-LED анімацій
        uint32_t computeColor(const LedState& s, float elapsed);
        // Обчислення кольору за elapsed для strip-level анімацій
        uint32_t computeStripColor(const StripState& s, float elapsed);
        // Рендер RUNNING_LIGHT по всій стрічці
        void renderRunningLight(const StripState& s, float elapsed);

        uint32_t blendColors(uint32_t color1, uint32_t color2, float factor);
        std::pair<uint32_t, uint8_t> getActualColorAndBrightness(int highest_bit);
        const char* getStripName(Adafruit_NeoPixel* strip);
        
        // Перевірка чи preview активний для поточного map mode
        // Повертає true якщо preview:
        // - глобально активний (previewActive == true)
        // - має активний стан стрічки (previewState.active == true)
        // - має валідний період (previewState.period > 0, захист від ділення на нуль)
        // - призначений для поточного режиму мапи (previewState.mapMode == mapMode)
        bool isPreviewActiveForMode(const StripState& previewState, uint8_t mapMode) const;

    public:
        AnimationManager();
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
                           int bit = 0,
                           int initialBit = -1);
        void update();
        void clearAllAnimations();
        void logActiveAnimations();
        std::vector<FreeLedInfo> getFreeLeds(Adafruit_NeoPixel* strip, uint32_t num_leds);
        bool safeStripOperation(Adafruit_NeoPixel* strip, std::function<void(Adafruit_NeoPixel*)> operation);
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

        // Методи синхронізації
        void setSynchronizedMode(bool enabled);
        bool isSynchronizedMode() const;
        void resetAllGlobalTimes();

        // Методи попереднього перегляду
        void startPreview(int8_t eventType, uint16_t animType, uint32_t color, uint32_t period, uint8_t brightness);
        void stopPreview();
        bool isPreviewActive() const;
};
