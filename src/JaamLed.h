#pragma once
#include <Adafruit_NeoPixel.h>// Оголошуємо стріпки як покажчики
#include "JaamSettings.h"


// Глобальні змінні для стріпок
extern Adafruit_NeoPixel* strip_main;
extern Adafruit_NeoPixel* strip_bg;
extern Adafruit_NeoPixel* strip_service;
extern SemaphoreHandle_t stripMutex;

// Флаги ініціалізації стріпок
extern bool strip_main_initialized;
extern bool strip_bg_initialized;
extern bool strip_service_initialized;

// Convert percentage to 0-255 scale
inline uint8_t brightnessAbsolute(uint8_t percent) {
  return map(percent, 0, 100, 0, 255);
}

inline uint8_t brightnessMapped(uint8_t percent) {
  return map(percent, 0, 100, 0, 100);
}

// Перевірка ініціалізації стріпки
inline bool isStripInitialized(Adafruit_NeoPixel* strip) {
    return strip != nullptr;
}

// Безпечний доступ до стріпки
inline bool safeStripOperation(Adafruit_NeoPixel* strip, std::function<void(Adafruit_NeoPixel*)> operation) {
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