# Bitfighter in the browser (Emscripten proof of concept)

This is a minimal WebAssembly build of the Bitfighter client. Native Linux, Windows, and macOS builds are unchanged: every difference is behind `if(EMSCRIPTEN)` or `#ifdef __EMSCRIPTEN__`.

It loads the main menu, plays sound, and can host a local game (the client and server in the same page talk through `NetConnection::connectLocal`, not UDP). With the default relay it also talks to the master server and to game servers the master has named, on any port.

## What you need

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) (tested with 7.0.0)
- CMake 3.10+ (invoked via `emcmake`)
- Python 3, only to serve the files

```
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

## Build

From the repository root, with `emcc` on `PATH`:

```
mkdir -p build/emscripten
cd build/emscripten
emcmake cmake ../.. -DCMAKE_BUILD_TYPE=Release
emmake make -j$(nproc)
```

`emcmake` points CMake at Emscripten's toolchain. `emmake` is not strictly required for this Makefile generator, but it keeps `emcc` available to the port builds.

The link writes these files into `exe/`:

- `bitfighter.html`
- `bitfighter.js`
- `bitfighter.wasm`
- `bitfighter.data` (the `resource/` tree, preloaded at `/bitfighter`)

## Run

The page has to be served over HTTP. Opening the html file from disk will not load the wasm.

```
cd exe
python3 -m http.server 8080
```

Open `http://127.0.0.1:8080/bitfighter.html`.

A copy of this build is published from the `emscripten-poc` branch by `.github/workflows/emscripten-pages.yml` to <https://elementc.github.io/bitfighter/>. That workflow runs only on pushes to `emscripten-poc` (and when started by hand).

The page has a **Fullscreen** button. The in-game display-mode control calls `SDL_SetWindowFullscreen`, and SDL's Emscripten backend turns that into `emscripten_request_fullscreen_strategy`. The browser only accepts either request from a click or a key. The canvas CSS is `width: 100%` and a height that fills the space under the toolbar, so the picture tracks the window.

## UDP relay

Browsers cannot open raw UDP sockets. Emscripten's `websocket_to_posix_proxy` (`-sPROXY_POSIX_SOCKETS`) does bridge POSIX sockets, but it waits on a futex and needs pthreads plus `SharedArrayBuffer`. GitHub Pages does not send the cross-origin isolation headers that requires, and that mode also fights `emscripten_set_main_loop`. The browser build therefore uses a small relay, `tools/udp-ws-relay/relay.py`.

One WebSocket carries every datagram. Each browser socket is a real UDP socket on the relay. The game code changes only `Socket` send/receive. With the relay disabled, sockets stay on the failing POSIX path and a local game still uses `connectLocal`.

The browser build connects to `wss://bitfighter-web-relay.spacewalkpublishing.us` unless told otherwise. That relay is self-hosted. It always allows UDP ports 25955 and 28000, and for ten minutes it also allows addresses the master named for that browser. The page opens a throwaway WebSocket as soon as the HTML runs. The game then opens its own socket and retries that for 90 seconds. `?relay=wss://other.example/` or `?relay=ws://127.0.0.1:28080` replaces the default. `?relay=`, `?relay=off`, or `?relay=0` disables it. `Module.relay` is only used when the query parameter is absent.

The browser's master list is `IP:107.172.71.173:25955` first. Hostnames are not resolved, and a rejected hostname does not spend the 10 second master retry when another entry remains. JOIN's "gave up on the master" clock starts when the relay socket opens, not when the game starts, and is still 8 seconds after that. Native builds keep the hostname-first list and the original clock.

The browser seeds TNL's random pool from `crypto.getRandomValues` before any nonce or player id is drawn. The old seed was the player name plus `Platform::getRealMilliseconds()`, and that clock is milliseconds since the page started. It was 0 at startup, so every reload produced the same player id and the master rejected the new session as a duplicate. Connection nonces and keys use the same pool.

```
python3 tools/udp-ws-relay/relay.py 28080
```

A local relay is `http://127.0.0.1:8080/bitfighter.html?relay=ws://127.0.0.1:28080`. `?master=host:port` replaces the master list (a LAN test passes a name that does not resolve, so the page never opens a public connection). The hosted page is HTTPS, so a relay it can reach has to be `wss://`. An open `ws://` URL from that page is mixed content and the browser blocks it.

`.github/workflows/emscripten-pages.yml` runs that LAN path on every push: native `bitfighterd` on `127.0.0.1:28123` with `-serverpassword` and `-master IP:127.0.0.1:9`, `relay.py` on port 28080, and headless Chromium. A datagram from port 25955 names that address before the client connects. The test types a wrong password (the client logs `Server requires a password` again and the server does not record a join), then the real password, and requires `WebPilot [...] joined` in `bitfighter_server.log`. `test_relay.py` also checks that a port the master did not name is dropped. The Pages deploy does not wait on this job.

Outbound destination ports default to `25955` (master) and `28000` (game). Override with `RELAY_ALLOW_PORTS=25955,28000,28001` or `RELAY_ALLOW_ANY=1`. The default allowlist is intentional: without it the process is an open UDP proxy. Do not put an unrestricted relay on the public internet.

```
docker build -t bitfighter-udp-ws-relay tools/udp-ws-relay
docker run --rm --network host -e RELAY_ALLOW_PORTS=25955,28000 bitfighter-udp-ws-relay
```

What the relay host needs:

- A TCP port for the WebSocket, and TLS if browsers load the HTTPS Pages build.
- Outbound UDP, and a NAT that accepts the replies to those sockets. Client-to-server play only needs that.
- For other people to send *to* the browser (the browser hosting a public game, or an arranged peer connection), the relay's UDP ports have to be reachable from the internet. `--network host` is the straightforward way.

What works: the client talks to the real master at `107.172.71.173:25955` (the hostname entries are skipped, because Emscripten's `getaddrinfo` returns a fake address) and joins a server the master named, on whatever port that listing or arranged-connection reply uses. A private `bitfighterd` on a non-28000 port is reachable once a master announcement names it. The static list still allows 25955 and 28000 with no announcement.

The relay does not read real master RPC bodies: TNL encrypts them. After a datagram arrives from UDP port 25955, the browser tells the relay each address it decoded (`ALLOW` frame), and that address is permitted for ten minutes. A clear announcement (magic `0xbfbf`, version 1, then IP:port pairs) from port 25955 is parsed directly and limits later `ALLOW` frames to those pairs; `tools/udp-ws-relay/test_relay.py` uses that. The source of any inbound datagram is also permitted, so a punch reply can be sent back. Anything else is dropped. There is no new environment variable. `RELAY_ALLOW_ANY=1` is still an open proxy and must not be set on the public relay.

What does not: two browsers still only see each other as the relay's address, not their own. A destination the master never named, and that has not written to this session, is dropped.

The first screen is the nickname entry, as on native. A name saved in the INI is filled in on the next visit. `?name=WebPilot` skips the screen (the LAN test does this). Click the canvas, then:

- Click **HOST GAME**, then **START HOSTING** (or press Enter on that row)
- `WASD` moves, the mouse aims and fires, `Esc` opens the in-game menu
- `H` is the global help binding, not the host shortcut

`ROBOTS` on the host menu is where bot counts live. The startup log may say the UDP socket failed to set its buffer sizes. That is the browser socket layer. A local game still connects through `NetConnection::connectLocal`.

Settings and the user INI live in `/persist`, an IndexedDB filesystem mounted before `main` and flushed when the INI is written and when the page unloads. Assets stay in the preloaded `/bitfighter` tree.

## Main loop

`dedicatedServerLoop()` is `for(;;) idle();` on native targets. On Emscripten it is `emscripten_set_main_loop(bitfighter_emscripten_main_loop, 0, 1)`, so the browser calls the same `idle()` from `requestAnimationFrame`. The `1` keeps `main()` from returning, which matches the native loop. `Platform::sleep()` is a no-op here because a blocking sleep cannot yield to the browser without Asyncify.

Asyncify was rejected. It rewrites every function that can reach `Platform::sleep()`, makes the wasm much larger, and conflicts with the C++ exceptions used while Lua is starting up.

## Disabled or stubbed

| Piece | What the browser build does | Why |
| --- | --- | --- |
| LuaJIT | Official Lua 5.1.5, downloaded by CMake at configure time | The vendored LuaJIT 2.0.5 generates machine code with dynasm and has no WebAssembly backend. The game's C API and scripts are Lua 5.1. The tarball is `https://www.lua.org/ftp/lua-5.1.5.tar.gz` pinned by SHA256, not copied into the git tree. |
| Threads | `-DNO_THREADS=ON` (`TNL_NO_THREADS`) | pthreads need `SharedArrayBuffer` and cross-origin isolation headers. One-shot threads run inline. `DatabaseAccessThread` drains its queue once instead of looping forever, which would lock the tab. |
| Master server | UDP via the WebSocket relay, numeric address first | Emscripten's `getaddrinfo` returns a fake address, so hostname entries are skipped. The numeric master is first in the browser list. JOIN waits out a cold relay before it reports a master failure. |
| UDP to other games | WebSocket relay | Raw UDP does not exist in the browser. Ports 25955 and 28000 are always allowed. Other ports are allowed for ten minutes when the master named them, or when that peer has already sent a packet to the session. |
| Audio | OpenAL (`-lopenal`) and in-tree ALURE, with the Ogg/Vorbis ports | Sound effects are wav files loaded straight into OpenAL. Menu and credits music are ogg. A click or key resumes the WebAudio context (autoplay policy). |
| Voice chat (Speex) | Speex 1.2.1, fetched at configure time | Encode and decode match native. The microphone is the OpenAL capture device, which asks for `getUserMedia` on the voice-chat key. A headless browser has no microphone, so capture fails there and playback of received voice still works. |
| Tracker music (ModPlug, `.xm`) | Emscripten libmodplug port, ALURE decoder | Game tracks are `.xm`. The same `createDecoder` path used for ogg reads them. `menu.ogg` and `credits.ogg` are unchanged. |
| Discord RPC | `DISCORD=OFF` | The RPC transport is a local socket. |
| Test suite | Not built | gtest's pthread setup fights the single-threaded build. Native tests are unchanged. |

## Still native

`if(EMSCRIPTEN)` is checked before the UNIX/Linux platform file, so a normal `cmake .. && make` does not see these flags, Lua 5.1, or the main-loop change.
