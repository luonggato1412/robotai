// TU SINH bang tools/wokwi_pack.py - dung sua o day, sua trong libraries/ roi chay lai script.
#pragma once
#include <Arduino.h>
#include <Wire.h>

// ===== tools/wokwi/sim_prelude.h =====
// ---------- Chỉ cho giả lập Wokwi ----------
#define ROBOT_SIM 1
// Wi-Fi giả lập của Wokwi (không mật khẩu)
#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define AP_SSID       "XeTang-AI"
#define AP_PASSWORD   "xetang123"

// ===== libraries/RobotConfig/src/pins.h =====
// Bảng chân dùng chung cho mọi phase — ESP32-S3 N16R8 (DevKitC-1).
// Tránh: GPIO 0/3/45/46 (strapping), 19/20 (USB), 26–37 (flash + PSRAM octal), 43/44 chỉ dùng khi Serial qua USB-CDC.
// -1 = không nối (chân đó cắm cứng lên 3V3 / bỏ qua).

// ---------- Phase 1: di chuyển ----------
// LED RGB onboard (WS2812). DevKitC-1 v1.0 = 48, v1.1 = 38. Nếu không sáng, đổi sang 38.
#define PIN_RGB_LED 48

// TB6612FNG + 2 N20. A = xích trái, B = xích phải.
#define PIN_MOTOR_AIN1 4
#define PIN_MOTOR_AIN2 5
#define PIN_MOTOR_PWMA 6
#define PIN_MOTOR_BIN1 7
#define PIN_MOTOR_BIN2 15
#define PIN_MOTOR_PWMB 16
#define PIN_MOTOR_STBY -1   // nối STBY thẳng lên 3V3 để tiết kiệm chân
#define MOTOR_LEFT_INVERT  false  // đổi true nếu xích trái quay ngược
#define MOTOR_RIGHT_INVERT false

// I2C: VL53L0X, MPU6050 (sau này thêm BH1750, DS3231)
#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9

// TCRT5000 x2 (chân DO của module) dưới mũi xe
#define PIN_LINE_L 41
#define PIN_LINE_R 42
#define EDGE_ACTIVE_LEVEL HIGH   // module ra HIGH khi không thấy mặt bàn (= mép). Ngược thì đổi LOW.

// Pin 2S (8.4 V max) qua cầu chia R1 = 100k (lên pin+), R2 = 33k (xuống GND) -> GPIO1 (ADC1)
#define PIN_BATT_ADC 1
#define BATT_DIVIDER_RATIO ((100.0f + 33.0f) / 33.0f)
#define BATT_CELLS 2

// ---------- Các phase sau (đề xuất, chưa dùng) ----------
// ST7789 1.54" (SPI chung với microSD)
#define PIN_TFT_SCLK 12
#define PIN_TFT_MOSI 11
#define PIN_TFT_CS   10
#define PIN_TFT_DC   13
#define PIN_TFT_RST  -1   // nối RST màn hình vào EN
#define PIN_TFT_BL   21
// microSD (SPI chung)
#define PIN_SPI_MISO 2
#define PIN_SD_CS    18
// I2S: mic INMP441 và amp MAX98357A dùng chung BCLK/WS
#define PIN_MIC_SCK  39
#define PIN_MIC_WS   40
#define PIN_MIC_SD   38   // nếu board v1.1 (LED ở 38) thì phải đổi chân này
#define PIN_AMP_BCLK PIN_MIC_SCK
#define PIN_AMP_LRC  PIN_MIC_WS
#define PIN_AMP_DIN  47
// Khác
#define PIN_WS2812    14
#define PIN_SERVO     17
#define PIN_RADAR_OUT -1  // đọc LD2410C qua UART là đủ
#define PIN_RADAR_RX  44  // UART0 RX (cần USB CDC On Boot = Enabled)
#define PIN_RADAR_TX  43

// ===== tools/wokwi/sim_vl53l0x.h =====
// ---------- VL53L0X giả: Wokwi chưa có cảm biến này ----------
// Đọc biến trở trên GPIO2: xoay hết trái = 0 mm, ~25% = 150 mm (ngưỡng dừng), gần hết phải = 600 mm,
// hết phải = 8190 (ngoài tầm, giống cảm biến thật khi không thấy gì).
#define PIN_SIM_TOF_POT 2
class VL53L0X {
 public:
  enum regAddr : uint8_t { RESULT_INTERRUPT_STATUS = 0x13 };
  void setTimeout(uint16_t) {}
  bool init(bool = true) { pinMode(PIN_SIM_TOF_POT, INPUT); return true; }
  void startContinuous(uint32_t = 0) {}
  uint8_t readReg(uint8_t) { return 0x07; }  // luôn có kết quả mới
  bool timeoutOccurred() { return false; }
  uint16_t readRangeContinuousMillimeters() {
    int raw = analogRead(PIN_SIM_TOF_POT);
    return raw > 4000 ? 8190 : (uint16_t)(raw * 600L / 4000);
  }
};

// ===== libraries/RobotCore/src/RobotRgb.h =====
#include <Arduino.h>
// LED RGB onboard. Core 3.x: rgbLedWrite(pin, r, g, b).
inline void rgb(uint8_t r, uint8_t g, uint8_t b) { rgbLedWrite(PIN_RGB_LED, r, g, b); }

// ===== libraries/RobotCore/src/RobotMotors.h =====
// TB6612FNG + 2 N20 (xích). Tốc độ -255..255. Dùng LEDC API của core 3.x (ledcAttach/ledcWrite).
#include <Arduino.h>

class RobotMotors {
 public:
  void begin(uint32_t freqHz = 20000, uint8_t bits = 8);
  void drive(int left, int right);   // -255..255
  void stop();                       // thả trôi (coast)
  void brake();                      // phanh gấp (2 chân IN = HIGH)
  void forward(int s) { drive(s, s); }
  void backward(int s) { drive(-s, -s); }
  void turnLeft(int s) { drive(-s, s); }   // quay tại chỗ
  void turnRight(int s) { drive(s, -s); }
  int left() const { return _l; }
  int right() const { return _r; }
  float trimLeft = 1.0f, trimRight = 1.0f;  // bù chênh lệch 2 động cơ
 private:
  void side(int in1, int in2, int pwm, int speed, bool invert);
  int _l = 0, _r = 0;
};

// ===== libraries/RobotCore/src/RobotMotors.cpp =====

void RobotMotors::begin(uint32_t freqHz, uint8_t bits) {
  const int outs[] = {PIN_MOTOR_AIN1, PIN_MOTOR_AIN2, PIN_MOTOR_BIN1, PIN_MOTOR_BIN2};
  for (int p : outs) { pinMode(p, OUTPUT); digitalWrite(p, LOW); }
  ledcAttach(PIN_MOTOR_PWMA, freqHz, bits);
  ledcAttach(PIN_MOTOR_PWMB, freqHz, bits);
  if (PIN_MOTOR_STBY >= 0) { pinMode(PIN_MOTOR_STBY, OUTPUT); digitalWrite(PIN_MOTOR_STBY, HIGH); }
  stop();
}

void RobotMotors::side(int in1, int in2, int pwm, int speed, bool invert) {
  if (invert) speed = -speed;
  speed = constrain(speed, -255, 255);
  digitalWrite(in1, speed > 0 ? HIGH : LOW);
  digitalWrite(in2, speed < 0 ? HIGH : LOW);
  ledcWrite(pwm, abs(speed));
}

void RobotMotors::drive(int left, int right) {
  _l = constrain(left, -255, 255);
  _r = constrain(right, -255, 255);
  side(PIN_MOTOR_AIN1, PIN_MOTOR_AIN2, PIN_MOTOR_PWMA, (int)(_l * trimLeft), MOTOR_LEFT_INVERT);
  side(PIN_MOTOR_BIN1, PIN_MOTOR_BIN2, PIN_MOTOR_PWMB, (int)(_r * trimRight), MOTOR_RIGHT_INVERT);
}

void RobotMotors::stop() { drive(0, 0); }

void RobotMotors::brake() {
  _l = _r = 0;
  digitalWrite(PIN_MOTOR_AIN1, HIGH); digitalWrite(PIN_MOTOR_AIN2, HIGH);
  digitalWrite(PIN_MOTOR_BIN1, HIGH); digitalWrite(PIN_MOTOR_BIN2, HIGH);
  ledcWrite(PIN_MOTOR_PWMA, 255); ledcWrite(PIN_MOTOR_PWMB, 255);
}

// ===== libraries/RobotCore/src/RobotEdge.h =====
// 2 cảm biến TCRT5000 dưới mũi: true = mất mặt bàn (sắp rơi).
#include <Arduino.h>

namespace RobotEdge {
inline void begin() { pinMode(PIN_LINE_L, INPUT); pinMode(PIN_LINE_R, INPUT); }
inline bool left() { return digitalRead(PIN_LINE_L) == EDGE_ACTIVE_LEVEL; }
inline bool right() { return digitalRead(PIN_LINE_R) == EDGE_ACTIVE_LEVEL; }
}  // namespace RobotEdge

// ===== libraries/RobotCore/src/RobotBattery.h =====
// Đo pin 2S qua cầu chia trên GPIO1. analogReadMilliVolts() đã hiệu chỉnh theo eFuse.
#include <Arduino.h>

namespace RobotBattery {
inline float volts() {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(PIN_BATT_ADC);
  return mv / 16.0f / 1000.0f * BATT_DIVIDER_RATIO;
}
// Đường xả Li-ion gần đúng, tính theo 1 cell.
inline int percent(float packV) {
  static const float v[] = {3.30f, 3.50f, 3.60f, 3.70f, 3.80f, 3.90f, 4.00f, 4.10f, 4.20f};
  static const int p[] = {0, 10, 20, 35, 50, 62, 75, 88, 100};
  float c = packV / BATT_CELLS;
  if (c <= v[0]) return 0;
  for (int i = 1; i < 9; i++)
    if (c < v[i]) return p[i - 1] + (int)((c - v[i - 1]) / (v[i] - v[i - 1]) * (p[i] - p[i - 1]));
  return 100;
}
}  // namespace RobotBattery

// ===== libraries/RobotCore/src/RobotImu.h =====
// MPU6050 đọc thẳng qua Wire (không cần thư viện). Giữ hướng bằng gyro Z + phát hiện bị nhấc lên.
#include <Arduino.h>
#include <Wire.h>

class RobotImu {
 public:
  bool begin(TwoWire &w = Wire, uint8_t addr = 0x68);
  void calibrate(uint16_t samples = 300);  // để xe ĐỨNG YÊN khi gọi
  void update();                           // gọi liên tục trong loop()
  float yawDeg() const { return _yaw; }    // góc quay tích phân (độ)
  float gyroZ() const { return _gz; }      // độ/giây
  float pitchDeg() const { return _pitch; }
  float rollDeg() const { return _roll; }
  float accelG() const { return _mag; }
  bool lifted() const { return _lifted; }
  void resetYaw() { _yaw = 0; }
  bool ok() const { return _ok; }
  float liftTiltDeg = 35.0f;   // nghiêng quá góc này = bị nhấc / lật
  float liftAccelDev = 0.35f;  // lệch khỏi 1 g quá mức này (g)
 private:
  bool read(int16_t *v);
  TwoWire *_w = nullptr;
  uint8_t _a = 0x68;
  bool _ok = false, _lifted = false;
  float _gzBias = 0, _gz = 0, _yaw = 0, _pitch = 0, _roll = 0, _mag = 1;
  uint32_t _lastUs = 0, _abnormalSince = 0, _normalSince = 0;
};

// ===== libraries/RobotCore/src/RobotImu.cpp =====

static void wr(TwoWire *w, uint8_t a, uint8_t reg, uint8_t val) {
  w->beginTransmission(a); w->write(reg); w->write(val); w->endTransmission();
}

bool RobotImu::begin(TwoWire &w, uint8_t addr) {
  _w = &w; _a = addr;
  _w->beginTransmission(_a);
  if (_w->endTransmission() != 0) return _ok = false;
  wr(_w, _a, 0x6B, 0x01);  // thức dậy, clock theo gyro X
  wr(_w, _a, 0x1A, 0x03);  // lọc DLPF ~44 Hz
  wr(_w, _a, 0x1B, 0x00);  // gyro ±250 °/s (131 LSB/°/s)
  wr(_w, _a, 0x1C, 0x00);  // accel ±2 g (16384 LSB/g)
  _lastUs = micros();
  return _ok = true;
}

bool RobotImu::read(int16_t *v) {  // ax ay az temp gx gy gz
  _w->beginTransmission(_a); _w->write(0x3B);
  if (_w->endTransmission(false) != 0) return false;
  if (_w->requestFrom(_a, (uint8_t)14) != 14) return false;
  for (int i = 0; i < 7; i++) { uint8_t hi = _w->read(), lo = _w->read(); v[i] = (int16_t)((hi << 8) | lo); }
  return true;
}

void RobotImu::calibrate(uint16_t samples) {
  if (!_ok) return;
  int16_t v[7]; float sum = 0; uint16_t n = 0;
  for (uint16_t i = 0; i < samples; i++) { if (read(v)) { sum += v[6]; n++; } delay(3); }
  if (n) _gzBias = sum / n / 131.0f;
  _yaw = 0; _lastUs = micros();
}

void RobotImu::update() {
  if (!_ok) return;
  int16_t v[7];
  if (!read(v)) return;
  uint32_t now = micros();
  float dt = (now - _lastUs) * 1e-6f; _lastUs = now;
  float ax = v[0] / 16384.0f, ay = v[1] / 16384.0f, az = v[2] / 16384.0f;
  _gz = v[6] / 131.0f - _gzBias;
  if (fabsf(_gz) > 0.5f) _yaw += _gz * dt;  // bỏ trôi nhỏ khi đứng yên
  _mag = sqrtf(ax * ax + ay * ay + az * az);
  _pitch = atan2f(ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;
  _roll = atan2f(ay, az) * RAD_TO_DEG;

  bool abnormal = fabsf(_pitch) > liftTiltDeg || fabsf(_roll) > liftTiltDeg ||
                  fabsf(_mag - 1.0f) > liftAccelDev;
  uint32_t ms = millis();
  if (abnormal) { _normalSince = 0; if (!_abnormalSince) _abnormalSince = ms; }
  else          { _abnormalSince = 0; if (!_normalSince) _normalSince = ms; }
  if (!_lifted && _abnormalSince && ms - _abnormalSince > 120) _lifted = true;   // bất thường >120 ms
  if (_lifted && _normalSince && ms - _normalSince > 1000) _lifted = false;      // đặt xuống yên 1 s
}

// ===== libraries/RobotCore/src/RobotWeb.h =====
// Trang web điều khiển trên điện thoại: nút lái (giữ để chạy), chuyển Tay/Tự động, tốc độ, % pin.
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

// ===== libraries/RobotCore/src/RobotWeb.cpp =====
#include <WiFi.h>
#include <WebServer.h>

static WebServer server(80);
static RobotWebState *S = nullptr;

static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Xe tăng tí hon</title><style>
body{font-family:sans-serif;background:#111;color:#eee;text-align:center;margin:0;padding:12px;user-select:none;-webkit-user-select:none}
.g{display:grid;grid-template-columns:repeat(3,90px);gap:10px;justify-content:center;margin:16px 0}
button{height:90px;font-size:32px;border:0;border-radius:16px;background:#333;color:#fff;touch-action:none}
button:active,.on{background:#2a7}#m{height:50px;width:280px;font-size:20px}
.bar{height:14px;background:#333;border-radius:7px;overflow:hidden;width:280px;margin:6px auto}
#bf{height:100%;background:#2a7}small{color:#999}
</style></head><body>
<h3>🛡️ Xe tăng tí hon</h3>
<div>Pin: <b id="bv">-</b> V · <b id="bp">-</b>%</div><div class="bar"><div id="bf" style="width:0"></div></div>
<button id="m" onclick="tm()">Chế độ: TAY</button>
<div class="g"><span></span><button data-c="f">▲</button><span></span>
<button data-c="l">◀</button><button data-c="s">■</button><button data-c="r">▶</button>
<span></span><button data-c="b">▼</button><span></span></div>
<div>Tốc độ <input id="sp" type="range" min="60" max="255" value="180" onchange="fetch('/speed?v='+this.value)"></div>
<p><small id="st">...</small></p>
<script>
let c='s',t=null,auto=false;
function send(){fetch('/cmd?d='+c).catch(()=>{})}
function press(x){c=x;send();clearInterval(t);if(x!='s')t=setInterval(send,150)}
function rel(){clearInterval(t);c='s';send()}
document.querySelectorAll('[data-c]').forEach(b=>{
 b.addEventListener('pointerdown',e=>{e.preventDefault();b.setPointerCapture(e.pointerId);press(b.dataset.c)});
 ['pointerup','pointercancel'].forEach(ev=>b.addEventListener(ev,rel));});
function tm(){auto=!auto;fetch('/mode?m='+(auto?'auto':'manual'));show()}
function show(){const m=document.getElementById('m');m.textContent='Chế độ: '+(auto?'TỰ ĐỘNG':'TAY');m.classList.toggle('on',auto)}
setInterval(()=>fetch('/status').then(r=>r.json()).then(j=>{
 bv.textContent=j.v.toFixed(2);bp.textContent=j.p;bf.style.width=j.p+'%';
 bf.style.background=j.p<20?'#c33':'#2a7';auto=j.auto;show();
 st.textContent='Vật cản: '+(j.d<0?'-':(j.d/10).toFixed(0)+' cm')+' · Mép: '+(j.el?'T':'-')+(j.er?'P':'-')+(j.lift?' · BỊ NHẤC':'')+' · '+j.s;
}).catch(()=>{}),700);
</script></body></html>)HTML";

static void onCmd() {
  String d = server.arg("d");
  if (d.length() == 1 && strchr("fblrs", d[0])) { S->cmd = d[0]; S->lastCmdMs = millis(); }
  server.send(204);
}
static void onMode() { S->autoMode = server.arg("m") == "auto"; S->cmd = 's'; server.send(204); }
static void onSpeed() { S->speed = constrain(server.arg("v").toInt(), 60, 255); server.send(204); }
static void onStatus() {
  char buf[200];
  snprintf(buf, sizeof buf,
           "{\"v\":%.2f,\"p\":%d,\"d\":%d,\"el\":%d,\"er\":%d,\"lift\":%d,\"auto\":%d,\"s\":\"%s\"}",
           S->battV, S->battPct, S->distMm, S->edgeL, S->edgeR, S->lifted, S->autoMode, S->status);
  server.send(200, "application/json", buf);
}

String RobotWeb::begin(RobotWebState &st) {
  S = &st;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // giảm trễ khi bấm nút
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(200);
  String ip;
  if (WiFi.status() == WL_CONNECTED) {
    ip = WiFi.localIP().toString();
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    ip = WiFi.softAPIP().toString();
  }
  server.on("/", [] { server.send_P(200, "text/html", PAGE); });
  server.on("/cmd", onCmd);
  server.on("/mode", onMode);
  server.on("/speed", onSpeed);
  server.on("/status", onStatus);
  server.begin();
  return ip;
}

#ifdef ROBOT_SIM
// Chỉ dùng khi chạy giả lập Wokwi (wokwi/): lái bằng Serial Monitor vì Wokwi miễn phí không mở được trang web.
// f b l r s = lái, a = đổi Tay/Tự động, + - = tốc độ. Mỗi giây in trạng thái.
static char simCmd = 's';
static void simSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c && strchr("fblrs", c)) simCmd = c;
    else if (c == 'a') { S->autoMode = !S->autoMode; simCmd = 's'; }
    else if (c == '+') S->speed = min(255, S->speed + 20);
    else if (c == '-') S->speed = max(60, S->speed - 20);
    else continue;
    Serial.printf("lenh %c | %s | toc do %d\n", c, S->autoMode ? "TU DONG" : "TAY", S->speed);
  }
  if (simCmd != 's') { S->cmd = simCmd; S->lastCmdMs = millis(); }
  static uint32_t last = 0;
  if (millis() - last > 1000) {
    last = millis();
    Serial.printf("[%s] vat can %d mm | mep %c%c | %s| pin %.2f V %d%%\n", S->status, S->distMm,
                  S->edgeL ? 'T' : '-', S->edgeR ? 'P' : '-', S->lifted ? "BI NHAC " : "", S->battV, S->battPct);
  }
}
#endif
void RobotWeb::handle() {
  server.handleClient();
#ifdef ROBOT_SIM
  simSerial();
#endif
}
