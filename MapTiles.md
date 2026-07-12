# Tiled Walls Specification

## Overview

The Tiled Walls system replaces the current monolithic `s2cAddWalls` RPC mechanism with a **tile-based, incremental delivery model** for wall geometry. This enables:

- **Fog-of-war:** clients receive only wall tiles near their ship's position
- **Scalability:** large maps send tiles closest-first, filling in over a few seconds
- **Unified geometry:** `WallItem` (line spines) and `PolyWall` (polygons) are converted to a single polygon representation
- **Seamless tiling:** shared edges between tiles are marked as non-rendered, so no seams appear
- **More flexibility** for scripting wall changes during a game -- new tiles can be regenerated and sent to clients as needed

## Architecture

### Server-side (Level Load)

1. **Extrude all walls to polygons:**
   - Every `WallItem` spine → call `Barrier::createBarrier()` to extrude to outline polygon
   - Every `PolyWall` → use directly as polygon
   - Result: list of polygons (no distinction between original types)

2. **Compute tile grid:**
   ```
   levelArea = levelBounds.width * levelBounds.height
   tileSize = max(256, nextPow2(sqrt(levelArea / MAX_TILES))); for now MAX_TILES = 2048
   tileSize can be overridden with an optional setting in LevelFile, which will be implemented later
   ```
   This ensures tile count stays under ~2048 for any map size. Tile size adapts to map dimensions.

3. **Clip polygons to tiles:**
   - For each polygon, clip against all tiles it intersects using Clipper2 library (clipper1 and clipper2 are part of the project, we'll use clipper2 for this)
   - Each resulting fragment is a `WallPoly` with:
	 - `verts`: the clipped polygon vertices (flattened x,y pairs)
	 - `outline`: per-edge boolean indicating whether to render that edge
   - Edges introduced by clipping → `outline = false`
   - Original edges that survive clipping → `outline = true`

4. **Store per-connection delivery state:**
   ```cpp
   struct WallDeliveryState {
	   BitSet sentTiles;           // sentTiles[tileId] = true iff this tile has been sent; set to false to force resend of tile
	   priority_queue<tileId> pending;  // sorted by distance to ship
   };
   ```
   One instance per `GameConnection`.

5. **Store tiles in game database:**
   All `WallTile` objects are created at level load and placed in `getGameObjDatabase()`. They are **not** flagged `setScopeAlways()`.

### Server-side (Each Tick)

**Per active `GameConnection`:**

1. Compute the set of tiles currently in scope:
   - Player ship position + SCOPE_RADIUS (SCOPE_RADIUS = 1000 units)
   - If scope sharing is enabled, use union of all teammate positions
   - Query: all tiles whose AABB intersects this scope circle

2. For each tile in scope that hasn't been sent (`!sentTiles[tileId]`):
   - Compute distance from player to tile center
   - Enqueue into `pending` (priority queue, sorted closest-first)

3. Dequeue and send up to **N tiles per tick** (default: 1–3 for fog-of-war, higher for non-fog):
   - Send `s2cSendWallTile(tileId, Vector<WallPoly>)` RPC
   - Set `sentTiles[tileId] = true`

4. **Non-fog-of-war mode:** on level load, enqueue **all** tiles for all connections immediately. Same delivery mechanism, just with full coverage from the start.

### Wire Format

```cpp
struct WallPoly {
	Vector<F32> verts;      // flattened x,y pairs: [x0, y0, x1, y1, ..., xN, yN]
	Vector<bool> outline;   // outline[i] = render edge from vert[i] to vert[(i+1) % N]
							// size() == verts.size() / 2
};

struct WallTile {
	U16 tileId;             // tile index in the grid
	Vector<WallPoly> polys; // all wall polygons in this tile
};
```

**RPC:**
```cpp
s2cSendWallTile(U16 tileId, Vector<WallPoly> polys);
```

Sent as `RPCGuaranteedOrderedBigData` to ensure tiles are received in order (though delivery order is by distance, not sequentially).

### Client-side (On Receiving Tile)

```
On s2cSendWallTile(tileId, polys):
	for each WallPoly in polys:
		store polygon in client's wall database (permanent, never deleted)
		call constructWalls(game) to create rendering geometry

On render:
	for each stored WallPoly:
		fill polygon
		for each edge i where outline[i] == true:
			draw edge line
```

**Key: tiles are never un-sent.** Once a client receives a tile, it keeps that geometry forever (until level unload).

### Tile Updates via Script

If a Lua script modifies wall geometry inside an existing tile at runtime:

1. Server detects the change
2. Server re-clips the affected polygon(s) and updates the `WallTile`
3. Server clears `sentTiles[tileId]` for **all connections**
4. On next tick, tile is re-enqueued for all connections and re-sent
5. Client receives updated tile, replaces old geometry with new

This ensures clients eventually converge to the current state, but tiles are only re-sent if the geometry actually changes.

## Coordinate System & Tiling

**Tile ID computation:**
```cpp
U16 getTileId(Point pos, U32 tileSize, S32 gridWidth) {
	S32 gx = (S32)(pos.x / tileSize);
	S32 gy = (S32)(pos.y / tileSize);
	return (U16)(gy * gridWidth + gx);
}
```

**Tile bounds:**
```cpp
Rect getTileBounds(U16 tileId, U32 tileSize, S32 gridWidth) {
	S32 gx = tileId % gridWidth;
	S32 gy = tileId / gridWidth;
	Point ul(gx * tileSize, gy * tileSize);
	Point lr(ul.x + tileSize, ul.y + tileSize);
	return Rect(ul, lr);
}
```

All arithmetic is integer-based to ensure consistency between server and client.

## Rendering Behavior

### Seamless Tiling

Two adjacent tiles sharing a wall segment:
- **Tile A** contains the left polygon fragment; right edge is marked `outline = false`
- **Tile B** contains the right polygon fragment; left edge is marked `outline = false`
- Result: no line is drawn along the shared boundary, even though the geometry is present

### Partial Visibility During Streaming

As tiles arrive in closest-first order:
- Walls near the player render immediately upon tile receipt
- Distant portions of the map appear over a few seconds as tiles stream in
- Once a tile is received, that portion is permanently visible (even if the player later drives/flies away)

This behavior is identical for both fog-of-war and non-fog modes; the only difference is the scope radius used to determine which tiles are eligible for delivery.

## Configuration

**Server:**
```
TileSize = adaptive (computed at level load based on map size, can be overridden in level file)
MaxTiles = 2048 (unless more required to satisfy level file specifier)
TilesPerTickFogOfWar = 1  // send slowly to limit bandwidth
TilesPerTickNormal = 5    // send faster for non-fog maps
```

**Scope:**
```
ScopeRadius >= 1.5 * TileSize  // ensure player's tile and one ring around it are enqueued
```

## Backwards Compatibility

- `s2cAddWalls` RPC is **removed entirely**
- Existing level loading via `WallRec` and `Barrier::constructWalls()` remains unchanged; only the delivery mechanism changes
- Client behavior is identical (same rendering paths), only the source of geometry changes from bulk RPC to tiled stream

## Implementation Phases

### Phase 1: Server Tile Builder
- Compute tile grid size based on level bounds
- Extrude all `WallItem` spines to polygons
- Implement polygon clipping (clip each wall polygon against tile grid)
- Create `WallTile` objects and store in game database
- Remove `s2cAddWalls` from level-load RPC chain

### Phase 2: Server Tile Delivery
- Implement per-connection `WallDeliveryState` (sentTiles BitSet, pending queue)
- Add server tick hook to enqueue new tiles and drain delivery queue
- Implement `s2cSendWallTile` RPC
- Handle tile updates (script-driven geometry changes)

### Phase 3: Client Reception
- Receive and construct `WallPoly` geometry from `s2cSendWallTile`
- Verify rendering with `outline` flags works correctly
- Test seamless tiling (no seams at tile boundaries)

### Phase 4: Fog-of-War Gating
- Integrate scope radius computation into tile enqueueing logic
- Test closest-first delivery
- Verify that hacked clients cannot know about off-screen walls

### Phase 5: Mode Handling
- Implement full-tile-delivery path for non-fog-of-war games
- Implement scope-sharing path for teammate visibility (if feature exists)
- Test on small, medium, and large maps

## Open Questions

1. **Polygon clipping library:** Use Clipper2

2. **Scope sharing:** Not yet implmented; add a stub hook in the tile-enqueueing logic for future use.

3. **Performance profiling:** once Phase 3 is working, measure memory usage per connection (`BitSet` + queue overhead) and CPU cost of per-tick tile enqueueing on maps with thousands of tiles.

4. **Tile size sensitivity:** is 256 units (at small map scale) may not be the right minimum? Will tune with playtesting.

## Related Files

- **Server:** `gameType.cpp` (scope queries), `WallSegmentManager.cpp` (or new tile builder)
- **Network:** `gameType.h` (RPC declaration)
- **Client:** `gameObjectRender.cpp`, rendering code for `WallPoly`
- **Shared:** `barrier.h`, `barrier.cpp` (wall construction), new `WallTile` class
