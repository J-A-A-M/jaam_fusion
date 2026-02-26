#pragma once

#include "JaamSettings.h"
#include <Arduino.h>

class JaamSiren {
public:
    JaamSiren();
    
    void setSettings(JaamSettings* settings);
    void init();
    
    // Обробка змін налаштувань
    void onSettingsChange(Type type, int intValue, const char* strValue);
    
    // Основні методи управління GPIO пінами
    void setAlert();     // Активувати пін тривоги
    void clearAlert();   // Активувати пін відбою
    
    // Перевірка стану пінів
    bool isAlertActive() const;
    bool isClearActive() const;
    
    // Метод для періодичного виклику з loop для обробки таймерів
    void tick();
    
private:
    JaamSettings* settings;
    int alertPin;
    int clearPin;
    int pinModeConfig;
    int activeLevel;  // HIGH або LOW
    int pinTime;      // час активації в мілісекундах
    
    unsigned long alertActivationTime;
    unsigned long clearActivationTime;
    bool alertActive;
    bool clearActive;
    bool alertInHomeRegion;
    
    // Внутрішні методи
    void updateConfiguration();
    void setupPins();
    void deactivatePin(int pin);
    void activatePin(int pin);
    void resetPins();        // Скинути всі піни

};
