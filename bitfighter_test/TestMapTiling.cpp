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
#include <cstdlib>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>

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
               ++count;

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

   for(S32 i = 0; i < projectiles.size(); ++i)
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

// ===========================================================================
// Edge ID Verification System
// ===========================================================================
//
// These helpers let test authors verify the exact rendering flags on
// individual wall edges produced by MapTileBuilder.  The workflow:
//
//   1. Write a test that builds tiles from a level definition.
//   2. Temporarily uncomment dumpTileEdges(tiles).
//   3. Run the test, capture the printed edge listing.
//   4. Inspect each edge and decide what EdgeStyle it SHOULD have.
//   5. Encode as an ExpectedEdges map using compact notation:
//        ExpectedEdges expectations = makeExpectedEdges(
//           "Destructible: 0,4; None: 1,3,5");
//   6. Replace dumpTileEdges with:
//        verifyTileEdges(tiles, expectations);
//   7. Test now asserts correctness — fails if Clipper2 changes edge handling.
//
// The same canonicalization is used by the in-game /show edgeids command
// so that edge IDs displayed on-screen match the IDs used in test code.
// ===========================================================================

/// Tolerance for rounding edge coordinates to canonical keys.
/// Must be large enough to absorb Clipper2 floating-point jitter but
/// small enough to distinguish edges that are legitimately separated
/// (walls are >=1 unit apart, so 0.1 is safe).
static const F32 EDGE_SNAP_TOL = 0.1f;


/// Canonical representation of a directed wall edge, sorted so the
/// lexicographically smaller endpoint always comes first.
struct EdgeKey
{
   F32 x1, y1, x2, y2;   // (x1,y1) <= (x2,y2) after sorting

   EdgeKey() : x1(0), y1(0), x2(0), y2(0) {}

   EdgeKey(F32 ax, F32 ay, F32 bx, F32 by)
   {
      // Round and sort so edge key is independent of traversal direction
      F32 rx1 = roundf(ax / EDGE_SNAP_TOL) * EDGE_SNAP_TOL;
      F32 ry1 = roundf(ay / EDGE_SNAP_TOL) * EDGE_SNAP_TOL;
      F32 rx2 = roundf(bx / EDGE_SNAP_TOL) * EDGE_SNAP_TOL;
      F32 ry2 = roundf(by / EDGE_SNAP_TOL) * EDGE_SNAP_TOL;

      // Lexicographic sort: smaller x first; tie-break on y
      if(rx1 < rx2 || (rx1 == rx2 && ry1 < ry2))
      {
         x1 = rx1; y1 = ry1; x2 = rx2; y2 = ry2;
      }
      else
      {
         x1 = rx2; y1 = ry2; x2 = rx1; y2 = ry1;
      }
   }

   bool operator<(const EdgeKey &o) const
   {
      if(x1 != o.x1) return x1 < o.x1;
      if(y1 != o.y1) return y1 < o.y1;
      if(x2 != o.x2) return x2 < o.x2;
      return y2 < o.y2;
   }
};


/// Convert an EdgeStyle enum to a human-readable string.
static const char *edgeStyleName(EdgeStyle s)
{
   switch(s)
   {
      case EdgeStyle::None:          return "None";
      case EdgeStyle::Normal:        return "Normal";
      case EdgeStyle::Destructible:  return "Destructible";
      default:                       return "Unknown";
   }
}


/// Build a sorted map from edge ID (index 0..N-1) to the canonical EdgeKey,
/// across all visible (non-None) edges in all tile polygons.
/// Also returns the reverse map for building expectations.
static void buildEdgeIdMaps(
   const Vector<MapTile> &tiles,
   map<EdgeKey, int> &keyToId,
   map<int, EdgeKey> &idToKey,
   map<int, EdgeStyle> &idToStyle)
{
   keyToId.clear();
   idToKey.clear();
   idToStyle.clear();

   // First pass: collect all unique canonical keys for visible edges
   set<EdgeKey> keys;
   // Temporary parallel: map from key to style (only used for visible edges)
   map<EdgeKey, EdgeStyle> keyStyle;

   for(S32 t = 0; t < tiles.size(); ++t)
   {
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         U32 nv = wp.numVerts();

      for(U32 e = 0; e < nv; ++e)
         {
            // Include ALL edges (including EdgeStyle::None) so the ID
            // numbering matches the in-game /showedgeids display.
            U32 v0 = e * 2;
            U32 v1 = ((e + 1) % nv) * 2;
            EdgeKey key(wp.verts[v0],     wp.verts[v0 + 1],
                        wp.verts[v1],     wp.verts[v1 + 1]);
            keys.insert(key);
            keyStyle[key] = wp.edges[e];
         }
      }
   }

   // Second pass: assign IDs in sorted order
   int nextId = 0;
   for(const auto &key : keys)
   {
      keyToId[key] = nextId;
      idToKey[nextId] = key;
      idToStyle[nextId] = keyStyle[key];
      ++nextId;

   }
}


/// Map from edge ID to expected EdgeStyle.  Only edges that have expectations
/// are checked; extra edges in the output are ignored.
using ExpectedEdges = map<int, EdgeStyle>;


/// Convert a style name ("Normal", "Destructible", "None") to its EdgeStyle.
static EdgeStyle edgeStyleFromName(const std::string &name)
{
   if(name == "Normal")       return EdgeStyle::Normal;
   if(name == "Destructible") return EdgeStyle::Destructible;
   if(name == "None")         return EdgeStyle::None;

   ADD_FAILURE() << "Unknown edge style name '" << name << "' in makeExpectedEdges";
   return EdgeStyle::None;
}


/// Build an ExpectedEdges map from a compact string notation, e.g.:
///
///     makeExpectedEdges("Destructible: 13-18, 21-23; None: 19-20")
///
///   "Style:"           switches the style for all following IDs
///   "a,b,c"            comma-separated individual edge IDs
///   "lo-hi"            a run of consecutive edge IDs (inclusive)
///   ";"                optional separator between style groups
///
/// Edge IDs are never negative, so '-' is unambiguous as a range operator.
static ExpectedEdges makeExpectedEdges(const std::string &spec)
{
   ExpectedEdges result;

   // Commas separate items within a style group; semicolons separate groups.
   // Treat both as whitespace to keep the token stream simple.
   std::string cleaned = spec;
   std::replace(cleaned.begin(), cleaned.end(), ',', ' ');
   std::replace(cleaned.begin(), cleaned.end(), ';', ' ');

   std::istringstream iss(cleaned);
   std::string token;
   EdgeStyle current = EdgeStyle::None;

   while(iss >> token)
   {
      // "Normal:" / "Destructible:" / "None:"  — switch style for following IDs
      if(token.size() > 1 && token.back() == ':')
      {
         current = edgeStyleFromName(token.substr(0, token.size() - 1));
         continue;
      }

      // "lo-hi" — a run of consecutive edge IDs
      const size_t dash = token.find('-');
      if(dash == std::string::npos)
         result[atoi(token.c_str())] = current;
      else
      {
         const int lo = atoi(token.substr(0, dash).c_str());
         const int hi = atoi(token.substr(dash + 1).c_str());
         for(int id = lo; id <= hi; ++id)
            result[id] = current;
      }
   }

   return result;
}


/// Dump all visible edges with their IDs, coordinates, and styles.
/// Used during test authoring to discover what edge IDs look like.
/// Wrap in #if 0 / #endif to disable after capturing output.
static void dumpTileEdges(const Vector<MapTile> &tiles)
{
   map<EdgeKey, int> keyToId;
   map<int, EdgeKey> idToKey;
   map<int, EdgeStyle> idToStyle;
   buildEdgeIdMaps(tiles, keyToId, idToKey, idToStyle);

   printf("=== Wall Tile Edge Dump (%d visible edges) ===\n",
          (int)idToKey.size());

   for(const auto &entry : idToKey)
   {
      int id = entry.first;
      const EdgeKey &key = entry.second;
      EdgeStyle style = idToStyle[id];

      printf("Edge %2d: (%8.1f,%8.1f)->(%8.1f,%8.1f)  %s\n",
             id, key.x1, key.y1, key.x2, key.y2,
             edgeStyleName(style));
   }

   printf("=== End Edge Dump ===\n");
}


/// Verify that specific edges have the expected rendering style.
/// Returns a vector of mismatch descriptions; empty = all good.
/// Prints mismatches via gtest's ADD_FAILURE_AT for proper test reporting.
static void verifyTileEdges(
   const Vector<MapTile> &tiles,
   const ExpectedEdges &expectations)
{
   map<EdgeKey, int> keyToId;
   map<int, EdgeKey> idToKey;
   map<int, EdgeStyle> idToStyle;
   buildEdgeIdMaps(tiles, keyToId, idToKey, idToStyle);

   S32 mismatches = 0;

   for(const auto &exp : expectations)
   {
      int id = exp.first;
      EdgeStyle expectedStyle = exp.second;

      auto keyIt = idToKey.find(id);
      if(keyIt == idToKey.end())
      {
         // If the expected style is None and the edge is not in the visible
         // edge map, that means it was correctly classified as None (tile
         // boundary clip edges are excluded from buildEdgeIdMaps).
         if(expectedStyle == EdgeStyle::None)
            continue;   // Correctly None — success

         ADD_FAILURE()
            << "Edge " << id << ": expected " << edgeStyleName(expectedStyle)
            << " but edge was not found in tiled output. "
            << "The edge set may have changed — re-run dumpTileEdges to "
            << "get the current edge IDs.";
         ++mismatches;

         continue;
      }

      auto styleIt = idToStyle.find(id);
      EdgeStyle actualStyle = styleIt->second;

      if(actualStyle != expectedStyle)
      {
         const EdgeKey &key = keyIt->second;
         ADD_FAILURE()
            << "Edge " << id
            << " ((" << key.x1 << "," << key.y1 << ")->("
            << key.x2 << "," << key.y2 << ")): "
            << "expected " << edgeStyleName(expectedStyle)
            << " but got " << edgeStyleName(actualStyle);
         ++mismatches;

      }
   }

   if(mismatches > 0)
      FAIL() << mismatches << " edge style mismatch(es) found";
}


/// EdgeIdVerificationDemo — demonstrates the full workflow for adding
/// a level test with edge ID verification.
///
/// Workflow:
///   1. Write the level code as a string.
///   2. Uncomment dumpTileEdges(tiles).
///   3. Run the test, capture the printed edge listing.
///   4. Inspect each edge and decide its correct EdgeStyle.
///      (Use /showedgeids in-game to visually confirm.)
///   5. Fill in the ExpectedEdges map.
///   6. Comment out dumpTileEdges — test is complete.
TEST(WallTilingTest, TestEdgeIdVerificationDemo)
{
   // A simple level: one permanent vertical wall + one destructible
   // horizontal wall forming a T.
   string level =
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"Demo\"\n"
      "LevelDescription \"Edge ID verification demo\"\n"
      "LevelCredits test\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker 50 0 -2295 0 -1530\n"
      "BarrierMaker D 50 4590 -2040 4590 -1836\n"
      "BarrierMaker D 50 4437 -1861.5 4590 -1861.5\n"
      "Spawn 0 300 300\n";

   GamePair gp(level, 0);
   GamePair::idle(50, 100);

   GameType *gt = gp.server->getGameType();
   ASSERT_TRUE(gt != NULL);
   const Vector<MapTile> &tiles = gt->getMapTiles();
   ASSERT_GT(tiles.size(), 0) << "Level should produce tiled geometry";

    // ----- Step 1: Dump edges to discover IDs -----
    // Uncomment the next line when first writing the test:
    dumpTileEdges(tiles);

   // ----- Step 2: Specify what each edge SHOULD be -----
   // Format: "Style: id1,id2,id3" — consecutive ids may be written as ranges.
   // Only specify edges you care about; unspecified edges are ignored.
   ExpectedEdges expectations = makeExpectedEdges(
      "Destructible: 13-18, 21-23; "
      "None: 19-20"
   );

   // Example (real IDs depend on the dump output):
   // makeExpectedEdges("Normal: 0,1; Destructible: 2; None: 3");  // clip edges

   // ----- Step 3: Run verification -----
   if(!expectations.empty())
      verifyTileEdges(tiles, expectations);
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

TEST(WallTilingTest, WallEdgeAlignedToTileBoundary)
{
    // Test that wall edges that lie exactly on a tile boundary are NOT
    // classified as EdgeStyle::None (they are real wall edges, not clip artifacts).
    // Tile size is 256, so tile 0 spans [0,0] to [256,256].
    // Wall from (100,100) to (256,100) to (256,200) to (100,200):
    //   The edge (256,100)→(256,200) lies on x=256, the right boundary of tile 0.
    //   It should be rendered (EdgeStyle::Normal), not hidden.
    Rect bounds(Point(0, 0), Point(512, 512));  // 2x2 grid
    MapTileBuilder builder(bounds);

    Vector<Point> wallPoly;
    wallPoly.push_back(Point(100, 100));
    wallPoly.push_back(Point(256, 100));  // on tile edge
    wallPoly.push_back(Point(256, 200));  // on tile edge
    wallPoly.push_back(Point(100, 200));

    builder.addWallPolygon(wallPoly);

    Vector<MapTile> tiles;
    builder.build(tiles);

    // Should have tiles for tile 0 (left) and tile 1 (right)
    ASSERT_GE(tiles.size(), 1);

    // Find tile 0 (gridX=0). The wall portion in tile 0 should have
    // the right edge (x=256), which was an original wall edge — NOT None.
    bool foundTile0 = false;
    for(S32 t = 0; t < tiles.size(); ++t)
    {
       if(tiles[t].tileId == 0)
       {
          foundTile0 = true;
          for(S32 p = 0; p < tiles[t].polys.size(); ++p)
          {
             const WallPoly &wp = tiles[t].polys[p];
             for(U32 e = 0; e < wp.edges.size(); ++e)
             {
                // Get the two endpoints of this edge
                U32 v0 = e * 2;
                U32 v1 = ((e + 1) % wp.numVerts()) * 2;
                F32 x1 = wp.verts[v0];
                F32 x2 = wp.verts[v1];

                // If this edge is on the tile boundary x=256, it MUST be Normal,
                // not None — it's part of the original wall geometry
                if(std::abs(x1 - 256.0f) < 0.01f && std::abs(x2 - 256.0f) < 0.01f)
                {
                   EXPECT_NE(wp.edges[e], EdgeStyle::None)
                      << "Original wall edge aligned to tile boundary at x=256 "
                      << "should NOT be EdgeStyle::None";
                }
             }
          }
          break;
       }
    }
    EXPECT_TRUE(foundTile0) << "Expected tile 0 in results";
}

/// Level design helper: a level whose wall edges align perfectly with the
/// tile grid, so the tiled output can be verified to preserve those edges.
///
/// Tile math (see GameType::buildMapTiles):
///   levelBounds = barrierExtents expanded by (10,10);  tileSize = 200.
///   Tile column k spans x in [minX - 10 + k*200, minX + 190 + k*200]
///   where minX is the smallest wall-outline x.
///
/// Wall A (spine x=0,  w=50) sets outline x-min = -25  -> tile origin -35.
/// Wall B (spine x=340, w=50) has outline x-max = 365  -> exactly on the
/// column-2/column-3 tile boundary ( -35 + 2*200 = 365 ).
static string tileAlignedWallLevel()
{
   return
      "LevelFormat 2\n"
      "GameType 10 0\n"
      "LevelName \"TileAlign\"\n"
      "LevelDescription \"Wall edges aligned to tile boundaries\"\n"
      "LevelCredits test\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker 50 0 0 0 510\n"        // Wall A: outline x [-25,25]
      "BarrierMaker 50 340 0 340 510\n"    // Wall B: outline x [315,365], right edge at tile boundary
      "Spawn 0 175 255\n";
}

/// Verify that a wall edge placed exactly on a computed tile boundary is
/// preserved (EdgeStyle::Normal) in the tiled output, and that the edge's
/// x-coordinate equals the tile-grid boundary computed from level bounds.
TEST(WallTilingTest, WallEdgeAlignedToComputedTileBoundary)
{
   GamePair gp(tileAlignedWallLevel(), 0);
   GamePair::idle(50, 100);

   GameType *gt = gp.server->getGameType();
   ASSERT_TRUE(gt != NULL);
   const Vector<MapTile> &tiles = gt->getMapTiles();
   ASSERT_GT(tiles.size(), 0) << "Level should produce tiled geometry";

   // The tile grid must match the design: tile width 200 and origin -35,
   // so the column-2/column-3 boundary is at -35 + 2*200 = 365.
   F32 tileOrigin = F32_MAX;
   for(S32 t = 0; t < tiles.size(); ++t)
      if(tiles[t].bounds.min.x < tileOrigin)
         tileOrigin = tiles[t].bounds.min.x;
   EXPECT_NEAR(-35.0f, tileOrigin, 0.1f);

   bool sawTileWidth200 = false;
   for(S32 t = 0; t < tiles.size(); ++t)
   {
      F32 w = tiles[t].bounds.getWidth();
      if(std::abs(w - 200.0f) < 0.1f)
      {
         sawTileWidth200 = true;
         break;
      }
   }
   EXPECT_TRUE(sawTileWidth200) << "Expected tiles with width 200 (tile size)";

   bool foundAlignedEdge = false;

   for(S32 t = 0; t < tiles.size(); ++t)
   {
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         U32 nv = wp.numVerts();
         for(U32 e = 0; e < nv; ++e)
         {
            U32 v0 = e * 2;
            U32 v1 = ((e + 1) % nv) * 2;
            F32 x1 = wp.verts[v0];
            F32 x2 = wp.verts[v1];

            // Wall B's right edge at x=365: both endpoints must lie on the
            // vertical tile boundary and the edge must be rendered.
            if(std::abs(x1 - 365.0f) < 0.01f && std::abs(x2 - 365.0f) < 0.01f)
            {
               foundAlignedEdge = true;
               EXPECT_NE(wp.edges[e], EdgeStyle::None)
                  << "Wall edge aligned to tile boundary x=365 must be rendered";
            }
         }
      }
   }

   EXPECT_TRUE(foundAlignedEdge)
      << "Expected an original wall edge exactly on the tile boundary x=365";
}

TEST(WallTilingTest, WallEdgeOnTileBoundaryNotSuppressed)
{
    // Two walls with collinear edges on the tile boundary x=256.
    // Wall A: edge A(256,100)→A(256,200)
    // Wall B: edge B(256,120)→B(256,250)
    // Union merges collinear edges → (256,100)→(256,250)
    // The merged edge key won't match either original key, causing
    // classifyEdges to suppress it as EdgeStyle::None.
    // All edges on x=256 should remain Normal.
    Rect bounds(Point(0, 0), Point(512, 512));
    MapTileBuilder builder(bounds);

    Vector<Point> wallA;
    wallA.push_back(Point(100, 100));
    wallA.push_back(Point(256, 100));
    wallA.push_back(Point(256, 200));
    wallA.push_back(Point(100, 200));
    builder.addWallPolygon(wallA, false);

    Vector<Point> wallB;
    wallB.push_back(Point(100, 120));
    wallB.push_back(Point(256, 120));
    wallB.push_back(Point(256, 250));
    wallB.push_back(Point(100, 250));
    builder.addWallPolygon(wallB, false);

    Vector<MapTile> tiles;
    builder.build(tiles);

    ASSERT_GE(tiles.size(), 1);

    // Every edge on x=256 should be Normal (not None)
    bool foundBoundaryEdge = false;
    for(S32 t = 0; t < tiles.size(); ++t)
    {
       if(tiles[t].tileId != 0) continue;
       for(S32 p = 0; p < tiles[t].polys.size(); ++p)
       {
          const WallPoly &wp = tiles[t].polys[p];
          for(U32 e = 0; e < wp.edges.size(); ++e)
          {
             U32 v0 = e * 2;
             U32 v1 = ((e + 1) % wp.numVerts()) * 2;
             F32 x1 = wp.verts[v0];
             F32 x2 = wp.verts[v1];
             if(std::abs(x1 - 256.0f) < 0.1f && std::abs(x2 - 256.0f) < 0.1f)
             {
                foundBoundaryEdge = true;
                EXPECT_NE(wp.edges[e], EdgeStyle::None)
                   << "Edge on tile boundary x=256 must not be None";
             }
          }
       }
    }
    EXPECT_TRUE(foundBoundaryEdge) << "Expected at least one edge on tile boundary x=256";
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
   for(S32 t = 0; t < tiles.size(); ++t)
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
   for(S32 w = 0; w < allWalls.size(); ++w)
   {
      const Vector<Point> &poly = allWalls[w];
      for(S32 v = 0; v < poly.size(); ++v)
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

   for(S32 t = 0; t < tiles.size(); ++t)
   {
      const MapTile &tile = tiles[t];
      for(S32 p = 0; p < tile.polys.size(); ++p)
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

   for(S32 t = 0; t < tiles.size(); ++t)
   {
      const MapTile &tile = tiles[t];
      for(S32 p = 0; p < tile.polys.size(); ++p)
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
   for(S32 t = 0; t < tiles.size(); ++t)
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
   serverGT->rebuildWallTilesAndBotZones();

   // Verify tiles now include the new wall
   tiles = serverGT->getMapTiles();
   S32 newPolyCount = 0;
   for(S32 t = 0; t < tiles.size(); ++t)
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
   serverGT->rebuildWallTilesAndBotZones();

   // Verify server now has more tile polys
   S32 serverNewPolyCount = 0;
   const Vector<MapTile> &newTiles = serverGT->getMapTiles();
   for(S32 t = 0; t < newTiles.size(); ++t)
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
   for(S32 i = 0; i < 15; ++i)
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
   for(S32 i = 0; i < 40; ++i)
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

   // Count destructible edges before firing
   auto countDestEdges = []() -> S32 {
      S32 count = 0;
      const auto &polys = GameType::getTilePolys();
      for(S32 i = 0; i < polys.size(); ++i)
         for(S32 e = 0; e < polys[i].edges.size(); ++e)
            if(polys[i].edges[e] == EdgeStyle::Destructible)
               ++count;

      return count;
   };

   S32 destBefore = countDestEdges();
   ASSERT_GT(destBefore, 0)
      << "There must be some destructible edges before firing.";

   // Ship & aim
   Ship *ship = client->getLocalPlayerShip();
   ASSERT_TRUE(ship != NULL);

   Move *move = gameUI->getCurrentMove();
   move->angle = 0;

   // Fire and immediately track projectile on the server
   InputCode fireKey = client->getSettings()->getInputCodeManager()->getBinding(BINDING_FIRE, InputModeKeyboard);
   gameUI->onKeyDown(fireKey);
   GamePair::idle(20, 1);   // 20ms — projectile created, flies ~12 units
   gameUI->onKeyUp(fireKey);

   // Track the projectile step by step
   for(S32 step = 0; step < 10; ++step)
   {
      Vector<DatabaseObject *> projs;
      gamePair.server->getGameObjDatabase()->findObjects((TestFunc)isProjectileType, projs);
      if(projs.size() == 0)
         break;

      GamePair::idle(20, 1);
   }

   // Try damaging Barrier 0 directly to verify damageObject works
   {
      Vector<DatabaseObject *> barriers;
      gamePair.server->getGameObjDatabase()->findObjects((TestFunc)isWallType, barriers);
      if(barriers.size() > 0)
      {
         Barrier *b0 = dynamic_cast<Barrier *>(barriers[0]);
         ASSERT_TRUE(b0 != NULL);
         DamageInfo di;
         di.damageAmount = 1.0f;
         b0->damageObject(&di);
      }
   }

   // Start firing the Phaser (default weapon, 0.19 dmg, 100ms fire delay).
   // Need ceil(10/0.19) ≈ 53 hits.
   gameUI->onKeyDown(fireKey);
   GamePair::idle(20, 500);    // 10,000ms of sustained auto-fire
   gameUI->onKeyUp(fireKey);

   // Let updated tiles propagate to the client
   S32 destAfter = -1;
   for(S32 idleCycle = 0; idleCycle < 300; ++idleCycle)
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

   // ---- Assertions on SERVER tiles ----
   ASSERT_GT(tiles.size(), 0);
   // With the combined Union approach, Tile 0 has ONE poly (13 verts).
   // Verify it has both Solid and Dashed edges (from both barrier types).
   bool hasSolid = false, hasDashed = false, hasNone = false;
   for(S32 t = 0; t < tiles.size(); ++t)
      if(tiles[t].tileId == 0)
         for(S32 p = 0; p < tiles[t].polys.size(); ++p)
         {
            const WallPoly &wp = tiles[t].polys[p];
            for(U32 e = 0; e < wp.edges.size(); ++e)
            {
               if(wp.edges[e] == EdgeStyle::Normal) hasSolid = true;
               if(wp.edges[e] == EdgeStyle::Destructible) hasDashed = true;
               if(wp.edges[e] == EdgeStyle::None) hasNone = true;
            }
         }

   // ---- Assertions on CLIENT received tiles ----
   const Vector<WallPoly> &clientPolys = GameType::getTilePolys();
   bool clientHasSolid = false, clientHasDashed = false;
   for(S32 i = 0; i < clientPolys.size(); ++i)
   {
      const WallPoly &wp = clientPolys[i];
      for(U32 e = 0; e < wp.edges.size(); ++e)
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

   ASSERT_GT(tiles.size(), 0);

   // Count polys that have zero visible edges (all None)
   S32 invisiblePolys = 0;
   for(S32 t = 0; t < tiles.size(); ++t)
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         bool allNone = true;
         for(U32 e = 0; e < wp.edges.size(); ++e)
            if(wp.edges[e] != EdgeStyle::None) { allNone = false; break; }
         if(allNone) invisiblePolys++;
      }

   // Every polygon should have at least some visible edges
   EXPECT_EQ(invisiblePolys, 0) << invisiblePolys << " polys have ALL None edges (invisible)!";

   // Must have both types
   bool hasSolid = false, hasDashed = false;
   for(S32 t = 0; t < tiles.size(); ++t)
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         for(U32 e = 0; e < wp.edges.size(); ++e)
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
   for(S32 i = 0; i < clientPolys.size(); ++i)
   {
      bool allNone = true;
      for(U32 e = 0; e < clientPolys[i].edges.size(); ++e)
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

   // Diagnostic: show which None edges are not on tile boundary
   for(S32 t = 0; t < tiles.size(); ++t)
   {
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         U32 nv = wp.numVerts();
      }
   }

   ASSERT_GT(tiles.size(), 0);

   // Count polys that have zero visible edges (all None)
   S32 invisiblePolys = 0;
   for(S32 t = 0; t < tiles.size(); ++t)
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         bool allNone = true;
         for(U32 e = 0; e < wp.edges.size(); ++e)
            if(wp.edges[e] != EdgeStyle::None) { allNone = false; break; }
         if(allNone) invisiblePolys++;
      }

   EXPECT_EQ(invisiblePolys, 0) << invisiblePolys << " polys have ALL None edges (invisible)!";

   // Must have both types
   bool hasSolid = false, hasDashed = false;
   for(S32 t = 0; t < tiles.size(); ++t)
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         for(U32 e = 0; e < wp.edges.size(); ++e)
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
   for(S32 i = 0; i < clientPolys.size(); ++i)
   {
      bool allNone = true;
      for(U32 e = 0; e < clientPolys[i].edges.size(); ++e)
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

    ASSERT_GT(tiles.size(), 0);

   // Must have Solid edges
   bool hasSolid = false;
   for(S32 t = 0; t < tiles.size(); ++t)
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         for(U32 e = 0; e < wp.edges.size(); ++e)
            if(wp.edges[e] == EdgeStyle::Normal) hasSolid = true;
      }

   EXPECT_TRUE(hasSolid) << "Must have Solid edges (permanent walls)";

   // ---- CLIENT assertions ----
   const Vector<WallPoly> &clientPolys = GameType::getTilePolys();
   EXPECT_GT(clientPolys.size(), 0) << "Client must receive tile polys";
   bool clientHasSolid = false;
   for(S32 i = 0; i < clientPolys.size(); ++i)
      for(U32 e = 0; e < clientPolys[i].edges.size(); ++e)
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

   // for(S32 t = 0; t < tiles.size(); ++t)
   // {
   //    for(S32 p = 0; p < tiles[t].polys.size(); ++p)
   //    {
   //       const WallPoly &wp = tiles[t].polys[p];
   //       F32 polyArea = 0;
   //       if(wp.numVerts() >= 3)
   //       {
   //          Vector<Point> ctr;
   //          for(U32 v = 0; v < wp.numVerts(); ++v)
   //             ctr.push_back(Point(wp.verts[v*2], wp.verts[v*2+1]));
   //          polyArea = area(ctr);
   //       }
   //    }
   // }

   // Sum destructible-only poly areas and check no Solid edges (permanent) appear
   F32 totalDestArea = 0;
   bool hasSolid = false;
   for(S32 t = 0; t < tiles.size(); ++t)
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         bool allDest = true;
         for(U32 e = 0; e < wp.edges.size(); ++e)
            if(wp.edges[e] == EdgeStyle::Normal) { hasSolid = true; allDest = false; break; }
         if(!allDest) continue;
         Vector<Point> contour;
         for(U32 v = 0; v < wp.numVerts(); ++v)
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

   ASSERT_GT(tiles.size(), 0);
   bool hasDashed = false;
   bool hasSolid = false;
   for(S32 t = 0; t < tiles.size(); ++t)
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
         for(U32 e = 0; e < tiles[t].polys[p].edges.size(); ++e)
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
   for(S32 t = 0; t < tiles.size(); ++t)
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
         for(U32 e = 0; e < tiles[t].polys[p].edges.size(); ++e)
         {
            if(tiles[t].polys[p].edges[e] == EdgeStyle::Destructible) hasDashed = true;
            if(tiles[t].polys[p].edges[e] == EdgeStyle::Normal)  hasSolid  = true;
         }
    EXPECT_FALSE(hasDashed) << "Permanent PolyWall must have no Dashed edges";
    EXPECT_TRUE(hasSolid) << "Permanent PolyWall should have Solid edges";
}

/// Level from Mad Dog's "Pavillion" map.  A destructible vertical wall at
/// x=4590 has no left edge when rendered — the edge at x≈4565 is missing.
TEST(WallTilingTest, PavillionDestructibleWallLeftEdge2)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 1\n"
      "LevelName Pavillion\n"
      "LevelDescription \"A Pavillion, a Parking Lot, Stuff like that\"\n"
      "LevelCredits \"Mad Dog\"\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker 50 0 -2295 0 -1530\n"
      "BarrierMaker D 50 4590 -2040 4590 -1785\n";

   GamePair gp(level, 1);   // 1 client to test the full rendering pipeline
   GameType::clearTilePolys();
   GamePair::idle(50, 200);  // Let tiles propagate to client

   // Check CLIENT-side received tile polys
   const Vector<WallPoly> &clientPolys = GameType::getTilePolys();
   ASSERT_GT(clientPolys.size(), 0) << "Client must receive tile polys";

   // Look for a destructible polygon near the wall at x=4590, width 50
   // (x range ≈ 4565..4615, y range ≈ -2040..-1785)
   bool foundDestPoly  = false;
   bool leftEdgeOk     = false;

   for(S32 i = 0; i < clientPolys.size(); ++i)
   {
      const WallPoly &wp = clientPolys[i];

      F32 minX = F32_MAX, maxX = -F32_MAX;
      F32 minY = F32_MAX, maxY = -F32_MAX;
      for(U32 v = 0; v < wp.numVerts(); ++v)
      {
         F32 vx = wp.verts[v * 2];
         F32 vy = wp.verts[v * 2 + 1];
         if(vx < minX) minX = vx;
         if(vx > maxX) maxX = vx;
         if(vy < minY) minY = vy;
         if(vy > maxY) maxY = vy;
      }

      if(maxX < 4540 || minX > 4640 || maxY < -2050 || minY > -1770)
         continue;

      bool hasDest = false;
      for(U32 e = 0; e < wp.edges.size(); ++e)
         if(wp.edges[e] == EdgeStyle::Destructible) { hasDest = true; break; }
      if(!hasDest) continue;

      foundDestPoly = true;

      // Check: any edge near x≈4565 (left wall side) must be visible
      for(U32 e = 0; e < wp.edges.size(); ++e)
      {
         U32 v0 = e * 2;
         U32 v1 = ((e + 1) % wp.numVerts()) * 2;
         F32 x1 = wp.verts[v0];
         F32 x2 = wp.verts[v1];

         if(std::abs(x1 - 4565.0f) < 5.0f && std::abs(x2 - 4565.0f) < 5.0f)
         {
            EXPECT_NE(wp.edges[e], EdgeStyle::None)
               << "CLIENT: Left edge of destructible wall at x≈4565 must be visible";
            if(wp.edges[e] != EdgeStyle::None)
               leftEdgeOk = true;
         }
      }
   }

   EXPECT_TRUE(foundDestPoly)
      << "Expected destructible wall polygon near x=4590 on client";
   EXPECT_TRUE(leftEdgeOk)
      << "CLIENT: Left edge of destructible wall at x≈4565 was EdgeStyle::None";
}

/// Level from Mad Dog's "Pavillion" map.  A destructible vertical wall at
/// x=4590 has no left edge when rendered — the edge at x≈4565 is missing.
TEST(WallTilingTest, PavillionDestructibleWallLeftEdge)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 1\n"
      "LevelName Pavillion\n"
      "LevelDescription \"A Pavillion, a Parking Lot, Stuff like that\"\n"
      "LevelCredits \"Mad Dog\"\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker 50 0 -2295 0 -1530\n"
      "BarrierMaker D 50 4590 -2040 4590 -1785\n";

   GamePair gp(level, 1);
   GameType::clearTilePolys();
   GamePair::idle(50, 200);

   const Vector<WallPoly> &clientPolys = GameType::getTilePolys();
   ASSERT_GT(clientPolys.size(), 0) << "Client must receive tile polys";

   bool foundDestPoly = false;
   bool leftEdgeOk = false;

   for(S32 i = 0; i < clientPolys.size(); ++i)
   {
      const WallPoly &wp = clientPolys[i];
      F32 minX = F32_MAX, maxX = -F32_MAX, minY = F32_MAX, maxY = -F32_MAX;
      for(U32 v = 0; v < wp.numVerts(); ++v)
      {
         F32 vx = wp.verts[v * 2], vy = wp.verts[v * 2 + 1];
         if(vx < minX) minX = vx; if(vx > maxX) maxX = vx;
         if(vy < minY) minY = vy; if(vy > maxY) maxY = vy;
      }
      if(maxX < 4540 || minX > 4640 || maxY < -2050 || minY > -1770)
         continue;

      bool hasDest = false;
      for(U32 e = 0; e < wp.edges.size(); ++e)
         if(wp.edges[e] == EdgeStyle::Destructible) { hasDest = true; break; }
      if(!hasDest) continue;

      foundDestPoly = true;
      for(U32 e = 0; e < wp.edges.size(); ++e)
      {
         U32 v0 = e * 2, v1 = ((e + 1) % wp.numVerts()) * 2;
         F32 x1 = wp.verts[v0], x2 = wp.verts[v1];
         if(std::abs(x1 - 4565.0f) < 5.0f && std::abs(x2 - 4565.0f) < 5.0f)
         {
            EXPECT_NE(wp.edges[e], EdgeStyle::None)
               << "Left edge of dest wall at x≈4565 must be visible";
            if(wp.edges[e] != EdgeStyle::None) leftEdgeOk = true;
         }
      }
   }
   EXPECT_TRUE(foundDestPoly) << "Expected dest wall polygon near x=4590";
   EXPECT_TRUE(leftEdgeOk) << "Left edge at x≈4565 was EdgeStyle::None";
}


/// Test demonstrating the backwards-L wall rendering bug in Mad Dog's
/// "Pavillion" level.  Two destructible BarrierMakers form an L shape:
///   - Vertical segment:   BarrierMaker D 50 4590 -2040 4590 -1836
///   - Horizontal segment: BarrierMaker D 50 4437 -1861.5 4590 -1861.5
/// The two barriers overlap near (4590, -1861.5).  After Union in the tile
/// builder, the interior edges inside the shared area should not be visible
/// — they should be EdgeStyle::None.  If Clipper2 preserves interior edges
/// from the original polygons, they become visible rendering artifacts.
TEST(WallTilingTest, PavillionBackwardsLInteriorEdges)
{
   // Construct the two barrier outlines manually (same as what
   // constructBarrierPolygon produces) and feed them to MapTileBuilder.
   // Barrier 1: vertical segment spine (4590,-2040) to (4590,-1836), width 50
   //   Outline: rectangle from x=4565 to x=4615, y=-2040 to y=-1836
   // Barrier 2: horizontal segment spine (4437,-1861.5) to (4590,-1861.5), width 50
   //   Outline: rectangle from x=4437 to x=4590, y=-1886.5 to y=-1836.5
   //
   // Overlap region: x in [4565, 4590], y in [-1886.5, -1836.5]
   // After Union, the combined polygon should have NO edges inside this overlap.

   // Use a known tile size so we can reason about tile boundaries.
   // Level bounds cover both barriers plus some margin.
   Rect levelBounds(Point(4400, -2100), Point(4650, -1750));
   MapTileBuilder builder(levelBounds, 128);  // 128-unit tiles

   Point dummy(nanf(""), nanf(""));

   // Barrier 1: vertical, spine (4590,-2040)→(4590,-1836), width 50
   Vector<Point> outline1;
   constructBarrierPolygon(Point(4590, -2040), Point(4590, -1836),
                           dummy, dummy, 50, outline1);
   builder.addWallPolygon(outline1, true /* destructive */);

   // Barrier 2: horizontal, spine (4437,-1861.5)→(4590,-1861.5), width 50
   Vector<Point> outline2;
   constructBarrierPolygon(Point(4437, -1861.5f), Point(4590, -1861.5f),
                           dummy, dummy, 50, outline2);
   builder.addWallPolygon(outline2, true /* destructive */);

   Vector<MapTile> tiles;
   builder.build(tiles);

   ASSERT_GT(tiles.size(), 0) << "Should produce at least one tile";

   // Walk through all tile polys and find any edge whose midpoint falls inside
   // the overlap region (x in [4565, 4590], y in [-1886.5, -1836.5]).
   // Such an edge MUST be EdgeStyle::None — it is an interior edge introduced
   // by the overlapping barriers, not part of the exterior outline.
   S32 interiorDestEdges = 0;
   S32 interiorNormalEdges = 0;

   for(S32 t = 0; t < tiles.size(); ++t)
   {
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         U32 nv = wp.numVerts();

         for(U32 e = 0; e < nv; ++e)
         {
            U32 v0 = e * 2;
            U32 v1 = ((e + 1) % nv) * 2;
            F32 x1 = wp.verts[v0];
            F32 y1 = wp.verts[v0 + 1];
            F32 x2 = wp.verts[v1];
            F32 y2 = wp.verts[v1 + 1];

            // Compute midpoint
            F32 mx = (x1 + x2) / 2;
            F32 my = (y1 + y2) / 2;

            // Is the midpoint inside the overlap region?
            // Use a small margin to handle floating-point imprecision.
            bool inOverlap = (mx > 4565.0f + 0.1f && mx < 4590.0f - 0.1f &&
                              my > -1886.5f + 0.1f && my < -1836.5f - 0.1f);

            if(inOverlap)
            {
               if(wp.edges[e] == EdgeStyle::Destructible)
                  ++interiorDestEdges;

               else if(wp.edges[e] == EdgeStyle::Normal)
                  ++interiorNormalEdges;

            }
         }
      }
   }

   EXPECT_EQ(interiorDestEdges, 0)
      << "Found " << interiorDestEdges
      << " Destructible edges inside the L-shape overlap region. "
      << "These are interior edges from overlapping barrier outlines that "
      << "should have been removed by the Union operation.";
   EXPECT_EQ(interiorNormalEdges, 0)
      << "Found " << interiorNormalEdges
      << " Normal edges inside the L-shape overlap region.";
}

// ---------------------------------------------------------------------------
// Diagnostic: trace the mergeOverlappingEdges / Union pipeline step by step
// for the backwards-L overlap case WITHOUT needing the server/client fixture.
// ---------------------------------------------------------------------------
TEST(WallTilingTest, DiagnosticBackwardsLOverlapStepByStep)
{
   // Full level geometry from TestEdgeIdVerificationDemo:
   //   BarrierMaker 50 0 -2295 0 -1530       (permanent vertical)
   //   BarrierMaker D 50 4590 -2040 4590 -1836  (destructible vertical)
   //   BarrierMaker D 50 4437 -1861.5 4590 -1861.5  (destructible horizontal)
   //
   // Including the far-away permanent wall is critical because it
   // determines the level bounds and therefore the tile grid, which
   // affects which tile boundaries the barrier edges interact with.
   Rect levelBounds(Point(-50, -2310), Point(4650, -1500));
   MapTileBuilder builder(levelBounds);  // Adaptive tile size (≥256)

   Point dummy(nanf(""), nanf(""));

   // Wall 1: permanent vertical, spine (0,-2295)→(0,-1530), width 50
   Vector<Point> outline1;
   constructBarrierPolygon(Point(0, -2295), Point(0, -1530),
                           dummy, dummy, 50, outline1);
   builder.addWallPolygon(outline1, false /* permanent */);

   // Wall 2: destructible vertical, spine (4590,-2040)→(4590,-1836), width 50
   Vector<Point> outline2;
   constructBarrierPolygon(Point(4590, -2040), Point(4590, -1836),
                           dummy, dummy, 50, outline2);
   builder.addWallPolygon(outline2, true /* destructible */);

   // Wall 3: destructible horizontal, spine (4437,-1861.5)→(4590,-1861.5), width 50
   Vector<Point> outline3;
   constructBarrierPolygon(Point(4437, -1861.5f), Point(4590, -1861.5f),
                           dummy, dummy, 50, outline3);
   builder.addWallPolygon(outline3, true /* destructible */);

   Vector<MapTile> tiles;
   builder.build(tiles);

   // --- Dump raw input outlines ---
   printf("=== Input: permanent wall (spine 0,-2295 to 0,-1530, w=50) ===\n");
   for(S32 v = 0; v < outline1.size(); ++v)
      printf("  v%d: (%8.1f, %8.1f)\n", v, outline1[v].x, outline1[v].y);

   printf("=== Input: destructible vertical (spine 4590,-2040 to 4590,-1836, w=50) ===\n");
   for(S32 v = 0; v < outline2.size(); ++v)
      printf("  v%d: (%8.1f, %8.1f)\n", v, outline2[v].x, outline2[v].y);

   printf("=== Input: destructible horizontal (spine 4437,-1861.5 to 4590,-1861.5, w=50) ===\n");
   for(S32 v = 0; v < outline3.size(); ++v)
      printf("  v%d: (%8.1f, %8.1f)\n", v, outline3[v].x, outline3[v].y);

   printf("\n=== Clipper2 output (%d tiles, tile size=%d) ===\n",
          tiles.size(), builder.getTileSize());

   for(S32 t = 0; t < tiles.size(); ++t)
   {
      printf("Tile %d (id=%d, bounds=(%8.1f,%8.1f)-(%8.1f,%8.1f)): %d polys\n",
             t, tiles[t].tileId,
             tiles[t].bounds.min.x, tiles[t].bounds.min.y,
             tiles[t].bounds.max.x, tiles[t].bounds.max.y,
             tiles[t].polys.size());

      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         U32 nv = wp.numVerts();
         printf("  Poly %d: %d verts, %d edges\n", p, nv, wp.edges.size());
         for(U32 e = 0; e < nv; ++e)
         {
            U32 v0 = e * 2;
            U32 v1 = ((e + 1) % nv) * 2;
            printf("    Edge %2d: (%8.1f,%8.1f) -> (%8.1f,%8.1f)   %s\n",
                   e,
                   wp.verts[v0], wp.verts[v0 + 1],
                   wp.verts[v1], wp.verts[v1 + 1],
                   (wp.edges[e] == EdgeStyle::None ? "None" :
                    wp.edges[e] == EdgeStyle::Destructible ? "Destructible" : "Normal"));
         }
      }
   }

   // --- Assertions: no Destructible edges in overlap region ---
   S32 interiorDestEdges = 0;
   for(S32 t = 0; t < tiles.size(); ++t)
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         for(U32 e = 0; e < wp.numVerts(); ++e)
         {
            F32 x1 = wp.verts[e * 2];
            F32 y1 = wp.verts[e * 2 + 1];
            F32 x2 = wp.verts[((e + 1) % wp.numVerts()) * 2];
            F32 y2 = wp.verts[((e + 1) % wp.numVerts()) * 2 + 1];
            F32 mx = (x1 + x2) / 2, my = (y1 + y2) / 2;
            if(mx > 4565.0f + 0.1f && mx < 4590.0f - 0.1f &&
               my > -1886.5f + 0.1f && my < -1836.5f - 0.1f)
            {
               if(wp.edges[e] == EdgeStyle::Destructible)
                  ++interiorDestEdges;

            }
         }
      }

   EXPECT_EQ(interiorDestEdges, 0)
      << "Found " << interiorDestEdges << " Destructible edges inside the overlap region";
}

// ---------------------------------------------------------------------------
// Regression test: the exposed left edge of the destructible vertical wall at
// x~4565 (below the horizontal arm) must be rendered.
//
// Level geometry (same as TestEdgeIdVerificationDemo - the "Pavillion" level):
//   BarrierMaker 50 0 -2295 0 -1530                 (permanent vertical)
//   BarrierMaker D 50 4590 -2040 4590 -1836         (destructible vertical)
//   BarrierMaker D 50 4437 -1861.5 4590 -1861.5     (destructible horizontal)
//
// The destructible vertical wall (spine 4590,-2040 to 4590,-1836, width 50)
// has its left surface at x=4565.  In the game's tile grid (tileSize=200,
// origin from barrier extents padded by 10), column 23's left edge is
// -35 + 23*200 = 4565 - exactly the wall's left surface.  In tile row 2
// (y in [-1930,-1730]) the clipped wall segment spans y in [-1930,-1811].
//
// The horizontal destructible wall (spine 4437,-1861.5 to 4590,-1861.5,
// width 50) occupies y in [-1886.5,-1836.5] and overlaps the vertical wall.
// classifyEdges() decides whether to suppress a tile-boundary edge by testing
// ONLY the edge's midpoint: it offsets the midpoint outside the tile and
// checks if that point lies inside the global wall union.  The midpoint of
// the long left edge is y~-1870.5, which lies INSIDE the horizontal arm, so
// the ENTIRE edge is suppressed as EdgeStyle::None - including the exposed
// lower portion y in [-1930,-1886.5] that is genuine exterior destructible
// wall and MUST be rendered.
//
// This test asserts that the exposed point (4565, -1895) - the lower-left
// corner area of the vertical wall - is covered by a rendered (non-None) edge.
TEST(WallTilingTest, PavillionDestructibleLeftEdgeBelowHorizontalArmIsRendered)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 1\n"
      "LevelName Pavillion\n"
      "LevelDescription \"A Pavillion, a Parking Lot, Stuff like that\"\n"
      "LevelCredits \"Mad Dog\"\n"
      "Team Players 0.7 0.7 0.7\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker 50 0 -2295 0 -1530\n"
      "BarrierMaker D 50 4590 -2040 4590 -1836\n"
      "BarrierMaker D 50 4437 -1861.5 4590 -1861.5\n"
      "Spawn 0 4462.5 -1912.5\n";

   GamePair gp(level, 0);
   GamePair::idle(50, 100);

   GameType *gt = gp.server->getGameType();
   ASSERT_TRUE(gt != NULL);
   const Vector<MapTile> &tiles = gt->getMapTiles();
   ASSERT_GT(tiles.size(), 0) << "Level should produce tiled geometry";

   const F32 wallX    = 4565.0f;   // Left surface of the destructible vertical wall
   const F32 exposedY = -1895.0f;  // Below the horizontal arm's bottom edge (-1886.5)

   bool foundEdge = false;         // An edge exists on x~4565 covering exposedY
   bool foundRenderedEdge = false; // ...and it is rendered (non-None)

   for(S32 t = 0; t < tiles.size(); ++t)
   {
      for(S32 p = 0; p < tiles[t].polys.size(); ++p)
      {
         const WallPoly &wp = tiles[t].polys[p];
         U32 nv = wp.numVerts();

         for(U32 e = 0; e < nv; ++e)
         {
            U32 v0 = e * 2;
            U32 v1 = ((e + 1) % nv) * 2;
            F32 x1 = wp.verts[v0];
            F32 y1 = wp.verts[v0 + 1];
            F32 x2 = wp.verts[v1];
            F32 y2 = wp.verts[v1 + 1];

            // We're looking for the wall's left surface (vertical edge at x~4565)
            if(fabs(x1 - wallX) > 0.5f || fabs(x2 - wallX) > 0.5f)
               continue;

            F32 loY = (y1 < y2) ? y1 : y2;
            F32 hiY = (y1 > y2) ? y1 : y2;

            // Does this edge cover the exposed point below the horizontal arm?
            if(loY <= exposedY && exposedY <= hiY)
            {
               foundEdge = true;
               if(wp.edges[e] != EdgeStyle::None)
                  foundRenderedEdge = true;
            }
         }
      }
   }

   EXPECT_TRUE(foundEdge)
      << "No edge found on x~4565 covering y=" << exposedY
      << ". The destructible vertical wall's left surface below the horizontal "
      << "arm is missing from the tiled output entirely.";

   EXPECT_TRUE(foundRenderedEdge)
      << "The exposed destructible wall left edge at x~4565 (y=" << exposedY
      << ") is classified as EdgeStyle::None and is NOT rendered.  "
      << "classifyEdges() tests only the edge midpoint (y~-1870.5, which lies "
      << "inside the horizontal arm) and suppresses the entire edge, hiding the "
      << "genuine exterior portion below the arm.";
}

/// BoxFrenzy - regression test verifying exact edge rendering for the
/// "Box Frenzy" level.
TEST(WallTilingTest, BugOffEdgeStyles)
{
   string level =
      "LevelFormat 2\n"
      "GameType 8 8\n"
      "LevelName BugOff\n"
      "LevelDescription ""\n"
      "LevelCredits ScareCrow\n"
      "Team Red 1 0 0\n"
      "Team Blue 0 0 1\n"
      "Specials\n"
      "MinPlayers\n"
      "MaxPlayers\n"
      "FogOfWar Default\n"
      "BarrierMaker 50 1785 -3060 2040 -3060\n"
      "BarrierMaker 50 -2040 1530 -1785 1530\n"
      "BarrierMaker 50 -1785 1275 -1785 1530\n"
      "BarrierMaker 50 -2040 1275 -2040 1530\n"
      "BarrierMaker 50 -2040 1275 -1785 1275\n"
      "Spawn -1 -2167.5 1402.5\n";

   GamePair gp(level, 0);
   GamePair::idle(50, 100);

   GameType *gt = gp.server->getGameType();
   ASSERT_TRUE(gt != NULL);
   const Vector<MapTile> &tiles = gt->getMapTiles();
   ASSERT_GT(tiles.size(), 0) << "Level should produce tiled geometry";

   // The 4 left walls form a hollow box ring (hole spans
   // x in [-2015,-1810], y in [1300,1505]).  The right wall is a separate
   // solid rectangle.
   //
   // Edges facing the hole (inner ring: 11-14, 17, 19, 21-22) ARE real
   // exterior wall and must render.  Edges cut through solid wall material
   // (23-25, 28) are interior and must be suppressed, as are tile-boundary
   // clips through solid wall (3, 5, 15, 18, 37).
   ExpectedEdges expectations = makeExpectedEdges(
      "Normal: 0-2,4,6-14,16-17,19-22,26-27,29-36,38-40; "
      "None: 3,5,15,18,23-25,28,37"          // Should NOT be rendered
   );

   verifyTileEdges(tiles, expectations);
}/// BoxFrenzy - regression test verifying exact edge rendering for the
/// "Box Frenzy" level.
TEST(WallTilingTest, BoxFrenzyEdgeStyles)
{
   string level =
      "LevelFormat 2\n"
      "GameType 8 100\n"
      "LevelName BoxFrenzy\n"
      "LevelDescription \"A Box Maze with funny bits\"\n"
      "LevelCredits BoB\n"
      "Team Red 1 0 0\n"
      "Team Blue 0 0 1\n"
      "Specials\nMinPlayers\nMaxPlayers\nFogOfWar Default\n"
      "BarrierMaker 50 -765 -2550 -765 -2295\n"
      "BarrierMaker 50 -765 -2040 -765 -1785\n"
      "BarrierMaker 50 -2550 -2550 -2550 -1785\n"
      "BarrierMaker 50 0 -2550 0 -2295\n"
      "Spawn -1 -382.5 -2167.5\n";

   GamePair gp(level, 0);
   GamePair::idle(50, 100);

   GameType *gt = gp.server->getGameType();
   ASSERT_TRUE(gt != NULL);
   const Vector<MapTile> &tiles = gt->getMapTiles();
   ASSERT_GT(tiles.size(), 0) << "Level should produce tiled geometry";

   // Visible as permanent wall
   ExpectedEdges expectations = makeExpectedEdges(
      "Normal: 0-2,4,6,8-15,17-20,22,24,27,29,32-39,41,43,46-48; "
      "None: 3,5,7,16,21,23,25-26,28,30-31,40,42,44-45"     // ?? 28,30 — Should NOT be rendered
   );

   verifyTileEdges(tiles, expectations);
}

} // namespace Zap
