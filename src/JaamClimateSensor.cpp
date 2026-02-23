#include "JaamClimateSensor.h"
#include <Arduino.h>
#include "JaamLogs.h"

JaamClimateSensor::JaamClimateSensor() {
}

bool JaamClimateSensor::begin() {
#if BME280_ENABLED || SHT2X_ENABLED || SHT3X_ENABLED || AHTXX_ENABLED
  Wire.begin();
#endif
#if BME280_ENABLED
  // Clean up previous instance to prevent memory leak
  if (bme280 != nullptr) {
    delete bme280;
    bme280 = nullptr;
  }
  bme280 = new ForcedBME280Float();
  bme280->begin();
  switch (bme280->getChipID()) {
    case CHIP_ID_BME280:
      bme280Initialized = true;
      LOG.printf("[SENSORS] Found BME280 temp/hum/presure sensor! Success.\n");
      break;
    case CHIP_ID_BMP280:
      bmp280Initialized = true;
      LOG.printf("[SENSORS] Found BMP280 temp/presure sensor! No Humidity available.\n");
      break;
    default:
      bme280Initialized = false;
      bmp280Initialized = false;
      LOG.printf("[SENSORS] Not found BME280 or BMP280!\n");
  }
#endif
#if SHT2X_ENABLED
  // Clean up previous instance to prevent memory leak
  if (sht2x != nullptr) {
    delete sht2x;
    sht2x = nullptr;
  }
  sht2x = new SHTSensor(SHTSensor::SHT2X);
  sht2xInitialized = sht2x->init();
  if (sht2xInitialized) {
    LOG.printf("[SENSORS] Found HTU2x temp/hum sensor! Success.\n");
  } else {
    LOG.printf("[SENSORS] Not found HTU2x temp/hum sensor!\n");
  }
#endif
#if SHT3X_ENABLED
  // Clean up previous instance to prevent memory leak
  if (sht3x != nullptr) {
    delete sht3x;
    sht3x = nullptr;
  }
  sht3x = new SHTSensor(SHTSensor::SHT3X);
  sht3xInitialized = sht3x->init();
  if (sht3xInitialized) {
    LOG.printf("[SENSORS] Found SHT3x temp/hum sensor! Success.\n");
  } else {
    LOG.printf("[SENSORS] Not found SHT3x temp/hum sensor!\n");
  }
#endif
#if AHTXX_ENABLED
  // Clean up previous instance to prevent memory leak
  if (ahtxx != nullptr) {
    delete ahtxx;
    ahtxx = nullptr;
  }
  ahtxx = new AHTxx(AHTXX_ADDRESS_X38, AHT2x_SENSOR);
  ahtxxInitialized = ahtxx->begin();
  if (ahtxxInitialized) {
    LOG.printf("[SENSORS] Found AHT2x/AHT3x temp/hum sensor! Success.\n");
  } else {
    LOG.printf("[SENSORS] Not found AHT2x/AHT3x temp/hum sensor!\n");
  }
#endif
return bme280Initialized || bmp280Initialized || sht2xInitialized || sht3xInitialized || ahtxxInitialized;
}

void JaamClimateSensor::read() {
#if BME280_ENABLED
  if (bme280Initialized || bmp280Initialized) {
    bme280->takeForcedMeasurement();

    localTemp = bme280->getTemperatureCelsiusAsFloat();
    localPressure = bme280->getPressureAsFloat() * 0.75006157584566;  //mmHg

    if (bme280Initialized) {
      localHum = bme280->getRelativeHumidityAsFloat();
    }

    LOG.printf("[CLIMATE] BME280! Temp: %.2f°C  Humidity: %.2f%%  Pressure: %.2fmmHg\n", localTemp, localHum, localPressure);
    return;
  }
#endif
#if SHT3X_ENABLED
  if (sht3xInitialized && sht3x->readSample()) {
    localTemp = sht3x->getTemperature();
    localHum = sht3x->getHumidity();

    LOG.printf("[CLIMATE] SHT3X! Temp: %.2f°C  Humidity: %.2f%%\n", localTemp, localHum);
    return;
  }
#endif
#if SHT2X_ENABLED
  if (sht2xInitialized && sht2x->readSample()) {
    localTemp = sht2x->getTemperature();
    localHum = sht2x->getHumidity();

    LOG.printf("[CLIMATE] SHT2X! Temp: %.2f°C. Humidity: %.2f%%\n", localTemp, localHum);
    return;
  }
#endif
#if AHTXX_ENABLED
  if (ahtxxInitialized) {
    float tempReading = ahtxx->readTemperature();
    float humReading = ahtxx->readHumidity();

    if (tempReading == AHTXX_ERROR || humReading == AHTXX_ERROR) {
      LOG.printf("[CLIMATE] AHT2x/AHT3x read error! Temp: %.2f, Humidity: %.2f\n", tempReading, humReading);
      return;
    }

    localTemp = tempReading;
    localHum = humReading;
    LOG.printf("[CLIMATE] AHT2x/AHT3x! Temp: %.2f°C  Humidity: %.2f%%\n", localTemp, localHum);
    return;
  }
#endif
}

bool JaamClimateSensor::isTemperatureAvailable() {
  return (bme280Initialized || bmp280Initialized || sht2xInitialized || sht3xInitialized || ahtxxInitialized) && localTemp > -273;
}

bool JaamClimateSensor::isHumidityAvailable() {
  return (bme280Initialized || sht2xInitialized || sht3xInitialized || ahtxxInitialized) && localHum >= 0;
}

bool JaamClimateSensor::isPressureAvailable() {
  return (bme280Initialized || bmp280Initialized) && localPressure >= 0;
}

float JaamClimateSensor::getTemperature(float tempCorrection) {
  return localTemp + tempCorrection;
}

float JaamClimateSensor::getHumidity(float humCorrection) {
  return localHum + humCorrection;
}

float JaamClimateSensor::getPressure(float pressCorrection) {
  return localPressure + pressCorrection;
}

String JaamClimateSensor::getSensorModel() {
  if (bme280Initialized) {
    return "BME280";
  }
  if (bmp280Initialized) {
    return "BMP280";
  }
  if (sht3xInitialized) {
    return "SHT3X";
  }
  if (sht2xInitialized) {
    return "SHT2X";
  }
  if (ahtxxInitialized) {
    return "AHTxx";
  }
  return "Немає";
}

bool JaamClimateSensor::isAnySensorAvailable() {
  return isBME280Available() || isBMP280Available() || isSHT2XAvailable() || isSHT3XAvailable() || isAHTxxAvailable();
}

bool JaamClimateSensor::isAnySensorEnabled() {
  return BME280_ENABLED || SHT2X_ENABLED || SHT3X_ENABLED || AHTXX_ENABLED;
}

bool JaamClimateSensor::isBME280Available() {

  return bme280Initialized;
}

bool JaamClimateSensor::isBMP280Available() {
  return bmp280Initialized;
}

bool JaamClimateSensor::isSHT2XAvailable() {
  return sht2xInitialized;
}

bool JaamClimateSensor::isSHT3XAvailable() {
  return sht3xInitialized;
}

bool JaamClimateSensor::isAHTxxAvailable() {
  return ahtxxInitialized;
}
