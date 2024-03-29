#!/usr/bin/env python3

import sys
import socketserver
from http.server import SimpleHTTPRequestHandler

class WasmHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        SimpleHTTPRequestHandler.end_headers(self)

        WasmHandler.extensions_map['.js']   = 'application/x-javascript'
        WasmHandler.extensions_map['.html'] = 'text/html',
        WasmHandler.extensions_map['.png']  = 'image/png',
        WasmHandler.extensions_map['.jpg']  = 'image/jpg',
        WasmHandler.extensions_map['.svg']  = 'image/svg+xml',
        WasmHandler.extensions_map['.css']  = 'text/css',

        if sys.version_info < (3, 7, 5):
            WasmHandler.extensions_map['.wasm'] = 'application/wasm'

if __name__ == '__main__':
    PORT = 8080
    with socketserver.TCPServer(("", PORT), WasmHandler) as httpd:
        print(f"Listening on port {PORT}. Press Ctrl+C to stop.")
        httpd.serve_forever()


