#pragma once

#if BH1750_ENABLED
#include <BH1750_WE.h>
#define BH1750_POWER_PIN 19
#endif
#include <WString.h>

class JaamLightSensor {
public:
    JaamLightSensor();
    bool begin(bool activation = false);
    void read();
    float getLightLevel();
    bool isLightSensorAvailable();
    bool isLightSensorEnabled();
    bool isAnySensorAvailable();
    String getSensorModel();

private:
#if BH1750_ENABLED
    BH1750_WE* bh1750 = nullptr;
#endif
    bool bh1750Initialized = false;
    float lightLevel = 0.0f;
};
