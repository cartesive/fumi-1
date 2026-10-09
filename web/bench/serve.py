#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The audition bench's local server: serves the repository (the bench page from build/bench/, the reference
files from refs/) and lists what refs/ holds, so the page can offer the .syx banks and the recordings.
Nothing leaves the machine; notes and ratings live in the browser and in bench/sessions/ when exported.

  web/bench/serve.py [--port 8765]      then open http://localhost:8765/build/bench/

  GET /refs/list                  JSON: {"syx": [...], "audio": [...]} (file names in refs/)
  POST /bench/sessions/NAME.json  saves an exported session into bench/sessions/ (the repo, for the next round)
"""
import argparse
import json
import re
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AUDIO = {".wav", ".mp3", ".m4a", ".aif", ".aiff", ".flac", ".ogg"}


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, **k):
        super().__init__(*a, directory=str(ROOT), **k)

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def do_GET(self):
        if self.path.split("?")[0] == "/refs/list":
            refs = ROOT / "refs"
            files = sorted(p.name for p in refs.iterdir() if p.is_file()) if refs.is_dir() else []
            body = json.dumps({"syx": [f for f in files if f.lower().endswith(".syx")],
                               "audio": [f for f in files if Path(f).suffix.lower() in AUDIO]}).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        super().do_GET()

    def do_POST(self):
        m = re.fullmatch(r"/bench/sessions/([A-Za-z0-9_.-]+\.json)", self.path)
        if not m:
            self.send_error(404)
            return
        n = int(self.headers.get("Content-Length", "0"))
        data = self.rfile.read(n)
        try:
            json.loads(data)
        except ValueError:
            self.send_error(400, "not JSON")
            return
        out = ROOT / "bench" / "sessions"
        out.mkdir(parents=True, exist_ok=True)
        (out / m.group(1)).write_bytes(data)
        self.send_response(204)
        self.end_headers()
        print(f"saved bench/sessions/{m.group(1)} ({n} bytes)")

    def log_message(self, fmt, *args):
        if "/refs/list" in fmt % args or "GET /build/bench" in fmt % args:
            return
        super().log_message(fmt, *args)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8765)
    a = ap.parse_args()
    print(f"FuMi-1 bench: http://localhost:{a.port}/build/bench/   (refs in {ROOT / 'refs'})")
    ThreadingHTTPServer(("127.0.0.1", a.port), Handler).serve_forever()


if __name__ == "__main__":
    main()
