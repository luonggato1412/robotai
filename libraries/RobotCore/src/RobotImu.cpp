#include "RobotImu.h"

static void wr(TwoWire *w, uint8_t a, uint8_t reg, uint8_t val) {
  w->beginTransmission(a); w->write(reg); w->write(val); w->endTransmission();
}

bool RobotImu::begin(TwoWire &w, uint8_t addr) {
  _w = &w; _a = addr;
  _w->beginTransmission(_a);
  if (_w->endTransmission() != 0) return _ok = false;
  wr(_w, _a, 0x6B, 0x01);  // thức dậy, clock theo gyro X
  wr(_w, _a, 0x1A, 0x03);  // lọc DLPF ~44 Hz
  wr(_w, _a, 0x1B, 0x00);  // gyro ±250 °/s (131 LSB/°/s)
  wr(_w, _a, 0x1C, 0x00);  // accel ±2 g (16384 LSB/g)
  _lastUs = micros();
  return _ok = true;
}

bool RobotImu::read(int16_t *v) {  // ax ay az temp gx gy gz
  _w->beginTransmission(_a); _w->write(0x3B);
  if (_w->endTransmission(false) != 0) return false;
  if (_w->requestFrom(_a, (uint8_t)14) != 14) return false;
  for (int i = 0; i < 7; i++) { uint8_t hi = _w->read(), lo = _w->read(); v[i] = (int16_t)((hi << 8) | lo); }
  return true;
}

void RobotImu::calibrate(uint16_t samples) {
  if (!_ok) return;
  int16_t v[7]; float sum = 0; uint16_t n = 0;
  for (uint16_t i = 0; i < samples; i++) { if (read(v)) { sum += v[6]; n++; } delay(3); }
  if (n) _gzBias = sum / n / 131.0f;
  _yaw = 0; _lastUs = micros();
}

void RobotImu::update() {
  if (!_ok) return;
  int16_t v[7];
  if (!read(v)) return;
  uint32_t now = micros();
  float dt = (now - _lastUs) * 1e-6f; _lastUs = now;
  float ax = v[0] / 16384.0f, ay = v[1] / 16384.0f, az = v[2] / 16384.0f;
  _gz = v[6] / 131.0f - _gzBias;
  if (fabsf(_gz) > 0.5f) _yaw += _gz * dt;  // bỏ trôi nhỏ khi đứng yên
  _mag = sqrtf(ax * ax + ay * ay + az * az);
  _pitch = atan2f(ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;
  _roll = atan2f(ay, az) * RAD_TO_DEG;

  bool abnormal = fabsf(_pitch) > liftTiltDeg || fabsf(_roll) > liftTiltDeg ||
                  fabsf(_mag - 1.0f) > liftAccelDev;
  uint32_t ms = millis();
  if (abnormal) { _normalSince = 0; if (!_abnormalSince) _abnormalSince = ms; }
  else          { _abnormalSince = 0; if (!_normalSince) _normalSince = ms; }
  if (!_lifted && _abnormalSince && ms - _abnormalSince > 120) _lifted = true;   // bất thường >120 ms
  if (_lifted && _normalSince && ms - _normalSince > 1000) _lifted = false;      // đặt xuống yên 1 s
}
