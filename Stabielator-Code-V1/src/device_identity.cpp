/**
 * @file device_identity.cpp
 * @brief Leidt de vaste apparaatcode af uit het ESP32 eFuse-MAC-adres.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "device_identity.h"

DeviceIdentity deviceIdentity;

void DeviceIdentity::begin() {
  if (_serialNumber[0] != '\0') {
    return;
  }

  const uint64_t mac = ESP.getEfuseMac();
  snprintf(_serialNumber, sizeof(_serialNumber), "%04lX%08lX",
           static_cast<unsigned long>((mac >> 32) & 0xFFFF),
           static_cast<unsigned long>(mac & 0xFFFFFFFF));
}

const char* DeviceIdentity::serialNumber() const {
  return _serialNumber;
}
