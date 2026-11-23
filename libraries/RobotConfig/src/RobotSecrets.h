// Dùng secrets.h nếu có, không thì rơi về secrets.example.h (vẫn biên dịch được).
#pragma once
#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "Chua co secrets.h - dang dung secrets.example.h"
#include "secrets.example.h"
#endif
#ifndef AP_SSID
#define AP_SSID "XeTang-AI"
#endif
#ifndef AP_PASSWORD
#define AP_PASSWORD "xetang123"
#endif
#ifndef BACKUP_AI_KEY
#define BACKUP_AI_KEY ""  // AI dự phòng (Groq...), để trống = tắt
#endif
