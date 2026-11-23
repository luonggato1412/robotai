// TU SINH bang tools/wokwi_pack.py - dung sua o day, sua trong libraries/ roi chay lai script.
// Bước 1: nháy LED RGB onboard đỏ -> xanh lá -> xanh dương. Kiểm tra board + nạp code.
#include "robot_lib.h"  // Wokwi: gop san thu vien, xem wokwi/README.md

void setup() { Serial.begin(115200); }

void loop() {
  rgb(40, 0, 0); Serial.println("Do");        delay(500);
  rgb(0, 40, 0); Serial.println("Xanh la");   delay(500);
  rgb(0, 0, 40); Serial.println("Xanh duong"); delay(500);
  rgb(0, 0, 0);  delay(500);
}
