#!/usr/bin/env python3
"""
============================================================================
ESP32-R3X Mobile Companion - Server & Hardware Serial Bridge (v3.0)
============================================================================
Serves the Mobile Web App with CORS headers, and provides an active
Serial Bridge between the ESP32-S3 hardware and any connected phone/browser.
"""

import http.server
import socketserver
import os
import socket
import sys
import json
import time
import threading
import urllib.parse

try:
    import serial
    import serial.tools.list_ports
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False

PORT = 8080
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------
# Global Hardware Serial Bridge State
# ---------------------------------------------------------------------------
class HardwareBridge:
    def __init__(self):
        self.lock = threading.Lock()
        self.ser = None
        self.port_name = None
        self.is_connected = False
        self.last_status = {
            "connected": False,
            "port": "None",
            "menu_idx": 6,
            "menu_name": "Tools",
            "in_sub_menu": 0,
            "sub_idx": 0,
            "feature_active": 0,
            "vBat": 4.15,
            "free_heap": 131720,
            "uptime": 0
        }
        self.recent_logs = []
        self.running = True
        self.thread = threading.Thread(target=self._worker_loop, daemon=True)
        self.thread.start()

    def _log(self, msg):
        timestamp = time.strftime("%H:%M:%S")
        entry = f"[{timestamp}] {msg}"
        with self.lock:
            self.recent_logs.append(entry)
            if len(self.recent_logs) > 50:
                self.recent_logs.pop(0)

    def _find_port(self):
        if not SERIAL_AVAILABLE:
            return None
        ports = list(serial.tools.list_ports.comports())
        # Priority 1: Check COM9 or devices with CH343, CH340, CP210, ESP32 in description
        for p in ports:
            d = (p.description or "").lower()
            if "com9" in p.device.lower() or "ch34" in d or "cp210" in d or "esp" in d or "usb-enhanced" in d:
                return p.device
        if ports:
            return ports[0].device
        return None

    def _worker_loop(self):
        last_poll = 0
        while self.running:
            if not self.is_connected:
                target_port = self._find_port()
                if target_port and SERIAL_AVAILABLE:
                    try:
                        s = serial.Serial(target_port, 115200, timeout=0.1)
                        time.sleep(0.3)
                        with self.lock:
                            self.ser = s
                            self.port_name = target_port
                            self.is_connected = True
                            self.last_status["connected"] = True
                            self.last_status["port"] = target_port
                        self._log(f"Connected to ESP32 on {target_port}")
                    except Exception as e:
                        time.sleep(2)
                else:
                    time.sleep(2)
                    continue

            # Read serial data
            try:
                line = None
                if self.ser and self.ser.is_open and self.ser.in_waiting:
                    raw = self.ser.readline().decode('utf-8', errors='replace').strip()
                    if raw:
                        self._handle_incoming(raw)

                # Periodic status poll every 1.5s
                now = time.time()
                if now - last_poll > 1.5:
                    last_poll = now
                    if self.ser and self.ser.is_open:
                        self.ser.write(b"\nSTATUS\n")

                time.sleep(0.05)
            except Exception as e:
                self._log(f"Serial link lost: {e}")
                with self.lock:
                    if self.ser:
                        try:
                            self.ser.close()
                        except:
                            pass
                    self.ser = None
                    self.is_connected = False
                    self.last_status["connected"] = False
                time.sleep(1)

    def _handle_incoming(self, line):
        self._log(line)
        if line.startswith("[STATUS]"):
            # Example: [STATUS] menu_idx=6 (Tools), in_sub_menu=0, sub_idx=5, feature_active=0, exit_req=0, vBat=4.12V
            try:
                data = {}
                parts = line[8:].split(",")
                for p in parts:
                    if "=" in p:
                        k, v = p.strip().split("=", 1)
                        data[k.strip()] = v.strip()

                with self.lock:
                    if "menu_idx" in data:
                        raw_idx = data["menu_idx"]
                        idx_num = int(raw_idx.split()[0])
                        self.last_status["menu_idx"] = idx_num
                        if "(" in raw_idx and ")" in raw_idx:
                            self.last_status["menu_name"] = raw_idx.split("(")[1].split(")")[0]
                    if "in_sub_menu" in data:
                        self.last_status["in_sub_menu"] = int(data["in_sub_menu"])
                    if "sub_idx" in data:
                        self.last_status["sub_idx"] = int(data["sub_idx"])
                    if "feature_active" in data:
                        self.last_status["feature_active"] = int(data["feature_active"])
                    if "vBat" in data:
                        v_str = data["vBat"].replace("V", "").strip()
                        self.last_status["vBat"] = float(v_str)
            except Exception:
                pass
        elif line.startswith("[PONG]"):
            # Example: [PONG] uptime=232430 free_heap=131720 min_heap=101624
            try:
                parts = line[6:].split()
                with self.lock:
                    for p in parts:
                        if "=" in p:
                            k, v = p.split("=", 1)
                            if k == "uptime":
                                self.last_status["uptime"] = int(v) // 1000
                            elif k == "free_heap":
                                self.last_status["free_heap"] = int(v)
            except Exception:
                pass

    def send_command(self, cmd):
        with self.lock:
            if not self.ser or not self.ser.is_open:
                return False, "Hardware serial bridge not connected"
            try:
                payload = (cmd.strip() + "\n").encode('utf-8')
                self.ser.write(payload)
                self.recent_logs.append(f"> {cmd.strip()}")
                return True, "OK"
            except Exception as e:
                return False, str(e)

    def get_status(self):
        with self.lock:
            return dict(self.last_status)

    def get_logs(self):
        with self.lock:
            return list(self.recent_logs)

bridge = HardwareBridge()

# ---------------------------------------------------------------------------
# HTTP Handler with Static Serving + API Endpoints
# ---------------------------------------------------------------------------
class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'X-Requested-With, Content-Type')
        self.send_header('Cache-Control', 'no-cache, must-revalidate')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path

        if path == "/api/status" or path == "/ping":
            st = bridge.get_status()
            payload = json.dumps({"ok": True, "bridge": True, "status": st}).encode('utf-8')
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if path == "/api/send" or path == "/api/cmd":
            qs = urllib.parse.parse_qs(parsed.query)
            cmd = qs.get("cmd", qs.get("c", [""]))[0]
            if cmd:
                success, msg = bridge.send_command(cmd)
                payload = json.dumps({"ok": success, "msg": msg, "cmd": cmd}).encode('utf-8')
            else:
                payload = json.dumps({"ok": False, "msg": "Missing cmd parameter"}).encode('utf-8')
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if path == "/api/logs":
            logs = bridge.get_logs()
            payload = json.dumps({"ok": True, "logs": logs}).encode('utf-8')
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        return super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/api/send":
            length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(length).decode('utf-8', errors='replace')
            try:
                data = json.loads(body)
                cmd = data.get("cmd", "")
            except Exception:
                cmd = body.strip()

            if cmd:
                success, msg = bridge.send_command(cmd)
                payload = json.dumps({"ok": success, "msg": msg, "cmd": cmd}).encode('utf-8')
            else:
                payload = json.dumps({"ok": False, "msg": "Empty command"}).encode('utf-8')

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        self.send_response(404)
        self.end_headers()

def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

if __name__ == "__main__":
    local_ip = get_local_ip()
    print("=" * 65)
    print("      ESP32-R3X MOBILE COMPANION & HARDWARE BRIDGE (v3.0)")
    print("=" * 65)
    print(f"Local URL   : http://localhost:{PORT}")
    print(f"Network URL : http://{local_ip}:{PORT} (Open on your Mobile Phone!)")
    print(f"Directory   : {DIRECTORY}")
    print("Hardware    : Auto-scanning for ESP32 on USB Serial (COM9)...")
    print("=" * 65)
    print("Press Ctrl+C to stop.\n")

    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            bridge.running = False
            print("\nServer stopped.")
