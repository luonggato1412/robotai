# Phase 5 — Gemini Live

## Bước 5.0: `p5_0_gemini_text` — thử gọi Gemini bằng chữ
ESP32 gửi câu hỏi (gõ ở Serial Monitor) lên Gemini qua HTTPS và in câu trả lời. Chưa cần micro / loa.

### Lấy API key (miễn phí)
1. Vào https://aistudio.google.com/apikey, đăng nhập Google.
2. Bấm **Create API key** (chọn hoặc tạo project bất kỳ) → copy chuỗi bắt đầu bằng `AIza…`.
3. Key là bí mật: không dán vào file được commit, không chia sẻ dự án Wokwi công khai có key.

### (Tuỳ chọn) AI dự phòng khi Gemini quá tải
Gemini hay báo 503 giờ cao điểm. Sketch thử lần lượt các model Gemini, vẫn bận thì hỏi một nhà cung cấp khác theo chuẩn OpenAI (mặc định **Groq** `openai/gpt-oss-20b` và `gpt-oss-120b`, miễn phí; Llama trên Groq cần gói Enterprise).
1. Vào https://console.groq.com/keys, đăng nhập (Google được), bấm **Create API Key**, copy chuỗi `gsk_…`.
2. Dán vào `BACKUP_AI_KEY` (Wokwi: thay `DAN-KEY-GROQ-VAO-DAY`; xe thật: trong `secrets.h`). Để trống = tắt dự phòng.
3. Khi chạy sẽ thấy `AI dự phòng: BẬT`. Muốn dùng OpenAI/OpenRouter thì sửa `BACKUP_AI_URL` và tên model dự phòng trong `AI_MODELS`.

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
| `HTTP 429` / `503` | Model hết lượt hoặc quá tải (hay gặp giờ cao điểm). Sketch tự chuyển model khác trong `AI_MODELS` (Gemini và Groq xen kẽ), hết danh sách thì đợi rồi thử lại tối đa 3 vòng |
| `HTTP -1` / `-11` | Mất mạng hoặc quá thời gian chờ |

Ghi chú: bản thử dùng `setInsecure()` (không kiểm chứng chỉ máy chủ) cho đơn giản; bản Gemini Live sau sẽ gắn chứng chỉ gốc.

## Bước 5.1: `p5_1_gemini_live` — thử Gemini Live (WebSocket) bằng chữ
Gemini Live là kiểu nói chuyện "trực tiếp": giữ một kết nối WebSocket mở, gửi câu nói lên và nhận lời đáp chảy về từng đoạn. Robot thật sẽ gửi tiếng từ micro và phát tiếng ra loa (giai đoạn 4–5). Bước này chỉ thử phần kết nối: bạn gõ chữ, Gemini trả lời bằng **giọng nói**, sketch in ra **bản chép lời** (`outputTranscription`) và đếm số byte âm thanh nhận được (chưa phát).

- Model thử lần lượt trong `LIVE_MODELS`: `gemini-3.1-flash-live-preview` (Gemini 3.1 Flash Live), rồi `gemini-3.8-live`, cuối cùng `gemini-2.5-flash-native-audio-preview-12-2025` (bản cũ, rẻ hơn).
- Giữ kết nối cho cả cuộc trò chuyện nên Gemini nhớ câu trước. Máy chủ cắt kết nối sau ~10 phút, sketch tự nối lại ở câu hỏi sau (mất trí nhớ cuộc trò chuyện).
- `LIVE_TEXT_MODE 1` = xin trả chữ thẳng (TEXT) thay vì giọng nói; model Live mới có thể không nhận, khi đó để 0.

### Thử nhanh trên trình duyệt (không cần ESP32)
1. Vào https://aistudio.google.com/live (menu trái **Stream**/**Live**), đăng nhập.
2. Chọn model **Gemini 3.1 Flash Live** ở khung bên phải, bấm micro và nói tiếng Việt. Nghe được trả lời là key/tài khoản dùng được Live.

### Chạy trên Wokwi (VS Code)
1. Mở `wokwi/p5_1_gemini_live/p5_1_gemini_live.ino`, thay `DAN-KEY-VAO-DAY` bằng key (đừng commit file có key).
2. Biên dịch cho ESP32-S3 (giống bước 5.0) để có file trong `build/esp32.esp32.esp32s3/`, rồi bấm **Wokwi: Start Simulator** với `wokwi/p5_1_gemini_live/wokwi.toml`.
3. Đợi `Đã vào Gemini Live`, gõ câu hỏi rồi Enter. Lời đáp hiện dần sau `Robot:`.

### Lỗi thường gặp
| Thấy | Nghĩa |
|---|---|
| `Máy chủ từ chối: 1007 API key not valid` | Key sai / chưa dán |
| `1011 ...` | Hết hạn mức hoặc quá tải (gói miễn phí hay gặp với model preview). Đợi vài phút, xem **Rate limits** trong AI Studio |
| `1008 ... not found` / `not supported` | Tên model đã đổi, sửa `LIVE_MODELS` |
| `Bắt tay WebSocket lỗi` | Mạng chặn hoặc sai địa chỉ |

## Bước 5.2: `p5_2_robot_server` — nói chuyện qua server riêng
Robot không gọi thẳng Google nữa mà nối tới `server/robot_server.py` chạy trên máy tính (cùng Wi-Fi). Server giữ API key, tính cách, trí nhớ, gọi hàm cử động và AI dự phòng. Cách chạy từng bước: [server/README.md](../server/README.md).
