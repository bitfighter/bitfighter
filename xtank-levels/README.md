# Xtank Levels for Bitfighter

This directory contains Bitfighter `.level` files converted from the original
[xtank](https://github.com/lidl/xtank) maze files.

## Converted Levels

| File | Xtank Name | Game Type | Notes |
|---|---|---|---|
| 4x4.level | 4x4 | Deathmatch | Small 4×4 room arena |
| Alex1.level | Alex1 | Deathmatch | Large complex maze |
| Arena.level | Arena | Deathmatch | Circular arena with banked scroll curves |
| Arena1.level | Arena1 | Deathmatch | Alternate arena layout |
| Aztec.level | Aztec | Deathmatch | Aztec-themed maze |
| Basic.level | Basic | Deathmatch | Minimal combat room |
| BigBox.level | BigBox | Deathmatch | Open box with inner structures |
| Bottle.level | Bottle | Deathmatch | Bottle-shaped arena |
| Bouncy.level | Bouncy | Deathmatch | Small maze |
| BoxFrenzy.level | BoxFrenzy | Deathmatch | Dense box maze |
| Capture.level | Capture | CTF | Generic two-team capture maze |
| Capture2.level | Capture2 | CTF | Alternate capture layout |
| Capture4.level | Capture4 | CTF | Four-team capture |
| CaptureX.level | CaptureX | CTF | Experimental capture design |
| Castle.level | Castle | Deathmatch | Castle-themed maze |
| City.level | City | Deathmatch | City block layout |
| Col.level | Col | Deathmatch | Column maze |
| Collider.level | Collider | Deathmatch | Small open arena |
| Commaze1.level | Commaze1 | Deathmatch | Community maze (has scroll/slow cells) |
| Crazed.level | Crazed | Deathmatch (was Race) | 4-team chaotic maze |

## Scale

Xtank mazes use a 30×30 grid (26×26 usable cells). The conversion uses:

- **1 xtank cell = 255 Bitfighter world units** (matches the legacy GridSize=255 convention)
- **Wall thickness = 50 WU** (standard Bitfighter default, ≈ 1 ship diameter)
- **Corridors ≈ 205 WU** (≈ 4 ship diameters)
- **Full 26×26 maze ≈ 6 630 × 6 630 WU**, centered at (0, 0)

## Unsupported Xtank Features

The following xtank landmark types have no direct Bitfighter equivalent:

| Xtank Type | Handling |
|---|---|
| `AMMO` | Converted to `EnergyItem` (energy pickup) |
| `SLOW` | Skipped (no slowdown zone in BF) |
| `PEACE` | Skipped (no damage-protection zone in BF) |
| `RACE_GAME` | Uses `GameType` (no dedicated race mode in BF) |
| Destructible walls (`NORTH_DEST`, `WEST_DEST`) | Treated as normal walls |

## Xtank → Bitfighter Type Mapping

| Xtank Type | Bitfighter Object |
|---|---|
| `FUEL` | `EnergyItem` |
| `AMMO` | `EnergyItem` |
| `ARMOR` | `RepairItem` |
| `GOAL` (CTF) | `FlagItem` |
| `GOAL` (other) | `GoalZone` |
| `OUTPOST` | `Turret` (snaps to nearest wall) |
| `SCROLL_N/S/E/W/…` | `SpeedZone` (pointing in scroll direction) |
| `SLIP` | `SlipZone` |
| `START_POS` | `Spawn` |
| `TELEPORT` | `Teleporter` (cyclic pairing by teleport code) |

## Conversion Script

Levels were generated using `convert_xtank_to_bf.py` in the repository root:

```
python3 convert_xtank_to_bf.py Arena.m Arena.level
python3 convert_xtank_to_bf.py /path/to/xtank/Mazes/  xtank-levels/
```
