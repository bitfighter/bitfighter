//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer Vector graphics space game
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
// removed references to Vector2 and replaced with Point
// added in prototypes for circle collisions

#ifndef _GEOM_UTILS_
#define _GEOM_UTILS_

#include <string>
#include <vector>

#include "tnlTypes.h"

// forward declarations
namespace ClipperLib {
   struct IntPoint;
   typedef std::vector<IntPoint> Polygon;
   typedef std::vector<Polygon> Polygons;
   class PolyTree;
};
struct rcPolyMesh;


using namespace TNL;
using namespace ClipperLib;

namespace Zap {

class Point;
class Rect;

// Test between a polygon and a swept sphere with radius inRadius moving from inBegin to inBegin + inDelta
// If there is an intersection the intersection position is returned in outPoint and the center of the
// sphere is at inBegin + outFraction * inDelta when it collides
//bool PolygonSweptSphereIntersect(const Plane &inPlane, const Vector2 *inVertices, int inNumVertices, const Vector3 &inBegin, const Vector3 &inDelta, float inRadius, Vector3 &outPoint, float &outFraction);

// Test intersection with a swept ellipsoid with principal axis inAxis1, inAxis2, inAxis3 moving from inBegin to inBegin + inDelta
// If there is an intersection the intersection position is returned in outPoint and the center of the
// sphere is at inBegin + outFraction * inDelta when it collides
//bool PolygonSweptEllipsoidIntersect(const Plane &inPlane, const Vector2 *inVertices, int inNumVertices, const Vector3 &inBegin, const Vector3 &inDelta, const Vector3 &inAxis1, const Vector3 &inAxis2, const Vector3 &inAxis3, Vector3 &outPoint, float &outFraction);

bool PolygonSweptCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inBegin, const Point &inDelta, F32 inRadius, Point &outPoint, F32 &outFraction);
bool polygonContainsPoint(const Point *vertices, S32 vertexCount, const Point &point);
bool segmentsColinear(const Point &p1, const Point &p2, const Point &p3, const Point &p4, F32 scaleFact);
bool segsOverlap(const Point &p1, const Point &p2, const Point &p3, const Point &p4, Point &overlapStart, Point &overlapEnd);
bool zonesTouch(const Vector<Point> *zone1, const Vector<Point> *zone2, F32 scaleFact, Point &overlapStart, Point &overlapEnd);
bool pointOnSegment(const Point &c, const Point &a, const Point &b, F32 closeEnough);
bool polygonCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inCenter, F32 inRadiusSq, Point &outPoint, Point *ignoreVelocityEpsilon = NULL);
bool circleCircleIntersect(const Point &center1, F32 radius1, const Point &center2, F32 radius2);

bool polygonsIntersect(const Vector<Point> &p1, const Vector<Point> &p2);
bool polygonIntersectsSegment(const Vector<Point> &points, const Point &start, const Point &end);  // This is four times faster than the Detailed one.
bool polygonIntersectsSegmentDetailed(const Point *poly, U32 vertexCount, bool format, const Point &start, const Point &end, float &collisionTime, Point &normal);
bool circleIntersectsSegment(Point center, F32 radius, Point start, Point end, float &collisionTime);

Point findCentroid(const Vector<Point> &polyPoints);
F32 area(const Vector<Point> &polyPoints);
F32 angleOfLongestSide(const Vector<Point> &polyPoints);

// These functions returns true if it found an appropriate point, false if not
bool findNormalPoint(const Point &p, const Point &s1, const Point &s2, Point &closest);
bool segmentsIntersect(const Point &p1, const Point &p2, const Point &p3, const Point &p4, F32 &collisionTime);
bool findIntersection(const Point &p1, const Point &p2, const Point &p3, const Point &p4, Point &intersection);

// This will return a new end point of a segment that you wish to shorten
Point shortenSegment(const Point &startPoint, const Point &endPoint, F32 lengthReduction);

void removeCollinearPoints(Vector<Point> &points, bool isPolygon);

Vector<Point> createPolygon(const Point &center, F32 radius, U32 sideCount, F32 angle = 0);
void calcPolygonVerts(const Point &pos, S32 sides, F32 radius, F32 angle, Vector<Point> &points);


// Returns index of points Vector closest to point
S32 findClosestPoint(const Point &point, const Vector<Point> &points);

// Offset a complex polygon by a given amount
void offsetPolygon(const Vector<Point> *inputPoly, Vector<Point> &outputPoly, const F32 offset);
void offsetPolygons(Vector<const Vector<Point> *> &inputPolys, Vector<Vector<Point> > &outputPolys, const F32 offset);

// Convert a list of floats into a list of points, removing all collinear points
Vector<Point> floatsToPoints(const Vector<F32> floats);

// Use Clipper to merge inputPolygons, placing the result in solution
bool mergePolys(const Vector<const Vector<Point> *> &inputPolygons, Vector<Vector<Point> > &outputPolygons);
bool mergePolysToPolyTree(const Vector<const Vector<Point> *> &inputPolygons, PolyTree &solution);

// Convert a Polygons to a list of points in a-b b-c c-d d-a format
void unpackPolygons(const Vector<Vector<Point> > &solution, Vector<Point> &lineSegmentPoints);

// test if a complex polygon has clockwise point winding order
bool isWoundClockwise(const Vector<Point>& inputPoly);


// Return true out if point is in polygon given a triangulated fill
bool triangulatedFillContains(const Vector<Point> *triangulatedFillPoints, const Point &point);
bool isConvex(const Vector<Point> *verts);

// scale Geometric points for clipper
Polygons upscaleClipperPoints(const Vector<const Vector<Point> *> &inputPolygons);
Vector<Vector<Point> > downscaleClipperPoints(const Polygons &inputPolygons);

/*****************************************************************/
/** Static class to triangulate any contour/polygon efficiently **/
/** You should replace Vector2d with whatever your own Vector   **/
/** class might be.  Does not support polygons with holes.      **/
/** Uses STL Vectors to represent a dynamic array of vertices.  **/
/** This code snippet was submitted to FlipCode.com by          **/
/** John W. Ratcliff (jratcliff@verant.com) on July 22, 2000    **/
/** I did not write the original code/algorithm for this        **/
/** this triangulator, in fact, I can't even remember where I   **/
/** found it in the first place.  However, I did rework it into **/
/** the following black-box static class so you can make easy   **/
/** use of it in your own code.  Simply replace Vector2d with   **/
/** whatever your own Vector implementation might be.           **/
/*****************************************************************/


class Triangulate
{
public:

   // Triangulate a contour/polygon, places results in  Vector as series of triangles
   static bool Process(const Vector<Point> &contour, Vector<Point> &result);

   // Triangulate a bounded area with complex polygon holes
   static bool processComplex(Vector<Point> &outputTriangles, const Rect& bounds, const PolyTree &polygonList);

   // Merge triangles into convex polygons
   static bool mergeTriangles(const Vector<Point> &triangleData, rcPolyMesh& mesh, S32 maxVertices = 6);

   // Decide if point Px/Py is inside triangle defined by
   // (Ax,Ay) (Bx,By) (Cx,Cy)
   static bool InsideTriangle(float Ax, float Ay,
         float Bx, float By,
         float Cx, float Cy,
         float Px, float Py);

private:
   static bool Snip(const Vector<Point> &contour, int u, int v, int w, int n, int *V);

};


////////////////////////////////////////
////////////////////////////////////////

// Wall related methods

void constructBarrierEndPoints(const Vector<Point> *vec, F32 width, Vector<Point> &barrierEnds);

// Takes a list of vertices and converts them into a list of lines representing the edges of an object
void cornersToEdges(const Vector<Point> &corners, Vector<Point> &edges);

// Simply takes a segment and "puffs it out" to a rectangle of a specified width, filling cornerPoints.  Does not modify endpoints.
void expandCenterlineToOutline(const Point &start, const Point &end, F32 width, Vector<Point> &cornerPoints);


};
#endif // _GEOM_UTILS_

