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
namespace JaamHardwareLedCounts {
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
    static const RegionLedMapEntry* getRegionMap();
    static int getMainLedColorFormat();
    static int getBgLedColorFormat();
    static int getServiceLedColorFormat();
    
private:
    static uint8_t getCurrentHardwareType();
};
