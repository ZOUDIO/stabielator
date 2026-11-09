/**
 * @file haptic_pattern.h
 * @brief Tijdbegrensde puls- en bevestigingspatronen met selectie per motor.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <cstdint>

#define DOUBLE_PULSE_GAP_MULTIPLIER 3U

enum HapticMode : uint8_t { HAPTIC_CONTINUOUS, HAPTIC_PULSES, HAPTIC_DOUBLE_PULSES };

inline bool hapticPatternOn(uint8_t mode, uint32_t elapsed, uint32_t onMs, uint32_t offMs) {
  if (mode == HAPTIC_CONTINUOUS) return true;
  const uint32_t period = onMs + offMs;
  if (mode == HAPTIC_DOUBLE_PULSES) {
    const uint32_t pairDuration = period + onMs;
    if (elapsed >= pairDuration) {
      const uint32_t secondPair = pairDuration + DOUBLE_PULSE_GAP_MULTIPLIER * offMs;
      if (elapsed < secondPair) return false;
      elapsed -= secondPair;
      if (elapsed >= pairDuration) return false;
    }
  }
  return elapsed % period < onMs;
}

// Alleen envelop/tijd: geen claim over de mechanische motorfrequentie.
class HapticPattern {
 public:
  /**
   * @brief Start een patroon; duration omvat aan- en uit-fasen in milliseconden.
   * @pre De aanroeper valideert sterkte, motorbits en pulstijden; onMs is positief.
   * @details Maskerbit 0-3 selecteert motor 1-4. Tijdverschillen verdragen millis-omloop.
   */
  void start(uint32_t now, uint32_t duration, uint8_t strength,
             uint8_t mode, uint32_t onMs, uint32_t offMs, uint8_t mask) {
    _at=now; _duration=duration; _strength=strength; _mode=mode;
    _on=onMs; _off=offMs; _mask=mask; _active=duration>0 && onMs>0 && mask>0;
  }
  void stop() { _active=false; }
  bool active(uint32_t now) const { return _active && now-_at < _duration; }
  uint8_t level(uint32_t now, unsigned motor) const {
    if (!active(now) || motor>=4 || !(_mask & (1U<<motor))) return 0;
    return hapticPatternOn(_mode, now-_at, _on, _off) ? _strength : 0;
  }
 private:
  uint32_t _at=0, _duration=0, _on=1, _off=0;
  uint8_t _strength=0, _mask=0, _mode=HAPTIC_CONTINUOUS;
  bool _active=false;
};
