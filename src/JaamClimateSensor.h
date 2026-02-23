#pragma once

#if BME280_ENABLED
#include <forcedBMX280.h>
#endif
#if SHT2X_ENABLED || SHT3X_ENABLED
#include <SHTSensor.h>
#endif
#if AHTXX_ENABLED
#include <AHTxx.h>
#endif
#include <WString.h>

class JaamClimateSensor {
    public:
        JaamClimateSensor();
        ~JaamClimateSensor();
        bool begin();
        void read();
        bool isTemperatureAvailable();
        bool isHumidityAvailable();
        bool isPressureAvailable();
        bool isAnySensorAvailable();
        bool isAnySensorEnabled();
        bool isBME280Available();
        bool isBMP280Available();
        bool isSHT2XAvailable();
        bool isSHT3XAvailable();
        bool isAHTxxAvailable();
        float getTemperature(float tempCorrection = 0.0);
        float getHumidity(float humCorrection = 0.0);
        float getPressure(float pressCorrection = 0.0);
        String getSensorModel();

    private:
#if BME280_ENABLED
        ForcedBME280Float* bme280 = nullptr;
#endif
#if SHT2X_ENABLED
        SHTSensor* sht2x = nullptr;
#endif
#if SHT3X_ENABLED
        SHTSensor* sht3x = nullptr;
#endif
#if AHTXX_ENABLED
        AHTxx* ahtxx = nullptr;
#endif
        bool bme280Initialized = false;
        bool bmp280Initialized = false;
        bool sht2xInitialized = false;
        bool sht3xInitialized = false;
        bool ahtxxInitialized = false;
        float localTemp = -273.0f;
        float localHum = -1.0f;
        float localPressure = -1.0f;
};
