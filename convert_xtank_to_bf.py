#!/usr/bin/env python3
"""
Convert xtank .m maze files to Bitfighter .level files.

Usage:
  python3 convert_xtank_to_bf.py <input.m> <output.level>
  python3 convert_xtank_to_bf.py <input_dir/> <output_dir/>   (batch)

Xtank maze file format (binary):
  Byte 0        : game type  (0=Combat, 1=War, 2=Ultimate, 3=Capture, 4=Race, 5=Madman)
  Null string   : maze name
  Null string   : designer
  Null string   : description
  Binary data   : maze grid (column-major, run-length encoded, null terminated)

  Grid encoding:
    - Column-major order: outer loop x (0..29), inner loop y (0..29)
    - EMPTY_BOXES (0x80) | count : run of (count+1) empty/default cells
    - Otherwise: flags byte + optional type + optional teleport_code + optional team
    - Flags bits: INSIDE_MAZE(1) NORTH_WALL(2) WEST_WALL(4) NORTH_DEST(8) WEST_DEST(16)
                  TYPE_EXISTS(32) TEAM_EXISTS(64) EMPTY_BOXES(128)
    - Type byte uses external_type[] index: 0=NORMAL, 1=FUEL, 2=AMMO, 3=ARMOR,
      4=GOAL, 5=OUTPOST, 6=SCROLL_N..13=SCROLL_NW, 14=SLIP, 15=SLOW, 16=START_POS,
      17=NORTH_SYM, 18=WEST_SYM, 19=NORTH_DEST_SYM, 20=WEST_DEST_SYM,
      21=PEACE, 22=TELEPORT
    - TELEPORT type is followed by an extra teleport_code byte
    - Team byte: 0=NEUTRAL, 1..6=teams (xtank team 1 -> BF team 0)

Scale:
  1 xtank cell = 255 BF world units  (matches the legacy GridSize=255 convention)
  BarrierMaker wall width = 50 BF units (standard BF wall thickness)
  BF ship CollisionRadius = 24 units; ship diameter = 48 units
  This means each xtank cell is about 5 ship-widths wide

Unsupported xtank features (written as comments in output):
  - AMMO boxes      : no ammo concept in BF; converted to EnergyItem
  - SLOW boxes      : no slowdown zones in BF; skipped
  - PEACE boxes     : no damage-protection zones in BF; skipped
  - RACE_GAME       : no dedicated race mode in BF; uses GameType
  - Destructible walls (NORTH_DEST, WEST_DEST): not directly supported in BF
"""

import os
import sys
from collections import defaultdict

# ---- Xtank constants --------------------------------------------------------

GRID_WIDTH  = 30
GRID_HEIGHT = 30
MAZE_LEFT   = 2
MAZE_RIGHT  = 27
MAZE_TOP    = 2
MAZE_BOTTOM = 27

# Box flag bits
INSIDE_MAZE = (1 << 0)
NORTH_WALL  = (1 << 1)
WEST_WALL   = (1 << 2)
NORTH_DEST  = (1 << 3)
WEST_DEST   = (1 << 4)
TYPE_EXISTS = (1 << 5)
TEAM_EXISTS = (1 << 6)
EMPTY_BOXES = (1 << 7)
MAZE_FLAGS  = INSIDE_MAZE | NORTH_WALL | WEST_WALL | NORTH_DEST | WEST_DEST

# External type index -> type name
LANDMARK_TYPES = [
    'NORMAL',          # 0
    'FUEL',            # 1
    'AMMO',            # 2
    'ARMOR',           # 3
    'GOAL',            # 4
    'OUTPOST',         # 5
    'SCROLL_N',        # 6
    'SCROLL_NE',       # 7
    'SCROLL_E',        # 8
    'SCROLL_SE',       # 9
    'SCROLL_S',        # 10
    'SCROLL_SW',       # 11
    'SCROLL_W',        # 12
    'SCROLL_NW',       # 13
    'SLIP',            # 14
    'SLOW',            # 15
    'START_POS',       # 16
    'NORTH_SYM',       # 17  (symbolic wall — north)
    'WEST_SYM',        # 18  (symbolic wall — west)
    'NORTH_DEST_SYM',  # 19  (symbolic destructible wall — north)
    'WEST_DEST_SYM',   # 20  (symbolic destructible wall — west)
    'PEACE',           # 21
    'TELEPORT',        # 22
]

GAME_TYPES = {
    0: 'COMBAT_GAME',
    1: 'WAR_GAME',
    2: 'ULTIMATE_GAME',
    3: 'CAPTURE_GAME',
    4: 'RACE_GAME',
    5: 'MADMAN_GAME',
}

XTANK_NEUTRAL = 0

# ---- Bitfighter constants ----------------------------------------------------

# 1 xtank cell = CELL_SIZE BF world units.
# Rationale:  BF ship diameter = 48 WU; xtank cell = 192 px (768/4 screen).
# A vehicle is ~30-50 px wide => ~4-6 vehicles across one cell.
# 4-6 ships × 48 WU = 192-288 WU/cell + 50 WU wall = 242-338 WU/cell.
# 255 fits this range and matches the legacy BF GridSize=255 unit system.
CELL_SIZE   = 255
WALL_WIDTH  = 50          # BarrierMaker wall thickness
SCROLL_SPD  = 2000        # SpeedZone speed (BF default)
REPOP_TIME  = 30          # Pickup respawn delay (seconds)

# Centering offset: shift origin so level is centered at (0, 0)
MAZE_CELLS_W = MAZE_RIGHT  - MAZE_LEFT + 1   # = 26
MAZE_CELLS_H = MAZE_BOTTOM - MAZE_TOP  + 1   # = 26
MAZE_WU_W    = MAZE_CELLS_W * CELL_SIZE       # 6630 world units
MAZE_WU_H    = MAZE_CELLS_H * CELL_SIZE       # 6630 world units
CENTER_X     = MAZE_WU_W / 2                  # 3315
CENTER_Y     = MAZE_WU_H / 2                  # 3315

# BF team definitions: xtank team N (1-based) → BF team N-1 (0-based)
BF_TEAMS = [
    ('Red',    '1 0 0'),
    ('Blue',   '0 0 1'),
    ('Yellow', '1 1 0'),
    ('Green',  '0 1 0'),
    ('Purple', '0.5 0 1'),
    ('Orange', '1 0.67 0'),
]

# SpeedZone direction table: (P1 offset from center, P2 offset from center)
# Ships are boosted from P1 toward P2.
# SCROLL_N means "ships move north" → P1 south of center, P2 north of center.
# The 0.35 factor gives each SpeedZone a length of 70% of the cell width,
# keeping it well clear of the cell walls (wall half-thickness ≈ 10% of cell)
# while still covering the central travel path through the cell.
_SZ_HALF = CELL_SIZE * 0.35
SCROLL_DIR = {
    'N':  ( 0,  _SZ_HALF,  0, -_SZ_HALF),
    'S':  ( 0, -_SZ_HALF,  0,  _SZ_HALF),
    'E':  (-_SZ_HALF, 0,   _SZ_HALF, 0),
    'W':  ( _SZ_HALF, 0,  -_SZ_HALF, 0),
    'NE': (-_SZ_HALF,  _SZ_HALF,  _SZ_HALF, -_SZ_HALF),
    'NW': ( _SZ_HALF,  _SZ_HALF, -_SZ_HALF, -_SZ_HALF),
    'SE': (-_SZ_HALF, -_SZ_HALF,  _SZ_HALF,  _SZ_HALF),
    'SW': ( _SZ_HALF, -_SZ_HALF, -_SZ_HALF,  _SZ_HALF),
}


# ---- Coordinate helpers -----------------------------------------------------

def cx(ci):
    """X world coordinate of left edge of cell column ci."""
    return (ci - MAZE_LEFT) * CELL_SIZE - CENTER_X

def cy(cj):
    """Y world coordinate of top edge of cell row cj."""
    return (cj - MAZE_TOP) * CELL_SIZE - CENTER_Y

def cell_center(ci, cj):
    x = cx(ci) + CELL_SIZE / 2
    y = cy(cj) + CELL_SIZE / 2
    return round(x, 1), round(y, 1)

def xt2bf(xt_team):
    """Convert xtank team (0=neutral, 1+=teams) to BF team (-1=neutral, 0+=teams).

    255 is treated as neutral because some xtank maze files store 255 (0xFF) in
    the team byte when no team was explicitly set (uninitialized / NO_TEAM).
    """
    if xt_team == XTANK_NEUTRAL or xt_team == 255:
        return -1
    return xt_team - 1


# ---- Parsing ----------------------------------------------------------------

def _read_null_str(data, pos):
    end = pos
    while end < len(data) and data[end]:
        end += 1
    return data[pos:end].decode('latin-1', errors='replace'), end + 1


def parse_maze(path):
    """Parse an xtank .m file.  Returns a dict with maze metadata and grid."""
    with open(path, 'rb') as f:
        data = f.read()

    pos = 0
    game_type = data[pos]; pos += 1
    name, pos  = _read_null_str(data, pos)
    designer, pos = _read_null_str(data, pos)
    desc, pos  = _read_null_str(data, pos)

    # boxes[x][y]
    boxes = [[{
        'flags': 0, 'type': 'NORMAL', 'team': XTANK_NEUTRAL, 'tc': 0,
    } for _ in range(GRID_HEIGHT)] for _ in range(GRID_WIDTH)]

    ix, iy, empties = 0, 0, 0
    while ix < GRID_WIDTH and pos < len(data):
        if empties:
            empties -= 1
        else:
            b = data[pos]; pos += 1
            if b == 0:
                break
            if b & EMPTY_BOXES:
                empties = b & ~EMPTY_BOXES  # additional empty cells after this
            else:
                box = boxes[ix][iy]
                box['flags'] = b & MAZE_FLAGS
                if b & TYPE_EXISTS:
                    tb = data[pos]; pos += 1
                    box['type'] = LANDMARK_TYPES[tb] if tb < len(LANDMARK_TYPES) else 'NORMAL'
                    if box['type'] == 'TELEPORT':
                        box['tc'] = data[pos]; pos += 1
                if b & TEAM_EXISTS:
                    box['team'] = data[pos]; pos += 1
        # Column-major advance
        iy += 1
        if iy >= GRID_HEIGHT:
            iy = 0
            ix += 1

    return {
        'game_type': game_type,
        'game_name': GAME_TYPES.get(game_type, 'COMBAT_GAME'),
        'name':      name,
        'designer':  designer,
        'desc':      desc,
        'boxes':     boxes,
    }


# ---- Wall generation --------------------------------------------------------

def _merge_runs(vals):
    """Merge a sorted list of integers into (start, end) run tuples."""
    if not vals:
        return []
    runs, start, prev = [], vals[0], vals[0]
    for v in vals[1:]:
        if v == prev + 1:
            prev = v
        else:
            runs.append((start, prev))
            start = prev = v
    runs.append((start, prev))
    return runs


def collect_walls(boxes):
    """
    Return all wall line segments from the maze grid.

    A NORTH_WALL at cell (ci, cj) means a horizontal wall along the north
    (top) edge of that cell — i.e. at y = cy(cj).

    A WEST_WALL at cell (ci, cj) means a vertical wall along the west
    (left) edge of that cell — i.e. at x = cx(ci).

    NORTH_SYM / WEST_SYM box types also generate wall segments.

    Consecutive wall segments along the same row or column are merged into
    a single BarrierMaker line.

    Returns a list of (x1, y1, x2, y2) tuples (all in BF world units).
    """
    walls = []

    # --- Horizontal walls (NORTH_WALL) ---
    for cj in range(MAZE_TOP, MAZE_BOTTOM + 2):
        y_wall = round(cy(cj), 1)
        if cj <= MAZE_BOTTOM:
            has_wall = [ci for ci in range(MAZE_LEFT, MAZE_RIGHT + 1)
                        if boxes[ci][cj]['flags'] & NORTH_WALL]
        else:
            has_wall = []
        # NORTH_SYM types also count as north walls for that cell
        sym_wall = [ci for ci in range(MAZE_LEFT, MAZE_RIGHT + 1)
                    if cj <= MAZE_BOTTOM and
                    boxes[ci][cj]['type'] in ('NORTH_SYM', 'NORTH_DEST_SYM')]
        all_wall = sorted(set(has_wall) | set(sym_wall))
        for ci_s, ci_e in _merge_runs(all_wall):
            x1 = round(cx(ci_s), 1)
            x2 = round(cx(ci_e + 1), 1)
            walls.append((x1, y_wall, x2, y_wall))

    # --- Vertical walls (WEST_WALL) ---
    for ci in range(MAZE_LEFT, MAZE_RIGHT + 2):
        x_wall = round(cx(ci), 1)
        if ci <= MAZE_RIGHT:
            has_wall = [cj for cj in range(MAZE_TOP, MAZE_BOTTOM + 1)
                        if boxes[ci][cj]['flags'] & WEST_WALL]
        else:
            has_wall = []
        sym_wall = [cj for cj in range(MAZE_TOP, MAZE_BOTTOM + 1)
                    if ci <= MAZE_RIGHT and
                    boxes[ci][cj]['type'] in ('WEST_SYM', 'WEST_DEST_SYM')]
        all_wall = sorted(set(has_wall) | set(sym_wall))
        for cj_s, cj_e in _merge_runs(all_wall):
            y1 = round(cy(cj_s), 1)
            y2 = round(cy(cj_e + 1), 1)
            walls.append((x_wall, y1, x_wall, y2))

    return walls


# ---- Active maze bounds -----------------------------------------------------

def find_active_bounds(boxes):
    """Return (min_ci, max_ci, min_cj, max_cj) of cells that are part of the maze."""
    min_ci, max_ci = MAZE_RIGHT, MAZE_LEFT
    min_cj, max_cj = MAZE_BOTTOM, MAZE_TOP

    for ci in range(MAZE_LEFT, MAZE_RIGHT + 1):
        for cj in range(MAZE_TOP, MAZE_BOTTOM + 1):
            box = boxes[ci][cj]
            active = (box['flags'] != 0 or box['type'] != 'NORMAL')
            if active:
                min_ci = min(min_ci, ci)
                max_ci = max(max_ci, ci)
                min_cj = min(min_cj, cj)
                max_cj = max(max_cj, cj)

    if min_ci > max_ci:
        return MAZE_LEFT, MAZE_RIGHT, MAZE_TOP, MAZE_BOTTOM
    return min_ci, max_ci, min_cj, max_cj


# ---- Object generation ------------------------------------------------------

def collect_objects(boxes, game_name):
    """
    Scan the grid and collect game objects.

    Returns:
        obj_lines   : list of BF level directive strings
        starts      : dict  bf_team -> [(cx,cy), ...]
        teams_used  : set of bf team indices seen
        tele_groups : dict  teleport_code -> [(cx,cy), ...]
        unsupported : list of unsupported-feature description strings
    """
    obj_lines   = []
    starts      = defaultdict(list)
    teams_used  = set()
    tele_groups = defaultdict(list)
    unsupported = []
    _seen_unsup = set()

    def warn(msg):
        if msg not in _seen_unsup:
            unsupported.append(msg)
            _seen_unsup.add(msg)

    for ci in range(MAZE_LEFT, MAZE_RIGHT + 1):
        for cj in range(MAZE_TOP, MAZE_BOTTOM + 1):
            box = boxes[ci][cj]
            t   = box['type']
            bft = xt2bf(box['team'])
            ocx, ocy = cell_center(ci, cj)
            hw  = CELL_SIZE / 2 - WALL_WIDTH * 0.6   # inset half-width for zones

            if t == 'NORMAL':
                pass

            elif t == 'FUEL':
                obj_lines.append(f"EnergyItem {ocx} {ocy}")

            elif t == 'AMMO':
                obj_lines.append(f"EnergyItem {ocx} {ocy}")
                warn("AMMO boxes: no ammo concept in BF; converted to EnergyItem")

            elif t == 'ARMOR':
                obj_lines.append(f"RepairItem {ocx} {ocy} {REPOP_TIME}")

            elif t == 'GOAL':
                if bft >= 0:
                    teams_used.add(bft)
                if game_name == 'CAPTURE_GAME':
                    obj_lines.append(f"FlagItem {bft} {ocx} {ocy}")
                else:
                    x1 = round(ocx - hw, 1); y1 = round(ocy - hw, 1)
                    x2 = round(ocx + hw, 1); y2 = round(ocy + hw, 1)
                    obj_lines.append(f"GoalZone {bft} {x1} {y1}  {x2} {y1}  {x2} {y2}  {x1} {y2}")

            elif t == 'OUTPOST':
                if bft >= 0:
                    teams_used.add(bft)
                obj_lines.append(f"Turret {bft} {ocx} {ocy} 1")

            elif t.startswith('SCROLL_'):
                direction = t[7:]
                dx1, dy1, dx2, dy2 = SCROLL_DIR.get(direction, SCROLL_DIR['N'])
                sx1 = round(ocx + dx1, 1); sy1 = round(ocy + dy1, 1)
                sx2 = round(ocx + dx2, 1); sy2 = round(ocy + dy2, 1)
                obj_lines.append(f"SpeedZone {sx1} {sy1} {sx2} {sy2} {SCROLL_SPD}")

            elif t == 'SLIP':
                x1 = round(ocx - hw, 1); y1 = round(ocy - hw, 1)
                x2 = round(ocx + hw, 1); y2 = round(ocy + hw, 1)
                obj_lines.append(f"SlipZone 0.5 {x1} {y1}  {x2} {y1}  {x2} {y2}  {x1} {y2}")

            elif t == 'SLOW':
                warn("SLOW boxes: no slowdown zone in BF; skipped")

            elif t == 'START_POS':
                team = bft if bft >= 0 else 0
                starts[team].append((ocx, ocy))
                teams_used.add(team)

            elif t == 'TELEPORT':
                tele_groups[box['tc']].append((ocx, ocy))

            elif t == 'PEACE':
                warn("PEACE boxes: no damage-protection zone in BF; skipped")

            # NORTH_SYM / WEST_SYM / *_DEST_SYM handled as walls; skip here.

    # --- Teleporters: chain each group cyclically ---
    for _code, positions in sorted(tele_groups.items()):
        if len(positions) < 2:
            continue
        for idx, (tx1, ty1) in enumerate(positions):
            tx2, ty2 = positions[(idx + 1) % len(positions)]
            obj_lines.append(f"Teleporter {tx1} {ty1} {tx2} {ty2}")

    return obj_lines, starts, teams_used, unsupported


# ---- Level generation -------------------------------------------------------

def generate_level(maze_data, output_path):
    """Write a .level file from parsed maze_data.  Returns list of warnings."""
    boxes  = maze_data['boxes']
    gname  = maze_data['game_name']
    name   = maze_data['name']   or 'Untitled'
    dsgn   = maze_data['designer'] or ''
    desc   = maze_data['desc']   or ''

    walls, obj_lines, starts, teams_used, unsupported = (
        collect_walls(boxes),
        *collect_objects(boxes, gname),
    )

    # ---- Determine BF game type and team count ----
    num_teams = max(len(teams_used), 1)

    if gname == 'COMBAT_GAME':
        gametype = "GameType 10 0"
        num_teams = 1  # FFA uses a single generic team

    elif gname == 'WAR_GAME':
        gametype  = "GameType 10 0"
        num_teams = max(num_teams, 2)

    elif gname == 'CAPTURE_GAME':
        gametype  = "CTFGameType 10 3"
        num_teams = max(num_teams, 2)

    elif gname == 'ULTIMATE_GAME':
        gametype  = "NexusGameType 8 1.0 30 5000"
        num_teams = max(num_teams, 2)

    elif gname == 'RACE_GAME':
        gametype  = "GameType 10 0"
        num_teams = max(num_teams, 2)
        unsupported.insert(0, "RACE_GAME: no dedicated race mode in BF; using GameType")

    elif gname == 'MADMAN_GAME':
        gametype  = "RabbitGameType 8 50 10 12"
        num_teams = max(num_teams, 1)

    else:
        gametype  = "GameType 10 0"
        num_teams = 1

    num_teams = min(num_teams, len(BF_TEAMS))

    # ---- Build header ----
    def quote(s):
        s = s.replace('"', '""')
        return f'"{s}"' if (' ' in s or '"' in s or '#' in s or not s) else s

    out = []
    out.append("LevelFormat 2")
    out.append(gametype)
    out.append(f"LevelName {quote(name)}")
    out.append(f"LevelDescription {quote(desc)}")
    out.append(f"LevelCredits {quote(dsgn)}")

    if gname in ('COMBAT_GAME', 'RACE_GAME') and num_teams == 1:
        out.append("Team Players 0.7 0.7 0.7")
    else:
        for i in range(num_teams):
            tname, tcolor = BF_TEAMS[i]
            out.append(f"Team {tname} {tcolor}")

    out.append("Specials")
    out.append("MinPlayers")
    out.append("MaxPlayers")

    # Unsupported feature comments
    if unsupported:
        out.append("")
        seen = set()
        for msg in unsupported:
            if msg not in seen:
                out.append(f"# UNSUPPORTED: {msg}")
                seen.add(msg)

    # ---- Walls ----
    out.append("")
    out.append("# Walls")
    for (x1, y1, x2, y2) in walls:
        out.append(f"BarrierMaker {WALL_WIDTH} {x1} {y1} {x2} {y2}")

    # ---- Objects ----
    if obj_lines:
        out.append("")
        out.append("# Objects")
        out.extend(obj_lines)

    # ---- Spawns ----
    out.append("")
    out.append("# Spawns")
    if starts:
        for bf_team in sorted(starts.keys()):
            for scx, scy in starts[bf_team]:
                out.append(f"Spawn {bf_team} {scx} {scy}")

    # For any team that has no explicit spawns, add default spawns.
    # This handles mazes where goals/flags exist for a team but no START_POS.
    mn_ci, mx_ci, mn_cj, mx_cj = find_active_bounds(boxes)
    mid_ci = (mn_ci + mx_ci) / 2
    mid_cj = (mn_cj + mx_cj) / 2
    q_ci   = max((mx_ci - mn_ci) / 4, 1)
    q_cj   = max((mx_cj - mn_cj) / 4, 1)

    for bf_team in range(num_teams):
        if bf_team not in starts or not starts[bf_team]:
            # Place default spawns for this team (4 for single-team, 2 for multi-team)
            if bf_team % 2 == 0:
                sci_r = round(mid_ci - q_ci)
                scj_r = round(mid_cj - q_cj)
            else:
                sci_r = round(mid_ci + q_ci)
                scj_r = round(mid_cj + q_cj)
            scx, scy = cell_center(sci_r, scj_r)
            out.append(f"Spawn {bf_team} {scx} {scy}")
            scx2, scy2 = cell_center(sci_r, scj_r + 1)
            out.append(f"Spawn {bf_team} {scx2} {scy2}")
            if num_teams == 1:
                # Add spawns on the opposite side for FFA maps
                scx3, scy3 = cell_center(round(mid_ci + q_ci), round(mid_cj + q_cj))
                out.append(f"Spawn {bf_team} {scx3} {scy3}")
                scx4, scy4 = cell_center(round(mid_ci + q_ci), round(mid_cj + q_cj) + 1)
                out.append(f"Spawn {bf_team} {scx4} {scy4}")

    # ---- Loadout zones (for multi-team games) ----
    if num_teams >= 2 and gname not in ('COMBAT_GAME',):
        out.append("")
        out.append("# Loadout zones")
        lw = CELL_SIZE * 0.8
        # mn_ci/mx_ci/q_ci already computed above
        for bf_team in range(num_teams):
            if bf_team in starts and starts[bf_team]:
                lcx, lcy = starts[bf_team][0]
            else:
                # Near a spawn corner
                sci = mid_ci - q_ci if bf_team == 0 else mid_ci + q_ci
                scj = mid_cj - q_cj if bf_team == 0 else mid_cj + q_cj
                lcx, lcy = cell_center(round(sci), round(scj))
            x1 = round(lcx - lw / 2, 1); y1 = round(lcy - lw / 2, 1)
            x2 = round(lcx + lw / 2, 1); y2 = round(lcy + lw / 2, 1)
            out.append(f"LoadoutZone {bf_team} {x1} {y1}  {x2} {y1}  {x2} {y2}  {x1} {y2}")

    # ---- Mode-specific extras ----
    if gname == 'ULTIMATE_GAME':
        nhw = CELL_SIZE
        out.append("")
        out.append("# Nexus zone (center of map)")
        out.append(f"NexusZone {-nhw} {-nhw}  {nhw} {-nhw}  {nhw} {nhw}  {-nhw} {nhw}")

    elif gname == 'MADMAN_GAME':
        out.append("")
        out.append("# Rabbit flag")
        out.append("FlagItem -1 0 0")

    out.append("")  # trailing newline

    with open(output_path, 'w') as f:
        f.write('\n'.join(out) + '\n')

    return unsupported


# ---- Main -------------------------------------------------------------------

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    src, dst = sys.argv[1], sys.argv[2]

    if os.path.isdir(src):
        os.makedirs(dst, exist_ok=True)
        for fname in sorted(os.listdir(src)):
            if not fname.endswith('.m'):
                continue
            in_path  = os.path.join(src, fname)
            out_name = fname[:-2] + '.level'
            out_path = os.path.join(dst, out_name)
            try:
                maze = parse_maze(in_path)
                warns = generate_level(maze, out_path)
                print(f"  {fname:20s} -> {out_name}")
                for w in warns:
                    print(f"      [warn] {w}")
            except Exception as e:
                print(f"  ERROR {fname}: {e}")
                import traceback; traceback.print_exc()
    else:
        maze  = parse_maze(src)
        warns = generate_level(maze, dst)
        for w in warns:
            print(f"[warn] {w}")


if __name__ == '__main__':
    main()
