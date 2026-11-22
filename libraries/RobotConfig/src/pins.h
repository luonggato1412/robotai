// Bảng chân dùng chung cho mọi phase — ESP32-S3 N16R8 (DevKitC-1).
// Tránh: GPIO 0/3/45/46 (strapping), 19/20 (USB), 26–37 (flash + PSRAM octal), 43/44 chỉ dùng khi Serial qua USB-CDC.
// -1 = không nối (chân đó cắm cứng lên 3V3 / bỏ qua).
#pragma once

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
