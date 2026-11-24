#!/usr/bin/env python3
"""Giả làm robot để thử server (không cần ESP32). Chạy server trước, rồi mở cửa sổ khác:
    py thu_robot.py                 (nối tới server trên chính máy này)
    py thu_robot.py 192.168.1.50    (server ở máy khác)
Gõ câu hỏi rồi Enter, gõ "thoat" để thoát."""
import asyncio
import json
import pathlib
import sys

from websockets.asyncio.client import connect

HERE = pathlib.Path(__file__).resolve().parent


def env(key, default):
    f = HERE / ".env"
    for line in f.read_text(encoding="utf-8-sig").splitlines() if f.exists() else []:
        if line.strip().startswith(key + "="):
            return line.split("=", 1)[1].strip().strip('"')
    return default


async def main():
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    url = f"ws://{host}:{env('PORT', '8765')}/robot?token={env('ROBOT_TOKEN', '')}"
    print("Nối", url.split("?")[0], "...")
    async with connect(url) as ws:
        print("Server:", await ws.recv())
        while True:
            q = (await asyncio.to_thread(input, "Bạn: ")).strip()
            if q == "thoat":
                return
            if not q:
                continue
            await ws.send(json.dumps({"type": "text", "text": q}, ensure_ascii=False))
            print("Robot: ", end="", flush=True)
            while True:
                m = json.loads(await ws.recv())
                if m["type"] == "text":
                    print(m["text"], end="", flush=True)
                elif m["type"] == "action":
                    print(f" [cử động: {m['hanh_dong']} {m['ms']} ms] ", end="", flush=True)
                elif m["type"] == "done":
                    print(f"\n({m['ai']}, {m['ms']} ms, âm thanh {m['audio']} byte)\n")
                    break
                elif m["type"] == "error":
                    print("\n[Lỗi]", m["text"], "\n")
                    break


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except (KeyboardInterrupt, EOFError):
        pass
    except OSError as e:
        print("Không nối được server (đã chạy robot_server.py chưa?):", e)
