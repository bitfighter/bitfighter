//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _MAPTILE_H_
#define _MAPTILE_H_

#include "tnlTypes.h"
#include "tnlVector.h"
#include "Point.h"
#include "Rect.h"

namespace Zap
{

/// Per-edge rendering style for wall tile polygons.
enum class EdgeStyle : U8
{
   None   = 0,		  // Do not render this edge (tile boundary, not rendered)
   Normal  = 1,		  // Render as blue line
   Destructible = 2,  // Render as green line
};

/// A single wall polygon fragment within a tile.
/// verts: flattened x,y pairs [x0, y0, x1, y1, ..., xN, yN]
/// edges[i] determines how edge from vert[i] to vert[(i+1) % N] is rendered:
///   None   — edge was introduced by clipping (tile boundary), do not render
///   Normal  — edge came from a non-destructible barrier (or both types)
///   Destructible — edge came from a destructible barrier only
struct WallPoly
{
   Vector<F32> verts;
   Vector<EdgeStyle> edges;

   U32 numVerts() const { return verts.size() / 2; }

   // Precomputed render cache — filled once when tile data arrives.
   // Triangulated fill triangles
   Vector<Point> cachedFill;
   // Normal (non-destructible) wall outline edges as point pairs.
   Vector<Point> cachedNormalEdges;
   // Destructible wall outline edges as point pairs.
   Vector<Point> cachedDestructibleEdges;
   /// true if any edge is EdgeStyle::Destructible
   bool cachedDestructible = false;
};


/// A tile containing zero or more wall polygon fragments.
struct MapTile
{
   U16 tileId = 0;
   S32 gridX  = 0;        // Absolute grid column (tiles to right of world-origin)
   S32 gridY  = 0;        // Absolute grid row    (tiles above world-origin)
   Rect bounds;           // World-space bounding rect
   Vector<WallPoly> polys;
};


/// Builds a tiled representation of wall geometry using Clipper2.
/// Call addWallPolygon() for each wall outline, then build() to
/// generate the tile grid and clipped fragments.
class MapTileBuilder
{
public:
   /// Construct a builder for the given level bounds.
   /// tileSizeOverride = 0 means compute adaptively.
   MapTileBuilder(const Rect &levelBounds, U32 tileSizeOverride = 0);

   /// Add a wall polygon (outline) to be tiled.
   /// polygon vertices should form a closed CCW polygon (e.g. from Barrier::getCollisionPoly()).
   /// Add a wall polygon (outline) to be tiled.
   /// @param destructible  Set to true if this wall is destructible
   void addWallPolygon(const Vector<Point> &polygon, bool destructible = false);

   /// Build all tiles.  Clears outTiles before populating.
   void build(Vector<MapTile> &outTiles);

   /// Access the computed tile grid parameters.
   U32  getTileSize()   const { return mTileSize; }
   S32  getGridWidth()  const { return mGridWidth; }
   S32  getGridHeight() const { return mGridHeight; }
   U16  getTileCount()  const { return static_cast<U16>(mGridWidth * mGridHeight); }

private:
   Rect   mLevelBounds;
   U32    mTileSize;
   S32    mGridWidth;
   S32    mGridHeight;

   Vector<Vector<Point> > mWallPolygons;
   Vector<bool> mWallDestructible;            // Parallel to mWallPolygons: true if the wall is destructible

   void computeTileGrid();

   /// Convert tile-grid coordinates to a linear tile ID.
   U16 tileId(S32 gx, S32 gy) const;

   /// Return the world-space Rect for the given tile.
   Rect tileRect(U16 tileId) const;
};

} // namespace Zap

#endif // _MAPTILE_H_
