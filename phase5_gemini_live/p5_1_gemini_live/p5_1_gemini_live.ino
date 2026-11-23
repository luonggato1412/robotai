// Bước 5.1: thử Gemini Live (WebSocket BidiGenerateContent) bằng chữ, chưa cần micro / loa.
// Gõ câu hỏi vào Serial Monitor (115200, kết thúc dòng = Newline) -> robot in lời nói của Gemini
// (bản chép lời outputTranscription). Âm thanh trả về chỉ được đếm số byte, chưa phát (cần loa, bước 4-5).
// Giữ một kết nối mở cho cả cuộc trò chuyện, nên Gemini nhớ các câu trước. Rớt kết nối thì tự nối lại.
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "RobotSecrets.h"  // WIFI_SSID, WIFI_PASSWORD, GEMINI_API_KEY

// Tên model Live (xem https://ai.google.dev/gemini-api/docs/models). Thử lần lượt, model nào
// không có / không dùng được thì sang model sau.
static const char *LIVE_MODELS[] = {
    "gemini-3.1-flash-live-preview",  // "Gemini 3.1 Flash Live" trong AI Studio
    "gemini-3.8-live",                // bản Google khuyên dùng mới nhất
};
// 0 = AUDIO + chép lời (cách tài liệu hướng dẫn); 1 = xin trả chữ thẳng (TEXT), model nào không hỗ trợ sẽ báo lỗi.
#define LIVE_TEXT_MODE 0
static const char *PERSONA =
    "Bạn là Chú xe tăng tí hon, một robot nhỏ vui tính. Trả lời bằng tiếng Việt, ngắn gọn 1-3 câu.";

#define LIVE_HOST "generativelanguage.googleapis.com"
#define LIVE_PATH "/ws/google.ai.generativelanguage.v1beta.GenerativeService.BidiGenerateContent?key="
#define REPLY_TIMEOUT_MS 30000
#define MAX_MSG 6000  // giữ tối đa chừng này ký tự JSON mỗi tin (đã bỏ phần âm thanh base64)

WiFiClientSecure ws;
bool live = false;
int modelIdx = 0;

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

// Bộ lọc: giữ JSON nhưng bỏ ruột chuỗi "data": "<base64 âm thanh>" (có thể dài hàng chục KB).
struct MsgFilter {
  String msg;
  size_t audioBytes = 0;
  bool skipping = false;
  void reset() { msg = ""; skipping = false; }
  void feed(char c) {
    if (skipping) {
      if (c == '"') { skipping = false; msg += c; }
      else audioBytes++;
      return;
    }
    if (msg.length() < MAX_MSG) msg += c;
    if (c == '"' && (msg.endsWith("\"data\":\"") || msg.endsWith("\"data\": \""))) skipping = true;
  }
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

// ---------- Gemini Live ----------
// Giải thích mã đóng kết nối hay gặp (rx.msg = "<mã> <lý do>").
void closeHint(const String &m) {
  if (m.indexOf("API key") >= 0) Serial.println("=> Kiểm tra lại GEMINI_API_KEY.");
  else if (m.startsWith("1011")) Serial.println("=> Hết hạn mức / máy chủ quá tải (gói miễn phí hay gặp). Đợi vài phút, xem Rate limits trong AI Studio.");
  else if (m.indexOf("not found") >= 0 || m.indexOf("not supported") >= 0) Serial.println("=> Model không dùng được, thử model sau.");
  else if (m.startsWith("1007")) Serial.println("=> Máy chủ không hiểu tin gửi lên (tên trường/model đổi?).");
}

bool connectLive(const char *model) {
  ws.stop();
  ws.setInsecure();  // bản thử: không kiểm chứng chỉ (chạy được cả trên Wokwi)
  Serial.printf("Mở WebSocket tới %s (model %s)...\n", LIVE_HOST, model);
  if (!ws.connect(LIVE_HOST, 443)) { Serial.println("Không kết nối được máy chủ (mạng?)."); return false; }
  ws.print(String("GET ") + LIVE_PATH + GEMINI_API_KEY + " HTTP/1.1\r\nHost: " LIVE_HOST "\r\n"
           "Upgrade: websocket\r\nConnection: Upgrade\r\n"
           "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n");
  ws.setTimeout(10);  // giây, cho readStringUntil
  String status = ws.readStringUntil('\n');
  while (ws.connected()) {  // bỏ qua các dòng header
    String l = ws.readStringUntil('\n');
    if (l == "\r" || l.length() == 0) break;
  }
  if (status.indexOf(" 101") < 0) { Serial.println("Bắt tay WebSocket lỗi: " + status); ws.stop(); return false; }

  String setup = String("{\"setup\":{\"model\":\"models/") + model + "\",\"generationConfig\":{\"responseModalities\":[\"" +
                 (LIVE_TEXT_MODE ? "TEXT" : "AUDIO") + "\"]},\"systemInstruction\":{\"parts\":[{\"text\":" +
                 jsonEscape(PERSONA) + "}]}" + (LIVE_TEXT_MODE ? "" : ",\"outputAudioTranscription\":{}") + "}}";
  sendText(setup);
  uint32_t t0 = millis();
  while (millis() - t0 < 20000) {
    int r = readMessage();
    if (r == 1 && rx.msg.indexOf("setupComplete") >= 0) {
      Serial.printf("Đã vào Gemini Live (%lu ms).\n", (unsigned long)(millis() - t0));
      return true;
    }
    if (r == -1) {
      Serial.println("Máy chủ từ chối: " + rx.msg);
      closeHint(rx.msg);
      return false;
    }
    if (r == -2) break;
    delay(5);
  }
  Serial.println("Không nhận được setupComplete.");
  ws.stop();
  return false;
}

bool ensureLive() {
  if (live && ws.connected()) return true;
  live = false;
  int n = sizeof(LIVE_MODELS) / sizeof(LIVE_MODELS[0]);
  for (int i = 0; i < n && !live; i++) {
    int m = (modelIdx + i) % n;
    if (connectLive(LIVE_MODELS[m])) { live = true; modelIdx = m; }
  }
  return live;
}

void ask(const String &q) {
  if (!ensureLive()) { Serial.println("[Lỗi] Không vào được Gemini Live."); return; }
  rx.audioBytes = 0;
  if (!sendText("{\"realtimeInput\":{\"text\":" + jsonEscape(q) + "}}")) { live = false; Serial.println("[Lỗi] Gửi thất bại, thử lại."); return; }
  uint32_t t0 = millis(), first = 0;
  Serial.print("Robot: ");
  while (millis() - t0 < REPLY_TIMEOUT_MS) {
    int r = readMessage();
    if (r == 0) { delay(5); continue; }
    if (r < 0) {
      Serial.println(r == -1 ? "\n[Máy chủ đóng kết nối] " + rx.msg : String("\n[Mất kết nối]"));
      if (r == -1) closeHint(rx.msg);
      live = false;
      return;
    }
    const String &m = rx.msg;
    String piece;
    int ot = m.indexOf("\"outputTranscription\"");
    if (ot >= 0) piece = valueAfter(m, "\"text\"", ot);
    else if (LIVE_TEXT_MODE && m.indexOf("\"modelTurn\"") >= 0 && m.indexOf("\"thought\": true") < 0) piece = valueAfter(m, "\"text\"");
    if (piece.length()) { if (!first) first = millis() - t0; Serial.print(piece); }
    if (m.indexOf("\"goAway\"") >= 0) live = false;  // máy chủ sắp đóng phiên, lần sau nối lại
    if (m.indexOf("\"turnComplete\": true") >= 0 || m.indexOf("\"turnComplete\":true") >= 0) {
      Serial.printf("\n(%s: chữ đầu sau %lu ms, xong sau %lu ms, âm thanh ~%u byte)\n\n", LIVE_MODELS[modelIdx],
                    (unsigned long)first, (unsigned long)(millis() - t0), (unsigned)(rx.audioBytes * 3 / 4));
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
  if (String(GEMINI_API_KEY).startsWith("your-") || String(GEMINI_API_KEY).startsWith("DAN-")) {
    Serial.println("Chưa điền GEMINI_API_KEY! Lấy key miễn phí tại https://aistudio.google.com/apikey");
    return;
  }
  ensureLive();
  Serial.println("Gõ câu hỏi rồi Enter. Ví dụ: Xin chào, bạn là ai?");
}

void loop() {
  if (live && ws.available()) readMessage();  // đọc tin lặt vặt / ping giữa các câu hỏi
  if (!Serial.available() || WiFi.status() != WL_CONNECTED) return;
  String q = Serial.readStringUntil('\n');
  q.trim();
  if (!q.length()) return;
  Serial.println("Bạn: " + q);
  ask(q);
}
