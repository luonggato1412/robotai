// Bước 5.2: robot nói chuyện qua SERVER RIÊNG (server/robot_server.py chạy trên máy tính) thay vì gọi
// thẳng Google. Server giữ API key, tính cách robot, trí nhớ, AI dự phòng; robot chỉ cần biết địa chỉ server.
// Gõ câu hỏi vào Serial Monitor (115200, Newline) -> server hỏi Gemini Live -> robot in lời đáp từng đoạn.
// Gemini muốn cử động (hàm cu_dong) -> robot in lệnh ra (chưa chạy động cơ). Rớt kết nối thì tự nối lại.
#include <WiFi.h>
#include "RobotSecrets.h"  // WIFI_SSID, WIFI_PASSWORD, SERVER_HOST, SERVER_PORT, ROBOT_TOKEN
#ifndef SERVER_HOST
#define SERVER_HOST "192.168.1.100"  // IP máy tính chạy server (server in ra khi khởi động)
#endif
#ifndef SERVER_PORT
#define SERVER_PORT 8765
#endif
#ifndef ROBOT_TOKEN
#define ROBOT_TOKEN "xetang-bi-mat"  // phải giống ROBOT_TOKEN trong server/.env
#endif

#define REPLY_TIMEOUT_MS 40000
#define MAX_MSG 6000

WiFiClient ws;  // ws:// thường (mạng nhà), không cần TLS nên nhẹ và nhanh hơn
bool online = false;

// ---------- JSON nhỏ gọn ----------
String jsonEscape(const String &s) {
  String o = "\"";
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if (c == '\n') o += "\\n";
    else if ((uint8_t)c < 0x20) {}
    else o += c;
  }
  return o + "\"";
}

void appendUtf8(String &o, uint32_t cp) {
  if (cp < 0x80) o += (char)cp;
  else if (cp < 0x800) { o += (char)(0xC0 | cp >> 6); o += (char)(0x80 | (cp & 0x3F)); }
  else { o += (char)(0xE0 | cp >> 12); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
}

// Đọc chuỗi JSON bắt đầu ở vị trí dấu " (q). Trả nội dung đã bỏ escape.
String jsonString(const String &b, int q) {
  String part;
  for (int i = q + 1; i < (int)b.length() && b[i] != '"'; i++) {
    char c = b[i];
    if (c != '\\') { part += c; continue; }
    char e = b[++i];
    if (e == 'n') part += '\n';
    else if (e == 't') part += '\t';
    else if (e == 'u') { appendUtf8(part, strtoul(b.substring(i + 1, i + 5).c_str(), nullptr, 16)); i += 4; }
    else part += e;
  }
  return part;
}

// Giá trị chuỗi của khoá key, tìm từ vị trí from. "" nếu không có.
String valueAfter(const String &b, const char *key, int from = 0) {
  int k = b.indexOf(key, from);
  if (k < 0) return "";
  int q = b.indexOf('"', b.indexOf(':', k + strlen(key)));
  return q < 0 ? "" : jsonString(b, q);
}

// ---------- WebSocket tối giản (RFC 6455) ----------
bool readExact(uint8_t *buf, size_t n, uint32_t timeoutMs = 10000) {
  size_t got = 0;
  uint32_t t0 = millis();
  while (got < n) {
    int a = ws.available();
    if (a > 0) {
      int r = ws.read(buf + got, min((size_t)a, n - got));
      if (r > 0) { got += r; t0 = millis(); continue; }
    }
    if (!ws.connected() || millis() - t0 > timeoutMs) return false;
    delay(1);
  }
  return true;
}

// Gửi 1 khung (máy khách bắt buộc che "mask" dữ liệu).
bool sendFrame(uint8_t opcode, const uint8_t *data, size_t len) {
  uint8_t h[14];
  size_t n = 0;
  h[n++] = 0x80 | opcode;
  if (len < 126) h[n++] = 0x80 | len;
  else if (len < 65536) { h[n++] = 0x80 | 126; h[n++] = len >> 8; h[n++] = len; }
  else { h[n++] = 0x80 | 127; for (int i = 7; i >= 0; i--) h[n++] = i < 4 ? (uint8_t)(len >> (8 * i)) : 0; }
  uint32_t m = esp_random();
  uint8_t *mask = h + n;
  memcpy(mask, &m, 4);
  n += 4;
  if (ws.write(h, n) != n) return false;
  uint8_t chunk[256];
  for (size_t off = 0; off < len; off += sizeof(chunk)) {
    size_t c = min(sizeof(chunk), len - off);
    for (size_t i = 0; i < c; i++) chunk[i] = data[off + i] ^ mask[(off + i) & 3];
    if (ws.write(chunk, c) != c) return false;
  }
  return true;
}
bool sendText(const String &s) { return sendFrame(1, (const uint8_t *)s.c_str(), s.length()); }

// Tin nhắn đang đọc (cắt bớt nếu quá dài).
struct MsgBuf {
  String msg;
  void reset() { msg = ""; }
  void feed(char c) { if (msg.length() < MAX_MSG) msg += c; }
} rx;

// Đọc 1 tin nhắn (ghép các khung nối tiếp). Trả: 1 = có tin (rx.msg), 0 = chưa có gì,
// -1 = máy chủ đóng kết nối (rx.msg = mã + lý do), -2 = mất kết nối / quá giờ.
int readMessage() {
  if (!ws.available()) return ws.connected() ? 0 : -2;
  rx.reset();
  while (true) {
    uint8_t h[2];
    if (!readExact(h, 2)) return -2;
    uint8_t op = h[0] & 0x0F;
    bool fin = h[0] & 0x80;
    uint64_t len = h[1] & 0x7F;
    uint8_t ext[8];
    if (len == 126) { if (!readExact(ext, 2)) return -2; len = (ext[0] << 8) | ext[1]; }
    else if (len == 127) { if (!readExact(ext, 8)) return -2; len = 0; for (int i = 0; i < 8; i++) len = (len << 8) | ext[i]; }
    if (h[1] & 0x80) { uint8_t mk[4]; if (!readExact(mk, 4)) return -2; }  // máy chủ không mask, phòng hờ
    if (op >= 8) {  // khung điều khiển: đóng / ping / pong (<= 125 byte)
      uint8_t c[126];
      if (len > 125 || !readExact(c, len)) return -2;
      c[len] = 0;
      if (op == 9) sendFrame(10, c, len);  // ping -> pong
      if (op == 8) {
        int code = len >= 2 ? (c[0] << 8 | c[1]) : 0;
        rx.msg = String(code) + " " + (len > 2 ? String((const char *)c + 2) : String(""));
        ws.stop();
        return -1;
      }
      continue;
    }
    uint8_t buf[512];
    while (len) {
      size_t c = min((uint64_t)sizeof(buf), len);
      if (!readExact(buf, c)) return -2;
      for (size_t i = 0; i < c; i++) rx.feed((char)buf[i]);
      len -= c;
    }
    if (fin) return 1;
  }
}

// ---------- Nói chuyện với server ----------
// Giá trị số của khoá key ("ms":1234). -1 nếu không có.
long numberAfter(const String &b, const char *key) {
  int k = b.indexOf(key);
  return k < 0 ? -1 : b.substring(b.indexOf(':', k) + 1).toInt();
}
bool isType(const String &m, const char *t) { return m.startsWith(String("{\"type\":\"") + t + "\""); }

bool connectServer() {
  ws.stop();
  Serial.printf("Nối server %s:%d...\n", SERVER_HOST, SERVER_PORT);
  if (!ws.connect(SERVER_HOST, SERVER_PORT, 5000)) {
    Serial.println("Không nối được. Server đã chạy chưa? SERVER_HOST đúng IP máy tính chưa? Tường lửa Windows?");
    return false;
  }
  ws.print(String("GET /robot?token=") + ROBOT_TOKEN + " HTTP/1.1\r\nHost: " + SERVER_HOST + "\r\n"
           "Upgrade: websocket\r\nConnection: Upgrade\r\n"
           "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n");
  ws.setTimeout(10);
  String status = ws.readStringUntil('\n');
  while (ws.connected()) {
    String l = ws.readStringUntil('\n');
    if (l == "\r" || l.length() == 0) break;
  }
  if (status.indexOf(" 101") < 0) { Serial.println("Bắt tay WebSocket lỗi: " + status); ws.stop(); return false; }
  uint32_t t0 = millis();
  while (millis() - t0 < 60000) {  // server thử lần lượt các model Gemini, có thể mất vài giây
    int r = readMessage();
    if (r == 1 && isType(rx.msg, "ready")) {
      Serial.println("Đã nối server, AI: " + valueAfter(rx.msg, "\"ai\""));
      return true;
    }
    if (r == -1) { Serial.println("Server từ chối: " + rx.msg + " (ROBOT_TOKEN giống server/.env chưa?)"); return false; }
    if (r == -2) break;
    delay(5);
  }
  Serial.println("Server không trả lời.");
  ws.stop();
  return false;
}

bool ensureServer() {
  if (online && ws.connected()) return true;
  online = connectServer();
  return online;
}

void doAction(const String &m) {
  // Sau này: gọi RobotMotors (phase 1) ở đây. Bây giờ chỉ in ra.
  Serial.printf(" [cử động: %s %ld ms] ", valueAfter(m, "\"hanh_dong\"").c_str(), numberAfter(m, "\"ms\""));
}

void ask(const String &q) {
  if (!ensureServer()) return;
  if (!sendText("{\"type\":\"text\",\"text\":" + jsonEscape(q) + "}")) { online = false; Serial.println("[Lỗi] Gửi thất bại, thử lại."); return; }
  uint32_t t0 = millis();
  Serial.print("Robot: ");
  while (millis() - t0 < REPLY_TIMEOUT_MS) {
    int r = readMessage();
    if (r == 0) { delay(5); continue; }
    if (r < 0) { Serial.println(r == -1 ? "\n[Server đóng kết nối] " + rx.msg : String("\n[Mất kết nối server]")); online = false; return; }
    const String &m = rx.msg;
    if (isType(m, "text")) Serial.print(valueAfter(m, "\"text\""));
    else if (isType(m, "action")) doAction(m);
    else if (isType(m, "error")) { Serial.println("\n[Lỗi] " + valueAfter(m, "\"text\"") + "\n"); return; }
    else if (isType(m, "done")) {
      Serial.printf("\n(%s, %ld ms, âm thanh %ld byte)\n\n", valueAfter(m, "\"ai\"").c_str(), numberAfter(m, "\"ms\""), numberAfter(m, "\"audio\""));
      return;
    }
  }
  Serial.println("\n[Hết giờ chờ trả lời]");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("\nKết nối Wi-Fi \"%s\"", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (int i = 0; WiFi.status() != WL_CONNECTED && i < 60; i++) { delay(500); Serial.print('.'); }
  if (WiFi.status() != WL_CONNECTED) { Serial.println("\nKhông vào được Wi-Fi, kiểm tra WIFI_SSID / WIFI_PASSWORD."); return; }
  Serial.printf("\nOK, IP %s\n", WiFi.localIP().toString().c_str());
  ensureServer();
  Serial.println("Gõ câu hỏi rồi Enter. Ví dụ: Xin chào, bạn là ai?  hoặc  Đi tới một chút nhé");
}

void loop() {
  if (online && ws.available()) readMessage();  // trả lời ping của server giữa các câu hỏi
  if (!Serial.available() || WiFi.status() != WL_CONNECTED) return;
  String q = Serial.readStringUntil('\n');
  q.trim();
  if (!q.length()) return;
  Serial.println("Bạn: " + q);
  ask(q);
}
