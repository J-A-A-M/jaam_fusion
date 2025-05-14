#include "JaamLed.h"

JaamLed::JaamLed() : settings(nullptr) {}

void JaamLed::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

// Convert percentage to 0-255 scale
uint8_t JaamLed::brightnessAbsolute(uint8_t percent) {
    return map(percent, 0, 100, 0, 255);
}

uint8_t JaamLed::brightnessMapped(uint8_t percent) {
    return map(percent, 0, 100, 0, 100);
}

// Перевірка ініціалізації стрічки
bool JaamLed::isStripInitialized(Adafruit_NeoPixel* strip) {
    return strip != nullptr;
}

// Функція для створення стрічки з обробкою помилок
StripStatus JaamLed::createStrip(Adafruit_NeoPixel*& strip, 
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