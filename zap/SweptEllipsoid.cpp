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

#include "point.h" 
#include "tnlVector.h"
#include "SweptEllipsoid.h"      // Must be last

using namespace TNL;

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

#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define ABS(x) (((x) > 0) ? (x) : -(x))


// From http://local.wasp.uwa.edu.au/~pbourke/geometry/insidepoly/
// Should work on all polygons, convex and concave alike
// To use with a Vector of points, pass in vector.address() and vector.size()
bool PolygonContains2(const Point *inVertices, int inNumVertices, const Point &inPoint)
{
   int counter = 0;
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
            counter++;
      }

      p1 = p2;
   }

   return (counter & 1) != 0;   // True when number is odd, false when number is even ==> essentially replaces (counter % 2 == 0)
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
bool pointInTriangle(Point p, Point a, Point b, Point c)
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


// Based on http://www.opengl.org/discussion_boards/ubbthreads.php?ubb=showflat&Number=248453
// No idea if this is optimal or not, but it is only used in the editor, and works fine for our purposes.
bool isConvex(const Vector<Point> &verts)
{
  Point v1, v2;
  double det_value, cur_det_value;
  int num_vertices = verts.size();
  
  if(num_vertices < 3)
     return true;
  
  v1 = verts[0] - verts[num_vertices-1];
  v2 = verts[1] - verts[0];
  det_value = v1.determinant(v2);
  
  for(S32 i = 1 ; i < num_vertices-1 ; i++)
  {
    v1 = v2;
    v2 = verts[i+1] - verts[i];
    cur_det_value = v1.determinant(v2);
    
    if( (cur_det_value * det_value) < 0.0 )
      return false;
  }
  
  v1 = v2;
  v2 = verts[0] - verts[num_vertices-1];
  cur_det_value = v1.determinant(v2);
  
  return  (cur_det_value * det_value) >= 0.0;
}


// Check if circle at inCenter with radius^2 = inRadiusSq intersects with a polygon.
// Function returns true when it does and the intersection point is in outPoint
// Works only for convex hulls.. maybe no longer true... may work for all polys now
bool polygonCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inCenter, F32 inRadiusSq, Point &outPoint)
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
   for(S32 i = 0; i < points.size(); i++)
      if(segmentsIntersect(start, end, points[points.size() - 1], points[i]))
         return true;
   
   //  Line inside polygon?  If so, then the start will be within.
   return PolygonContains2(points.address(), points.size(), start);
}


// Returns true if polygons represented by p1 & p2 intersect or one contains the other
bool polygonsIntersect(const Vector<Point> &p1, const Vector<Point> &p2)
{
   const Point *rp1 = &p1[p1.size() - 1];
   for(S32 i = 0; i < p1.size(); i++)
   {
      const Point *rp2 = &p1[i];
      
      const Point *cp1 = &p2[p2.size() - 1];

      for(S32 j = 0; j < p2.size(); j++)
      {
         const Point *cp2 = &p2[j];
         if(segmentsIntersect(*rp1, *rp2, *cp1, *cp2))
            return true;
         cp1 = cp2;
      }
      rp1 = rp2;
   }
   //  All points of polygon is inside the other polygon?  At this point, if any are, all are.
   return PolygonContains2(p1.address(), p1.size(), p2[0]) || PolygonContains2(p2.address(), p2.size(), p1[0]);
}


// Do segments sit on same virtual line?
bool segmentsColinear(const Point &p1, const Point &p2, const Point &p3, const Point &p4)
{
   const float smallNumber = (float) 0.0000001;

   float denom = ((p4.y - p3.y) * (p2.x - p1.x)) - ((p4.x - p3.x) * (p2.y - p1.y));
   float numerator = ((p4.x - p3.x) * (p1.y - p3.y)) - ((p4.y - p3.y) * (p1.x - p3.x));
   float numerator2 = ((p2.x - p1.x) * (p1.y - p3.y)) - ((p2.y - p1.y) * (p1.x - p3.x));

   if(ABS(denom) < smallNumber && ABS(numerator) < smallNumber && ABS(numerator2) < smallNumber)
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
//	F32 r_numerator   = (c.x - a.x) * (b.x - a.x) + (c.y - a.y) * (b.y - a.y);
//	F32 r_denomenator = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
//   F32 r = r_numerator / r_denomenator;
//
//   F32 s = F32((a.y - c.y) * (b.x - a.x) - (a.x - c.x) * (b.y - a.y));
//
//	if ((r >= 0) && (r <= 1)) 
//	{
//		return(ABS(s) < .0001);      
//	}
//	else
//      return false;
//}


// See if segment p1-p2 overlaps p3-p4
// Coincident endpoints alone do not count!
// Pass back the overpping extent in two points
bool segsOverlap(const Point &p1, const Point &p2, const Point &p3, const Point &p4, Point &overlapStart, Point &overlapEnd, F32 scaleFact)
{
   const Point *pInt;
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


bool zonesTouch(const Vector<Point> &zone1, const Vector<Point> &zone2, F32 scaleFact, Point &overlapStart, Point &overlapEnd)
{
   // Check for unlikely but fatal situation: Not enough vertices
   if(zone1.size() < 3 || zone2.size() < 3)
      return false;

   const Point *pi1, *pi2, *pj1, *pj2;

   // Now, do we actually touch?  Let's look, segment by segment
   for(S32 i = 0; i < zone1.size(); i++)
   {
      pi1 = &zone1[i];
      if(i == zone1.size() - 1)
         pi2 = &zone1[0];
      else
         pi2 = &zone1[i + 1];

      for(S32 j = 0; j < zone2.size(); j++)
      {
         pj1 = &zone2[j];
         if(j == zone2.size() - 1)
            pj2 = &zone2[0];
         else
            pj2 = &zone2[j + 1];

         if(segsOverlap(*pi1, *pi2, *pj1, *pj2, overlapStart, overlapEnd, scaleFact))
            return true;
      }
   }

   return false;
}


void Swap(F32 &f1, F32 &f2)
{
   F32 temp = f1;
   f1 = f2;
   f2 = temp;
}


// Solve the equation inA * x^2 + inB * x + inC == 0 for the lowest x in [0, inUpperBound].
// Returns true if there is such a solution and returns the solution in outX
bool FindLowestRootInInterval(F32 inA, F32 inB, F32 inC, F32 inUpperBound, F32 &outX)
{
   // Check if a solution exists
   F32 determinant = inB * inB - 4.0f * inA * inC;
   if (determinant < 0.0f)
      return false;

   // The standard way of doing this is by computing: x = (-b +/- Sqrt(b^2 - 4 a c)) / 2 a
   // is not numerically stable when a is close to zero.
   // Solve the equation according to "Numerical Recipies in C" paragraph 5.6
   F32 q = -0.5f * (inB + (inB < 0.0f? -1.0f : 1.0f) * sqrt(determinant));

   // Both of these can return +INF, -INF or NAN that's why we test both solutions to be in the specified range below
   F32 x1 = q / inA;
   F32 x2 = inC / q;

   // Order the results
   if (x2 < x1)
      Swap(x1, x2);

   // Check if x1 is a solution
   if (x1 >= 0.0f && x1 <= inUpperBound)
   {
      outX = x1;
      return true;
   }

   // Check if x2 is a solution
   if (x2 >= 0.0f && x2 <= inUpperBound)
   {
      outX = x2;
      return true;
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
      if (FindLowestRootInInterval(a1, b1, c1, upper_bound, t))
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
      if (FindLowestRootInInterval(a2, b2, c2, upper_bound, t))
      {
         // Check if the intersection point is on the edge
         F32 f = t * v1v2_dot_delta - v1v2_dot_bv1;
         if (f >= 0.0f && f <= v1v2_len_sq)
         {
            // We have a collision
            collision = true;
            upper_bound = t;
            outPoint = *v1 + v1v2 * (f / v1v2_len_sq);
         }
      }
   }

   // Check if we had a collision
   if (!collision)
      return false;
   outFraction = upper_bound;
   return true;
}

// I believe this will work only for convex polygons
bool PolygonSweptCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inBegin, const Point &inDelta, F32 inRadius, Point &outPoint, F32 &outFraction)
{
   // Test if circle intersects at t = 0
   if(polygonCircleIntersect(inVertices, inNumVertices, inBegin, inRadius * inRadius, outPoint))
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

  float A=0.0f;

  for(int p=n-1,q=0; q<n; p=q++)
  {
    A+= contour[p].x*contour[q].y - contour[q].x*contour[p].y;
  }
  return A*0.5f;
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
  if ( n < 3 ) return false;

  int *V = new int[n];

  /* we want a counter-clockwise polygon in V */

  if ( 0.0f < area(contour) )
    for (int v=0; v<n; v++) V[v] = v;
  else
    for(int v=0; v<n; v++) V[v] = (n-1)-v;

  int nv = n;

  /*  remove nv-2 Vertices, creating 1 triangle every time */
  int count = 2*nv;   /* error detection */

  for(int m=0, v=nv-1; nv>2; )
  {
    /* if we loop, it is probably a non-simple polygon */
    if (0 >= (count--))
    {
      //** Triangulate: ERROR - probable bad polygon!
      return false;
    }

    /* three consecutive vertices in current polygon, <u,v,w> */
    int u = v  ; if (nv <= u) u = 0;     /* previous */
    v = u+1; if (nv <= v) v = 0;     /* new v    */
    int w = v+1; if (nv <= w) w = 0;     /* next     */

    if ( Snip(contour,u,v,w,nv,V) )
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
      for(s=v,t=v+1;t<nv;s++,t++) V[s] = V[t]; nv--;

      /* resest error detection counter */
      count = 2*nv;
    }
  }

  delete[] V;

  return true;
}


// Derived from formulae here: http://local.wasp.uwa.edu.au/~pbourke/geometry/polyarea/
Point findCentroid(const Vector<Point> &polyPoints)
{
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
         if(ABS(p1.angleTo(p2)) < ABS(ang))
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
bool segmentsIntersect(const Point &p1, const Point &p2, const Point &p3, const Point &p4)
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

    return (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0);
    // Point intersection(p1.x + ua * (p2.x - p1.x), p1.y + ua * (p2.y - p1.y));
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



};


#undef MIN
#undef MAX
#undef ABS

