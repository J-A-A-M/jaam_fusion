#include "JaamLed.h"
#include "JaamUtils.h"

extern uint8_t legacy;

JaamLed::JaamLed() : settings(nullptr) {}

void JaamLed::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

uint8_t JaamLed::brightnessAbsolute(uint8_t percent) {
    // Використовуємо пряму параболічну залежність
    float normalized = percent / 100.0f;
    float squared = normalized * normalized;
    return (uint8_t)(squared * 255.0f);
}

uint8_t JaamLed::brightnessMapped(uint8_t percent) {
    if (legacy == 4) {
        return map(percent, 0, 100, 0, 40);
    } else {
        return map(percent, 0, 100, 0, 100);
    }
}

// Перевірка ініціалізації стрічки
bool JaamLed::isStripInitialized(Adafruit_NeoPixel* strip) {
    return strip != nullptr;
}

// Функція для створення стрічки з обробкою помилок
StripStatus JaamLed::createStrip(Adafruit_NeoPixel*& strip, 
                    int pin, 
                    uint32_t count, 
                    uint8_t brightness,
                    uint32_t color, 
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
    strip->setBrightness(brightnessMapped(brightness));
    strip->clear();
    
    for(int i = 0; i < count; i++) {
        strip->setPixelColor(i, color);
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
                                 uint8_t brightness,
                                 uint32_t color,
                                 uint8_t type) {
    LOG.printf("[LED] Recreating strip: pin=%d, count=%d\n", pin, count);
    
    // Analyze memory fragmentation before operation
    analyzeMemoryFragmentation("before strip recreation");
    
    // First, safely destroy existing strip
    destroyStrip(strip);
    
    // Force memory defragmentation after cleanup
    defragmentMemory("after strip destruction");
    
    // Create new strip
    StripStatus result = createStrip(strip, pin, count, brightness, color, type);
    
    // Log final memory state
    logMemoryUsage("after strip recreation");
    
    return result;
}