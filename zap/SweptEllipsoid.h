//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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
// removed references to vector2 and replaced with Point
// added in prototypes for circle collisions

#ifndef _SWEPT_ELLIPSOID_H_
#define _SWEPT_ELLIPSOID_H_

#include "point.h"
#include <string>

using namespace TNL;

namespace Zap {

// Test between a polygon and a swept sphere with radius inRadius moving from inBegin to inBegin + inDelta
// If there is an intersection the intersection position is returned in outPoint and the center of the
// sphere is at inBegin + outFraction * inDelta when it collides
//bool PolygonSweptSphereIntersect(const Plane &inPlane, const Vector2 *inVertices, int inNumVertices, const Vector3 &inBegin, const Vector3 &inDelta, float inRadius, Vector3 &outPoint, float &outFraction);

// Test intersection with a swept ellipsoid with principal axis inAxis1, inAxis2, inAxis3 moving from inBegin to inBegin + inDelta
// If there is an intersection the intersection position is returned in outPoint and the center of the
// sphere is at inBegin + outFraction * inDelta when it collides
//bool PolygonSweptEllipsoidIntersect(const Plane &inPlane, const Vector2 *inVertices, int inNumVertices, const Vector3 &inBegin, const Vector3 &inDelta, const Vector3 &inAxis1, const Vector3 &inAxis2, const Vector3 &inAxis3, Vector3 &outPoint, float &outFraction);

bool PolygonSweptCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inBegin, const Point &inDelta, Point::member_type inRadius, Point &outPoint, Point::member_type &outFraction);
bool PolygonContains2(const Point *inVertices, int inNumVertices, const Point &inPoint);
bool segmentsColinear(Point p1, Point p2, Point p3, Point p4);
bool segsOverlap(Point p1, Point p2, Point p3, Point p4, Point &overlapStart, Point &overlapEnd);
bool pointOnSegment(Point c, Point a, Point b);


// Note that inlined functions seem to need to be defined in the header file, not just declared
inline std::string trim_right(const std::string &source, const std::string &t = " ")
{
   std::string str = source;
   return str.erase(str.find_last_not_of(t) + 1);
}

inline std::string trim_left(const std::string &source, const std::string &t = " ")
{
   std::string str = source;
   return str.erase(0, source.find_first_not_of(t));
}

inline std::string trim(const std::string &source, const std::string &t = " ")
{
   return trim_left(trim_right(source, t), t);
}


/*****************************************************************/
/** Static class to triangulate any contour/polygon efficiently **/
/** You should replace Vector2d with whatever your own Vector   **/
/** class might be.  Does not support polygons with holes.      **/
/** Uses STL vectors to represent a dynamic array of vertices.  **/
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

  // triangulate a contour/polygon, places results in  vector
  // as series of triangles.
   static bool Process(const Vector<Point> &contour, Vector<Point> &result);

  // compute area of a contour/polygon
  static float Area(const Vector<Point> &contour);

  // decide if point Px/Py is inside triangle defined by
  // (Ax,Ay) (Bx,By) (Cx,Cy)
  static bool InsideTriangle(float Ax, float Ay,
                             float Bx, float By,
                             float Cx, float Cy,
                             float Px, float Py);

private:
  static bool Snip(const Vector<Point> &contour, int u, int v, int w, int n, int *V);

};



};
#endif // _SWEPT_ELLIPSOID_H_
