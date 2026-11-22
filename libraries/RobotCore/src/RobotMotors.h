// TB6612FNG + 2 N20 (xích). Tốc độ -255..255. Dùng LEDC API của core 3.x (ledcAttach/ledcWrite).
#pragma once
#include <Arduino.h>
#include <pins.h>

class RobotMotors {
 public:
  void begin(uint32_t freqHz = 20000, uint8_t bits = 8);
  void drive(int left, int right);   // -255..255
  void stop();                       // thả trôi (coast)
  void brake();                      // phanh gấp (2 chân IN = HIGH)
  void forward(int s) { drive(s, s); }
  void backward(int s) { drive(-s, -s); }
  void turnLeft(int s) { drive(-s, s); }   // quay tại chỗ
  void turnRight(int s) { drive(s, -s); }
  int left() const { return _l; }
  int right() const { return _r; }
  float trimLeft = 1.0f, trimRight = 1.0f;  // bù chênh lệch 2 động cơ
 private:
  void side(int in1, int in2, int pwm, int speed, bool invert);
  int _l = 0, _r = 0;
};
