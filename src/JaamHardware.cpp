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
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
            return JaamHardwarePins::MAIN_LED_PIN_JAAM;
        default:
            return settings.getInt(MAIN_LED_PIN);
    }
}

int JaamHardware::getMainLedsCount() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
            return JaamHardwareLed::MAIN_LED_COUNT_JAAM_3_2;
        case HARDWARE::JAAM_3_0:
            return JaamHardwareLed::MAIN_LED_COUNT_JAAM_3_0;
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::ZAKARPATTIA_KYIV:
            return JaamHardwareLed::MAIN_LED_COUNT_KYIV;
        case HARDWARE::CUSTOM_MAPPING:
            return settings.getInt(MAIN_LED_COUNT);
        default:
            return JaamHardwareLed::MAIN_LED_COUNT_DEFAULT;
    }
}

int JaamHardware::getMainLedColorFormat() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
            return NEO_GRB;
        default:
            return settings.getInt(MAIN_LED_COLOR_FORMAT);
    }
}

int JaamHardware::getBgLedPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return JaamHardwarePins::BG_LED_PIN_JAAM;
        case HARDWARE::JAAM_1_3:
            return JaamHardwarePins::BG_LED_PIN_DISABLED;
        default:
            return settings.getInt(BG_LED_PIN);
    }
}

int JaamHardware::getBgLedsCount() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
            return JaamHardwareLed::BG_LED_COUNT_JAAM_3;
        case HARDWARE::JAAM_2_1:
            return JaamHardwareLed::BG_LED_COUNT_JAAM_2;
        case HARDWARE::JAAM_1_3:
            return JaamHardwareLed::BG_LED_COUNT_DISABLED;
        default:
            return settings.getInt(BG_LED_COUNT);
    }
}

int JaamHardware::getBgLedColorFormat() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return NEO_GRB;
        case HARDWARE::JAAM_1_3:
            return JaamHardwareLed::BG_LED_FORMAT_DISABLED;
        default:
            return settings.getInt(BG_LED_COLOR_FORMAT);
    }
}

int JaamHardware::getServiceLedPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return JaamHardwarePins::SERVICE_LED_PIN_JAAM;
        case HARDWARE::JAAM_1_3:
            return JaamHardwarePins::SERVICE_LED_PIN_DISABLED;
        default:
            return settings.getInt(SERVICE_LED_PIN);
    }
}

int JaamHardware::getServiceLedsCount() {
    uint8_t hwType = getCurrentHardwareType();
    int serviceLedCount = 0;
    if (settings.getInt(SERVICE_LED_PIN) > 0) {
        serviceLedCount = JaamHardwareLed::SERVICE_LED_COUNT_DEFAULT;
    }

    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return JaamHardwareLed::SERVICE_LED_COUNT_DEFAULT;
        case HARDWARE::JAAM_1_3:
            return JaamHardwareLed::SERVICE_LED_COUNT_DISABLED;
        default:
            return serviceLedCount;
    }
}

int JaamHardware::getServiceLedColorFormat() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return NEO_GRB;
        case HARDWARE::JAAM_1_3:
            return JaamHardwareLed::SERVICE_LED_FORMAT_DISABLED;
        default:
            return settings.getInt(SERVICE_LED_COLOR_FORMAT);
    }
}

int JaamHardware::getButton1Pin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
            return JaamHardwarePins::BUTTON_1_PIN_JAAM_3;
        case HARDWARE::JAAM_2_1:
            return JaamHardwarePins::BUTTON_1_PIN_JAAM_2; 
        case HARDWARE::JAAM_1_3:
            return JaamHardwarePins::BUTTON_1_PIN_JAAM_1;
        default:
            return settings.getInt(BUTTON_1_PIN);
    }
}

int JaamHardware::getButton2Pin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return JaamHardwarePins::BUTTON_2_PIN_JAAM;
        case HARDWARE::JAAM_1_3:
            return JaamHardwarePins::BUTTON_2_PIN_DISABLED;
        default:
            return settings.getInt(BUTTON_2_PIN);
    }
}

int JaamHardware::getButton3Pin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
            return JaamHardwarePins::BUTTON_3_PIN_JAAM_3;
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
            return JaamHardwarePins::BUTTON_3_PIN_DISABLED;
        default:
            return settings.getInt(BUTTON_3_PIN);
    }
}

int JaamHardware::getBuzzerPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return JaamHardwarePins::BUZZER_PIN_JAAM;
        default:
            return settings.getInt(BUZZER_PIN);
    }
}

int JaamHardware::getDfRxPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
            return JaamHardwarePins::DF_RX_PIN_JAAM;
        default:
            return settings.getInt(DF_RX_PIN);
    }
}

int JaamHardware::getDfTxPin() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
        case HARDWARE::JAAM_3_0:
            return JaamHardwarePins::DF_TX_PIN_JAAM;
        default:
            return settings.getInt(DF_TX_PIN);
    }
}

int JaamHardware::getDisplayModel() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
            return static_cast<int>(JaamDisplayType::SH1106G);
        case HARDWARE::JAAM_1_3:
            return static_cast<int>(JaamDisplayType::SSD1306);
        default:
            return settings.getInt(DISPLAY_MODEL);
    }
}

int JaamHardware::getDisplayHeight() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
            return static_cast<int>(JaamDisplayHeight::HEIGHT_64);
        default:
            return settings.getInt(DISPLAY_HEIGHT);
    }
}

int JaamHardware::getDisplayRotation() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_0:
        case HARDWARE::JAAM_2_1:
        case HARDWARE::JAAM_1_3:
            return static_cast<int>(JaamDisplayRotation::ROTATION_0);
        default:
            return settings.getInt(DISPLAY_ROTATION);
    }
}

const RegionLedMapEntry* JaamHardware::getRegionMap() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_0:
            return REGION_MAP_JAAM_3_0;
        case HARDWARE::JAAM_3_2:
            return REGION_MAP_JAAM_3_2;
        case HARDWARE::ODESA_KYIV:
        case HARDWARE::JAAM_1_3:
        case HARDWARE::JAAM_2_1:
            return STATE_MAP_LED_ODESA_WITH_KYIV;
        case HARDWARE::ODESA:
            return STATE_MAP_LED_ODESA_WITHOUT_KYIV;
        case HARDWARE::ZAKARPATTIA_KYIV:
            return STATE_MAP_LED_TRANSCARPATHIA_WITH_KYIV;
        case HARDWARE::ZAKARPATTIA:
            return STATE_MAP_LED_TRANSCARPATHIA_WITHOUT_KYIV;
        default:
            return STATE_MAP_LED_ODESA_WITH_KYIV;
    }
}

uint8_t JaamHardware::getMaxBrightness() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
            return JaamHardwareLed::BRIGHTNESS_JAAM_3_2_MAX;
        case HARDWARE::JAAM_3_0:
            return JaamHardwareLed::BRIGHTNESS_JAAM_3_0_MAX;
        case HARDWARE::JAAM_2_1:
            return JaamHardwareLed::BRIGHTNESS_JAAM_2_1_MAX;
        case HARDWARE::JAAM_1_3:
            return JaamHardwareLed::BRIGHTNESS_JAAM_1_3_MAX;
        default:
            return JaamHardwareLed::BRIGHTNESS_DEFAULT_MAX;
    }
}

uint8_t JaamHardware::getMinBrightness() {
    uint8_t hwType = getCurrentHardwareType();
    switch (hwType) {
        case HARDWARE::JAAM_3_2:
            return JaamHardwareLed::BRIGHTNESS_JAAM_3_2_MIN;
        case HARDWARE::JAAM_3_0:
            return JaamHardwareLed::BRIGHTNESS_JAAM_3_0_MIN;
        case HARDWARE::JAAM_2_1:
            return JaamHardwareLed::BRIGHTNESS_JAAM_2_1_MIN;
        case HARDWARE::JAAM_1_3:
            return JaamHardwareLed::BRIGHTNESS_JAAM_1_3_MIN;
        default: {
            // Read settings-derived min brightness (int), clamp to [0,100],
            // ensure it does not exceed hardware max, then cast.
            int val = settings.getInt(BRIGHTNESS_MIN);
            if (val < 0) val = 0;
            if (val > 100) val = 100;
            uint8_t maxB = getMaxBrightness();
            if (val > maxB) val = maxB;
            return static_cast<uint8_t>(val);
        }
    }
}