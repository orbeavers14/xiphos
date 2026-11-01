# SPDX-License-Identifier: GPL-2.0-or-later
"""Loopback-only TLS server; all module/archive data is generated original prose."""
import http.server
import io
import os
from pathlib import Path
import ssl
import subprocess
import sys
import tarfile
import threading
import time
import zipfile
import warnings
warnings.filterwarnings("ignore", message="Duplicate name:")

binary, folder, openssl, *client = sys.argv[1:]
assert len(client) <= 1
root = Path(folder)
assert root.name == "remote-install-fixture"
subprocess.run([binary, str(root), "--fixture"], check=True)
(root / "bad").mkdir()
(root / "tls.cnf").write_text("[req]\ndistinguished_name=dn\nx509_extensions=ext\nprompt=no\n[dn]\nCN=localhost\n[ext]\nsubjectAltName=DNS:localhost,IP:127.0.0.1\nbasicConstraints=critical,CA:TRUE\nkeyUsage=critical,digitalSignature,keyEncipherment,keyCertSign\n")
subprocess.run([openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1", "-config", str(root / "tls.cnf"), "-keyout", str(root / "key.pem"), "-out", str(root / "certificate.pem")], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
with tarfile.open(root / "catalog.tar.gz", "w:gz") as archive:
    archive.add(root / "source/mods.d/Added.conf", arcname="mods.d/Added.conf")
with zipfile.ZipFile(root / "module.zip", "w", zipfile.ZIP_STORED) as archive:
    for path in (root / "source").rglob("*"):
        if path.is_file():
            archive.write(path, str(path.relative_to(root / "source")))

cases = {
    "traversal": [("../escaped", b"original")],
    "absolute": [("/escaped", b"original")],
    "backslash": [("data\\..\\escaped", b"original")],
    "duplicate": [("data/a", b"one"), ("data/a", b"two")],
    "case-alias": [("data/A", b"one"), ("data/a", b"two")],
    "unicode-alias": [("data/\u00e9", b"one"), ("data/e\u0301", b"two")],
    "directory-conflict": [("data/a/file", b"one"), ("data/a", b"two")],
    "device": [("data/CON.txt", b"original")],
    "trailing-dot": [("data/name.", b"original")],
}
for name, entries in cases.items():
    with zipfile.ZipFile(root / "bad" / (name + ".zip"), "w") as archive:
        for path, data in entries:
            archive.writestr(path, data)
        archive.write(root / "source/mods.d/Added.conf", "mods.d/Added.conf")
with zipfile.ZipFile(root / "bad/symlink.zip", "w") as archive:
    archive.write(root / "source/mods.d/Added.conf", "mods.d/Added.conf")
    entry = zipfile.ZipInfo("data/link")
    entry.create_system = 3
    entry.external_attr = 0o120777 << 16
    archive.writestr(entry, "../../escape")
with tarfile.open(root / "bad/hardlink.gz", "w:gz") as archive:
    archive.add(root / "source/mods.d/Added.conf", arcname="mods.d/Added.conf")
    entry = tarfile.TarInfo("mods.d/link.conf")
    entry.type = tarfile.LNKTYPE
    entry.linkname = "../../escape"
    archive.addfile(entry)
with tarfile.open(root / "bad/catalog-extra.gz", "w:gz") as archive:
    archive.add(root / "source/mods.d/Added.conf", arcname="mods.d/Added.conf")
    entry = tarfile.TarInfo("data/outside.conf")
    entry.size = 8
    archive.addfile(entry, io.BytesIO(b"original"))
with zipfile.ZipFile(root / "bad/expansion.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    archive.write(root / "source/mods.d/Added.conf", "mods.d/Added.conf")
    archive.writestr("data/Added/large", b"o" * 32768)
corrupt = bytearray((root / "module.zip").read_bytes())
# Corrupt stored file bytes without changing central-directory CRC metadata.
with zipfile.ZipFile(root / "module.zip") as archive:
    entry = archive.getinfo("data/Added/large-original.bin")
    start = entry.header_offset + 30 + len(entry.filename.encode()) + len(entry.extra)
    corrupt[start + 20] ^= 0x40
(root / "bad/crc.zip").write_bytes(corrupt)
(root / "bad/truncated.zip").write_bytes((root / "module.zip").read_bytes()[:-12])
(root / "bad/corrupt.gz").write_bytes(b"not a gzip archive")

class QuietServer(http.server.ThreadingHTTPServer):
    def handle_error(self, *_):
        pass  # Intentional certificate/cancellation failures reset sockets.

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def do_GET(self):
        if self.path.startswith("/redirect/") or self.path.startswith("/downgrade/"):
            scheme = "http" if self.path.startswith("/downgrade/") else "https"
            self.send_response(302)
            port = self.server.plain_port if scheme == "http" else self.server.server_port
            self.send_header("Location", f"{scheme}://127.0.0.1:{port}/catalog/mods.d.tar.gz")
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        if self.path.startswith("/notfound/"):
            self.send_error(404)
            return
        body = (root / ("catalog.tar.gz" if self.path.endswith("mods.d.tar.gz") else "module.zip")).read_bytes()
        if self.path.startswith("/corrupt/"):
            body = b"original invalid archive"
        self.send_response(200)
        if not self.path.startswith("/chunked/"):
            self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        try:
            if self.path.startswith("/slow/"):
                for at in range(0, len(body), 1024):
                    self.wfile.write(body[at:at + 1024])
                    self.wfile.flush()
                    time.sleep(0.02)
            else:
                self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError, ssl.SSLError):
            pass

plain = QuietServer(("127.0.0.1", 0), Handler)
plain_thread = threading.Thread(target=plain.serve_forever, daemon=True)
plain_thread.start()
server = QuietServer(("127.0.0.1", 0), Handler)
server.plain_port = plain.server_port
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(root / "certificate.pem", root / "key.pem")
server.socket = context.wrap_socket(server.socket, server_side=True)
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    env = os.environ.copy()
    env["NO_PROXY"] = "127.0.0.1,localhost"
    result = subprocess.run([client[0] if client else binary, str(root), str(server.server_port)], env=env)
    raise SystemExit(result.returncode)
finally:
    plain.shutdown()
    plain.server_close()
    plain_thread.join()
    server.shutdown()
    server.server_close()
    thread.join()
