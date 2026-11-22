// Bước 6: đo pin 2S trên GPIO1. So sánh với đồng hồ vạn năng, lệch thì chỉnh BATT_DIVIDER_RATIO trong pins.h.
#include <RobotBattery.h>

void setup() { Serial.begin(115200); }

void loop() {
  float v = RobotBattery::volts();
  Serial.printf("Pin: %.2f V  (%d%%)\n", v, RobotBattery::percent(v));
  delay(1000);
}
