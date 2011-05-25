//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer TNL::Vector graphics space game
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


// Example code for: Collision Detection with Swept Spheres and Ellipsoids
// See: http://www.three14.demon.nl/sweptellipsoid/SweptEllipsoid.pdf
//
// Copyright (C) 2003 Jorrit Rouwe
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// This is free software, you can do with it what you want.
//
// This file contains the main swept sphere / ellipsoid intersection tests.
//
// Please have a look at the notes. They indicate obvious places for optimization
// if you are using a swept ellipsoid against a large number of polygons.

// this file has been modified by Mark Frohnmayer:
// removed swept ellipsoid routine
// removed swept sphere
// removed references to TNL::Vector2 and replaced with Point
// added in prototypes for circle collisions

#ifndef _GEOM_UTILS_
#define _GEOM_UTILS_

#include <string>
#include <vector>

#include "tnlTypes.h"

// forward declarations
namespace TNL {
   template<class T> class Vector;
};
namespace clipper {
   struct IntPoint;
   typedef std::vector<IntPoint> Polygon;
   typedef std::vector<Polygon> Polygons;
};
struct rcPolyMesh;

namespace Zap {

class Point;
class Rect;

// Test between a polygon and a swept sphere with radius inRadius moving from inBegin to inBegin + inDelta
// If there is an intersection the intersection position is returned in outPoint and the center of the
// sphere is at inBegin + outFraction * inDelta when it collides
//bool PolygonSweptSphereIntersect(const Plane &inPlane, const TNL::Vector2 *inVertices, int inNumVertices, const TNL::Vector3 &inBegin, const TNL::Vector3 &inDelta, float inRadius, TNL::Vector3 &outPoint, float &outFraction);

// Test intersection with a swept ellipsoid with principal axis inAxis1, inAxis2, inAxis3 moving from inBegin to inBegin + inDelta
// If there is an intersection the intersection position is returned in outPoint and the center of the
// sphere is at inBegin + outFraction * inDelta when it collides
//bool PolygonSweptEllipsoidIntersect(const Plane &inPlane, const TNL::Vector2 *inVertices, int inNumVertices, const TNL::Vector3 &inBegin, const TNL::Vector3 &inDelta, const TNL::Vector3 &inAxis1, const TNL::Vector3 &inAxis2, const TNL::Vector3 &inAxis3, TNL::Vector3 &outPoint, float &outFraction);

bool PolygonSweptCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inBegin, const Point &inDelta, TNL::F32 inRadius, Point &outPoint, TNL::F32 &outFraction);
bool PolygonContains2(const Point *inVertices, int inNumVertices, const Point &inPoint);
bool segmentsColinear(const Point &p1, const Point &p2, const Point &p3, const Point &p4, TNL::F32 scaleFact);
bool segsOverlap(const Point &p1, const Point &p2, const Point &p3, const Point &p4, Point &overlapStart, Point &overlapEnd);
bool zonesTouch(const TNL::Vector<Point> *zone1, const TNL::Vector<Point> *zone2, TNL::F32 scaleFact, Point &overlapStart, Point &overlapEnd);
bool pointOnSegment(const Point &c, const Point &a, const Point &b, TNL::F32 closeEnough);
bool polygonCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inCenter, TNL::F32 inRadiusSq, Point &outPoint);

bool polygonsIntersect(const TNL::Vector<Point> &p1, const TNL::Vector<Point> &p2);
bool polygonIntersectsSegment(const TNL::Vector<Point> &points, const Point &start, const Point &end);  // This is four times faster than the Detailed one.
bool polygonIntersectsSegmentDetailed(Point *poly, TNL::U32 vertexCount, bool format, const Point &start, const Point &end, float &collisionTime, Point &normal);
bool circleIntersectsSegment(Point center, float radius, Point start, Point end, float &collisionTime);

Point findCentroid(const TNL::Vector<Point> &polyPoints);
TNL::F32 area(const TNL::Vector<Point> &polyPoints);
TNL::F32 angleOfLongestSide(const TNL::Vector<Point> &polyPoints);

// These functions returns true if it found an appropriate point, false if not
bool findNormalPoint(const Point &p, const Point &s1, const Point &s2, Point &closest);
bool segmentsIntersect(const Point &p1, const Point &p2, const Point &p3, const Point &p4);
bool findIntersection(const Point &p1, const Point &p2, const Point &p3, const Point &p4, Point &intersection);

void removeCollinearPoints(TNL::Vector<Point> &points, bool isPolygon);

// TODO: Should we create a MathUtils class for this and other useful purely mathematic functions?
bool FindLowestRootInInterval(TNL::F32 inA, TNL::F32 inB, TNL::F32 inC, TNL::F32 inUpperBound, TNL::F32 &outX);

// Returns index of points TNL::Vector closest to point
TNL::S32 findClosestPoint(const Point &point, const TNL::Vector<Point> &points);

// Offset a complex polygon by a given amount
void offsetPolygon(const TNL::Vector<Point>& inputPoly, TNL::Vector<Point>& outputPoly, const TNL::F32 offset);

// Use Clipper to merge inputPolygons, placing the result in solution
bool mergePolys(const TNL::Vector<TNL::Vector<Point> > &inputPolygons, TNL::Vector<TNL::Vector<Point> > &outputPolygons);

// Convert a Polygons to a list of points in a-b b-c c-d d-a format
void unpackPolygons(const TNL::Vector<TNL::Vector<Point> > &solution, TNL::Vector<Point> &lineSegmentPoints);

// test if a complex polygon has clockwise point winding order
bool isWoundClockwise(const TNL::Vector<Point>& inputPoly);

bool isConvex(const TNL::Vector<Point> *verts);

// scale Geometric points for clipper
clipper::Polygons upscaleClipperPoints(const TNL::Vector<TNL::Vector<Point> >& inputPolygons);
TNL::Vector<TNL::Vector<Point> > downscaleClipperPoints(const clipper::Polygons& inputPolygons);

/*****************************************************************/
/** Static class to triangulate any contour/polygon efficiently **/
/** You should replace TNL::Vector2d with whatever your own TNL::Vector   **/
/** class might be.  Does not support polygons with holes.      **/
/** Uses STL TNL::Vectors to represent a dynamic array of vertices.  **/
/** This code snippet was submitted to FlipCode.com by          **/
/** John W. Ratcliff (jratcliff@verant.com) on July 22, 2000    **/
/** I did not write the original code/algorithm for this        **/
/** this triangulator, in fact, I can't even remember where I   **/
/** found it in the first place.  However, I did rework it into **/
/** the following black-box static class so you can make easy   **/
/** use of it in your own code.  Simply replace TNL::Vector2d with   **/
/** whatever your own TNL::Vector implementation might be.           **/
/*****************************************************************/


class Triangulate
{
public:
   // class for output data of the triangulate process methods; hopefully cleans itself up
   class TriangleData
   {
   public:
      TriangleData() {}
      ~TriangleData()
      {
         if(pointList) free(pointList);
         if(triangleList) free(triangleList);
      }
      TNL::F32* pointList;
      TNL::S32 pointCount;
      TNL::S32* triangleList;
      TNL::S32 triangleCount;
   };

   // Triangulate a contour/polygon, places results in  TNL::Vector as series of triangles
   static bool Process(const TNL::Vector<Point> &contour, TNL::Vector<Point> &result);

   // Triangulate a bounded area with complex polygon holes
   static bool processComplex(TriangleData& outputData, const Rect& bounds, const TNL::Vector<TNL::Vector<Point> >& polygonList, TNL::Vector<TNL::F32>& holeMarkerList);

   // Merge triangles into convex polygons
   static bool mergeTriangles(TriangleData& triangleData, rcPolyMesh& mesh, TNL::S32 maxVertices = 6);

   // Decide if point Px/Py is inside triangle defined by
   // (Ax,Ay) (Bx,By) (Cx,Cy)
   static bool InsideTriangle(float Ax, float Ay,
         float Bx, float By,
         float Cx, float Cy,
         float Px, float Py);

private:
   static bool Snip(const TNL::Vector<Point> &contour, int u, int v, int w, int n, int *V);

};


};
#endif // _GEOM_UTILS_

