/**
 * @file test_main.cpp
 * @brief Controleert begrensde GUI-patronen en ledprioriteiten zonder hardware.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "../../src/haptic_pattern.h"
#include "../../src/led_status.h"
#include <cassert>

int main() {
  auto paused=statusLed(0,false,false,true,StabilizerState::Paused,false,true,true);
  assert(paused.red==255&&paused.green==0&&paused.blue==0); // Pauze blijft rood, ook bij LED uit.
  paused=statusLed(0,false,false,true,StabilizerState::Paused,true,true,true);
  assert(paused.red==255&&paused.green==80&&paused.blue==0); // Expliciete test is tijdelijk oranje.
  paused=statusLed(200,false,false,false,StabilizerState::HardwareFault,false,true,true);
  assert(paused.red==255 && paused.blue==255 && paused.green==0);
  auto dimmed=dimLed({255,80,0},25);
  assert(dimmed.red==64 && dimmed.green==20 && dimmed.blue==0);
  dimmed=dimLed({255,80,255},0);
  assert(dimmed.red==0 && dimmed.green==0 && dimmed.blue==0);
  dimmed=dimLed({255,80,255},100);
  assert(dimmed.red==255 && dimmed.green==80 && dimmed.blue==255);
  HapticPattern p;
  p.start(100,1200,64,true,200,200,2);
  assert(p.level(100,1)==64 && p.level(299,1)==64 && p.level(300,1)==0);
  assert(p.level(499,1)==0 && p.level(500,1)==64 && p.level(100,0)==0);
  assert(p.active(1299) && !p.active(1300) && p.level(1300,1)==0);
  p.start(0xFFFFFF00U,600,64,true,200,200,15);
  for (unsigned m=0;m<4;++m) assert(p.level(0xFFFFFF01U,m)==64);
  assert(p.active(100) && !p.active(344));
  p.stop();assert(!p.active(0));
  // Kalibratiebevestiging: twee korte pulsen op vier motoren.
  p.start(0,300,64,true,100,100,15);
  for(unsigned m=0;m<4;++m) {
    assert(p.level(50,m)==64 && p.level(150,m)==0 && p.level(250,m)==64 && p.level(300,m)==0);
  }
  auto led=statusLed(1000,true,false,true,StabilizerState::Active,false,false);
  assert(led.green==255 && led.red==0 && led.blue==0);
  led=statusLed(1000,true,false,true,StabilizerState::Active,true,true);
  assert(led.red==255 && led.green==80 && led.blue==0);
  led=statusLed(1000,true,true,false,StabilizerState::SensorFault,true,true);
  assert(led.red==255 && led.green==0 && led.blue==0); // Accu boven fout/motor.
  led=statusLed(1000,false,true,false,StabilizerState::SensorFault,true,true);
  assert(led.red==0 && led.green==0 && led.blue==0);
  led=statusLed(1000,true,false,false,StabilizerState::SensorFault,false,false);
  assert(led.red==255 && led.blue==255 && led.green==0);
}
