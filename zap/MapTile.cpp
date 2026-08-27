//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "MapTile.h"


#include <clipper2/clipper.h>          // Clipper2
#include <clipper2/clipper.rectclip.h> // RectClip64
#include <algorithm>
#include <cmath>
#include <unordered_set>
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

/// Hole-aware point-in-union test using the NonZero winding rule.
///
/// The global union (Paths64) produced by Clipper2 with FillRule::NonZero
/// represents outer boundaries and holes as separate paths with opposite
/// winding.  A point is inside the filled region iff the NET winding across
/// all paths is non-zero.  Testing each path individually with
/// PointInPolygon() treats hole paths as solid filled regions, which wrongly
/// suppresses genuine exterior wall edges that border a hole (for example the
/// inner boundary of a hollow box ring).
static bool pointInUnionPaths(const Point64 &pt, const Paths64 &paths)
{
   if(paths.empty())
      return false;

   int winding = 0;
   for(const auto &path : paths)
   {
      if(path.size() < 3)
         continue;

      const PointInPolygonResult res = PointInPolygon(pt, path);
      if(res == PointInPolygonResult::IsOutside)
         continue;

      // Outer boundaries and holes carry opposite orientations.  Count the
      // contribution using the path's signed area so that a point inside a
      // hole (inside both the outer path and the hole path) nets to zero and
      // is correctly reported as outside the filled region.
      winding += (Area(path) > 0) ? 1 : -1;
   }

   return winding != 0;
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


/// Encode a (tileId, side) pair as a 16-bit key.
/// side: 0=left, 1=right, 2=bottom, 3=top
static U16 protectedKey(U16 tileId, S32 side)
{
   return static_cast<U16>((tileId << 2) | (side & 3));
}


/// Pre-compute which tile boundary sides are "protected" — meaning an original
/// wall polygon had an edge lying on that side.  Edges on protected sides will
/// not be suppressed even if Clipper2 operations modify their vertex positions.
static void buildProtectedBoundaries(const Vector<Vector<Point>> &wallPolygons,
                                      const Rect &levelBounds, U32 tileSize,
                                      S32 gridWidth, S32 gridHeight,
                                      std::unordered_set<U16> &outProtected)
{
   for(S32 w = 0; w < wallPolygons.size(); w++)
   {
      const Vector<Point> &poly = wallPolygons[w];
      S32 n = poly.size();
      for(S32 v = 0; v < n; v++)
      {
         const Point &p1 = poly[v];
         const Point &p2 = poly[(v + 1) % n];

         // Only axis-aligned edges can lie on tile boundaries
         bool vertical   = std::abs(p1.x - p2.x) < 0.01f;
         bool horizontal = std::abs(p1.y - p2.y) < 0.01f;
         if(!vertical && !horizontal)
            continue;

         if(vertical)
         {
            F32 edgeX = p1.x;
            S32 gx = static_cast<S32>(std::floor(
               (edgeX - levelBounds.min.x) / static_cast<F32>(tileSize)));

            F32 leftEdge  = levelBounds.min.x + gx * static_cast<F32>(tileSize);
            F32 rightEdge = leftEdge + static_cast<F32>(tileSize);
            S32 side = -1;
            if(std::abs(edgeX - leftEdge) < 0.01f)
               side = 0;  // left
            else if(std::abs(edgeX - rightEdge) < 0.01f)
               side = 1;  // right
            else
               continue;

            F32 y1 = std::min(p1.y, p2.y);
            F32 y2 = std::max(p1.y, p2.y);
            S32 gyStart = static_cast<S32>(std::floor(
               (y1 - levelBounds.min.y) / static_cast<F32>(tileSize)));
            S32 gyEnd   = static_cast<S32>(std::floor(
               (y2 - levelBounds.min.y) / static_cast<F32>(tileSize)));
            if(gyStart < 0) gyStart = 0;
            if(gyEnd >= gridHeight) gyEnd = gridHeight - 1;

            // Protect the tile this edge's grid coordinate maps to
            for(S32 gy = gyStart; gy <= gyEnd; gy++)
            {
               U16 tid = static_cast<U16>(gy * gridWidth + gx);
               outProtected.insert(protectedKey(tid, side));
            }

            // Also protect the adjacent tile's opposite side.
            // An edge on a tile grid line touches BOTH tiles.
            S32 adjSide, adjGx;
            if(side == 0) // left side of tile gx → right side of tile gx-1
            {
               adjSide = 1;
               adjGx   = gx - 1;
            }
            else // side == 1, right side of tile gx → left side of tile gx+1
            {
               adjSide = 0;
               adjGx   = gx + 1;
            }
            if(adjGx >= 0 && adjGx < gridWidth)
            {
               for(S32 gy = gyStart; gy <= gyEnd; gy++)
               {
                  U16 tid = static_cast<U16>(gy * gridWidth + adjGx);
                  outProtected.insert(protectedKey(tid, adjSide));
               }
            }
         }
         else  // horizontal
         {
            F32 edgeY = p1.y;
            S32 gy = static_cast<S32>(std::floor(
               (edgeY - levelBounds.min.y) / static_cast<F32>(tileSize)));

            F32 bottomEdge = levelBounds.min.y + gy * static_cast<F32>(tileSize);
            F32 topEdge    = bottomEdge + static_cast<F32>(tileSize);
            S32 side = -1;
            if(std::abs(edgeY - bottomEdge) < 0.01f)
               side = 2;  // bottom
            else if(std::abs(edgeY - topEdge) < 0.01f)
               side = 3;  // top
            else
               continue;

            F32 x1 = std::min(p1.x, p2.x);
            F32 x2 = std::max(p1.x, p2.x);
            S32 gxStart = static_cast<S32>(std::floor(
               (x1 - levelBounds.min.x) / static_cast<F32>(tileSize)));
            S32 gxEnd   = static_cast<S32>(std::floor(
               (x2 - levelBounds.min.x) / static_cast<F32>(tileSize)));
            if(gxStart < 0) gxStart = 0;
            if(gxEnd >= gridWidth) gxEnd = gridWidth - 1;

            for(S32 gx = gxStart; gx <= gxEnd; gx++)
            {
               U16 tid = static_cast<U16>(gy * gridWidth + gx);
               outProtected.insert(protectedKey(tid, side));
            }

            // Also protect the adjacent tile's opposite side
            S32 adjSide, adjGy;
            if(side == 2) // bottom of tile gy → top of tile gy-1
            {
               adjSide = 3;
               adjGy   = gy - 1;
            }
            else // side == 3, top of tile gy → bottom of tile gy+1
            {
               adjSide = 2;
               adjGy   = gy + 1;
            }
            if(adjGy >= 0 && adjGy < gridHeight)
            {
               for(S32 gx = gxStart; gx <= gxEnd; gx++)
               {
                  U16 tid = static_cast<U16>(adjGy * gridWidth + gx);
                  outProtected.insert(protectedKey(tid, adjSide));
               }
            }
         }
      }
   }
}


/// Describes a split operation for one tile-boundary edge: the original edge
/// index, the sorted split parameters (in [0,1]), and the style for each
/// resulting sub-segment.
struct EdgeSpanSplit
{
   U32 edgeIdx;
   Vector<F32> ts;
   Vector<EdgeStyle> segStyles;
};


/// Compute where the global union's boundary crosses the given axis-aligned
/// tile-boundary edge, and classify each resulting sub-segment.  Returns false
/// if the edge should not be split (no crossings), in which case the caller
/// falls back to the single-midpoint test.
static bool computeEdgeSpans(const WallPoly &wp, U32 edgeIdx,
                             const Point64 &p1, const Point64 &p2,   // edge endpoints in C2 space
                             S32 side,
                             const Paths64 &globalUnion,
                             Vector<F32> &splitTs,
                             Vector<EdgeStyle> &segStyles)
{
   splitTs.clear();
   segStyles.clear();

   const bool vertical = (side == 0 || side == 1);

   // Guard against zero-length edges
   const int64_t lenC2 = vertical ? (p2.y - p1.y) : (p2.x - p1.x);
   if(lenC2 == 0)
      return false;

   // Collect crossing parameters (0..1) where union boundary edges cross our line
   std::vector<double> crossings;
   crossings.reserve(16);

   const int64_t fixedC = vertical ? p1.x : p1.y;   // x (vertical) or y (horizontal)

   for(S32 pi = 0; pi < globalUnion.size(); pi++)
   {
      const Path64 &path = globalUnion[pi];
      if(path.size() < 2)
         continue;

      for(S32 ei = 0; ei < path.size(); ei++)
      {
         const Point64 &a = path[ei];
         const Point64 &b = path[(ei + 1) % path.size()];

         double s;
         if(vertical)
         {
            // Does segment a-b cross the vertical line fixedC?
            if(a.x == b.x)
               continue;                     // parallel or collinear — no clean crossing
            const double t = (double)(fixedC - a.x) / (double)(b.x - a.x);
            if(t < 0.0 || t > 1.0)
               continue;
            const double cy = (double)a.y + t * (double)(b.y - a.y);
            const int64_t loY = (p1.y < p2.y) ? p1.y : p2.y;
            const int64_t hiY = (p1.y > p2.y) ? p1.y : p2.y;
            if(cy < (double)loY - 1.0 || cy > (double)hiY + 1.0)
               continue;                     // crossing outside our edge's span
            s = (cy - (double)p1.y) / (double)(p2.y - p1.y);
         }
         else
         {
            if(a.y == b.y)
               continue;
            const double t = (double)(fixedC - a.y) / (double)(b.y - a.y);
            if(t < 0.0 || t > 1.0)
               continue;
            const double cx = (double)a.x + t * (double)(b.x - a.x);
            const int64_t loX = (p1.x < p2.x) ? p1.x : p2.x;
            const int64_t hiX = (p1.x > p2.x) ? p1.x : p2.x;
            if(cx < (double)loX - 1.0 || cx > (double)hiX + 1.0)
               continue;
            s = (cx - (double)p1.x) / (double)(p2.x - p1.x);
         }

         // Ignore crossings at the exact endpoints — those are handled naturally
         // by the adjacent sub-segments' midpoint tests.
         if(s > 0.0001 && s < 0.9999)
            crossings.push_back(s);
      }
   }

   if(crossings.empty())
      return false;

   // Sort & dedupe
   std::sort(crossings.begin(), crossings.end());
   std::vector<double> uniq;
   uniq.reserve(crossings.size());
   for(S32 i = 0; i < (S32)crossings.size(); i++)
      if(uniq.empty() || std::fabs(crossings[i] - uniq.back()) > 1e-4)
         uniq.push_back(crossings[i]);

   if(uniq.empty())
      return false;

   // Classify each sub-segment by testing its offset midpoint
   const EdgeStyle origStyle = wp.edges[edgeIdx];
   const F32 x0 = wp.verts[edgeIdx * 2];
   const F32 y0 = wp.verts[edgeIdx * 2 + 1];
   const U32 nv = wp.numVerts();
   const F32 x1 = wp.verts[((edgeIdx + 1) % nv) * 2];
   const F32 y1 = wp.verts[((edgeIdx + 1) % nv) * 2 + 1];

   const F32 eps = 1.0f;    // small enough to detect thin wall slivers adjacent
                            // to a tile boundary, yet large enough to clear
                            // Clipper2 precision shifts (1 unit = 1000 C2 units)

   double prevS = 0.0;
   for(S32 i = 0; i <= (S32)uniq.size(); i++)
   {
      const double sNext = (i < (S32)uniq.size()) ? uniq[i] : 1.0;
      const double midS = (prevS + sNext) * 0.5;

      F32 mx = x0 + (x1 - x0) * (F32)midS;
      F32 my = y0 + (y1 - y0) * (F32)midS;

      switch(side)
      {
         case 0: mx -= eps; break;  // left   -> outside is left
         case 1: mx += eps; break;  // right  -> outside is right
         case 2: my -= eps; break;  // bottom -> outside is down
         case 3: my += eps; break;  // top    -> outside is up
      }

      const Point64 testPt = pointToC2(Point(mx, my));
      segStyles.push_back(pointInUnionPaths(testPt, globalUnion) ? EdgeStyle::None : origStyle);
      prevS = sNext;
   }

   splitTs.reserve(uniq.size());
   for(S32 i = 0; i < (S32)uniq.size(); i++)
      splitTs.push_back((F32)uniq[i]);

   return true;
}


/// Rebuild the polygon's vertex/edge arrays, inserting intermediate vertices
/// for each recorded split so that the per-sub-segment styles take effect.
static void applyEdgeSplits(WallPoly &wp, const std::vector<EdgeSpanSplit> &splits)
{
   if(splits.empty())
      return;

   const U32 n = wp.numVerts();

   Vector<F32> newVerts;
   Vector<EdgeStyle> newEdges;
   newVerts.reserve(wp.verts.size() + splits.size() * 6);
   newEdges.reserve(wp.edges.size() + splits.size() * 6);

   auto findSplit = [&splits](U32 edgeIdx) -> const EdgeSpanSplit * {
      for(S32 s = 0; s < (S32)splits.size(); s++)
         if(splits[s].edgeIdx == edgeIdx)
            return &splits[s];
      return NULL;
   };

   // Emit v0
   newVerts.push_back(wp.verts[0]);
   newVerts.push_back(wp.verts[1]);

   for(U32 i = 0; i < n; i++)
   {
      const U32 next = (i + 1) % n;
      const F32 eX = wp.verts[next * 2];
      const F32 eY = wp.verts[next * 2 + 1];

      const EdgeSpanSplit *split = findSplit(i);

      if(!split || split->ts.size() == 0)
      {
         newEdges.push_back(wp.edges[i]);
      }
      else
      {
         const F32 sX = wp.verts[i * 2];
         const F32 sY = wp.verts[i * 2 + 1];

         for(S32 k = 0; k < split->ts.size(); k++)
         {
            const F32 t = split->ts[k];
            newVerts.push_back(sX + (eX - sX) * t);
            newVerts.push_back(sY + (eY - sY) * t);
            newEdges.push_back(split->segStyles[k]);
         }
         newEdges.push_back(split->segStyles.last());
      }

      // Emit the next vertex (unless it's v0, which is already emitted)
      if(next != 0)
      {
         newVerts.push_back(eX);
         newVerts.push_back(eY);
      }
   }

   wp.verts = move(newVerts);
   wp.edges = move(newEdges);
}


/// Mark edges that lie on tile boundaries as EdgeStyle::None so the renderer
/// skips them (they were introduced by clipping, not present in the original wall).
/// Edges on tile boundaries are tested against the global wall union:
/// we offset a point just OUTSIDE the tile and check whether it lies inside the
/// global union polygon.  If it does, the wall continues on the other side — this
/// is an interior clip artifact and that edge portion is suppressed.  If the
/// outside point is outside the union, the wall truly ends here and the edge is
/// kept visible.
///
/// A single edge can be PARTIALLY interior: another barrier may overlap only a
/// portion of a long tile-boundary edge.  In that case we split the edge at the
/// union boundary crossings so only the interior sub-segments are suppressed.
///
/// @param wallPoly        The clipped polygon being classified (mutated in place)
/// @param tileBounds      World-space bounds of this tile
/// @param tileId          Linear tile identifier
/// @param protectedBoundaries Set of (tileId,side) keys that are protected
/// @param globalUnion     Global (non-tiled) union of all wall polygons, as paths
static void classifyEdges(WallPoly &wallPoly, const Rect &tileBounds, U16 tileId,
                          const std::unordered_set<U16> &protectedBoundaries,
                          const Paths64 &globalUnion)
{
   Point64 tmin = pointToC2(Point(tileBounds.min.x, tileBounds.min.y));
   Point64 tmax = pointToC2(Point(tileBounds.max.x, tileBounds.max.y));
   const int64_t tol = 1;
   
   const U32 numVerts = wallPoly.numVerts();
   std::vector<EdgeSpanSplit> splits;   // collected split ops, applied after the loop

   for(U32 i = 0; i < numVerts; i++)
   {
      const F32 x1 = wallPoly.verts[i * 2];
      const F32 y1 = wallPoly.verts[i * 2 + 1];
      const F32 x2 = wallPoly.verts[((i + 1) % numVerts) * 2];
      const F32 y2 = wallPoly.verts[((i + 1) % numVerts) * 2 + 1];

      Point64 p1 = pointToC2(Point(x1, y1));
      Point64 p2 = pointToC2(Point(x2, y2));

      S32 side = -1;
      if(abs(p1.x - tmin.x) <= tol && abs(p2.x - tmin.x) <= tol)
         side = 0;  // left
      else if(abs(p1.x - tmax.x) <= tol && abs(p2.x - tmax.x) <= tol)
         side = 1;  // right
      else if(abs(p1.y - tmin.y) <= tol && abs(p2.y - tmin.y) <= tol)
         side = 2;  // bottom
      else if(abs(p1.y - tmax.y) <= tol && abs(p2.y - tmax.y) <= tol)
         side = 3;  // top

      if(side < 0)
         continue;  // Not on a tile boundary

      // If the global union's boundary crosses this edge, the edge may have both
      // exposed (exterior) and interior portions.  Split at the crossings and
      // classify each sub-segment independently.
      Vector<F32> splitTs;
      Vector<EdgeStyle> segStyles;
      if(computeEdgeSpans(wallPoly, i, p1, p2, side, globalUnion, splitTs, segStyles))
      {
         bool allNone    = true;
         bool allVisible = true;
         for(S32 s = 0; s < segStyles.size(); s++)
         {
            if(segStyles[s] != EdgeStyle::None)
               allNone = false;
            if(segStyles[s] == EdgeStyle::None)
               allVisible = false;
         }

         if(allVisible)
            continue;                      // No interior spans — keep entire edge visible
         if(allNone)
         {
            wallPoly.edges[i] = EdgeStyle::None;   // Fully interior — same behavior as before
            continue;
         }

         // Mixed: record split for later application
         EdgeSpanSplit split;
         split.edgeIdx = i;
         split.ts = splitTs;
         split.segStyles = segStyles;
         splits.push_back(split);
         continue;
      }

      // Fallback (no union boundary crossings): test the single midpoint.
      // Offset the edge midpoint OUTSIDE the tile and check if that point is
      // inside the global union polygon.  If inside, the wall continues on the
      // other side — this is an interior clip artifact.  If outside, the wall
      // stops here — this is a genuine boundary.
      F32 midX = (x1 + x2) * 0.5f;
      F32 midY = (y1 + y2) * 0.5f;

      const F32 eps = 1.0f;    // small enough to detect thin wall slivers adjacent
                               // to a tile boundary, yet large enough to clear
                               // Clipper2 precision shifts (1 unit = 1000 C2 units)
      switch(side)
      {
         case 0: midX -= eps; break;  // left   -> outside is left
         case 1: midX += eps; break;  // right  -> outside is right
         case 2: midY -= eps; break;  // bottom -> outside is down
         case 3: midY += eps; break;  // top    -> outside is up
      }

      Point64 testPt = pointToC2(Point(midX, midY));

      if(pointInUnionPaths(testPt, globalUnion))
         wallPoly.edges[i] = EdgeStyle::None;
      // else: keep visible — genuine exterior wall boundary
   }

   // Apply recorded splits — must happen after the loop since splitting changes
   // the vertex/edge indexing.
   if(!splits.empty())
      applyEdgeSplits(wallPoly, splits);
}


/// Strip collinear intermediate vertices from a Clipper2 Path64.
/// Three consecutive vertices a→b→c are collinear if the cross product
/// (b-a)×(c-b) is zero, meaning b can be removed without changing the shape.
static void stripCollinearPoints(Path64 &path)
{
   if(path.size() < 3)
      return;

   Path64 result;
   result.reserve(path.size());

   S32 n = static_cast<S32>(path.size());
   for(S32 i = 0; i < n; i++)
   {
      const Point64 &prev = path[(i - 1 + n) % n];
      const Point64 &curr = path[i];
      const Point64 &next = path[(i + 1) % n];

      // Cross product of incoming and outgoing edges
      int64_t dx1 = curr.x - prev.x;
      int64_t dy1 = curr.y - prev.y;
      int64_t dx2 = next.x - curr.x;
      int64_t dy2 = next.y - curr.y;

      // Non-collinear if cross product is non-zero → keep this vertex
      if(dx1 * dy2 != dy1 * dx2)
         result.push_back(curr);
   }

   path = std::move(result);
}

static void stripCollinearPoints(Paths64 &paths)
{
   for(auto &path : paths)
      stripCollinearPoints(path);
}


/// Merge overlapping collinear edge pairs on the same line.
/// Scans all edges and merges those that share the same axis and have
/// overlapping ranges, replacing both with a single combined edge.
/// The inner/overlapping portion of collinear edges is set to EdgeStyle::None
/// so the renderer skips it.
static void mergeOverlappingEdges(WallPoly &wp)
{
   const F32 EPS = 0.01f;
   U32 nv = wp.numVerts();
   if(nv < 4)
      return;

   // Build list of edge indices and their axis-aligned bounding information
   struct EdgeInfo
   {
      U32 idx;
      bool vertical;
      F32 fixedCoord;  // x if vertical, y if horizontal
      F32 lo, hi;       // sorted range along the other axis
   };

   Vector<EdgeInfo> infos;
   for(U32 e = 0; e < nv; e++)
   {
      if(wp.edges[e] == EdgeStyle::None)
         continue;

      F32 x1 = wp.verts[e * 2];
      F32 y1 = wp.verts[e * 2 + 1];
      F32 x2 = wp.verts[((e + 1) % nv) * 2];
      F32 y2 = wp.verts[((e + 1) % nv) * 2 + 1];

      bool vert = std::abs(x1 - x2) < EPS;
      bool horiz = std::abs(y1 - y2) < EPS;
      if(!vert && !horiz)
         continue;

      EdgeInfo info;
      info.idx = e;
      info.vertical = vert;
      if(vert)
      {
         info.fixedCoord = x1;
         info.lo = std::min(y1, y2);
         info.hi = std::max(y1, y2);
      }
      else
      {
         info.fixedCoord = y1;
         info.lo = std::min(x1, x2);
         info.hi = std::max(x1, x2);
      }
      infos.push_back(info);
   }

   // Find overlapping pairs and merge
   S32 n = infos.size();
   for(S32 i = 0; i < n; i++)
   {
      if(wp.edges[infos[i].idx] == EdgeStyle::None)
         continue;

      for(S32 j = i + 1; j < n; j++)
      {
         if(wp.edges[infos[j].idx] == EdgeStyle::None)
            continue;

         const EdgeInfo &a = infos[i];
         const EdgeInfo &b = infos[j];

         // Must share same axis and same fixed coordinate
         if(a.vertical != b.vertical)
            continue;
         if(std::abs(a.fixedCoord - b.fixedCoord) > EPS)
            continue;

         // Check overlap (with small tolerance)
         F32 overlapMin = std::max(a.lo, b.lo);
         F32 overlapMax = std::min(a.hi, b.hi);
         if(overlapMin >= overlapMax - EPS)
            continue;  // No meaningful overlap

         // Merge: keep the outer endpoints, suppress the inner segment
         // The combined edge spans from min to max of both
         F32 newLo = std::min(a.lo, b.lo);
         F32 newHi = std::max(a.hi, b.hi);

         // Rewrite edge A's vertices to span the full merged range
         U32 idxA = a.idx;
         U32 nextA = (idxA + 1) % nv;
         if(a.vertical)
         {
            wp.verts[idxA * 2 + 1] = newLo;
            wp.verts[nextA * 2 + 1] = newHi;
         }
         else
         {
            wp.verts[idxA * 2] = newLo;
            wp.verts[nextA * 2] = newHi;
         }

         // Mark the overlapping edge B as None (not rendered)
         wp.edges[b.idx] = EdgeStyle::None;

         // Also suppress the inner segment of the merged edge?
         // The merged edge A now spans the full range, so the overlap
         // region is interior and should be hidden.  Edge B (the other
         // overlapping edge) is already None.  The original non-overlapping
         // parts of A outside the overlap are exterior and remain visible.
      }
   }
}


/// After classifyEdges, edges on a tile boundary may still be visible
/// (not None) because they were "protected" by an original barrier edge
/// touching that boundary side.  However, if the edge's midpoint lies
/// strictly inside the polygon, it's an interior edge — the barrier
/// edge was actually at the polygon's outer boundary, and the Clipper2
/// Union has pushed it inward.  Suppress these.
static void suppressInteriorBoundaryEdges(WallPoly &wp)
{
   U32 nv = wp.numVerts();
   if(nv < 3)
      return;

   // Compute centroid/inside test: use ray-casting from a point known to
   // be far outside to the midpoint.
   // Simpler: for each vertex-aligned edge, compute midpoint, offset it
   // very slightly into the polygon interior, and check if that offset
   // point is also inside (via winding).  If an edge's midpoint is not
   // on the boundary of the convex hull projected onto the edge's line,
   // it is interior.

   // Approach: collect the polygon's axis-aligned bounding edges.
   // If two edges share the same line and one is fully contained within
   // the other's y-range (for vertical) / x-range (for horizontal), 
   // the inner one is interior.
   const F32 EPS = 0.1f;

   // Simple version: walk all edges and mark as None any axis-aligned
   // edge that is completely overlapped by another edge on the same line.
   struct EdgeRange
   {
      U32 idx;
      bool vertical;
      F32 fixedCoord;
      F32 lo, hi;
   };

   Vector<EdgeRange> ranges;
   for(U32 e = 0; e < nv; e++)
   {
      F32 x1 = wp.verts[e * 2];
      F32 y1 = wp.verts[e * 2 + 1];
      F32 x2 = wp.verts[((e + 1) % nv) * 2];
      F32 y2 = wp.verts[((e + 1) % nv) * 2 + 1];

      bool vert = std::abs(x1 - x2) < EPS;
      bool horiz = std::abs(y1 - y2) < EPS;
      if(!vert && !horiz)
         continue;

      EdgeRange er;
      er.idx = e;
      er.vertical = vert;
      er.fixedCoord = vert ? x1 : y1;
      er.lo = std::min(vert ? y1 : x1, vert ? y2 : x2);
      er.hi = std::max(vert ? y1 : x1, vert ? y2 : x2);
      ranges.push_back(er);
   }

   // Mark interior edges: if edge B's range is strictly inside edge A's range
   // on the same line, edge B is interior.
   for(S32 i = 0; i < ranges.size(); i++)
   {
      for(S32 j = i + 1; j < ranges.size(); j++)
      {
         EdgeRange &a = ranges[i];
         EdgeRange &b = ranges[j];
         if(a.vertical != b.vertical) continue;
         if(std::abs(a.fixedCoord - b.fixedCoord) > EPS) continue;

         // Check containment
         if(a.lo <= b.lo + EPS && b.hi <= a.hi + EPS)
         {
            // b is contained in a → b is interior
            wp.edges[b.idx] = EdgeStyle::None;
         }
         else if(b.lo <= a.lo + EPS && a.hi <= b.hi + EPS)
         {
            // a is contained in b → a is interior
            wp.edges[a.idx] = EdgeStyle::None;
         }
      }
   }
}


/// Convert clipped Paths64 into WallPoly objects, classify tile-boundary edges,
/// and append the results to the given MapTile.
static void appendClippedPaths(MapTile &mt, const Rect &tileBounds, const Paths64 &paths, EdgeStyle style,
                               const std::unordered_set<U16> &protectedBoundaries,
                               const Paths64 &globalUnion)
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
      // Merge overlapping collinear edges that share the same line.
      // When Clipper2 Unions overlapping barrier outlines, the junction
      // region may contain redundant parallel edges at slightly different
      // endpoint positions.  Merging them cleans up the output.
      mergeOverlappingEdges(wallPoly);

      classifyEdges(wallPoly, tileBounds, mt.tileId, protectedBoundaries, globalUnion);

      // Final cleanup: edges on protected tile boundaries whose midpoint
      // falls inside the polygon are interior and must be suppressed.
      // This handles the backwards-L overlap where a protected boundary
      // edge passes through the interior overlap region.
      suppressInteriorBoundaryEdges(wallPoly);

      mt.polys.push_back(move(wallPoly));
   }
}


void MapTileBuilder::build(Vector<MapTile> &outTiles)
{
   outTiles.clear();

   if(mWallPolygons.size() == 0)
      return;

   // -----------------------------------------------------------------------
   // Build a set of protected tile boundary sides — tile boundary sides
   // that have original wall edges on them.  After Clipper2 operations
   // (Union, Difference) in the second pass, vertex coordinates can shift,
   // making exact edge-key matching unreliable.  Using per-side protection
   // instead of per-edge keys makes the classification coordinate-independent.
   // -----------------------------------------------------------------------
   std::unordered_set<U16> protectedBoundaries;
   buildProtectedBoundaries(mWallPolygons, mLevelBounds, mTileSize, mGridWidth, mGridHeight, protectedBoundaries);

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
   std::vector<S32> tileLookup(mGridWidth * mGridHeight, -1);

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

   // ------------------------------------------------------------
   // Build global unions from original (pre-clip) wall paths for
   // interior-edge detection.  classifyEdges tests whether a point
   // just outside a tile boundary falls inside the global union.
   // Using the original wall outlines (not tile-clipped fragments)
   // ensures the union accurately represents the wall's true boundary.
   // ------------------------------------------------------------
   auto unionAll = [](const Paths64 &subject) -> Paths64
   {
      if(subject.empty()) return {};
      Clipper64 clipper;
      clipper.AddSubject(subject);
      Paths64 result;
      clipper.Execute(ClipType::Union, FillRule::NonZero, result);
      return result;
   };

   // Collect original (pre-clip) wall paths, separated by type
   Paths64 origPermPaths, origDestPaths;
   for(S32 w = 0; w < mWallPolygons.size(); w++)
   {
      Path64 path = pointsToPath64(mWallPolygons[w]);
      if(mWallDestructible[w])
         origDestPaths.push_back(path);
      else
         origPermPaths.push_back(path);
   }
   Paths64 globalPermUnion = unionAll(origPermPaths);
   Paths64 globalDestUnion = unionAll(origDestPaths);

   // ------------------------------------------------------------
   // Second pass: for each tile, compute separate geometry for permanent
   // and destructible walls.  Destructible areas that overlap permanent walls
   // are removed via Difference, so they render with the permanent (blue) fill.
   // This avoids the merged-Union Z-callback complexity entirely.
   // ------------------------------------------------------------

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
            clipper.AddSubject(tilePath.dest);
            clipper.Execute(ClipType::Union, FillRule::NonZero, destUnion);
         }
         if(!permResult.empty())
         {
            Clipper64 clipper;
            clipper.AddSubject(destUnion);
            clipper.AddClip(permResult);
            clipper.Execute(ClipType::Difference, FillRule::NonZero, destOnly);
         }
         else
            destOnly = std::move(destUnion);
      }

      tile.polys.clear();

      appendClippedPaths(tile, tileBounds, permResult, EdgeStyle::Normal,      protectedBoundaries, globalPermUnion);
      appendClippedPaths(tile, tileBounds, destOnly,   EdgeStyle::Destructible, protectedBoundaries, globalDestUnion);
   }
}

} // namespace Zap