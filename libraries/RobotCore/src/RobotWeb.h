// Trang web điều khiển trên điện thoại: nút lái (giữ để chạy), chuyển Tay/Tự động, tốc độ, % pin.
#pragma once
#include <Arduino.h>

struct RobotWebState {
  // điện thoại -> robot
  char cmd = 's';            // f b l r s
  uint32_t lastCmdMs = 0;    // để dừng khi mất kết nối (deadman)
  bool autoMode = false;
  int speed = 180;           // 60..255
  // robot -> điện thoại
  float battV = 0;
  int battPct = 0;
  int distMm = -1;
  bool edgeL = false, edgeR = false, lifted = false;
  const char *status = "ok";
};

namespace RobotWeb {
// Vào Wi-Fi trong secrets.h; sau 10 s không được thì tự phát Wi-Fi AP_SSID. Trả về IP.
String begin(RobotWebState &st);
void handle();
// true nếu lệnh tay còn "tươi" (điện thoại gửi lặp mỗi 150 ms khi đang giữ nút).
inline bool cmdFresh(const RobotWebState &st, uint32_t timeoutMs = 400) {
  return millis() - st.lastCmdMs < timeoutMs;
}
}  // namespace RobotWeb
