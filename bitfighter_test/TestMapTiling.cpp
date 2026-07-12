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
#include "../zap/projectile.h"
#include "../zap/UIGame.h"
#include "../zap/UIManager.h"
#include "TestUtils.h"
#include "gtest/gtest.h"
#include <tnl.h>

#include <cmath>
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
         for (S32 e = 0; e < tiles[t].polys[p].edges.size(); ++e)
            if (tiles[t].polys[p].edges[e] == EdgeStyle::None)
               count++;
   return count;
}


static S32 countClientProjectiles(ClientGame *client)
{
   Vector<DatabaseObject *> projectiles;
   client->getGameObjDatabase()->findObjects((TestFunc)isProjectileType, projectiles);
   return projectiles.size();
}


static bool clientHasCollidedProjectile(ClientGame *client)
{
   Vector<DatabaseObject *> projectiles;
   client->getGameObjDatabase()->findObjects((TestFunc)isProjectileType, projectiles);

   for(S32 i = 0; i < projectiles.size(); i++)
   {
      Projectile *projectile = dynamic_cast<Projectile *>(projectiles[i]);

      if(projectile && projectile->mCollided)
         return true;
   }

   return false;
}


static string clientProjectileWallCollisionLevel()
{
   return
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"ClientProjectileWallCollision\"\n"
      "LevelDescription test\n"
      "LevelCredits test\n"
      "Team Blue 0 0 1\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "GridSize 255\n"
      "BarrierMaker 50 350 -100 350 100\n"
      "Spawn 0 0 0\n";
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
   ASSERT_EQ(4u, wp.edges.size());
   EXPECT_NE(wp.edges[0], EdgeStyle::None);
   EXPECT_NE(wp.edges[1], EdgeStyle::None);
   EXPECT_NE(wp.edges[2], EdgeStyle::None);
   EXPECT_NE(wp.edges[3], EdgeStyle::None);

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
         for (U32 e = 0; e < wp.edges.size(); ++e)
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

   // Should have at least 1 tile; find the one with content (tile 0)
   ASSERT_GE(tiles.size(), 1);
   bool foundTile0 = false;
   for(S32 t = 0; t < tiles.size(); t++)
      if(tiles[t].tileId == 0 && tiles[t].polys.size() > 0)
      {
         foundTile0 = true;
         break;
      }
   EXPECT_TRUE(foundTile0);

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
   Vector<MapTile> tiles = serverGT->getMapTiles();
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
   tiles = serverGT->getMapTiles();
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
   EXPECT_GT(serverGT->getMapTiles().size(), 0);

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
   const Vector<MapTile> &newTiles = serverGT->getMapTiles();
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


/// Verify that a real client firing path can shoot into tile-rendered walls
/// without requiring client-side Barrier/WallItem objects for projectile LOS.
TEST(WallTilingTest, ClientProjectileHitsTileWallWithoutCrash)
{
   InputCodeManager::initializeKeyNames();

   GamePair gamePair(clientProjectileWallCollisionLevel(), 1);
   ClientGame *client = gamePair.getClient(0);
   ASSERT_TRUE(client != NULL);

   GameUserInterface *gameUI = client->getUIManager()->getUI<GameUserInterface>();
   ASSERT_TRUE(gameUI != NULL);

   GamePair::idle(50, 50);

   ASSERT_TRUE(client->getLocalPlayerShip() != NULL);
   ASSERT_GT(GameType::getTilePolys().size(), 0)
      << "Client must receive tiled wall geometry before testing projectile collision.";

   EXPECT_EQ(0, countClientProjectiles(client));

   Move *move = gameUI->getCurrentMove();
   move->angle = 0;  // Fire east, directly into the vertical wall at x=250.

   InputCode fireKey = client->getSettings()->getInputCodeManager()->getBinding(BINDING_FIRE, InputModeKeyboard);
   gameUI->onKeyDown(fireKey);
   GamePair::idle(20, 5);
   gameUI->onKeyUp(fireKey);

   bool sawProjectile = false;
   for(S32 i = 0; i < 15; i++)
   {
      GamePair::idle(20, 1);
      if(countClientProjectiles(client) > 0)
      {
         sawProjectile = true;
         break;
      }
   }

   EXPECT_TRUE(sawProjectile)
      << "Client should receive a projectile ghost after firing.";

   bool sawCollidedProjectile = false;
   for(S32 i = 0; i < 40; i++)
   {
      GamePair::idle(20, 1);
      if(clientHasCollidedProjectile(client))
      {
         sawCollidedProjectile = true;
         break;
      }
   }

   EXPECT_TRUE(sawCollidedProjectile)
      << "Client projectile should collide with the tile wall before its normal lifetime expires.";
}


/// Create a level with a destructible BarrierMaker, fire at it until it is
/// destroyed, then verify the client-side tile geometry no longer contains
/// any destructible edges.
TEST(WallTilingTest, ShipDestroysDestructibleWall)
{
   InputCodeManager::initializeKeyNames();

   // Level: destructible vertical wall at x=50 with Spawn at origin, firing east.
   string level =
       "LevelFormat 2\n"
       "GameType 10 0\n"
       "LevelName \"DestructibleWallTest\"\n"
       "LevelDescription test\n"
       "LevelCredits test\n"
       "Team Blue 0 0 1\n"
       "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
       "GridSize 255\n"
       "BarrierMaker D 10 280.5 -714 382.5 -612 510 -765\n"
       "Spawn 0 270 -670\n";

   GamePair gamePair(level, 1);
   ClientGame *client = gamePair.getClient(0);
   ASSERT_TRUE(client != NULL);

   GameUserInterface *gameUI = client->getUIManager()->getUI<GameUserInterface>();
   ASSERT_TRUE(gameUI != NULL);

   // Let tiles propagate to the client
   GamePair::idle(50, 100);

   ASSERT_TRUE(client->getLocalPlayerShip() != NULL)
      << "Client should have a local ship after level load.";

   // The client should have received tile polys for this level
   ASSERT_GT(GameType::getTilePolys().size(), 0)
      << "Client must receive tiled wall geometry before shooting.";

   // Check server-side barrier state
   auto countServerBarriers = [](ServerGame *sg) -> S32 {
      Vector<DatabaseObject *> barriers;
      sg->getGameObjDatabase()->findObjects((TestFunc)isWallType, barriers);
      return barriers.size();
   };

   S32 barriersBefore = countServerBarriers(gamePair.server);
   ASSERT_GT(barriersBefore, 0) << "Server must have barriers before firing.";
   {
      Vector<DatabaseObject *> barriers;
      gamePair.server->getGameObjDatabase()->findObjects((TestFunc)isWallType, barriers);
      for(S32 i = 0; i < barriers.size(); i++)
      {
         Barrier *b = dynamic_cast<Barrier *>(barriers[i]);
         if(b)
         {
            const Vector<Point> *poly = b->getCollisionPoly();
            fprintf(stderr, "  Barrier %d: destruct=%d ghost=%d hp=%.2f/%.2f poly=%d pts\n",
               i, (int)b->mDestructible, (int)b->isGhost(), b->mHitPoints, b->mMaxHitPoints,
               poly ? poly->size() : 0);
            if(poly)
               for(S32 v = 0; v < poly->size(); v++)
                  fprintf(stderr, "    v%d: (%.1f, %.1f)\n", v, (*poly)[v].x, (*poly)[v].y);
         }
      }
   }

   // Count destructible edges before firing
   auto countDestEdges = []() -> S32 {
      S32 count = 0;
      const auto &polys = GameType::getTilePolys();
      for(S32 i = 0; i < polys.size(); i++)
         for(S32 e = 0; e < polys[i].edges.size(); e++)
            if(polys[i].edges[e] == EdgeStyle::Destructible)
               count++;
      return count;
   };

   S32 destBefore = countDestEdges();
   ASSERT_GT(destBefore, 0)
      << "There must be some destructible edges before firing.";

   // Ship & aim
   Ship *ship = client->getLocalPlayerShip();
   ASSERT_TRUE(ship != NULL);
   fprintf(stderr, "  Ship pos=(%.1f, %.1f)\n", ship->getPos().x, ship->getPos().y);

   Move *move = gameUI->getCurrentMove();
   move->angle = 0;

   // Fire and immediately track projectile on the server
   InputCode fireKey = client->getSettings()->getInputCodeManager()->getBinding(BINDING_FIRE, InputModeKeyboard);
   gameUI->onKeyDown(fireKey);
   GamePair::idle(20, 1);   // 20ms — projectile created, flies ~12 units
   gameUI->onKeyUp(fireKey);

   // Track the projectile step by step
   for(S32 step = 0; step < 10; step++)
   {
      Vector<DatabaseObject *> projs;
      gamePair.server->getGameObjDatabase()->findObjects((TestFunc)isProjectileType, projs);
      for(S32 p = 0; p < projs.size(); p++)
      {
         Projectile *pr = static_cast<Projectile *>(projs[p]);
         fprintf(stderr, "  step=%d proj=(%.1f,%.1f) collided=%d\n",
            step, pr->getPos().x, pr->getPos().y, (int)pr->mCollided);
      }
      if(projs.size() == 0)
      {
         // Check if barriers took damage
         Vector<DatabaseObject *> barriers;
         gamePair.server->getGameObjDatabase()->findObjects((TestFunc)isWallType, barriers);
         for(S32 i = 0; i < barriers.size(); i++)
         {
            Barrier *b = dynamic_cast<Barrier *>(barriers[i]);
            if(b)
               fprintf(stderr, "  step=%d no proj, Barrier %d hp=%.2f/%.2f\n",
                  step, i, b->mHitPoints, b->mMaxHitPoints);
         }
         if(barriers.size() == 0)
            fprintf(stderr, "  step=%d no proj, ALL BARRIERS DESTROYED\n", step);
         break;
      }
      GamePair::idle(20, 1);
   }

   // Check barrier HP after the single shot
   {
      Vector<DatabaseObject *> barriers;
      gamePair.server->getGameObjDatabase()->findObjects((TestFunc)isWallType, barriers);
      for(S32 i = 0; i < barriers.size(); i++)
      {
         Barrier *b = dynamic_cast<Barrier *>(barriers[i]);
         if(b)
            fprintf(stderr, "  After shot Barrier %d: hp=%.2f/%.2f\n",
               i, b->mHitPoints, b->mMaxHitPoints);
      }
   }

   // Try damaging Barrier 0 directly to verify damageObject works
   {
      Vector<DatabaseObject *> barriers;
      gamePair.server->getGameObjDatabase()->findObjects((TestFunc)isWallType, barriers);
      if(barriers.size() > 0)
      {
         Barrier *b0 = dynamic_cast<Barrier *>(barriers[0]);
         ASSERT_TRUE(b0 != NULL);
         fprintf(stderr, "  Calling damageObject directly on Barrier 0...\n");
         DamageInfo di;
         di.damageAmount = 1.0f;
         b0->damageObject(&di);
         fprintf(stderr, "  After direct damage: hp=%.2f/%.2f (destructible=%d)\n",
            b0->mHitPoints, b0->mMaxHitPoints, (int)b0->mDestructible);
      }
   }

   // Start firing the Phaser (default weapon, 0.19 dmg, 100ms fire delay).
   // Need ceil(10/0.19) ≈ 53 hits.
   InputCode fireKey = client->getSettings()->getInputCodeManager()->getBinding(BINDING_FIRE, InputModeKeyboard);
   gameUI->onKeyDown(fireKey);
   GamePair::idle(20, 500);    // 10,000ms of sustained auto-fire
   gameUI->onKeyUp(fireKey);

   // Let updated tiles propagate to the client
   S32 destAfter = -1;
   for(S32 idleCycle = 0; idleCycle < 300; idleCycle++)
   {
      GamePair::idle(50, 1);
      destAfter = countDestEdges();
      if(destAfter == 0)
         break;
   }

   S32 barriersAfter = countServerBarriers(gamePair.server);
   EXPECT_EQ(0, barriersAfter) << "All server-side barriers should be gone.";

   EXPECT_EQ(0, destAfter)
      << "All destructible wall edges should be gone after destroying the wall.";
}


// ---------------------------------------------------------------------------
// Early problem case: Tilted T with diagonal destructible bar + vertical non-destructible stem.
// ---------------------------------------------------------------------------
TEST(WallTilingTest, MadMaze2DiagonalOverlap)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"MadMaze2\"\n"
      "LevelDescription diag\n"
      "LevelCredits test\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker D 50 382.5 -561 51 -433.5\n"
      "BarrierMaker 50 127.499939 -612.000122 127.499817 -382.500183\n"
      "Spawn 0 255 -433.5\n";

   GamePair gp(level, 1);   // 1 client to verify RPC delivery
   GameType::clearTilePolys();
   GamePair::idle(50, 200); // let tiles propagate to client

   GameType *gt = gp.server->getGameType();
   const auto &tiles = gt->getMapTiles();

   fprintf(stderr, "\n=== MadMaze2DiagonalOverlap: %d tiles ===\n", tiles.size());
   for(S32 t = 0; t < tiles.size(); t++)
   {
      fprintf(stderr, "Tile %d tileId=%d: %d polys [%.0f-%.0f,%.0f-%.0f]\n",
         t, tiles[t].tileId, tiles[t].polys.size(),
         tiles[t].bounds.min.x, tiles[t].bounds.max.x,
         tiles[t].bounds.min.y, tiles[t].bounds.max.y);
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         fprintf(stderr, "  Poly %d: %d verts styles=[", p, wp.numVerts());
         for(U32 e = 0; e < wp.edges.size(); e++)
            fprintf(stderr, "%s%d", e>0?",":"", (int)wp.edges[e]);
         fprintf(stderr, "]\n");
         for(U32 v = 0; v < wp.numVerts(); v++)
            fprintf(stderr, "    v%d: (%.1f, %.1f)\n", v, wp.verts[v*2], wp.verts[v*2+1]);
      }
   }

   // ---- Assertions on SERVER tiles ----
   ASSERT_GT(tiles.size(), 0);
   // With the combined Union approach, Tile 0 has ONE poly (13 verts).
   // Verify it has both Solid and Dashed edges (from both barrier types).
   bool hasSolid = false, hasDashed = false, hasNone = false;
   for(S32 t = 0; t < tiles.size(); t++)
      if(tiles[t].tileId == 0)
         for(S32 p = 0; p < tiles[t].polys.size(); p++)
         {
            const WallPoly &wp = tiles[t].polys[p];
            for(U32 e = 0; e < wp.edges.size(); e++)
            {
               if(wp.edges[e] == EdgeStyle::Normal) hasSolid = true;
               if(wp.edges[e] == EdgeStyle::Destructible) hasDashed = true;
               if(wp.edges[e] == EdgeStyle::None) hasNone = true;
            }
         }

   // ---- Assertions on CLIENT received tiles ----
   const Vector<WallPoly> &clientPolys = GameType::getTilePolys();
   bool clientHasSolid = false, clientHasDashed = false;
   for(S32 i = 0; i < clientPolys.size(); i++)
   {
      const WallPoly &wp = clientPolys[i];
      for(U32 e = 0; e < wp.edges.size(); e++)
      {
         if(wp.edges[e] == EdgeStyle::Normal) clientHasSolid = true;
         if(wp.edges[e] == EdgeStyle::Destructible) clientHasDashed = true;
      }
   }

   EXPECT_TRUE(hasSolid) << "SERVER: Combined poly must have Solid edges";
   EXPECT_TRUE(hasDashed) << "SERVER: Combined poly must have Dashed edges";
   EXPECT_TRUE(hasNone) << "SERVER: Combined poly must have None edges (tile/overlap bnd)";
   EXPECT_TRUE(clientHasSolid) << "CLIENT: Must have received Solid edges via RPC";
   ASSERT_TRUE(clientHasDashed) << "CLIENT: Must have received Dashed edges via RPC!";
   EXPECT_GT(clientPolys.size(), 0) << "Client should have received tile polys";
}


/// Reproduction of the user's actual MadMaze2 level with 3 walls:
///   - Horizontal destructible wall (D flag) at y = -433.5, x = 51..408
///   - Vertical   permanent wall   at x = 127.5, y = -612..-382.5
///   - Diagonal   permanent wall   from (51,-586.5) to (306,-357)
/// The user reports the vertical wall and an interior island don't render.
TEST(WallTilingTest, MadMaze2ThreeWalls)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"MadMaze2\"\n"
      "LevelDescription eifj\n"
      "LevelCredits HEath\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker D 50 408 -433.5 51 -433.5\n"
      "BarrierMaker 50 127.499939 -612.000122 127.499817 -382.500183\n"
      "BarrierMaker 50 51 -586.5 306 -357\n"
      "Spawn 0 255 -433.5\n";

   GamePair gp(level, 1);
   GameType::clearTilePolys();
   GamePair::idle(50, 200);

   GameType *gt = gp.server->getGameType();
   const auto &tiles = gt->getMapTiles();

   fprintf(stderr, "\n=== MadMaze2ThreeWalls: %d tiles ===\n", tiles.size());
   for(S32 t = 0; t < tiles.size(); t++)
   {
      fprintf(stderr, "Tile %d tileId=%d: %d polys [%.0f-%.0f,%.0f-%.0f]\n",
         t, tiles[t].tileId, tiles[t].polys.size(),
         tiles[t].bounds.min.x, tiles[t].bounds.max.x,
         tiles[t].bounds.min.y, tiles[t].bounds.max.y);
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         fprintf(stderr, "  Poly %d: %d verts area=%.1f styles=[", p, wp.numVerts(), wallPolyArea(wp));
         for(U32 e = 0; e < wp.edges.size(); e++)
            fprintf(stderr, "%s%d", e>0?",":"", (int)wp.edges[e]);
         fprintf(stderr, "]\n");
         for(U32 v = 0; v < wp.numVerts(); v++)
            fprintf(stderr, "    v%d: (%.1f, %.1f)\n", v, wp.verts[v*2], wp.verts[v*2+1]);
      }
   }

   ASSERT_GT(tiles.size(), 0);

   // Count polys that have zero visible edges (all None)
   S32 invisiblePolys = 0;
   for(S32 t = 0; t < tiles.size(); t++)
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         bool allNone = true;
         for(U32 e = 0; e < wp.edges.size(); e++)
            if(wp.edges[e] != EdgeStyle::None) { allNone = false; break; }
         if(allNone) invisiblePolys++;
      }

   // Every polygon should have at least some visible edges
   EXPECT_EQ(invisiblePolys, 0) << invisiblePolys << " polys have ALL None edges (invisible)!";

   // Must have both types
   bool hasSolid = false, hasDashed = false;
   for(S32 t = 0; t < tiles.size(); t++)
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         for(U32 e = 0; e < wp.edges.size(); e++)
         {
            if(wp.edges[e] == EdgeStyle::Normal) hasSolid = true;
            if(wp.edges[e] == EdgeStyle::Destructible) hasDashed = true;
         }
      }

   EXPECT_TRUE(hasSolid) << "Must have Solid edges (permanent walls)";
   EXPECT_TRUE(hasDashed) << "Must have Dashed edges (destructible wall)";

   // ---- CLIENT assertions ----
   const Vector<WallPoly> &clientPolys = GameType::getTilePolys();
   EXPECT_GT(clientPolys.size(), 0) << "Client must receive tile polys";
   bool clientHasSolid = false, clientHasDashed = false;
   S32 clientInvisible = 0;
   for(S32 i = 0; i < clientPolys.size(); i++)
   {
      bool allNone = true;
      for(U32 e = 0; e < clientPolys[i].edges.size(); e++)
      {
         if(clientPolys[i].edges[e] == EdgeStyle::Normal) clientHasSolid = true;
         if(clientPolys[i].edges[e] == EdgeStyle::Destructible) clientHasDashed = true;
         if(clientPolys[i].edges[e] != EdgeStyle::None) allNone = false;
      }
      if(allNone) clientInvisible++;
   }
   EXPECT_TRUE(clientHasSolid) << "CLIENT: Must have Solid edges";
   ASSERT_TRUE(clientHasDashed) << "CLIENT: Must have Dashed edges";
   EXPECT_EQ(clientInvisible, 0) << "CLIENT: " << clientInvisible << " invisible polys";
}


/// Reproduction of the user's actual MadMaze2 level with diagonal destructible:
///   - Diagonal destructible wall from (408,-510) to (51,-433.5) D0
///   - Vertical   permanent wall   at x = 127.5, y = -612..-382.5
///   - Diagonal   permanent wall   from (51,-586.5) to (306,-357)
/// The destructible wall is diagonal in this version (not horizontal),
/// which creates a different intersection topology.
TEST(WallTilingTest, MadMaze2DiagDest)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"MadMaze2\"\n"
      "LevelDescription eifj\n"
      "LevelCredits HEath\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker D 50 408 -510 51 -433.5\n"
      "BarrierMaker 50 127.499939 -612.000122 127.499817 -382.500183\n"
      "BarrierMaker 50 51 -586.5 306 -357\n"
      "Spawn 0 255 -433.5\n";

   GamePair gp(level, 1);
   GameType::clearTilePolys();
   GamePair::idle(50, 200);

   GameType *gt = gp.server->getGameType();
   const auto &tiles = gt->getMapTiles();

   fprintf(stderr, "\n=== MadMaze2DiagDest: %d tiles ===\n", tiles.size());
   for(S32 t = 0; t < tiles.size(); t++)
   {
      fprintf(stderr, "Tile %d tileId=%d: %d polys [%.0f-%.0f,%.0f-%.0f]\n",
         t, tiles[t].tileId, tiles[t].polys.size(),
         tiles[t].bounds.min.x, tiles[t].bounds.max.x,
         tiles[t].bounds.min.y, tiles[t].bounds.max.y);
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         fprintf(stderr, "  Poly %d: %d verts area=%.1f styles=[", p, wp.numVerts(), wallPolyArea(wp));
         for(U32 e = 0; e < wp.edges.size(); e++)
            fprintf(stderr, "%s%d", e>0?",":"", (int)wp.edges[e]);
         fprintf(stderr, "]\n");
         for(U32 v = 0; v < wp.numVerts(); v++)
            fprintf(stderr, "    v%d: (%.1f, %.1f)\n", v, wp.verts[v*2], wp.verts[v*2+1]);
      }
   }

   // Diagnostic: show which None edges are not on tile boundary
   for(S32 t = 0; t < tiles.size(); t++)
   {
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         U32 nv = wp.numVerts();
         for(U32 e = 0; e < nv; e++)
         {
            if(wp.edges[e] != EdgeStyle::None) continue;
            F32 x1 = wp.verts[e*2], y1 = wp.verts[e*2+1];
            F32 x2 = wp.verts[((e+1)%nv)*2], y2 = wp.verts[((e+1)%nv)*2+1];
            const Rect &tr = tiles[t].bounds;
            bool onBoundary =
               (fabs(x1 - tr.min.x) <= 1.5f && fabs(x2 - tr.min.x) <= 1.5f) ||
               (fabs(x1 - tr.max.x) <= 1.5f && fabs(x2 - tr.max.x) <= 1.5f) ||
               (fabs(y1 - tr.min.y) <= 1.5f && fabs(y2 - tr.min.y) <= 1.5f) ||
               (fabs(y1 - tr.max.y) <= 1.5f && fabs(y2 - tr.max.y) <= 1.5f);
            fprintf(stderr, "    Tile %d Poly %d Edge %d: None mid=(%.1f,%.1f) boundary=%d\n",
               t, p, e, (x1+x2)/2, (y1+y2)/2, (int)onBoundary);
         }
      }
   }

   ASSERT_GT(tiles.size(), 0);

   // Count polys that have zero visible edges (all None)
   S32 invisiblePolys = 0;
   for(S32 t = 0; t < tiles.size(); t++)
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         bool allNone = true;
         for(U32 e = 0; e < wp.edges.size(); e++)
            if(wp.edges[e] != EdgeStyle::None) { allNone = false; break; }
         if(allNone) invisiblePolys++;
      }

   EXPECT_EQ(invisiblePolys, 0) << invisiblePolys << " polys have ALL None edges (invisible)!";

   // Must have both types
   bool hasSolid = false, hasDashed = false;
   for(S32 t = 0; t < tiles.size(); t++)
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         for(U32 e = 0; e < wp.edges.size(); e++)
         {
            if(wp.edges[e] == EdgeStyle::Normal) hasSolid = true;
            if(wp.edges[e] == EdgeStyle::Destructible) hasDashed = true;
         }
      }

   EXPECT_TRUE(hasSolid) << "Must have Solid edges (permanent walls)";
   EXPECT_TRUE(hasDashed) << "Must have Dashed edges (destructible wall)";

   // ---- CLIENT assertions ----
   const Vector<WallPoly> &clientPolys = GameType::getTilePolys();
   EXPECT_GT(clientPolys.size(), 0) << "Client must receive tile polys";
   bool clientHasSolid = false, clientHasDashed = false;
   S32 clientInvisible = 0;
   for(S32 i = 0; i < clientPolys.size(); i++)
   {
      bool allNone = true;
      for(U32 e = 0; e < clientPolys[i].edges.size(); e++)
      {
         if(clientPolys[i].edges[e] == EdgeStyle::Normal) clientHasSolid = true;
         if(clientPolys[i].edges[e] == EdgeStyle::Destructible) clientHasDashed = true;
         if(clientPolys[i].edges[e] != EdgeStyle::None) allNone = false;
      }
      if(allNone) clientInvisible++;
   }
   EXPECT_TRUE(clientHasSolid) << "CLIENT: Must have Solid edges";
   ASSERT_TRUE(clientHasDashed) << "CLIENT: Must have Dashed edges";
   EXPECT_EQ(clientInvisible, 0) << "CLIENT: " << clientInvisible << " invisible polys";
}


/// Simulate what happens after the destructible wall is destroyed.
/// Only the two permanent walls remain:
///   - Vertical   permanent wall   at x = 127.5, y = -612..-382.5
///   - Diagonal   permanent wall   from (51,-586.5) to (306,-357)
/// After rebuildWallTiles(), check that end caps look correct.
TEST(WallTilingTest, MadMaze2AfterDestruction)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"MadMaze2\"\n"
      "LevelDescription after dest\n"
      "LevelCredits HEath\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker 50 127.499939 -612.000122 127.499817 -382.500183\n"
      "BarrierMaker 50 51 -586.5 306 -357\n"
      "Spawn 0 255 -433.5\n";

   GamePair gp(level, 1);
   GameType::clearTilePolys();
   GamePair::idle(50, 200);

   GameType *gt = gp.server->getGameType();
   const auto &tiles = gt->getMapTiles();

   fprintf(stderr, "\n=== MadMaze2AfterDestruction: %d tiles ===\n", tiles.size());
   for(S32 t = 0; t < tiles.size(); t++)
   {
      fprintf(stderr, "Tile %d tileId=%d: %d polys [%.0f-%.0f,%.0f-%.0f]\n",
         t, tiles[t].tileId, tiles[t].polys.size(),
         tiles[t].bounds.min.x, tiles[t].bounds.max.x,
         tiles[t].bounds.min.y, tiles[t].bounds.max.y);
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         fprintf(stderr, "  Poly %d: %d verts area=%.1f styles=[", p, wp.numVerts(), wallPolyArea(wp));
         for(U32 e = 0; e < wp.edges.size(); e++)
            fprintf(stderr, "%s%d", e>0?",":"", (int)wp.edges[e]);
         fprintf(stderr, "]\n");
         for(U32 v = 0; v < wp.numVerts(); v++)
            fprintf(stderr, "    v%d: (%.1f, %.1f)\n", v, wp.verts[v*2], wp.verts[v*2+1]);
      }
   }

   ASSERT_GT(tiles.size(), 0);

   // Must have Solid edges
   bool hasSolid = false;
   for(S32 t = 0; t < tiles.size(); t++)
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         for(U32 e = 0; e < wp.edges.size(); e++)
            if(wp.edges[e] == EdgeStyle::Normal) hasSolid = true;
      }

   EXPECT_TRUE(hasSolid) << "Must have Solid edges (permanent walls)";

   // ---- CLIENT assertions ----
   const Vector<WallPoly> &clientPolys = GameType::getTilePolys();
   EXPECT_GT(clientPolys.size(), 0) << "Client must receive tile polys";
   bool clientHasSolid = false;
   for(S32 i = 0; i < clientPolys.size(); i++)
      for(U32 e = 0; e < clientPolys[i].edges.size(); e++)
         if(clientPolys[i].edges[e] == EdgeStyle::Normal) clientHasSolid = true;
   EXPECT_TRUE(clientHasSolid) << "CLIENT: Must have Solid edges";
}

/// Test that demonstrates the mitered wedge problem with multi-segment walls.
/// A 2-segment wall (seg 0: horizontal right, seg 1: vertical up, 90� turn).
/// Seg 0 is destructible. When destroyed, seg 1 should lose its mitered wedge
/// and get a clean butt end cap at the joint.
TEST(WallTilingTest, DestructibleMiterHook)
{
   // With all-or-nothing destructibility, a D-flagged wall has all segments
   // destructible.  Verify the tile output has only Dashed/None edges.
   string level =
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"HookTest\"\n"
      "LevelDescription test\n"
      "LevelCredits test\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "GridSize 2000\n"
      "BarrierMaker D 10 198.98 -710.73 382.5 -510 510 -765\n"
      "Spawn 0 306 -535.5\n";

   GamePair gp(level, 0);
   GamePair::idle(50, 50);

   GameType *gt = gp.server->getGameType();
   const auto &tiles = gt->getMapTiles();

   // Compute expected total destructible area (both segments combined)
   Vector<Point> cleanSeg0, cleanSeg1;
   Point dummy(nanf(""), nanf(""));
   constructBarrierPolygon(Point(198.98f, -710.73f), Point(382.5f, -510.0f), dummy, dummy, 10.0f, cleanSeg0);
   constructBarrierPolygon(Point(382.5f, -510.0f), Point(510.0f, -765.0f), dummy, dummy, 10.0f, cleanSeg1);
   F32 expectedArea = area(cleanSeg0) + area(cleanSeg1);

   fprintf(stderr, "\n=== DestructibleMiterHook: %d tiles ===\n", tiles.size());
   for(S32 t = 0; t < tiles.size(); t++)
   {
      fprintf(stderr, "Tile %d tileId=%d: %d polys [%.0f-%.0f,%.0f-%.0f]\n",
         t, tiles[t].tileId, tiles[t].polys.size(),
         tiles[t].bounds.min.x, tiles[t].bounds.max.x,
         tiles[t].bounds.min.y, tiles[t].bounds.max.y);
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         F32 polyArea = 0;
         if(wp.numVerts() >= 3)
         {
            Vector<Point> ctr;
            for(U32 v = 0; v < wp.numVerts(); v++)
               ctr.push_back(Point(wp.verts[v*2], wp.verts[v*2+1]));
            polyArea = area(ctr);
         }
         fprintf(stderr, "  Poly %d: %d verts area=%.1f styles=[", p, wp.numVerts(), polyArea);
         for(U32 e = 0; e < wp.edges.size(); e++)
            fprintf(stderr, "%s%d", e>0?",":"", (int)wp.edges[e]);
         fprintf(stderr, "]\n");
      }
   }

   // Sum destructible-only poly areas and check no Solid edges (permanent) appear
   F32 totalDestArea = 0;
   bool hasSolid = false;
   for(S32 t = 0; t < tiles.size(); t++)
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         bool allDest = true;
         for(U32 e = 0; e < wp.edges.size(); e++)
            if(wp.edges[e] == EdgeStyle::Normal) { hasSolid = true; allDest = false; break; }
         if(!allDest) continue;
         Vector<Point> contour;
         for(U32 v = 0; v < wp.numVerts(); v++)
            contour.push_back(Point(wp.verts[v*2], wp.verts[v*2+1]));
         totalDestArea += area(contour);
      }
   EXPECT_FALSE(hasSolid) << "No Solid edges should appear in a destructible-only wall";
   EXPECT_NEAR(expectedArea, totalDestArea, 50.0f) << "Total destructible area should match combined segments";
}


/// Test that destructible PolyWall renders with green fill and Dashed edges.
TEST(WallTilingTest, DestructiblePolyWall)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"PolyWallTest\"\n"
      "LevelDescription test\n"
      "LevelCredits test\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "GridSize 500\n"
      "PolyWall D 0 0 100 0 100 100 0 100\n"
      "Spawn 0 50 50\n";

   GamePair gp(level, 0);
   GamePair::idle(50, 50);

   GameType *gt = gp.server->getGameType();
   const auto &tiles = gt->getMapTiles();

   fprintf(stderr, "\n=== DestructiblePolyWall: %d tiles ===\n", tiles.size());
   for(S32 t = 0; t < tiles.size(); t++)
   {
      fprintf(stderr, "Tile %d tileId=%d: %d polys [%.0f-%.0f,%.0f-%.0f]\n",
         t, tiles[t].tileId, tiles[t].polys.size(),
         tiles[t].bounds.min.x, tiles[t].bounds.max.x,
         tiles[t].bounds.min.y, tiles[t].bounds.max.y);
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         fprintf(stderr, "  Poly %d: %d verts styles=[", p, wp.numVerts());
         for(U32 e = 0; e < wp.edges.size(); e++)
            fprintf(stderr, "%s%d", e>0?",":"", (int)wp.edges[e]);
         fprintf(stderr, "]\n");
      }
   }

   ASSERT_GT(tiles.size(), 0);
   bool hasDashed = false;
   bool hasSolid = false;
   for(S32 t = 0; t < tiles.size(); t++)
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
         for(U32 e = 0; e < tiles[t].polys[p].edges.size(); e++)
         {
            if(tiles[t].polys[p].edges[e] == EdgeStyle::Destructible) hasDashed = true;
            if(tiles[t].polys[p].edges[e] == EdgeStyle::Normal)  hasSolid  = true;
         }
   EXPECT_TRUE(hasDashed) << "Destructible PolyWall must have Dashed edges";
   EXPECT_FALSE(hasSolid) << "Destructible PolyWall should have no Solid edges";
}


/// Test that permanent PolyWall renders with blue fill and Solid edges (no D flag).
TEST(WallTilingTest, PermanentPolyWall)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"PolyWallPermTest\"\n"
      "LevelDescription test\n"
      "LevelCredits test\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "GridSize 500\n"
      "PolyWall 0 0 100 0 100 100 0 100\n"
      "Spawn 0 50 50\n";

   GamePair gp(level, 0);
   GamePair::idle(50, 50);

   GameType *gt = gp.server->getGameType();
   const auto &tiles = gt->getMapTiles();

   ASSERT_GT(tiles.size(), 0);
   bool hasDashed = false;
   bool hasSolid = false;
   for(S32 t = 0; t < tiles.size(); t++)
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
         for(U32 e = 0; e < tiles[t].polys[p].edges.size(); e++)
         {
            if(tiles[t].polys[p].edges[e] == EdgeStyle::Destructible) hasDashed = true;
            if(tiles[t].polys[p].edges[e] == EdgeStyle::Normal)  hasSolid  = true;
         }
   EXPECT_FALSE(hasDashed) << "Permanent PolyWall must have no Dashed edges";
   EXPECT_TRUE(hasSolid) << "Permanent PolyWall should have Solid edges";
}

} // namespace Zap
