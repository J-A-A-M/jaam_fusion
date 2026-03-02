#pragma once

#include "JaamSettings.h"
#include <Arduino.h>

class JaamSiren {
public:
    JaamSiren();
    
    void setSettings(JaamSettings* settings);
    void init();
    
    // Обробка змін налаштувань
    void onSettingsChange(Type type, int intValue, float fltValue, const char* strValue);
    
    // Основні методи управління GPIO пінами
    void setAlert();     // Активувати пін тривоги
    void clearAlert();   // Активувати пін відбою
    
    // Перевірка стану пінів
    bool isAlertActive() const;
    bool isClearActive() const;
    
    // Callback методи для таймерів (викликаються через глобальні функції)
    void onAlertTimeout();
    void onClearTimeout();
    void onAlertTimeout2();
    void onClearTimeout2();
    
private:
    JaamSettings* settings;
    int alertPin;
    int clearPin;
    int pinModeConfig;
    int activeLevel;  // HIGH або LOW
    int pinTime;      // час активації в мілісекундах
    
    int alertTimer;
    int clearTimer;
    bool alertActive;
    bool clearActive;
    bool alertInHomeRegion;
    
    // Другий комплект пінів
    int alertPin2;
    int clearPin2;
    int pinModeConfig2;
    int activeLevel2;
    int pinTime2;
    
    int alertTimer2;
    int clearTimer2;
    bool alertActive2;
    bool clearActive2;
    
    // Внутрішні методи
    void updateConfiguration();
    void setupPins();
    void deactivatePin(int pin, int activeLevel);
    void activatePin(int pin);
    void resetPins();        // Скинути всі піни

};
