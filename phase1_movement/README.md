# Phase 1 — Di chuyển

## Cài đặt một lần
1. Arduino IDE 2 → **File → Preferences → Sketchbook location** = thư mục repo này (`robotai`). Nhờ vậy IDE tự thấy 2 thư viện trong `libraries/` (`RobotConfig`, `RobotCore`).
2. Library Manager → cài **VL53L0X** (của Pololu).
3. Copy `libraries/RobotConfig/src/secrets.example.h` thành `secrets.h` cùng thư mục, điền Wi-Fi.
4. Board: **ESP32S3 Dev Module**, PSRAM = **OPI PSRAM**, Flash Size = **16MB**, USB CDC On Boot = **Enabled**.

## Các bước (nạp lần lượt, xong bước nào mới qua bước sau)
| Sketch | Kiểm tra |
|---|---|
| `p1_1_rgb_blink` | LED RGB onboard nháy đỏ / xanh lá / xanh dương |
| `p1_2_motors` | **Kê xe lên**. Tiến, lùi, quay trái, phải, tăng tốc. Lái tay qua Serial: `f b l r s + -` |
| `p1_3_tof_avoid` | Đi thẳng, vật cản < 15 cm thì dừng và quay |
| `p1_4_edge_safety` | Ra tới mép bàn → phanh, lùi, quay tránh |
| `p1_5_imu` | Gõ `g`: chạy thẳng 3 s có giữ hướng. Nhấc xe lên → dừng |
| `p1_6_battery` | In điện áp + % pin (so với đồng hồ vạn năng) |
| `p1_7_web_drive` | Lái bằng điện thoại, xem % pin |
| `phase1_robot` | **Bản ghép đủ**: web Tay/Tự động + né vật cản + chống rơi + giữ thẳng + chống nhấc + pin |

Web: mở Serial Monitor (115200) để xem IP rồi vào `http://<IP>` trên điện thoại. Không vào được Wi-Fi nhà thì robot phát Wi-Fi **XeTang-AI** (mật khẩu `xetang123`), vào `http://192.168.4.1`. Nút lái phải **giữ** mới chạy; thả tay hoặc mất sóng 0,4 s là xe dừng.

## Sơ đồ nối dây
| Linh kiện | Chân linh kiện | ESP32-S3 |
|---|---|---|
| TB6612FNG | AIN1 / AIN2 / PWMA (xích trái) | GPIO 4 / 5 / 6 |
| | BIN1 / BIN2 / PWMB (xích phải) | GPIO 7 / 15 / 16 |
| | STBY | **3V3** (nối cứng) |
| | VCC (logic) | 3V3 |
| | VM (động cơ) | Pin 2S + (qua công tắc) |
| | AO1/AO2, BO1/BO2 | Động cơ N20 trái, phải |
| | GND | GND chung |
| VL53L0X | SDA / SCL | GPIO 8 / 9 |
| MPU6050 | SDA / SCL (chung bus I2C) | GPIO 8 / 9 |
| | VCC / GND | 3V3 / GND |
| TCRT5000 trái / phải | DO | GPIO 41 / 42 |
| | VCC | 3V3 |
| Pin 2S | Pin+ → R1 100k → **GPIO1** → R2 33k → GND | GPIO 1 |
| LED RGB | có sẵn trên board | GPIO 48 (board v1.1: 38) |

Lưu ý:
- GND của pin, TB6612 và ESP32 phải nối chung.
- ESP32 lấy nguồn 5V từ mạch hạ áp (buck) từ pin 2S, **không** cấp 8,4 V thẳng vào board.
- Cầu chia 100k/33k cho 8,4 V → 2,08 V (an toàn cho ADC). Dùng điện trở khác thì sửa `BATT_DIVIDER_RATIO`.
- Xích quay ngược chiều → đổi `MOTOR_LEFT_INVERT` / `MOTOR_RIGHT_INVERT` trong `pins.h`.
- Module TCRT5000 báo ngược (đứng trên bàn mà xe vẫn lùi) → đổi `EDGE_ACTIVE_LEVEL` thành `LOW`.
- Giữ thẳng mà xe càng lúc càng lệch → đổi dấu `KP_HEADING` (MPU6050 lắp ngược).
- Chân tránh dùng: 0/3/45/46 (strapping), 19/20 (USB), 26–37 (flash + PSRAM của N16R8).
