//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "MapTile.h"


#include <clipper2/clipper.h>          // Clipper2
#include <clipper2/clipper.rectclip.h> // RectClip64
#include <cmath>
#include <vector>

namespace Zap
{

using namespace Clipper2Lib;

// ---------------------------------------------------------------------------
// Scale factor matching the existing Clipper1 usage in GeomUtils.cpp.
// All game-coordinate F32 values are multiplied by this before being passed
// to Clipper2, and divided by it when converting back.
// ---------------------------------------------------------------------------
static const F32 CLIPPER2_SCALE     = 1000.0f;
static const F32 CLIPPER2_INV_SCALE = 1.0f / CLIPPER2_SCALE;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Point64 pointToC2(const Point &p)
{
   return Point64(static_cast<int64_t>(p.x * CLIPPER2_SCALE),
                  static_cast<int64_t>(p.y * CLIPPER2_SCALE));
}

static Point c2ToPoint(const Point64 &p)
{
   return Point(static_cast<F32>(p.x * CLIPPER2_INV_SCALE),
                static_cast<F32>(p.y * CLIPPER2_INV_SCALE));
}

/// Convert a Vector<Point> (game format) to a Clipper2 Path64.
static Path64 pointsToPath64(const Vector<Point> &pts)
{
   Path64 path;
   path.reserve(pts.size());
   for(S32 i = 0; i < pts.size(); ++i)
      path.push_back(pointToC2(pts[i]));
   return path;
}

/// Compute next power of two (for unsigned values)
static U32 nextPow2(U32 v)
{
   if(v == 0) return 1;
   v--;
   v |= v >> 1;
   v |= v >> 2;
   v |= v >> 4;
   v |= v >> 8;
   v |= v >> 16;
   return v + 1;
}

// ---------------------------------------------------------------------------
// Compute the tile grid dimensions from level bounds.
// ---------------------------------------------------------------------------
void MapTileBuilder::computeTileGrid()
{
   const F32 levelW = mLevelBounds.getWidth();
   const F32 levelH = mLevelBounds.getHeight();
   const F32 levelArea = levelW * levelH;

   // Adaptive tile size: target ~ MAX_TILES tiles
   static const U32 MAX_TILES   = 2048;
   static const U32 MIN_TILE_SZ = 256;

   // sqrt(area / MAX_TILES) gives approximate tile size, rounded up to power-of-2
   F32 tileSizeF = std::sqrt(levelArea / static_cast<F32>(MAX_TILES));
   U32 tileSize = nextPow2(static_cast<U32>(tileSizeF));
   if(tileSize < MIN_TILE_SZ)
      tileSize = MIN_TILE_SZ;

   mTileSize = tileSize;

   // Guard against degenerate level bounds
   if(levelW <= 0 || levelH <= 0 || mTileSize == 0)
   {
      mGridWidth  = 1;
      mGridHeight = 1;
      return;
   }

   mGridWidth  = static_cast<S32>(std::ceil(levelW / static_cast<F32>(mTileSize)));
   mGridHeight = static_cast<S32>(std::ceil(levelH / static_cast<F32>(mTileSize)));

   // Clamp to prevent overflow
   if(mGridWidth  > 256) 
      mGridWidth  = 256;

   if(mGridHeight > 256) 
      mGridHeight = 256;
}

// ---------------------------------------------------------------------------
// Convert tile-grid coordinates to a 16-bit linear tile ID.
// ---------------------------------------------------------------------------
U16 MapTileBuilder::tileId(S32 gx, S32 gy) const
{
   return static_cast<U16>(gy * mGridWidth + gx);
}

// ---------------------------------------------------------------------------
// Return the world-space Rect for the given tile.
// ---------------------------------------------------------------------------
Rect MapTileBuilder::tileRect(U16 id) const
{
   const S32 gx = id % mGridWidth;
   const S32 gy = id / mGridWidth;
   const F32 left   = mLevelBounds.min.x + gx * static_cast<F32>(mTileSize);
   const F32 top    = mLevelBounds.min.y + gy * static_cast<F32>(mTileSize);
   return Rect(left, top, left + mTileSize, top + mTileSize);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

MapTileBuilder::MapTileBuilder(const Rect &levelBounds, U32 tileSizeOverride)
   : mLevelBounds(levelBounds)
{
   if(tileSizeOverride > 0)
   {
      mTileSize   = tileSizeOverride;
      const F32 levelW = mLevelBounds.getWidth();
      const F32 levelH = mLevelBounds.getHeight();
      mGridWidth  = static_cast<S32>(std::ceil(levelW  / static_cast<F32>(mTileSize)));
      mGridHeight = static_cast<S32>(std::ceil(levelH / static_cast<F32>(mTileSize)));
      if(mGridWidth  < 1) 
         mGridWidth  = 1;
      if(mGridHeight < 1) 
         mGridHeight = 1;
   }
   else
      computeTileGrid();
}


void MapTileBuilder::addWallPolygon(const Vector<Point> &polygon, bool destructible)
{
   mWallPolygons.push_back(polygon);
   mWallDestructible.push_back(destructible);
}


/// Mark edges that lie on tile boundaries as EdgeStyle::None so the renderer
/// skips them (they were introduced by clipping, not present in the original wall).
static void classifyEdges(WallPoly &wallPoly, const Rect &tileBounds)
{
   Point64 tmin = pointToC2(Point(tileBounds.min.x, tileBounds.min.y));
   Point64 tmax = pointToC2(Point(tileBounds.max.x, tileBounds.max.y));
   const int64_t tol = 1;
   
   U32 numVerts = wallPoly.numVerts();

   for(U32 i = 0; i < numVerts; i++)
   {
      const F32 x1 = wallPoly.verts[i * 2];
      const F32 y1 = wallPoly.verts[i * 2 + 1];
      const F32 x2 = wallPoly.verts[((i + 1) % numVerts) * 2];
      const F32 y2 = wallPoly.verts[((i + 1) % numVerts) * 2 + 1];

      Point64 p1 = pointToC2(Point(x1, y1));
      Point64 p2 = pointToC2(Point(x2, y2));

      if((abs(p1.x - tmin.x) <= tol && abs(p2.x - tmin.x) <= tol) ||
         (abs(p1.x - tmax.x) <= tol && abs(p2.x - tmax.x) <= tol) ||
         (abs(p1.y - tmin.y) <= tol && abs(p2.y - tmin.y) <= tol) ||
         (abs(p1.y - tmax.y) <= tol && abs(p2.y - tmax.y) <= tol)
      )
         wallPoly.edges[i] = EdgeStyle::None;
   }
}


/// Convert clipped Paths64 into WallPoly objects, classify tile-boundary edges,
/// and append the results to the given MapTile.
static void appendClippedPaths(MapTile &mt, const Rect &tileBounds, const Paths64 &paths, EdgeStyle style)
{
   for(S32 i = 0; i < paths.size(); i++)
   {
      if(paths[i].size() < 3) 
         continue;

      WallPoly wallPoly;

      for(const auto &pt : paths[i])
      {
         wallPoly.verts.push_back(static_cast<F32>(pt.x * CLIPPER2_INV_SCALE));
         wallPoly.verts.push_back(static_cast<F32>(pt.y * CLIPPER2_INV_SCALE));
         wallPoly.edges.push_back(style);
      }
      classifyEdges(wallPoly, tileBounds);
      mt.polys.push_back(move(wallPoly));
   }
}


void MapTileBuilder::build(Vector<MapTile> &outTiles)
{
   outTiles.clear();

   if(mWallPolygons.size() == 0)
      return;

   // Pre-convert all wall polygons to Clipper2 Path64.
   struct ClipperWall 
   {
      Path64 path;
   };

   Vector<ClipperWall> clipperWalls;
   clipperWalls.reserve(mWallPolygons.size());

   for(S32 w = 0; w < mWallPolygons.size(); w++)
   {
      ClipperWall cw;
      cw.path = pointsToPath64(mWallPolygons[w]);
      clipperWalls.push_back(move(cw));
   }

   // Use a simple approach: iterate over walls × tiles, clip each intersecting pair.
   // For small to moderate map sizes this is fast enough.  Could be optimised later
   // with a spatial index if profiling shows it's needed.
   //
   // We store clipped paths per tile in a temporary structure; the second pass
   // Unions all paths per tile and computes per-edge rendering styles.
   struct TilePaths 
   {
      Paths64 nonDest;
      Paths64 dest;
   };

   Vector<TilePaths> tilePaths;
   tilePaths.resize(outTiles.size());

   // Direct tileId → index lookup table (tileId = gy * mGridWidth + gx)
   vector<S32> tileLookup(mGridWidth * mGridHeight, -1);

   for(S32 i = 0; i < clipperWalls.size(); i++)
   {
      const Path64 &wallPath = clipperWalls[i].path;

      // Quick AABB check to narrow down candidate tiles.
      // Compute the wall's bounding box in game coordinates.
      Rect wallBounds(mWallPolygons[i]);
      const S32 tileMinGX = static_cast<S32>(std::floor(
         (wallBounds.min.x - mLevelBounds.min.x) / static_cast<F32>(mTileSize)));
      const S32 tileMaxGX = static_cast<S32>(std::floor(
         (wallBounds.max.x - mLevelBounds.min.x) / static_cast<F32>(mTileSize)));
      const S32 tileMinGY = static_cast<S32>(std::floor(
         (wallBounds.min.y - mLevelBounds.min.y) / static_cast<F32>(mTileSize)));
      const S32 tileMaxGY = static_cast<S32>(std::floor(
         (wallBounds.max.y - mLevelBounds.min.y) / static_cast<F32>(mTileSize)));

      for(S32 gy = tileMinGY; gy <= tileMaxGY; gy++)
      {
         for(S32 gx = tileMinGX; gx <= tileMaxGX; gx++)
         {
            if(gx < 0 || gx >= mGridWidth || gy < 0 || gy >= mGridHeight)
               continue;

            const U16 tid = tileId(gx, gy);
            const Rect tileRectF = tileRect(tid);

            // Convert tile rect to Clipper2 int64_t
            const Point64 tileMinC2 = pointToC2(Point(tileRectF.min.x, tileRectF.min.y));
            const Point64 tileMaxC2 = pointToC2(Point(tileRectF.max.x, tileRectF.max.y));
            const Rect64 tileRectC2(tileMinC2.x, tileMinC2.y, tileMaxC2.x, tileMaxC2.y);

            // Perform the clip
            RectClip64 clipper(tileRectC2);
            Paths64 clipped = clipper.Execute(Paths64{wallPath});

            // Find index in outTiles matching this tileId (O(1) lookup table)
            S32 tileIdx = tileLookup[tid];
            if(tileIdx < 0)
            {
               MapTile newTile;
               newTile.tileId = tid;
               newTile.bounds = tileRect(tid);
               newTile.gridX = static_cast<S32>(std::floor(
                  newTile.bounds.min.x / static_cast<F32>(mTileSize)));
               newTile.gridY = static_cast<S32>(std::floor(
                  newTile.bounds.min.y / static_cast<F32>(mTileSize)));
               outTiles.push_back(newTile);
               tileIdx = outTiles.size() - 1;
               tileLookup[tid] = tileIdx;
               tilePaths.resize(outTiles.size());
            }

            // Store clipped paths by type
            for(const auto &resultPath : clipped)
            {
               if(resultPath.size() < 3)
                  continue;

               if(mWallDestructible[i])
                  tilePaths[tileIdx].dest.push_back(resultPath);
               else
                  tilePaths[tileIdx].nonDest.push_back(resultPath);
            }
         }
      }
   }

   // Second pass: for each tile, compute separate geometry for permanent
   // and destructible walls.  Destructible areas that overlap permanent walls
   // are removed via Difference, so they render with the permanent (blue) fill.
   // This avoids the merged-Union Z-callback complexity entirely.

   for(S32 i = 0; i < outTiles.size(); i++)
   {
      MapTile &tile = outTiles[i];
      TilePaths &tilePath = tilePaths[i];

      if(tilePath.nonDest.empty() && tilePath.dest.empty())
         continue;

      const Rect &tileBounds = tile.bounds;

      // Union permanent paths → all edges Solid
      Paths64 permResult;
      if(!tilePath.nonDest.empty())
      {
         Clipper64 clipper;
         clipper.PreserveCollinear(true);
         clipper.AddSubject(tilePath.nonDest);
         clipper.Execute(ClipType::Union, FillRule::NonZero, permResult);
      }

      // Union destructible paths, then subtract permanent areas
      Paths64 destOnly;
      if(!tilePath.dest.empty())
      {
         Paths64 destUnion;
         {
            Clipper64 clipper;
            clipper.PreserveCollinear(true);
            clipper.AddSubject(tilePath.dest);
            clipper.Execute(ClipType::Union, FillRule::NonZero, destUnion);
         }
         if(!permResult.empty())
         {
            Clipper64 clipper;
            clipper.PreserveCollinear(true);
            clipper.AddSubject(destUnion);
            clipper.AddClip(permResult);
            clipper.Execute(ClipType::Difference, FillRule::NonZero, destOnly);
         }
         else
            destOnly = std::move(destUnion);
      }

      tile.polys.clear();

      appendClippedPaths(tile, tileBounds, permResult, EdgeStyle::Normal);
      appendClippedPaths(tile, tileBounds, destOnly,   EdgeStyle::Destructible);
   }
}

} // namespace Zap