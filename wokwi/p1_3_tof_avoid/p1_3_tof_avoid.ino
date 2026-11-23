// TU SINH bang tools/wokwi_pack.py - dung sua o day, sua trong libraries/ roi chay lai script.
// Bước 3: VL53L0X né vật cản. Đi thẳng, gặp vật < 15 cm thì dừng và quay phải tới khi thoáng.
// Thư viện: "VL53L0X" của Pololu (Library Manager).
#include <Wire.h>
#include "robot_lib.h"  // Wokwi: gop san thu vien, xem wokwi/README.md

const int STOP_MM = 150, CLEAR_MM = 250, SPEED = 150;
VL53L0X tof;
RobotMotors motors;

void setup() {
  Serial.begin(115200);
  motors.begin();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  tof.setTimeout(100);
  if (!tof.init()) { Serial.println("Khong thay VL53L0X!"); while (true) delay(1000); }
  tof.startContinuous(33);
  delay(1000);
}

void loop() {
  int mm = tof.readRangeContinuousMillimeters();
  bool bad = tof.timeoutOccurred() || mm >= 8000;  // 8190 = ngoài tầm
  if (!bad && mm < STOP_MM) {
    motors.brake(); delay(150);
    Serial.printf("Vat can %d mm -> quay\n", mm);
    uint32_t t0 = millis();
    motors.turnRight(SPEED);
    while (millis() - t0 < 2000) {  // tối đa 2 s
      int d = tof.readRangeContinuousMillimeters();
      if (d > CLEAR_MM) break;
    }
    motors.brake(); delay(100);
  } else {
    motors.forward(SPEED);
  }
}
