// Copy thành secrets.h (CÙNG thư mục này, đã bị .gitignore) và điền giá trị thật. KHÔNG commit secrets.h.
#pragma once
#define WIFI_SSID      "your-ssid"
#define WIFI_PASSWORD  "your-password"
#define GEMINI_API_KEY "your-gemini-api-key"
// AI dự phòng khi Gemini quá tải (Groq, miễn phí: https://console.groq.com/keys). Để "" = tắt.
#define BACKUP_AI_KEY  ""
// Nếu không vào được Wi-Fi nhà, robot tự phát Wi-Fi riêng:
#define AP_SSID        "XeTang-AI"
#define AP_PASSWORD    "xetang123"
// Server riêng (bước 5.2, server/robot_server.py): IP máy tính chạy server (server in ra khi khởi động).
#define SERVER_HOST    "192.168.1.100"
#define SERVER_PORT    8765
#define ROBOT_TOKEN    "xetang-bi-mat"  // giống ROBOT_TOKEN trong server/.env
