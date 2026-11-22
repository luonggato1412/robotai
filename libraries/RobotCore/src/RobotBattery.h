// Đo pin 2S qua cầu chia trên GPIO1. analogReadMilliVolts() đã hiệu chỉnh theo eFuse.
#pragma once
#include <Arduino.h>
#include <pins.h>

namespace RobotBattery {
inline float volts() {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(PIN_BATT_ADC);
  return mv / 16.0f / 1000.0f * BATT_DIVIDER_RATIO;
}
// Đường xả Li-ion gần đúng, tính theo 1 cell.
inline int percent(float packV) {
  static const float v[] = {3.30f, 3.50f, 3.60f, 3.70f, 3.80f, 3.90f, 4.00f, 4.10f, 4.20f};
  static const int p[] = {0, 10, 20, 35, 50, 62, 75, 88, 100};
  float c = packV / BATT_CELLS;
  if (c <= v[0]) return 0;
  for (int i = 1; i < 9; i++)
    if (c < v[i]) return p[i - 1] + (int)((c - v[i - 1]) / (v[i] - v[i - 1]) * (p[i] - p[i - 1]));
  return 100;
}
}  // namespace RobotBattery
