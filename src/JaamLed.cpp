#include "JaamLed.h"
#include "JaamUtils.h"
#include "JaamHardware.h"
#include "JaamSettings.h"

extern JaamHardware hardwareConfig;
extern JaamSettings settings;

JaamLed::JaamLed() {}

uint8_t JaamLed::brightnessRelative(uint8_t percentLocal) {
    uint8_t percentCommon = settings.getInt(CURRENT_BRIGHTNESS);
    if (percentCommon == 0 || percentLocal == 0) return 0; // Швидкий вихід для 0% яскравості
    if (percentCommon > 100) percentCommon = 100;
    if (percentLocal > 100) percentLocal = 100;
    uint8_t combinedPercent = (percentCommon * percentLocal) / 100;
    return brightnessMapped(combinedPercent);
}

uint8_t JaamLed::brightnessParabolic(uint8_t percent) {
    if (percent == 0) return 0; // Швидкий вихід для 0% яскравості
    // // Використовуємо пряму параболічну залежність
    float normalized = percent / 100.0f;
    float squared = normalized * normalized;
    return (uint8_t)(squared * 255.0f);
}

uint8_t JaamLed::brightnessMapped(uint8_t percent) {
    if (percent == 0) return 0; // Швидкий вихід для 0% яскравості
    if (percent > 100) percent = 100;
    uint8_t maxBrightness = hardwareConfig.getMaxBrightness();
    uint8_t minBrightness = hardwareConfig.getMinBrightness();
    // Використовуємо функцію map для лінійного масштабування відсотків до діапазону мінімальної та максимальної яскравості,
    // визначених для поточного апаратного забезпечення
    return (uint8_t) map(percent, 1, 100, minBrightness, maxBrightness);
}

// Перевірка ініціалізації стрічки
bool JaamLed::isStripInitialized(Adafruit_NeoPixel* strip) {
    return strip != nullptr;
}

// Функція для створення стрічки з обробкою помилок
StripStatus JaamLed::createStrip(Adafruit_NeoPixel*& strip, 
                    int pin, 
                    uint32_t count, 
                    uint8_t type) {
    if (pin <= 0) {
        return StripStatus::STRIP_PIN_NOT_SET;
    }
    
    // Estimate memory needed for NeoPixel strip
    size_t estimatedSize = sizeof(Adafruit_NeoPixel) + (count * 3); // Approximate memory for RGB data
    
    // Check if we can allocate memory before attempting
    if (!canAllocateMemory(estimatedSize, "NeoPixel creation")) {
        LOG.printf("[LED] Cannot create strip - insufficient contiguous memory\n");
        return StripStatus::STRIP_CREATION_FAILED;
    }
    
    strip = new(std::nothrow) Adafruit_NeoPixel(count, pin, type);
    if (strip == nullptr) {
        LOG.printf("[LED] Strip creation failed despite memory check\n");
        return StripStatus::STRIP_CREATION_FAILED;
    }
    
    strip->begin();
    strip->clear();
    strip->setBrightness(255); // Start with max brightness for color setting

    for(int i = 0; i < count; i++) {
        strip->setPixelColor(i, DefaultColors::OFF);
    }
    strip->show();
    return StripStatus::SUCCESS;
}

// Функція для безпечного видалення стрічки
void JaamLed::destroyStrip(Adafruit_NeoPixel*& strip) {
    if (strip != nullptr) {
        LOG.printf("[LED] Destroying strip at address: %p\n", (void*)strip);
        
        // Clear strip before deletion to avoid potential issues
        strip->clear();
        strip->show();
        
        // Add small delay to ensure operations complete
        delay(50);
        
        // Log memory before deletion
        size_t memBefore = ESP.getFreeHeap();
        
        delete strip;
        strip = nullptr;
        
        // Force yield to allow system cleanup
        yield();
        delay(10);
        
        size_t memAfter = ESP.getFreeHeap();
        LOG.printf("[LED] Strip destroyed, memory freed: %d bytes, pointer set to nullptr\n", 
                  (int)(memAfter - memBefore));
    }
}

// Add memory cleanup verification
void JaamLed::verifyStripCleanup(Adafruit_NeoPixel* strip) {
    if (strip != nullptr) {
        LOG.printf("[LED] WARNING: Strip pointer not cleaned up properly! Address: %p\n", (void*)strip);
    } else {
        LOG.printf("[LED] Strip cleanup verified - pointer is nullptr\n");
    }
}

// Add strip recreation with proper cleanup
StripStatus JaamLed::recreateStrip(Adafruit_NeoPixel*& strip,
                                 int pin,
                                 uint32_t count,
                                 uint8_t type) {
    LOG.printf("[LED] Recreating strip: pin=%d, count=%d\n", pin, count);
    
    // Analyze memory fragmentation before operation
    analyzeMemoryFragmentation("before strip recreation");
    
    // First, safely destroy existing strip
    destroyStrip(strip);
    
    // Force memory defragmentation after cleanup
    defragmentMemory("after strip destruction");
    
    // Create new strip
    StripStatus result = createStrip(strip, pin, count, type);
    
    // Log final memory state
    logMemoryUsage("after strip recreation");
    
    return result;
}