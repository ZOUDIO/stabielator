/**
 * @file imu_sensor.h
 * @brief IMU-meetwaarden, samplevolgorde en hardwarestatus voor de regeltaak.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <Adafruit_BNO08x.h>
#include "config.h"

struct ImuReadings {
  float real = 1, i = 0, j = 0, k = 0;
  uint8_t accuracy = 0;
  uint32_t timestampMs = 0;
  bool received = false;
  uint32_t sequence = 0;
};
class ImuSensor {
 public:
  bool begin();
  void update();
  bool healthy() const;
  bool hardwareReady() const { return _ready; }
  uint32_t generation() const { return _generation; }
  const ImuReadings& readings() const { return _readings; }
 private:
  Adafruit_BNO08x _sensor{IMU_RESET_PIN};
  ImuReadings _readings;
  bool _ready = false;
  uint32_t _generation = 0;
};
extern ImuSensor imuSensor;
