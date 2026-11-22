// Bảng chân dùng chung cho mọi phase. Điền số GPIO khi có phần cứng (-1 = chưa gán).
#pragma once
// TB6612 + N20 (xích)
#define PIN_MOTOR_AIN1 -1
#define PIN_MOTOR_AIN2 -1
#define PIN_MOTOR_PWMA -1
#define PIN_MOTOR_BIN1 -1
#define PIN_MOTOR_BIN2 -1
#define PIN_MOTOR_PWMB -1
#define PIN_MOTOR_STBY -1
// ST7789 1.54" (SPI)
#define PIN_TFT_SCLK -1
#define PIN_TFT_MOSI -1
#define PIN_TFT_CS   -1
#define PIN_TFT_DC   -1
#define PIN_TFT_RST  -1
#define PIN_TFT_BL   -1
// I2C: VL53L0X, MPU6050, BH1750, DS3231
#define PIN_I2C_SDA -1
#define PIN_I2C_SCL -1
// TCRT5000 x2
#define PIN_LINE_L -1
#define PIN_LINE_R -1
// INMP441 mic (I2S)
#define PIN_MIC_SCK -1
#define PIN_MIC_WS  -1
#define PIN_MIC_SD  -1
// MAX98357A amp (I2S)
#define PIN_AMP_BCLK -1
#define PIN_AMP_LRC  -1
#define PIN_AMP_DIN  -1
// microSD (SPI)
#define PIN_SD_CS -1
// Khác
#define PIN_WS2812 -1
#define PIN_SERVO  -1
#define PIN_RADAR_OUT -1   // LD2410C OUT
#define PIN_RADAR_RX  -1   // LD2410C UART
#define PIN_RADAR_TX  -1
