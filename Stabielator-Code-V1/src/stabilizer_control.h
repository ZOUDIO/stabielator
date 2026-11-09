/**
 * @file stabilizer_control.h
 * @brief Regeltaak, consistente statussnapshots en veilige bedieningsopdrachten.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "imu_sensor.h"
#include "power_monitor.h"
#include "stabilizer_logic.h"
#include "haptic_pattern.h"

struct StabilizerSnapshot {
  StabilizerSettings settings;
  ImuReadings imu;
  PowerMonitor power;
  StabilizerState state = StabilizerState::Waiting;
  bool referenceValid = false, imuHealthy = false, imuReady = false, motorsHealthy = false;
  bool initialized = false, manualTest = false, storageHealthy = true;
  bool testMode = false, feedbackCue = false;
  bool motorStopFailed = false;
  bool userPaused = true, pauseStorageHealthy = false;
  uint32_t inactivityMs = 0;
  bool present[4] = {}, hardwarePresent[4] = {};
  uint8_t levels[4] = {};
  uint32_t calibrationSamples = 0;
  float roll = 0, pitch = 0, yaw = 0, tiltRoll = 0, tiltPitch = 0;
};

// Klein, atomair snapshot voor de liveweergave; geen extra sensoruitlezing.
struct OrientationSnapshot {
  Quaternion absolute, relative;
  uint32_t sampleMs=0, nowMs=0, sequence=0, generation=0;
  bool valid=false, referenceValid=false, debugEnabled=false;
};

class StabilizerControl {
 public:
  void begin();
  StabilizerSnapshot snapshot();
  OrientationSnapshot orientationSnapshot();
  void logDebug();
  bool saveSettings(const StabilizerSettings& settings);
  void calibrate();
  bool pause(bool paused);
  void stopForRestart();
  bool testPulse(uint8_t motor, uint8_t strength, uint32_t durationMs, bool pattern = false);
  bool setTestMode(bool enabled);
  void reinitializeHardware();
  bool beginFirmwareUpdate();
  void endFirmwareUpdate();
 private:
  static void taskEntry(void* context);
  void tick();
  StabilizerLogic _logic;
  PowerMonitor _power;
  SemaphoreHandle_t _mutex = nullptr;
  bool _initialized = false, _storageHealthy = true, _manualActive = false;
  bool _testMode = false, _startupCuePending = true;
  bool _updating = false;
  bool _taskFailed = false;
  bool _userPaused = true, _pauseStorageHealthy = false, _storedPaused = true;
  bool _driverInitPending = false;
  HapticPattern _manual, _cue;
  uint32_t _generation = 0, _lastDebug = 0;
};
extern StabilizerControl stabilizerControl;
