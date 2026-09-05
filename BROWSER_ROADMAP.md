# Bitfighter → Browser (WASM) Porting Roadmap

This document outlines the work required to cross-compile the Bitfighter game client to WebAssembly using Emscripten, allowing it to run in a browser. The master server and its MySQL/database infrastructure are out of scope.

---

## Dependency Compatibility Summary

| Library | In-tree? | WASM compatibility | Notes |
|---|---|---|---|
| **SDL2** | No (system) | ✅ Emscripten port exists | Window, input, GL context |
| **OpenGL 2.1 / GLAD** | GLAD in-tree | ✅ via GLES2 / WebGL | Disable GLAD loader |
| **OpenAL + ALURE** | ALURE in-tree | ⚠️ Partial | Emscripten emulates OpenAL; no capture |
| **OGG/Vorbis** | No (system) | ✅ Pure C | Recompile for WASM |
| **ModPlug** | No (system) | ✅ C++ | Recompile for WASM |
| **Speex (voice)** | No (system) | ⚠️ Disable | OpenAL capture not available in browsers |
| **libpng** | No (system) | ✅ Pure C | Recompile for WASM |
| **LuaJIT** | In-tree (`lua/luajit/`) | ❌ JIT incompatible | Replace with PUC Lua 5.1 |
| **libtomcrypt** | In-tree (`tomcrypt/`) | ✅ Pure C | Recompile |
| **SQLite3** | In-tree (`sqlite/`) | ✅ Single-file C | Recompile; use IDBFS for persistence |
| **Boost headers** | In-tree (`boost/`) | ✅ Header-only | No change needed |
| **Clipper** | In-tree (`clipper/`) | ✅ Pure C++ | Recompile |
| **Poly2tri** | In-tree (`poly2tri/`) | ✅ Pure C++ | Recompile |
| **Recast** (pathfinding) | In-tree (`recast/`) | ✅ Pure C++ | Recompile |
| **TNL** | In-tree (`tnl/`) | ❌ Raw UDP sockets | Largest blocker; needs WebRTC or WebSocket transport shim |
| **Discord RPC** | In-tree (`discord-rpc/`) | ❌ Native IPC | Disable with `-DDISCORD=OFF` |
| **fontstash / stb_truetype** | In-tree | ✅ Pure C | Recompile |

---

## Blockers by Severity

| Issue | Severity | Phase |
|---|---|---|
| Raw UDP sockets (TNL) | **Critical** | 6 |
| LuaJIT JIT compiler | **Critical** | 2 |
| Blocking main loop | Medium | 4 |
| GLAD GL loader | Low | 1 |
| Voice capture (Speex/OpenAL) | Low | 5 |
| Discord RPC | Trivial | 1 |

---

## Phased Plan

### Phase 1 — Build System Foundation

**Goal**: get the project to compile with Emscripten, even if nothing runs yet.

- Add `cmake/Platform/Emscripten.cmake` analogous to the existing `Win32.cmake` / `Linux.cmake`
- Add a new platform define `TNL_OS_EMSCRIPTEN` in `tnl/tnlTypes.h` / `tnlPlatform.h` and propagate it through the socket/thread `#ifdef` chains
- Set default CMake flags for Emscripten builds:
  - `USE_GLES=ON`
  - `NO_THREADS=ON`
  - `DISCORD=OFF`
  - `USE_LEGACY_GL=OFF`
  - `LUAJIT_BUILTIN=OFF`
- Disable the GLAD GL function-pointer loader (Emscripten resolves GL symbols statically at link time)
- Exclude platform-specific blocks that don't apply:
  - Windows updater (`USE_GUP` / `ShellExecuteA`)
  - `#ifdef WIN32` and `#ifdef TNL_OS_WIN32` blocks in `main.cpp`, `udp.cpp`, `thread.cpp`, `platform.cpp`
  - `#ifdef TNL_OS_MAC_OSX` / `IOS` blocks

**Effort**: Low–medium. Mostly CMake and `#ifdef` work.

---

### Phase 2 — Replace LuaJIT with PUC Lua 5.1

**Goal**: eliminate the hardest compile-time blocker.

LuaJIT's JIT compiler generates and executes native machine code at runtime. WebAssembly explicitly prohibits writable+executable memory pages (W^X), so **LuaJIT cannot be compiled to WASM** in JIT mode, and its interpreter-only mode is not reliably supported.

- Wire in PUC Lua 5.1 as the fallback when `LUAJIT_BUILTIN=OFF` (the CMake path for this partially exists already)
- Audit bot and level scripts for LuaJIT-specific extensions (`bit` library, `ffi`, `jit.*`) and polyfill or remove them
- PUC Lua's C API is compatible enough that the `zap/` Lua binding layer should require only minor changes

**Effort**: Medium. Most game scripts are expected to be interpreter-compatible.

---

### Phase 3 — Rendering

**Goal**: confirm the GL2 modern renderer works via WebGL.

The modern `GL2Renderer` path already uses VBOs, custom vertex/fragment shaders, and software matrix math — it maps almost 1:1 to **WebGL 1.0 (GLES2)**. Emscripten translates GLES2 API calls to WebGL automatically.

- Enable `USE_GLES=ON` to activate the `BF_USE_GLES` compile path
- Audit `glVertexAttribPointer` / `glBufferData` calls for stride/offset correctness (WebGL is stricter than desktop OpenGL about these)
- SDL2's Emscripten port handles window creation and `SDL_GL_SwapWindow` — no changes needed there
- Do **not** use the legacy renderer path (`USE_LEGACY_GL`); the fixed-function pipeline has no WebGL equivalent

**Effort**: Low. This is the most favorable part of the codebase for porting.

---

### Phase 4 — Main Loop Restructure

**Goal**: make the game loop compatible with the browser's event model.

The current loop in `main.cpp` is a blocking `for(;;) idle()`. Browsers do not allow blocking indefinitely on the main thread — doing so freezes the tab.

- Under `#ifdef __EMSCRIPTEN__`, replace the blocking loop with:
  ```cpp
  emscripten_set_main_loop(idle, 0, 1);
  ```
- The existing `idle()` function already polls SDL events and returns promptly, making this a near-ideal fit for Emscripten's main loop model

**Effort**: Very low. Likely a handful of lines guarded by `#ifdef __EMSCRIPTEN__`.

---

### Phase 5 — Audio

**Goal**: get in-game sound effects and music working.

- **Sound effects (OpenAL via ALURE)**: Emscripten emulates OpenAL via the Web Audio API — should work with minimal changes
- **Music (OGG/Vorbis, ModPlug)**: both are pure C/C++ and will recompile for WASM; alternatively, defer music to browser `<audio>` elements for simplicity
- **Voice chat**: disable entirely under Emscripten using the existing `BF_NO_VOICECHAT` mechanism. OpenAL capture (`alcCaptureOpenDevice`) is not available in browsers. The `TNL_OS_MOBILE` guard already stub-outs voice recording — a similar `TNL_OS_EMSCRIPTEN` guard can reuse this path.

**Effort**: Low–medium.

---

### Phase 6 — Networking

**Goal**: restore multiplayer (or provide a functional single-player fallback first).

This is the single largest architectural challenge. TNL (`tnl/`) is built on **raw UDP sockets** (`socket()` / `sendto()` / `recvfrom()`), which browsers cannot open. The GhostConnection and EventConnection protocols that sync game objects run on top of this transport.

#### Phase 6a — Single-player with bots (Low effort)

Disable all networking. The game already supports local bot matches. This proves Phases 1–5 are working before tackling the networking problem.

#### Phase 6b — WebSocket relay proxy (Medium effort, recommended next step)

A lightweight server-side relay process translates between **WebSocket** (what the browser can open) and **UDP** (what the game server expects). The TNL protocol above the transport layer is preserved intact.

- Replace `tnl/udp.cpp` and the socket layer in `tnl/netInterface.cpp` with a WebSocket client under `TNL_OS_EMSCRIPTEN`, using Emscripten's WebSocket API or a small C library
- The relay proxy can be a simple Node.js / Go / Python process running alongside the game server
- Adds a server-side infrastructure component but avoids touching GhostConnection/EventConnection

#### Phase 6c — WebRTC data channels (High effort, best long-term quality)

WebRTC unreliable ordered data channels are the closest browser analogue to UDP. This would give the best latency and eliminate the relay dependency, but requires:

- STUN/ICE/TURN infrastructure for NAT traversal
- A signaling server for session establishment
- Replacing `tnl/udp.cpp` and `tnl/netInterface.cpp` with a WebRTC transport shim (e.g. using libdatachannel compiled to WASM, or a JS-side WebRTC bridge)

**Secondary concern**: `HttpsRequest.cpp` (used for level database downloads) also uses TNL raw sockets. Replace with `emscripten_fetch()` under `#ifdef __EMSCRIPTEN__`.

---

### Phase 7 — Filesystem & Persistence

**Goal**: handle file I/O for settings, levels, and SQLite databases.

- Bundle read-only assets (levels, resources, fonts) at compile time using Emscripten's `--preload-file` mechanism
- User settings and SQLite databases should use **IDBFS** (Emscripten's IndexedDB-backed virtual filesystem) so data persists across sessions
- Add an `#ifdef __EMSCRIPTEN__` branch in the `Platform::*` file-path helper functions to return appropriate virtual paths

**Effort**: Medium. Mostly mechanical substitution of path and file-open logic.

---

### Phase 8 — Threading (optional improvement)

**Goal**: restore async DNS resolution and database operations.

Initially disabled via `NO_THREADS=ON`, which activates `TNL_NO_THREADS` and makes all thread operations synchronous stubs. This is functional but means DNS lookups and SQLite writes block the main thread briefly.

Emscripten supports pthreads via Web Workers + `SharedArrayBuffer`, but the hosting server must send the `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp` HTTP headers. This can be deferred until the rest of the port is stable.

**Effort**: Medium, plus server configuration.

---

## Minimum Viable Browser Build

Completing **Phases 1–6a** produces a playable single-player browser build (local matches with bots, full rendering and audio). This is the recommended first milestone.

**Phase 6b** (WebSocket relay) then restores multiplayer with the smallest possible change to the TNL codebase.
