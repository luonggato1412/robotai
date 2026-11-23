// TU SINH bang tools/wokwi_pack.py - dung sua o day, sua trong libraries/ roi chay lai script.
// Bước 6: đo pin 2S trên GPIO1. So sánh với đồng hồ vạn năng, lệch thì chỉnh BATT_DIVIDER_RATIO trong pins.h.
#include "robot_lib.h"  // Wokwi: gop san thu vien, xem wokwi/README.md

void setup() { Serial.begin(115200); }

void loop() {
  float v = RobotBattery::volts();
  Serial.printf("Pin: %.2f V  (%d%%)\n", v, RobotBattery::percent(v));
  delay(1000);
}
