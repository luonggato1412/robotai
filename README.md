# robot-ai — Chú xe tăng tí hon

Robot AI dùng ESP32-S3 N16R8: xích N20 (TB6612), mặt ST7789 1.54", trò chuyện bằng Gemini Live, nhiều cảm biến.

**Công cụ:** Arduino IDE 2 + Espressif ESP32 core 3.x.

## Lộ trình
| Phase | Thư mục | Nội dung |
|---|---|---|
| 0 | `phase0_setup` | Cài đặt, blink, kiểm tra board |
| 1 | `phase1_movement` | Động cơ N20 + TB6612 |
| 2 | `phase2_face` | Mặt ST7789 |
| 3 | `phase3_sensors` | VL53L0X, TCRT5000, MPU6050, BH1750, DS3231, LD2410C |
| 4 | `phase4_audio` | INMP441 + MAX98357A, microSD |
| 5 | `phase5_gemini_live` | Hội thoại Gemini Live |
| 6 | `phase6_behaviors` | Hành vi, WS2812, SG90 |
| 7 | `phase7_integration` | Ghép toàn bộ |

## Cài đặt Arduino IDE
Đặt **Sketchbook location** = thư mục repo này, để IDE tự thấy thư viện dùng chung trong `libraries/`:
- `libraries/RobotConfig` — bảng chân [`pins.h`](libraries/RobotConfig/src/pins.h) và bí mật.
- `libraries/RobotCore` — động cơ, IMU, pin, cảm biến mép, web điều khiển.

Chi tiết và sơ đồ nối dây: [`phase1_movement/README.md`](phase1_movement/README.md).

Chưa có phần cứng? Chạy thử Phase 1 trên trình duyệt bằng Wokwi: [`wokwi/README.md`](wokwi/README.md).

## Bí mật (Wi-Fi, Gemini key)
Copy `libraries/RobotConfig/src/secrets.example.h` thành `secrets.h` cùng thư mục rồi điền. File `secrets.h` không bao giờ được commit.

GitHub Actions tự biên dịch mọi sketch mỗi lần push.
