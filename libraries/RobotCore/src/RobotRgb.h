#pragma once
#include <Arduino.h>
#include <pins.h>
// LED RGB onboard. Core 3.x: rgbLedWrite(pin, r, g, b).
inline void rgb(uint8_t r, uint8_t g, uint8_t b) { rgbLedWrite(PIN_RGB_LED, r, g, b); }
