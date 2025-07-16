#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

// Board-specific I2C pin definitions
#if defined(ARDUINO_ESP32S3_DEV)
    #define DEFAULT_SDA_PIN 8
    #define DEFAULT_SCL_PIN 9
    #define BOARD_NAME "ESP32-S3"
#elif defined(ARDUINO_ESP32C3_DEV)
    #define DEFAULT_SDA_PIN 5
    #define DEFAULT_SCL_PIN 6
    #define BOARD_NAME "ESP32-C3"
#else
    #define DEFAULT_SDA_PIN 21
    #define DEFAULT_SCL_PIN 22
    #define BOARD_NAME "ESP32"
#endif

// Supported display types
enum class JaamDisplayType {
    NONE = 0,
    SSD1306 = 1,
    SH1106G = 2,
    SH1107 = 3
};

enum class JaamDisplayHeight {
    HEIGHT_32 = 32,
    HEIGHT_64 = 64
};

enum class JaamDisplayIcon {
    TRIDENT = 1
};

/**
 * JaamDisplay - Display driver for Jaam Fusion.
 * 
 * Supported boards with automatic I2C pin detection:
 * - ESP32: SDA=21, SCL=22
 * - ESP32-S3: SDA=8, SCL=9  
 * - ESP32-C3: SDA=5, SCL=6
 * 
 * Pins are automatically configured at compile time based on board type.
 */
class JaamDisplay {
public:
    JaamDisplay();
    ~JaamDisplay();
    
    // Delete copy constructor and copy assignment operator (rule of five)
    JaamDisplay(const JaamDisplay&) = delete;
    JaamDisplay& operator=(const JaamDisplay&) = delete;
    
    // Move constructor and move assignment operator
    JaamDisplay(JaamDisplay&& other) noexcept;
    JaamDisplay& operator=(JaamDisplay&& other) noexcept;

    void begin(JaamDisplayType type = JaamDisplayType::SSD1306, JaamDisplayHeight height = JaamDisplayHeight::HEIGHT_64);
    void clear();
    void printMessage(const String& mainText, const String& title = "");
    void printClock(const String& time, const String& date = "");
    void drawIconWithText(JaamDisplayIcon icon, const String& text);

private:
    JaamDisplayType _type = JaamDisplayType::SSD1306;
    JaamDisplayHeight _height = JaamDisplayHeight::HEIGHT_64;
    
    // I2C configuration using board-specific defaults
    uint8_t _sda = DEFAULT_SDA_PIN;
    uint8_t _scl = DEFAULT_SCL_PIN;
    uint8_t _address = 0x3C;
    
    U8G2* _u8g2 = nullptr;

    void setupU8g2();

};