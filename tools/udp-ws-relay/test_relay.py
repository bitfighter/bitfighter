#!/usr/bin/env python3
"""LAN checks for the relay allowlist. No public game servers."""

import asyncio
import os
import socket
import struct
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import relay  # noqa: E402

BIND = relay.BIND
SEND = relay.SEND
ALLOW = relay.ALLOW
HERE = os.path.dirname(os.path.abspath(__file__))
RELAY_PORT = 28771


def announcement(pairs):
    body = struct.pack("!2sBH", b"\xbf\xbf", 1, len(pairs))
    for ip, port in pairs:
        body += struct.pack("!IH", ip, port)
    return body


def ip_of(host):
    return struct.unpack("!I", socket.inet_aton(host))[0]


class Ws:
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer

    async def send(self, payload):
        mask = os.urandom(4)
        header = bytes([0x82, 0x80 | len(payload)])
        masked = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
        self.writer.write(header + mask + masked)
        await self.writer.drain()

    async def recv(self):
        hdr = await self.reader.readexactly(2)
        ln = hdr[1] & 0x7F
        if ln == 126:
            ln = struct.unpack("!H", await self.reader.readexactly(2))[0]
        data = b""
        while len(data) < ln:
            data += await self.reader.readexactly(ln - len(data))
        return data


async def connect():
    reader, writer = await asyncio.open_connection("127.0.0.1", RELAY_PORT)
    key = "dGhlIHNhbXBsZSBub25jZQ=="
    writer.write(
        (
            "GET / HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Key: {key}\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n"
        ).encode()
    )
    await writer.drain()
    buf = b""
    while b"\r\n\r\n" not in buf:
        buf += await reader.read(1024)
    if b"101" not in buf.split(b"\r\n", 1)[0]:
        raise RuntimeError(buf[:80])
    return Ws(reader, writer)


def udp_bind(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", port))
    sock.setblocking(False)
    return sock


async def wait_packet(sock, timeout=0.6):
    loop = asyncio.get_running_loop()
    try:
        return await asyncio.wait_for(loop.sock_recv(sock, 2048), timeout)
    except asyncio.TimeoutError:
        return None


async def scenario():
    failures = []

    def check(name, cond):
        print(("ok  " if cond else "FAIL") + " " + name, flush=True)
        if not cond:
            failures.append(name)

    ws = await connect()
    await ws.send(struct.pack("!BIH", BIND, 1, 0))
    bound = await ws.recv()
    _kind, _sock, bound_port = struct.unpack("!BIH", bound[:7])

    # Static game port is allowed with no master involvement.
    game = udp_bind(28000)
    await ws.send(struct.pack("!BIIHH", SEND, 1, ip_of("127.0.0.1"), 28000, 4) + b"stat")
    check("static 28000 delivered", await wait_packet(game) == b"stat")

    # A non-listed port is dropped.
    other = udp_bind(28199)
    await ws.send(struct.pack("!BIIHH", SEND, 1, ip_of("127.0.0.1"), 28199, 4) + b"nope")
    check("unnamed 28199 dropped", await wait_packet(other) is None)

    # Client ALLOW before any master traffic is dropped.
    named = udp_bind(28123)
    await ws.send(struct.pack("!BIH", ALLOW, ip_of("127.0.0.1"), 28123))
    await asyncio.sleep(0.05)
    await ws.send(struct.pack("!BIIHH", SEND, 1, ip_of("127.0.0.1"), 28123, 4) + b"earl")
    check("allow before master dropped", await wait_packet(named) is None)

    # A clear announcement from the master port names 28123 only.
    master = udp_bind(25955)
    master.sendto(
        announcement([(ip_of("127.0.0.1"), 28123)]),
        ("127.0.0.1", bound_port),
    )
    # The announcement is also delivered to the client as a RECV.
    await ws.recv()
    await ws.send(struct.pack("!BIIHH", SEND, 1, ip_of("127.0.0.1"), 28123, 4) + b"list")
    check("announced 28123 delivered", await wait_packet(named) == b"list")
    await ws.send(struct.pack("!BIIHH", SEND, 1, ip_of("127.0.0.1"), 28199, 4) + b"nope")
    check("still dropped 28199", await wait_packet(other) is None)

    # An ALLOW for something the announcement did not name is refused.
    await ws.send(struct.pack("!BIH", ALLOW, ip_of("127.0.0.1"), 28199))
    await asyncio.sleep(0.05)
    await ws.send(struct.pack("!BIIHH", SEND, 1, ip_of("127.0.0.1"), 28199, 4) + b"nope")
    check("allow of unannounced port dropped", await wait_packet(other) is None)

    # Inbound source can be answered (punch reply).
    punch = udp_bind(28444)
    punch.sendto(b"punch", ("127.0.0.1", bound_port))
    await ws.recv()
    await ws.send(struct.pack("!BIIHH", SEND, 1, ip_of("127.0.0.1"), 28444, 4) + b"back")
    check("punch reply delivered", await wait_packet(punch) == b"back")

    # A bind that cannot get its port must not close the session or the
    # socket that already worked.
    busy = udp_bind(28991)
    await ws.send(struct.pack("!BIH", BIND, 2, 28991))
    closed = False
    got = None
    try:
        got = await asyncio.wait_for(ws.recv(), 0.4)
    except asyncio.TimeoutError:
        got = None
    except (asyncio.IncompleteReadError, ConnectionResetError):
        closed = True
    check("in-use bind did not succeed", got is None and not closed)
    check("session open after in-use bind", not closed)
    await ws.send(struct.pack("!BIIHH", SEND, 1, ip_of("127.0.0.1"), 28000, 5) + b"still")
    check("first socket still delivers", await wait_packet(game) == b"still")
    busy.close()

    for sock in (game, other, named, master, punch):
        sock.close()
    ws.writer.close()
    return failures


def main():
    proc = subprocess.Popen(
        [sys.executable, os.path.join(HERE, "relay.py"), str(RELAY_PORT)],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    try:
        # Wait until the process is listening.
        deadline = time.time() + 5
        while time.time() < deadline:
            try:
                socket.create_connection(("127.0.0.1", RELAY_PORT), 0.2).close()
                break
            except OSError:
                time.sleep(0.05)
        else:
            print(proc.stdout.read() if proc.stdout else "")
            raise SystemExit("relay did not listen")
        failures = asyncio.run(scenario())
    finally:
        proc.terminate()
        try:
            out, _ = proc.communicate(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()
            out, _ = proc.communicate()
        print("--- relay log ---")
        print(out)
    if failures:
        raise SystemExit("failed: " + ", ".join(failures))
    print("all relay allowlist checks passed")


if __name__ == "__main__":
    main()
