#include "JaamBattery.h"
#include "JaamConfig.h"

JaamBattery::JaamBattery() : adcPin(-1), enabled(false), settings(nullptr) {}

void JaamBattery::setSettings(JaamSettings* s) {
    this->settings = settings;
}

void JaamBattery::begin() {
    adcPin = settings->getInt(BATTERY_PIN);
    enabled = settings->getBool(ENABLE_BATTERY_MONITORING);
    if (settings->getInt(BATTERY_PIN) >= 0) {
        
        pinMode(adcPin, INPUT);
    }
}

void JaamBattery::end() {
    // No hardware deinit needed for ADC
}

void JaamBattery::reinit() {
    end();
    begin();
}

void JaamBattery::setPin(int pin) {
    adcPin = pin;
    if (settings) settings->saveInt(BATTERY_PIN, pin);
    reinit();
}

int JaamBattery::getPin() const {
    return adcPin;
}

void JaamBattery::setEnabled(bool en) {
    enabled = en;
    if (settings) settings->saveBool(ENABLE_BATTERY_MONITORING, en);
}

bool JaamBattery::isEnabled() const {
    return enabled;
}

float JaamBattery::readVoltage() const {
    if (adcPin < 0 || !enabled) return 0.0f;
    int raw = analogRead(adcPin);
    float vout = (raw / 4095.0f) * VIN;
    float vin = vout * ((R1 + R2) / R2);
    return vin;
}

void JaamBattery::logVoltage() const {
    if (!enabled) return;
    float voltage = readVoltage();
    LOG.printf("[BATTERY] Voltage: %.2f V (pin %d)\n", voltage, adcPin);
}
