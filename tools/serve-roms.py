#!/usr/bin/env python3
"""Tiny ROM server for the GB-EMU Tizen app's "Download ROMs (Wi-Fi)" menu.

Serves every .gb/.gbc file in a directory, plus /roms.json (the file-name
index the TV app fetches first). Sends CORS headers so the TV's web runtime
accepts the responses. Only needs to run while the TV is downloading.

Usage:
    ./tools/serve-roms.py [roms_dir] [port]     # defaults: ./roms, 8000
"""
import http.server
import json
import os
import socket
import sys

ROMS_DIR = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "roms")
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8000
ROM_EXTENSIONS = (".gb", ".gbc")


def list_roms():
    return sorted(
        f for f in os.listdir(ROMS_DIR)
        if f.lower().endswith(ROM_EXTENSIONS)
        and os.path.isfile(os.path.join(ROMS_DIR, f))
    )


def lan_ip():
    # Opening a UDP "connection" is a portable way to find the outbound
    # interface's address without sending any packets.
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        s.close()


class RomHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=ROMS_DIR, **kwargs)

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()

    def do_GET(self):
        if self.path == "/roms.json":
            body = json.dumps(list_roms()).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            super().do_GET()


if __name__ == "__main__":
    roms = list_roms()
    print(f"Serving {len(roms)} ROM(s) from {ROMS_DIR}:")
    for rom in roms:
        print(f"  - {rom}")
    print(f"\nROM server URL for the app: http://{lan_ip()}:{PORT}")
    print("On the TV, open the game menu (BLUE button) and check that")
    print('"Set ROM Server" shows this address (it is remembered once saved),')
    print('then pick "Download ROMs (Wi-Fi)". Ctrl+C stops the server.\n')
    http.server.ThreadingHTTPServer(("", PORT), RomHandler).serve_forever()
