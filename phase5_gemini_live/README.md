# Phase 5 — Gemini Live

## Bước 5.0: `p5_0_gemini_text` — thử gọi Gemini bằng chữ
ESP32 gửi câu hỏi (gõ ở Serial Monitor) lên Gemini qua HTTPS và in câu trả lời. Chưa cần micro / loa.

### Lấy API key (miễn phí)
1. Vào https://aistudio.google.com/apikey, đăng nhập Google.
2. Bấm **Create API key** (chọn hoặc tạo project bất kỳ) → copy chuỗi bắt đầu bằng `AIza…`.
3. Key là bí mật: không dán vào file được commit, không chia sẻ dự án Wokwi công khai có key.

### Chạy trên Wokwi
1. Dự án ESP32-S3 mới trên wokwi.com, dán `wokwi/p5_0_gemini_text/diagram.json` và `p5_0_gemini_text.ino` (không cần `robot_lib.h`).
2. Thay `DAN-KEY-VAO-DAY` ở dòng `#define GEMINI_API_KEY` bằng key của bạn.
3. Bấm ▶, đợi `OK, IP …`, gõ câu hỏi vào Serial Monitor rồi Enter. Lần đầu mất vài giây (bắt tay TLS trong giả lập chậm).

### Chạy trên xe thật
Điền `WIFI_SSID`, `WIFI_PASSWORD`, `GEMINI_API_KEY` trong `libraries/RobotConfig/src/secrets.h` (đã bị .gitignore), nạp `phase5_gemini_live/p5_0_gemini_text`.

### Lỗi thường gặp
| Thấy | Nghĩa |
|---|---|
| `HTTP 400` / `403` | Key sai hoặc chưa dán |
| `HTTP 404` | Model đã đổi tên, sửa `GEMINI_MODEL` |
| `HTTP 429` / `503` | Model hết lượt hoặc quá tải (hay gặp giờ cao điểm). Sketch tự chuyển model khác trong `GEMINI_MODELS`, hết danh sách thì đợi rồi thử lại tối đa 3 vòng |
| `HTTP -1` / `-11` | Mất mạng hoặc quá thời gian chờ |

Ghi chú: bản thử dùng `setInsecure()` (không kiểm chứng chỉ máy chủ) cho đơn giản; bản Gemini Live sau sẽ gắn chứng chỉ gốc.
