#pragma once
#include <Adafruit_NeoPixel.h>// Оголошуємо стріпки як покажчики
#include "JaamConfig.h"


// --- Configuration for two independent NeoPixel strips ---
#define NUM_LEDS_STRIP 26

Adafruit_NeoPixel* strip1 = nullptr;
Adafruit_NeoPixel* strip2 = nullptr;
Adafruit_NeoPixel* strip3 = nullptr;

// Convert percentage to 0-255 scale
uint8_t brightnessVal(uint8_t percent) {
  return map(percent, 0, 100, 0, 255);
}

// Функція для створення стріпки
Adafruit_NeoPixel* createStrip(uint8_t pin, neoPixelType pixelType) {
    Adafruit_NeoPixel* strip = new Adafruit_NeoPixel(NUM_LEDS_STRIP, pin, pixelType);
    strip->begin();
    strip->setBrightness(brightnessVal(DEFAULT_BRIGHTNESS_PERCENT));
    strip->clear();
    strip->show();
    return strip;
}


// Mutex to protect strip access
SemaphoreHandle_t stripMutex;