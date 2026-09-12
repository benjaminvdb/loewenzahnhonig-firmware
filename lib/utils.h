#pragma once

namespace loewy {

// Clamps value to the range [min, max].
inline float clamp(float value, float min, float max) {
  return value < min ? min : (value > max ? max : value);
}

}  // namespace loewy
