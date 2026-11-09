/**
 * @file haptic_control.h
 * @brief Hardware-interface voor ERM/LRA-aansturing, kalibratie en herhaalde motorstop.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include "config.h"
class HapticControl {
 public:
  void begin(bool allowActuation = true);
  bool calibrationPending() const;
  bool prepare(uint8_t motor);
  void update();
  bool setLevel(uint8_t motor, uint8_t strength);
  uint8_t level(uint8_t motor) const { return motor < HAPTIC_MOTOR_COUNT && _running[motor] ? _strength[motor] : 0; }
  bool stopAll();
  bool stopFailed() const { return _stopFailed; }
  bool present(uint8_t motor) const;
  bool hardwarePresent(uint8_t motor) const;
  bool healthy() const;
  bool enabled() const;
 private:
  bool start(uint8_t motor, uint8_t strength);
  bool select(uint8_t motor);
  bool disconnect();
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readRegister(uint8_t reg, uint8_t& value);
  bool calibrateLra();
  bool checkMotor(uint8_t motor);
  bool stop(uint8_t motor);
  bool _present[HAPTIC_MOTOR_COUNT] = {};
  bool _running[HAPTIC_MOTOR_COUNT] = {};
  bool _calibrated[HAPTIC_MOTOR_COUNT] = {};
  bool _stopRequired[HAPTIC_MOTOR_COUNT] = {};
  uint32_t _lastStopAttempt[HAPTIC_MOTOR_COUNT] = {};
  uint8_t _strength[HAPTIC_MOTOR_COUNT] = {};
  bool _muxReady = false;
  bool _stopFailed = false;
  uint8_t _nextHealthMotor = 0;
  uint32_t _lastHealthCheck = 0;
};
extern HapticControl hapticControl;
