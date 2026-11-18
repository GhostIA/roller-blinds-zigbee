#pragma once

#include <cstdint>

// Pure percent <-> step conversions, kept free of Arduino headers so they can
// be unit tested on the host (pio test -e native).
//
// Steps:   0 = fully open, travelSteps = fully closed.
// Percent: 100 = fully open, 0 = fully closed.
namespace blindmath {

inline int32_t percentToSteps(uint8_t openPercent, int32_t travelSteps) {
  const int32_t open = openPercent > 100 ? 100 : openPercent;
  return (int32_t)((int64_t)(100 - open) * travelSteps / 100);
}

inline uint8_t stepsToPercent(int32_t steps, int32_t travelSteps) {
  if (steps < 0) steps = 0;
  if (steps > travelSteps) steps = travelSteps;
  const int64_t closed = ((int64_t)steps * 100 + travelSteps / 2) / travelSteps;
  return (uint8_t)(100 - closed);
}

}  // namespace blindmath