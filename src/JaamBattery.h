#pragma once
#include "JaamSettings.h"
#include "JaamLogs.h"
#include <Arduino.h>

class JaamBattery {
public:
    JaamBattery();
    void setSettings(JaamSettings* settings);
    void begin();
    void end();
    void reinit();
    void setPin(int pin);
    int getPin() const;
    void setEnabled(bool enabled);
    bool isEnabled() const;
    float readVoltage() const;
    void logVoltage() const;
private:
    int adcPin;
    bool enabled;
    JaamSettings* settings;
    static constexpr float R1 = 26100.0f;
    static constexpr float R2 = 10000.0f;
    static constexpr float VIN = 3.3f;
};
