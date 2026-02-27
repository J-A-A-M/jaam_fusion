#include "JaamDisplay.h"
#include "JaamLogs.h"
#include <Wire.h>

static const unsigned char trident[] PROGMEM = {
    0x20, 0x00, 0x01, 0x08, 0x60, 0x80, 0x03, 0x0c, 0xe0, 0x80, 0x03, 0x0e, 0xe0, 0x81, 0x03, 0x0f,
    0xe0, 0x83, 0x83, 0x0f, 0x60, 0x83, 0x83, 0x0d, 0x60, 0x87, 0xc3, 0x0d, 0x60, 0x86, 0xc3, 0x0c,
    0x60, 0x8e, 0xe3, 0x0c, 0x60, 0x8c, 0x63, 0x0c, 0x60, 0x8c, 0x63, 0x0c, 0x60, 0x8c, 0x63, 0x0c,
    0x60, 0x8c, 0x63, 0x0c, 0x60, 0x8c, 0x63, 0x0c, 0x60, 0x8f, 0xe3, 0x0d, 0xe0, 0x87, 0xc3, 0x0f,
    0xe0, 0xc1, 0x07, 0x0f, 0xe0, 0xc0, 0x06, 0x0e, 0xe0, 0xe1, 0x0e, 0x0f, 0xe0, 0x63, 0x8c, 0x0f,
    0x60, 0x77, 0xdc, 0x0d, 0x60, 0xfe, 0xfe, 0x0c, 0x60, 0xbc, 0x7b, 0x0c, 0x60, 0x98, 0x33, 0x0c,
    0xe0, 0xff, 0xff, 0x0f, 0xc0, 0xff, 0xff, 0x07, 0x80, 0x3f, 0xf9, 0x03, 0x00, 0x70, 0x1d, 0x00,
    0x00, 0xe0, 0x0f, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00
};
#if DISPLAY_ENABLED
#define JAAM_FONT_TITLE u8g2_font_6x12_t_cyrillic
#define JAAM_FONT_CLOCK_64 u8g2_font_osr35_tn
#define JAAM_FONT_CLOCK_32 u8g2_font_osr21_tn
static const uint8_t* JAAM_FONT_SIZES[] = {
    u8g2_font_inr38_t_cyrillic, // 38
    u8g2_font_inr33_t_cyrillic, // 33
    u8g2_font_inr30_t_cyrillic, // 30
    u8g2_font_inr27_t_cyrillic, // 27
    u8g2_font_inr24_t_cyrillic, // 24
    u8g2_font_10x20_t_cyrillic, // 20
    u8g2_font_9x15_t_cyrillic,  // 15
    u8g2_font_6x13_t_cyrillic,  // 13
    u8g2_font_6x12_t_cyrillic,  // 12
};
static const uint8_t JAAM_FONT_SIZES_COUNT = 9;

// Alternative font sizes for 32-height displays (max font height: 20px)
static const uint8_t* JAAM_FONT_SIZES_32[] = {
    u8g2_font_10x20_t_cyrillic, // 20
    u8g2_font_9x15_t_cyrillic,  // 15
    u8g2_font_6x13_t_cyrillic,  // 13
    u8g2_font_6x12_t_cyrillic,  // 12
    u8g2_font_5x8_t_cyrillic,   // 8
};
#endif
static const uint8_t JAAM_FONT_SIZES_32_COUNT = 5;

// Helper function to get appropriate font array and count based on display height
static void getFontArrayForHeight(JaamDisplayHeight height, const uint8_t**& fontArray, uint8_t& fontCount) {
#if DISPLAY_ENABLED
    if (height == JaamDisplayHeight::HEIGHT_32) {
        fontArray = JAAM_FONT_SIZES_32;
        fontCount = JAAM_FONT_SIZES_32_COUNT;
    } else {
        fontArray = JAAM_FONT_SIZES;
        fontCount = JAAM_FONT_SIZES_COUNT;
    }
#endif
}

JaamDisplay::JaamDisplay() {}

JaamDisplay::~JaamDisplay() {
#if DISPLAY_ENABLED
    if (_u8g2) {
        delete _u8g2;
        _u8g2 = nullptr;
    }
#endif
}

JaamDisplay::JaamDisplay(JaamDisplay&& other) noexcept
    : _type(other._type), _height(other._height)
#if DISPLAY_ENABLED
    , _u8g2(other._u8g2)
#endif
{
#if DISPLAY_ENABLED
    other._u8g2 = nullptr;
#endif
}

JaamDisplay& JaamDisplay::operator=(JaamDisplay&& other) noexcept {
    if (this != &other) {
#if DISPLAY_ENABLED
        // Clean up existing resources
        if (_u8g2) {
            delete _u8g2;
        }
        
        // Move data from other
        _type = other._type;
        _height = other._height;
        _u8g2 = other._u8g2;
        
        // Reset other
        other._u8g2 = nullptr;
#else
        _type = other._type;
        _height = other._height;
#endif
    }
    return *this;
}

void JaamDisplay::begin(JaamDisplayType type, JaamDisplayHeight height) {
#if DISPLAY_ENABLED
    if (_u8g2) {
        clear();
        LOG.printf("[DISPLAY] Clearing existing display\n");
        delete _u8g2;
        _u8g2 = nullptr;
        LOG.printf("[DISPLAY] Cleaned up existing U8G2 object\n");
    }
#endif

    _type = type;
    _height = height;
    _isConnected = false;
    
    // Check if display is physically connected
    if (!_checkI2CConnection()) {
        LOG.printf("[DISPLAY] Display not detected on I2C bus - skipping initialization\n");
        return;
    }
    
#if DISPLAY_ENABLED
    _setupU8g2();
    if (_u8g2) {
        _u8g2->begin();
        _u8g2->enableUTF8Print(); // Enable UTF-8 support for printing
        _isConnected = true;
        LOG.printf("[DISPLAY] Display successfully initialized\n");
    } else {
        LOG.printf("[DISPLAY] Failed to initialize U8G2 object\n");
    }
#else
    LOG.printf("[DISPLAY] Display support is disabled (DISPLAY_ENABLED=0)\n");
#endif
}

void JaamDisplay::clear() {
    if (!_isConnected) return;
#if DISPLAY_ENABLED
    if (_u8g2) {
        _u8g2->clearBuffer();
        _u8g2->sendBuffer();
    }
#endif
}

void JaamDisplay::invertDisplay(bool invert) {
    if (!_isConnected) return;
#if DISPLAY_ENABLED
    if (_u8g2) {
        if (invert) {
            _u8g2->sendF("c", 0xa7);  // Set inverse display on
        } else {
            _u8g2->sendF("c", 0xa6);  // Set normal display
        }
        LOG.printf("[DISPLAY] Display color inversion %s\n", invert ? "enabled" : "disabled");
    }
#endif
}

void JaamDisplay::rotateDisplay(JaamDisplayRotation rotation) {
    if (!_isConnected) return;
#if DISPLAY_ENABLED
    if (_u8g2) {
        const u8g2_cb_t* u8g2_rotation;
        switch (rotation) {
            case JaamDisplayRotation::ROTATION_0:
                u8g2_rotation = U8G2_R0;
                break;
            case JaamDisplayRotation::ROTATION_90:
                u8g2_rotation = U8G2_R1;
                break;
            case JaamDisplayRotation::ROTATION_180:
                u8g2_rotation = U8G2_R2;
                break;
            case JaamDisplayRotation::ROTATION_270:
                u8g2_rotation = U8G2_R3;
                break;
            default:
                u8g2_rotation = U8G2_R0;
                break;
        }
        _u8g2->setDisplayRotation(u8g2_rotation);
        LOG.printf("[DISPLAY] Display rotation set to %d degrees\n", (int)rotation);
    }
#endif
}

void JaamDisplay::_setupU8g2() {
#if DISPLAY_ENABLED
    if (_u8g2) return; // Already initialized

    LOG.printf("[DISPLAY] setupU8g2: type=%d height=%d\n", (int)_type, (int)_height);
    
    // Select constructor based on type/height
    switch (_type) {
        case JaamDisplayType::NONE:
            LOG.printf("[DISPLAY] setupU8g2: Display type is NONE - skipping U8G2 initialization\n");
            return;
        case JaamDisplayType::SSD1306:
            if (_height == JaamDisplayHeight::HEIGHT_64) {
                _u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0);
                LOG.printf("[DISPLAY] Using U8G2_SSD1306_128X64_NONAME_F_HW_I2C\n");
            } else {
                _u8g2 = new U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C(U8G2_R0);
                LOG.printf("[DISPLAY] Using U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C\n");
            }
            break;
        case JaamDisplayType::SH1106G:
            if (_height == JaamDisplayHeight::HEIGHT_64) {
                _u8g2 = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0);
                LOG.printf("[DISPLAY] Using U8G2_SH1106_128X64_NONAME_F_HW_I2C\n");
            } else {
                _u8g2 = new U8G2_SH1106_128X32_VISIONOX_F_HW_I2C(U8G2_R0);
                LOG.printf("[DISPLAY] Using U8G2_SH1106_128X32_VISIONOX_F_HW_I2C\n");
            }
            break;
        case JaamDisplayType::SH1107:
            if (_height == JaamDisplayHeight::HEIGHT_64) {
                _u8g2 = new U8G2_SH1107_128X128_1_HW_I2C(U8G2_R0);
                LOG.printf("[DISPLAY] Using U8G2_SH1107_128X128_1_HW_I2C\n");
            } else {
                // SH1107 128x32 is rare, fallback to 64
                _u8g2 = new U8G2_SH1107_128X80_2_HW_I2C(U8G2_R0);
                LOG.printf("[DISPLAY] Using U8G2_SH1107_128X80_2_HW_I2C (fallback)\n");
            }
            break;
    }
#endif
}

void JaamDisplay::printMessage(const String& mainText, const String& title) {
    if (!_isConnected) return;
#if DISPLAY_ENABLED
    if (!_u8g2) return;
    if (_isServiceMessageActive()) {
        LOG.printf("[DISPLAY] Skipping printMessage, service message is active\n");
        return;
    }
    _u8g2->clearBuffer();

    uint8_t dispWidth = 128;
    uint8_t dispHeight = (uint8_t)_height;

    // Draw title if provided
    uint8_t titleHeight = 0;
    if (!title.isEmpty()) {
        _u8g2->setFont(JAAM_FONT_TITLE);
        _u8g2->setCursor(3, _u8g2->getAscent());
        _u8g2->print(title);
        titleHeight = _u8g2->getAscent() + 2;
    }

    // Prepare main text
    String line1 = mainText;
    String line2 = "";

    // Get appropriate font array based on display height
    const uint8_t** fontArray;
    uint8_t fontCount;
    getFontArrayForHeight(_height, fontArray, fontCount);

    // Try to fit mainText in one line, if not, split to two lines and recalc font size
    int fontIdx = 0;
    int textWidth = 0;
    int textHeight = 0;
    bool fits = false;
    for (; fontIdx < fontCount; ++fontIdx) {
        _u8g2->setFont(fontArray[fontIdx]);
        textWidth = _u8g2->getUTF8Width(mainText.c_str());
        textHeight = _u8g2->getAscent();
        if (textWidth <= dispWidth - 2) {
            fits = true;
            break;
        }
    }
    if (!fits) {
        // Need to split to two lines and recalc font size
        // Find a space near the middle to split
        int mid = mainText.length() / 2;
        int splitPos = mainText.lastIndexOf(' ', mid);
        if (splitPos == -1) splitPos = mainText.indexOf(' ', mid);
        if (splitPos == -1) splitPos = mid; // fallback: split at mid
        line1 = mainText.substring(0, splitPos);
        line2 = mainText.substring(splitPos);
        line2.trim();

        // Now find font size that fits both lines
        for (fontIdx = 0; fontIdx < fontCount; ++fontIdx) {
            _u8g2->setFont(fontArray[fontIdx]);
            int w1 = _u8g2->getUTF8Width(line1.c_str());
            int w2 = _u8g2->getUTF8Width(line2.c_str());
            int h = _u8g2->getAscent();
            if (w1 <= dispWidth - 2 && w2 <= dispWidth - 2 && (h * 2 + titleHeight + 4) <= dispHeight) {
                fits = true;
                break;
            }
        }
        if (fontIdx == fontCount) fontIdx = fontCount - 1; // fallback to smallest
    }

    _u8g2->setFont(fontArray[fontIdx]);
    int mainFontHeight = _u8g2->getAscent();

    if (line2.isEmpty()) {
        // One line, center vertically (below title if present)
        int y = (dispHeight + mainFontHeight) / 2;
        if (titleHeight > 0) {
            // Center in remaining area below title
            int availHeight = dispHeight - titleHeight;
            y = titleHeight + (availHeight + mainFontHeight) / 2;
        }
        int x = (dispWidth - _u8g2->getUTF8Width(line1.c_str())) / 2;
        _u8g2->setCursor(x > 0 ? x : 0, y);
        _u8g2->print(line1);
    } else {
        // Two lines, center both
        int totalTextHeight = mainFontHeight * 2 + 4;
        int y1 = titleHeight + (dispHeight - titleHeight - totalTextHeight) / 2 + mainFontHeight;
        int y2 = y1 + mainFontHeight + 4;
        int x1 = (dispWidth - _u8g2->getUTF8Width(line1.c_str())) / 2;
        int x2 = (dispWidth - _u8g2->getUTF8Width(line2.c_str())) / 2;
        _u8g2->setCursor(x1 > 0 ? x1 : 0, y1);
        _u8g2->print(line1);
        _u8g2->setCursor(x2 > 0 ? x2 : 0, y2);
        _u8g2->print(line2);
    }

    _u8g2->sendBuffer();
#endif
}

// Draw a monochrome bitmap icon at (x, y) with width w and height h
void JaamDisplay::drawIconWithText(JaamDisplayIcon iconType, const String& text) {
    if (!_isConnected) return;
#if DISPLAY_ENABLED
    if (!_u8g2) return;
    if (_isServiceMessageActive()) {
        LOG.printf("[DISPLAY] Skipping drawIconWithText, service message is active\n");
        return;
    }

    const uint8_t* icon = nullptr;
    switch (iconType) {
        case JaamDisplayIcon::TRIDENT:
        default:
            icon = trident;
            break;
    }
    uint8_t iconW = 32;
    uint8_t iconH = 32;

    _u8g2->clearBuffer();

    uint8_t dispWidth = 128;
    uint8_t dispHeight = (uint8_t)_height;

    // Icon position: left-middle
    uint8_t iconX = 0;
    uint8_t iconY = (dispHeight - iconH) / 2;
    _u8g2->drawXBMP(iconX, iconY, iconW, iconH, icon);

    // Text area: to the right of the icon
    uint8_t textX = iconW + 2;
    uint8_t textW = dispWidth - textX;
    uint8_t textY = 0;

    // Try to fit text in 1, 2, or 3 lines with dynamic font size
    String lines[3];
    int linesCount = 1;
    int fontIdx = 0;
    bool fits = false;

    // Get appropriate font array based on display height
    const uint8_t** fontArray;
    uint8_t fontCount;
    getFontArrayForHeight(_height, fontArray, fontCount);

    // Try 1 line, but only allow if font is not the smallest
    for (fontIdx = 0; fontIdx < fontCount; ++fontIdx) {
        _u8g2->setFont(fontArray[fontIdx]);
        int w = _u8g2->getUTF8Width(text.c_str());
        int h = _u8g2->getAscent();
        if (w <= textW && h <= dispHeight - 2) {
            // Only allow 1-line if font is not the smallest
            if (fontIdx < fontCount - 1) {
                lines[0] = text;
                linesCount = 1;
                fits = true;
                break;
            }
            // else: force split below
        }
    }

    // Try 2 lines if not fit
    if (!fits) {
        // Split text into two lines near the middle
        int mid = text.length() / 2;
        int splitPos = text.lastIndexOf(' ', mid);
        if (splitPos == -1) splitPos = text.indexOf(' ', mid);
        if (splitPos == -1) splitPos = mid;
        lines[0] = text.substring(0, splitPos);
        lines[1] = text.substring(splitPos);
        lines[1].trim();
        for (fontIdx = 0; fontIdx < fontCount; ++fontIdx) {
            _u8g2->setFont(fontArray[fontIdx]);
            int w1 = _u8g2->getUTF8Width(lines[0].c_str());
            int w2 = _u8g2->getUTF8Width(lines[1].c_str());
            int h = _u8g2->getAscent();
            if (w1 <= textW && w2 <= textW && (h * 2 + 2) <= dispHeight) {
                linesCount = 2;
                fits = true;
                break;
            }
        }
        // Only allow 2 lines if font is not the smallest
        if (!fits && fontIdx == fontCount) fontIdx = fontCount - 1;
    }

    // Try 3 lines if still not fit
    if (!fits) {
        // Smart split into 3 lines
        int textLen = text.length();
        int part = textLen / 3;
        
        // Find first split point around 1/3
        int split1 = text.indexOf(' ', part);
        if (split1 == -1) {
            split1 = text.lastIndexOf(' ', part);
            if (split1 == -1) split1 = part; // fallback
        }
        
        // Find second split point around 2/3, but start after split1
        int searchStart = split1 + 1;
        int target2 = (textLen * 2) / 3;
        int split2 = text.indexOf(' ', target2);
        if (split2 == -1 || split2 <= split1) {
            split2 = text.lastIndexOf(' ', target2);
            if (split2 == -1 || split2 <= split1) {
                split2 = split1 + (textLen - split1) / 2; // fallback: split remaining text in half
            }
        }
        
        lines[0] = text.substring(0, split1);
        lines[1] = text.substring(split1, split2);
        lines[2] = text.substring(split2);
        lines[1].trim();
        lines[2].trim();
        for (fontIdx = 0; fontIdx < fontCount; ++fontIdx) {
            _u8g2->setFont(fontArray[fontIdx]);
            int w1 = _u8g2->getUTF8Width(lines[0].c_str());
            int w2 = _u8g2->getUTF8Width(lines[1].c_str());
            int w3 = _u8g2->getUTF8Width(lines[2].c_str());
            int h = _u8g2->getAscent();
            if (w1 <= textW && w2 <= textW && w3 <= textW && (h * 3 + 2) <= dispHeight) {
                linesCount = 3;
                fits = true;
                break;
            }
        }
        if (!fits) fontIdx = fontCount - 1; // fallback to smallest
    }

    // Set the font and calculate line heights
    _u8g2->setFont(fontArray[fontIdx]);
    int lineHeight = _u8g2->getAscent();
    int lineSpacing = 5; // Space between lines

    // Vertically center the text block in the display
    int totalTextHeight = lineHeight * linesCount + lineSpacing * (linesCount - 1);
    int baseY = (dispHeight - totalTextHeight) / 2 + lineHeight;

    // Draw the lines, left-aligned in the text area (next to icon)
    for (int i = 0; i < linesCount; ++i) {
        int x = textX;
        int y = baseY + i * (lineHeight + lineSpacing);
        _u8g2->setCursor(x, y);
        _u8g2->print(lines[i]);
    }

    _u8g2->sendBuffer();
#endif
}

void JaamDisplay::printClock(const String& time, const String& date) {
    if (!_isConnected) return;
#if DISPLAY_ENABLED
    if (!_u8g2) return;
    if (_isServiceMessageActive()) {
        LOG.printf("[DISPLAY] Skipping printClock, service message is active\n");
        return;
    }
    _u8g2->clearBuffer();

    uint8_t dispWidth = 128;
    uint8_t dispHeight = (uint8_t)_height;

    // Draw date title if provided
    uint8_t titleHeight = 0;
    if (!date.isEmpty()) {
        _u8g2->setFont(JAAM_FONT_TITLE);
        _u8g2->setCursor(3, _u8g2->getAscent());
        _u8g2->print(date);
        titleHeight = _u8g2->getAscent() + 2;
    }

    // Draw time using clock font (always single line)
    if (_height == JaamDisplayHeight::HEIGHT_32) {
        _u8g2->setFont(JAAM_FONT_CLOCK_32);
    } else {
        // Default to 64 height
        _u8g2->setFont(JAAM_FONT_CLOCK_64);
    }
    int timeWidth = _u8g2->getUTF8Width(time.c_str());
    int timeFontHeight = _u8g2->getAscent();

    // Center time vertically (below title if present)
    int y = (dispHeight + timeFontHeight) / 2;
    if (titleHeight > 0) {
        // Center in remaining area below title
        int availHeight = dispHeight - titleHeight;
        y = titleHeight + (availHeight + timeFontHeight) / 2;
    }
    
    // Center time horizontally
    int x = (dispWidth - timeWidth) / 2;
    _u8g2->setCursor(x > 0 ? x : 0, y);
    _u8g2->print(time);

    _u8g2->sendBuffer();
#endif
}

void JaamDisplay::showServiceMessage(const String& message, const String& title, int duration) {
    if (!_isConnected) return;
#if DISPLAY_ENABLED
    if (!_u8g2) return;
#endif
    _serviceMessageEndTime = 0; // Reset end time
    printMessage(message, title);
    _serviceMessageEndTime = millis() + duration;
}

bool JaamDisplay::_isServiceMessageActive() {
    return millis() < _serviceMessageEndTime;
}

bool JaamDisplay::_checkI2CConnection() {
    // Common I2C addresses for OLED displays
    uint8_t addresses[] = {0x3C, 0x3D};
    
    for (uint8_t addr : addresses) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            LOG.printf("[DISPLAY] Display detected at I2C address 0x%02X\n", addr);
            return true;
        }
    }
    
    LOG.printf("[DISPLAY] No display detected at common I2C addresses (0x3C, 0x3D)\n");
    return false;
}
