/**
 * @file led_status.h
 * @brief Bepaalt ledkleur en helderheid met fout- en pauzeprioriteiten.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <cstdint>
#include "stabilizer_logic.h"

struct LedColor { uint8_t red, green, blue; };
inline LedColor dimLed(LedColor color, uint8_t percent) {
  if (percent>100) percent=100;
  return {uint8_t((uint16_t(color.red)*percent+50)/100),
          uint8_t((uint16_t(color.green)*percent+50)/100),
          uint8_t((uint16_t(color.blue)*percent+50)/100)};
}
// Accu en hardwarefouten blijven zichtbaar tijdens opgeslagen pauze.
inline LedColor statusLed(uint32_t now, bool enabled, bool batteryLow,
                          bool hardwareHealthy, StabilizerState state,
                          bool motorActive, bool wifiClient, bool userPaused=false) {
  if (!enabled && !userPaused) return {0,0,0};
  if (batteryLow) return {255,0,0};
  if (!hardwareHealthy || state == StabilizerState::HardwareFault || state == StabilizerState::SensorFault)
    return (now / 200) % 2 ? LedColor{255,0,255} : LedColor{0,0,0};
  if (userPaused) return motorActive?LedColor{255,80,0}:LedColor{255,0,0};
  if (state == StabilizerState::Waiting)
    return (now / 500) % 2 ? LedColor{0,0,255} : LedColor{0,0,0};
  if (state == StabilizerState::Calibrating)
    return (now / 200) % 2 ? LedColor{0,255,255} : LedColor{0,0,0};
  if (motorActive) return {255,80,0};
  if (state == StabilizerState::Paused) return {255,0,0};
  if (state == StabilizerState::Inactive) return {0,0,0};
  // Korte blauwe flits per 2 s bij verbonden configuratieclient; verder normaal groen.
  if (wifiClient && now % 2000 < 100) return {0,0,255};
  return {0,255,0};
}
