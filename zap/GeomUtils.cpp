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

#include "tnlVector.h"
#include "tnlTypes.h"
#include "tnlLog.h"

#include "GeomUtils.h"
#include "MathUtils.h"                    // For findLowestRootInInterval()
#include "Point.h"
#include "Rect.h"
#include "../recast/Recast.h"
#include "../recast/RecastAlloc.h"
#include "../clipper/clipper.hpp"
#include "../poly2tri/poly2tri.h"

#include <math.h>

using namespace TNL;
using namespace ClipperLib;

namespace Zap
{

// Check if a polygon contains inPoint, returns true when it does
// Assumes the polygon is a convex hull, and that the points are ordered in a certain direction
// (is that CW or CCW??)

// To use with a Vector of points, pass in vector.address() and vector.size()
/*
bool PolygonContains(const Point *inVertices, int inNumVertices, const Point &inPoint)
{
   // Loop through edges
   for (const Point *v1 = inVertices, *v2 = inVertices + inNumVertices - 1; v1 < inVertices + inNumVertices; v2 = v1, ++v1)
   {
      // If the point is outside this edge, the point is outside the polygon
      Point v1_v2 = *v2 - *v1;
      Point v1_point = inPoint - *v1;
      if (v1_v2.x * v1_point.y - v1_point.x * v1_v2.y > 0.0f)
         return false;
   }

   return true;
}
*/


Vector<Point> createPolygon(const Point &center, F32 radius, U32 sideCount, F32 angle)
{
   Vector<Point> outputPoly(sideCount);
   for(F32 theta = 0; theta < FloatTau; theta += FloatTau / sideCount)
      outputPoly.push_back(Point(center.x + cos(theta + angle) * radius, center.y + sin(theta + angle) * radius));

   return outputPoly;
}


// From http://local.wasp.uwa.edu.au/~pbourke/geometry/insidepoly/
// Should work on all polygons, convex and concave alike
// To use with a Vector of points, pass in vector.address() and vector.size()
bool PolygonContains2(const Point *inVertices, int inNumVertices, const Point &inPoint)
{
   bool oddCount = false;
   int i;
   double xinters;
   Point p1, p2;

   p1 = inVertices[inNumVertices-1];

   for(i = 0; i < inNumVertices; i++) 
   {
      p2 = inVertices[i];
      if(inPoint.y > MIN(p1.y, p2.y) && inPoint.y <= MAX(p1.y, p2.y) && inPoint.x <= MAX(p1.x, p2.x) && (p1.y != p2.y))
      {
         xinters = (inPoint.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y) + p1.x;
         if(p1.x == p2.x || inPoint.x <= xinters)
            oddCount = !oddCount;
      }

      p1 = p2;
   }

   return oddCount;   // True if we've crossed an odd number of lines
}


bool PolygonContains2p2t(p2t::Point **inVertices, int inNumVertices, const p2t::Point *inPoint)
{
   bool oddCount = false;
   int i;
   double xinters;
   const p2t::Point *p1, *p2;

   p1 = inVertices[inNumVertices-1];

   for(i = 0; i < inNumVertices; i++)
   {
      p2 = inVertices[i];
      if(inPoint->y > MIN(p1->y, p2->y) && inPoint->y <= MAX(p1->y, p2->y) && inPoint->x <= MAX(p1->x, p2->x) && (p1->y != p2->y))
      {
         xinters = (inPoint->y - p1->y) * (p2->x - p1->x) / (p2->y - p1->y) + p1->x;
         if(p1->x == p2->x || inPoint->x <= xinters)
            oddCount = !oddCount;
      }

      p1 = p2;
   }

   return oddCount;   // True if we've crossed an odd number of lines
}


// Remove collinear points from list.  If this is a polygon, consider removing endpoints as well as midpoints.
void removeCollinearPoints(Vector<Point> &points, bool isPolygon)
{
   // Check for duplicate points
   for(S32 i = 1; i < points.size() - 1; i++)
      if(points[i-1] == points[i])
      {
         points.erase(i);
         i--;
      }


   for(S32 i = 1; i < points.size() - 1; i++)
   {
      S32 j = i;
      while(i < points.size() - 1 && (points[j] - points[j-1]).ATAN2() == (points[i+1] - points[i]).ATAN2())
         points.erase(i);
   }

   if(isPolygon)
   {
      // Handle wrap-around, where second-to-last, last, and first are collinear
      while((points[points.size() - 2] - points[points.size() - 1]).ATAN2() == (points[points.size() - 1] - points[0]).ATAN2())
         points.erase(points.size() - 1);

      // Handle wrap-around, where last, first, and second are collinear
      while((points[points.size() - 1] - points[0]).ATAN2() == (points[0] - points[1]).ATAN2())
         points.erase(0);
   }
}


// From http://www.blackpawn.com/texts/pointinpoly/default.html
// Messy looking! Quick!
bool pointInTriangle(const Point &p, const Point &a, const Point &b, const Point &c)
{
   // Compute vectors
   Point v0(c - a);
   Point v1(b - a);
   Point v2(p - a);

   // Compute dot products
   float dot00 = v0.dot(v0);
   float dot01 = v0.dot(v1);
   float dot02 = v0.dot(v2);
   float dot11 = v1.dot(v1);
   float dot12 = v1.dot(v2);

   // Compute barycentric coordinates
   float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
   float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
   float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

   // Check if point is in triangle
   return (u > 0) && (v > 0) && (u + v < 1);
}


// Return true out if point is in polygon given a triangulated fill
bool triangulatedFillContains(const Vector<Point> *triangulatedFillPoints, const Point &point)
{
   for(S32 i = 0; i < triangulatedFillPoints->size(); i += 3)     // Using traingulated fill may be a little clumsy, but it should be fast!
      if(pointInTriangle(point, triangulatedFillPoints->get(i), triangulatedFillPoints->get(i + 1), triangulatedFillPoints->get(i + 2)))
         return true;

   return false;
}


//// Based on http://www.opengl.org/discussion_boards/ubbthreads.php?ubb=showflat&Number=248453
//// No idea if this is optimal or not, but it is only used in the editor, and works fine for our purposes.
//bool isConvex(const Vector<Point> *verts)
//{
//  Point v1, v2;
//  double det_value, cur_det_value;
//  int num_vertices = verts->size();
//  
//  if(num_vertices < 3)
//     return true;
//  
//  v1 = verts->get(0) - verts->get(num_vertices-1);
//  v2 = verts->get(1) - verts->get(0);
//  det_value = v1.determinant(v2);
//  
//  for(S32 i = 1 ; i < num_vertices-1 ; i++)
//  {
//    v1 = v2;
//    v2 = verts->get(i+1) - verts->get(i);
//    cur_det_value = v1.determinant(v2);
//    
//    if( (cur_det_value * det_value) < 0.0 )
//      return false;
//  }
//  
//  v1 = v2;
//  v2 = verts->get(0) - verts->get(num_vertices-1);
//  cur_det_value = v1.determinant(v2);
//  
//  return  (cur_det_value * det_value) >= 0.0;
//}


// If the sum of the radii is greater than the distance between the center points,
// then the circles intersect
bool circleCircleIntersect(const Point &center1, F32 radius1, const Point &center2, F32 radius2)
{
   // Remove square root for speed
   // (r1+r2)^2 > (x2-x1)^2 + (y2-y1)^2
   if(sq(radius1 + radius2) > center1.distSquared(center2))
      return true;

   return false;
}


// Check if circle at inCenter with radius^2 = inRadiusSq intersects with a polygon.
// Function returns true when it does and the intersection point is in outPoint
// Works only for convex hulls.. maybe no longer true... may work for all polys now
bool polygonCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inCenter, F32 inRadiusSq, Point &outPoint, Point *ignoreVelocityEpsilon)
{
   // Check if the center is inside the polygon  ==> now works for all polys
   if(PolygonContains2(inVertices, inNumVertices, inCenter))
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


// Returns true if polygon instersects or contains segment defined by start - end
bool polygonIntersectsSegment(const Vector<Point> &points, const Point &start, const Point &end)
{
   const Point *pointPrev = &points[points.size() - 1];
   F32 ct;

   for(S32 i = 0; i < points.size(); i++)
   {
      if(segmentsIntersect(start, end, *pointPrev, points[i], ct))
         return true;
      // else
      pointPrev = &points[i];
   }

   //  Entire line inside polygon?  If so, then the start will be within.
   return PolygonContains2(points.address(), points.size(), start);
}


// Returns true if polygons represented by p1 & p2 intersect or one contains the other
bool polygonsIntersect(const Vector<Point> &p1, const Vector<Point> &p2)
{
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
   return PolygonContains2(p1.address(), p1.size(), p2[0]) || PolygonContains2(p2.address(), p2.size(), p1[0]);
}


// Check to see if segment start-end intersects poly
// Assumes a polygon in format A-B-C-D if format is true, A-B, C-D, E-F if format is false
bool polygonIntersectsSegmentDetailed(const Point *poly, U32 vertexCount, bool format, const Point &start, const Point &end,
                                      F32 &collisionTime, Point &normal)
{
   Point v1 = poly[vertexCount - 1];
   Point v2, dv;
   Point dp = end - start;

   S32 inc = format ? 1 : 2;

   F32 currentCollisionTime = 100;

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

      F32 denom = dp.y * dv.x - dp.x * dv.y;
      if(denom != 0) // otherwise, the lines are parallel
      {
         F32 s = ( (start.x - v1.x) * dv.y + (v1.y - start.y) * dv.x ) / denom;
         F32 t = ( (start.x - v1.x) * dp.y + (v1.y - start.y) * dp.x ) / denom;

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
      collisionTime = currentCollisionTime;
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

   if(d.len() <= radius)
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
bool segmentsColinear(const Point &p1, const Point &p2, const Point &p3, const Point &p4)
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
bool segsOverlap(const Point &p1, const Point &p2, const Point &p3, const Point &p4, Point &overlapStart, Point &overlapEnd, F32 scaleFact)
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


// Returns index of points vector closest to point
S32 findClosestPoint(const Point &point, const Vector<Point> &points)
{
   F32 dist = F32_MAX;
   S32 closest = -1;

   for(S32 i = 0; i < points.size(); i++)
   {
      F32 d = points[i].distSquared(point);

      if(d < dist)
      {
         dist = d;
         closest = i;
      }
   }

   return closest;
}


bool zonesTouch(const Vector<Point> *zone1, const Vector<Point> *zone2, F32 scaleFact, Point &overlapStart, Point &overlapEnd)
{
   // Check for unlikely but fatal situation: Not enough vertices
   if(zone1->size() < 3 || zone2->size() < 3)
      return false;

   const Point *pi1, *pi2, *pj1, *pj2;

   // Now, do we actually touch?  Let's look, segment by segment
   for(S32 i = 0; i < zone1->size(); i++)
   {
      pi1 = &zone1->get(i);
      if(i == zone1->size() - 1)
         pi2 = &zone1->get(0);
      else
         pi2 = &zone1->get(i + 1);

      for(S32 j = 0; j < zone2->size(); j++)
      {
         pj1 = &zone2->get(j);
         if(j == zone2->size() - 1)
            pj2 = &zone2->get(0);
         else
            pj2 = &zone2->get(j + 1);

         if(segsOverlap(*pi1, *pi2, *pj1, *pj2, overlapStart, overlapEnd, scaleFact))
            return true;
      }
   }

   return false;
}


// Checks intersection between a polygon an moving circle at inBegin + t * inDelta with radius^2 = inA * t^2 + inB * t + inC, t in [0, 1]
// Returns true when it does and returns the intersection position in outPoint and the intersection fraction (value for t) in outFraction
bool SweptCircleEdgeVertexIntersect(const Point *inVertices, int inNumVertices, const Point &inBegin, const Point &inDelta, F32 inA, F32 inB, F32 inC, Point &outPoint, F32 &outFraction)
{
   // Loop through edges
   F32 upper_bound = 1.0f;
   bool collision = false;
   for (const Point *v1 = inVertices, *v2 = inVertices + inNumVertices - 1; v1 < inVertices + inNumVertices; v2 = v1, ++v1)
   {
      F32 t;

      // Check if circle hits the vertex
      Point bv1 = *v1 - inBegin;
      F32 a1 = inA - inDelta.lenSquared();
      F32 b1 = inB + 2.0f * inDelta.dot(bv1);
      F32 c1 = inC - bv1.lenSquared();
      if (findLowestRootInInterval(a1, b1, c1, upper_bound, t))
         if(inDelta.dot((*v1) - inBegin) > 0)
         {
            // We have a collision
            collision = true;
            upper_bound = t;
            outPoint = *v1;
         }

      // Check if circle hits the edge
      Point v1v2 = *v2 - *v1;
      F32 v1v2_dot_delta = v1v2.dot(inDelta);
      F32 v1v2_dot_bv1 = v1v2.dot(bv1);
      F32 v1v2_len_sq = v1v2.lenSquared();
      F32 a2 = v1v2_len_sq * a1 + v1v2_dot_delta * v1v2_dot_delta;
      F32 b2 = v1v2_len_sq * b1 - 2.0f * v1v2_dot_bv1 * v1v2_dot_delta;
      F32 c2 = v1v2_len_sq * c1 + v1v2_dot_bv1 * v1v2_dot_bv1;
      if (findLowestRootInInterval(a2, b2, c2, upper_bound, t))
      {
         // Check if the intersection point is on the edge
         F32 f = t * v1v2_dot_delta - v1v2_dot_bv1;
         if (f >= 0.0f && f <= v1v2_len_sq)
         {
            Point p(*v1 + v1v2 * (f / v1v2_len_sq));
            if(inDelta.dot(p - inBegin) > 0)
            {
               // We have a collision
               collision = true;
               upper_bound = t;
               outPoint = p;
            }
         }
      }
   }

   // Check if we had a collision
   if (!collision)
      return false;
   outFraction = upper_bound;
   return true;
}

// Should work with any polygons, convex and concave
bool PolygonSweptCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inBegin, const Point &inDelta, F32 inRadius, Point &outPoint, F32 &outFraction)
{
   // Test if circle intersects at t = 0
   if(polygonCircleIntersect(inVertices, inNumVertices, inBegin, inRadius * inRadius, outPoint, (Point *)&inDelta))
   {
      outFraction = 0;
      return true;
   }

   // Test if sphere intersects with one of the edges or vertices
   if (SweptCircleEdgeVertexIntersect(inVertices, inNumVertices, inBegin, inDelta, 0, 0, inRadius * inRadius, outPoint, outFraction))
   {
      return true;
   }
   return false;
}


static const float EPSILON=0.0000000001f;

F32 area(const Vector<Point> &contour)
{
  int n = contour.size();

  float A = 0.0f;

  for(int p = n-1, q = 0; q < n; p = q++)
    A += contour[p].x * contour[q].y - contour[q].x * contour[p].y;

  return A * 0.5f;
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
            V[s] = V[t]; nv--;

         /* resest error detection counter */
         count = 2*nv;
      }
   }

   delete[] V;

   return true;
}


static const F32 CLIPPER_SCALE_FACT = 1000;

Polygons upscaleClipperPoints(const Vector<const Vector<Point> *> &inputPolygons) 
{
   Polygons outputPolygons;

   outputPolygons.resize(inputPolygons.size());

   for(S32 i = 0; i < inputPolygons.size(); i++) 
   {
      outputPolygons[i].resize(inputPolygons[i]->size());

      for(S32 j = 0; j < inputPolygons[i]->size(); j++)
         outputPolygons[i][j] = IntPoint(S64(inputPolygons[i]->get(j).x * CLIPPER_SCALE_FACT), S64(inputPolygons[i]->get(j).y * CLIPPER_SCALE_FACT));
   }

   return outputPolygons;
}


Vector<Vector<Point> > downscaleClipperPoints(const Polygons& inputPolygons) 
{
   Vector<Vector<Point> > outputPolygons;

   outputPolygons.resize((U32)inputPolygons.size());

   for(U32 i = 0; i < inputPolygons.size(); i++) 
   {
      outputPolygons[i].resize((U32)inputPolygons[i].size());

      for(U32 j = 0; j < inputPolygons[i].size(); j++)
         outputPolygons[i][j] = Point(F32(inputPolygons[i][j].X) / CLIPPER_SCALE_FACT, F32(inputPolygons[i][j].Y) / CLIPPER_SCALE_FACT);
   }

   return outputPolygons;
}


// Use Clipper to merge inputPolygons, placing the result in outputPolygons
bool mergePolys(const Vector<const Vector<Point> *> &inputPolygons, Vector<Vector<Point> > &outputPolygons)
{
   Polygons input = upscaleClipperPoints(inputPolygons);
   Polygons solution;

   // Fire up clipper and union!
   Clipper clipper;

   try  // there is a "throw" in AddPolygon
   {
      clipper.AddPolygons(input, ptSubject);
   }
   catch(...)
   {
      logprintf(LogConsumer::LogError, "clipper.AddPolygons, something went wrong");
   }

   bool success = clipper.Execute(ctUnion, solution, pftNonZero, pftNonZero);

   if(success)
      outputPolygons = downscaleClipperPoints(solution);

   return success;
}


// Convert a Polygons to a list of points in a-b b-c c-d d-a format
void unpackPolygons(const Vector<Vector<Point> > &solution, Vector<Point> &lineSegmentPoints)
{
   // Precomputing list size improves performance dramatically
   S32 segments = 0;

   for(S32 i = 0; i < solution.size(); i++)
      segments += solution[i].size();

   lineSegmentPoints.resize(segments * 2);      // 2 points per line segment

   S32 index = 0;

   for(S32 i = 0; i < solution.size(); i++)
   {
      if(solution[i].size() == 0)
         continue;

      for(S32 j = 1; j < solution[i].size(); j++)
      {
         lineSegmentPoints[index++] = solution[i][j - 1];
         lineSegmentPoints[index++] = solution[i][j];
      }

      // Close the loop
      lineSegmentPoints[index++] = solution[i][solution[i].size() - 1];
      lineSegmentPoints[index++] = solution[i][0];
   }
}


// Offset a complex polygon by a given amount
// Uses clipper to create a buffer around a polygon with the given offset
void offsetPolygon(const Vector<Point> *inputPoly, Vector<Point> &outputPoly, const F32 offset)
{
   Vector<const Vector<Point> *> tempInputVector;
   tempInputVector.push_back(inputPoly);

   // Upscale for clipper
   Polygons polygons = upscaleClipperPoints(tempInputVector);

   // Call Clipper to do the dirty work
   OffsetPolygons(polygons, polygons, offset * CLIPPER_SCALE_FACT);

   // Downscale
   Vector<Vector<Point> > tempOutputVector = downscaleClipperPoints(polygons);

   TNLAssert(tempOutputVector.size() > 0, "tempVector empty in offsetPolygon?");
   if(tempOutputVector.size() > 0)
      outputPoly = tempOutputVector[0];
}


// Convert a list of floats into a list of points, removing all collinear points
Vector<Point> floatsToPoints(const Vector<F32> floats)
{
   Vector<Point> points;
   points.reserve(floats.size() / 2);

   for(S32 i = 1; i < floats.size(); i += 2)
      points.push_back( Point(floats[i-1], floats[i]) );

   removeCollinearPoints(points, false);   // Remove collinear points to make rendering nicer and datasets smaller

   return points;
}


// Test if a complex polygon has clockwise point winding order
// Implemented from
// http://stackoverflow.com/questions/1165647/how-to-determine-if-a-list-of-polygon-points-are-in-clockwise-order/1165943#1165943
bool isWoundClockwise(const Vector<Point>& inputPoly)
{
   F32 finalSum = 0;
   S32 i_prev = inputPoly.size() - 1;

   for(S32 i = 0; i < inputPoly.size(); i++)
   {
      // (x2-x1)(y2+y1)
      finalSum += (inputPoly[i].x - inputPoly[i_prev].x) * (inputPoly[i].y + inputPoly[i_prev].y);
      i_prev = i;
   }

   // Negative result = counter-clockwise
   if(finalSum < 0)
      return false;
   else
      return true;
}


bool Triangulate::processComplex(Vector<Point> &outputTriangles, const Rect& bounds,
      const Vector<Vector<Point> >& polygonList)
{
   // Here we will divide up out input polygon list into holes and inner boundaries of circuit
   // holes.  We know an inner boundary of a circuit-hole is wound clockwise because that's how
   // we set up Clipper to output
   //
   // Also used to keep track of our memory to clean up
   // TODO:  Convert to use our Point and Vector classes somehow?
   vector<vector<p2t::Point*> > polylines;   // MUST be clockwise
   vector<vector<p2t::Point*> > holes;       // MUST be counterclockwise

   // First build our map extents outline polygon.  Clockwise!
   vector<p2t::Point*> outline;

   F32 minx = bounds.min.x;  F32 miny = bounds.min.y;
   F32 maxx = bounds.max.x;  F32 maxy = bounds.max.y;

   outline.push_back(new p2t::Point(minx, miny));
   outline.push_back(new p2t::Point(minx, maxy));
   outline.push_back(new p2t::Point(maxx, maxy));
   outline.push_back(new p2t::Point(maxx, miny));

   polylines.push_back(outline);

   // Now determine what is a hole and what is another polyline bound we need to triangulate
   // Almost everything will be a hole
   for(S32 i = 0; i < polygonList.size(); i++)
   {
      Vector<Point> currentPoly = polygonList[i];

      if(isWoundClockwise(currentPoly))  // Considered a bounding polyline to triangulate
      {
         vector<p2t::Point*> polyline;
         for(S32 j = 0; j < currentPoly.size(); j++)
            polyline.push_back(new p2t::Point(currentPoly[j].x, currentPoly[j].y));

         polylines.push_back(polyline);
      }
      else
      {
         vector<p2t::Point*> hole;
         for(S32 j = 0; j < currentPoly.size(); j++)
            hole.push_back(new p2t::Point(currentPoly[j].x, currentPoly[j].y));

         holes.push_back(hole);
      }
   }


   vector<p2t::CDT*> cdtRegistry;       // For keeping track of memory

   for(S32 i = 0; i < polylines.size(); i++)
   {
      vector<p2t::Point*> polyline = polylines[i];

      // Create our worker and keep track of it for clean-up later
      p2t::CDT* cdt = new p2t::CDT(polyline);
      cdtRegistry.push_back(cdt);

      // Here we filter holes so as to not make poly2tri do extra work.  This improved poly2tri
      // performance by at least 1 magnitude
      //
      // TODO: possibly be even smarter here and filter out holes that belong to a 'child' polyline bound
      // i.e. some sort of hierarchal hole-to-polyline bound relationship
      vector<vector<p2t::Point*> > filteredHoles;
      for(S32 k = 0; k < holes.size(); k++)
      {
         // If only one of the points of the hole is within our bounding polyline, the
         // entire holes should be.  This is because Clipper guarantees no overlapping
         // polygons.  Thanks Clipper!
         if(PolygonContains2p2t(&polyline[0], polyline.size(), holes[k][0]))  // &polyline[0]  <- vector to array!
            filteredHoles.push_back(holes[k]);
      }

      // Add our holes
      for(S32 j = 0; j < filteredHoles.size(); j++)
         cdt->AddHole(filteredHoles[j]);

      // Do the work!
      cdt->Triangulate();

      // Add current output triangles to our total
      vector<p2t::Triangle*> currentOutput = cdt->GetTriangles();

      // Copy our data to TNL::Point and to our output Vector
      p2t::Triangle *currentTriangle;
      for(S32 i = 0; i < currentOutput.size(); i++)
      {
         currentTriangle = currentOutput[i];
         outputTriangles.push_back(Point(currentTriangle->GetPoint(0)->x, currentTriangle->GetPoint(0)->y));
         outputTriangles.push_back(Point(currentTriangle->GetPoint(1)->x, currentTriangle->GetPoint(1)->y));
         outputTriangles.push_back(Point(currentTriangle->GetPoint(2)->x, currentTriangle->GetPoint(2)->y));
      }
   }


   // Clean up memory used with poly2tri
   // Clean-up worker
   for(S32 i = 0; i < cdtRegistry.size(); i++)
      delete cdtRegistry[i];

   // Free our many, many constructed pt2::Point objects
   for(S32 i = 0; i < polylines.size(); i++)
   {
      vector<p2t::Point*> polyline = polylines[i];
      vector<p2t::Point*>::iterator iterator;
      for(iterator = polyline.begin(); iterator != polyline.end(); ++iterator)
         delete *iterator;

      polyline.clear();
   }

   // Make sure we have output data
   if(outputTriangles.size() == 0)
      return false;

   return true;
}


// Merge triangles into convex polygons, uses Recast method
bool Triangulate::mergeTriangles(const Vector<Point> &triangleData, rcPolyMesh& mesh, S32 maxVertices)
{
   S32 pointCount = triangleData.size();
   S32 triangleCount = triangleData.size() / 3;

   TNLAssert(pointCount % 3 == 0, "Triangles are not triangles?");

   S32 intPoints[pointCount * 2];     // 2 entries per point: x,y
   S32 triangleList[pointCount];      // 1 entry per vertex

   if(pointCount > U16_MAX) // too many points for rcBuildPolyMesh
      return false;

   for(S32 i = 0; i < pointCount; i++)
   {
      intPoints[2*i]   = (S32)floor(triangleData[i].x + 0.5) + mesh.offsetX;
      intPoints[2*i+1] = (S32)floor(triangleData[i].y + 0.5) + mesh.offsetY;

      triangleList[i] = i;  // Our triangle list is ordered so every 3 is a triangle in correct winding order
   }

   return rcBuildPolyMesh(maxVertices, intPoints, pointCount, triangleList, triangleCount, mesh);
}


// Derived from formulae here: http://local.wasp.uwa.edu.au/~pbourke/geometry/polyarea/
Point findCentroid(const Vector<Point> &polyPoints)
{
   if(polyPoints.size() == 0)
      return Point(0,0);

   F32 area6 = area(polyPoints) * 6;
   F32 x = 0;
   F32 y = 0;

   for(S32 i = 0; i < polyPoints.size(); i++)
   {
      Point p1 = polyPoints[i];
      Point p2 = polyPoints[(i < polyPoints.size() - 1) ? i + 1 : 0];

      F32 z = (p1.x * p2.y - p2.x * p1.y);

      x += (p1.x + p2.x) * z;
      y += (p1.y + p2.y) * z;
   }

   x /= area6;
   y /= area6;

   return Point(x,y);
}


// Find longest edge, so we can align text with it...
F32 angleOfLongestSide(const Vector<Point> &polyPoints)
{
   if(polyPoints.size() <= 1)
      return 0;

   Point start;
   Point end;
   F32 maxlen = -1;
   F32 ang = 0;

   for(S32 i = 0; i < polyPoints.size(); i++)
   {
      Point p1 = polyPoints[i];
      Point p2 = polyPoints[(i < polyPoints.size() - 1) ? i + 1 : 0];
      F32 len = p1.distSquared(p2);

      if(len > maxlen + .1)     // .1 helps in editor if two sides are essentially equal 
      { 
         start = p1;
         end = p2;
         maxlen = len;

         ang = start.angleTo(end);
      }
      else if(len > maxlen - .1)    // Lengths are essentially equal... align text along "more horizontal" axis
      {
         if(fabs(p1.angleTo(p2)) < fabs(ang))
         {
            start = p1;
            end = p2;
            ang = start.angleTo(end);
         }
      }
   }

   // Make sure text is right-side-up
   if(ang < -FloatHalfPi || ang > FloatHalfPi)
      ang += FloatPi;
   return ang;
}


// Find closest point from p on segment s1-s2 that is perpendicular to s1-s2
bool findNormalPoint(const Point &p, const Point &s1, const Point &s2, Point &closest)
{
   Point edgeDelta = s2 - s1;    // Vector defining extent of segment
   Point pointDelta = p - s1;    // Distance from p to start of segment

   float fraction = pointDelta.dot(edgeDelta);  // "Perpendicularize" pointDelta towards edgeDelta
   float lenSquared = edgeDelta.lenSquared();

   if(fraction > 0 && fraction < lenSquared)                // Intersection!
   {
      closest = s1 + edgeDelta * (fraction / lenSquared);   // Closest point
      return true;
   }
   else   // Didn't find a good match
      return false;
}


// Based on http://www.gamedev.net/community/forums/topic.asp?topic_id=440350
bool segmentsIntersect(const Point &p1, const Point &p2, const Point &p3, const Point &p4, F32 &collisionTime)
{
    F32 denom = ((p4.y - p3.y) * (p2.x - p1.x)) - ((p4.x - p3.x) * (p2.y - p1.y));

    F32 numerator1 = ((p4.x - p3.x) * (p1.y - p3.y)) - ((p4.y - p3.y) * (p1.x - p3.x));
    F32 numerator2 = ((p2.x - p1.x) * (p1.y - p3.y)) - ((p2.y - p1.y) * (p1.x - p3.x));

    if ( denom == 0.0 )
       //if ( numerator1 == 0.0 && numerator2 == 0.0 )
       //   return false;  //COINCIDENT;
    return false;  // PARALLEL;

    collisionTime = numerator1 / denom;
    F32 ub = numerator2 / denom;

    return (collisionTime >= 0.0 && collisionTime <= 1.0 && ub >= 0.0 && ub <= 1.0);
    // Point intersection(p1.x + collisionTime * (p2.x - p1.x), p1.y + collisionTime * (p2.y - p1.y));
}


bool findIntersection(const Point &p1, const Point &p2, const Point &p3, const Point &p4, Point &intersection)
{
    F32 denom = ((p4.y - p3.y) * (p2.x - p1.x)) - ((p4.x - p3.x) * (p2.y - p1.y));
    F32 numerator = ((p4.x - p3.x) * (p1.y - p3.y)) - ((p4.y - p3.y) * (p1.x - p3.x));

    F32 numerator2 = ((p2.x - p1.x) * (p1.y - p3.y)) - ((p2.y - p1.y) * (p1.x - p3.x));

    if ( denom == 0.0 )
       //if ( numerator == 0.0 && numerator2 == 0.0 )
       //   return false;  //COINCIDENT;
    return false;  // PARALLEL;

    F32 ua = numerator / denom;
    F32 ub = numerator2/ denom;

    if (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0)
    {
      intersection.set(p1.x + ua * (p2.x - p1.x), p1.y + ua * (p2.y - p1.y));
      return true;
    }
    else
       return false;
}


Point shortenSegment(const Point &startPoint, const Point &endPoint, F32 lengthReduction)
{
   // Determine the directional vector
   Point dir = endPoint - startPoint;

   // Save current length
   F32 length = dir.len();

   // Normalize into a unit vector of sorts
   dir.normalize();

   // Multiply by the new length
   // Careful!  If lengthReduction is greater than the segment length, you'll get an
   // end point on the opposite vector!!
   dir *= (length - lengthReduction);

   // Return the new end-point
   return startPoint + dir;
}


////////////////////////////////////////
////////////////////////////////////////

// Takes a list of vertices representing corners and converts them into a list of lines representing the edges of an object
// Basically, taking a vector like A-B-C-D and converting it to A-B-B-C-C-D
void cornersToEdges(const Vector<Point> &corners, Vector<Point> &edges)  
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


// Given the points in points, figure out where the ends of the walls should be (they'll need to be extended slighly in some cases
// for better rendering).  Set extendAmt to 0 to see why it's needed.
// Populates barrierEnds with the results.
void constructBarrierEndPoints(const Vector<Point> *points, F32 width, Vector<Point> &barrierEnds)
{
   barrierEnds.clear();       // Local static vector

   if(points->size() <= 1)    // Protect against bad data
      return;

   bool loop = (points->first() == points->last());      // Does our barrier form a closed loop?

   Vector<Point> edgeVector;
   for(S32 i = 0; i < points->size() - 1; i++)
   {
      Point e = points->get(i+1) - points->get(i);
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

      cosTheta = fabs(cosTheta);     // Seems to reduce "end gap" on acute junction angles
      
      F32 extendAmt = width * 0.5f * F32(tan( acos(cosTheta) / 2 ));
      if(extendAmt > 0.01f)
         extendAmt -= 0.01f;
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

      Point start = points->get(i)   - edgeVector[i] * extendBack;
      Point end   = points->get(i+1) + edgeVector[i] * extendForward;

      barrierEnds.push_back(start);
      barrierEnds.push_back(end);
   }
}


// Takes a segment and "puffs its width out" to a rectangle of a specified width, filling cornerPoints.  Does not extend endpoints.
void expandCenterlineToOutline(const Point &start, const Point &end, F32 width, Vector<Point> &cornerPoints)  
{
   cornerPoints.clear();

   Point dir = end - start;
   Point crossVec(dir.y, -dir.x);
   crossVec.normalize(width * 0.5f);

   cornerPoints.push_back(start + crossVec);
   cornerPoints.push_back(end   + crossVec);
   cornerPoints.push_back(end   - crossVec);
   cornerPoints.push_back(start - crossVec);
}



};
