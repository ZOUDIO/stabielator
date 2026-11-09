/**
 * @file test_main.cpp
 * @brief Controleert quaternionomzetting en Eulerhoekgrenzen zonder hardware.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "../../src/orientation.h"
#include <cassert>

void expect(float w, float x, float y, float z, float roll, float pitch, float yaw) {
  const auto a = quaternionToDegrees(w, x, y, z);
  assert(std::fabs(a.roll - roll) < 0.05f);
  assert(std::fabs(a.pitch - pitch) < 0.05f);
  assert(std::fabs(a.yaw - yaw) < 0.05f);
}

int main() {
  constexpr float s = 0.707106781f;
  expect(1, 0, 0, 0, 0, 0, 0);
  expect(s, s, 0, 0, 90, 0, 0);
  expect(s, -s, 0, 0, -90, 0, 0);
  expect(s, 0, s, 0, 0, 90, 0);
  expect(s, 0, -s, 0, 0, -90, 0);
  expect(s, 0, 0, s, 0, 0, 90);
  expect(s, 0, 0, -s, 0, 0, -90);
  // Samengestelde rotatie: roll 30, pitch 20, yaw -40 graden.
  expect(0.8785122f, 0.2968829f, 0.0704393f, -0.3675801f, 30, 20, -40);
  // Schaal en teken van een quaternion veranderen de rotatie niet.
  expect(2*s, 2*s, 0, 0, 90, 0, 0);
  expect(-s, -s, 0, 0, 90, 0, 0);
  // Afgeronde sensorsamples vlak bij de pitch-singulariteit blijven eindig.
  const auto nearPole = quaternionToDegrees(0.707107f, 0, 0.707106f, 0);
  assert(std::isfinite(nearPole.pitch));
  assert(nearPole.pitch <= 90 && nearPole.pitch > 89.9f);
  assert(std::isnan(quaternionToDegrees(0, 0, 0, 0).roll));
  assert(std::isnan(quaternionToDegrees(NAN, 0, 0, 0).roll));
  assert(std::isnan(quaternionToDegrees(INFINITY, 0, 0, 0).roll));
}
