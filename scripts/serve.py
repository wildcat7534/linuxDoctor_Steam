#!/usr/bin/env python3
"""Serve Linux Doctor locally with the headers required by threaded WebAssembly."""

import os
import sys
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


class LinuxDoctorHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def main():
    project_root = Path(__file__).resolve().parents[1]
    os.chdir(project_root / "frontend")
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 4545
    server = ThreadingHTTPServer(("127.0.0.1", port), LinuxDoctorHandler)
    print(f"Linux Doctor: http://127.0.0.1:{port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
