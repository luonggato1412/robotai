// TU SINH bang tools/wokwi_pack.py - dung sua o day, sua trong libraries/ roi chay lai script.
// Bước 5.0: thử gọi Gemini (REST generateContent, văn bản) qua Wi-Fi.
// Gõ câu hỏi vào Serial Monitor (115200, kết thúc dòng = Newline) -> in câu trả lời.
// Bước đệm cho Gemini Live (giọng nói, WebSocket) ở các bước sau.
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
// Wokwi: Wi-Fi gia lap (khong mat khau). DAN KEY Gemini cua ban vao dong duoi (dung luu du an cong khai!)
#define WIFI_SSID      "Wokwi-GUEST"
#define WIFI_PASSWORD  ""
#define GEMINI_API_KEY "DAN-KEY-VAO-DAY"

// Đổi model ở đây nếu Google ngừng hỗ trợ (xem https://ai.google.dev/gemini-api/docs/models)
// Model chính quá tải (HTTP 503) thì tự hỏi lại bằng model dự phòng (bản Lite, ít bị nghẽn hơn).
#define GEMINI_MODEL          "gemini-flash-latest"
#define GEMINI_MODEL_FALLBACK "gemini-flash-lite-latest"
#define GEMINI_URL(m) "https://generativelanguage.googleapis.com/v1beta/models/" m ":generateContent"
static const char *PERSONA =
    "Bạn là Chú xe tăng tí hon, một robot nhỏ vui tính. Trả lời bằng tiếng Việt, ngắn gọn 1-3 câu.";

// Chuỗi -> JSON string (có dấu ngoặc kép)
String jsonEscape(const String &s) {
  String o = "\"";
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if (c == '\n') o += "\\n";
    else if (c == '\r') {}
    else if ((uint8_t)c < 0x20) o += ' ';
    else o += c;
  }
  return o + "\"";
}

void appendUtf8(String &o, uint32_t cp) {
  if (cp < 0x80) o += (char)cp;
  else if (cp < 0x800) { o += (char)(0xC0 | cp >> 6); o += (char)(0x80 | (cp & 0x3F)); }
  else { o += (char)(0xE0 | cp >> 12); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
}

// Gộp mọi trường "text": "..." trong phản hồi (bỏ qua phần suy nghĩ "thought": true nếu có).
String extractText(const String &body) {
  String out;
  int pos = 0;
  while ((pos = body.indexOf("\"text\"", pos)) >= 0) {
    int q = body.indexOf('"', body.indexOf(':', pos + 6));
    if (q < 0) break;
    String part;
    int i = q + 1;
    for (; i < (int)body.length() && body[i] != '"'; i++) {
      char c = body[i];
      if (c != '\\') { part += c; continue; }
      char e = body[++i];
      if (e == 'n') part += '\n';
      else if (e == 't') part += '\t';
      else if (e == 'u') { appendUtf8(part, strtoul(body.substring(i + 1, i + 5).c_str(), nullptr, 16)); i += 4; }
      else part += e;  // \" \\ \/
    }
    int close = body.indexOf('}', i);
    bool thought = body.substring(i, close).indexOf("\"thought\": true") >= 0;
    if (!thought) out += part;
    pos = i;
  }
  return out;
}

String askGemini(const String &question, const char *url) {
  WiFiClientSecure tls;
  tls.setInsecure();  // bản thử: không kiểm chứng chỉ (đơn giản, chạy được cả trên Wokwi)
  HTTPClient http;
  http.setTimeout(30000);
  if (!http.begin(tls, url)) return "[Lỗi] Không mở được kết nối";
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-goog-api-key", GEMINI_API_KEY);
  String req = String("{\"system_instruction\":{\"parts\":[{\"text\":") + jsonEscape(PERSONA) +
               "}]},\"contents\":[{\"role\":\"user\",\"parts\":[{\"text\":" + jsonEscape(question) + "}]}]}";
  uint32_t t0 = millis();
  int code = http.POST(req);
  String body = http.getString();
  http.end();
  Serial.printf("(HTTP %d, %lu ms)\n", code, (unsigned long)(millis() - t0));
  if (code == 503) return "";  // quá tải -> để loop() thử model dự phòng
  if (code != 200) {
    if (code == 400 || code == 403) Serial.println("Kiểm tra lại GEMINI_API_KEY.");
    if (code == 404) Serial.println("Model không còn, đổi GEMINI_MODEL.");
    if (code == 429) Serial.println("Hết hạn mức miễn phí, đợi một lát rồi thử lại.");
    return "[Lỗi] " + body.substring(0, 400);
  }
  String text = extractText(body);
  return text.length() ? text : "[Không có chữ] " + body.substring(0, 400);
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
  Serial.println("Gõ câu hỏi rồi Enter. Ví dụ: Xin chào, bạn là ai?");
}

void loop() {
  if (!Serial.available() || WiFi.status() != WL_CONNECTED) return;
  String q = Serial.readStringUntil('\n');
  q.trim();
  if (!q.length()) return;
  Serial.println("Bạn: " + q);
  String a = askGemini(q, GEMINI_URL(GEMINI_MODEL));
  if (!a.length()) {
    Serial.println("Model chính đang quá tải, thử " GEMINI_MODEL_FALLBACK "...");
    a = askGemini(q, GEMINI_URL(GEMINI_MODEL_FALLBACK));
    if (!a.length()) a = "[Lỗi] Gemini đang quá tải (HTTP 503), thử lại sau ít phút.";
  }
  Serial.println("Robot: " + a + "\n");
}
