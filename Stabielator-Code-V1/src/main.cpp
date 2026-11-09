/**
 * @file main.cpp
 * @brief Start de firmware en verwerkt webbediening, WiFi, knop en USB-serial.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include <Arduino.h>
#include "config.h"
#include "device_identity.h"
#include "gpio_control.h"
#include "serial_shell.h"
#include "web_status_server.h"
#include "wifi_portal.h"
#include "stabilizer_control.h"
#include "firmware_update.h"
void setup() {
  digitalWrite(KILL_PIN, LOW);
  pinMode(KILL_PIN, OUTPUT);
  Serial.begin(SERIAL_BAUD_RATE);
  const uint32_t start = millis();
  while (!Serial && millis() - start < SERIAL_WAIT_TIMEOUT_MS) yield();
  Serial.printf("\n%s v%s\nESP32-C3 + BNO08x (schema: BNO086) + 4x DRV2605L\n", APP_NAME, APP_VERSION);
  deviceIdentity.begin();
  gpioControl.begin();
  stabilizerControl.begin();
  wifiPortal.begin();
  webStatusServer.begin();
  serialShell.begin();
}
void loop() {
  firmwareUpdate.update();
  wifiPortal.update();
  const auto status = stabilizerControl.snapshot();
  bool motorActive = false;
  for (auto level : status.levels) motorActive = motorActive || level != 0;
  gpioControl.update(WiFi.softAPgetStationNum() > 0,
                     status.imuHealthy && status.motorsHealthy, status.state,
                     status.power.low, status.settings.ledEnabled, motorActive, status.settings.ledBrightness, status.userPaused);
  if (gpioControl.consumePairingRequest() && !firmwareUpdate.busy()) {
    wifiPortal.startPairing();
  }
  webStatusServer.update(wifiPortal.isConnected(), wifiPortal.isPairingActive());
  serialShell.update();
  stabilizerControl.logDebug();
  yield();
}
