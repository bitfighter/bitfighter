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
#include "SweptEllipsoid.h"    // For polygon triangulation

#include "gameType.h"          // For BarrierRec struct

#include "glutInclude.h"
#include <cmath>               // C++ version of this headers includes float overloads

using namespace TNL;

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Barrier);



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
      return;
   }

   Rect extent(points);

   mWidth = width;

   if(points.size() == 2)    // It's a regular segment, need to make a little larger to accomodate width
      extent.expand(Point(width, width));

   setExtent(extent);

    mSolid = solid;

   if(mSolid)
       Triangulate::Process(mPoints, mRenderFillGeometry);
   else if(mPoints.size() == 2 && mWidth > 0)   // It's a regular segment, so apply width
   {
      expandCenterlineToOutline(mPoints[0], mPoints[1], mWidth, mRenderFillGeometry);     // Fills with 4 points
      bufferBarrierForBotZone(mPoints[0], mPoints[1], mWidth, mBotZoneBufferGeometry);     // Fills with 4 points
      mPoints = mRenderFillGeometry;
   }

   getCollisionPoly(mRenderOutlineGeometry);    // Outline is the same for both barrier geometries
}


void Barrier::onAddedToGame(Game *theGame)
{
  getGame()->mObjectsLoaded++;
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

// Puffs out segment to the specified width with a further buffer for bot zones
void Barrier::bufferBarrierForBotZone(const Point &start, const Point &end, F32 barrierWidth, Vector<Point> &bufferedPoints)
{
   bufferedPoints.clear();

   Point difference = end - start;
   Point crossVector(difference.y, -difference.x);  // create a point whose vector from 0,0 is perpenticular to the original vector
   crossVector.normalize((barrierWidth * 0.5) + BotNavMeshZone::BufferRadius);  // reduce point so the vector has length of barrier width + ship radius

   Point parallelVector(difference.x, difference.y); // create a vector parallel to original segment
   parallelVector.normalize(BotNavMeshZone::BufferRadius);  // reduce point so vector has length of ship radius

   // now add/subtract perpendicular and parallel vectors to buffer the segments
   bufferedPoints.push_back(Point((start.x - parallelVector.x) + crossVector.x, (start.y - parallelVector.y) + crossVector.y));
   bufferedPoints.push_back(Point(end.x + parallelVector.x + crossVector.x, end.y + parallelVector.y + crossVector.y));
   bufferedPoints.push_back(Point(end.x + parallelVector.x - crossVector.x, end.y + parallelVector.y - crossVector.y));
   bufferedPoints.push_back(Point((start.x - parallelVector.x) - crossVector.x, (start.y - parallelVector.y) - crossVector.y));
}


// Clears out overlapping barrier lines for better rendering appearance, modifies lineSegmentPoints.
// This is effectively called on every pair of potentially intersecting barriers, and lineSegmentPoints gets 
// refined as each additional intersecting barrier gets processed.
// static method
void Barrier::clipRenderLinesToPoly(const Vector<Point> &polyPoints, Vector<Point> &lineSegmentPoints)
{
   Vector<Point> clippedSegments;

   // Loop through all the segments
   for(S32 i = 0; i < lineSegmentPoints.size(); i+= 2)
   {
      Point rp1 = lineSegmentPoints[i];
      Point rp2 = lineSegmentPoints[i + 1];

      Point cp1 = polyPoints[polyPoints.size() - 1];
      for(S32 j = 0; j < polyPoints.size(); j++)
      {
         Point cp2 = polyPoints[j];
         Point ce = cp2 - cp1;
         Point n(-ce.y, ce.x);

         n.normalize();
         F32 distToZero = n.dot(cp1);

         F32 d1 = n.dot(rp1);
         F32 d2 = n.dot(rp2);

         // Setting the following comparisons to >= will cause collinear end segments to go away, but will
         // cause overlapping walls to disappear
         bool d1in = (d1 > distToZero);
         bool d2in = (d2 > distToZero);

         if(!d1in && !d2in) // Both points are outside this edge of the poly...
         {
            // ...so add them to the render poly
            clippedSegments.push_back(rp1);
            clippedSegments.push_back(rp2);
            break;
         }
         else if((d1in && !d2in) || (d2in && !d1in))
         {
            // Find the clip intersection point:
            F32 t = (distToZero - d1) / (d2 - d1);
            Point clipPoint = rp1 + (rp2 - rp1) * t;

            if(d1in)
            {
               clippedSegments.push_back(clipPoint);
               clippedSegments.push_back(rp2);
               rp2 = clipPoint;
            }
            else
            {
               clippedSegments.push_back(rp1);
               clippedSegments.push_back(clipPoint);
               rp1 = clipPoint;
            }
         }

         // If both are in, go to the next edge
         cp1 = cp2;
      }
   }

   lineSegmentPoints = clippedSegments;
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


//#define ROUND(x) (((x) < 0) ? F32(S32((x) - 0.5)) : F32(S32((x) + 0.5)))
#define ROUND(x) (x)

void getPolygonLineCollisionPoints(Vector<Point> &output, const Vector<Point> &input, Point start, Point end);

// Sam's optimized version, replaces prepareRenderingGeometry(), leaves small holes
// Clean up edge geometry and get barriers ready for proper rendering -- client and server (client for rendering, server for building zones)
void Barrier::prepareRenderingGeometry2()
{
   GridDatabase *gridDB = getGridDatabase();
   S32 i_prev = mPoints.size()-1;
   mRenderOutlineGeometry.clear();

   // Reusable containers
   Vector<Point> points;
   Vector<Point> boundaryPoints;
   Vector<DatabaseObject *> objects;
   Rect rect;
   Point offset(0.003, 0.007);

   for(S32 i = 0; i < mPoints.size(); i++)
   {
      points.clear();
      objects.clear();

      rect.set(mPoints[i], mPoints[i_prev]);                // Creates bounding box around these two points

      points.push_back(mPoints[i]);
      points.push_back(mPoints[i_prev]);
      gridDB->findObjects(BarrierType, objects, rect);      // Find all barriers in bounding box of mPoints[i] & mPoints[i_prev], fills objects

      for(S32 j = objects.size() - 1; j >= 0; j--)
      {
         Barrier *wall = dynamic_cast<Barrier *>(objects[j]);
         if(wall == this || wall == NULL)                   // Self or invalid object...              
            objects.erase_fast(j);                          // ...remove from list (will need cleaned list later)
         else
         {
            wall->getCollisionPoly(boundaryPoints);         // Put wall outline into boundaryPoints

            // Fill points with intersections of wall outline and segment mPoints[i] -> mPoints[i_prev]
            getPolygonLineCollisionPoints(points, boundaryPoints, mPoints[i], mPoints[i_prev]);  
         }
      }

      // Now points contains all intersections of all our walls and the segment mPoints[i] -> mPoints[i_prev]

      // Make sure points is spatially sorted
      if(abs(mPoints[i].x - mPoints[i_prev].x) > abs(mPoints[i].y - mPoints[i_prev].y))
         points.sort(pointDataSortX);
      else
         points.sort(pointDataSortY);

      // Remove duplicate points -- due to sorting, dupes will be adjacent to one another
      for(S32 j = points.size() - 1; j >=1 ; j--)
         if(points[j] == points[j-1])
            points.erase(j);

      for(S32 j = 1; j < points.size(); j++)
      {
         // Create a pair of midpoints, each a tiny bit offset from the true center
         Point midPoint  = (points[j] + points[j-1]) * 0.5 + offset;    // To avoid missing lines, duplicate segments are better then mising ones
         Point midPoint2 = (points[j] + points[j-1]) * 0.5 - offset;

         bool isInside = false;

         // Loop through all walls we found earlier, and see if either our offsetted midpoints falls inside any of those wall outlines
         for(S32 k = 0; k < objects.size() && !isInside; k++)           
         {
            Barrier *obj = dynamic_cast<Barrier *>(objects[k]);
            isInside = (PolygonContains2(obj->mPoints.address(), obj->mPoints.size(), midPoint)
                                 && PolygonContains2(obj->mPoints.address(), obj->mPoints.size(), midPoint2));
         }
         if(!isInside)     // No -- add segment to our collection to be rendered
         {
            Point rounded(ROUND(points[j-1].x), ROUND(points[j-1].y));
            mRenderLineSegments.push_back(rounded);

            rounded.set(ROUND(points[j].x), ROUND(points[j].y));
            mRenderLineSegments.push_back(rounded);
         }
      }

      i_prev = i;
   }
}


// Original method, creates too many segments
void Barrier::prepareRenderGeom(Vector<Point> &outlines, Vector<Point> &segments)
{
   resetEdges(outlines, segments);

   static Vector<DatabaseObject *> fillObjects;
   fillObjects.clear();

   findObjects(BarrierType, fillObjects, getExtent());      // Find all potentially colliding wall segments (fillObjects)

   for(S32 i = 0; i < fillObjects.size(); i++)
   {
      outlines.clear();
      if(fillObjects[i] != this && dynamic_cast<GameObject *>(fillObjects[i])->getCollisionPoly(outlines))
         clipRenderLinesToPoly(outlines, segments);     // Populates segments
   }
}


void Barrier::prepareRenderingGeometry()
{
   prepareRenderGeom(mRenderOutlineGeometry, mRenderLineSegments);
}


// Create buffered edge geometry around the barrier for bot zone generation
void Barrier::prepareBotZoneGeometry()
{
   prepareRenderGeom(mBotZoneBufferGeometry, mBotZoneBufferLineSegments);
}


void Barrier::render(S32 layerIndex)
{
   if(layerIndex == 0)           // First pass: draw the fill
   {
      glColor(gIniSettings.wallFillColor);

      glBegin(mSolid ? GL_TRIANGLES : GL_POLYGON);   // Rendering is a bit different for solid polys
         for(S32 i = 0; i < mRenderFillGeometry.size(); i++)
            glVertex(mRenderFillGeometry[i]);
      glEnd();

   }
   else if(layerIndex == 1)      // Second pass: draw the outlines
      renderWallEdges(mRenderLineSegments);
}

};
