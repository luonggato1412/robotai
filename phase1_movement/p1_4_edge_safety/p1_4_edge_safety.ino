// Bước 4: 2 TCRT5000 dưới mũi. Mất mặt bàn -> phanh ngay, lùi, quay tránh mép. An toàn luôn thắng.
// Thử trên bàn thấp trước! Chỉnh biến trở trên module để đèn DO tắt/bật đúng khi ra khỏi mép.
#include <RobotMotors.h>
#include <RobotEdge.h>

const int SPEED = 120;
RobotMotors motors;

void setup() {
  Serial.begin(115200);
  motors.begin();
  RobotEdge::begin();
  delay(2000);
}

void loop() {
  bool l = RobotEdge::left(), r = RobotEdge::right();
  if (l || r) {
    motors.brake();                 // phanh trước, nghĩ sau
    Serial.printf("MEP! trai=%d phai=%d\n", l, r);
    delay(80);
    motors.backward(150); delay(350);
    if (l && !r) motors.turnRight(150); else motors.turnLeft(150);
    delay(300);
    motors.brake(); delay(200);
  } else {
    motors.forward(SPEED);
  }
}
