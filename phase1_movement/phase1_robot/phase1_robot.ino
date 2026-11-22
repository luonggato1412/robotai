// Phase 1 tổng hợp: lái bằng điện thoại (Tay / Tự động) + né vật cản + chống rơi mép + giữ thẳng + chống nhấc + % pin.
// Thứ tự ưu tiên mỗi vòng loop (không dùng delay):
//   1. Bị nhấc lên        -> dừng hẳn
//   2. Mất mặt bàn (mép)  -> phanh, lùi, quay tránh   (an toàn luôn thắng, kể cả đang lái tay)
//   3. Vật cản < 15 cm    -> Tay: chặn đi tới. Tự động: dừng và quay tới khi thoáng
//   4. Lệnh lái / tự động đi thẳng (giữ hướng bằng gyro)
#include <Wire.h>
#include <VL53L0X.h>
#include <RobotMotors.h>
#include <RobotImu.h>
#include <RobotEdge.h>
#include <RobotBattery.h>
#include <RobotRgb.h>
#include <RobotWeb.h>

const int STOP_MM = 150, CLEAR_MM = 250;
const float KP_HEADING = 6.0f;  // xe càng lệch càng tệ -> đổi dấu (MPU lắp ngược)

RobotMotors motors;
RobotImu imu;
VL53L0X tof;
bool tofOk = false;
RobotWebState web;

enum Escape { NONE, EDGE_BRAKE, EDGE_BACK, EDGE_TURN, OBST_BRAKE, OBST_TURN };
Escape esc = NONE;
uint32_t escT0 = 0;
bool escTurnRight = true;
bool holdingHeading = false;

void startEscape(Escape e) { esc = e; escT0 = millis(); holdingHeading = false; }

void readTof() {
  if (!tofOk) return;
  // không chặn: chỉ đọc khi đã có kết quả mới
  if ((tof.readReg(VL53L0X::RESULT_INTERRUPT_STATUS) & 0x07) == 0) return;
  int mm = tof.readRangeContinuousMillimeters();
  web.distMm = (tof.timeoutOccurred() || mm >= 8000) ? -1 : mm;
}

bool obstacle() { return web.distMm >= 0 && web.distMm < STOP_MM; }

// Đi thẳng có giữ hướng bằng gyro.
void straight(int speed) {
  if (!holdingHeading) { imu.resetYaw(); holdingHeading = true; }
  int corr = imu.ok() ? (int)(KP_HEADING * imu.yawDeg()) : 0;  // yaw>0 = lệch trái -> xoay phải
  motors.drive(speed + corr, speed - corr);
}

// Trả về true khi đang bận thoát hiểm.
bool runEscape() {
  uint32_t t = millis() - escT0;
  int s = 150;
  switch (esc) {
    case NONE: return false;
    case EDGE_BRAKE: motors.brake(); if (t > 80) startEscape(EDGE_BACK); break;
    case EDGE_BACK:  motors.backward(s); if (t > 350) startEscape(EDGE_TURN); break;
    case EDGE_TURN:
      escTurnRight ? motors.turnRight(s) : motors.turnLeft(s);
      if (t > 300) { motors.brake(); esc = NONE; }
      break;
    case OBST_BRAKE: motors.brake(); if (t > 150) startEscape(OBST_TURN); break;
    case OBST_TURN:
      motors.turnRight(s);
      if ((web.distMm < 0 || web.distMm > CLEAR_MM) || t > 2500) { motors.brake(); esc = NONE; }
      break;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  motors.begin();
  RobotEdge::begin();
  rgb(20, 20, 0);  // vàng = đang khởi động
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  tof.setTimeout(50);
  tofOk = tof.init();
  if (tofOk) tof.startContinuous(33); else Serial.println("Khong thay VL53L0X");
  if (imu.begin()) imu.calibrate(); else Serial.println("Khong thay MPU6050");
  Serial.println("Mo trinh duyet: http://" + RobotWeb::begin(web));
  rgb(0, 20, 0);
}

void loop() {
  RobotWeb::handle();
  imu.update();
  readTof();
  web.edgeL = RobotEdge::left();
  web.edgeR = RobotEdge::right();
  web.lifted = imu.lifted();

  // 1. Bị nhấc
  if (web.lifted) {
    motors.stop(); esc = NONE; holdingHeading = false;
    web.status = "bi nhac len"; rgb(20, 0, 20);
  }
  // 2. Mép bàn (chỉ khi đang không lùi ra; đang lùi mà vẫn thấy mép thì cứ tiếp tục lùi)
  else if ((web.edgeL || web.edgeR) && esc != EDGE_BACK && esc != EDGE_BRAKE) {
    escTurnRight = web.edgeL && !web.edgeR;  // mép bên trái -> quay phải
    startEscape(EDGE_BRAKE); motors.brake();
    web.status = "mep ban!"; rgb(40, 0, 0);
  }
  else if (runEscape()) {
    // đang thoát hiểm
  }
  // 3 + 4. Tự động
  else if (web.autoMode) {
    if (obstacle()) { startEscape(OBST_BRAKE); web.status = "ne vat can"; rgb(20, 10, 0); }
    else { straight(web.speed); web.status = "tu dong"; rgb(0, 0, 30); }
  }
  // 3 + 4. Lái tay
  else {
    char c = RobotWeb::cmdFresh(web) ? web.cmd : 's';
    int s = web.speed;
    if (c != 'f' && c != 'b') holdingHeading = false;
    web.status = "lai tay"; rgb(0, 20, 0);
    switch (c) {
      case 'f':
        if (obstacle()) { motors.brake(); web.status = "vat can phia truoc"; rgb(20, 10, 0); }
        else straight(s);
        break;
      case 'b': straight(-s); break;
      case 'l': motors.turnLeft(s); break;
      case 'r': motors.turnRight(s); break;
      default: motors.stop();
    }
  }

  static uint32_t lastBatt = 0;
  if (millis() - lastBatt > 2000) {
    lastBatt = millis();
    web.battV = RobotBattery::volts();
    web.battPct = RobotBattery::percent(web.battV);
  }
}
