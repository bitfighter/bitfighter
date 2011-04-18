//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _BARRIER_H_
#define _BARRIER_H_

#include "gameObject.h"
#include "Point.h"
#include "tnlNetObject.h"

namespace Zap
{

/// The Barrier class represents rectangular barriers that player controlled
/// ships cannot pass through... i.e. walls  Barrier objects, once created, never
/// change state, simplifying the pack/unpack update methods.  Barriers are
/// constructed as an expanded line segment.
class Barrier : public GameObject
{
public:
   Vector<Point> mPoints; ///< The points of the barrier --> if only two, first will be start, second end of an old-school segment

   bool mSolid;
   // By precomputing and storing, we should ease the rendering cost
   Vector<Point> mRenderFillGeometry; ///< Actual geometry used for rendering fill.
   Vector<Point> mRenderOutlineGeometry; ///< Actual geometry used for rendering outline.
   Vector<Point> mBotZoneBufferGeometry; ///< Geometry used for rendering the botzones.

   F32 mWidth;

   static const S32 MIN_BARRIER_WIDTH = 1;         // Clipper doesn't much like 0 width walls
   static const S32 MAX_BARRIER_WIDTH = 2500;      // Geowar has walls at least 350 units wide, so going lower will break at least one level

   static const S32 BarrierWidth = 50; ///< The default width of the barrier in game units

   static Vector<Point> mRenderLineSegments; ///< The clipped line segments representing this barrier.
   Vector<Point> mBotZoneBufferLineSegments; ///< The line segments representing a buffered barrier.

   /// Barrier constructor
   Barrier(const Vector<Point> &points = Vector<Point>(), F32 width = BarrierWidth, bool solid = false);

   /// Adds the server object to the net interface's scope always list
   void onAddedToGame(Game *theGame);

   /// Renders barrier fill
   void render(S32 layer);                // Renders barrier fill barrier-by-barrier
   static void renderEdges(S32 layer);    // Renders all edges in one pass

   /// Returns a sorting key for the object.  Barriers should be drawn first so as to appear behind other objects.
   S32 getRenderSortValue() { return 0; }

   /// returns the collision polygon of this barrier, which is the boundary extruded from the start,end line segment.
   bool getCollisionPoly(Vector<Point> &polyPoints);

   /// collide always returns true for Barrier objects.
   bool collide(GameObject *otherObject) { return true; }

   // Takes a list of vertices and converts them into a list of lines representing the edges of an object
   static void resetEdges(const Vector<Point> &corners, Vector<Point> &edges);

   // Simply takes a segment and "puffs it out" to a rectangle of a specified width, filling cornerPoints.  Does not modify endpoints.
   static void expandCenterlineToOutline(const Point &start, const Point &end, F32 width, Vector<Point> &cornerPoints);

   // Takes a segment and "puffs it out" to a rectangle for bot zone generation.
   // This rectangle is the width of the barrier plus the ship's collision radius added to the outside
   static void bufferBarrierForBotZone(const Point &start, const Point &end, F32 barrierWidth, Vector<Point> &bufferedPoints);
   static void bufferPolyWallForBotZone(const Vector<Point> &inputPoints, Vector<Point> &bufferedPoints);

   // Combines multiple barriers into a single complex polygon
   static bool unionBarriers(const Vector<DatabaseObject *> &barriers, bool useBotGeom, Vector<Vector<Point> > &solution);

   /// Clips the current set of render lines against the polygon passed as polyPoints, modifies lineSegmentPoints.
   static void clipRenderLinesToPoly(const Vector<DatabaseObject *> &barrierList, Vector<Point> &lineSegmentPoints);

   static void constructBarrierEndPoints(const Vector<Point> &vec, F32 width, Vector<Point> &barrierEnds);

   // Clean up edge geometry and get barriers ready for proper rendering
   static void prepareRenderingGeometry(Game *game);


   TNL_DECLARE_CLASS(Barrier);
};

};

#endif

