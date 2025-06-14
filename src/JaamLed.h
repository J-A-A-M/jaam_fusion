#pragma once
#include <Adafruit_NeoPixel.h>// Оголошуємо стрічки як покажчики
#include "JaamSettings.h"
#include "JaamLogs.h"
#include "JaamConfig.h"


// Глобальні змінні для стрічок
extern Adafruit_NeoPixel* strip_main;
extern Adafruit_NeoPixel* strip_bg;
extern Adafruit_NeoPixel* strip_service;
extern SemaphoreHandle_t stripMutex;

// Флаги ініціалізації стрічок
// extern bool strip_main_initialized;
// extern bool strip_bg_initialized;
// extern bool strip_service_initialized;

enum class StripStatus {
    SUCCESS = 0,
    STRIP_PIN_NOT_SET,
    STRIP_CREATION_FAILED,
    ANIMATION_CREATION_FAILED,
    NULL_STRIP_POINTER,
    MUTEX_CREATION_FAILED
};

class JaamLed {
private:
    JaamSettings* settings;

public:
    JaamLed();
    void setSettings(JaamSettings* settings);
    
    // Convert percentage to 0-255 scale
    static uint8_t brightnessAbsolute(uint8_t percent);
    static uint8_t brightnessMapped(uint8_t percent);

    // Перевірка ініціалізації стрічки
    static bool isStripInitialized(Adafruit_NeoPixel* strip);

    // Функція для створення стрічки з обробкою помилок
    static StripStatus createStrip(Adafruit_NeoPixel*& strip, 
                        int pin, 
                        uint32_t count, 
                        uint8_t brightness,
                        uint32_t color, 
                        uint8_t type);
    
    // Функція для безпечного видалення стрічки
    static void destroyStrip(Adafruit_NeoPixel*& strip);
    
    // Add memory cleanup verification
    static void verifyStripCleanup(Adafruit_NeoPixel* strip);
    
    // Add strip recreation with proper cleanup
    static StripStatus recreateStrip(Adafruit_NeoPixel*& strip,
                                   int pin,
                                   uint32_t count,
                                   uint8_t brightness,
                                   uint32_t color,
                                   uint8_t type);
};

