#!/usr/bin/env python3
"""WebSocket <-> UDP relay for the Bitfighter browser client.

Emscripten's posix socket proxy needs pthreads and SharedArrayBuffer.
GitHub Pages does not send the cross-origin headers those require, so the
browser build speaks this small framing protocol instead. One WebSocket
carries every datagram for that page. Each browser socket is a real UDP
socket on this process; replies come back on the same WebSocket.

Frame layout, big-endian, one WebSocket binary message per frame:

  BIND  client -> relay
    u8  1
    u32 sock_id     chosen by the client
    u16 port        0 picks an ephemeral port
  BOUND relay -> client
    u8  1
    u32 sock_id
    u16 port        the port actually bound

  SEND  client -> relay
    u8  2
    u32 sock_id
    u32 ipv4
    u16 port
    u16 length
    u8  payload[length]
  RECV  relay -> client
    u8  3
    u32 sock_id
    u32 ipv4        source address
    u16 port
    u16 length
    u8  payload[length]

Outbound destinations are restricted. The static allowlist is the master
port and the default game port (RELAY_ALLOW_PORTS, default 25955,28000).
RELAY_ALLOW_ANY=1 turns filtering off; do not set that on a public host.

A session may also send to addresses the master named, for ten minutes:

  - A datagram from UDP port 25955 whose payload is a clear announcement
    (see parse_master_announcement) adds those IP:port pairs.
  - TNL encrypts real master RPC bodies, so the game client also sends an
    ALLOW frame for each address it decoded. That frame is honored only
    after this session has received a datagram from port 25955, and only
    for addresses in a parsed announcement when one was present.
  - The source of any inbound datagram is allowed, so a punch reply can
    be sent back to the peer that just wrote to this session.

  ALLOW client -> relay
    u8  4
    u32 ipv4
    u16 port

Usage:
  python3 tools/udp-ws-relay/relay.py [port]
Default listen port is 28080.
"""

import asyncio
import os
import struct
import sys
import time

BIND = 1
SEND = 2
RECV = 3
ALLOW = 4
MAX_PAYLOAD = 8192
MASTER_PORT = 25955
DYNAMIC_TTL = 600.0  # seconds; a listed server stays reachable for one browse

DEFAULT_PORTS = {25955, 28000}


def allowed_ports():
    if os.environ.get("RELAY_ALLOW_ANY") == "1":
        return None
    raw = os.environ.get("RELAY_ALLOW_PORTS", "25955,28000")
    ports = set()
    for part in raw.split(","):
        part = part.strip()
        if part:
            ports.add(int(part))
    return ports or DEFAULT_PORTS


ALLOW_PORTS = allowed_ports()


def host_of(ip):
    return ".".join(str((ip >> shift) & 0xFF) for shift in (24, 16, 8, 0))


def fmt_addr(ip, port):
    return f"{host_of(ip)}:{port}"


def parse_master_announcement(data):
    """Clear announcement a test (or a future clear reply) can send.

    Layout, big-endian: u16 0xbfbf, u8 version 1, u16 count,
    then count times (u32 ipv4, u16 port). Real master RPCs are
    encrypted and do not match this, so they are ignored here.
    """
    if len(data) < 5 or data[0] != 0xBF or data[1] != 0xBF or data[2] != 1:
        return []
    count = struct.unpack("!H", data[3:5])[0]
    need = 5 + count * 6
    # Exact length, so an encrypted master datagram cannot accidentally match.
    if count > 64 or count < 1 or len(data) != need:
        return []
    found = []
    off = 5
    for _ in range(count):
        ip, port = struct.unpack("!IH", data[off:off + 6])
        off += 6
        if ip and port:
            found.append((ip, port))
    return found


def ws_accept(key: str) -> str:
    import base64
    import hashlib
    guid = b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    digest = hashlib.sha1(key.encode("ascii") + guid).digest()
    return base64.b64encode(digest).decode("ascii")


class Client:
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer
        self.socks = {}  # sock_id -> transport
        self.dynamic = {}  # (ip, port) -> monotonic expiry
        self.saw_master = False
        self.master_named = set()

    def send_frame(self, payload: bytes):
        n = len(payload)
        header = bytearray()
        header.append(0x82)  # FIN + binary
        if n < 126:
            header.append(n)
        elif n < 65536:
            header.append(126)
            header += struct.pack("!H", n)
        else:
            header.append(127)
            header += struct.pack("!Q", n)
        self.writer.write(bytes(header) + payload)

    async def read_exact(self, n):
        buf = bytearray()
        while len(buf) < n:
            chunk = await self.reader.read(n - len(buf))
            if not chunk:
                raise EOFError
            buf += chunk
        return bytes(buf)

    async def read_frame(self):
        b0, b1 = await self.read_exact(2)
        opcode = b0 & 0x0F
        masked = (b1 & 0x80) != 0
        length = b1 & 0x7F
        if length == 126:
            length = struct.unpack("!H", await self.read_exact(2))[0]
        elif length == 127:
            length = struct.unpack("!Q", await self.read_exact(8))[0]
        mask = await self.read_exact(4) if masked else b""
        data = bytearray(await self.read_exact(length)) if length else bytearray()
        if masked:
            for i in range(length):
                data[i] ^= mask[i % 4]
        return opcode, bytes(data)


def remember(client, ip, port, reason):
    if not ip or not port:
        return
    key = (ip, port)
    now = time.monotonic()
    fresh = client.dynamic.get(key, 0) <= now
    client.dynamic[key] = now + DYNAMIC_TTL
    if fresh:
        print(f"allow {fmt_addr(ip, port)} ({reason})", flush=True)


def destination_allowed(client, ip, port):
    if ALLOW_PORTS is None:
        return True
    if port in ALLOW_PORTS:
        return True
    exp = client.dynamic.get((ip, port))
    if exp is None:
        return False
    if exp <= time.monotonic():
        del client.dynamic[(ip, port)]
        return False
    return True


def note_from_master(client, data):
    client.saw_master = True
    named = parse_master_announcement(data)
    for ip, port in named:
        client.master_named.add((ip, port))
        remember(client, ip, port, "master announcement")


def note_inbound(client, ip, port, data):
    # A peer that already wrote to this session can be answered (NAT punch).
    remember(client, ip, port, "inbound source")
    if port == MASTER_PORT:
        note_from_master(client, data)


def honor_allow(client, ip, port):
    key = (ip, port)
    if key in client.master_named:
        remember(client, ip, port, "client confirm of master announcement")
        return
    if client.master_named:
        print(f"drop allow {fmt_addr(ip, port)} (not in this session's master announcement)", flush=True)
        return
    if not client.saw_master:
        print(f"drop allow {fmt_addr(ip, port)} (no master traffic this session)", flush=True)
        return
    # Real master RPC bodies are encrypted, so the decoded address arrives
    # from the game client. The session still had to hear the master first.
    remember(client, ip, port, "client report after master traffic")


class UDP(asyncio.DatagramProtocol):
    def __init__(self, client, sock_id):
        self.client = client
        self.sock_id = sock_id

    def datagram_received(self, data, addr):
        if ":" in addr[0]:
            return  # IPv6 replies are not part of this PoC
        ip = struct.unpack("!I", bytes(map(int, addr[0].split("."))))[0]
        note_inbound(self.client, ip, addr[1], data)
        frame = struct.pack("!BIIHH", RECV, self.sock_id, ip, addr[1], len(data)) + data
        try:
            self.client.send_frame(frame)
        except Exception:
            pass


async def bind_socket(client, sock_id, port):
    loop = asyncio.get_running_loop()

    def factory():
        return UDP(client, sock_id)

    try:
        transport, _protocol = await loop.create_datagram_endpoint(
            factory, local_addr=("0.0.0.0", port)
        )
    except OSError as exc:
        # EADDRINUSE / EACCES must not drop the WebSocket. Other sockets on
        # this client keep their UDP ports.
        print(f"bind sock={sock_id} requested={port} failed: {exc}", flush=True)
        return
    bound = transport.get_extra_info("sockname")[1]
    client.socks[sock_id] = transport
    client.send_frame(struct.pack("!BIH", BIND, sock_id, bound))
    print(f"bind sock={sock_id} requested={port} actual={bound}", flush=True)


async def handle_message(client, payload):
    if not payload:
        return
    kind = payload[0]
    if kind == BIND and len(payload) >= 7:
        sock_id, port = struct.unpack("!IH", payload[1:7])
        if sock_id in client.socks:
            client.socks[sock_id].close()
            del client.socks[sock_id]
        await bind_socket(client, sock_id, port)
    elif kind == SEND and len(payload) >= 13:
        sock_id, ip, port, length = struct.unpack("!IIHH", payload[1:13])
        data = payload[13:13 + length]
        if not destination_allowed(client, ip, port):
            print(f"drop send to {fmt_addr(ip, port)} (not allowed)", flush=True)
            return
        transport = client.socks.get(sock_id)
        if transport is None or len(data) != length:
            return
        print(f"send sock={sock_id} {fmt_addr(ip, port)} {len(data)} bytes", flush=True)
        transport.sendto(data, (host_of(ip), port))
    elif kind == ALLOW and len(payload) >= 7:
        ip, port = struct.unpack("!IH", payload[1:7])
        honor_allow(client, ip, port)


async def connection(reader, writer):
    peer = writer.get_extra_info("peername")
    try:
        req = b""
        while b"\r\n\r\n" not in req:
            chunk = await reader.read(1024)
            if not chunk:
                return
            req += chunk
            if len(req) > 8192:
                return
        text = req.decode("latin1", "replace")
        if "Upgrade: websocket" not in text and "Upgrade: WebSocket" not in text:
            writer.close()
            return
        key = ""
        for line in text.split("\r\n"):
            if line.lower().startswith("sec-websocket-key:"):
                key = line.split(":", 1)[1].strip()
        if not key:
            writer.close()
            return
        accept = ws_accept(key)
        writer.write(
            (
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                f"Sec-WebSocket-Accept: {accept}\r\n\r\n"
            ).encode("ascii")
        )
        await writer.drain()
        client = Client(reader, writer)
        print(f"websocket from {peer}", flush=True)
        while True:
            opcode, data = await client.read_frame()
            if opcode == 0x8:
                break
            if opcode == 0x9:  # ping
                # pong, unmasked
                n = len(data)
                writer.write(bytes([0x8A, n]) + data)
                continue
            if opcode == 0x2 or opcode == 0x0:
                await handle_message(client, data)
    except (asyncio.IncompleteReadError, EOFError, ConnectionResetError):
        pass
    finally:
        if "client" in dir():
            for transport in client.socks.values():
                transport.close()
        writer.close()
        print(f"closed {peer}", flush=True)


async def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 28080
    server = await asyncio.start_server(connection, "0.0.0.0", port)
    ports = "any" if ALLOW_PORTS is None else ",".join(str(p) for p in sorted(ALLOW_PORTS))
    print(f"udp-ws-relay listening on ws://0.0.0.0:{port}/  allow_ports={ports}", flush=True)
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(main())
