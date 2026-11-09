/**
 * @file haptic_control.cpp
 * @brief Stuurt vier DRV2605L-drivers via de TCA9546A aan en bewaakt motorfouten.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "haptic_control.h"
#include <Wire.h>
HapticControl hapticControl;
namespace {
/** @brief Verstuurt een complete I2C-write en geeft de echte ACK-status terug. */
bool writeI2c(uint8_t address, const uint8_t* data, size_t size) {
  Wire.beginTransmission(address);
  const size_t written = Wire.write(data, size);
  const uint8_t result = Wire.endTransmission();
  return written == size && result == 0;
}
}
bool HapticControl::disconnect() {
  const uint8_t value = 0;
  return writeI2c(HAPTIC_MUX_ADDRESS, &value, 1);
}
bool HapticControl::select(uint8_t motor) {
  if (motor >= HAPTIC_MOTOR_COUNT) return false;
  const uint8_t value = uint8_t(1U << HAPTIC_CHANNELS[motor]);
  return writeI2c(HAPTIC_MUX_ADDRESS, &value, 1);
}
bool HapticControl::writeRegister(uint8_t reg, uint8_t value) {
  const uint8_t bytes[] = {reg, value};
  return writeI2c(HAPTIC_DRIVER_ADDRESS, bytes, sizeof(bytes));
}
bool HapticControl::readRegister(uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(HAPTIC_DRIVER_ADDRESS);
  Wire.write(reg);
  // Repeated START: requestFrom voltooit op ESP32 de gecombineerde transactie.
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(HAPTIC_DRIVER_ADDRESS, uint8_t(1)) != 1) return false;
  value = Wire.read();
  return true;
}
bool HapticControl::enabled() const {
  return HAPTIC_PROFILE_CONFIGURED && HAPTIC_RATED_VOLTAGE_REGISTER > 0 &&
         HAPTIC_OVERDRIVE_CLAMP_REGISTER > 0;
}
/**
 * @brief Kalibreert de geselecteerde LRA met een begrensde wachttijd, zonder OTP-writes.
 * @return True als de driver de kalibratie en alle afsluitende stopwrites bevestigt.
 */
bool HapticControl::calibrateLra() {
  bool completed = false, passed = false;
  const bool started = writeRegister(0x01, 0x07) && writeRegister(0x0C, 1);
  const uint32_t began = millis();
  while (started && millis() - began < HAPTIC_LRA_CALIBRATION_TIMEOUT_MS) {
    uint8_t go = 0;
    if (!readRegister(0x0C, go)) break;
    if (!(go & 1)) {
      completed = true;
      uint8_t status = 0;
      passed = readRegister(0x00, status) && (status & 0xE0) == 0xE0 &&
               !(status & 0x0B); // DIAG_RESULT, overtemp, overstroom.
      break;
    }
    delay(10);
  }
  // Ook bij fout/timeout alle stopwrites proberen. Geen OTP-programmering.
  const bool goStopped = writeRegister(0x0C, 0);
  const bool rtpStopped = writeRegister(0x02, 0);
  const bool standby = writeRegister(0x01, 0x40);
  return completed && passed && goStopped && rtpStopped && standby;
}
void HapticControl::begin(bool allowActuation) {
  _stopFailed=false;
  _nextHealthMotor=0;
  _lastHealthCheck=millis();
  _muxReady = disconnect();
  for (uint8_t motor = 0; motor < HAPTIC_MOTOR_COUNT; ++motor) {
    _running[motor] = false;
    _stopRequired[motor]=true;
    _calibrated[motor]=false;
    uint8_t status = 0;
    const bool selected = select(motor);
    const bool readOk = selected && readRegister(0x00, status);
    bool ok = selected && readOk && (status & 0xE0) == 0xE0 && !(status & 0x0B);
    if (selected) {
      const bool go = writeRegister(0x0C, 0);
      const bool rtp = writeRegister(0x02, 0);
      const bool standby = writeRegister(0x01, 0x40);
      // Ook bij een mislukte statusread kan een oude uitgang nog actief zijn.
      _stopRequired[motor] = !(go && rtp && standby);
      _lastStopAttempt[motor] = millis();
      if (_stopRequired[motor]) _stopFailed = true;
      ok = ok && go && rtp && standby;
    }
    else _stopFailed = true;
    if (ok && enabled()) {
      const bool rated = writeRegister(0x16, HAPTIC_RATED_VOLTAGE_REGISTER);
      const bool clamp = writeRegister(0x17, HAPTIC_OVERDRIVE_CLAMP_REGISTER);
      const bool feedback = writeRegister(0x1A, HAPTIC_FEEDBACK_REGISTER);
      const bool control1 = writeRegister(0x1B, HAPTIC_CONTROL1_REGISTER);
      const bool control2 = writeRegister(0x1C, HAPTIC_CONTROL2_REGISTER);
      const bool control3 = writeRegister(0x1D, HAPTIC_CONTROL3_REGISTER);
      ok = ok && rated && clamp && feedback && control1 && control2 && control3;
      if (HAPTIC_IS_LRA) {
        const bool control4 = writeRegister(0x1E, HAPTIC_CONTROL4_REGISTER);
        const bool control5 = writeRegister(0x1F, 0x80); // Geen automatische open-loop fallback.
        ok = ok && control4 && control5;
        if (ok && allowActuation) {
          ok = calibrateLra();
          if (!ok) stop(motor);
          _calibrated[motor]=ok;
          Serial.printf("Motor %u LRA-kalibratie: %s\n", motor+1, ok ? "geslaagd" : "mislukt/timeout");
        }
      }
    }
    _present[motor] = ok;
    if (!disconnect()) _muxReady = false;
    Serial.printf("Motor %u fysiek: %s\n", motor + 1,
                  ok ? "DRV2605L gevonden" : "niet beschikbaar");
  }
}
bool HapticControl::calibrationPending() const {
  if (!HAPTIC_IS_LRA) return false;
  for (unsigned m=0;m<HAPTIC_MOTOR_COUNT;++m) if (_present[m]&&!_calibrated[m]) return true;
  return false;
}
bool HapticControl::hardwarePresent(uint8_t motor) const {
  return motor < HAPTIC_MOTOR_COUNT && _muxReady && _present[motor];
}
bool HapticControl::present(uint8_t motor) const {
  return hardwarePresent(motor);
}
bool HapticControl::healthy() const {
  for (uint8_t motor = 0; motor < HAPTIC_MOTOR_COUNT; ++motor)
    if (!present(motor)) return false;
  return true;
}
bool HapticControl::setLevel(uint8_t motor, uint8_t strength) {
  if (motor >= HAPTIC_MOTOR_COUNT || strength > 127) return false;
  if (!strength) {
    if (!_running[motor] && !_stopRequired[motor]) return true;
    const bool stopped = stop(motor);
    const bool disconnected = disconnect();
    if (!disconnected) _muxReady = false;
    return stopped && disconnected;
  }
  if (_running[motor] && _strength[motor] == strength)
    return present(motor);
  return start(motor, strength);
}
bool HapticControl::start(uint8_t motor, uint8_t strength) {
  if (!present(motor) || strength == 0 || strength > 127) return false;
  if (!prepare(motor)) return false;
  // Wis een eventuele vorige opdracht voordat een nieuwe puls begint.
  bool ok = select(motor) && writeRegister(0x02, 0) &&
            writeRegister(0x01, 0x05) && writeRegister(0x02, strength);
  if (ok) {
    _running[motor] = true;
    _strength[motor] = strength;
  } else {
    stop(motor);
    _present[motor] = false;
  }
  if (!disconnect()) { _muxReady = false; return false; }
  return ok;
}
bool HapticControl::prepare(uint8_t motor) {
  if (!present(motor) || !enabled()) return false;
  // Expliciete handmatige test na gepauzeerd starten: alleen dit kanaal kalibreren.
  if (HAPTIC_IS_LRA && !_calibrated[motor]) {
    const bool calibrated=select(motor)&&calibrateLra();
    if (!calibrated) stop(motor);
    const bool disconnected=disconnect();
    if (!disconnected) _muxReady=false;
    if (!calibrated || !disconnected) { _present[motor]=false; return false; }
    _calibrated[motor]=true;
  }
  return true;
}
/**
 * @brief Probeert RTP, sequencer en standby onafhankelijk te stoppen.
 * @details Een mislukte stop blijft openstaan en wordt met minstens 100 ms tussenruimte
 * herhaald. De foutmelding blijft zichtbaar tot expliciete hardwareherinitialisatie.
 */
bool HapticControl::stop(uint8_t motor) {
  // Bij een blijvende busfout geen onbeperkte reeks stoptransacties in iedere tick.
  if (_stopRequired[motor] && millis()-_lastStopAttempt[motor]<100) return false;
  _lastStopAttempt[motor]=millis();
  if (!select(motor)) {
    _stopFailed=true;
    _stopRequired[motor]=true;
    _present[motor]=false;
    return false;
  }
  // Probeer standby ook wanneer een eerdere registerwrite mislukt.
  const bool rtpStopped = writeRegister(0x02, 0);
  const bool sequenceStopped = writeRegister(0x0C, 0);
  const bool standby = writeRegister(0x01, 0x40);
  const bool ok = rtpStopped && sequenceStopped && standby;
  _stopRequired[motor]=!ok;
  if (ok) _running[motor] = false;
  else { _stopFailed=true; _present[motor]=false; }
  return ok;
}
bool HapticControl::stopAll() {
  bool stopped=true;
  for (uint8_t motor = 0; motor < HAPTIC_MOTOR_COUNT; ++motor) {
    if ((_present[motor] || _running[motor] || _stopRequired[motor]) && !stop(motor)) stopped=false;
  }
  if (!disconnect()) {
    _muxReady=false;
    _stopFailed=true;
    stopped=false;
  }
  return stopped;
}
bool HapticControl::checkMotor(uint8_t motor) {
  uint8_t status=0, mode=0, clamp=0, rtp=0;
  bool ok=select(motor) && readRegister(0x00,status) &&
          (status & 0xE0)==0xE0 && !(status & 0x0B) &&
          readRegister(0x01,mode) && readRegister(0x17,clamp);
  const uint8_t expectedMode=_running[motor] ? 0x05 : 0x40;
  ok=ok && (mode & 0x47)==expectedMode && clamp==HAPTIC_OVERDRIVE_CLAMP_REGISTER;
  if (ok && _running[motor]) {
    ok=readRegister(0x02,rtp) && rtp==_strength[motor];
  }
  if (!ok) {
    // Ook onverwachte actieve uitgangen stoppen als de software ze niet als actief kende.
    stop(motor);
    _present[motor]=false;
    stopAll();
  }
  if (!disconnect()) {
    _muxReady=false;
    return false;
  }
  return ok;
}
void HapticControl::update() {
  // Stopverzoeken blijven lopen, ook voor drivers die niet meer als aanwezig gelden.
  for (uint8_t motor = 0; motor < HAPTIC_MOTOR_COUNT; ++motor)
    if (_stopRequired[motor]) { stop(motor); if (!disconnect()) _muxReady=false; }
  // Een kanaal per 25 ms: alle vier drivers worden iedere circa 100 ms gecontroleerd.
  if (millis()-_lastHealthCheck>=25) {
    _lastHealthCheck=millis();
    const uint8_t motor=_nextHealthMotor;
    _nextHealthMotor=uint8_t((_nextHealthMotor+1)%HAPTIC_MOTOR_COUNT);
    if (hardwarePresent(motor)) checkMotor(motor);
  }
}
