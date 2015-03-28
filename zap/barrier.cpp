//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "barrier.h"

#include "game.h"
#include "GameObjectRender.h"
#include "GeomUtils.h"
#include "Level.h"
#include "WallItem.h"      // For WallSegment def

#include "tnlLog.h"

#include <cmath>

using namespace TNL;

namespace Zap
{

using namespace LuaArgs;

// Statics
Vector<Point> Barrier::mRenderLineSegments;



// Constructor
BarrierX::BarrierX()
{
   // Do nothing
}


// Destructor
BarrierX::~BarrierX()
{
   mSegments.deleteAndClear();
}


void BarrierX::setSegments(const Vector<WallSegment *> &segments)
{
   mSegments.deleteAndClear();

   for(S32 i = 0; i < segments.size(); i++)   
   {
      mSegments.push_back(segments[i]);
      if(i == 0)
         mSegmentExtent.set(mSegments[i]->getExtent());
      else
         mSegmentExtent.unionRect(mSegments[i]->getExtent());
   }
}


const Vector<WallSegment *> &BarrierX::getSegments() const
{
   return mSegments;
}


const Rect &BarrierX::getSegmentExtent() const
{
   return mSegmentExtent;
}


S32 BarrierX::getSegmentCount() const
{
   return mSegments.size();   
}


const WallSegment *BarrierX::getSegment(S32 index) const
{
   return mSegments[index];
}


bool BarrierX::isPointOnWall(const Point &point) const
{
   for(S32 i = 0; i < mSegments.size(); i++)
      if(triangulatedFillContains(mSegments[i]->getTriangulatedFillPoints(), point))
         return true;

   return false;
}


void BarrierX::cloneSegments(const BarrierX *source)
{
   // First clear out (but do not delete) any existing segments.  These will be copies of the pointers
   // of the segments from source, and we don't want to delete those, as they're needed by source.
   mSegments.clear();

   for(S32 i = 0; i < source->getSegmentCount(); i++)
      mSegments.push_back(new WallSegment(source->getSegment(i), this));

}



////////////////////////////////////////
////////////////////////////////////////


// Constructor --> gets called from constructBarriers above
Barrier::Barrier(const Vector<Point> &points, F32 width, bool isPolywall)
{
   mObjectTypeNumber = BarrierTypeNumber;
   mPoints = points;

   if(points.size() < (isPolywall ? 3 : 2))      // Invalid barrier!
   {
      logprintf(LogConsumer::LogWarning, "Invalid barrier detected (has only one point).  Disregarding...");
      return;
   }

   Rect extent(points);

   F32 w = abs(width);

   mWidth = w;                      // Must be positive to avoid problem with bufferBarrierForBotZone
   w = w * 0.5f + 1;                // Divide by 2 to avoid double size extents, add 1 to avoid rounding errors

   if(points.size() == 2)           // It's a regular segment, need to make a little larger to accomodate width
      extent.expand(Point(w, w));


   setExtent(extent);

   mIsPolywall = isPolywall;

   if(isPolywall)
   {
      if(isWoundClockwise(mPoints))         // All walls must be CCW to clip correctly
         mPoints.reverse();

      Triangulate::Process(mPoints, mRenderFillGeometry);

      if(mRenderFillGeometry.size() == 0)    // Geometry is bogus; perhaps duplicated points, or other badness
      {
         logprintf(LogConsumer::LogWarning, "Invalid barrier detected (polywall with invalid geometry).  Disregarding...");
         return;
      }

      setNewGeometry(geomPolygon);
   }
   else     // Normal wall
   {
      if(mPoints.size() == 2 && mPoints[0] == mPoints[1])      // Test for zero-length barriers
         mPoints[1] += Point(0, 0.5);                          // Add vertical vector of half a point so we can see outline in-game

      if(mPoints.size() == 2 && mWidth != 0)                   // It's a regular segment, so apply width
         expandCenterlineToOutline(mPoints[0], mPoints[1], mWidth, mRenderFillGeometry);     // Fills mRenderFillGeometry with 4 points

      setNewGeometry(geomPolyLine);
   }

   // Outline is the same for regular walls and polywalls
   mRenderOutlineGeometry = getCollisionPoly(); 

   GeomObject::setGeom(*mRenderOutlineGeometry);
}


// Destructor
Barrier::~Barrier()
{
   // Do nothing
}


// Processes mPoints and fills polyPoints 
const Vector<Point> *Barrier::getCollisionPoly() const
{
   if(mIsPolywall)
      return &mPoints;
   else
      return &mRenderFillGeometry;
}


bool Barrier::collide(BfObject *otherObject)
{
   return true;
}


// Adds walls to the game database -- used when playing games, but not in the editor
// On client, is called when a wall object is sent from the server.
// static method
void Barrier::constructBarriers(Game *game, const Vector<Point> &verts, F32 width)
{
   if(verts.size() < 2)      // Enough verts?
      return;

   // First, fill a vector with barrier segments
   Vector<Point> barrierEnds;
   constructBarrierEndPoints(&verts, width, barrierEnds);

   Vector<Point> pts;      // Reusable container

   // Add individual segments to the game
   for(S32 i = 0; i < barrierEnds.size(); i += 2)
   {
      pts.clear();
      pts.push_back(barrierEnds[i]);
      pts.push_back(barrierEnds[i+1]);

      Barrier *b = new Barrier(pts, width, false);    // false = not solid
      b->addToGame(game, game->getLevel());
   }
}


// Adds polywalls to the game database -- used when playing games, but not in the editor
// On client, is called when a wall object is sent from the server.
// static method
void Barrier::constructPolyWalls(Game *game, const Vector<Point> &verts)
{
   if(verts.size() < 3)      // Enough verts?
      return;
  
   Barrier *b = new Barrier(verts, 1, true);
   b->addToGame(game, game->getLevel());
}


// Server only -- fills points
void Barrier::getBufferForBotZone(F32 bufferRadius, Vector<Point> &points) const
{
   // Use a clipper library to buffer polywalls; should be counter-clockwise by here
   if(mIsPolywall)
      offsetPolygon(&mPoints, points, bufferRadius);

   // If a barrier, do our own buffer
   // Puffs out segment to the specified width with a further buffer for bot zones, has an inset tangent corner cut
   else
   {
      const Point &start = mPoints[0];
      const Point &end   = mPoints[1];
      Point difference   = end - start;

      Point crossVector(difference.y, -difference.x);          // Create a point whose vector from 0,0 is perpenticular to the original vector
      crossVector.normalize((mWidth * 0.5f) + bufferRadius);   // Reduce point so the vector has length of barrier width + ship radius

      Point parallelVector(difference.x, difference.y);        // Create a vector parallel to original segment
      parallelVector.normalize(bufferRadius);                  // Reduce point so vector has length of ship radius

      // For octagonal zones
      //   create extra vectors that are offset full offset to create 'cut' corners
      //   (FloatSqrtHalf * BotNavMeshZone::BufferRadius)  creates a tangent to the radius of the buffer
      //   we then subtract a little from the tangent cut to shorten the buffer on the corners and allow zones to be created when barriers are close
      Point crossPartial = crossVector;
      crossPartial.normalize((FloatSqrtHalf * bufferRadius) + (mWidth * 0.5f) - (0.3f * bufferRadius));

      Point parallelPartial = parallelVector;
      parallelPartial.normalize((FloatSqrtHalf * bufferRadius) - (0.3f * bufferRadius));

      // Now add/subtract perpendicular and parallel vectors to buffer the segments
      points.push_back((start - parallelVector)  + crossPartial);
      points.push_back((start - parallelPartial) + crossVector);
      points.push_back((end   + parallelPartial) + crossVector);
      points.push_back((end   + parallelVector)  + crossPartial);
      points.push_back((end   + parallelVector)  - crossPartial);
      points.push_back((end   + parallelPartial) - crossVector);
      points.push_back((start - parallelPartial) - crossVector);
      points.push_back((start - parallelVector)  - crossPartial);
   }
}


void Barrier::clearRenderItems()
{
   mRenderLineSegments.clear();
}


// Merges wall outlines together, client only
// This is used for barriers and polywalls
void Barrier::prepareRenderingGeometry(Game *game)    // static
{
   mRenderLineSegments.clear();

   Vector<DatabaseObject *> barrierList;

   game->getLevel()->findObjects((TestFunc)isWallType, barrierList);

   clipRenderLinesToPoly(barrierList, mRenderLineSegments);
}


// Clears out overlapping barrier lines for better rendering appearance, modifies lineSegmentPoints.
// This is effectively called on every pair of potentially intersecting barriers, and lineSegmentPoints gets 
// refined as each additional intersecting barrier gets processed.
void Barrier::clipRenderLinesToPoly(const Vector<DatabaseObject *> &barrierList, Vector<Point> &lineSegmentPoints)  
{
   Vector<Vector<Point> > solution;

   unionBarriers(barrierList, solution);

   unpackPolygons(solution, lineSegmentPoints);
}


// Combines multiple barriers into a single complex polygon... fills solution
bool Barrier::unionBarriers(const Vector<DatabaseObject *> &barriers, Vector<Vector<Point> > &solution)
{
   Vector<const Vector<Point> *> inputPolygons;
   Vector<Point> points;

   for(S32 i = 0; i < barriers.size(); i++)
      inputPolygons.push_back(static_cast<Barrier *>(barriers[i])->getCollisionPoly());

   return mergePolys(inputPolygons, solution);
}


// Render wall fill only for this wall; all edges rendered in a single pass later
void Barrier::renderLayer(S32 layerIndex)
{
#ifndef ZAP_DEDICATED
   static const Color fillColor(GameSettings::get()->getWallFillColor());

   if(layerIndex == 0)           // First pass: draw the fill
      GameObjectRender::renderWallFill(&mRenderFillGeometry, fillColor, mIsPolywall);
#endif
}


// Render all edges for all barriers... faster to do it all at once than try to sort out whose edges are whose.
// Static method.
void Barrier::renderEdges(const GameSettings *settings, S32 layerIndex)
{
   static const Color outlineColor(settings->getWallOutlineColor());

   if(layerIndex == 1)
      GameObjectRender::renderWallEdges(mRenderLineSegments, outlineColor);
}


S32 Barrier::getRenderSortValue()
{
   return 0;
}


};
