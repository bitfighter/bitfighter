//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _BARRIER_H_
#define _BARRIER_H_

#include "BfObject.h"

#include "Point.h"

#include "tnlTypes.h"
#include "tnlVector.h"

#include "gtest/gtest_prod.h"


namespace Zap
{


class BarrierX
{
private:
   Vector<WallSegment *> mSegments;

   Rect mSegmentExtent;

public:
   BarrierX();             // Constructor
   virtual ~BarrierX();    // Destructor

   void setSegments(const Vector<WallSegment *> &segments);
   const Vector<WallSegment *> &getSegments() const;

   const Rect &getSegmentExtent() const;
   S32 getSegmentCount() const;
   const WallSegment *getSegment(S32 index) const;

   bool isPointOnWall(const Point &point) const;

   void cloneSegments(const BarrierX *source);
};


////////////////////////////////////////
////////////////////////////////////////

/// The Barrier class represents rectangular barriers that player controlled
/// ships cannot pass through... i.e. walls  Barrier objects, once created, never
/// change state, simplifying the pack/unpack update methods.  Barriers are
/// constructed as an expanded line segment.
class Barrier : public BfObject
{
   typedef BfObject Parent;

private:
   Vector<Point> mPoints;  // The points of the barrier --> if only two, first will be start, second end of an old-school segment

   bool mIsPolywall;       // True if this represents a polywall

   // By precomputing and storing, we should ease the rendering cost
   Vector<Point> mRenderFillGeometry;        // Actual geometry used for rendering fill
   const Vector<Point> *mRenderOutlineGeometry;     // Actual geometry used for rendering outline

   F32 mWidth;

public:
   // Constructor
   Barrier(const Vector<Point> &points = Vector<Point>(), F32 width = DEFAULT_BARRIER_WIDTH, bool solid = false);
   virtual ~Barrier();


   static const S32 MIN_BARRIER_WIDTH = 1;         // Clipper doesn't much like 0 width walls
   static const S32 MAX_BARRIER_WIDTH = 2500;      // Geowar has walls 350+ units wide, so going lower will break at least one level

   static const S32 DEFAULT_BARRIER_WIDTH = 50;    // The default width of the barrier in game units

   static Vector<Point> mRenderLineSegments;       // The clipped line segments representing this barrier
   Vector<Point> mBotZoneBufferLineSegments;       // The line segments representing a buffered barrier

   static void constructBarriers (Game *game, const Vector<Point> &verts, F32 width);
   static void constructPolyWalls(Game *game, const Vector<Point> &verts);

   void renderLayer(S32 layerIndex);                                          // Renders barrier fill barrier-by-barrier
   static void renderEdges(const GameSettings *settings, S32 layerIndex);     // Renders all edges in one pass

   // Returns a sorting key for the object.  Barriers should be drawn first so as to appear behind other objects.
   S32 getRenderSortValue();

   // Returns the collision polygon of this barrier, which is the boundary extruded from the start,end line segment
   const Vector<Point> *getCollisionPoly() const;

   // Collide always returns true for Barrier objects
   bool collide(BfObject *otherObject);

   void getBufferForBotZone(F32 bufferRadius, Vector<Point> &points) const;

   // Clips the current set of render lines against the polygon passed as polyPoints, modifies lineSegmentPoints
   static void clipRenderLinesToPoly(const Vector<DatabaseObject *> &barrierList, Vector<Point> &lineSegmentPoints);

   // Combine multiple barriers into a single complex polygon
   static bool unionBarriers(const Vector<DatabaseObject *> &barriers, Vector<Vector<Point> > &solution);

   static void prepareRenderingGeometry(Game *game);
   static void clearRenderItems();

   // Test access
   FRIEND_TEST(IntegrationTest, LevelReadingAndItemPropagation);
};


};

#endif

