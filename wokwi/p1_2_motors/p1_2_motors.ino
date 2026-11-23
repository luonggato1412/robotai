// TU SINH bang tools/wokwi_pack.py - dung sua o day, sua trong libraries/ roi chay lai script.
// Bước 2: TB6612 + 2 N20. KÊ XE LÊN cho xích quay tự do. Chạy lần lượt tiến/lùi/trái/phải, tăng dần tốc độ.
// Gõ qua Serial Monitor để lái tay: f b l r s, + / - đổi tốc độ.
#include "robot_lib.h"  // Wokwi: gop san thu vien, xem wokwi/README.md

RobotMotors motors;
int speed = 150;

void demo() {
  Serial.println("Tien, tang toc 60 -> 255");
  for (int s = 60; s <= 255; s += 5) { motors.forward(s); delay(40); }
  motors.brake(); delay(500);
  Serial.println("Lui");     motors.backward(150);  delay(1000); motors.brake(); delay(500);
  Serial.println("Quay trai"); motors.turnLeft(150); delay(1000); motors.brake(); delay(500);
  Serial.println("Quay phai"); motors.turnRight(150); delay(1000); motors.stop();
  Serial.println("Xong. Neu xich nao quay nguoc, doi MOTOR_*_INVERT trong pins.h");
}

void setup() {
  Serial.begin(115200);
  motors.begin();
  delay(2000);
  demo();
}

void loop() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
    case 'f': motors.forward(speed); break;
    case 'b': motors.backward(speed); break;
    case 'l': motors.turnLeft(speed); break;
    case 'r': motors.turnRight(speed); break;
    case 's': motors.brake(); break;
    case '+': speed = min(255, speed + 20); break;
    case '-': speed = max(40, speed - 20); break;
    default: return;
  }
  Serial.printf("lenh %c, toc do %d\n", c, speed);
}
