// 2 cảm biến TCRT5000 dưới mũi: true = mất mặt bàn (sắp rơi).
#pragma once
#include <Arduino.h>
#include <pins.h>

namespace RobotEdge {
inline void begin() { pinMode(PIN_LINE_L, INPUT); pinMode(PIN_LINE_R, INPUT); }
inline bool left() { return digitalRead(PIN_LINE_L) == EDGE_ACTIVE_LEVEL; }
inline bool right() { return digitalRead(PIN_LINE_R) == EDGE_ACTIVE_LEVEL; }
}  // namespace RobotEdge
