#pragma once
#include <Adafruit_NeoPixel.h>// Оголошуємо стрічки як покажчики
#include "JaamSettings.h"
#include "JaamLogs.h"
#include "JaamConfig.h"
#include "JaamGlobals.h"


// Глобальні змінні для стрічок
extern Adafruit_NeoPixel* strip_main;
extern Adafruit_NeoPixel* strip_bg;
extern Adafruit_NeoPixel* strip_service;
extern SemaphoreHandle_t stripMutex;

// Флаги ініціалізації стрічок
extern bool strip_main_initialized;
extern bool strip_bg_initialized;
extern bool strip_service_initialized;

enum class StripStatus {
    SUCCESS = 0,
    STRIP_PIN_NOT_SET,
    STRIP_CREATION_FAILED,
    ANIMATION_CREATION_FAILED,
    NULL_STRIP_POINTER,
    MUTEX_CREATION_FAILED
};

// Convert percentage to 0-255 scale
static inline uint8_t brightnessAbsolute(uint8_t percent) {
  return map(percent, 0, 100, 0, 255);
}

static inline uint8_t brightnessMapped(uint8_t percent) {
  return map(percent, 0, 100, 0, 100);
}

// Перевірка ініціалізації стрічки
static inline bool isStripInitialized(Adafruit_NeoPixel* strip) {
    return strip != nullptr;
}

// Безпечний доступ до стрічки
static inline bool safeStripOperation(Adafruit_NeoPixel* strip, std::function<void(Adafruit_NeoPixel*)> operation) {
    if (!isStripInitialized(strip)) {
        return false;
    }
    
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        operation(strip);
        xSemaphoreGive(stripMutex);
        return true;
    }
    return false;
}

// Функція для створення стрічки з обробкою помилок
static inline StripStatus createStrip(Adafruit_NeoPixel*& strip, 
                        int pin, 
                        uint8_t count, 
                        uint8_t brightness,
                        uint32_t defaultColor, 
                        uint8_t type) {
    if (pin <= 0) {
        return StripStatus::STRIP_PIN_NOT_SET;
    }
    
    strip = new Adafruit_NeoPixel(count, pin, type);
    if (strip == nullptr) {
        return StripStatus::STRIP_CREATION_FAILED;
    }
    
    strip->begin();
    strip->setBrightness(brightnessMapped(brightness));
    strip->clear();
    
    for(int i = 0; i < count; i++) {
        strip->setPixelColor(i, defaultColor);
    }
    strip->show();
    return StripStatus::SUCCESS;
}

