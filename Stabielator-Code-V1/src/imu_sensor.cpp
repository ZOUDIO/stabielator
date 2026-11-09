/**
 * @file imu_sensor.cpp
 * @brief Leest BNO08x-rotation-vectorrapporten en bewaakt resets en meetactualiteit.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "imu_sensor.h"
#include <Wire.h>
#include <cmath>
ImuSensor imuSensor;
bool ImuSensor::begin() {
  ++_generation;
  _readings.received = false;
  pinMode(IMU_INT_PIN, INPUT_PULLUP);
  _ready = _sensor.begin_I2C(IMU_ADDRESS, &Wire);
  if (_ready) _ready = _sensor.enableReport(SH2_ROTATION_VECTOR, IMU_REPORT_INTERVAL_US);
  Serial.println(_ready ? "BNO08x gereed (schema: BNO086)" : "BNO08x initialisatie mislukt");
  return _ready;
}
void ImuSensor::update() {
  if (!_ready) return;
  if (_sensor.wasReset()) {
    ++_generation;
    _readings.received = false;
    _ready = _sensor.enableReport(SH2_ROTATION_VECTOR, IMU_REPORT_INTERVAL_US);
    if (!_ready) return;
  }
  sh2_SensorValue_t event;
  // Begrens werk per loop; de library pollt I2C, INT is beschikbaar als debugpin.
  if (!_sensor.getSensorEvent(&event) || event.sensorId != SH2_ROTATION_VECTOR) return;
  _readings.real = event.un.rotationVector.real;
  _readings.i = event.un.rotationVector.i;
  _readings.j = event.un.rotationVector.j;
  _readings.k = event.un.rotationVector.k;
  _readings.accuracy = event.status;
  _readings.timestampMs = millis();
  _readings.received = true;
  ++_readings.sequence;
}
bool ImuSensor::healthy() const {
  const float n = _readings.real*_readings.real + _readings.i*_readings.i +
                  _readings.j*_readings.j + _readings.k*_readings.k;
  return _ready && _readings.received && std::isfinite(n) &&
         n >= 0.5f && n <= 1.5f && millis() - _readings.timestampMs < IMU_STALE_MS;
}
