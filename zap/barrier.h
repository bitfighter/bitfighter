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
#include "point.h"
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

   F32 mWidth;

   static const S32 BarrierWidth = 50; ///< The default width of the barrier in game units

   static U32 mBarrierChangeIndex; ///< Global counter that is incremented every time a new barrier is added on the client.
   U32 mLastBarrierChangeIndex;    ///< Index to check against the global counter - if it is different, then this barrier's polygon outline will be clipped against all adjacent barriers.

   Vector<Point> mRenderLineSegments; ///< The clipped line segments representing this barrier.

   /// Barrier constructor
   Barrier(const Vector<Point> &points = Vector<Point>(), F32 width = BarrierWidth, bool solid = false);

   /// Adds the server object to the net interface's scope always list
   void onAddedToGame(Game *theGame);

   /// renders this barrier by drawing the render line segments,
   void render(S32 layer);

   /// returns a sorting key for the object.  Barriers should sort behind other objects
   S32 getRenderSortValue() { return 0; }

   /// returns the collision polygon of this barrier, which is the boundary extruded from the start,end line segment.
   bool getCollisionPoly(Vector<Point> &polyPoints);

   /// collide always returns true for Barrier objects.
   bool collide(GameObject *otherObject) { return true; }

   /// clips the current set of render lines against the polygon passed as polyPoints, modifies lineSegmentPoints.
   //void clipRenderLinesToPoly(const Vector<Point> &polyPoints, Vector<Point> &lineSegmentPoints);

   TNL_DECLARE_CLASS(Barrier);
};

};

#endif

