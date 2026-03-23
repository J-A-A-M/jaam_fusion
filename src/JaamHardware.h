#pragma once

#include "JaamConfig.h"

// Hardware pin constants for JAAM devices
namespace JaamHardwarePins {
    // Main LED strip pins
    constexpr int MAIN_LED_PIN_JAAM = 13;

    // Background LED strip pins
    constexpr int BG_LED_PIN_JAAM = 12;
    constexpr int BG_LED_PIN_DISABLED = -1;

    // Service LED strip pins
    constexpr int SERVICE_LED_PIN_JAAM = 25;
    constexpr int SERVICE_LED_PIN_DISABLED = -1;

    // Button pins
    constexpr int BUTTON_1_PIN_JAAM_3 = 5;
    constexpr int BUTTON_1_PIN_JAAM_2 = 4;
    constexpr int BUTTON_1_PIN_JAAM_1 = 35;

    constexpr int BUTTON_2_PIN_JAAM = 2;
    constexpr int BUTTON_2_PIN_DISABLED = -1;

    constexpr int BUTTON_3_PIN_JAAM_3 = 4;
    constexpr int BUTTON_3_PIN_DISABLED = -1;

    // Buzzer pin
    constexpr int BUZZER_PIN_JAAM = 33;

    // DFPlayer pins
    constexpr int DF_RX_PIN_JAAM = 17;
    constexpr int DF_TX_PIN_JAAM = 16;
}

// Hardware LED count constants
namespace JaamHardwareLed {
    // Main LED strip counts
    constexpr int MAIN_LED_COUNT_JAAM_3_2 = 405;
    constexpr int MAIN_LED_COUNT_JAAM_3_0 = 273;
    constexpr int MAIN_LED_COUNT_JAAM_2_1 = 26;
    constexpr int MAIN_LED_COUNT_JAAM_1_3 = 26;
    constexpr int MAIN_LED_COUNT_KYIV = 26;
    constexpr int MAIN_LED_COUNT_DEFAULT = 25;
    constexpr int MAIN_LED_COUNT_DISABLED = -1;
    constexpr int MAIN_LED_FORMAT_DISABLED = -1;

    // Background LED strip counts
    constexpr int BG_LED_COUNT_JAAM_3 = 39;
    constexpr int BG_LED_COUNT_JAAM_2 = 44;
    constexpr int BG_LED_COUNT_DISABLED = -1;
    constexpr int BG_LED_FORMAT_DISABLED = -1;

    // Service LED strip count
    constexpr int SERVICE_LED_COUNT_DEFAULT = 5;
    constexpr int SERVICE_LED_COUNT_DISABLED = -1;
    constexpr int SERVICE_LED_FORMAT_DISABLED = -1;

    // Brightness factors for different hardware types
    constexpr uint8_t BRIGHTNESS_JAAM_3_0_MAX = 35;     // ~19% of 180, consumption ~1,6A (~8W at 5V) with white at full brightness
    constexpr uint8_t BRIGHTNESS_JAAM_3_2_MAX = 35;     // ~19% of 180
    constexpr uint8_t BRIGHTNESS_JAAM_1_3_MAX = 127;    // 70% of 180, consumption ~0,6A (~3W at 5V) with white at full brightness
    constexpr uint8_t BRIGHTNESS_JAAM_2_1_MAX = 65;     // ~36% of 180, consumption ~1,4A (~7W at 5V) with white at full brightness
    constexpr uint8_t BRIGHTNESS_DEFAULT_MAX = 90;      // 50% of 180, a safer default for custom hardware

    constexpr uint8_t BRIGHTNESS_JAAM_3_0_MIN = 3;
    constexpr uint8_t BRIGHTNESS_JAAM_3_2_MIN = 3;
    constexpr uint8_t BRIGHTNESS_JAAM_1_3_MIN = 3;
    constexpr uint8_t BRIGHTNESS_JAAM_2_1_MIN = 3;

    // Absolute maximum brightness cap (safety limit, ~70% of 255)
    constexpr uint8_t BRIGHTNESS_ABSOLUTE_MAX = 180;
}

class JaamHardware {
public:
    static int getMainLedsCount();
    static int getBgLedsCount();
    static int getServiceLedsCount();
    static int getMainLedPin();
    static int getBgLedPin();
    static int getServiceLedPin();
    static int getButton1Pin();
    static int getButton2Pin();
    static int getButton3Pin();
    static int getBuzzerPin();
    static int getDfRxPin();
    static int getDfTxPin();
    static int getDisplayModel();
    static int getDisplayHeight();
    static int getDisplayRotation();
    static int getMainLedColorFormat();
    static int getBgLedColorFormat();
    static int getServiceLedColorFormat();
    static uint8_t getMaxBrightness();
    static uint8_t getMinBrightness();

private:
    static uint8_t getCurrentHardwareType();
};
