#include "RobotWeb.h"
#include <WiFi.h>
#include <WebServer.h>
#include <RobotSecrets.h>

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
