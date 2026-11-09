/**
 * @file power_monitor.h
 * @brief Bewaakt LiPo-spanning, accuwaarschuwing en vertraagde kritieke uitschakeling.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include "config.h"

class PowerMonitor {
 public:
  // Grove spanningsschatting voor een 1-cels LiPo, geen coulombmeting.
  int estimatedPercent() const {
    if (!available) return -1;
    const float voltage[] = {3.2f,3.5f,3.6f,3.7f,3.8f,3.9f,4.0f,4.1f,4.2f};
    const int percent[] = {0,5,10,20,40,60,75,90,100};
    if (volts <= voltage[0]) return 0;
    for (unsigned i=1; i<9; ++i)
      if (volts < voltage[i]) return int(percent[i-1] +
        (volts-voltage[i-1])/(voltage[i]-voltage[i-1])*(percent[i]-percent[i-1]) + 0.5f);
    return 100;
  }
  void begin() {
    digitalWrite(KILL_PIN, LOW);
    pinMode(KILL_PIN, OUTPUT);
    analogReadResolution(12);
    // Standaard voor de eerste ADC-attach; de pin bestaat hier nog niet als ADC-kanaal.
    analogSetAttenuation(ADC_11db);
  }
  /**
   * @brief Meet elke 100 ms en bevestigt kritieke gefilterde spanning na 3000 ms.
   * @details Ontbrekende/ongeloofwaardige accuspanning wordt als niet beschikbaar gemeld.
   * Uitschakelen gebeurt door de regeltaak, zodat motorstop voor KILL wordt uitgevoerd.
   */
  void update(uint32_t now, float lowVolts, float criticalVolts) {
    if (_sampled && now - _lastSample < 100) return;
    _sampled = true; _lastSample = now;
    const float measured = analogReadMilliVolts(BATTERY_PIN) * 0.001f * BATTERY_DIVIDER_RATIO;
    // Een niet-aangesloten accu bij USB-voeding krijgt geen verzonnen percentage.
    available = measured >= 1.0f && measured <= 5.0f;
    if (!available) { volts = 0; low = critical = false; _below = false; return; }
    volts = volts > 0 ? volts * 0.9f + measured * 0.1f : measured;
    low = volts < lowVolts;
    if (volts < criticalVolts) {
      if (!_below) { _below = true; _belowAt = now; }
      critical = now - _belowAt >= 3000;
    } else { _below = false; critical = false; }
  }
  /// @brief Vergrendelt de uitschakelstatus; de aanroeper stopt eerst alle motoren.
  void shutdown() { shuttingDown = true; digitalWrite(KILL_PIN, HIGH); }
  float volts = 0;
  bool available = false, low = false, critical = false, shuttingDown = false;
 private:
  uint32_t _lastSample = 0, _belowAt = 0;
  bool _sampled = false, _below = false;
};
