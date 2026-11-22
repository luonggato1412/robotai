// Bước 7: lái bằng điện thoại. Mở Serial Monitor để xem IP, rồi vào http://<IP> trên điện thoại
// (cùng Wi-Fi). Không vào được Wi-Fi nhà thì kết nối Wi-Fi "XeTang-AI", mở http://192.168.4.1
// Bản này chỉ lái tay + hiện pin; ghép cảm biến nằm ở phase1_robot.
#include <RobotMotors.h>
#include <RobotBattery.h>
#include <RobotWeb.h>

RobotMotors motors;
RobotWebState web;

void setup() {
  Serial.begin(115200);
  motors.begin();
  Serial.println("Dang ket noi Wi-Fi...");
  Serial.println("Mo trinh duyet: http://" + RobotWeb::begin(web));
}

void loop() {
  RobotWeb::handle();
  char c = RobotWeb::cmdFresh(web) ? web.cmd : 's';  // mất sóng 0.4 s -> dừng
  int s = web.speed;
  switch (c) {
    case 'f': motors.forward(s); break;
    case 'b': motors.backward(s); break;
    case 'l': motors.turnLeft(s); break;
    case 'r': motors.turnRight(s); break;
    default: motors.stop();
  }
  static uint32_t last = 0;
  if (millis() - last > 1000) {
    last = millis();
    web.battV = RobotBattery::volts();
    web.battPct = RobotBattery::percent(web.battV);
    web.status = web.autoMode ? "tu dong (chua co o buoc nay)" : "lai tay";
  }
}
