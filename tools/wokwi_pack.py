#!/usr/bin/env python3
"""Sinh thư mục wokwi/ từ phase1_movement + libraries/ (chạy lại mỗi khi sửa code).

Mỗi wokwi/<sketch>/ gồm: <sketch>.ino (dán vào tab sketch.ino), robot_lib.h (gộp pins.h + RobotCore
+ VL53L0X giả), diagram.json (sơ đồ giả lập), wokwi.toml (cho Wokwi trong VS Code).
Dùng: python3 tools/wokwi_pack.py        (ghi file)
      python3 tools/wokwi_pack.py --check (CI: báo lỗi nếu wokwi/ chưa cập nhật)
"""
import pathlib, re, sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TPL = ROOT / "tools" / "wokwi"
OUT = ROOT / "wokwi"
SRC = ROOT / "phase1_movement"
NET = ROOT / "phase5_gemini_live"  # sketch chi can Wi-Fi: 1 file, khong can robot_lib.h
CFG = ROOT / "libraries" / "RobotConfig" / "src"
CORE = ROOT / "libraries" / "RobotCore" / "src"

LIB_PARTS = [
    TPL / "sim_prelude.h", CFG / "pins.h", TPL / "sim_vl53l0x.h",
    CORE / "RobotRgb.h", CORE / "RobotMotors.h", CORE / "RobotMotors.cpp",
    CORE / "RobotEdge.h", CORE / "RobotBattery.h", CORE / "RobotImu.h", CORE / "RobotImu.cpp",
    CORE / "RobotWeb.h", CORE / "RobotWeb.cpp",
]
DROP = re.compile(r'^\s*(#pragma once|#include\s*[<"](pins|RobotSecrets|Robot\w+|VL53L0X)\.h[>"])\s*$')
HEADER = "// TU SINH bang tools/wokwi_pack.py - dung sua o day, sua trong libraries/ roi chay lai script.\n"


def strip(text):
    return "\n".join(l for l in text.splitlines() if not DROP.match(l)) + "\n"


def build_lib():
    out = [HEADER, "#pragma once\n#include <Arduino.h>\n#include <Wire.h>\n"]
    for p in LIB_PARTS:
        out.append(f"\n// ===== {p.relative_to(ROOT)} =====\n")
        out.append(strip(p.read_text(encoding="utf-8")))
    return "".join(out)


def build_sketch(text):
    lines, done = [], False
    for l in text.splitlines():
        if DROP.match(l):
            if not done:
                lines.append('#include "robot_lib.h"  // Wokwi: gop san thu vien, xem wokwi/README.md')
                done = True
            continue
        lines.append(l)
    return HEADER + "\n".join(lines) + "\n"


NET_SECRETS = """// Wokwi: Wi-Fi gia lap (khong mat khau). DAN KEY Gemini cua ban vao dong duoi (dung luu du an cong khai!)
#define WIFI_SSID      "Wokwi-GUEST"
#define WIFI_PASSWORD  ""
#define GEMINI_API_KEY "DAN-KEY-VAO-DAY"
// (Tuy chon) AI du phong khi Gemini qua tai: key Groq mien phi tai https://console.groq.com/keys
#define BACKUP_AI_KEY  "DAN-KEY-GROQ-VAO-DAY"\n"""
NET_DIAGRAM = """{
  "version": 1,
  "author": "robotai",
  "editor": "wokwi",
  "parts": [ { "type": "board-esp32-s3-devkitc-1", "id": "esp", "top": 0, "left": 0, "attrs": {} } ],
  "connections": [ [ "esp:TX", "$serialMonitor:RX", "", [] ], [ "esp:RX", "$serialMonitor:TX", "", [] ] ]
}
"""


SERVER_SECRETS = """// Wokwi: Wi-Fi gia lap. host.wokwi.internal = may tinh dang chay Wokwi (VS Code), noi chay server/robot_server.py
#define WIFI_SSID      "Wokwi-GUEST"
#define WIFI_PASSWORD  ""
#define SERVER_HOST    "host.wokwi.internal"
#define SERVER_PORT    8765
#define ROBOT_TOKEN    "xetang-bi-mat"  // giong ROBOT_TOKEN trong server/.env\n"""


def build_net_sketch(text):
    inc = '#include "RobotSecrets.h"'
    sec = SERVER_SECRETS if "SERVER_HOST" in text else NET_SECRETS
    return HEADER + "\n".join(sec.rstrip() if l.startswith(inc) else l for l in text.splitlines()) + "\n"


def toml(name, forward=True):
    b = f"build/esp32.esp32.esp32s3/{name}.ino"
    return (f'[wokwi]\nversion = 1\nfirmware = "{b}.merged.bin"\nelf = "{b}.elf"\n'
            + ('\n# Mo trang web lai xe: http://localhost:8180\n'
               '[[net.forward]]\nfrom = "localhost:8180"\nto = "target:80"\n' if forward else ''))


def main():
    check = "--check" in sys.argv
    lib, diagram = build_lib(), (TPL / "diagram.json").read_text(encoding="utf-8")
    files = {}
    for ino in sorted(SRC.glob("*/*.ino")):
        d = OUT / ino.parent.name
        files[d / ino.name] = build_sketch(ino.read_text(encoding="utf-8"))
        files[d / "robot_lib.h"] = lib
        files[d / "diagram.json"] = diagram
        files[d / "wokwi.toml"] = toml(ino.parent.name)
    for ino in sorted(NET.glob("*/*.ino")):
        d = OUT / ino.parent.name
        files[d / ino.name] = build_net_sketch(ino.read_text(encoding="utf-8"))
        files[d / "diagram.json"] = NET_DIAGRAM
        files[d / "wokwi.toml"] = toml(ino.parent.name, forward=False)
    stale = [p for p, t in files.items() if not p.exists() or p.read_text(encoding="utf-8") != t]
    if check:
        if stale:
            print("wokwi/ chua cap nhat, chay: python3 tools/wokwi_pack.py\n" +
                  "\n".join(str(p.relative_to(ROOT)) for p in stale))
            sys.exit(1)
        print("wokwi/ da cap nhat")
        return
    for p in stale:
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(files[p], encoding="utf-8")
    print(f"Da ghi {len(stale)} file vao wokwi/")


if __name__ == "__main__":
    main()
