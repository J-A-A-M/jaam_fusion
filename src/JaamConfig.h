#pragma once

// Strip configuration
#define NUM_LEDS_STRIP 26
#define DEFAULT_BRIGHTNESS_PERCENT 10

// Pin configuration
#define PIN_STRIP1 13
#define PIN_STRIP2 12
#define PIN_STRIP3 25

// WiFi configuration
struct WiFiConfig {
    static const bool ENABLED = true;  // Set to false to completely disable WiFi
    static const uint16_t WEB_PORT = 8080;
    static const uint32_t CONNECT_TIMEOUT = 20000;  // 20 seconds
    static const uint8_t CONNECT_RETRIES = 5;
    static const uint32_t PORTAL_TIMEOUT = 180000;  // 3 minutes
};

// Default colors for strips (RGB format)
struct DefaultColors {
    static const uint32_t STRIP1 = 0x00FF00;  // Green
    static const uint32_t STRIP2 = 0xFF0000;  // Red
    static const uint32_t STRIP3 = 0x0000FF;  // Blue
};

// Animation timing
#define ANIMATION_INTERVAL 200    // ms
#define MEMORY_CHECK_INTERVAL 60000 // ms
#define WIFI_CHECK_INTERVAL 1000 // ms



// Animation parameters
struct AnimationConfig {
    // Period range (ms)
    static const uint32_t MIN_PERIOD = 1000;
    static const uint32_t MAX_PERIOD = 1000;
    
    // Cycles range
    static const uint8_t MIN_CYCLES = 3;
    static const uint8_t MAX_CYCLES = 3;
    
    // Brightness range
    static const uint8_t MIN_START_BRIGHTNESS = 0;
    static const uint8_t MAX_START_BRIGHTNESS = 0;
    static const uint8_t MIN_END_BRIGHTNESS = 250;
    static const uint8_t MAX_END_BRIGHTNESS = 250;
};

// Error codes
enum class ErrorCode {
    SUCCESS = 0,
    STRIP_CREATION_FAILED,
    ANIMATION_CREATION_FAILED,
    NULL_STRIP_POINTER,
    MUTEX_CREATION_FAILED
}; 