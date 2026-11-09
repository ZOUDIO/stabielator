/**
 * @file stabilizer_control.cpp
 * @brief Verbindt regellogica, motoren, IMU, voeding en opslag onder een taakmutex.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "stabilizer_control.h"
#include <Preferences.h>
#include <freertos/task.h>
#include "haptic_control.h"
#include "i2c_bus.h"

StabilizerControl stabilizerControl;
namespace {
class Lock {
 public:
  explicit Lock(SemaphoreHandle_t mutex) : _mutex(mutex) { xSemaphoreTakeRecursive(_mutex, portMAX_DELAY); }
  ~Lock() { xSemaphoreGiveRecursive(_mutex); }
 private:
  SemaphoreHandle_t _mutex;
};
}

void StabilizerControl::begin() {
  _mutex = xSemaphoreCreateRecursiveMutex();
  if (!_mutex) return;
  Preferences prefs;
  if (prefs.begin(APP_PREFERENCES_NAMESPACE, false)) {
    const size_t length=prefs.isKey("fw_config")?prefs.getBytesLength("fw_config"):0;
    uint8_t bytes[sizeof(StabilizerSettings)] = {};
    StabilizerSettings saved;
    bool reset=false;
    if (length && length<=sizeof(bytes) && prefs.getBytes("fw_config",bytes,length)==length &&
        decodeSettingsRecord(bytes,length,saved,reset)) {
      _logic.settings=saved;
      if (reset) {
        uint32_t oldVersion; memcpy(&oldVersion,bytes,sizeof(oldVersion));
        Serial.println(oldVersion>=3 ? "FW-instellingen gemigreerd naar v6." :
                       "FW-instellingen v1/v2 vervangen door afgesproken defaults v6; WiFi behouden.");
        _storageHealthy=prefs.putBytes("fw_config",&saved,sizeof(saved))==sizeof(saved);
      }
    } else if (length) {
      _storageHealthy=false;
      Serial.println("FW-instellingen ongeldig/onbekend; defaults actief, record niet overschreven.");
    }
    // Apart van het instellingenrecord: oude formulieren mogen pauze niet opheffen.
    if (!prefs.isKey("paused")) {
      _userPaused=true;
      _pauseStorageHealthy=prefs.putBool("paused",true)==1;
    } else if (prefs.getType("paused")==PT_U8) {
      const uint8_t value=prefs.getUChar("paused",255);
      _pauseStorageHealthy=value<=1;
      _userPaused=value!=0;
    }
    _storedPaused=_userPaused;
    if (!_pauseStorageHealthy) Serial.println("Pauzestatus onleesbaar; veilig gepauzeerd starten.");
    prefs.end();
  } else _storageHealthy = false;
  _power.begin();
  _logic.begin(millis());
  reinitializeHardware();
  TaskHandle_t task = nullptr;
  if (xTaskCreate(taskEntry, "stabilizer", 8192, this, 2, &task) != pdPASS) {
    _taskFailed = true; _initialized = false; hapticControl.stopAll();
    _userPaused = true; _logic.pause(true, millis());
    _pauseStorageHealthy = false; // Noodpauze is niet als gebruikerskeuze opgeslagen.
    Serial.println("FOUT: regeltaak kon niet starten.");
  }
}
void StabilizerControl::taskEntry(void* context) {
  for (;;) {
    static_cast<StabilizerControl*>(context)->tick();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
void StabilizerControl::reinitializeHardware() {
  if (!_mutex || _taskFailed) return;
  Lock lock(_mutex);
  if (_updating) return;
  _manualActive = false;
  _testMode=_logic.settings.testMode;
  _manual.stop(); _cue.stop(); _startupCuePending=!_userPaused&&!_testMode;
  _driverInitPending=false;
  _logic.pause(_userPaused||_testMode,millis());
  if (_initialized) hapticControl.stopAll();
  _logic.calibrate(millis(), false);
  if (_initialized && !restartI2cBus()) {
    _initialized = false; _logic.state = StabilizerState::HardwareFault; return;
  }
  if (!_initialized && !beginI2cBus()) { _logic.state = StabilizerState::HardwareFault; return; }
  hapticControl.begin(!_userPaused&&!_testMode);
  imuSensor.begin();
  _initialized = true;
  _generation = imuSensor.generation();
  _logic.calibrate(millis(), false);
  _logic.pause(_userPaused||_testMode,millis());
}
/**
 * @brief Verwerkt een regelcyclus; uitsluitend deze taak leest de IMU en stuurt motoren.
 * @details De mutex beschermt ook hardwareherinitialisatie en statussnapshots.
 * Kritieke accuspanning stopt eerst de motoren en activeert daarna KILL, ook tijdens OTA.
 * Ontbrekende/oude IMU-data of een driverfout blokkeert alle automatische en handmatige feedback.
 */
void StabilizerControl::tick() {
  Lock lock(_mutex);
  const uint32_t now = millis();
  if (_power.shuttingDown) return;
  _power.update(now, _logic.settings.batteryLowVolts, _logic.settings.batteryCriticalVolts);
  if (_power.critical) { hapticControl.stopAll(); _power.shutdown(); return; }
  if (_updating) return; // Accubeveiliging blijft ook tijdens flashschrijven actief.
  if (_driverInitPending && !_userPaused) {
    _driverInitPending=false;
    hapticControl.begin(true); // Alleen na expliciet, opgeslagen Hervatten.
  }
  if (_initialized) {
    hapticControl.update();
    imuSensor.update();
  }
  if (_generation != imuSensor.generation()) {
    _generation = imuSensor.generation();
    _logic.calibrate(now);
  }
  const auto& r = imuSensor.readings();
  const bool imuHealthy = _initialized && imuSensor.healthy();
  const bool motorsHealthy = _initialized && hapticControl.healthy() && hapticControl.enabled();
  if (_startupCuePending && !_userPaused && !_testMode && imuHealthy && motorsHealthy) {
    _startupCuePending=false;
    _cue.start(now,80,_logic.settings.strength,HAPTIC_CONTINUOUS,80,0,15);
  }
  const bool referenceWasValid=_logic.referenceValid;
  if ((!_cue.active(now) && !(_manualActive && !_logic.referenceValid)) || !imuHealthy || !motorsHealthy)
    _logic.update(now, {r.real,r.i,r.j,r.k}, imuHealthy, motorsHealthy, r.sequence);
  else _logic.stopOutputs(); // Geen kalibratiesamples verzamelen tijdens bevestigingstrilling.
  if (!referenceWasValid && _logic.referenceValid && !_testMode && !_userPaused) {
    _cue.start(now,300,_logic.settings.strength,HAPTIC_PULSES,100,100,15);
  }
  if (_power.critical || _logic.state == StabilizerState::Inactive) {
    _manualActive = false;
    if (_initialized) hapticControl.stopAll();
    _power.shutdown(); return;
  }
  if (_manualActive) {
    if (!_manual.active(now) || !imuHealthy || !motorsHealthy) {
      _manual.stop(); _manualActive=false;
    }
  }
  if (!imuHealthy || !motorsHealthy) _cue.stop();
  bool ok = true;
  for (uint8_t motor = 0; motor < 4; ++motor) {
    const uint8_t level=_manualActive ? _manual.level(now,motor) :
      _cue.active(now) ? _cue.level(now,motor) : _testMode ? 0 : _logic.outputs[motor];
    if (!hapticControl.setLevel(motor, level)) ok = false;
  }
  if (!ok) {
    hapticControl.stopAll();
    _logic.update(now, {r.real,r.i,r.j,r.k}, imuHealthy, false, r.sequence);
  }
}
OrientationSnapshot StabilizerControl::orientationSnapshot() {
  OrientationSnapshot s;
  if (!_mutex) return s;
  Lock lock(_mutex);
  const auto& r=imuSensor.readings();
  s.nowMs=millis(); s.sampleMs=r.timestampMs; s.sequence=r.sequence; s.generation=imuSensor.generation();
  s.debugEnabled=_logic.settings.debugEnabled;
  s.absolute={r.real,r.i,r.j,r.k};
  s.valid=s.debugEnabled && !_updating && !_power.shuttingDown && _initialized &&
          imuSensor.healthy() && s.absolute.normalize();
  s.referenceValid=s.valid && _logic.referenceValid;
  if (s.referenceValid) s.relative=_logic.relativeOrientation(s.absolute);
  return s;
}
void StabilizerControl::logDebug() {
  const uint32_t now=millis();
  if (now-_lastDebug<1000) return;
  _lastDebug=now;
  const auto s=snapshot();
  if (!s.settings.debugEnabled) return;
  char line[128];
  const int length=snprintf(line,sizeof(line),"[Stabielator] %s roll=%.1f pitch=%.1f accu=%.2f V\n",
                            stateName(s.state),s.tiltRoll,s.tiltPitch,s.power.volts);
  // In de hoofdloop, buiten de regelmutex. Bij volle USB-buffer overslaan.
  if (length>0 && size_t(length)<sizeof(line) && Serial.availableForWrite()>=length)
    Serial.write(reinterpret_cast<const uint8_t*>(line),size_t(length));
}
StabilizerSnapshot StabilizerControl::snapshot() {
  StabilizerSnapshot s;
  if (!_mutex) { s.storageHealthy = false; return s; }
  Lock lock(_mutex);
  s.settings = _logic.settings; s.imu = imuSensor.readings(); s.power = _power;
  s.state = _logic.state; s.referenceValid = _logic.referenceValid;
  s.userPaused=_userPaused; s.pauseStorageHealthy=_pauseStorageHealthy;
  s.imuHealthy = _initialized && imuSensor.healthy(); s.imuReady = _initialized && imuSensor.hardwareReady();
  s.initialized = _initialized; s.manualTest = _manualActive; s.storageHealthy = _storageHealthy;
  s.testMode=_testMode; s.feedbackCue=_cue.active(millis()); s.inactivityMs=_logic.inactivityMs(millis());
  s.motorStopFailed=hapticControl.stopFailed();
  s.motorsHealthy = _initialized && hapticControl.healthy() && hapticControl.enabled();
  s.roll = _logic.roll; s.pitch = _logic.pitch; s.yaw = _logic.yaw;
  s.tiltRoll = _logic.tiltRoll; s.tiltPitch = _logic.tiltPitch;
  s.calibrationSamples = _logic.calibrationSamples;
  for (uint8_t m = 0; m < 4; ++m) {
    s.present[m] = hapticControl.present(m); s.hardwarePresent[m] = hapticControl.hardwarePresent(m);
    s.levels[m] = hapticControl.level(m);
  }
  return s;
}
bool StabilizerControl::saveSettings(const StabilizerSettings& settings) {
  if (!validSettings(settings) || !_mutex) return false;
  { Lock lock(_mutex); if (_updating) return false; }
  // Flashopslag buiten de regelmutex: de regeltaak blijft beschikbaar.
  Preferences prefs;
  bool ok = prefs.begin(APP_PREFERENCES_NAMESPACE, false);
  if (ok) { ok = prefs.putBytes("fw_config", &settings, sizeof(settings)) == sizeof(settings); prefs.end(); }
  Lock lock(_mutex);
  _storageHealthy = ok;
  if (ok) {
    const bool changed=_testMode!=bool(settings.testMode);
    _manual.stop(); _manualActive=false; _logic.applySettings(settings);
    _testMode=settings.testMode;
    if(changed) {
      _cue.stop(); _startupCuePending=false;
      _logic.pause(_userPaused||_testMode,millis());
      if(_initialized) hapticControl.stopAll();
      _driverInitPending=!_userPaused&&!_testMode && hapticControl.calibrationPending();
    }
  }
  return ok;
}
void StabilizerControl::calibrate() {
  if (!_mutex) return;
  Lock lock(_mutex);
  if (_updating) return;
  _manualActive = false; if (_initialized) hapticControl.stopAll();
  _manual.stop(); _cue.stop();
  _logic.calibrate(millis()); _logic.pause(_userPaused||_testMode,millis());
}
/**
 * @brief Stopt direct en bewaart de pauzekeuze voordat hervatten mogelijk wordt.
 * @param paused True bewaart pauze; false vraagt hervatten van de regeling aan.
 * @return False bij opslagfouten of wanneer hervatten niet veilig kan worden verwerkt.
 * @details Flashopslag gebeurt buiten de regelmutex. Bij opslagfalen blijven motoren gepauzeerd.
 */
bool StabilizerControl::pause(bool paused) {
  if (!_mutex || (!paused && _taskFailed)) return false;
  auto apply=[this,paused]() {
    _storedPaused=paused; _userPaused=paused;
    _logic.pause(paused||_testMode,millis());
    _driverInitPending=!paused && !_testMode && hapticControl.calibrationPending();
  };
  {
    Lock lock(_mutex);
    if (_updating) return false;
    if (!paused && !_userPaused && !_logic.paused() && !_testMode && _pauseStorageHealthy) return true;
    const bool alreadySaved=_pauseStorageHealthy&&_storedPaused==paused;
    // Meteen stoppen; bij opslagfout nooit alvast hervatten.
    _userPaused=true; _logic.pause(true,millis());
    _manualActive=false; _manual.stop(); _cue.stop(); _startupCuePending=false;
    _driverInitPending=false;
    if (_initialized) hapticControl.stopAll();
    if (alreadySaved) { apply(); return true; }
  }
  Preferences prefs;
  bool ok=prefs.begin(APP_PREFERENCES_NAMESPACE,false);
  if (ok) { ok=prefs.putBool("paused",paused)==1; prefs.end(); }
  Lock lock(_mutex);
  _pauseStorageHealthy=ok;
  if (ok) apply();
  return ok;
}
void StabilizerControl::stopForRestart() {
  if (!_mutex) return;
  Lock lock(_mutex);
  _updating=true; _manualActive=false; _manual.stop(); _cue.stop(); _startupCuePending=false;
  _logic.pause(true,millis()); hapticControl.stopAll();
}
/**
 * @brief Plant een handmatige motorpuls zonder de opgeslagen pauze op te heffen.
 * @param motor Motorindex 0-3.
 * @param strength Positieve signed-RTP-waarde 1-127.
 * @param durationMs Totale duur inclusief uit-fasen, maximaal 3000 ms voor GUI-patronen.
 * @param pattern Gebruik het opgeslagen pulspatroon; anders maximaal 1000 ms continu.
 * @return True als IMU, voeding en motoren beschikbaar zijn en de puls is ingepland.
 */
bool StabilizerControl::testPulse(uint8_t motor, uint8_t strength, uint32_t durationMs, bool pattern) {
  if (!_mutex || motor >= 4 || strength < 1 || strength > 127 || !durationMs || durationMs > (pattern ? 3000U : 1000U)) return false;
  Lock lock(_mutex);
  if (_updating) return false;
  if (!_initialized || _power.critical || _power.shuttingDown || !imuSensor.healthy()) return false;
  if (!hapticControl.stopAll()) return false;
  if (!hapticControl.healthy() || !hapticControl.enabled()) return false;
  _cue.stop();
  if (!hapticControl.prepare(motor)) return false;
  const auto& settings = _logic.settings;
  const uint8_t mode = pattern ? settings.pattern : uint8_t(HAPTIC_CONTINUOUS);
  const uint32_t limit = settings.feedbackLimitMs();
  if (mode != HAPTIC_CONTINUOUS && limit && limit < durationMs) durationMs = limit;
  _manual.start(millis(),durationMs,strength,mode,settings.pulseOnMs,settings.pulseOffMs,uint8_t(1U<<motor));
  _manualActive=true;
  return _manualActive;
}
bool StabilizerControl::setTestMode(bool enabled) {
  if (!_mutex) return false;
  auto settings=snapshot().settings;
  settings.testMode=enabled;
  return saveSettings(settings);
}
/** @brief Blokkeert feedback voor OTA; slaagt pas na bevestigde stop van alle motoren. */
bool StabilizerControl::beginFirmwareUpdate() {
  if (!_mutex) return false;
  Lock lock(_mutex);
  if (_updating || _power.critical || _power.shuttingDown) return false;
  _manual.stop(); _cue.stop(); _manualActive=false; _startupCuePending=false;
  _logic.pause(true,millis());
  if (!hapticControl.stopAll()) return false;
  _updating=true;
  return true;
}
void StabilizerControl::endFirmwareUpdate() {
  if (!_mutex) return;
  Lock lock(_mutex);
  _updating=false; // Na een fout bewust gepauzeerd; gebruiker kan hervatten.
}
