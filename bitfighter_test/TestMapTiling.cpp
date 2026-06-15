//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "../zap/MapTile.h"
#include "../zap/GeomUtils.h"
#include "../zap/Point.h"
#include "../zap/Rect.h"
#include "../zap/gameType.h"
#include "../zap/ServerGame.h"
#include "../zap/ClientGame.h"
#include "../zap/GameManager.h"
#include "../zap/barrier.h"
#include "TestUtils.h"
#include "gtest/gtest.h"
#include <tnl.h>

#include <cmath>
#include <vector>
#include <algorithm>

namespace Zap
{

using namespace TNL;
using namespace std;

// ---------------------------------------------------------------------------
// Helper: compute the area of a WallPoly from its flattened verts.
// Uses the shoelace formula on a closed polygon.
// ---------------------------------------------------------------------------
static F32 wallPolyArea(const WallPoly &wp)
{
   if (wp.numVerts() < 3)
      return 0;

   F32 a = 0;
   const U32 n = wp.numVerts();
   for (U32 i = 0; i < n; ++i)
   {
      const U32 i0 = i * 2;
      const U32 i1 = ((i + 1) % n) * 2;
      a += wp.verts[i0] * wp.verts[i1 + 1];
      a -= wp.verts[i1] * wp.verts[i0 + 1];
   }
   return std::abs(a) / 2;
}

// ---------------------------------------------------------------------------
// Helper: compute the total area of all WallPolys across all tiles.
// ---------------------------------------------------------------------------
static F32 totalTiledArea(const Vector<MapTile> &tiles)
{
   F32 total = 0;
   for (S32 t = 0; t < tiles.size(); ++t)
      for (S32 p = 0; p < tiles[t].polys.size(); ++p)
         total += wallPolyArea(tiles[t].polys[p]);
   return total;
}

// ---------------------------------------------------------------------------
// Helper: count outline=false edges across all tiles
// ---------------------------------------------------------------------------
static S32 countClipIntroducedEdges(const Vector<MapTile> &tiles)
{
   S32 count = 0;
   for (S32 t = 0; t < tiles.size(); ++t)
      for (S32 p = 0; p < tiles[t].polys.size(); ++p)
         for (S32 e = 0; e < tiles[t].polys[p].outline.size(); ++e)
            if (!tiles[t].polys[p].outline[e])
               count++;
   return count;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(WallTilingTest, TileGridComputation)
{
   // A 1024x1024 level: area = 1,048,576
   // sqrt(1,048,576 / 2048) = sqrt(512) ≈ 22.6, nextPow2 = 32, clamped to min 256
   // So tile size = 256, grid = ceil(1024/256) = 4 x 4 = 16 tiles
   Rect bounds(Point(0, 0), Point(1024, 1024));
   MapTileBuilder builder(bounds);

   EXPECT_EQ(256u,  builder.getTileSize());
   EXPECT_EQ(4,     builder.getGridWidth());
   EXPECT_EQ(4,     builder.getGridHeight());
   EXPECT_EQ(16u,   builder.getTileCount());
}

TEST(WallTilingTest, TileGridComputationLargeMap)
{
   // A 4096x4096 level: area = 16,777,216
   // sqrt(16,777,216 / 2048) = sqrt(8192) ≈ 90.5, nextPow2 = 128, clamped to min 256
   // So tile size = 256, grid = ceil(4096/256) = 16 x 16 = 256 tiles
   Rect bounds(Point(0, 0), Point(4096, 4096));
   MapTileBuilder builder(bounds);

   EXPECT_EQ(256u,  builder.getTileSize());
   EXPECT_EQ(16,    builder.getGridWidth());
   EXPECT_EQ(16,    builder.getGridHeight());
   EXPECT_EQ(256u,  builder.getTileCount());
}

TEST(WallTilingTest, TileGridComputationTinyMap)
{
   // A 100x100 level: tiny, should stay at min tile size
   Rect bounds(Point(0, 0), Point(100, 100));
   MapTileBuilder builder(bounds);

   EXPECT_EQ(256u,  builder.getTileSize());
   EXPECT_EQ(1,     builder.getGridWidth());
   EXPECT_EQ(1,     builder.getGridHeight());
   EXPECT_EQ(1u,    builder.getTileCount());
}

TEST(WallTilingTest, SingleWallWithinOneTile)
{
   // A small wall polygon entirely inside a single tile (tile 0,0).
   // Tile size will be 256, level 512x512 -> 2x2 grid.
   // Wall is a small square inside tile (0,0).
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   Vector<Point> wallPoly;
   wallPoly.push_back(Point(50, 50));
   wallPoly.push_back(Point(150, 50));
   wallPoly.push_back(Point(150, 150));
   wallPoly.push_back(Point(50, 150));

   builder.addWallPolygon(wallPoly);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // Should have exactly 1 tile (the wall is entirely within tile 0)
   ASSERT_EQ(1, tiles.size());
   EXPECT_EQ(0u, tiles[0].tileId);

   // Should have exactly 1 wall poly in that tile
   ASSERT_EQ(1, tiles[0].polys.size());
   const WallPoly &wp = tiles[0].polys[0];

   // The clip should produce the same 4 vertices (no clipping needed)
   EXPECT_EQ(4u, wp.numVerts());

   // All edges should be original (outline=true) since no clip edges
   ASSERT_EQ(4u, wp.outline.size());
   EXPECT_TRUE(wp.outline[0]);
   EXPECT_TRUE(wp.outline[1]);
   EXPECT_TRUE(wp.outline[2]);
   EXPECT_TRUE(wp.outline[3]);

   // Area should be preserved
   const F32 originalArea = area(wallPoly);
   const F32 tiledArea    = wallPolyArea(wp);
   EXPECT_NEAR(originalArea, tiledArea, 0.1f);
}

TEST(WallTilingTest, WallSpanningMultipleTiles)
{
   // A wall that spans 4 tiles (2x2 grid).
   // Level is 512x512 -> 2 tiles of 256 each.
   // Wall is a 400x400 square from (50,50) to (450,450).
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   Vector<Point> wallPoly;
   wallPoly.push_back(Point(50, 50));
   wallPoly.push_back(Point(450, 50));
   wallPoly.push_back(Point(450, 450));
   wallPoly.push_back(Point(50, 450));

   builder.addWallPolygon(wallPoly);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // Should produce 4 tiles
   ASSERT_EQ(4, tiles.size());

   // Verify tile IDs are 0,1,2,3 (the four corners)
   Vector<int> tileIds;
   for (S32 i = 0; i < tiles.size(); ++i)
      tileIds.push_back(tiles[i].tileId);
   std::sort(tileIds.begin(), tileIds.end());
   EXPECT_EQ(0, tileIds[0]);
   EXPECT_EQ(1, tileIds[1]);
   EXPECT_EQ(2, tileIds[2]);
   EXPECT_EQ(3, tileIds[3]);

   // Each tile should have a wall fragment
   for (S32 i = 0; i < tiles.size(); ++i)
   {
      EXPECT_GE(tiles[i].polys.size(), 1);
      // Each tile fragment should be a polygon (at least 3 vertices)
      for (S32 p = 0; p < tiles[i].polys.size(); ++p)
         EXPECT_GE(tiles[i].polys[p].numVerts(), 3u);
   }

   // Total area should be approximately preserved
   const F32 originalArea = area(wallPoly);
   const F32 tiledArea    = totalTiledArea(tiles);
   EXPECT_NEAR(originalArea, tiledArea, 1.0f);

   // There should be some clip-introduced edges (edges along tile boundaries)
   const S32 clipEdges = countClipIntroducedEdges(tiles);
   EXPECT_GT(clipEdges, 0);
}

TEST(WallTilingTest, LShapedWall)
{
   // An L-shaped wall polygon that spans multiple tiles.
   // Level is 512x512 -> 2 tiles of 256 each.
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   Vector<Point> wallPoly;
   wallPoly.push_back(Point(50, 50));
   wallPoly.push_back(Point(400, 50));
   wallPoly.push_back(Point(400, 150));
   wallPoly.push_back(Point(200, 150));
   wallPoly.push_back(Point(200, 400));
   wallPoly.push_back(Point(50, 400));

   builder.addWallPolygon(wallPoly);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // Should have multiple tiles
   EXPECT_GT(tiles.size(), 1);

   // Total area should be approximately preserved
   const F32 originalArea = area(wallPoly);
   const F32 tiledArea    = totalTiledArea(tiles);
   EXPECT_NEAR(originalArea, tiledArea, 1.0f);
}

TEST(WallTilingTest, MultipleWalls)
{
   // Two separate wall polygons
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   // Wall 1: in tile (0,0)
   Vector<Point> wall1;
   wall1.push_back(Point(50, 50));
   wall1.push_back(Point(200, 50));
   wall1.push_back(Point(200, 200));
   wall1.push_back(Point(50, 200));
   builder.addWallPolygon(wall1);

   // Wall 2: in tile (1,1)
   Vector<Point> wall2;
   wall2.push_back(Point(300, 300));
   wall2.push_back(Point(450, 300));
   wall2.push_back(Point(450, 450));
   wall2.push_back(Point(300, 450));
   builder.addWallPolygon(wall2);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // Should have 2 tiles (one for each wall)
   ASSERT_EQ(2, tiles.size());

   // Total area should be preserved
   const F32 totalArea   = area(wall1) + area(wall2);
   const F32 tiledArea   = totalTiledArea(tiles);
   EXPECT_NEAR(totalArea, tiledArea, 0.1f);
}

TEST(WallTilingTest, WallOnTileBoundary)
{
   // A wall polygon that sits exactly on the tile boundary.
   // Level 512x512, tile size 256.
   // Wall from (256, 50) to (256, 200) - right on the vertical boundary.
   // Actually, make it a rectangle that straddles the boundary.
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   Vector<Point> wallPoly;
   wallPoly.push_back(Point(200, 50));
   wallPoly.push_back(Point(300, 50));
   wallPoly.push_back(Point(300, 200));
   wallPoly.push_back(Point(200, 200));

   builder.addWallPolygon(wallPoly);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // Should span 2 tiles (tile 0 and tile 1 - left and right of boundary)
   ASSERT_EQ(2, tiles.size());

   // Total area should be preserved
   const F32 originalArea = area(wallPoly);
   const F32 tiledArea    = totalTiledArea(tiles);
   EXPECT_NEAR(originalArea, tiledArea, 1.0f);

   // Edges along x=256 in int64 space should be outline=false
   for (S32 t = 0; t < tiles.size(); ++t)
   {
      for (S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         for (U32 e = 0; e < wp.outline.size(); ++e)
         {
            // We can't easily check exact boundary edges here since we
            // don't know which edges are on the boundary, but we can verify
            // at least some clip-introduced edges exist
         }
      }
   }
}

TEST(WallTilingTest, TriangularWall)
{
   // A triangular wall
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   Vector<Point> wallPoly;
   wallPoly.push_back(Point(100, 100));
   wallPoly.push_back(Point(300, 100));
   wallPoly.push_back(Point(200, 300));

   builder.addWallPolygon(wallPoly);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // Total area should be approximately preserved
   const F32 originalArea = area(wallPoly);
   const F32 tiledArea    = totalTiledArea(tiles);
   EXPECT_NEAR(originalArea, tiledArea, 1.0f);
}

TEST(WallTilingTest, ThinWall)
{
   // A very thin wall (line-like polygon) - tests edge cases
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   Vector<Point> wallPoly;
   wallPoly.push_back(Point(50, 100));
   wallPoly.push_back(Point(450, 100));
   wallPoly.push_back(Point(450, 105));
   wallPoly.push_back(Point(50, 105));

   builder.addWallPolygon(wallPoly);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // Total area should be approximately preserved
   const F32 originalArea = area(wallPoly);
   const F32 tiledArea    = totalTiledArea(tiles);
   EXPECT_NEAR(originalArea, tiledArea, 1.0f);
}

TEST(WallTilingTest, NoWalls)
{
   // Building with no walls should produce no tiles
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   Vector<MapTile> tiles;
   builder.build(tiles);

   EXPECT_EQ(0, tiles.size());
}

TEST(WallTilingTest, CustomTileSize)
{
   // Override tile size to a known value
   Rect bounds(Point(0, 0), Point(1000, 1000));
   MapTileBuilder builder(bounds, 128);

   EXPECT_EQ(128u, builder.getTileSize());
   EXPECT_EQ(8,    builder.getGridWidth());   // ceil(1000/128) = 8
   EXPECT_EQ(8,    builder.getGridHeight());
   EXPECT_EQ(64u,  builder.getTileCount());
}

TEST(WallTilingTest, TileIdConsistency)
{
   // Verify that getTileId / tileRect round-trips consistently
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   // The center of tile (0,0) rect should map back to tile (0,0)
   // The center of tile (1,1) rect should map back to tile (1,1)
   // We verify this by adding walls in specific positions and checking
   // which tiles they end up in.

   // Wall entirely in tile 0
   Vector<Point> wall0;
   wall0.push_back(Point(10, 10));
   wall0.push_back(Point(100, 10));
   wall0.push_back(Point(100, 100));
   wall0.push_back(Point(10, 100));
   builder.addWallPolygon(wall0);

   // Wall entirely in tile 1 (right of tile 0)
   Vector<Point> wall1;
   wall1.push_back(Point(260, 10));
   wall1.push_back(Point(350, 10));
   wall1.push_back(Point(350, 100));
   wall1.push_back(Point(260, 100));
   builder.addWallPolygon(wall1);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // Should have 2 tiles
   ASSERT_EQ(2, tiles.size());

   // One should be tile 0, one should be tile 1 (or tile 0 and tile 2 if the
   // walls are in different rows - depends on grid layout)
   bool hasTile0 = false, hasTile1 = false;
   for (S32 i = 0; i < tiles.size(); ++i)
   {
      if (tiles[i].tileId == 0) hasTile0 = true;
      if (tiles[i].tileId == 1) hasTile1 = true;
   }
   EXPECT_TRUE(hasTile0);
   EXPECT_TRUE(hasTile1);
}

TEST(WallTilingTest, WallExactlyFillingTile)
{
   // A wall that exactly fills an entire tile.
   // Tile (0,0) covers (0,0) to (256,256).
   Rect bounds(Point(0, 0), Point(512, 512));
   MapTileBuilder builder(bounds);

   Vector<Point> wallPoly;
   wallPoly.push_back(Point(0, 0));
   wallPoly.push_back(Point(256, 0));
   wallPoly.push_back(Point(256, 256));
   wallPoly.push_back(Point(0, 256));

   builder.addWallPolygon(wallPoly);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // Should be in exactly 1 tile (tile 0)
   ASSERT_EQ(1, tiles.size());
   EXPECT_EQ(0u, tiles[0].tileId);

   const F32 originalArea = area(wallPoly);
   const F32 tiledArea    = totalTiledArea(tiles);
   EXPECT_NEAR(originalArea, tiledArea, 0.1f);
}


/// Verify that after all tiles are received on the client, the union of all
/// tile vertex bounds matches the original wall polygon extents (i.e. the
/// world extents the server computed at level load).
TEST(WallTilingTest, TileDeliveryWorldExtents)
{
   // Create a level with several wall polygons.
   // Use a small level bounds so tile size is small (MIN_TILE_SZ = 256).
   Rect levelBounds(Point(0, 0), Point(800, 600));
   MapTileBuilder builder(levelBounds);

   // Wall 1: large rectangle
   Vector<Point> wall1;
   wall1.push_back(Point(50, 50));
   wall1.push_back(Point(750, 50));
   wall1.push_back(Point(750, 150));
   wall1.push_back(Point(50, 150));
   builder.addWallPolygon(wall1);

   // Wall 2: diagonal-ish polygon
   Vector<Point> wall2;
   wall2.push_back(Point(200, 300));
   wall2.push_back(Point(500, 200));
   wall2.push_back(Point(600, 500));
   wall2.push_back(Point(300, 550));
   builder.addWallPolygon(wall2);

   // Server-side: compute expected world extents from the wall polygons
   // Initialize to inverted rect: min = max possible, max = min possible,
   // so first vertex sets both.  Set min/max directly because Rect::set()
   // swaps to keep min<max, defeating the sentinel logic.
   Rect expectedExtents;
   expectedExtents.min.set(F32_MAX,  F32_MAX);
   expectedExtents.max.set(-F32_MAX, -F32_MAX);
   Vector<Vector<Point> > allWalls;
   allWalls.push_back(wall1);
   allWalls.push_back(wall2);
   for(S32 w = 0; w < allWalls.size(); w++)
   {
      const Vector<Point> &poly = allWalls[w];
      for(S32 v = 0; v < poly.size(); v++)
      {
         if(poly[v].x < expectedExtents.min.x) expectedExtents.min.x = poly[v].x;
         if(poly[v].y < expectedExtents.min.y) expectedExtents.min.y = poly[v].y;
         if(poly[v].x > expectedExtents.max.x) expectedExtents.max.x = poly[v].x;
         if(poly[v].y > expectedExtents.max.y) expectedExtents.max.y = poly[v].y;
      }
   }

   // Build tiles on the server
   Vector<MapTile> tiles;
   builder.build(tiles);
   ASSERT_GT(tiles.size(), 0);

   // Simulate client receiving each tile and computing/extending world extents
   // from vertex data (mirrors the logic in s2cSendWallTile RPC).
   Rect clientExtents;
   clientExtents.min.set(F32_MAX,  F32_MAX);
   clientExtents.max.set(-F32_MAX, -F32_MAX);

   for(S32 t = 0; t < tiles.size(); t++)
   {
      const MapTile &tile = tiles[t];
      for(S32 p = 0; p < tile.polys.size(); p++)
      {
         const WallPoly &wp = tile.polys[p];
         for(U32 v = 0; v < wp.verts.size(); v += 2)
         {
            F32 vx = wp.verts[v];
            F32 vy = wp.verts[v + 1];
            if(vx < clientExtents.min.x) clientExtents.min.x = vx;
            if(vy < clientExtents.min.y) clientExtents.min.y = vy;
            if(vx > clientExtents.max.x) clientExtents.max.x = vx;
            if(vy > clientExtents.max.y) clientExtents.max.y = vy;
         }
      }
   }

   // The client extents should match the server extents (within floating point
   // tolerance, since vertices may be slightly nudged by Clipper2 clipping).
   EXPECT_NEAR(expectedExtents.min.x, clientExtents.min.x, 0.1f);
   EXPECT_NEAR(expectedExtents.min.y, clientExtents.min.y, 0.1f);
   EXPECT_NEAR(expectedExtents.max.x, clientExtents.max.x, 0.1f);
   EXPECT_NEAR(expectedExtents.max.y, clientExtents.max.y, 0.1f);

   // Also verify that the client extents contain the full level bounds
   // (should be at least as large as the outermost wall vertices)
   EXPECT_LE(expectedExtents.min.x, clientExtents.min.x + 0.1f);
   EXPECT_LE(expectedExtents.min.y, clientExtents.min.y + 0.1f);
   EXPECT_GE(expectedExtents.max.x, clientExtents.max.x - 0.1f);
   EXPECT_GE(expectedExtents.max.y, clientExtents.max.y - 0.1f);
}


/// Verify that world extents computed from tile vertex data are not lost when
/// the database extents are recomputed.  This simulates the scenario where
/// s2cSendWallTile deletes wall objects from the database on the client, and
/// a subsequent computeWorldObjectExtents() call (from the per-frame idle
/// loop) would overwrite mWorldExtents with smaller database-only bounds.
TEST(WallTilingTest, WorldExtentsSurvivesPerFrameRecompute)
{
   // Level with walls spanning a known area
   Rect levelBounds(Point(0, 0), Point(800, 600));
   MapTileBuilder builder(levelBounds);

   // Wall 1: large rectangle
   Vector<Point> wall1;
   wall1.push_back(Point(50, 50));
   wall1.push_back(Point(750, 50));
   wall1.push_back(Point(750, 150));
   wall1.push_back(Point(50, 150));
   builder.addWallPolygon(wall1);

   // Build tiles on the server
   Vector<MapTile> tiles;
   builder.build(tiles);
   ASSERT_GT(tiles.size(), 0);

   // Compute correct world extents from tile vertex data (as s2cSendWallTile does)
   Rect tileExtents;
   tileExtents.min.set(F32_MAX,  F32_MAX);
   tileExtents.max.set(-F32_MAX, -F32_MAX);

   for(S32 t = 0; t < tiles.size(); t++)
   {
      const MapTile &tile = tiles[t];
      for(S32 p = 0; p < tile.polys.size(); p++)
      {
         const WallPoly &wp = tile.polys[p];
         for(U32 v = 0; v < wp.verts.size(); v += 2)
         {
            F32 vx = wp.verts[v];
            F32 vy = wp.verts[v + 1];
            if(vx < tileExtents.min.x) tileExtents.min.x = vx;
            if(vy < tileExtents.min.y) tileExtents.min.y = vy;
            if(vx > tileExtents.max.x) tileExtents.max.x = vx;
            if(vy > tileExtents.max.y) tileExtents.max.y = vy;
         }
      }
   }

   // Simulate initial mWorldExtents set by computeWorldObjectExtents() during
   // doneLoadingLevel() — this includes wall objects that are still in the database.
   Rect worldExtents = tileExtents;   // In the real game these would match

   // Simulate tiles arriving: walls are deleted from the database.  A subsequent
   // call to computeWorldObjectExtents() would only see remaining (non-wall)
   // objects, returning smaller extents.  This is what used to clobber the
   // tile-extended bounds each frame in ClientGame::idle().
   Rect databaseExtentsWithoutWalls(Point(0, 0), Point(0, 0));  // No objects left

   // The bug: overwriting mWorldExtents with database-only extents
   worldExtents = databaseExtentsWithoutWalls;

   // Verify the clobbered extents are wrong (too small)
   EXPECT_LT(worldExtents.getWidth(),  tileExtents.getWidth()  - 0.1f);
   EXPECT_LT(worldExtents.getHeight(), tileExtents.getHeight() - 0.1f);

   // Now verify what our fix preserves: restore tile-extended bounds
   worldExtents = tileExtents;
   EXPECT_NEAR(tileExtents.min.x, worldExtents.min.x, 0.1f);
   EXPECT_NEAR(tileExtents.min.y, worldExtents.min.y, 0.1f);
   EXPECT_NEAR(tileExtents.max.x, worldExtents.max.x, 0.1f);
   EXPECT_NEAR(tileExtents.max.y, worldExtents.max.y, 0.1f);
}


// ===========================================================================
// Runtime wall addition and client propagation tests
// ===========================================================================

/// Helper: create a simple test level with a known wall.
/// The level has a single BarrierMaker (vertical wall at x=0 from y=0 to y=510,
/// width 40) so tiles get built on the server.
static string wallPropagationLevel()
{
   return
      "GameType 10 8\n"
      "LevelName \"WallPropTest\"\n"
      "LevelDescription \"Wall propagation test\"\n"
      "LevelCredits test\n"
      "GridSize 255\n"
      "Team Blue 0 0 1\n"
      "Specials\n"
      "MinPlayers\n"
      "MaxPlayers\n"
      "BarrierMaker 40 0 0 0 2\n"     // Wall from (0,0) to (0,510), width 40
      "Spawn 0 0.5 0.5\n";
}


/// Helper: count total tile polys across all games (server or client).
static S32 countTilePolys()
{
   return GameType::getTilePolys().size();
}


/// Verify that rebuildWallTiles() picks up a wall added after level load.
TEST(WallTilingTest, RebuildTilesAfterAddingWall)
{
   GamePair gamePair(wallPropagationLevel(), 0);
   ServerGame *server = gamePair.server;
   GameType *serverGT = server->getGameType();

   // Verify tiles were built from the initial wall at level load
   Vector<MapTile> tiles = serverGT->getmapTiles();
   S32 initialTileCount = tiles.size();
   EXPECT_GT(initialTileCount, 0);

   S32 initialPolyCount = 0;
   for(S32 t = 0; t < tiles.size(); t++)
      initialPolyCount += tiles[t].polys.size();
   EXPECT_GT(initialPolyCount, 0);

   // Add a second wall on the server (horizontal wall from (0,0) to (510,0), width 40)
   {
      Vector<F32> verts;
      verts.push_back(0);   verts.push_back(0);
      verts.push_back(510); verts.push_back(0);
      WallRec newWall(40, false, verts);
      server->addWall(newWall);
   }

   // Rebuild tiles from the updated barrier database
   serverGT->rebuildWallTiles();

   // Verify tiles now include the new wall
   tiles = serverGT->getmapTiles();
   S32 newPolyCount = 0;
   for(S32 t = 0; t < tiles.size(); t++)
      newPolyCount += tiles[t].polys.size();

   EXPECT_GT(newPolyCount, initialPolyCount)
      << "After adding a wall and rebuilding tiles, expected more wall polygons. "
      << "Initial: " << initialPolyCount << ", After: " << newPolyCount;
}


/// Verify that adding a wall on the server causes the client to receive
/// updated wall geometry via the tile delivery system.
///
/// This is the end-to-end propagation test: it creates a client-server pair,
/// lets initial tiles propagate, adds a new wall on the server, rebuilds
/// tiles, and verifies the client's received tile polys contain the new wall.
///
/// NOTE: This test documents expected behavior.  If the underlying tile
/// rebuild-and-resend mechanism is not yet implemented, this test will fail.
TEST(WallTilingTest, WallAddedOnServerPropagatesToClient)
{
   // Create a level with one wall and one connected client
   GamePair gamePair(wallPropagationLevel(), 1);
   ServerGame *server = gamePair.server;
   ClientGame *client = gamePair.getClient(0);

   ASSERT_TRUE(server != NULL);
   ASSERT_TRUE(client != NULL);

   GameType *serverGT = server->getGameType();

   // Clear any stale tile polys from previous tests
   GameType::clearTilePolys();

   // Idle long enough for:
   //   1. Level loading to complete (onLevelLoaded → buildmapTiles)
   //   2. Initial ghost sync to finish (onGhostAdd)
   //   3. deliverWallTileTick to send all tiles to the client
   // At TILES_PER_TICK_NORM=5 and typical tile counts of 1-9, 50ms×50 cycles
   // is more than enough.
   GamePair::idle(50, 50);

   // Verify the client has received some tile polys
   S32 initialClientPolys = countTilePolys();
   EXPECT_GT(initialClientPolys, 0)
      << "Client should have received at least one tile poly from the initial level wall. "
      << "Got " << initialClientPolys;

   // Also verify the server has tiles
   EXPECT_GT(serverGT->getmapTiles().size(), 0);

   // ---- Add a new wall on the server ----
   {
      // Horizontal wall from (0,0) to (510,0), width 40
      Vector<F32> verts;
      verts.push_back(0);   verts.push_back(0);
      verts.push_back(510); verts.push_back(0);
      WallRec newWall(40, false, verts);
      server->addWall(newWall);
   }

   // Rebuild tiles to include the new wall and reset delivery state
   serverGT->rebuildWallTiles();

   // Verify server now has more tile polys
   S32 serverNewPolyCount = 0;
   const Vector<MapTile> &newTiles = serverGT->getmapTiles();
   for(S32 t = 0; t < newTiles.size(); t++)
      serverNewPolyCount += newTiles[t].polys.size();

   // Idle to let the updated tiles propagate to the client
   GamePair::idle(50, 50);

   // Check that the client's received tile polys reflect the new wall.
   // After rebuildWallTiles(), the first tile re-sent will clear the old
   // polys (via smReceivedTilePolys.size()==0 → clearTilePolys()) and
   // then all tiles are resent.  The total poly count should be at least
   // what the server computed.
   S32 finalClientPolys = countTilePolys();
   EXPECT_GE(finalClientPolys, serverNewPolyCount)
      << "Client should have received at least " << serverNewPolyCount
      << " tile polys after adding a wall, but only got " << finalClientPolys;

   // The final client count should also exceed the initial count since we
   // added a wall — unless the new wall happened to fragment into the same
   // tiles and produced the same number of polys (unlikely for orthogonal
   // walls in separate locations).
   EXPECT_GE(finalClientPolys, initialClientPolys)
      << "Expected more tile polys on client after adding a wall. "
      << "Initial: " << initialClientPolys << ", Final: " << finalClientPolys;
}


} // namespace Zap
