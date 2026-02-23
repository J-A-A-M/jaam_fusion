#pragma once

#if BH1750_ENABLED
#include <BH1750_WE.h>
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
};
