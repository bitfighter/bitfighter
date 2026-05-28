# Bitfighter Networking Data Flow

This document explains how gameplay data moves from a local player to the server and then out to other players, with focus on the classes that are easiest to confuse (`Move`, `Ship`, `MoveItem`, and the three `ClientInfo` variants).

It is a **data-flow and architecture** guide, not a low-level protocol guide.

## Big picture: three networking channels used together

Bitfighter uses three complementary patterns:

1. **Input stream (client -> server) for controlled objects**
   - The local client samples input into a `Move` each frame.
   - `ControlObjectConnection` batches and sends pending `Move`s in its packet stream.
   - Server replays those moves on the authoritative `Ship`.

2. **Ghost state replication (mostly server -> clients) for world objects**
   - World objects (including `Ship`, `MoveItem` subclasses, flags, etc.) are replicated as TNL ghosts.
   - `packUpdate()`/`unpackUpdate()` send only changed fields using mask bits.
   - Scoping decides which objects each client currently receives.

3. **RPC/event stream (mostly server -> clients, some client -> server)**
   - Used for discrete game events and metadata (join/leave, team changes, auth, busy state, messages, score updates, etc.).
   - Mostly declared on `GameConnection` and `GameType`.

A single gameplay moment usually touches all three: input stream drives simulation, ghosting sends resulting world state, RPCs send discrete announcements/metadata.

---

## Core classes and responsibilities

## `Move` (intent from a player or bot)

`Move` is a compact command snapshot:

- Movement axes (`x`, `y`)
- Aim angle
- Fire button
- Module primary/secondary toggles
- Time delta

`Move` is **not** a world object. It is player intent. It has `pack()`/`unpack()` helpers for compact transmission and `prepare()` to force client/server rounding consistency before prediction/replay.

## `Ship` (authoritative controllable world object)

`Ship` is a ghosted world object and simulation entity:

- Holds gameplay state (position, velocity, health, energy, loadout, timers, mounted items, etc.)
- Executes movement and firing logic from current `Move`
- Implements ghost replication (`packUpdate()`/`unpackUpdate()`)
- Implements control-state snapshots (`writeControlState()`/`readControlState()`) used for correction/replay

Important distinction:

- `Move` says what a player tried to do.
- `Ship` is what actually happened in simulation.

## `MoveItem` (networked moving items, not player input)

`MoveItem` is a subclass of `MoveObject` for moving world items (flags/resources/etc.).

- It uses ghost replication of position/velocity.
- It is unrelated to player-input `Move` except name similarity.

If you are debugging player control flow, `MoveItem` is usually not part of that path.

## `ClientInfo` family (identity/player metadata views)

`ClientInfo` base stores player metadata (name, team, score, auth flags, badges, spawn delay state, stats, ship pointer, etc.).

There are three practical forms in play:

1. **Client-side local `FullClientInfo`** (`ClientGame::mClientInfo`)
   - Represents local player preferences/identity from client perspective.
   - Exists even before full in-game synchronization completes.

2. **Server-side `FullClientInfo`** (one per connected player/bot)
   - Authoritative metadata used by server simulation and policy decisions.
   - For human clients, linked to a `GameConnection`.

3. **Client-side `RemoteClientInfo`** (in client game player list, including local player mirror)
   - Created from server RPC sync (`GameType::s2cAddClient`).
   - Represents server-approved view of each player for scoreboard/UI/gameplay metadata.
   - Client keeps a pointer to "my remote mirror" (`mLocalRemoteClientInfo`) for the local player’s server-synced in-game record.

This local/full vs remote/mirrored split is a major source of confusion and is intentional.

---

---

## Glossary

### Ghosts

A **ghost** is a client-side copy of a server-side object, managed by TNL's ghost system. The server is the only place where game objects truly exist and run their authoritative simulation. Each connected client receives a subset of those objects as ghosts — lightweight replicas that receive state updates but do not simulate independently (except for client-side interpolation and local-player prediction).

The lifecycle:
1. **Ghost create** — when an object enters a client's scope, TNL calls `packUpdate()` with `isInitialUpdate() == true`. The client constructs a new local ghost instance and calls `onGhostAdd()`.
2. **Ghost update** — subsequent changes are sent via `packUpdate()`/`unpackUpdate()` using mask bits to transmit only changed fields.
3. **Ghost destroy** — when an object leaves scope, TNL calls `onGhostRemove()` and deletes the local copy.

The net effect: clients always see a recent (slightly delayed) reflection of server state.

Objects opt into ghosting by setting `mNetFlags.set(Ghostable)` (see `Ship` constructor). Objects that are not ghostable are server-only.

### Warping

**Warping** is the decision to skip position interpolation and snap a ghost directly to its new location instead of animating smoothly toward it.

Interpolation works well for small, expected movements (normal flight). It breaks down when a ship teleports, respawns, or the server corrects a large divergence — smoothly sliding a ship halfway across the map would look wrong. In those cases, `shipwarped` is set in `unpackUpdate`, which copies `ActualState` directly to `RenderState` and resets the trail. The `WarpPositionMask` bit in `packUpdate` is what signals a large positional change to clients.

The warp-in visual effect (spinning ship) is triggered separately by `mWarpInTimer` after a teleport or spawn.

### ControlState

**ControlState** is a compact server snapshot used to resynchronize the locally-predicted ship when it has drifted from the server's authoritative position.

Every packet from the server includes a CRC of what the server believes the client's state should be. If this CRC does not match, the server sends a ControlState blob: the authoritative position, velocity, cooldown flag, and active weapon. The client:

1. Applies `readControlState()` to jump its ship to the server-authoritative values.
2. Sets `mNeedReplayMoves = true`.
3. Replays all still-unacknowledged pending moves (`pendingMoves`) over the corrected base.

This is the correction half of predict-then-correct. The fields in `ControlState` were chosen to be the minimum needed to fully reseed the simulation for replay. (Energy and fast-recharge are currently commented out as an optimization.)

### RPCs

**RPCs** (Remote Procedure Calls) are discrete, typed function calls sent between client and server over the connection. They are declared with `TNL_DECLARE_RPC` and implemented with `TNL_IMPLEMENT_RPC`.

Naming convention:
- `s2c` — server-to-client (e.g. `s2cAddClient`, `s2cSetAuthenticated`, `s2cDisplayMessage`)
- `c2s` — client-to-server (e.g. `c2sSendChat`, `c2sRequestLoadout`, `c2sChangeTeams`)
- `s2r` — server-to-recorder

RPCs are used for discrete, non-continuous events — anything that is not efficiently handled by the ghost update stream. Examples: a player joining or changing teams, a score change, a chat message, a weapon switch request, authentication results, announcement banners.

Ghost updates handle *ongoing state*. RPCs handle *events and control-plane metadata*.

---

## End-to-end flow: local input -> server -> other clients

## 1) Input capture on client

Each client frame (`ClientGame::idle`):

- UI gathers controls into `Move` (`GameUserInterface::getCurrentMove`).
- Move time is set to frame delta.
- `Move::prepare()` is run to match packing precision effects.
- Move is enqueued via `GameConnection::addPendingMove()` (inherited from `ControlObjectConnection`).

The client also immediately applies this move to its local controlled ship for responsive prediction.

## 2) Move transmission via `ControlObjectConnection`

Client `writePacket()` sends:

- Control CRC/state bookkeeping
- First move index + count
- Packed pending `Move`s (delta-compressed against prior move)

Server `readPacket()`:

- Unpacks incoming moves
- Applies anti-speed-cheat time credit checks (`mMoveTimeCredit`)
- Replays accepted moves on authoritative control object (`Ship`) using server simulation path

So client sends **intent**, server executes **authority**.

## 3) Authoritative simulation on server

Server advances authoritative object state from accepted moves.

For ships, movement, collision, weapons/modules, and game rules are resolved server-side.

## 4) State replication back out (ghost updates)

Server sends ghost updates to each client for objects in scope.

For ships (`Ship::packUpdate`):

- Initial association data (player name + mounted items)
- Team/loadout/health/explosion/respawn flags
- Position/velocity
- Current move/module activity bits (as needed)
- Optional energy meter sync path (recording/replay use)

Clients apply with `Ship::unpackUpdate`:

- Resolve `ClientInfo` association by player name on initial update
- Update state and interpolation/warp decisions
- Apply module state and visual behavior

## 5) Correction + pending-move replay for local player

When server sends control-state correction:

- Client reads control state (`readControlState`) for its controlled object.
- Client marks replay needed and replays still-pending moves over corrected base state.

This gives responsive prediction with eventual server convergence.

## 6) Other players see results

Other clients receive the same authoritative ghost stream (subject to scope), so they see local player actions as replicated world-state changes.

---

## How scoping gates what gets replicated

`GameType::performScopeQuery()` determines what each connection receives:

- `GameType` itself is always in scope.
- Before sync completion, regular object ghosting is held back (`isReadyForRegularGhosts`).
- After sync, scope includes control object + nearby relevant objects + always-in-scope objects.
- Extra logic includes commander map and spy bug visibility.

This keeps bandwidth focused on objects relevant to that client’s view.

---

## RPC-based player metadata synchronization (`ClientInfo` path)

A key pattern is: **metadata as RPCs, world simulation as ghosts**.

Examples:

- `s2cAddClient` creates `RemoteClientInfo` entries on clients
- `s2cClientJoinedTeam` updates team assignment for a named client
- `s2cClientChangedRoles`, auth updates, busy/spawn-delay updates, etc.

So, if data is identity/role/UI-ish and discrete, it is usually RPC-driven. If it is ongoing spatial/simulation state, it is usually ghost-driven.

---

## Major patterns and when they are used

## Pattern A: Client prediction + server authority + replay

Used for controlled objects (especially ships). The goal is to make local input feel instantaneous while the server remains the authority on what actually happened.

**How prediction works:**

Each frame, the client applies the new `Move` to its local ship immediately (in `ClientGame::idle`), without waiting for a server round-trip. From the player's perspective, their ship responds instantly.

At the same time, the move is added to `pendingMoves` — a queue of moves the server has not yet acknowledged. The client keeps this queue so it can replay moves if a correction arrives.

**How correction works:**

Every packet from the server contains a CRC of what the server believes the client's ControlState should be. If the client's local state matches, no correction is needed. If there is a mismatch:

1. The server sends the authoritative ControlState (position, velocity, cooldown, active weapon) in the same packet.
2. The client calls `readControlState()` to overwrite its local ship state with the server's values.
3. The client sets `mNeedReplayMoves = true`.
4. After the packet is fully processed, the client iterates over all moves still in `pendingMoves` and calls `controlObject->idle(ClientReplayingPendingMoves)` for each one — re-simulating those inputs on top of the now-correct base.

The result: the ship snaps to the server-authoritative position and then catches up to where it should be based on inputs the server has not yet processed.

**Why corrections are rare in practice:**

The server's simulation and the client's prediction use the same physics code and the same `Move::prepare()` rounding. They diverge only when something unpredictable happens server-side — a collision, a hit, a spawn, a teleport. For straight-line movement in open space, client and server usually agree exactly.

**Why pending moves stay small:**

The time window of unacknowledged moves equals the one-way network latency (roughly half of round-trip time). At 100 ms RTT, the pending queue typically holds ~50 ms worth of moves. Replaying 50 ms of simulation is fast.

Use this pattern when responsiveness matters but cheating and divergence must be bounded.

## Pattern B: Masked delta ghost replication

Used for continuous world state:

- `packUpdate()` sends only changed subsets via mask bits.
- `unpackUpdate()` applies partial updates safely.

Use this for efficient high-frequency object updates.

## Pattern C: Scope-based relevance filtering

Used for bandwidth and correctness:

- Each client gets only relevant objects.
- Visibility/range/game-mode modifiers alter scope.

Use this for scalability and information control.

## Pattern D: RPC for discrete events and player metadata

Used for non-continuous state:

- Join/leave/team/auth/roles/messages/etc.

Use this for explicit game events and control-plane data.

## Pattern E: Dual local-player identity model on clients

Used to separate concerns:

- Local `FullClientInfo` for local config/identity context
- Remote mirror (`RemoteClientInfo`) for server-authoritative in-game identity record

Use this to avoid conflating pre-sync local state with in-game authoritative state.

---

## Practical mental model for debugging

When debugging a networking issue, classify the data first:

1. **Input intent?** -> `Move` + `ControlObjectConnection`
2. **Continuous world state?** -> Ghosts (`packUpdate`/`unpackUpdate`)
3. **Discrete metadata/event?** -> RPCs (`GameType`/`GameConnection`)
4. **Player identity confusion on client?** -> check local `FullClientInfo` vs `mLocalRemoteClientInfo`

This classification usually leads you to the right code path quickly.

## Key files to read alongside this document

- `zap/move.h`
- `zap/move.cpp`
- `zap/controlObjectConnection.h`
- `zap/controlObjectConnection.cpp`
- `zap/ship.h`
- `zap/ship.cpp`
- `zap/ClientInfo.h`
- `zap/ClientInfo.cpp`
- `zap/gameType.cpp`
- `zap/gameConnection.cpp`
- `zap/ClientGame.cpp`
- `zap/moveObject.h`
- `zap/moveObject.cpp`
