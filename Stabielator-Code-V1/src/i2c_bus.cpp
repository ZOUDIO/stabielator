/**
 * @file i2c_bus.cpp
 * @brief Initialiseert en herstart de I2C-controller met begrensde transactietimeout.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "i2c_bus.h"
#include <Wire.h>
#include "config.h"
namespace {
bool controllerReady = false;
}
void printI2cBusStatus() {
  Serial.printf("[I2C BUS] controller=%s SDA=GPIO%u:%s SCL=GPIO%u:%s klok=%lu Hz\n",
                controllerReady ? "gestart" : "niet gestart",
                I2C_SDA_PIN, digitalRead(I2C_SDA_PIN) ? "HIGH" : "LOW",
                I2C_SCL_PIN, digitalRead(I2C_SCL_PIN) ? "HIGH" : "LOW",
                static_cast<unsigned long>(I2C_CLOCK_HZ));
  Serial.println("[I2C BUS] Rustniveau hoort HIGH/HIGH te zijn. Check 3V3-pull-ups en GND bij LOW.");
}
bool beginI2cBus() {
  controllerReady = Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ);
  Wire.setTimeOut(25);
  printI2cBusStatus();
  return controllerReady;
}
bool restartI2cBus() {
  if (!Wire.end()) {
    controllerReady = false;
    Serial.println("[I2C BUS] Wire.end mislukt; controller niet opnieuw gestart.");
    return false;
  }
  controllerReady = false;
  delay(2);
  return beginI2cBus();
}
