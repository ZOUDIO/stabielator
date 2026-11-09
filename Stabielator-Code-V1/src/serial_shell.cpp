/**
 * @file serial_shell.cpp
 * @brief Verwerkt begrensde seriele opdrachten voor bediening en hardwarediagnose.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "serial_shell.h"
#include <cstring>
#include <cstdio>
#include "i2c_bus.h"
#include "web_status_server.h"
#include "wifi_portal.h"
#include "stabilizer_control.h"
#include "firmware_update.h"
SerialShell serialShell;
void SerialShell::begin() { Serial.println("Typ help voor commando's."); }
void SerialShell::update() {
  // Ook bij veel invoer blijft de hoofdloop beschikbaar voor de stoptimer.
  for (unsigned budget = 0; budget < 64 && Serial.available(); ++budget) {
    const char c = Serial.read();
    // CR, LF en CRLF zijn geldig; de tweede terminator van CRLF is een lege regel.
    if (c == '\r' || c == '\n') {
      if (_overflow) Serial.println("Commando te lang; genegeerd.");
      else if (_length) { _buffer[_length] = 0; handleCommand(_buffer); }
      _length = 0; _overflow = false;
    } else if (!_overflow) {
      if (_length + 1 < sizeof(_buffer)) _buffer[_length++] = c;
      else _overflow = true;
    }
  }
}
void SerialShell::handleCommand(const char* command) {
  if (firmwareUpdate.busy() && strcmp(command,"status") && strcmp(command,"wifi_status")) {
    Serial.println("Firmware-update actief; opdracht geblokkeerd."); return;
  }
  if (!strcmp(command, "help")) {
    Serial.println("help | status | calibrate | resume | wifi_status | ap on | ap off | test on | test off | i2c_status | i2c_init | portal | haptic <1-4> <1-127> <1-1000 ms> | stop | reboot | factory_reset");
  } else if (!strcmp(command, "status")) {
    Serial.println(webStatusServer.statusJson());
    Serial.print("WiFi: "); Serial.println(wifiPortal.statusText());
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    Serial.print("Portal AP: "); Serial.println(wifiPortal.accessPointName());
    wifiPortal.printDiagnostics();
  } else if (!strcmp(command, "wifi_status")) {
    wifiPortal.printDiagnostics();
  } else if (!strcmp(command,"ap on") || !strcmp(command,"ap off")) {
    Serial.println(wifiPortal.setAccessPointEnabled(!strcmp(command,"ap on"))?
      "AP-keuze opgeslagen; wordt toegepast.":"AP-keuze opslaan mislukt; niet gewijzigd.");
  } else if (!strcmp(command,"test on") || !strcmp(command,"test off")) {
    Serial.println(stabilizerControl.setTestMode(!strcmp(command,"test on"))?
      "Ontwikkeltest opgeslagen.":"Ontwikkeltest opslaan mislukt; niet gewijzigd.");
  } else if (!strcmp(command, "i2c_status")) {
    printI2cBusStatus();
  } else if (!strcmp(command, "i2c_init")) {
    Serial.println("I2C-controller stoppen en opnieuw initialiseren...");
    stabilizerControl.reinitializeHardware();
    printI2cBusStatus();
    Serial.println("I2C-initialisatiepogingen afgerond; zie afzonderlijke resultaten.");
  } else if (!strcmp(command, "stop")) {
    Serial.println(stabilizerControl.pause(true)?"Pauze opgeslagen; motoren uit, ook na herstart.":"Gepauzeerd, maar opslaan mislukt; pauze na herstart niet gegarandeerd.");
  } else if (!strcmp(command, "portal")) {
    wifiPortal.startPairing();
  } else if (!strcmp(command, "calibrate")) {
    stabilizerControl.calibrate();
    Serial.println("Referentiekalibratie aangevraagd; houd het apparaat stil.");
  } else if (!strcmp(command, "resume")) {
    Serial.println(stabilizerControl.pause(false)?"Hervatten opgeslagen; regeling vrijgegeven.":"Hervatten niet opgeslagen; blijft gepauzeerd.");
  } else if (!strcmp(command, "reboot")) {
    stabilizerControl.stopForRestart(); ESP.restart();
  } else if (!strcmp(command, "factory_reset")) {
    stabilizerControl.stopForRestart(); wifiPortal.factoryResetAndReboot();
  } else {
    unsigned motor, strength, duration; char extra;
    if (sscanf(command, "haptic %u %u %u %c", &motor, &strength, &duration, &extra) == 3 &&
        motor >= 1 && motor <= HAPTIC_MOTOR_COUNT && strength >= 1 && strength <= 127 &&
        duration >= 1 && duration <= HAPTIC_MAX_PULSE_MS) {
      Serial.println(stabilizerControl.testPulse(motor - 1, strength, duration) ?
                     "Puls gestart." : "Geen puls: controleer IMU, voeding, motor of I2C.");
    } else Serial.println("Onbekend/ongeldig commando. Typ help.");
  }
}
