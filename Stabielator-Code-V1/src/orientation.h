/**
 * @file orientation.h
 * @brief Zet quaternions om naar ZYX-Eulerhoeken in graden voor de gebruikersinterface.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <cmath>

struct OrientationDegrees {
  float roll, pitch, yaw;
};

/**
 * @brief Berekent absolute ZYX-Eulerhoeken uit een quaternion in volgorde w,x,y,z.
 * @return Roll/yaw in [-180,180] en pitch in [-90,90] graden; NAN bij ongeldige norm.
 * @details Roll en yaw zijn gekoppeld bij pitch +/-90 graden. Deze omzetting past
 * geen montagekalibratie toe; de regeling berekent kanteling afzonderlijk.
 */
inline OrientationDegrees quaternionToDegrees(float w, float x, float y, float z) {
  const float normSquared = w * w + x * x + y * y + z * z;
  if (!std::isfinite(normSquared) || normSquared < 1e-12f) return {NAN, NAN, NAN};
  const float scale = 2.0f / normSquared;
  const float sinPitch = scale * (w * y - z * x);
  const float clampedPitch = sinPitch > 1 ? 1 : (sinPitch < -1 ? -1 : sinPitch);
  constexpr float toDegrees = 57.295779513f;
  return {
    std::atan2(scale * (w * x + y * z), 1 - scale * (x * x + y * y)) * toDegrees,
    std::asin(clampedPitch) * toDegrees,
    std::atan2(scale * (w * z + x * y), 1 - scale * (y * y + z * z)) * toDegrees
  };
}
