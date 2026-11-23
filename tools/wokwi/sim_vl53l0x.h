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
