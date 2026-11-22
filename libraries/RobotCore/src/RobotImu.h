// MPU6050 đọc thẳng qua Wire (không cần thư viện). Giữ hướng bằng gyro Z + phát hiện bị nhấc lên.
#pragma once
#include <Arduino.h>
#include <Wire.h>

class RobotImu {
 public:
  bool begin(TwoWire &w = Wire, uint8_t addr = 0x68);
  void calibrate(uint16_t samples = 300);  // để xe ĐỨNG YÊN khi gọi
  void update();                           // gọi liên tục trong loop()
  float yawDeg() const { return _yaw; }    // góc quay tích phân (độ)
  float gyroZ() const { return _gz; }      // độ/giây
  float pitchDeg() const { return _pitch; }
  float rollDeg() const { return _roll; }
  float accelG() const { return _mag; }
  bool lifted() const { return _lifted; }
  void resetYaw() { _yaw = 0; }
  bool ok() const { return _ok; }
  float liftTiltDeg = 35.0f;   // nghiêng quá góc này = bị nhấc / lật
  float liftAccelDev = 0.35f;  // lệch khỏi 1 g quá mức này (g)
 private:
  bool read(int16_t *v);
  TwoWire *_w = nullptr;
  uint8_t _a = 0x68;
  bool _ok = false, _lifted = false;
  float _gzBias = 0, _gz = 0, _yaw = 0, _pitch = 0, _roll = 0, _mag = 1;
  uint32_t _lastUs = 0, _abnormalSince = 0, _normalSince = 0;
};
