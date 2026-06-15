//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "MapTile.h"

#include <clipper2/clipper.h>          // Clipper2
#include <clipper2/clipper.rectclip.h> // RectClip64
#include <cmath>

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
   for (S32 i = 0; i < pts.size(); ++i)
      path.push_back(pointToC2(pts[i]));
   return path;
}

/// Convert a flat list of int64_t (x,y pairs) back to a Vector<Point>.
static Vector<Point> int64ToPoints(const Vector<int64_t> &flat)
{
   Vector<Point> pts;
   pts.reserve(flat.size() / 2);
   for (S32 i = 0; i < flat.size(); i += 2)
      pts.push_back(c2ToPoint(Point64(flat[i], flat[i + 1])));
   return pts;
}

/// Compute next power of two (for unsigned values)
static U32 nextPow2(U32 v)
{
   if (v == 0) return 1;
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
   static const U32 MIN_TILE_SZ = 5;

   // sqrt(area / MAX_TILES) gives approximate tile size, rounded up to power-of-2
   F32 tileSizeF = std::sqrt(levelArea / static_cast<F32>(MAX_TILES));
   U32 tileSize = nextPow2(static_cast<U32>(tileSizeF));
   if (tileSize < MIN_TILE_SZ)
      tileSize = MIN_TILE_SZ;

   mTileSize = tileSize;

   // Guard against degenerate level bounds
   if (levelW <= 0 || levelH <= 0 || mTileSize == 0)
   {
      mGridWidth  = 1;
      mGridHeight = 1;
      return;
   }

   mGridWidth  = static_cast<S32>(std::ceil(levelW / static_cast<F32>(mTileSize)));
   mGridHeight = static_cast<S32>(std::ceil(levelH / static_cast<F32>(mTileSize)));

   // Clamp to prevent overflow
   if (mGridWidth  > 256) mGridWidth  = 256;
   if (mGridHeight > 256) mGridHeight = 256;
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
// Determine which edges are original versus clip-introduced.
//
// An edge is considered "clip-introduced" (outline = false) if both of its
// endpoints lie on the tile boundary and the entire edge runs along that
// boundary.  All other edges are original edges that survived clipping
// (outline = true).
//
// Coordinates are in Clipper2 int64_t space so comparisons are exact.
// ---------------------------------------------------------------------------
void MapTileBuilder::computeOutlineFlags(
   const Vector<int64_t> &clippedVerts,
   const Vector<int64_t> &origVerts,
   int64_t tileLeft, int64_t tileTop,
   int64_t tileRight, int64_t tileBottom,
   Vector<bool> &outline)
{
   const U32 n = clippedVerts.size() / 2;   // number of vertices
   outline.clear();
   outline.reserve(n);

   if (n == 0)
      return;

   for (U32 i = 0; i < n; ++i)
   {
      const U32 i0 = i * 2;
      const U32 i1 = ((i + 1) % n) * 2;

      const int64_t x1 = clippedVerts[i0];
      const int64_t y1 = clippedVerts[i0 + 1];
      const int64_t x2 = clippedVerts[i1];
      const int64_t y2 = clippedVerts[i1 + 1];

      // Check if both endpoints lie on the same tile boundary line
      // Use a small tolerance (1 Clipper2 unit = 1/1000 game unit) to handle
      // any internal Clipper2 precision variances
      const int64_t tol = 1;
      const bool onLeft   = (abs(x1 - tileLeft)  <= tol && abs(x2 - tileLeft)  <= tol);
      const bool onRight  = (abs(x1 - tileRight) <= tol && abs(x2 - tileRight) <= tol);
      const bool onTop    = (abs(y1 - tileTop)   <= tol && abs(y2 - tileTop)   <= tol);
      const bool onBottom = (abs(y1 - tileBottom)<= tol && abs(y2 - tileBottom)<= tol);

      // If the edge runs along any tile boundary, it's clip-introduced
      outline.push_back(!(onLeft || onRight || onTop || onBottom));
   }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

MapTileBuilder::MapTileBuilder(const Rect &levelBounds, U32 tileSizeOverride)
   : mLevelBounds(levelBounds)
{
   if (tileSizeOverride > 0)
   {
      mTileSize   = tileSizeOverride;
      const F32 levelW = mLevelBounds.getWidth();
      const F32 levelH = mLevelBounds.getHeight();
      mGridWidth  = static_cast<S32>(std::ceil(levelW  / static_cast<F32>(mTileSize)));
      mGridHeight = static_cast<S32>(std::ceil(levelH / static_cast<F32>(mTileSize)));
      if (mGridWidth  < 1) mGridWidth  = 1;
      if (mGridHeight < 1) mGridHeight = 1;
   }
   else
   {
      computeTileGrid();
   }
}


void MapTileBuilder::addWallPolygon(const Vector<Point> &polygon)
{
   mWallPolygons.push_back(polygon);
}


void MapTileBuilder::build(Vector<MapTile> &outTiles)
{
   outTiles.clear();

   if(mWallPolygons.size() == 0)
      return;

   // Pre-convert all wall polygons to Clipper2 Path64 and store their
   // vertex data for outline-flag computation later.
   struct C2Wall {
      Path64          path;
      Vector<int64_t> flatVerts; // flat x,y pairs
   };
   Vector<C2Wall> c2Walls;
   c2Walls.reserve(mWallPolygons.size());

   for(S32 w = 0; w < mWallPolygons.size(); w++)
   {
      C2Wall cw;
      cw.path = pointsToPath64(mWallPolygons[w]);
      cw.flatVerts.reserve(cw.path.size() * 2);
      for(const auto &pt : cw.path)
      {
         cw.flatVerts.push_back(pt.x);
         cw.flatVerts.push_back(pt.y);
      }
      c2Walls.push_back(std::move(cw));
   }

   // Use a simple approach: iterate over walls × tiles, clip each intersecting pair.
   // For small to moderate map sizes this is fast enough.  Could be optimised later
   // with a spatial index if profiling shows it's needed.
   for(S32 w = 0; w < c2Walls.size(); w++)
   {
      const Path64 &wallPath = c2Walls[w].path;
      const Vector<int64_t> &origFlat = c2Walls[w].flatVerts;

      // Quick AABB check to narrow down candidate tiles.
      // Compute the wall's bounding box in game coordinates.
      Rect wallBounds(mWallPolygons[w]);
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

            for(const auto &resultPath : clipped)
            {
               if(resultPath.size() < 3)
                  continue; // degenerate

               // Build flat verts for the clipped result
               Vector<int64_t> clippedFlat;
               clippedFlat.reserve(resultPath.size() * 2);
               for(const auto &pt : resultPath)
               {
                  clippedFlat.push_back(pt.x);
                  clippedFlat.push_back(pt.y);
               }

               // Compute outline flags (use the tile rect in int64_t space)
               Vector<bool> outline;
               computeOutlineFlags(clippedFlat, origFlat,
                  tileRectC2.left, tileRectC2.top,
                  tileRectC2.right, tileRectC2.bottom,
                  outline);

               // Convert to WallPoly
               WallPoly wp;
               wp.verts.reserve(clippedFlat.size());
               for(S32 k = 0; k < clippedFlat.size(); k++)
                  wp.verts.push_back(static_cast<F32>(clippedFlat[k] * CLIPPER2_INV_SCALE));

               wp.outline = outline;

               // Find or create the MapTile
               MapTile *tile = nullptr;
               for(S32 t = 0; t < outTiles.size(); ++t)
               {
                  if(outTiles[t].tileId == tid)
                  {
                     tile = &outTiles[t];
                     break;
                  }
               }
               if(!tile)
               {
                  MapTile newTile;
                  newTile.tileId = tid;
                  newTile.bounds = tileRect(tid);
                  outTiles.push_back(newTile);
                  tile = &outTiles[outTiles.size() - 1];
               }
               tile->polys.push_back(std::move(wp));
            }
         }
      }
   }

   // Cleanup: for each tile, Union all its polys with Clipper2 to
   // resolve winding-based holes from merged donut-shaped polygons.
   for(S32 t = 0; t < outTiles.size(); t++)
   {
      MapTile &mt = outTiles[t];
      Paths64 tilePolys;
      for(S32 p = 0; p < mt.polys.size(); p++)
      {
         Path64 path;
         for(U32 v = 0; v < mt.polys[p].numVerts(); v++)
            path.push_back(Point64(
               static_cast<int64_t>(mt.polys[p].verts[v * 2] * CLIPPER2_SCALE),
               static_cast<int64_t>(mt.polys[p].verts[v * 2 + 1] * CLIPPER2_SCALE)));
         if(path.size() >= 3)
            tilePolys.push_back(std::move(path));
      }

      Paths64 unioned = Union(tilePolys, FillRule::NonZero);

      const Rect tileRectF = mt.bounds;
      const Point64 tileMinC2 = pointToC2(Point(tileRectF.min.x, tileRectF.min.y));
      const Point64 tileMaxC2 = pointToC2(Point(tileRectF.max.x, tileRectF.max.y));

      mt.polys.clear();
      for(const auto &up : unioned)
      {
         if(up.size() < 3) continue;

         U32 nv = up.size();
         WallPoly wp;
         wp.verts.reserve(nv * 2);
         wp.outline.reserve(nv);
         for(U32 v = 0; v < nv; v++)
         {
            wp.verts.push_back(static_cast<F32>(up[v].x * CLIPPER2_INV_SCALE));
            wp.verts.push_back(static_cast<F32>(up[v].y * CLIPPER2_INV_SCALE));

            const Point64 &p2 = up[(v + 1) % nv];
            const int64_t x1 = up[v].x, y1 = up[v].y;
            const int64_t x2 = p2.x, y2 = p2.y;
            const int64_t tL = tileMinC2.x, tT = tileMinC2.y;
            const int64_t tR = tileMaxC2.x, tB = tileMaxC2.y;
            const int64_t tol = 1;
            wp.outline.push_back(!(
               (abs(x1 - tL) <= tol && abs(x2 - tL) <= tol) ||
               (abs(x1 - tR) <= tol && abs(x2 - tR) <= tol) ||
               (abs(y1 - tT) <= tol && abs(y2 - tT) <= tol) ||
               (abs(y1 - tB) <= tol && abs(y2 - tB) <= tol)));
         }
         mt.polys.push_back(std::move(wp));
      }
   }
}

} // namespace Zap
