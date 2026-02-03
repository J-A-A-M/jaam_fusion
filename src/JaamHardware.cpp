#include "JaamHardware.h"
#include "JaamSettings.h"
#include "JaamDisplay.h"

extern JaamSettings settings;

uint8_t JaamHardware::getCurrentHardwareType() {
    return settings.getInt(HARDWARE);
}

int JaamHardware::getMainLedPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
            return 13;
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(MAIN_LED_PIN);
        default:
            return settings.getInt(MAIN_LED_PIN);
    }
}

int JaamHardware::getMainLedsCount() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
            return 405;
        case HARDWARE::JAAM_3_0:
            return 273;
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
            return 26;
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return 25;
        default:
            return 25;
    }
}

int JaamHardware::getBgLedPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return 12;
        case HARDWARE::JAAM_1_3:
            return -1; // No BG LEDs
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(BG_LED_PIN);
        default:
            return settings.getInt(BG_LED_PIN);
    }
}

int JaamHardware::getBgLedsCount() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_3_0:
            return 39;
        case HARDWARE::JAAM_2_1:
            return 44;
        case HARDWARE::JAAM_1_3:
            return -1; // No BG LEDs
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(BG_LED_COUNT);
        default:
            return settings.getInt(BG_LED_COUNT);
    }
}

int JaamHardware::getServiceLedPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return 25;
        case HARDWARE::JAAM_1_3:
            return -1; // No Service LEDs
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(SERVICE_LED_PIN);
        default:
            return settings.getInt(SERVICE_LED_PIN);
    }
}

int JaamHardware::getServiceLedsCount() {
    uint8_t hwType = getCurrentHardwareType();
    int serviceLedCount = 0;
    if (settings.getInt(SERVICE_LED_PIN) > 0) {
        serviceLedCount = 5;
    }

    switch (hwType) {
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return 5;
        case HARDWARE::JAAM_1_3:
            return -1; // No Service LEDs
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return serviceLedCount;
        default:
            return serviceLedCount;
    }
}

int JaamHardware::getButton1Pin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_3_0:
            return 5;
        case HARDWARE::JAAM_2_1:
            return 4; 
        case HARDWARE::JAAM_1_3:
            return 35;
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(BUTTON_1_PIN);
        default:
            return settings.getInt(BUTTON_1_PIN);
    }
}

int JaamHardware::getButton2Pin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return 2;
        case HARDWARE::JAAM_1_3:
            return -1; // No Button 2
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(BUTTON_2_PIN);
        default:
            return settings.getInt(BUTTON_2_PIN);
    }
}

int JaamHardware::getButton3Pin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_3_0:
            return 4;
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
            return -1; // No Button 3
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(BUTTON_3_PIN);
        default:
            return settings.getInt(BUTTON_3_PIN);
    }
}

int JaamHardware::getBuzzerPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_2_1:
            return 33;
        case HARDWARE::JAAM_1_3:
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(BUZZER_PIN);
        default:
            return settings.getInt(BUZZER_PIN);
    }
}

int JaamHardware::getDfRxPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_3_1:
            return 17;
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(DF_RX_PIN);
        default:
            return settings.getInt(DF_RX_PIN);
    }
}

int JaamHardware::getDfTxPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
        case HARDWARE::JAAM_3_0:
            return 16;
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(DF_TX_PIN);
        default:
            return settings.getInt(DF_TX_PIN);
    }
}

int JaamHardware::getDisplayModel() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
            return settings.getInt(DISPLAY_MODEL);
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return static_cast<int>(JaamDisplayType::SH1106G);
        case HARDWARE::JAAM_1_3:
            return static_cast<int>(JaamDisplayType::SSD1306);
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(DISPLAY_MODEL);
        default:
            return settings.getInt(DISPLAY_MODEL);
    }
}

int JaamHardware::getDisplayHeight() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_1:
            return settings.getInt(DISPLAY_HEIGHT);
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
            return static_cast<int>(JaamDisplayHeight::HEIGHT_64);
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
        case HARDWARE::ODESA:
        case HARDWARE::ZAKARPATTIA:
            return settings.getInt(DISPLAY_HEIGHT);
        default:
            return settings.getInt(DISPLAY_HEIGHT);
    }
}

