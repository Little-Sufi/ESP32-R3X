#!/usr/bin/env python3
"""
ESP32-R3X Mobile Companion - Local Development & Network Server
Serves the Mobile Web App with CORS headers, MIME types, and local network discovery.
"""

import http.server
import socketserver
import os
import socket
import sys

PORT = 8080
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def end_headers(self):
        # Enable CORS for local testing
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'X-Requested-With, Content-Type')
        self.send_header('Cache-Control', 'no-cache, must-revalidate')
        super().end_headers()

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
    print("=" * 60)
    print("      ESP32-R3X MOBILE COMPANION SERVER (v3.0)")
    print("=" * 60)
    print(f"Local URL   : http://localhost:{PORT}")
    print(f"Network URL : http://{local_ip}:{PORT} (Open on your Mobile Phone!)")
    print(f"Directory   : {DIRECTORY}")
    print("=" * 60)
    print("Press Ctrl+C to stop.\n")

    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")
