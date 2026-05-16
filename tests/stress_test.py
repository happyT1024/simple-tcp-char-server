#!/usr/bin/env python3
"""
Stress test: 10 TLS clients connect to the server, exchange messages,
and verify broadcast delivery semantics.

Server behaviour:
- "Server: X join" is broadcast only to clients already registered
  (user_is_ok). So client i receives joins from clients j where j >= i
  (including its own join).
- Messages from client i are broadcast to all registered clients, so
  every client receives all 9 messages from other clients.
"""

import socket
import ssl
import threading
import time
import subprocess
import sys
import os
import signal

HOST = "127.0.0.1"
PORT = 18443
NUM_CLIENTS = 10
TIMEOUT = 30
BUILD_DIR = "build"
SERVER_BIN = os.path.join(BUILD_DIR, "SimpleTCPChat")


def build_server() -> None:
    if os.path.exists(SERVER_BIN):
        return
    print("Building server...", flush=True)
    subprocess.check_call(
        ["cmake", "-DCMAKE_BUILD_TYPE=Release", "-B", BUILD_DIR]
    )
    subprocess.check_call(
        [
            "cmake", "--build", BUILD_DIR, "--target", "SimpleTCPChat",
            "-j", str(os.cpu_count() or 4),
        ]
    )


class TestClient(threading.Thread):
    def __init__(self, cid: int, host: str, port: int,
                 server_start_time: float) -> None:
        super().__init__()
        self.cid = cid
        self.nick = f"User-{cid}"
        self.host = host
        self.port = port
        self.server_start_time = server_start_time

        self.joins: set[str] = set()
        self.msgs: list[tuple[str, str]] = []
        self.error: str | None = None
        self._sock: ssl.SSLSocket | None = None
        self._running = True

    def run(self) -> None:
        buf = b""
        try:
            ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE

            raw = socket.create_connection((self.host, self.port),
                                           timeout=TIMEOUT)
            self._sock = ctx.wrap_socket(raw, server_hostname=self.host)
            self._sock.settimeout(TIMEOUT)
            self._sock.sendall(f"{self.nick}\n".encode())

            while self._running:
                try:
                    chunk = self._sock.recv(4096)
                except socket.timeout:
                    continue
                if not chunk:
                    break
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    line = line.decode("utf-8", errors="replace").strip("\r")
                    self._process_line(line)

        except Exception as e:
            self.error = str(e)
        finally:
            self._running = False
            if self._sock:
                try:
                    self._sock.close()
                except OSError:
                    pass

    def _process_line(self, line: str) -> None:
        if line.startswith("Server:"):
            rest = line[len("Server:"):].strip()
            if rest.endswith(" join"):
                nick = rest[:-len(" join")].strip()
                self.joins.add(nick)
        elif ": " in line:
            sep = line.index(": ")
            nick = line[:sep]
            msg = line[sep + 2:]
            if nick != self.nick:
                self.msgs.append((nick, msg))

    def send(self, text: str) -> None:
        if self._sock:
            self._sock.sendall(text.encode())

    def stop(self) -> None:
        self._running = False
        if self._sock:
            try:
                self._sock.close()
            except OSError:
                pass


def expected_joins(cid: int) -> set[str]:
    """Client with ID `cid` receives joins from clients j >= cid."""
    return {f"User-{j}" for j in range(cid, NUM_CLIENTS)}


def main() -> int:
    build_server()

    print(f"Starting server on {HOST}:{PORT}...", flush=True)
    srv = subprocess.Popen(
        [SERVER_BIN, "-p", str(PORT), "-l", "0"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        preexec_fn=lambda: signal.signal(signal.SIGTERM, signal.SIG_DFL),
    )
    time.sleep(2)

    if srv.poll() is not None:
        print("FAIL: server failed to start", flush=True)
        return 1

    server_start = time.monotonic()
    print("Server started.", flush=True)

    clients = [TestClient(i, HOST, PORT, server_start)
               for i in range(NUM_CLIENTS)]

    for c in clients:
        c.start()
        time.sleep(0.5)

    timeout_time = time.monotonic() + TIMEOUT

    while time.monotonic() < timeout_time:
        all_ready = True
        for c in clients:
            expected = expected_joins(c.cid)
            if not expected.issubset(c.joins):
                all_ready = False
                break
            if c.error:
                print(f"[ERROR] {c.nick}: {c.error}", flush=True)
                all_ready = False
        if all_ready:
            break
        time.sleep(0.1)
    else:
        print("TIMEOUT waiting for joins", flush=True)

    deadline = time.monotonic() + 10
    for c in clients:
        c.send(f"Hello from {c.nick}\n")

    while time.monotonic() < deadline:
        all_done = True
        for c in clients:
            if len(c.msgs) < NUM_CLIENTS - 1:
                all_done = False
                break
        if all_done:
            break
        time.sleep(0.1)

    for c in clients:
        c.stop()
    for c in clients:
        c.join(timeout=5)

    srv.terminate()
    try:
        srv.wait(timeout=5)
    except subprocess.TimeoutExpired:
        srv.kill()
        srv.wait()

    passed = 0
    failed = 0

    for c in clients:
        nick = c.nick
        if c.error:
            print(f"[FAIL] {nick}: {c.error}", flush=True)
            failed += 1
            continue

        exp_joins = expected_joins(c.cid)
        joins_ok = exp_joins.issubset(c.joins)
        expected_from = {f"User-{j}" for j in range(NUM_CLIENTS) if j != c.cid}
        received_from = {m[0] for m in c.msgs}
        msgs_ok = expected_from.issubset(received_from)

        if joins_ok and msgs_ok:
            print(f"[PASS] {nick}: {len(c.joins)} joins, "
                  f"{len(c.msgs)} msgs", flush=True)
            passed += 1
        else:
            if not joins_ok:
                missing = exp_joins - c.joins
                print(f"[FAIL] {nick}: missing joins {missing}", flush=True)
            if not msgs_ok:
                missing = expected_from - received_from
                print(f"[FAIL] {nick}: missing msgs from {missing}",
                      flush=True)
            failed += 1

    print(f"\n{'=' * 40}", flush=True)
    print(f"PASSED: {passed}/{NUM_CLIENTS}, FAILED: {failed}/{NUM_CLIENTS}",
          flush=True)
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
