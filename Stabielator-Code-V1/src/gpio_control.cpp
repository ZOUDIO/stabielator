/**
 * @file gpio_control.cpp
 * @brief Verwerkt de BOOT-knop en stuurt de common-anode RGB-statusled aan.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "gpio_control.h"
#include "led_status.h"
GpioControl gpioControl;
void GpioControl::begin() {
  // Common anode: HIGH is uit. Zet eerst de uitgangslatches.
  for (uint8_t pin : {RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN}) {
    digitalWrite(pin, RGB_COMMON_ANODE ? HIGH : LOW);
    pinMode(pin, OUTPUT);
  }
  color(0, 0, 0);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}
void GpioControl::color(uint8_t r, uint8_t g, uint8_t b) {
  _led={r,g,b};
  analogWrite(RGB_RED_PIN, RGB_COMMON_ANODE ? 255 - r : r);
  analogWrite(RGB_GREEN_PIN, RGB_COMMON_ANODE ? 255 - g : g);
  analogWrite(RGB_BLUE_PIN, RGB_COMMON_ANODE ? 255 - b : b);
}
void GpioControl::update(bool connected, bool hardwareHealthy,
                         StabilizerState state, bool batteryLow, bool ledEnabled, bool motorActive, uint8_t ledBrightness, bool userPaused) {
  const uint32_t now = millis();
  const bool reading = digitalRead(BUTTON_PIN);
  if (reading != _raw) { _raw = reading; _changed = now; }
  if (now - _changed >= BUTTON_DEBOUNCE_MS && _stable != _raw) {
    _stable = _raw;
    if (_stable == LOW) { _pressed = now; _handled = false; }
  }
  if (_stable == LOW && !_handled && now - _pressed >= WIFI_PAIR_HOLD_MS) {
    _handled = true; _request = true;
  }
  // Pauze-indicatie blijft zichtbaar, ook als de gewone statusled was uitgezet.
  const auto brightness=userPaused&&(!ledEnabled||ledBrightness==0)?100:ledBrightness;
  const auto led = dimLed(statusLed(now, ledEnabled, batteryLow, hardwareHealthy, state, motorActive, connected,userPaused),brightness);
  color(led.red, led.green, led.blue);
}
bool GpioControl::consumePairingRequest() {
  const bool result = _request; _request = false; return result;
}
