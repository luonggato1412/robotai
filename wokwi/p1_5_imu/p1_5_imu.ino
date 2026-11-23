// TU SINH bang tools/wokwi_pack.py - dung sua o day, sua trong libraries/ roi chay lai script.
// Bước 5: MPU6050. Đi thẳng giữ hướng bằng gyro (chữa xe bị lệch), nhấc xe lên là động cơ dừng.
// Gõ 'g' để chạy 3 s thẳng, 'p' để chỉ in số liệu.
#include "robot_lib.h"  // Wokwi: gop san thu vien, xem wokwi/README.md

RobotMotors motors;
RobotImu imu;
const float KP = 6.0f;  // tăng nếu vẫn lệch, giảm nếu xe lắc qua lại

void driveStraight(int speed, uint32_t ms) {
  imu.resetYaw();
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    imu.update();
    if (imu.lifted()) { motors.brake(); Serial.println("Bi nhac len!"); return; }
    int corr = (int)(KP * imu.yawDeg());  // yaw > 0 = lệch trái -> thêm lực bánh trái
    motors.drive(speed + corr, speed - corr);
    delay(5);
  }
  motors.brake();
}

void setup() {
  Serial.begin(115200);
  motors.begin();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  if (!imu.begin()) { Serial.println("Khong thay MPU6050!"); while (true) delay(1000); }
  Serial.println("Hieu chinh gyro, de xe dung yen...");
  imu.calibrate();
  Serial.println("Xong. 'g' = chay thang 3 s");
}

void loop() {
  imu.update();
  if (Serial.available() && Serial.read() == 'g') driveStraight(160, 3000);
  static uint32_t last = 0;
  if (millis() - last > 200) {
    last = millis();
    Serial.printf("yaw %.1f  pitch %.1f  roll %.1f  |a| %.2fg  %s\n", imu.yawDeg(), imu.pitchDeg(),
                  imu.rollDeg(), imu.accelG(), imu.lifted() ? "BI NHAC" : "");
  }
  delay(5);
}
