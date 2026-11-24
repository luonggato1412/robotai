# Server riêng cho robot (bản thử)

```
ESP32 (robot)  <--Wi-Fi nhà, ws://-->  máy tính chạy robot_server.py  <--Internet-->  Gemini Live
```

Server giữ **API key**, **tính cách robot** (`persona.txt`, giọng nói), **trí nhớ** (`memory.json`) và **AI dự phòng** (Groq). Robot chỉ cần biết IP máy tính. Đổi tính cách, model, key… chỉ cần sửa server, không phải nạp lại ESP32.

Bản thử chuyển **chữ**: robot gửi câu hỏi, server trả lời từng đoạn (bản chép lời giọng nói của Gemini). Gemini muốn cử động thì gọi hàm `cu_dong`, server gửi lệnh xuống robot (robot mới in ra). Âm thanh (micro/loa) làm ở bước sau.

## Chạy trên Windows, từng bước

1. **Cài Python**: tải ở https://www.python.org/downloads/ , khi cài **tích ô "Add python.exe to PATH"**.
2. Mở thư mục `server` của repo trong File Explorer, bấm vào thanh địa chỉ, gõ `cmd` rồi Enter (mở cửa sổ lệnh ngay tại đây).
3. Cài thư viện (chỉ làm 1 lần):
   ```
   py -m pip install -r requirements.txt
   ```
4. Copy `.env.example` thành `.env` (lệnh: `copy .env.example .env`), mở `.env` bằng Notepad, điền:
   - `GEMINI_API_KEY=` key của bạn (https://aistudio.google.com/apikey)
   - (tuỳ chọn) `BACKUP_AI_KEY=` key Groq `gsk_…` (https://console.groq.com/keys)
   - `ROBOT_TOKEN=` một mật khẩu tự đặt (robot phải dùng đúng mật khẩu này)

   File `.env` đã bị `.gitignore`, đừng gửi nó cho ai.
5. Chạy server:
   ```
   py robot_server.py
   ```
   Thấy `Server robot đang chạy. ... SERVER_HOST "192.168.x.y"` là được. Lần đầu Windows hỏi tường lửa: chọn **Allow** (mạng Private).
6. **Thử không cần ESP32**: mở cửa sổ `cmd` thứ hai ở cùng thư mục, chạy `py thu_robot.py`, gõ `Xin chào` rồi Enter. Gõ `thoat` để thoát.

Dừng server: bấm `Ctrl+C`.

## Nối robot (ESP32) vào server
Sketch `phase5_gemini_live/p5_2_robot_server`.

- **Xe thật**: trong `libraries/RobotConfig/src/secrets.h` thêm (hoặc sửa):
  ```cpp
  #define SERVER_HOST "192.168.x.y"   // IP server in ra ở bước 5
  #define SERVER_PORT 8765
  #define ROBOT_TOKEN "xetang-bi-mat" // giống ROBOT_TOKEN trong .env
  ```
  ESP32 và máy tính phải chung Wi-Fi. Nạp sketch, mở Serial Monitor 115200 (Newline), đợi `Đã nối server`, gõ câu hỏi.
- **Wokwi (VS Code)**: dùng `wokwi/p5_2_robot_server/`, đã đặt sẵn `SERVER_HOST "host.wokwi.internal"` (nghĩa là "máy tính đang chạy Wokwi"). Chạy server trước, rồi biên dịch và **Wokwi: Start Simulator** như bước 5.1. Nếu `ROBOT_TOKEN` trong `.env` khác `xetang-bi-mat` thì sửa cho giống.

## Tuỳ chỉnh
| Muốn | Sửa |
|---|---|
| Đổi tính cách, cách nói | `persona.txt` (khởi động lại server) |
| Đổi giọng | `VOICE=` trong `.env` (Puck, Kore, Charon, Aoede…) |
| Tắt / bật cử động | `TOOLS=0` / `1` |
| Quên hết | xoá `memory.json`; `MEMORY_TURNS=0` để không nhớ |
| Đổi model | `LIVE_MODELS` / `BACKUP_MODELS` đầu file `robot_server.py` |

## Lỗi thường gặp
| Thấy | Làm gì |
|---|---|
| `'py' is not recognized` | Cài lại Python, nhớ tích **Add to PATH** (hoặc dùng `python` thay `py`) |
| `Thiếu thư viện websockets` | Chạy lại bước 3 |
| `Gemini từ chối model ...: 1007 API key not valid` | Sai `GEMINI_API_KEY` |
| `... 1011 ...` | Gemini hết lượt / quá tải. Có `BACKUP_AI_KEY` thì server tự hỏi Groq |
| Robot: `Không nối được` | Server chưa chạy, sai IP, khác Wi-Fi, hoặc tường lửa chặn (Windows Defender Firewall → Allow an app → tích Python ở Private) |
| Robot: `Server từ chối: 1008 sai token` | `ROBOT_TOKEN` hai bên chưa giống nhau |
| IP máy tính đổi sau khi khởi động lại router | Cập nhật `SERVER_HOST`, hoặc đặt IP tĩnh (DHCP reservation) cho máy tính trong router |

## Sau này
- Chuyển tiếp **âm thanh**: robot gửi micro 16 kHz, server gửi tiếng Gemini 24 kHz xuống loa (khung WebSocket nhị phân).
- Robot thực hiện `cu_dong` bằng `RobotMotors`, thêm hàm khác (đổi mặt, đọc cảm biến).
- Session resumption để giữ phiên Gemini qua lần nối lại.
- Đưa server lên máy chủ trên mạng (VPS) thì cần `wss://` (TLS) và token mạnh.
