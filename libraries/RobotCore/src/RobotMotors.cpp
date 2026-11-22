#include "RobotMotors.h"

void RobotMotors::begin(uint32_t freqHz, uint8_t bits) {
  const int outs[] = {PIN_MOTOR_AIN1, PIN_MOTOR_AIN2, PIN_MOTOR_BIN1, PIN_MOTOR_BIN2};
  for (int p : outs) { pinMode(p, OUTPUT); digitalWrite(p, LOW); }
  ledcAttach(PIN_MOTOR_PWMA, freqHz, bits);
  ledcAttach(PIN_MOTOR_PWMB, freqHz, bits);
  if (PIN_MOTOR_STBY >= 0) { pinMode(PIN_MOTOR_STBY, OUTPUT); digitalWrite(PIN_MOTOR_STBY, HIGH); }
  stop();
}

void RobotMotors::side(int in1, int in2, int pwm, int speed, bool invert) {
  if (invert) speed = -speed;
  speed = constrain(speed, -255, 255);
  digitalWrite(in1, speed > 0 ? HIGH : LOW);
  digitalWrite(in2, speed < 0 ? HIGH : LOW);
  ledcWrite(pwm, abs(speed));
}

void RobotMotors::drive(int left, int right) {
  _l = constrain(left, -255, 255);
  _r = constrain(right, -255, 255);
  side(PIN_MOTOR_AIN1, PIN_MOTOR_AIN2, PIN_MOTOR_PWMA, (int)(_l * trimLeft), MOTOR_LEFT_INVERT);
  side(PIN_MOTOR_BIN1, PIN_MOTOR_BIN2, PIN_MOTOR_PWMB, (int)(_r * trimRight), MOTOR_RIGHT_INVERT);
}

void RobotMotors::stop() { drive(0, 0); }

void RobotMotors::brake() {
  _l = _r = 0;
  digitalWrite(PIN_MOTOR_AIN1, HIGH); digitalWrite(PIN_MOTOR_AIN2, HIGH);
  digitalWrite(PIN_MOTOR_BIN1, HIGH); digitalWrite(PIN_MOTOR_BIN2, HIGH);
  ledcWrite(PIN_MOTOR_PWMA, 255); ledcWrite(PIN_MOTOR_PWMB, 255);
}
