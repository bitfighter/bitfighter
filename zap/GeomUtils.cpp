//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Note: This file has become a sort of catchall for various geometric routines gleaned from
// various sources.


// Example code for: Collision Detection with Swept Spheres and Ellipsoids
// See: http://www.three14.demon.nl/sweptellipsoid/SweptEllipsoid.pdf
//
// Copyright (C) 2003 Jorrit Rouwe, except for routines otherwise noted below
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


#include "GeomUtils.h"
#include "MathUtils.h"                    // For findLowestRootInInterval()
#include "LuaModule.h"
#include "LuaBase.h"


#include "../recast/Recast.h"
#include "../recast/RecastAlloc.h"
#include <clipper.hpp>
#include <poly2tri.h>

#include "tnlVector.h"
#include "tnlTypes.h"
#include "tnlLog.h"

#include <math.h>
#include <deque>

using namespace TNL;
using namespace ClipperLib;


#ifdef TNL_OS_WIN32
#  define isnanf _isnanf
#endif

namespace Zap
{

using namespace LuaArgs;

Vector<Point> createPolygon(const Point &center, F32 radius, U32 sideCount, F32 angle)
{
   Vector<Point> outputPoly(sideCount);

   calcPolygonVerts(center, sideCount, radius, angle, outputPoly);
   return outputPoly;
}


void calcPolygonVerts(const Point &center, S32 sides, F32 radius, F32 angle, Vector<Point> &points)
{
   points.reserve(sides);

   F32 theta = 0;
   F32 dTheta = FloatTau / sides;

   for(S32 i = 0; i < sides; i++)
   {
      F32 x = center.x + cos(theta + angle) * radius;
      F32 y = center.y + sin(theta + angle) * radius;

      points.push_back(Point(x, y));
      theta += dTheta;
   }
}


static inline F64 isLeft(const Point &p1, const Point &p2, const Point &p )
{
    return ( (F64(p2.x) - p1.x) * (F64(p.y) - p1.y) - (F64(p.x) -  p1.x) * (F64(p2.y) - p1.y) );
}

// Fast winding number test for finding if a point is in a polygon.  Adapted from:
// http://geomalgorithms.com/a03-_inclusion.html#wn_PnPoly%28%29
bool polygonContainsPoint(const Point *vertices, S32 vertexCount, const Point &point)
{
   if (vertexCount < 3)
      return false;

   S32 counter = 0;    // Winding number counter

   // loop through all edges of the polygon
   S32 nextIndex;
   for (S32 i = 0; i < vertexCount; i++)
   {
      nextIndex = (i+1)%vertexCount;
      if (vertices[i].y <= point.y)
      {
         if (vertices[nextIndex].y  > point.y)                        // an upward crossing
            if (isLeft(vertices[i], vertices[nextIndex], point) > 0)  // point left of edge
               ++counter;                                             // have a valid up intersect
      }
      else
      {
         if (vertices[nextIndex].y  <= point.y)                       // a downward crossing
            if (isLeft(vertices[i], vertices[nextIndex], point) < 0)  // point right of edge
               --counter;                                             // have  a valid down intersect
      }
   }

   return counter != 0;   // Point is outside polygon only when counter is 0
}


// Fast winding number test for finding if a point is in a polygon.  Adapted from:
// http://geomalgorithms.com/a03-_inclusion.html#wn_PnPoly%28%29
static inline F64 isLeftP2t( p2t::Point *p1, p2t::Point *p2, const p2t::Point *p3 )
{
    return ( (p2->x - p1->x) * (p3->y - p1->y) - (p3->x -  p1->x) * (p2->y - p1->y) );
}

static bool PolygonContains2p2t(p2t::Point **vertices, int vertexCount, const p2t::Point *point)
{
   S32 counter = 0;    // Winding number counter

   // loop through all edges of the polygon
   S32 nextIndex;
   for (S32 i = 0; i < vertexCount; i++)
   {
      nextIndex = (i+1)%vertexCount;
      if (vertices[i]->y <= point->y)
      {
         if (vertices[nextIndex]->y  > point->y)                      // an upward crossing
            if (isLeftP2t(vertices[i], vertices[nextIndex], point) > 0)  // point left of edge
               ++counter;                                             // have a valid up intersect
      }
      else
      {
         if (vertices[nextIndex]->y  <= point->y)                     // a downward crossing
            if (isLeftP2t(vertices[i], vertices[nextIndex], point) < 0)  // point right of edge
               --counter;                                             // have  a valid down intersect
      }
   }

   return counter != 0;   // Point is outside polygon only when counter is 0
}


// Remove collinear points from list.  If this is a polygon, consider removing endpoints as well as midpoints.
void removeCollinearPoints(Vector<Point> &points, bool isPolygon)
{
   // Check for duplicate points
   for(S32 i = 1; i < (S32)points.size(); i++)
      if(points[i-1] == points[i])
      {
         points.erase(i);
         i--;
      }

   if(points.size() < 3)
      return;

   for(S32 i = 1; i < points.size() - 1; i++)
   {
      if((points[i] - points[i-1]).ATAN2() == (points[i+1] - points[i]).ATAN2())
      {
         points.erase(i);
         i--;
      }
   }

   if(isPolygon && points.size() >= 3)
   {
      // Handle wrap-around, where second-to-last, last, and first are collinear
      while(points.size() >= 3 && (points[points.size() - 2] - points[points.size() - 1]).ATAN2() == (points[points.size() - 1] - points[0]).ATAN2())
         points.erase(points.size() - 1);

      // Handle wrap-around, where last, first, and second are collinear
      while(points.size() >= 3 && (points[points.size() - 1] - points[0]).ATAN2() == (points[0] - points[1]).ATAN2())
         points.erase(0);
   }
}


//// From http://www.blackpawn.com/texts/pointinpoly/default.html
//// Messy looking! Quick!
// Formerly only used by triangulatedFillContains()
//static bool pointInTriangle(const Point &p, const Point &a, const Point &b, const Point &c)
//{
//   // Compute vectors
//   Point v0(c - a);
//   Point v1(b - a);
//   Point v2(p - a);

//   // Compute dot products
//   F32 dot00 = v0.dot(v0);
//   F32 dot01 = v0.dot(v1);
//   F32 dot02 = v0.dot(v2);
//   F32 dot11 = v1.dot(v1);
//   F32 dot12 = v1.dot(v2);

//   // Compute barycentric coordinates
//   F32 invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
//   F32 u = (dot11 * dot02 - dot01 * dot12) * invDenom;
//   F32 v = (dot00 * dot12 - dot01 * dot02) * invDenom;

//   // Check if point is in triangle
//   return (u >= 0) && (v >= 0) && (u + v < 1);
//}


// Use F64 for precision
static inline bool isLeft( const Point &p1, const Point &p2, const Point &p3, F64 &result )
{
    result = (F64(p2.x) - p1.x) * (F64(p3.y) - p1.y) - (F64(p3.x) -  p1.x) * (F64(p2.y) - p1.y);
    return result > 0;
}

// Return 1, 0, or -1 for CCW, collinear, and CW respectively
static inline S32 isClockwiseTriangle(const Point &p1, const Point &p2, const Point &p3)
{
   F64 result;
   isLeft(p1, p2, p3, result);

   if(result > 0)
      return 1;
   if(result < 0)
      return -1;
   return 0;
}


// Determine if polygon is wound clockwise
bool isWoundClockwise(const Vector<Point>& inputPoly)
{
   if(inputPoly.size() < 2)
      return true;

   F64 finalSum = 0;
   S32 i_prev = inputPoly.size() - 1;

   for(S32 i = 0; i < inputPoly.size(); i++)
   {
      // (x2-x1)(y2+y1)
      finalSum += (F64(inputPoly[i].x) - inputPoly[i_prev].x) * (F64(inputPoly[i].y) + inputPoly[i_prev].y);
      i_prev = i;
   }

   // Negative result = counter-clockwise
   if(finalSum < 0)
      return false;

   return true;
}


bool isConvex(const Vector<Point> *verts)
{
   int n = verts->size();
   if(n < 3)
      return true;

   double sign = 0;
   for(int i = 0; i < n; i++)
   {
      const Point &p1 = verts->get(i);
      const Point &p2 = verts->get((i + 1) % n);
      const Point &p3 = verts->get((i + 2) % n);

      // Perform subtraction in F64 to maintain precision
      F64 v1x = F64(p2.x) - p1.x;
      F64 v1y = F64(p2.y) - p1.y;
      F64 v2x = F64(p3.x) - p2.x;
      F64 v2y = F64(p3.y) - p2.y;

      double det = v1x * v2y - v1y * v2x;
      if(det != 0)
      {
         if(sign == 0)
            sign = det;
         else if(sign * det < 0)
            return false;
      }
   }
   return true;
}


// If the sum of the radii is greater than the distance between the center points,
// then the circles intersect
bool circleCircleIntersect(const Point &center1, F32 radius1, const Point &center2, F32 radius2)
{
   // Remove square root for speed
   return (center1.distSquared(center2) <= sq(radius1 + radius2));
}


bool polygonCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inCenter, F32 inRadiusSq, Point &outPoint, Point *ignoreVelocityEpsilon)
{
   if (inNumVertices == 0)
      return false;

   // Check if the center is inside the polygon  ==> now works for all polys
   if(polygonContainsPoint(inVertices, inNumVertices, inCenter))
   {
      outPoint = inCenter;
      return true;
   }

   // Loop through edges
   bool collision = false;
   for (const Point *v1 = inVertices, *v2 = inVertices + inNumVertices - 1; v1 < inVertices + inNumVertices; v2 = v1, ++v1)
   {
      // Get fraction where the closest point to this edge occurs
      Point v1_v2 = *v2 - *v1;
      Point v1_center = inCenter - *v1;
      F32 fraction = v1_center.dot(v1_v2);
      if (fraction < 0.0f)
      {
         // Closest point is v1
         F32 dist_sq = v1_center.lenSquared();
         if (dist_sq <= inRadiusSq)
            if(!ignoreVelocityEpsilon || ignoreVelocityEpsilon->dot((*v1) - inCenter) > 0)
            {
               collision = true;
               outPoint = *v1;
               inRadiusSq = dist_sq;
            }
      }
      else
      {
         F32 v1_v2_len_sq = v1_v2.lenSquared();
         if (fraction <= v1_v2_len_sq)
         {
            // Closest point is on line segment
            Point point = *v1 + v1_v2 * (fraction / v1_v2_len_sq);
            F32 dist_sq = (point - inCenter).lenSquared();
            if (dist_sq <= inRadiusSq)
               if(!ignoreVelocityEpsilon || ignoreVelocityEpsilon->dot(point - inCenter) > 0)
               {
                  collision = true;
                  outPoint = point;
                  inRadiusSq = dist_sq;
               }
         }
      }
   }

   return collision;
}


bool polygonIntersectsSegment(const Vector<Point> &p, const Point &start, const Point &end)
{
   if (p.empty())
      return false;

   F32 ct;
   const Point *v1 = &p[p.size() - 1];

   for(S32 i = 0; i < p.size(); i++)
   {
      const Point *v2 = &p[i];
      if(segmentsIntersect(*v1, *v2, start, end, ct))
         return true;
      v1 = v2;
   }

   return false;
}


bool polygonsIntersect(const Vector<Point> &p1, const Vector<Point> &p2)
{
   if (p1.empty() || p2.empty())
      return false;

   F32 ct;
   const Point *rp1 = &p1[p1.size() - 1];

   for(S32 i = 0; i < p1.size(); i++)
   {
      const Point *rp2 = &p1[i];

      const Point *cp1 = &p2[p2.size() - 1];

      for(S32 j = 0; j < p2.size(); j++)
      {
         const Point *cp2 = &p2[j];
         if(segmentsIntersect(*rp1, *rp2, *cp1, *cp2, ct))
            return true;
         cp1 = cp2;
      }
      rp1 = rp2;
   }
   //  All points of polygon is inside the other polygon?  At this point, if any are, all are.
   return polygonContainsPoint(p1.address(), p1.size(), p2[0]) || polygonContainsPoint(p2.address(), p2.size(), p1[0]);
}


// Check to see if segment start-end intersects poly
// Assumes a polygon in format A-B-C-D if format is true, A-B, C-D, E-F if format is false
bool polygonIntersectsSegmentDetailed(const Point *poly, U32 vertexCount, bool format, const Point &start, const Point &end,
                                      F32 &collisionTime, Point &normal)
{
   if (vertexCount == 0)
      return false;

   Point v1 = poly[vertexCount - 1];
   Point v2, dv;
   Point dp = end - start;

   S32 inc = format ? 1 : 2;

   F64 currentCollisionTime = 100;

   for(U32 i = 0; i < vertexCount - (inc - 1); i += inc)    // Count by 1s when format is true, 2 when false
   {
      if(format)     // A-B-C-D format ==> examine every contiguous pair of vertices
         v2.set(poly[i]);
      else           // A-B C-D format ==> don't examine segment B-C
      {
         v1.set(poly[i]);
         v2.set(poly[i + 1]);
      }

      // edge from v1 -> v2
      // ray from start -> end

      dv.set(v2 - v1);

      F64 dpx = dp.x; F64 dpy = dp.y;
      F64 dvx = dv.x; F64 dvy = dv.y;
      F64 sx  = start.x; F64 sy = start.y;
      F64 v1x = v1.x; F64 v1y = v1.y;

      F64 denom = dpy * dvx - dpx * dvy;
      if(denom != 0) // otherwise, the lines are parallel
      {
         F64 s = ( (sx - v1x) * dvy + (v1y - sy) * dvx ) / denom;
         F64 t = ( (sx - v1x) * dpy + (v1y - sy) * dpx ) / denom;

         if(s >= 0 && s <= 1 && t >= 0 && t <= 1 && s < currentCollisionTime)    // Found collision closer than others
         {
            normal.set(dv.y, -dv.x);
            currentCollisionTime = s;
         }
      }
      v1.set(v2);    // No real effect if format == false
   }

   if(currentCollisionTime <= 1)    // Found intersection
   {
      collisionTime = (F32)currentCollisionTime;
      return true;
   }

   // No intersection
   return false;
}

bool circleIntersectsSegment(Point center, float radius, Point start, Point end, float &collisionTime)
{
   // if the point is in the circle, it's a collision at the start
   Point d = center - start;
   Point v = end - start;

   if(d.lenSquared() <= sq(radius))
   {
      collisionTime = 0;
      return true;
   }

   // otherwise, solve the following equation for t
   // (d - vt)^2 = radius^2

   float a = v.dot(v);
   float b = -2 * d.dot(v);
   float c = d.dot(d) - radius * radius;

   return findLowestRootInInterval(a, b, c, 1, collisionTime);
}


// Do segments sit on same virtual line?
static bool segmentsColinear(const Point &p1, const Point &p2, const Point &p3, const Point &p4)
{
   const float smallNumber = (float) 0.0000001;

   float denom = ((p4.y - p3.y) * (p2.x - p1.x)) - ((p4.x - p3.x) * (p2.y - p1.y));
   float numerator = ((p4.x - p3.x) * (p1.y - p3.y)) - ((p4.y - p3.y) * (p1.x - p3.x));
   float numerator2 = ((p2.x - p1.x) * (p1.y - p3.y)) - ((p2.y - p1.y) * (p1.x - p3.x));

   if(fabs(denom) < smallNumber && fabs(numerator) < smallNumber && fabs(numerator2) < smallNumber)
    return true;    // Coincident

   return false;    // Not
}



// Based on http://www.codeguru.com/forum/showthread.php?t=194400
// Does point c sit on segment a-b?
// Optimized for speed, as we can do what we need with fewer computations -CE

bool pointOnSegment(const Point &c, const Point &a, const Point &b, F32 closeEnough)
{
   static Point closest;

   return c.distSquared(a) < closeEnough || c.distSquared(b) < closeEnough ||
         (findNormalPoint(c, a, b, closest) && c.distSquared(closest) < closeEnough);
}

//
///*
//Subject 1.02: How do I find the distance from a point to a line?
//
//    Let the point be C (Cx,Cy) and the line be AB (Ax,Ay) to (Bx,By).
//    Let P be the point of perpendicular projection of C on AB.  The parameter
//    r, which indicates P's position along AB, is computed by the dot product
//    of AC and AB divided by the square of the length of AB:
//
//    (1)     AC dot AB
//        r = ---------
//            ||AB||^2
//
//    r has the following meaning:
//
//        r=0      P = A
//        r=1      P = B
//        r<0      P is on the backward extension of AB
//        r>1      P is on the forward extension of AB
//        0<r<1    P is interior to AB
//
//    The length of a line segment in d dimensions, AB is computed by:
//
//        L = sqrt( (Bx-Ax)^2 + (By-Ay)^2 + ... + (Bd-Ad)^2)
//
//    so in 2D:
//
//        L = sqrt( (Bx-Ax)^2 + (By-Ay)^2 )
//
//    and the dot product of two vectors in d dimensions, U dot V is computed:
//
//        D = (Ux * Vx) + (Uy * Vy) + ... + (Ud * Vd)
//
//    so in 2D:
//
//        D = (Ux * Vx) + (Uy * Vy)
//
//    So (1) expands to:
//
//            (Cx-Ax)(Bx-Ax) + (Cy-Ay)(By-Ay)
//        r = -------------------------------
//                          L^2
//
//    The point P can then be found:
//
//        Px = Ax + r(Bx-Ax)
//        Py = Ay + r(By-Ay)
//
//    And the distance from A to P = r*L.
//
//    Use another parameter s to indicate the location along PC, with the
//    following meaning:
//           s<0      C is left of AB
//           s>0      C is right of AB
//           s=0      C is on AB
//
//    Compute s as follows:
//
//            (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
//        s = -----------------------------
//                        L^2
//
//    Then the distance from C to P = |s|*L.
//*/
//   F32 r_numerator   = (c.x - a.x) * (b.x - a.x) + (c.y - a.y) * (b.y - a.y);
//   F32 r_denomenator = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
//   F32 r = r_numerator / r_denomenator;
//
//   F32 s = F32((a.y - c.y) * (b.x - a.x) - (a.x - c.x) * (b.y - a.y));
//
//   if ((r >= 0) && (r <= 1))
//   {
//      return(fabs(s) < .0001);
//   }
//   else
//      return false;
//}


// See if segment p1-p2 overlaps p3-p4
// Coincident endpoints alone do not count!
// Pass back the overpping extent in two points
static bool segsOverlap(const Point &p1, const Point &p2, const Point &p3, const Point &p4, Point &overlapStart, Point &overlapEnd, F32 scaleFact)
{
   const Point* pInt = NULL;
   bool found = false;

   const F32 closeEnough = 1.0f * 1.0f * scaleFact * scaleFact;

   if(pointOnSegment(p1, p3, p4, closeEnough))
   {
      pInt = &p1;
      found = true;
   }

   if(pointOnSegment(p2, p3, p4, closeEnough))
   {
      if(found && *pInt != p2)
      {
         overlapStart.set(*pInt);
         overlapEnd.set(p2);
         return true;
      }

      pInt = &p2;
      found = true;
   }

   if(p1.distSquared(p3) > closeEnough && p2.distSquared(p3) > closeEnough && pointOnSegment(p3, p1, p2, closeEnough))
   {
      if(found && *pInt != p3)
      {
         overlapStart.set(*pInt);
         overlapEnd.set(p3);
         return true;
      }

      pInt = &p3;
      found = true;
   }

   if(p1.distSquared(p4) > closeEnough && p2.distSquared(p4) > closeEnough && pointOnSegment(p4, p1, p2, closeEnough))
   {
      if(found && *pInt != p4)
      {
         overlapStart.set(*pInt);
         overlapEnd.set(p4);
         return true;
      }
   }

   return false;
}


// Determine if an object at inBegin is colliding with any segments of polygon inVertices.  Calculates when the
// collision occurred (outCollisionTime) and where (outPoint).
bool SweptCircleEdgeVertexIntersect(const Point *inVertices, int inNumVertices, const Point &inBegin, const Point &inEnd, float inRadius, float &outCollisionTime, Point &outPoint)
{
   if (inNumVertices == 0)
      return false;

   bool collision = false;
   float min_t = outCollisionTime;

   Point v = inEnd - inBegin;

   // Edge intersection
   for (const Point *v1 = inVertices, *v2 = inVertices + inNumVertices - 1; v1 < inVertices + inNumVertices; v2 = v1, ++v1)
   {
      Point edge = *v2 - *v1;
      float edge_len_sq = edge.lenSquared();
      Point begin_v1 = inBegin - *v1;

      // The distance from a point P to a line v1 + edge * t is:
      // |(v1 - P) x edge| / |edge|
      // We want to find t such that the distance is inRadius:
      // |(v1 - (inBegin + v * t)) x edge|^2 = inRadius^2 * |edge|^2
      // |(v1 - inBegin) x edge - t * (v x edge)|^2 = inRadius^2 * |edge|^2
      // Let a = begin_v1 x edge, b = v x edge
      // (a - t * b)^2 = inRadius^2 * edge_len_sq
      // This is a quadratic equation in t:
      // t^2 * b^2 - 2 * t * a * b + a^2 - inRadius^2 * edge_len_sq = 0

      float a = begin_v1.determinant(edge);
      float b = v.determinant(edge);

      float t;
      if (findLowestRootInInterval(b * b, -2.0f * a * b, a * a - inRadius * inRadius * edge_len_sq, min_t, t))
      {
         // Check if the intersection point is on the edge
         Point point = inBegin + v * t;
         float fraction = (point - *v1).dot(edge);
         if (fraction >= 0.0f && fraction <= edge_len_sq)
         {
            min_t = t;
            outPoint = *v1 + edge * (fraction / edge_len_sq);
            collision = true;
         }
      }
   }

   // Vertex intersection
   for (const Point *v1 = inVertices; v1 < inVertices + inNumVertices; ++v1)
   {
      float t;
      if (circleIntersectsSegment(*v1, inRadius, inBegin, inEnd, t))
      {
         if (t < min_t)
         {
            min_t = t;
            outPoint = *v1;
            collision = true;
         }
      }
   }

   if (collision)
      outCollisionTime = min_t;

   return collision;
}


F32 area(const Vector<Point> &contour)
{
  int n = contour.size();
  if (n < 3)
     return 0.0f;

  F64 A = 0.0;

  for(int p = n-1, q = 0; q < n; p = q++)
    A += F64(contour[p].x) * contour[q].y - F64(contour[q].x) * contour[p].y;

  return (F32)(A * 0.5);
}

   /*
     InsideTriangle decides if a point P is Inside of the triangle
     defined by A, B, C.
   */
bool Triangulate::InsideTriangle(float Ax, float Ay,
                                 float Bx, float By,
                                 float Cx, float Cy,
                                 float Px, float Py)

{
  float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
  float cCROSSap, bCROSScp, aCROSSbp;

  ax = Cx - Bx;  ay = Cy - By;
  bx = Ax - Cx;  by = Ay - Cy;
  cx = Bx - Ax;  cy = By - Ay;
  apx= Px - Ax;  apy= Py - Ay;
  bpx= Px - Bx;  bpy= Py - By;
  cpx= Px - Cx;  cpy= Py - Cy;

  aCROSSbp = ax*bpy - ay*bpx;
  cCROSSap = cx*apy - cy*apx;
  bCROSScp = bx*cpy - by*cpx;

  return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
};

bool Triangulate::Snip(const Vector<Point> &contour, int u, int v, int w, int n, int *V)
{
  int p;
  float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

  Ax = contour[V[u]].x;
  Ay = contour[V[u]].y;

  Bx = contour[V[v]].x;
  By = contour[V[v]].y;

  Cx = contour[V[w]].x;
  Cy = contour[V[w]].y;

  if ( EPSILON > (((Bx-Ax)*(Cy-Ay)) - ((By-Ay)*(Cx-Ax))) ) return false;

  for (p=0;p<n;p++)
  {
    if( (p == u) || (p == v) || (p == w) ) continue;
    Px = contour[V[p]].x;
    Py = contour[V[p]].y;
    if (InsideTriangle(Ax,Ay,Bx,By,Cx,Cy,Px,Py)) return false;
  }

  return true;
}

// Takes points in contour, triangulates and put the results in result
bool Triangulate::Process(const Vector<Point> &contour, Vector<Point> &result)
{
   result.clear();
   /* allocate and initialize list of Vertices in polygon */

   int n = contour.size();
   if(n < 3)
      return false;

   int *V = new int[n];

   /* we want a counter-clockwise polygon in V */

   if(area(contour) > 0)
      for (int v=0; v < n; v++)
         V[v] = v;
   else
      for(int v = 0; v < n; v++)
         V[v] = (n-1)-v;

   int nv = n;

   /*  Remove nv-2 Vertices, creating 1 triangle every time */
   int count = 2*nv;   /* error detection */

   for(int m = 0, v = nv - 1; nv > 2; )
   {
      /* If we loop, it is probably a non-simple polygon */
      if (0 >= (count--))
      {
         //** Triangulate: ERROR - probable bad polygon!
         delete[] V;
         return false;
      }

      /* Three consecutive vertices in current polygon, <u,v,w> */
      int u = v;
      if (nv <= u)
         u = 0;     // previous

      v = u + 1;
      if(nv <= v)
         v = 0;     // new v

      int w = v+1;
      if(nv <= w)
         w = 0;     // next

      if( Snip(contour,u,v,w,nv,V) )
      {
         int a,b,c,s,t;

         /* true names of the vertices */
         a = V[u]; b = V[v]; c = V[w];

         /* output Triangle */
         result.push_back( contour[a] );
         result.push_back( contour[b] );
         result.push_back( contour[c] );

         m++;

         /* remove v from remaining polygon */
         for(s = v, t = v+1; t < nv; s++, t++)
            V[s] = V[t];

         nv--;

         /* reset error detection counter */
         count = 2*nv;
      }
   }

   delete[] V;

   return true;
}


static const F32 CLIPPER_SCALE_FACT = 1000.0f;


// Check to see if polygon is complex (i.e. self-intersecting)
bool polygonIsComplex(const Vector<Point> &contour)
{
   Clipper c;

   Path p;
   p.reserve(contour.size());
   for(S32 i = 0; i < contour.size(); i++)
      p.push_back(IntPoint(cround(contour[i].x * CLIPPER_SCALE_FACT), cround(contour[i].y * CLIPPER_SCALE_FACT)));

   c.AddPath(p, ptSubject, true);

   Paths solution;
   c.Execute(ctUnion, solution, pftEvenOdd, pftEvenOdd);

   if(solution.size() != 1)      // Self intersection split it into multiple parts
      return true;

   if(solution[0].size() != contour.size())    // Self intersection resulted in fewer or more vertices
      return true;

   return false;
}


// Given a list of (possibly self-intersecting) polygons, return a list of non-self-intersecting polygons
void splitSelfIntersectingPolys(const Vector<Vector<Point> > &polys, Vector<Vector<Point> > &result)
{
   Clipper c;

   for(S32 i = 0; i < polys.size(); i++)
   {
      Path p;
      p.reserve(polys[i].size());

      for(S32 j = 0; j < polys[i].size(); j++)
         p.push_back(IntPoint(cround(polys[i][j].x * CLIPPER_SCALE_FACT), cround(polys[i][j].y * CLIPPER_SCALE_FACT)));

      c.AddPath(p, ptSubject, true);
   }

   Paths solution;
   c.Execute(ctUnion, solution, pftEvenOdd, pftEvenOdd);

   result.reserve(solution.size());

   for(S32 i = 0; i < (S32)solution.size(); i++)
   {
      Vector<Point> poly;
      poly.reserve(solution[i].size());

      for(S32 j = 0; j < (S32)solution[i].size(); j++)
         poly.push_back(Point(F32(solution[i][j].X) / CLIPPER_SCALE_FACT, F32(solution[i][j].Y) / CLIPPER_SCALE_FACT));

      result.push_back(poly);
   }
}


// Given a polygon that may be self-intersecting, return a set of triangles that fills it
void triangulateComplexPoly(const Vector<Point> &poly, Vector<Point> &triangles)
{
   Vector<Vector<Point> > polys;
   polys.push_back(poly);

   Vector<Vector<Point> > splitPolys;
   splitSelfIntersectingPolys(polys, splitPolys);

   for(S32 i = 0; i < splitPolys.size(); i++)
   {
      Vector<Point> result;
      Triangulate::Process(splitPolys[i], result);
      for(S32 j = 0; j < result.size(); j++)
         triangles.push_back(result[j]);
   }
}


// Is point in a possibly self-intersecting poly?
bool triangulatedFillContains(const Vector<Point> &triangles, const Point &point)
{
   // Standard winding-number/ray casting tests will fail with self-intersecting polys, but we can
   // just test if it's in any of our triangles.
   for(S32 i = 0; i < triangles.size(); i += 3)
      if(polygonContainsPoint(&triangles[i], 3, point))
         return true;

   return false;
}


// Compute the area of a set of triangles
F32 areaOfTriangles(const Vector<Point> &triangles)
{
   F32 totalArea = 0;
   for(S32 i = 0; i < triangles.size(); i += 3)
   {
      Vector<Point> tri;
      tri.push_back(triangles[i]);
      tri.push_back(triangles[i + 1]);
      tri.push_back(triangles[i + 2]);
      totalArea += fabs(area(tri));
   }
   return totalArea;
}


void cornersToEdges(const Vector<Point> &corners, Vector<Point> &edges)
{
   edges.clear();
   if(corners.empty())
      return;

   edges.reserve(corners.size() * 2);

   for(S32 i = 0; i < corners.size(); i++)
   {
      edges.push_back(corners[i]);
      edges.push_back(corners[(i + 1) % corners.size()]);
   }
}


// Take a poly represented as a list of points (A-B-C-D) and convert to a list of segments
// (A-B, B-C, C-D, D-A)
void barrierLineToSegmentData(const Vector<Point> &points, Vector<Vector<Point> > &outData)
{
   outData.clear();
   if(points.empty())
      return;

   for(S32 i = 0; i < points.size(); i++)
   {
      Vector<Point> segment;
      segment.push_back(points[i]);
      segment.push_back(points[(i + 1) % points.size()]);
      outData.push_back(segment);
   }
}


// Given a path (A-B-C-D), find its centroid.
// Based on http://en.wikipedia.org/wiki/Centroid#Centroid_of_polygon
Point findCentroid(const Vector<Point> &poly)
{
   if(poly.size() == 0)
      return Point(0, 0);

   if(poly.size() == 1)
      return poly[0];

   if(poly.size() == 2)
      return (poly[0] + poly[1]) * 0.5f;

   // Handle larger polygons
   F64 x = 0;
   F64 y = 0;

   F64 sArea = 0;  // Signed area
   F64 area = 0;   // Partial signed area

   for (S32 i = 0; i < poly.size(); i++)
   {
      const Point &p1 = poly[i];
      const Point &p2 = poly[(i + 1) % poly.size()];

      area = (F64(p1.x) * p2.y - F64(p2.x) * p1.y);
      sArea += area;

      x += (F64(p1.x) + p2.x) * area;
      y += (F64(p1.y) + p2.y) * area;
   }

   // Zero area means it's likely a complex polygon or something with all points
   // on a line.  In this case, just return the average of all points.
   if(sArea == 0)
   {
      Point p(0, 0);
      for(S32 i = 0; i < poly.size(); i++)
         p += poly[i];
      return p / (F32)poly.size();
   }

   sArea *= 3.0;  // 0.5 * 6  (from area6)

   return Point(x / sArea, y / sArea);
}


// Get a point that is definitely inside the polygon.  For convex polys, we can use the centroid.
// For concave polys, the centroid might be outside.  In that case, we find a point that is inside.
Point findPointInPoly(const Vector<Point> &poly)
{
   Point centroid = findCentroid(poly);

   if(polygonContainsPoint(poly.address(), poly.size(), centroid))
      return centroid;

   // If centroid is not in poly, find a point that is.  We'll use the midpoint of a triangle
   // formed by the first vertex and its two neighbors.
   Vector<Point> triangles;
   Triangulate::Process(poly, triangles);

   if(triangles.size() >= 3)
      return (triangles[0] + triangles[1] + triangles[2]) / 3.0f;

   return centroid;     // Should never get here for valid polys
}



bool segmentsIntersect(const Point &p1, const Point &p2, const Point &p3, const Point &p4, F32 &collisionTime)
{
    F64 p1x = p1.x; F64 p1y = p1.y;
    F64 p2x = p2.x; F64 p2y = p2.y;
    F64 p3x = p3.x; F64 p3y = p3.y;
    F64 p4x = p4.x; F64 p4y = p4.y;

    F64 denom = (p4y - p3y) * (p2x - p1x) - (p4x - p3x) * (p2y - p1y);

    F64 numerator1 = (p4x - p3x) * (p1y - p3y) - (p4y - p3y) * (p1x - p3x);
    F64 numerator2 = (p2x - p1x) * (p1y - p3y) - (p2y - p1y) * (p1x - p3x);

    if ( denom == 0.0 )
    {
       //if ( numerator1 == 0.0 && numerator2 == 0.0 )
       //   return false;  //COINCIDENT;
       return false;  // PARALLEL;
    }

    F64 ua = numerator1 / denom;
    F64 ub = numerator2 / denom;

    if (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0)
    {
        collisionTime = (F32)ua;
        return true;
    }

    return false;
    // Point intersection(p1.x + collisionTime * (p2.x - p1.x), p1.y + collisionTime * (p2.y - p1.y));
}


bool findIntersection(const Point &p1, const Point &p2, const Point &p3, const Point &p4, Point &intersection)
{
    F64 p1x = p1.x; F64 p1y = p1.y;
    F64 p2x = p2.x; F64 p2y = p2.y;
    F64 p3x = p3.x; F64 p3y = p3.y;
    F64 p4x = p4.x; F64 p4y = p4.y;

    F64 denom = (p4y - p3y) * (p2x - p1x) - (p4x - p3x) * (p2y - p1y);
    F64 numerator = (p4x - p3x) * (p1y - p3y) - (p4y - p3y) * (p1x - p3x);

    F64 numerator2 = (p2x - p1x) * (p1y - p3y) - (p2y - p1y) * (p1x - p3x);

    if ( denom == 0.0 )
    {
       //if ( numerator == 0.0 && numerator2 == 0.0 )
       //   return false;  //COINCIDENT;
       return false;  // PARALLEL;
    }

    F64 ua = numerator / denom;
    F64 ub = numerator2/ denom;

    if (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0)
    {
      intersection.set(p1x + ua * (p2x - p1x), p1y + ua * (p2y - p1y));
      return true;
    }

    return false;
}

};
