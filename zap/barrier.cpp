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

#include "barrier.h"
#include "BotNavMeshZone.h"
#include "gameObjectRender.h"
#include "GeomUtils.h"    // For polygon triangulation

#include "gameType.h"          // For BarrierRec struct

#include "glutInclude.h"
#include <cmath>               // C++ version of this headers includes float overloads

using namespace TNL;

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Barrier);

Vector<Point> Barrier::mRenderLineSegments;

extern void removeCollinearPoints(Vector<Point> &points, bool isPolygon);

bool loadBarrierPoints(const BarrierRec &barrier, Vector<Point> &points)
{
   // Convert the list of floats into a list of points
   for(S32 i = 1; i < barrier.verts.size(); i += 2)
      points.push_back( Point(barrier.verts[i-1], barrier.verts[i]) );

   removeCollinearPoints(points, false);   // Remove collinear points to make rendering nicer and datasets smaller

   return (points.size() >= 2);
}


void constructBarriers(Game *theGame, const BarrierRec &barrier)
{
   Vector<Point> vec;

   if(!loadBarrierPoints(barrier, vec))
      return;

   if(barrier.solid)   // This is a solid polygon
   {
      if(vec.first() == vec.last())      // Does our barrier form a closed loop?
         vec.erase(vec.size() - 1);      // If so, remove last vertex

      Barrier *b = new Barrier(vec, barrier.width, true);
      b->addToGame(theGame);
   }
   else        // This is a standard series of segments
   {
      // First, fill a vector with barrier segments
      Vector<Point> barrierEnds;
      Barrier::constructBarrierEndPoints(vec, barrier.width, barrierEnds);

      Vector<Point> pts;
      // Then add individual segments to the game
      for(S32 i = 0; i < barrierEnds.size(); i += 2)
      {
         pts.clear();
         pts.push_back(barrierEnds[i]);
         pts.push_back(barrierEnds[i+1]);

         Barrier *b = new Barrier(pts, barrier.width, false);    // false = not solid
         b->addToGame(theGame);
      }
   }
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor --> gets called from constructBarriers above
Barrier::Barrier(const Vector<Point> &points, F32 width, bool solid)
{
   mObjectTypeMask = BarrierType | CommandMapVisType;
   mPoints = points;

   if(points.size() < 2)      // Invalid barrier!
   {
      delete this;
      logprintf(LogConsumer::LogWarning, "Invalid barrier detected (has only one point).  Disregarding...");
      return;
   }

   Rect extent(points);

   if(width < 0)             // Force positive width
      width = -width;

   mWidth = width;           // must be positive to avoid problem with bufferBarrierForBotZone
   width = width * 0.5 + 1;  // divide by 2 to avoid double size extents, add 1 to avoid rounding errors.
   if(points.size() == 2)    // It's a regular segment, need to make a little larger to accomodate width
      extent.expand(Point(width, width));
   // use mWidth, not width, for anything below this.

   setExtent(extent);

    mSolid = solid;

   if(mSolid)
   {
      Triangulate::Process(mPoints, mRenderFillGeometry);

      if(mRenderFillGeometry.size() == 0)      // Geometry is bogus; perhaps duplicated points, or other badness
      {
         delete this;
         logprintf(LogConsumer::LogWarning, "Invalid barrier detected (polywall with invalid geometry).  Disregarding...");
         return;
      }

      bufferPolyWallForBotZone(mPoints, mBotZoneBufferGeometry);
   }
   else
   {
      bufferBarrierForBotZone(mPoints[0], mPoints[1], mWidth, mBotZoneBufferGeometry);     // Fills with 8 points, octagonal

      if(mPoints.size() == 2 && mWidth != 0)   // It's a regular segment, so apply width
      {
         expandCenterlineToOutline(mPoints[0], mPoints[1], mWidth, mRenderFillGeometry);     // Fills with 4 points
         mPoints = mRenderFillGeometry;
      }
   }

   getCollisionPoly(mRenderOutlineGeometry);    // Outline is the same for both barrier geometries
}


void Barrier::onAddedToGame(Game *theGame)
{
  getGame()->mObjectsLoaded++;
}


// Combines multiple barriers into a single complex polygon... fills solution
bool Barrier::unionBarriers(const Vector<DatabaseObject *> &barriers, bool useBotGeom, TPolyPolygon &solution)
{
   TPolyPolygon inputPolygons;
   TPolygon inputPoly;

   // Reusable container
   Vector<Point> points;

   for(S32 i = 0; i < barriers.size(); i++)
   {
      Barrier *barrier = dynamic_cast<Barrier *>(barriers[i]);

      if(!barrier)
         continue;

      inputPoly.clear();
      if(useBotGeom)
      {
         for (S32 j = 0; j < barrier->mBotZoneBufferGeometry.size(); j++)
            inputPoly.push_back(DoublePoint(barrier->mBotZoneBufferGeometry[j].x, barrier->mBotZoneBufferGeometry[j].y));
      }
      else    
      {
         barrier->getCollisionPoly(points);        // Puts object bounds into points
         for (S32 j = 0; j < points.size(); j++)
            inputPoly.push_back(DoublePoint(points[j].x, points[j].y));
      }

#ifdef DUMP_DATA
      for (S32 j = 0; j < barrier->mBotZoneBufferGeometry.size(); j++)
         logprintf("Before Clipper Point: %f %f", barrier->mBotZoneBufferGeometry[j].x, barrier->mBotZoneBufferGeometry[j].y);
#endif

      inputPolygons.push_back(inputPoly);
   }

   // Fire up clipper and union!
   Clipper clipper;
   clipper.IgnoreOrientation(false);
   clipper.AddPolyPolygon(inputPolygons, ptSubject);
   return clipper.Execute(ctUnion, solution, pftNonZero, pftNonZero);
}


// Processes mPoints and fills polyPoints 
bool Barrier::getCollisionPoly(Vector<Point> &polyPoints)
{
   polyPoints = mPoints;
   return true;
}


// Takes a list of vertices and converts them into a list of lines representing the edges of an object -- static method
void Barrier::resetEdges(const Vector<Point> &corners, Vector<Point> &edges)
{
   edges.clear();

   S32 last = corners.size() - 1;             
   for(S32 i = 0; i < corners.size(); i++)
   {
      edges.push_back(corners[last]);
      edges.push_back(corners[i]);
      last = i;
   }
}


// Given the points in vec, figure out where the ends of the walls should be (they'll need to be extended slighly in some cases
// for better rendering).  Set extendAmt to 0 to see why it's needed.
// Populates barrierEnds with the results.
void Barrier::constructBarrierEndPoints(const Vector<Point> &vec, F32 width, Vector<Point> &barrierEnds)    // static
{
   barrierEnds.clear();    // local static vector

   if(vec.size() <= 1)     // Protect against bad data
      return;

   bool loop = (vec.first() == vec.last());      // Does our barrier form a closed loop?

   Vector<Point> edgeVector;
   for(S32 i = 0; i < vec.size() - 1; i++)
   {
      Point e = vec[i+1] - vec[i];
      e.normalize();
      edgeVector.push_back(e);
   }

   Point lastEdge = edgeVector[edgeVector.size() - 1];
   Vector<F32> extend;

   for(S32 i = 0; i < edgeVector.size(); i++)
   {
      Point curEdge = edgeVector[i];
      double cosTheta = curEdge.dot(lastEdge);

      // Do some bounds checking.  Crazy, I know, but trust me, it's worth it!
      if (cosTheta > 1.0)
         cosTheta = 1.0;
      else if(cosTheta < -1.0)  
         cosTheta = -1.0;

      cosTheta = abs(cosTheta);     // Seems to reduce "end gap" on acute junction angles
      
      F32 extendAmt = width * 0.5 * tan( acos(cosTheta) / 2 );
      if(extendAmt > 0.01)
         extendAmt -= 0.01;
      extend.push_back(extendAmt);
   
      lastEdge = curEdge;
   }

   F32 first = extend[0];
   extend.push_back(first);

   for(S32 i = 0; i < edgeVector.size(); i++)
   {
      F32 extendBack = extend[i];
      F32 extendForward = extend[i+1];
      if(i == 0 && !loop)
         extendBack = 0;
      if(i == edgeVector.size() - 1 && !loop)
         extendForward = 0;

      Point start = vec[i] - edgeVector[i] * extendBack;
      Point end = vec[i+1] + edgeVector[i] * extendForward;
      barrierEnds.push_back(start);
      barrierEnds.push_back(end);
   }
}


// Simply takes a segment and "puffs it out" to a rectangle of a specified width, filling cornerPoints.  Does not modify endpoints.
void Barrier::expandCenterlineToOutline(const Point &start, const Point &end, F32 width, Vector<Point> &cornerPoints)
{
   cornerPoints.clear();

   Point dir = end - start;
   Point crossVec(dir.y, -dir.x);
   crossVec.normalize(width * 0.5);

   cornerPoints.push_back(Point(start.x + crossVec.x, start.y + crossVec.y));
   cornerPoints.push_back(Point(end.x + crossVec.x, end.y + crossVec.y));
   cornerPoints.push_back(Point(end.x - crossVec.x, end.y - crossVec.y));
   cornerPoints.push_back(Point(start.x - crossVec.x, start.y - crossVec.y));
}

// Puffs out segment to the specified width with a further buffer for bot zones, has an inset tangent corner cut
void Barrier::bufferBarrierForBotZone(const Point &start, const Point &end, F32 barrierWidth, Vector<Point> &bufferedPoints)
{
   bufferedPoints.clear();

   Point difference;
   if (start == end)  // test for zero-length barriers
      difference = (end + Point(0,1)) - start;
   else
      difference = end - start;

   Point crossVector(difference.y, -difference.x);  // create a point whose vector from 0,0 is perpenticular to the original vector
   crossVector.normalize((barrierWidth * 0.5) + BotNavMeshZone::BufferRadius);  // reduce point so the vector has length of barrier width + ship radius

   Point parallelVector(difference.x, difference.y); // create a vector parallel to original segment
   parallelVector.normalize(BotNavMeshZone::BufferRadius);  // reduce point so vector has length of ship radius

   // for octagonal zones
   //   create extra vectors that are offset full offset to create 'cut' corners
   //   (0.5 * sqrt(2.0) * BotNavMeshZone::BufferRadius)  creates a tangent to the radius of the buffer
   //   we then subtract a little from the tangent cut to shorten the buffer on the corners and allow zones to be created when barriers are close
   Point crossPartial = crossVector;
   crossPartial.normalize((0.5 * sqrt(2.0) * BotNavMeshZone::BufferRadius) + (barrierWidth * 0.5) - (0.30 * BotNavMeshZone::BufferRadius));
   Point parallelPartial = parallelVector;
   parallelPartial.normalize((0.5 * sqrt(2.0) * BotNavMeshZone::BufferRadius) - (0.30 * BotNavMeshZone::BufferRadius));

   // now add/subtract perpendicular and parallel vectors to buffer the segments
   bufferedPoints.push_back(Point((start.x - parallelVector.x) + crossPartial.x, (start.y - parallelVector.y) + crossPartial.y));
   bufferedPoints.push_back(Point((start.x - parallelPartial.x) + crossVector.x, (start.y - parallelPartial.y) + crossVector.y));
   bufferedPoints.push_back(Point(end.x + parallelPartial.x + crossVector.x, end.y + parallelPartial.y + crossVector.y));
   bufferedPoints.push_back(Point(end.x + parallelVector.x + crossPartial.x, end.y + parallelVector.y + crossPartial.y));
   bufferedPoints.push_back(Point(end.x + parallelVector.x - crossPartial.x, end.y + parallelVector.y - crossPartial.y));
   bufferedPoints.push_back(Point(end.x + parallelPartial.x - crossVector.x, end.y + parallelPartial.y - crossVector.y));
   bufferedPoints.push_back(Point((start.x - parallelPartial.x) - crossVector.x, (start.y - parallelPartial.y) - crossVector.y));
   bufferedPoints.push_back(Point((start.x - parallelVector.x) - crossPartial.x, (start.y - parallelVector.y) - crossPartial.y));
}


void Barrier::bufferPolyWallForBotZone(const Vector<Point>& inputPoints, Vector<Point>& bufferedPoints)
{
   if (isWoundClockwise(inputPoints))  // Must make CCW for clipper's offset method to work
   {
      Vector<Point> reversePoints;
      for(S32 i = inputPoints.size() - 1; i >= 0; i--)
         reversePoints.push_back(inputPoints[i]);

      offsetPolygon(reversePoints, bufferedPoints, BotNavMeshZone::BufferRadius);
   }
   else
      offsetPolygon(inputPoints, bufferedPoints, BotNavMeshZone::BufferRadius);
}


extern Rect gServerWorldBounds;

// Clears out overlapping barrier lines for better rendering appearance, modifies lineSegmentPoints.
// This is effectively called on every pair of potentially intersecting barriers, and lineSegmentPoints gets 
// refined as each additional intersecting barrier gets processed.
// static method
void Barrier::clipRenderLinesToPoly(Vector<Point> &lineSegmentPoints)
{
   TPolyPolygon solution;

   Vector<DatabaseObject *> barrierList;
   gServerGame->getGridDatabase()->findObjects(BarrierType, barrierList, gServerWorldBounds);

   unionBarriers(barrierList, false, solution);


   TPolygon poly;
   for (U32 j = 0; j < solution.size(); j++)
   {
      poly = solution[j];

      if(poly.size() == 0)
         continue;

      for (U32 k = 1; k < poly.size(); k++)
      {
         lineSegmentPoints.push_back(Point((F32)poly[k-1].X, (F32)poly[k-1].Y));
         lineSegmentPoints.push_back(Point((F32)poly[k].X,   (F32)poly[k].Y));
      }

      // Close the loop
      lineSegmentPoints.push_back(Point((F32)poly[poly.size()-1].X, (F32)poly[poly.size()-1].Y));
      lineSegmentPoints.push_back(Point((F32)poly[0].X, (F32)poly[0].Y));
   }
}


S32 QSORT_CALLBACK pointDataSortX(Point *a, Point *b)
{
   if(a->x == b->x)
      return 0;

   return (a->x > b->x) ? 1 : -1;
}


S32 QSORT_CALLBACK pointDataSortY(Point *a, Point *b)
{
   if(a->y == b->y)
      return 0;

   return (a->y > b->y) ? 1 : -1;
}


void Barrier::prepareRenderGeom(Vector<Point> &segments)
{
   //resetEdges(outlines, segments);
   segments.clear();
   clipRenderLinesToPoly(segments);
}


// Merges wall outlines together
void Barrier::prepareRenderingGeometry()
{
   prepareRenderGeom(mRenderLineSegments);
}


// Create buffered edge geometry around the barrier for bot zone generation
void Barrier::prepareBotZoneGeometry()
{
   prepareRenderGeom(mBotZoneBufferLineSegments);
}


// Render wall fill only for this wall; all edges rendered in a single pass
void Barrier::render(S32 layerIndex)
{
   if(layerIndex == 0)           // First pass: draw the fill
      renderWallFill(mRenderFillGeometry, mSolid);
}

// static 
void Barrier::renderEdges(S32 layerIndex)
{
   if(layerIndex == 1)
      renderWallEdges(mRenderLineSegments);
}

};
