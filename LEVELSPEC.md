# Bitfighter Level File Specification

This document describes the structure and syntax of Bitfighter `.level` files, intended as a reference for tools, AI systems, or humans generating levels programmatically.

---

## Overview

A level file is a plain-text file where each non-empty line is a **directive** — a keyword followed by space-separated arguments. Lines beginning with `#` are comments and are ignored.

```
# This is a comment
GameType 10 8
LevelName My Level
```

Tokens are separated by whitespace. Strings containing spaces, `"`, or `#` must be enclosed in double-quotes. A literal `"` inside a quoted string is represented as `""`.

---

## File Structure

Lines fall into three categories, which should appear roughly in this order:

1. **Header / metadata** — `LevelFormat`, game type, level name, teams, specials, script, player counts
2. **Geometry** — `BarrierMaker`, `PolyWall` (walls)
3. **Objects** — all other game objects (spawns, items, zones, engineered items, etc.)

### Minimal valid level

```
LevelFormat 2
GameType 10 8
LevelName Example
LevelDescription ""
LevelCredits ""
Team Blue 0 0 1
Specials
MinPlayers
MaxPlayers
Spawn 0 0 0
```

---

## Header Directives

### `LevelFormat <version>`

Should be the **first line** of the file. Current version is `2`.

- Version 1 (legacy): all coordinates are stored as grid units and multiplied by the `GridSize` value (default 255) to get world units.
- Version 2 (current): all coordinates are stored directly as world units. No `GridSize` needed.

If omitted, the engine assumes version 1 with `GridSize 255`.

```
LevelFormat 2
```

### Game Type Line

Must appear before any objects. Ends with the suffix `GameType`. Available types:

| Keyword | Mode | Parameters |
|---|---|---|
| `GameType` | Bitmatch (FFA/team deathmatch) | `<time_min> <win_score>` |
| `CTFGameType` | Capture the Flag | `<time_min> <win_score>` |
| `NexusGameType` | Nexus (Hunters) | `<time_min> <nexus_closed_min> <nexus_open_sec> <win_score>` |
| `RabbitGameType` | Rabbit (flag carrier) | `<time_min> <win_score> <flag_return_sec> <points_per_event>` |
| `HTFGameType` | Hold the Flag | `<time_min> <win_score> <points_per_min>` |
| `CoreGameType` | Core | `<time_min> [<redist_method>]` |
| `SoccerGameType` | Soccer | `<time_min> <win_score>` |
| `ZoneControlGameType` | Zone Control | `<time_min> <win_score>` |
| `RetrieveGameType` | Retrieve | `<time_min> <win_score>` |

- `time_min`: game duration in **minutes** (float). Use `0` for unlimited.
- `win_score`: score required to win. Use `0` for score-unlimited.
- `HuntersGameType` is accepted as a legacy alias for `NexusGameType`.

**CoreGameType `redist_method`** (optional, controls player redistribution when a team is eliminated):

| Value | Meaning |
|---|---|
| `RedistNone` | No redistribution (default) |
| `RedistBalanced` | Balance players across remaining teams |
| `RedistBalancedNonWinners` | Balance across all but the winning team |
| `RedistRandom` | Randomly reassign players |
| `RedistLoser` | Move players to team with fewest Cores |
| `RedistWinner` | Move players to team with most Cores |

Examples:
```
GameType 10 20
CTFGameType 8 3
NexusGameType 8.000 1.000 30 5000
RabbitGameType 8 50 10 12
CoreGameType 8 RedistBalanced
```

### `LevelName <name>`

Display name of the level. Quote if it contains spaces.

```
LevelName "My Cool Level"
LevelName Capture01
```

### `LevelDescription <description>`

Short description shown to players. Use `""` for empty.

```
LevelDescription "Fight for the flags!"
LevelDescription ""
```

### `LevelCredits <author>`

Level author credit. Use `""` for empty.

```
LevelCredits Watusimoto
LevelCredits ""
```

### `LevelDatabaseId <id>`

Optional. Integer ID from the Bitfighter level database. Omit for locally-created levels.

### `Team <name> <r> <g> <b>`

Defines a team. At least one team is required for most game modes; two for team games. Maximum **7 named teams** (indices 0–6). Color components are floats in range `0.0`–`1.0`.

```
Team Blue 0 0 1
Team Red 1 0 0
Team Yellow 1 1 0
Team Orange 1 0.67 0
Team Green 0 1 0
Team Purple 0.5 0 1
```

Teams are referenced by **index**: `0` = first `Team` line, `1` = second, etc. Two special team values exist:

| Value | Meaning |
|---|---|
| `-1` | Neutral (no team; objects hostile to nobody) |
| `-2` | Hostile to All |

### `Specials [Engineer] [EngineerUnrestricted]`

Enables optional gameplay features. May be empty.

- `Engineer` — allows players to use the Engineer module to build turrets, force fields, and teleporters.
- `EngineerUnrestricted` — Engineer with no placement restrictions (implies `Engineer`).

```
Specials Engineer
Specials
```

### `Script <script_file> [args...]`

Optional. Attaches a Lua level-generation script (`.levelgen` file). Arguments after the filename are passed to the script.

```
Script mazegen.levelgen 10 10 0 0 255
```

### `MinPlayers [n]`

Recommended minimum number of players. Omit the number to leave unspecified.

### `MaxPlayers [n]`

Recommended maximum number of players. Omit the number to leave unspecified.

### `FogOfWar [Default|Yes|No]`

Controls whether Fog of War is enabled for the level. When Fog of War is on, wall tiles are only sent to clients as they come within the player's scope radius, reducing network traffic on large maps.

- `FogOfWar Yes` — Fog of War is enabled. Wall tiles are delivered closest-first within a scope radius around each player.
- `FogOfWar No` — Fog of War is disabled. All wall tiles are delivered immediately on level load.
- `FogOfWar Default` (or line omitted) — Fog of War is enabled for xtank vehicle games and disabled for bitfighter ship games.

The keyword is case-insensitive.

```
FogOfWar Yes
FogOfWar No
FogOfWar Default
```

---

## Coordinate System

All coordinates in **LevelFormat 2** files are in **world units**:

- A typical ship is approximately **1 unit** in diameter.
- Most levels span roughly **10–30 units** in each dimension.
- Coordinates may be negative; the origin `(0, 0)` can be anywhere.
- The X axis increases to the right; Y increases downward.
- Coordinates are stored as floats (usually 6 or fewer decimal places).

**Legacy (LevelFormat 1 / GridSize):** If a file lacks `LevelFormat 2`, coordinates are in *grid units* and multiplied by the `GridSize` value (default 255) at load time. Modern levels should always use `LevelFormat 2` and real coordinates.

---

## Object ID Syntax

Any object keyword can be followed by `!<id>` to assign a user-defined integer ID, which Lua scripts can use to reference specific objects:

```
Turret!42 0 5.0 3.0 0
FlagItem!7 -1 0 0
```

---

## Geometry Notation

Many objects use trailing `x y` pairs to describe their shape. When noted below as `<x1> <y1> <x2> <y2> ...`, these are floating-point world coordinates.

---

## Wall Objects

Walls are solid obstacles that block movement and line-of-sight.

### `BarrierMaker <width> <x1> <y1> <x2> <y2> [<x3> <y3> ...]`

A **line-based wall** — a polyline extruded to the given width. Minimum 2 points.

- `width`: integer, range `1`–`2500`, default `50`.
- The wall is centered on the polyline; actual world thickness = `width` units.
- Multiple consecutive segments are joined smoothly.

```
BarrierMaker 50 -5 0 5 0
BarrierMaker 50 0 0 2 0 4 2 4 6
BarrierMaker 200 -1.5 -1.5 1.5 1.5
```

### `PolyWall <x1> <y1> <x2> <y2> <x3> <y3> [...]`

A **solid polygon wall**. Minimum 3 vertices, maximum 64.

```
PolyWall 0 0  4 0  4 3  0 3
PolyWall 3.5 2.7  3.7 3  3.3 3
```

---

## Spawn Points

### `Spawn <team> <x> <y>`

Ship spawn location. At least one per team is required.

- `team`: team index (0, 1, 2, …)

```
Spawn 0 1.5 1.5
Spawn 1 10.5 5.5
```

### `FlagSpawn <team> <x> <y> [<respawn_time_sec>]`

Where a flag respawns after being dropped. `respawn_time_sec` defaults to `0` (immediate).

- `team`: team index, or `-1` for neutral.

```
FlagSpawn -1 6.5 6.5 30
FlagSpawn 0 1 3
```

*Note:* Placing a `FlagItem` automatically creates a `FlagSpawn` at its initial location.

### `AsteroidSpawn <x> <y> [<interval_sec>] [Size=<n>] [Team=<n>]`

Periodically spawns an `Asteroid`. Optional keyword arguments:

- `Size=<n>` — asteroid size class (integer; larger = bigger)
- `Team=<n>` — team that owns the spawn (default neutral)

```
AsteroidSpawn 0 0 20
AsteroidSpawn 5 5 15 Size=3 Team=0
```

---

## Items / Pickups

### `RepairItem <x> <y> [<repop_time_sec>]`

A repair pickup that restores ship health.

- `repop_time_sec`: integer seconds until respawn after pickup. `0` = does not respawn.

```
RepairItem 4 0 45
RepairItem 0 0 0
```

### `ResourceItem <x> <y>`

An engineering resource block, picked up by the Engineer module to build objects.

```
ResourceItem 6 3.5
```

### `EnergyItem <x> <y>`

An energy pickup that restores ship energy.

```
EnergyItem 3 3
```

---

## Flag and Ball Items

### `FlagItem <team> <x> <y> [SpawnLock]`

A pre-placed flag. Used in CTF, HTF, Retrieve, ZoneControl, Rabbit.

- `team`: team index the flag belongs to, or `-1` for neutral.
- `SpawnLock` (optional): locks the flag to its initial spawn point.  Without this, items can respawn at any flag/ball spawn point.

```
FlagItem 0 0.5 0.5
FlagItem 1 11.5 7.5
FlagItem -1 0 0
```

### `SoccerBallItem <x> <y> [SpawnLock]`

The soccer ball for Soccer game mode.

```
SoccerBallItem 3 5
```

---

## Game-Mode-Specific Objects

### `CoreItem <team> <health> <x> <y> [<rotation_speed>]`

A Core object for Core game mode. Each team typically has one or more cores.

- `health`: float, typically `5`–`100`.
- `rotation_speed`: integer, rotational speed in arbitrary units. Default `0`.
- `team`: `-1` for neutral.

```
CoreItem 0 80 -8 0
CoreItem 1 90 3.7 0
CoreItem -1 15 0 0
```

### `HuntersNexusObject` / `NexusZone` — see Zones below.

---

## Zones

All zone polygons require at least 3 vertices, maximum 64.

### `LoadoutZone <team> <x1> <y1> <x2> <y2> <x3> <y3> [...]`

An area where players configure their ship loadout. Each team should have at least one.  If there are no loadout zones, ship configuration will change when a ship respawns.

- `team`: team index, `-1` for neutral (any team can use it), `-2` for hostile.

```
LoadoutZone 0 -1.1 -1.4 -1.1 -1.2 1.1 -1.2 1.1 -1.4
LoadoutZone 1 8 6 8 7 8.2 7 8.2 6
```

### `GoalZone <team> <x1> <y1> <x2> <y2> <x3> <y3> [...]`

A scoring zone. Used in Soccer (goals), HTF, Retrieve, ZoneControl.

- `team`: which team the zone belongs to, or `-1`/`-2`.

```
GoalZone 0 2.1 9 2.113673 9.156282 2.154277 9.307818 3.9 9
GoalZone 1 3.9 1 3.886327 0.843717 2.1 1
```

### `NexusZone <x1> <y1> <x2> <y2> <x3> <y3> [...]`

The Nexus drop zone in Nexus/Hunters game mode. No team argument.

Accepted legacy aliases: `HuntersNexusObject`, `NexusObject`.

```
NexusZone 3.3 3.3  3.7 3.3  3.7 3.7  3.3 3.7
```

### `SlipZone [<slip_amount>] <x1> <y1> <x2> <y2> <x3> <y3> [...]`

An inertia (slip) zone that reduces friction. Polygon.

- `slip_amount`: optional float; if the total number of numeric arguments is **odd**, the first is treated as the slip amount.

```
SlipZone 0.500 -3 -3  3 -3  3 3  -3 3
```

---

## Engineered Items

These objects are placed on walls. Their position is snapped to the nearest wall surface at load time.

### `ForceFieldProjector <team> <x> <y> <heal_rate>`

Projects a force field across a passage.

- `team`: team index, `-1` for neutral, `-2` for hostile.
- `x y`: position near a wall (engine snaps to wall).
- `heal_rate`: integer; rate at which the projector repairs itself (0 = default/no self-repair).

```
ForceFieldProjector 0 1.5 2.2 0
ForceFieldProjector 1 10.5 2.5 1
ForceFieldProjector -1 1.499999 5.5 1
```

### `Turret <team> <x> <y> <heal_rate> [<weapon_type>]`

An auto-targeting turret.

- `team`: team index, `-1` for neutral, `-2` for hostile.
- `x y`: position near a wall.
- `heal_rate`: integer; self-repair rate (0 = default).
- `weapon_type` (optional): one of `Turret` (default), `Burst`, `Triple`, `Seeker`.

```
Turret 0 0.319325 0.319324 0
Turret 1 11.680676 7.680676 0
Turret -1 8 2 1
Turret -1 2.4 0 0 Burst
```

---

## Movement Objects

### `Teleporter <from_x> <from_y> <to_x> <to_y> [Delay=<seconds>]`

Teleports a ship from the entry point to the destination.

- Multiple `Teleporter` lines with the **same `from` position** create a multi-destination teleporter (random destination chosen each time).
- `Delay=<seconds>`: cooldown before the teleporter activates again (minimum 0.1 s).

```
Teleporter 0 7  8 4
Teleporter 12 1  4 4
Teleporter -3.7 0  -7 1
Teleporter -3.7 0  -9 1
```

### `SpeedZone <x1> <y1> <x2> <y2> [<speed>] [SnapEnabled] [Rotate=<rot_speed>]`

A boost pad that launches ships from point 1 toward point 2.

- `speed`: integer in world units/second. Default `2000`.
- `SnapEnabled`: flag (no value); snaps arriving ships to the launch point.
- `Rotate=<rot_speed>`: float; the speed zone rotates at this angular velocity.

```
SpeedZone 8.5 8  8.24902 8  2000 SnapEnabled
SpeedZone 1275 612  1275 459  2000
SpeedZone 0 0  5 0  1500 Rotate=0.5
```

---

## Decorative / Informational Objects

### `TextItem <team> <x1> <y1> <x2> <y2> <size> <text...>`

Displays text in the level. The text stretches from `(x1,y1)` to `(x2,y2)`.

- `team`: which team sees this text (or `-1` for all).
- `size`: float, text height in world units.
- `text`: remaining tokens are joined with spaces to form the display string.

```
TextItem -1 -1.1 0.1 1.1 0.1 129.711 Bitfighter
TextItem 0 0 0 5 0 80 "Team 0 only message"
```

---

## Bots

### `Robot [<team>] [<script_file>] [args...]`

Places a bot (AI-controlled ship) in the level. The bot is added at level load.

- `team`: team index (or `-1`/`-2`). Defaults to no team.
- `script_file`: filename of a `.bot` script. Defaults to the server's configured default bot.
- Additional arguments are passed to the bot script.

```
Robot 0 s_bot.bot
Robot 1
Robot -1 hunter.bot aggressive
```

---

## Complete Example

The following is a minimal two-team CTF level:

```
LevelFormat 2
CTFGameType 10 3
LevelName SimpleCTF
LevelDescription "Capture the enemy flag 3 times to win."
LevelCredits Author
Team Blue 0 0 1
Team Red 1 0 0
Specials Engineer
MinPlayers 2
MaxPlayers 8

# Outer walls
BarrierMaker 50 -10 -8  10 -8  10 8  -10 8  -10 -8

# Interior obstacles
BarrierMaker 50 -4 -2  -4 2
BarrierMaker 50  4 -2   4 2
PolyWall -1 -1  1 -1  0 1

# Loadout zones
LoadoutZone 0 -9 -1  -9 1  -8 1  -8 -1
LoadoutZone 1  8 -1   9 -1  9 1  8 1

# Flags
FlagItem 0 -8 0
FlagItem 1  8 0

# Spawns
Spawn 0 -7 -2
Spawn 0 -7  0
Spawn 0 -7  2
Spawn 1  7 -2
Spawn 1  7  0
Spawn 1  7  2
```

---

## Tips for AI Level Generation

- **Every level must have a game type line** (e.g., `GameType 10 8`) before any objects.
- **Every level must have at least one `Spawn` per team.** Without spawns, players cannot join.
- **Team games** (CTF, HTF, Core, Soccer, ZC, Retrieve) require at least two `Team` lines.
- **Loadout zones** should be present in most levels so players can set up their ship.  These also allow ships to recover faster.
- **Flag-based games** need `FlagItem` and/or `FlagSpawn` entries.
  - CTF: one `FlagItem` per team at their base.
  - HTF/Retrieve/ZC: flags (`FlagItem -1`) placed neutrally around the map.
  - Nexus: use `FlagSpawn -1` (flags spawn at those locations) and one or more `NexusZone` polygons.
  - Rabbit: one neutral `FlagItem -1` at center, `FlagSpawn` locations for respawn.
- **Core game**: each team needs one or more `CoreItem` objects.
- **Soccer**: Often levels will have one `SoccerBallItem` and one `GoalZone` per team, but sometimes multiples are fun.
- **Walls should enclose the play area** — ships and objects that leave the play area can fly off to infinity, so a bounded level is usually fun and intentional.
- **BarrierMaker width** of `50` is a good default. Use `100`–`200` for thick interior walls or cover.
- **Teleporters** with matching origins create multi-destination teleporters (random exit).
- **Neutral turrets** (`team = -1`) attack everyone; **team turrets** attack opposing teams only.
- **Heal rate** on engineered items (Turrets, ForceFieldProjectors): `0` = server default, `1` = slow self-repair, higher = faster.
- **SpeedZone speed** of `2000` is the default and a good starting value. Direction is from point 1 → point 2.  Use snapping if a specific destination is desired.
- **SlipZones** should be used sparingly.  They sound more fun than they are.
- Coordinates in `LevelFormat 2` are raw world units. A square play area of 20×20 units is typical for small maps; 40×40 for large ones.
- **Shape** -- Maps can be any shape; square and rectangle are common, but the best maps often have a distinctive or thematic shape.
- **Symmetry** -- Levels are often symmetrical (2, 3, or 4 way symmetry, depending on the number of teams), but a well designed and balanced asymmetrical level can be great.
- **Theme** -- Most levels don't have any inherent theme, but occasionally a designer will find a way to create one, through level naming, level shape, text, team names and colors, and other elements.
- **Team count** -- Levels with more teams require more players (or bots), and player count can sometimes be a challenge.

