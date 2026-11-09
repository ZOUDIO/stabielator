/**
 * @file gpio_control.h
 * @brief Interface voor knopbediening, WiFi-herstelverzoeken en actuele ledkleur.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include "config.h"
#include "stabilizer_logic.h"
#include "led_status.h"
class GpioControl {
 public:
  void begin();
  void update(bool connected, bool hardwareHealthy,
              StabilizerState state, bool batteryLow, bool ledEnabled, bool motorActive, uint8_t ledBrightness, bool userPaused);
  bool consumePairingRequest();
  LedColor ledColor() const { return _led; }
 private:
  void color(uint8_t r, uint8_t g, uint8_t b);
  bool _raw = HIGH, _stable = HIGH, _handled = false, _request = false;
  uint32_t _changed = 0, _pressed = 0;
  LedColor _led{0,0,0};
};
extern GpioControl gpioControl;
