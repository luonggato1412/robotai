#!/usr/bin/env python3
"""Server riêng cho robot (bản thử): ESP32 <-> server này <-> Gemini Live.

- Server giữ API key và "tính cách" robot (persona.txt, giọng nói), robot chỉ cần biết địa chỉ server.
- Bây giờ chuyển CHỮ: robot gửi câu hỏi, server trả lời từng đoạn (bản chép lời giọng nói Gemini).
  Âm thanh Gemini trả về mới được đếm số byte (sau này chuyển tiếp cho loa).
- Nhớ các lượt nói gần đây vào memory.json, lần nối Gemini sau vẫn nhớ.
- Gemini gọi hàm cu_dong -> server gửi lệnh {"type":"action"} xuống robot.
- Gemini Live không vào được / quá tải -> hỏi AI dự phòng chuẩn OpenAI (Groq) bằng chữ.

Chạy:  py robot_server.py      (Windows)      hoặc  python3 robot_server.py
Cần:   py -m pip install -r requirements.txt , và file .env (copy từ .env.example).

Giao thức với robot (WebSocket, mỗi tin là 1 JSON chữ, "type" luôn đứng đầu):
  robot -> server : ws://<ip>:<PORT>/robot?token=<ROBOT_TOKEN>
                    {"type":"text","text":"câu hỏi"}
  server -> robot : {"type":"ready","ai":"..."}          đã sẵn sàng
                    {"type":"text","text":"đoạn trả lời"}  (nhiều tin, ghép lại)
                    {"type":"action","name":"cu_dong","hanh_dong":"tien","ms":1000}
                    {"type":"done","ai":"...","ms":1234,"audio":5678}  hết lượt trả lời
                    {"type":"error","text":"..."}
"""
import asyncio
import json
import pathlib
import socket
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

try:
    from websockets.asyncio.client import connect as ws_connect
    from websockets.asyncio.server import serve
    from websockets.exceptions import ConnectionClosed
except ImportError:
    sys.exit("Thiếu thư viện websockets (bản 13 trở lên). Chạy:  py -m pip install -r requirements.txt")

HERE = pathlib.Path(__file__).resolve().parent

# Model Gemini Live, thử lần lượt (giống sketch p5_1_gemini_live).
LIVE_MODELS = [
    "gemini-3.1-flash-live-preview",
    "gemini-3.8-live",
    "gemini-2.5-flash-native-audio-preview-12-2025",
]
LIVE_URL = "wss://generativelanguage.googleapis.com/ws/google.ai.generativelanguage.v1beta.GenerativeService.BidiGenerateContent"
# AI dự phòng chuẩn OpenAI (Groq). Muốn dùng OpenAI/OpenRouter thì đổi URL + tên model.
BACKUP_URL = "https://api.groq.com/openai/v1/chat/completions"
BACKUP_MODELS = ["openai/gpt-oss-20b", "openai/gpt-oss-120b"]
REPLY_TIMEOUT = 30  # giây chờ Gemini trả lời xong 1 lượt

TOOLS = [{"functionDeclarations": [{
    "name": "cu_dong",
    "description": "Điều khiển xe tăng robot di chuyển.",
    "parameters": {"type": "OBJECT", "properties": {
        "hanh_dong": {"type": "STRING", "enum": ["tien", "lui", "trai", "phai", "xoay", "dung"],
                      "description": "tien = đi tới, lui = đi lùi, trai/phai = quay trái/phải, xoay = xoay tròn, dung = dừng"},
        "ms": {"type": "INTEGER", "description": "Thời gian chạy, mili giây (100-5000)"},
    }, "required": ["hanh_dong"]},
}]}]


def load_env():
    env = {"PORT": "8765", "VOICE": "Puck", "TOOLS": "1", "MEMORY_TURNS": "20", "BACKUP_AI_KEY": "", "ROBOT_TOKEN": ""}
    f = HERE / ".env"
    if not f.exists():
        sys.exit("Chưa có file .env. Copy .env.example thành .env rồi điền GEMINI_API_KEY (xem README.md).")
    for line in f.read_text(encoding="utf-8-sig").splitlines():  # utf-8-sig: Notepad hay thêm BOM
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            k, v = line.split("=", 1)
            env[k.strip()] = v.strip().strip('"')
    if not env.get("GEMINI_API_KEY") or env["GEMINI_API_KEY"].startswith("dan-"):
        sys.exit("Chưa điền GEMINI_API_KEY trong .env (lấy key tại https://aistudio.google.com/apikey).")
    return env


ENV = load_env()
PERSONA = (HERE / "persona.txt").read_text(encoding="utf-8-sig").strip()
MEMORY_FILE = HERE / "memory.json"
MEMORY_TURNS = int(ENV["MEMORY_TURNS"])


def log(*a):
    print(time.strftime("%H:%M:%S"), *a, flush=True)


# ---------- Trí nhớ: các lượt nói gần đây ----------
def load_memory():
    try:
        return json.loads(MEMORY_FILE.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return []


memory = load_memory()


def remember(user, robot):
    if MEMORY_TURNS <= 0 or not robot.strip():
        return
    memory.append({"nguoi": user, "robot": robot.strip()})
    del memory[:-MEMORY_TURNS]
    MEMORY_FILE.write_text(json.dumps(memory, ensure_ascii=False, indent=1), encoding="utf-8")


def system_text():
    if not memory:
        return PERSONA
    lines = "\n".join(f"- Người: {m['nguoi']}\n  Bạn: {m['robot']}" for m in memory)
    return PERSONA + "\n\nNhững gì đã nói gần đây (để nhớ, không cần nhắc lại):\n" + lines


# ---------- Gemini Live ----------
class Live:
    """Một kết nối Gemini Live cho 1 robot. Rớt / hết hạn (goAway ~10 phút) thì nối lại ở câu sau."""

    def __init__(self):
        self.ws = None
        self.model = None
        self.start = 0  # vị trí bắt đầu thử trong LIVE_MODELS

    async def close(self):
        if self.ws:
            await self.ws.close()
        self.ws = None

    async def connect(self):
        n = len(LIVE_MODELS)
        for i in range(n):
            model = LIVE_MODELS[(self.start + i) % n]
            gen = {"responseModalities": ["AUDIO"],
                   "speechConfig": {"voiceConfig": {"prebuiltVoiceConfig": {"voiceName": ENV["VOICE"]}}}}
            setup = {"model": "models/" + model, "generationConfig": gen,
                     "systemInstruction": {"parts": [{"text": system_text()}]},
                     "outputAudioTranscription": {}}
            if ENV["TOOLS"] == "1":
                setup["tools"] = TOOLS
            try:
                url = ENV.get("GEMINI_LIVE_URL", LIVE_URL) + "?key=" + ENV["GEMINI_API_KEY"]
                ws = await ws_connect(url, max_size=None, open_timeout=15)
                await ws.send(json.dumps({"setup": setup}))
                while True:
                    msg = json.loads(await asyncio.wait_for(ws.recv(), 20))
                    if "setupComplete" in msg:
                        break
            except ConnectionClosed as e:
                log(f"Gemini từ chối model {model}: {e.rcvd.code if e.rcvd else ''} {e.rcvd.reason if e.rcvd else e}")
                continue
            except (OSError, asyncio.TimeoutError, ValueError) as e:
                log(f"Không vào được Gemini Live ({model}): {e!r}")
                continue
            self.ws, self.model, self.start = ws, model, (self.start + i) % n
            log(f"Đã vào Gemini Live, model {model}")
            return True
        return False

    async def ask(self, text, robot):
        """Gửi 1 câu, chuyển các đoạn trả lời xuống robot. Trả bản chép lời đầy đủ, hoặc None nếu lỗi."""
        if not self.ws and not await self.connect():
            return None
        try:
            await self.ws.send(json.dumps({"realtimeInput": {"text": text}}))
            answer, audio, t0, deadline = "", 0, time.monotonic(), time.monotonic() + REPLY_TIMEOUT
            while True:
                msg = json.loads(await asyncio.wait_for(self.ws.recv(), max(1, deadline - time.monotonic())))
                if "goAway" in msg:
                    log("Gemini báo sắp đóng phiên (goAway), câu sau sẽ nối lại.")
                if "toolCall" in msg:
                    await self.on_tool_call(msg["toolCall"], robot)
                sc = msg.get("serverContent") or {}
                piece = (sc.get("outputTranscription") or {}).get("text", "")
                if piece:
                    answer += piece
                    await send(robot, type="text", text=piece)
                for part in (sc.get("modelTurn") or {}).get("parts", []):
                    audio += len((part.get("inlineData") or {}).get("data", "")) * 3 // 4
                if sc.get("turnComplete"):
                    await send(robot, type="done", ai=self.model, ms=int((time.monotonic() - t0) * 1000), audio=audio)
                    if "goAway" in msg:
                        await self.close()
                    return answer
        except ConnectionClosed as e:
            log(f"Gemini đóng kết nối: {e.rcvd.code if e.rcvd else ''} {e.rcvd.reason if e.rcvd else ''}")
            if e.rcvd and e.rcvd.code == 1011:  # quá tải / hết lượt: lần sau thử model khác trước
                self.start = (self.start + 1) % len(LIVE_MODELS)
        except (asyncio.TimeoutError, OSError, ValueError) as e:
            log(f"Lỗi khi chờ Gemini: {e!r}")
        await self.close()
        return None

    async def on_tool_call(self, call, robot):
        responses = []
        for fc in call.get("functionCalls", []):
            args = fc.get("args") or {}
            log(f"Gemini gọi hàm {fc.get('name')} {args}")
            if fc.get("name") == "cu_dong":
                ms = max(100, min(5000, int(args.get("ms") or 1000)))
                await send(robot, type="action", name="cu_dong", hanh_dong=str(args.get("hanh_dong", "dung")), ms=ms)
                result = {"result": "ok"}
            else:
                result = {"error": "không có hàm này"}
            responses.append({"id": fc.get("id"), "name": fc.get("name"), "response": result})
        await self.ws.send(json.dumps({"toolResponse": {"functionResponses": responses}}))


# ---------- AI dự phòng (chữ, chuẩn OpenAI) ----------
def backup_chat(text):
    msgs = [{"role": "system", "content": system_text()}, {"role": "user", "content": text}]
    for model in BACKUP_MODELS:
        req = urllib.request.Request(BACKUP_URL, json.dumps({"model": model, "messages": msgs}).encode(), {
            "Authorization": "Bearer " + ENV["BACKUP_AI_KEY"], "Content-Type": "application/json",
            "User-Agent": "robotai-server/1.0"})
        try:
            with urllib.request.urlopen(req, timeout=15) as r:
                return model, json.load(r)["choices"][0]["message"]["content"].strip()
        except urllib.error.HTTPError as e:
            log(f"AI dự phòng {model}: HTTP {e.code} {e.read()[:200]!r}")
        except (OSError, ValueError, KeyError) as e:
            log(f"AI dự phòng {model}: {e!r}")
    return None, None


# ---------- Phục vụ robot ----------
async def send(robot, **msg):
    await robot.send(json.dumps(msg, ensure_ascii=False, separators=(",", ":")))


async def handle_robot(robot):
    url = urllib.parse.urlsplit(robot.request.path)
    token = urllib.parse.parse_qs(url.query).get("token", [""])[0]
    if url.path != "/robot" or token != ENV["ROBOT_TOKEN"]:
        log(f"Từ chối {robot.remote_address[0]}: sai đường dẫn hoặc ROBOT_TOKEN")
        await robot.close(1008, "sai token")
        return
    log(f"Robot {robot.remote_address[0]} đã nối")
    live = Live()
    try:
        ok = await live.connect()
        await send(robot, type="ready", ai=live.model if ok else "du-phong" if ENV["BACKUP_AI_KEY"] else "khong-co")
        async for raw in robot:
            try:
                msg = json.loads(raw)
            except ValueError:
                continue
            if msg.get("type") != "text" or not str(msg.get("text", "")).strip():
                continue
            text = msg["text"].strip()
            log(f"Người: {text}")
            answer = await live.ask(text, robot)
            if answer is None and ENV["BACKUP_AI_KEY"]:
                log("Gemini Live không trả lời được, hỏi AI dự phòng...")
                t0 = time.monotonic()
                model, answer = await asyncio.to_thread(backup_chat, text)
                if answer:
                    await send(robot, type="text", text=answer)
                    await send(robot, type="done", ai=model, ms=int((time.monotonic() - t0) * 1000), audio=0)
            if answer is None:
                await send(robot, type="error", text="AI đang bận, thử lại sau ít phút.")
                continue
            log(f"Robot: {answer.strip()}")
            remember(text, answer)
    except ConnectionClosed:
        pass
    finally:
        await live.close()
        log(f"Robot {robot.remote_address[0]} đã ngắt")


def lan_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))  # không gửi gì, chỉ để biết card mạng nào ra Internet
        return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        s.close()


async def main():
    port = int(ENV["PORT"])
    async with serve(handle_robot, "0.0.0.0", port, ping_interval=20, ping_timeout=20):
        ip = lan_ip()
        log(f"Server robot đang chạy. Trong secrets.h của robot đặt SERVER_HOST \"{ip}\", SERVER_PORT {port}")
        log(f"AI dự phòng: {'BẬT' if ENV['BACKUP_AI_KEY'] else 'tắt'}. Nhớ {len(memory)} lượt cũ. Thử nhanh: py thu_robot.py")
        log("Bấm Ctrl+C để dừng.")
        await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Đã dừng server.")
