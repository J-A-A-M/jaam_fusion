#include "JaamLightSensor.h"
#include <Arduino.h>
#include "JaamLogs.h"

JaamLightSensor::JaamLightSensor() {
}

bool JaamLightSensor::begin(bool activation) {
#if BH1750_ENABLED
  Wire.begin();

  // Additional activation sequence for BH1750
  if (activation) {
    LOG.println("[SENSORS] Additional activation sequence for BH1750.");
    pinMode(19, OUTPUT);
    digitalWrite(19, HIGH); 
    digitalWrite(19, LOW);
    delay(50); 
    digitalWrite(19, HIGH);
  }

  // Clean up previous instance to prevent memory leak
  if (bh1750 != nullptr) {
    delete bh1750;
    bh1750 = nullptr;
  }

  bh1750 = new BH1750_WE();
  bh1750Initialized = bh1750->init();
  if (bh1750Initialized) {
    bh1750->setMode(CHM_2);
    LOG.println("[SENSORS] Found BH1750 light sensor! Success.");
  } else {
    LOG.println("[SENSORS] Not found BH1750 light sensor!");
  }
#else
  bh1750Initialized = false;
#endif
  return bh1750Initialized;
}

void JaamLightSensor::read() {
#if BH1750_ENABLED
    if (!bh1750Initialized) return;
    lightLevel = bh1750->getLux();
#endif
}

float JaamLightSensor::getLightLevel() {
#if BH1750_ENABLED
    if (bh1750Initialized) {
        return lightLevel;
    }
#endif
  return -1;
}

bool JaamLightSensor::isLightSensorAvailable() {
#if BH1750_ENABLED
  return bh1750Initialized;
#else
    return false;
#endif
}

bool JaamLightSensor::isLightSensorEnabled() {
#if BH1750_ENABLED
  return true;
#else
  return false;
#endif
}

bool JaamLightSensor::isAnySensorAvailable() {
  return isLightSensorAvailable();
}

String JaamLightSensor::getSensorModel() {
  if (bh1750Initialized) {
    return "BH1750";
  }
  return "Немає";
}
