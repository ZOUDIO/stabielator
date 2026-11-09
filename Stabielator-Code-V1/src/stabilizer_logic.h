/**
 * @file stabilizer_logic.h
 * @brief Houdingskalibratie, kantelhysterese, trilduurlimieten en inactiviteitsbewaking.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include "orientation.h"

struct StabilizerSettings {
  uint32_t version = 5;
  float onDegrees = 1.5f, offDegrees = 1.0f;
  uint32_t bootDelayMs = 3000, calibrationMs = 1000;
  uint32_t pulseOnMs = 200, pulseOffMs = 200, maxFeedbackMs = 3000;
  uint32_t inactivityMinutes = 60;
  float activityDegrees = 1.0f;
  float axisRotationDegrees = 0;
  float batteryLowVolts = 3.5f, batteryCriticalVolts = 3.2f;
  uint8_t strength = 64, pattern = 1, diagonal = 1, ledEnabled = 1;
  uint8_t debugEnabled = 0;
  uint8_t motorMap[4] = {3, 1, 2, 0}; // +X -> motor 4, -X -> 2, +Y -> 3, -Y -> 1.
  uint8_t ledBrightness = 100; // Procenten; gebruikt een voormalige paddingbyte.
  uint8_t testMode = 0;
};
static_assert(offsetof(StabilizerSettings,ledBrightness)==61 &&
              offsetof(StabilizerSettings,testMode)==62 && sizeof(StabilizerSettings)==64,
              "Controleer compatibiliteit met opgeslagen instellingen.");

/** @brief Valideert eindige meetgrenzen, tijdslimieten en een unieke motor per richting. */
inline bool validSettings(const StabilizerSettings& s) {
  if (s.version != 5 || s.testMode > 1 || s.ledBrightness > 100 || !std::isfinite(s.onDegrees) || !std::isfinite(s.offDegrees) ||
      s.onDegrees < 0.1f || s.onDegrees > 60 || s.offDegrees < 0 ||
      s.offDegrees >= s.onDegrees || s.bootDelayMs > 60000 ||
      s.calibrationMs < 500 || s.calibrationMs > 5000 ||
      s.pulseOnMs < 20 || s.pulseOnMs > 1000 || s.pulseOffMs < 20 ||
      s.pulseOffMs > 5000 || s.maxFeedbackMs > 60000 || s.inactivityMinutes > 1440 ||
      !std::isfinite(s.activityDegrees) || s.activityDegrees < 0.1f || s.activityDegrees > 30 ||
      !std::isfinite(s.axisRotationDegrees) || std::fabs(s.axisRotationDegrees) > 180 ||
      s.strength < 1 || s.strength > 127 || s.pattern > 1 || s.diagonal > 1 ||
      s.ledEnabled > 1 || s.debugEnabled > 1 || !std::isfinite(s.batteryLowVolts) ||
      !std::isfinite(s.batteryCriticalVolts) || s.batteryCriticalVolts < 3.0f ||
      s.batteryLowVolts > 4.1f || s.batteryCriticalVolts >= s.batteryLowVolts) return false;
  unsigned mask = 0;
  for (auto m : s.motorMap) { if (m > 3) return false; mask |= 1U << m; }
  return mask == 15;
}

/**
 * @brief Valideert een opgeslagen record en migreert ondersteunde oudere versies.
 * @details Versie 1/2 krijgt standaardinstellingen; versie 3/4 behoudt compatibele waarden.
 * De aparte opgeslagen gebruikerspauze wordt hierdoor niet gewijzigd.
 * @param bytes Opgeslagen record met minimaal een versienummer.
 * @param size Aantal beschikbare bytes.
 * @param result Ontvangt alleen bij succes de geldige instellingen.
 * @param reset Wordt true als het gemigreerde record opnieuw moet worden opgeslagen.
 * @return False bij een onbekende versie, onjuiste lengte of ongeldige instellingen.
 */
inline bool decodeSettingsRecord(const void* bytes, size_t size, StabilizerSettings& result, bool& reset) {
  reset=false;
  if (size < sizeof(uint32_t)) return false;
  uint32_t version;
  std::memcpy(&version,bytes,sizeof(version));
  if (version==1 || version==2) { result=StabilizerSettings{}; reset=true; return true; }
  if ((version!=3 && version!=4 && version!=5) || size!=sizeof(result)) return false;
  StabilizerSettings candidate;
  std::memcpy(&candidate,bytes,sizeof(candidate));
  if (version==3) candidate.ledBrightness=100;
  if (version<5) {
    // v4 gebruikte debugEnabled ook als opgeslagen teststand. Behoud de motorpauze.
    candidate.testMode=version==4 ? candidate.debugEnabled : 0;
    candidate.version=5;
  }
  if (!validSettings(candidate)) return false;
  reset=version<5;
  result=candidate; return true;
}

struct Quaternion {
  float w = 1, x = 0, y = 0, z = 0;
  bool normalize() {
    const float n = w*w + x*x + y*y + z*z;
    if (!std::isfinite(n) || n < 0.5f || n > 1.5f) return false;
    const float k = 1 / std::sqrt(n);
    w *= k; x *= k; y *= k; z *= k;
    return true;
  }
};
inline float quaternionDot(const Quaternion& a, const Quaternion& b) {
  return a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
}
inline Quaternion relativeQuaternion(const Quaternion& a, const Quaternion& b) {
  return {a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z,
          a.w*b.x-a.x*b.w-a.y*b.z+a.z*b.y,
          a.w*b.y+a.x*b.z-a.y*b.w-a.z*b.x,
          a.w*b.z-a.x*b.y+a.y*b.x-a.z*b.w};
}
inline float quaternionSeparationDegrees(const Quaternion& a, const Quaternion& b) {
  const auto relative = relativeQuaternion(a,b);
  const float vectorLength = std::sqrt(relative.x*relative.x + relative.y*relative.y + relative.z*relative.z);
  return 114.591559026f * std::atan2(vectorLength,std::fabs(relative.w));
}

enum class StabilizerState { Waiting, Calibrating, Active, SensorFault, HardwareFault, Paused, Inactive };
inline const char* stateName(StabilizerState state) {
  switch (state) {
    case StabilizerState::Waiting: return "waiting";
    case StabilizerState::Calibrating: return "calibrating";
    case StabilizerState::Active: return "active";
    case StabilizerState::SensorFault: return "imu_fault";
    case StabilizerState::HardwareFault: return "motor_fault";
    case StabilizerState::Paused: return "paused";
    case StabilizerState::Inactive: return "inactive";
  }
  return "unknown";
}

// Pure regelkern: geen I2C, delays, webverkeer of flashwrites.
class StabilizerLogic {
 public:
  StabilizerSettings settings;
  StabilizerState state = StabilizerState::Waiting;
  uint8_t outputs[4] = {};
  bool referenceValid = false;
  float roll = 0, pitch = 0, yaw = 0;
  float tiltRoll = 0, tiltPitch = 0;
  uint32_t calibrationSamples = 0;

  void begin(uint32_t now) {
    _activityAt = now; _haveActivity = false; _paused = false;
    calibrate(now, false);
  }
  void calibrate(uint32_t now, bool immediate = true) {
    referenceValid = false; _waitAt = now; _waitMs = immediate ? 0 : settings.bootDelayMs;
    resetCalibration(); clearDirections();
    for (auto& limited : _limited) limited = false;
    _haveActivity = false;
    state = StabilizerState::Waiting;
  }
  void pause(bool paused, uint32_t now) {
    _paused = paused; stopOutputs();
    if (paused) state = StabilizerState::Paused;
    else if (!referenceValid) calibrate(now);
  }
  void clearDirections() {
    for (unsigned n = 0; n < 4; ++n) { _active[n] = false; outputs[n] = 0; }
  }
  void stopOutputs() { for (auto& out : outputs) out=0; }
  bool applySettings(const StabilizerSettings& candidate) {
    if (!validSettings(candidate)) return false;
    settings=candidate; stopOutputs(); // Geen herbewapening of nieuwe trilduurtimer.
    return true;
  }
  uint32_t inactivityMs(uint32_t now) const { return _haveActivity ? now-_activityAt : 0; }
  bool paused() const { return _paused; }
  Quaternion relativeOrientation(const Quaternion& q) const { return relativeQuaternion(_reference,q); }
  /**
   * @brief Werkt kalibratie, inactiviteit en directionele feedback bij.
   * @param now Actuele millis-teller; verschillen ondersteunen een telleromloop.
   * @param q Absolute IMU-quaternion, intern genormaliseerd en gecontroleerd.
   * @param valid Geeft aan dat de meting ontvangen en nog actueel is.
   * @param motorsReady Alle drivers zijn beschikbaar en correct geconfigureerd.
   * @param sequence Samplevolgnummer; dubbele samples tellen niet mee voor kalibratie.
   * @details Een trilduurlimiet blijft geblokkeerd tot terugkeer onder de uitschakelhoek.
   * Kalibratie middelt tekenuitgelijnde quaternions uit een stil meetvenster.
   */
  void update(uint32_t now, Quaternion q, bool valid, bool motorsReady, uint32_t sequence) {
    for (auto& out : outputs) out = 0;
    valid = valid && q.normalize();
    if (referenceValid && valid) updateOrientation(q);
    // Alleen unieke, geldige samples bewijzen stilstand. Een meetgat begint een nieuw venster.
    if (!valid) _haveActivity=false;
    else if (!_haveActivity || now-_activitySampleAt > 250) {
      _activityAnchor=q; _activityAt=now; _activitySequence=sequence; _activitySampleAt=now; _haveActivity=true;
    } else if (sequence != _activitySequence) {
      _activitySequence=sequence; _activitySampleAt=now;
      const float dot=std::fmin(1.0f,std::fabs(quaternionDot(_activityAnchor,q)));
      if (2*std::acos(dot)*57.295779513f >= settings.activityDegrees) { _activityAnchor=q; _activityAt=now; }
      if (settings.inactivityMinutes && now-_activityAt >= settings.inactivityMinutes*60000U) {
        stopOutputs(); state=StabilizerState::Inactive; return;
      }
    }
    if (!motorsReady) { state = StabilizerState::HardwareFault; return; }
    if (!valid) {
      if (!referenceValid) resetCalibration();
      state = StabilizerState::SensorFault; return;
    }
    if (_paused && referenceValid) { state = StabilizerState::Paused; return; }
    if (!referenceValid) {
      if (now - _waitAt < _waitMs) { state = StabilizerState::Waiting; return; }
      state = StabilizerState::Calibrating;
      if (_haveSequence && sequence == _sequence) return;
      _sequence = sequence; _haveSequence = true;
      // Maximaal 0,25 graden vanaf het eerste sample, kleiner bij lage kanteldrempels.
      // Daarmee blijft de volledige spreiding onder 0,5 graden bij de defaults.
      const float motionLimit = std::fmin(0.25f,settings.onDegrees*0.25f);
      if (calibrationSamples && quaternionSeparationDegrees(_anchor,q) > motionLimit)
        resetCalibration();
      _sequence = sequence; _haveSequence = true;
      if (!calibrationSamples) { _anchor = q; _sampleAt = now; }
      const float sign = quaternionDot(_anchor, q) < 0 ? -1.0f : 1.0f;
      _sum.w += sign*q.w; _sum.x += sign*q.x; _sum.y += sign*q.y; _sum.z += sign*q.z;
      ++calibrationSamples;
      if (now - _sampleAt < settings.calibrationMs || calibrationSamples < 20) return;
      const float scale = 1.0f / calibrationSamples;
      _reference = {_sum.w*scale, _sum.x*scale, _sum.y*scale, _sum.z*scale};
      referenceValid = _reference.normalize();
      if (!referenceValid) { resetCalibration(); return; }
      clearDirections();
    }
    if (_paused) { state=StabilizerState::Paused; return; }
    state = StabilizerState::Active;
    updateOrientation(q);
    const float direction[4] = {tiltRoll, -tiltRoll, tiltPitch, -tiltPitch};
    int strongest = -1;
    for (unsigned n = 0; n < 4; ++n) {
      if (direction[n] <= settings.offDegrees) { _active[n] = false; _limited[n] = false; }
      else if (!_active[n] && !_limited[n] && direction[n] >= settings.onDegrees) {
        _active[n] = true; _activeAt[n] = now;
      }
      if (_active[n] && settings.maxFeedbackMs && now - _activeAt[n] >= settings.maxFeedbackMs) {
        _active[n] = false; _limited[n] = true; // Pas herbewapenen onder off-grens.
      }
      if (_active[n] && (strongest < 0 || direction[n] > direction[strongest])) strongest = n;
    }
    for (unsigned n = 0; n < 4; ++n) {
      if (!_active[n] || (!settings.diagonal && strongest != int(n))) continue;
      const bool on = !settings.pattern ||
        (now - _activeAt[n]) % (settings.pulseOnMs + settings.pulseOffMs) < settings.pulseOnMs;
      if (on) outputs[settings.motorMap[n]] = settings.strength;
    }
  }
 private:
  struct Vector { float x, y, z; };
  static Vector cross(Vector a, Vector b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
  }
  static float dot(Vector a, Vector b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
  static Vector gravity(Quaternion q) {
    return {2*(q.x*q.z-q.w*q.y), 2*(q.y*q.z+q.w*q.x), 1-2*(q.x*q.x+q.y*q.y)};
  }
  /** @brief Projecteert kanteling op referentieassen; draaien om wereldverticaal telt niet mee. */
  void updateOrientation(Quaternion q) {
    const auto relative = relativeQuaternion(_reference, q);
    const auto angles = quaternionToDegrees(relative.w,relative.x,relative.y,relative.z);
    roll=angles.roll; pitch=angles.pitch; yaw=angles.yaw;
    // Zwaartekracht in sensorkader: wereld-yaw valt weg, ook bij schuine montage.
    const Vector ref=gravity(_reference), current=gravity(q);
    Vector axisX{1-ref.x*ref.x, -ref.x*ref.y, -ref.x*ref.z};
    float length=std::sqrt(dot(axisX,axisX));
    if(length<0.001f){axisX=cross({0,1,0},ref);length=std::sqrt(dot(axisX,axisX));}
    axisX={axisX.x/length,axisX.y/length,axisX.z/length};
    const Vector axisY=cross(ref,axisX), axis=cross(current,ref);
    const float sine=std::sqrt(dot(axis,axis));
    const float angle=std::atan2(sine,dot(current,ref))*57.295779513f;
    float x=sine>0.00001f?angle*dot(axis,axisX)/sine:(angle>90?180.0f:0.0f);
    float y=sine>0.00001f?angle*dot(axis,axisY)/sine:0.0f;
    const float rotation=settings.axisRotationDegrees*0.01745329252f;
    tiltRoll=std::cos(rotation)*x-std::sin(rotation)*y;
    tiltPitch=std::sin(rotation)*x+std::cos(rotation)*y;
  }
  void resetCalibration() { calibrationSamples = 0; _sum = {0,0,0,0}; _haveSequence = false; }
  Quaternion _reference, _anchor, _sum{0,0,0,0};
  uint32_t _activityAt=0, _activitySampleAt=0, _activitySequence=0;
  Quaternion _activityAnchor;
  bool _haveActivity=false;
  uint32_t _waitAt = 0, _waitMs = 0, _sampleAt = 0, _sequence = 0;
  uint32_t _activeAt[4] = {};
  bool _active[4] = {}, _limited[4] = {}, _haveSequence = false, _paused = false;
};
