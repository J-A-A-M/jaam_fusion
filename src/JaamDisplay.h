#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

// Supported display types
enum class JaamDisplayType {
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

class JaamDisplay {
public:
    JaamDisplay();

    void begin(JaamDisplayType type = JaamDisplayType::SSD1306, JaamDisplayHeight height = JaamDisplayHeight::HEIGHT_64);
    void clear();
    void printMessage(const String& mainText, const String& title = "");
    void printClock(const String& time, const String& date = "");
    void drawIconWithText(JaamDisplayIcon icon, const String& text);

private:
    JaamDisplayType _type = JaamDisplayType::SSD1306;
    JaamDisplayHeight _height = JaamDisplayHeight::HEIGHT_64;
    uint8_t _sda = 21, _scl = 22, _address = 0x3C;
    U8G2* _u8g2 = nullptr;

    void setupU8g2();

};