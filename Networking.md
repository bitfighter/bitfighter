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

Used for controlled objects (especially ships):

- Client predicts immediately from local input.
- Server remains source of truth.
- Corrections are merged by replaying unacknowledged moves.

Use this when responsiveness matters but cheating/divergence must be bounded.

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
