#include "JaamLightSensor.h"
#include <Arduino.h>
#include "JaamLogs.h"

#if BH1750_ENABLED
BH1750_WE *bh1750;
#endif
bool bh1750Initialized = false;
float lightLevel = -1;

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

  bh1750 = new BH1750_WE();
  bh1750Initialized = bh1750->init();
  if (bh1750Initialized) {
    bh1750->setMode(CHM_2);
    LOG.println("[SENSORS] Found BH1750 light sensor! Success.");
  } else {
    LOG.println("[SENSORS] Not found BH1750 light sensor!");
  }
#endif
return bh1750Initialized;
}

void JaamLightSensor::read() {
#if BH1750_ENABLED
    if (!bh1750Initialized) return;
    lightLevel = bh1750->getLux();
    // LOG.print("BH1750!\tLight: ");
    // LOG.print(lightLevel);
    // LOG.println(" lx");
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
