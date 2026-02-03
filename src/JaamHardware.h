#pragma once

#include "JaamConfig.h"

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
    
private:
    static uint8_t getCurrentHardwareType();
};
