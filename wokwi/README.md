# Chạy thử Phase 1 trên Wokwi (chưa cần phần cứng)

Wokwi giả lập ESP32-S3 ngay trên trình duyệt. Mỗi thư mục ở đây là một sketch của `phase1_movement`, đã gộp sẵn thư viện nên chỉ cần dán vào.

## Cách chạy trên wokwi.com (miễn phí)
1. Vào https://wokwi.com/projects/new/esp32-s3 (dự án mới, board ESP32-S3).
2. Tab **diagram.json**: xoá hết, dán nội dung `diagram.json` của thư mục sketch.
3. Bấm mũi tên ▾ cạnh tab → **New file…** → đặt tên `robot_lib.h` → dán nội dung `robot_lib.h`.
4. Tab **sketch.ino**: xoá hết, dán nội dung file `.ino` (ví dụ `p1_2_motors/p1_2_motors.ino`).
5. Bấm ▶. Gõ lệnh ở ô Serial Monitor phía dưới.

Mẹo: `diagram.json` và `robot_lib.h` giống nhau ở mọi bước, nên làm 1 dự án rồi chỉ thay `sketch.ino` khi đổi bước. Bấm **Save** để lưu dự án vào tài khoản Wokwi.

## Linh kiện thay thế (Wokwi chưa có)
| Thật | Trong giả lập | Cách thử |
|---|---|---|
| TB6612 + 2 N20 | 6 LED: xanh lá = tiến, đỏ = lùi, vàng = tốc độ (sáng mờ/rõ theo PWM), mỗi bên xích 1 bộ | Tiến: 2 LED xanh lá sáng. Quay trái: trái đỏ, phải xanh lá. Phanh: cả 3 LED sáng |
| VL53L0X | Biến trở trên **GPIO2** | Xoay trái dần = vật lại gần. Dưới ~25% (150 mm) xe dừng và quay. Hết phải = không thấy gì |
| TCRT5000 trái/phải | 2 công tắc gạt trên GPIO 41 / 42 | Gạt sang phải = mất mặt bàn (mép) |
| Pin 2S + cầu chia | Biến trở trên **GPIO1** | Mặc định ~2,08 V ở chân = 8,4 V pin. Xoay trái = pin yếu |
| LED RGB onboard | NeoPixel ngoài nối GPIO 48 | |
| MPU6050 | MPU6050 thật của Wokwi | Bấm vào MPU6050 khi đang chạy để chỉnh gia tốc / gyro. Gyro Z ≠ 0 = xe bị lệch hướng. Gia tốc Z ≠ 1 g hoặc nghiêng nhiều = bị nhấc |

## Thử từng bước
| Sketch | Thấy gì |
|---|---|
| `p1_1_rgb_blink` | NeoPixel đỏ → xanh lá → xanh dương |
| `p1_2_motors` | LED chạy demo; gõ `f b l r s + -` để lái |
| `p1_3_tof_avoid` | LED tiến; xoay biến trở GPIO2 về trái → phanh rồi quay phải |
| `p1_4_edge_safety` | Gạt 1 công tắc → phanh, lùi, quay ngược phía mép |
| `p1_5_imu` | In yaw/pitch/roll; gõ `g` và chỉnh gyro Z → LED tốc độ 2 bên lệch nhau để bù |
| `p1_6_battery` | Xoay biến trở GPIO1, xem V và % |
| `p1_7_web_drive` | Vào Wi-Fi `Wokwi-GUEST`, in IP. Lái bằng Serial: `f b l r s`, `a` (Tay/Tự động), `+ -` |
| `p5_0_gemini_text` | Hỏi Gemini qua Serial. Chỉ cần `diagram.json` + `.ino` của thư mục này, dán API key vào dòng `GEMINI_API_KEY`. Xem `phase5_gemini_live/README.md` |
| `phase1_robot` | Như trên, mỗi giây in trạng thái. Thử `a` (tự động) rồi xoay biến trở / gạt công tắc / chỉnh MPU6050 |

Lưu ý:
- Trang web lái xe trên điện thoại **không mở được** trên wokwi.com miễn phí (cần Wokwi IoT Gateway). Vì vậy bản giả lập cho lái bằng Serial. Nếu dùng **Wokwi cho VS Code**, file `wokwi.toml` đã có sẵn chuyển cổng: build bằng Arduino IDE (**Sketch → Export Compiled Binary**), mở thư mục sketch bằng VS Code, chạy Wokwi rồi vào `http://localhost:8180`.
- Giả lập kiểm tra **logic** (thứ tự ưu tiên, ngưỡng, đổi trạng thái), không kiểm tra được lực động cơ, độ nhạy cảm biến thật hay chiều quay xích. Phần đó vẫn phải thử khi hàng về.
- Phần chỉ chạy giả lập nằm trong `#ifdef ROBOT_SIM`, nạp lên xe thật bằng `phase1_movement` như cũ.

## Khi sửa code
Các file trong `wokwi/` là **tự sinh**. Sửa trong `phase1_movement/`, `phase5_gemini_live/` hoặc `libraries/`, rồi chạy:
```
python3 tools/wokwi_pack.py
```
CI sẽ báo lỗi nếu quên chạy, và biên dịch luôn các sketch Wokwi.
