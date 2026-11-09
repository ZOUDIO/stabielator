/**
 * @file device_identity.h
 * @brief Interface voor de unieke apparaatcode en de WiFi-netwerkidentiteit.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once

#include <Arduino.h>

#include "config.h"

/**
 * @class DeviceIdentity
 * @brief Maakt stabiele identificatie op basis van de ESP32 eFuse MAC.
 */
class DeviceIdentity {
 public:
  /**
   * @brief Bouwt de MAC-gebaseerde serial wanneer dat nog niet is gebeurd.
   */
  void begin();

  /**
   * @brief Geeft de unieke apparaatcode terug.
   * @return MAC-adres als 12 hextekens zonder scheidingstekens.
   */
  const char* serialNumber() const;

 private:
  char _serialNumber[DEVICE_SERIAL_BUFFER_SIZE] = "";
};

/**
 * @brief Globale device-identiteit.
 */
extern DeviceIdentity deviceIdentity;
