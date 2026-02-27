#pragma once

#include <Arduino.h>
#if DISPLAY_ENABLED
#include <U8g2lib.h>
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

enum class JaamDisplayRotation {
    ROTATION_0 = 0,
    ROTATION_90 = 90,
    ROTATION_180 = 180,
    ROTATION_270 = 270
};

/**
 * JaamDisplay - Display driver for Jaam Fusion.
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
    void invertDisplay(bool invert = true);
    void rotateDisplay(JaamDisplayRotation rotation = JaamDisplayRotation::ROTATION_0);
    void printMessage(const String& mainText, const String& title = "");
    void printClock(const String& time, const String& date = "");
    void drawIconWithText(JaamDisplayIcon icon, const String& text);
    void showServiceMessage(const String& message, const String& title = "", int duration = 2000);

private:
    JaamDisplayType _type = JaamDisplayType::SSD1306;
    JaamDisplayHeight _height = JaamDisplayHeight::HEIGHT_64;
    bool _isConnected = false;
    
#if DISPLAY_ENABLED
    U8G2* _u8g2 = nullptr;
#endif
    long _serviceMessageEndTime = 0;

    void _setupU8g2();
    bool _isServiceMessageActive();
    bool _checkI2CConnection();

};