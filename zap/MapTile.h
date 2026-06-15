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

/// A single wall polygon fragment within a tile.
/// verts: flattened x,y pairs [x0, y0, x1, y1, ..., xN, yN]
/// outline[i] == true  -> render edge from vert[i] to vert[(i+1) % N]
/// outline[i] == false -> edge was introduced by clipping (tile boundary), do not render
struct WallPoly
{
   Vector<F32> verts;
   Vector<bool> outline;

   U32 numVerts() const { return verts.size() / 2; }
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
   void addWallPolygon(const Vector<Point> &polygon);

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

   void computeTileGrid();

   /// Convert tile-grid coordinates to a linear tile ID.
   U16 tileId(S32 gx, S32 gy) const;

   /// Return the world-space Rect for the given tile.
   Rect tileRect(U16 tileId) const;

   /// Determine which edges in clipped are original vs clip-introduced.
   /// clippedVerts: int64_t coords of the clipped polygon (flattened x,y pairs).
   /// origVerts:    int64_t coords of the original polygon (flattened x,y pairs).
   /// tileLeft/Top/Right/Bottom: the tile rect in int64_t coordinate space.
   /// outline:      output bool vector, one per edge.
   static void computeOutlineFlags(
      const Vector<int64_t> &clippedVerts,
      const Vector<int64_t> &origVerts,
      int64_t tileLeft, int64_t tileTop,
      int64_t tileRight, int64_t tileBottom,
      Vector<bool> &outline);
};

} // namespace Zap

#endif // _MAPTILE_H_
