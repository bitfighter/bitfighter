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

#include "tnlVector.h"
#include "Rect.h"
#include "GeomUtils.h"

#include <math.h>

namespace Zap
{

// Constructors
IntRect::IntRect()
{
   set(0, 0, 0, 0);
}


IntRect::IntRect(S32 x1, S32 y1, S32 x2, S32 y2)
{
   set(x1, y1, x2, y2);
}


void IntRect::set(S32 x1, S32 y1, S32 x2, S32 y2)
{
   minx = min(x1, x2);
   miny = min(y1, y2);
   maxx = max(x1, x2);
   maxy = max(y1, y2);
}


////////////////////////////////////////
////////////////////////////////////////

// Constuctors
Rect::Rect()
{
   set(Point(), Point());
}


Rect::Rect(const Point &p1, const Point &p2)
{
   set(p1, p2);
}


Rect::Rect(F32 x1, F32 y1, F32 x2, F32 y2)
{
   set(Point(x1, y1), Point(x2, y2));
}


// Takes centerpoint and "diameter"
Rect::Rect(const Point &p, member_type diameter)
{
   set(p, diameter);
}


Rect::Rect(const Rect *r)
{
   set(*r);
}


// Construct as a bounding box around multiple points
Rect::Rect(const Vector<Point> &p)
{
   set(p);
}


Point Rect::getCenter() const
{
   return (max + min) * 0.5;
}


void Rect::set(const Point &p1, const Point &p2)
{
   if(p1.x < p2.x)
   {
      min.x = p1.x;
      max.x = p2.x;
   }
   else
   {
      min.x = p2.x;
      max.x = p1.x;
   }

   if(p1.y < p2.y)
   {
      min.y = p1.y;
      max.y = p2.y;
   }
   else
   {
      min.y = p2.y;
      max.y = p1.y;
   }
   // The above might be replaceable with:
   //       min = p1;
   //       max = p1;
   //       unionPoint(p2);

}

// Set to bounding box around multiple points
void Rect::set(const Vector<Point> &p)
{
   if(p.size() == 0)
   {
      set(Point(), Point());
      return;
   }

   min = p[0];
   max = p[0];

   for(int i = 1; i < p.size(); i++)
      unionPoint(p[i]);
}

void Rect::set(const Rect &r)
{
   min.x = r.min.x;
   min.y = r.min.y;

   max.x = r.max.x;
   max.y = r.max.y;
}

// Takes centerpoint and "diameter"
void Rect::set(const Point &p, member_type diameter)
{
   F32 radius = diameter * 0.5;
   min.x = p.x - radius;
   max.x = p.x + radius;
   min.y = p.y - radius;
   max.y = p.y + radius;
}

// Rect contains the point
bool Rect::contains(const Point &p) const
{
   return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
}

void Rect::unionPoint(const Point &p)
{
   if(p.x < min.x)        min.x = p.x;
   else if(p.x > max.x)   max.x = p.x;

   if(p.y < min.y)        min.y = p.y;
   else if(p.y > max.y)   max.y = p.y;
}

void Rect::unionRect(const Rect &r)
{
   if(r.min.x < min.x)    min.x = r.min.x;
   if(r.max.x > max.x)    max.x = r.max.x;
   if(r.min.y < min.y)    min.y = r.min.y;
   if(r.max.y > max.y)    max.y = r.max.y;
}

// Does rect interset rect r?
bool Rect::intersects(const Rect &r)
{
   return min.x < r.max.x && min.y < r.max.y &&
         max.x > r.min.x && max.y > r.min.y;
}

// Does rect interset or border on rect r?
bool Rect::intersectsOrBorders(const Rect &r)
{
   F32 littleBit = 0.001f;
   return min.x <= r.max.x + littleBit && min.y <= r.max.y + littleBit &&
         max.x >= r.min.x - littleBit && max.y >= r.min.y - littleBit;
}


// Does rect intersect line defined by p1 and p2?
bool Rect::intersects(const Point &p1, const Point &p2) const
{
   F32 ct;

   return intersects(p1, p2, ct);
}


// As above, but returns info about where along segment p1-p2 collision occurs.  Returns 0 if segment is entirely contained in rect.
bool Rect::intersects(const Point &p1, const Point &p2, member_type &collisionTime) const
{
   collisionTime = 0;      
   return ( 
            segmentsIntersect(p1, p2, Point(min.x, min.y), Point(min.x, max.y), collisionTime) ||
            segmentsIntersect(p1, p2, Point(min.x, max.y), Point(max.x, max.y), collisionTime) ||
            segmentsIntersect(p1, p2, Point(max.x, max.y), Point(max.x, min.y), collisionTime) ||
            segmentsIntersect(p1, p2, Point(max.x, min.y), Point(min.x, min.y), collisionTime) ||
            contains(p1) || contains(p2)
   );

}


// Adapted from hightest-rated solution on http://stackoverflow.com/questions/401847/circle-rectangle-collision-detection-intersection
bool Rect::intersects(const Point &center, F32 radius) const
{
    F32 rectHalfWidth = (max.x - min.x) / 2;
    F32 rectHalfHeight = (max.y - min.y) / 2;

    // Calculate the absolute values of the x and y difference between the center of the circle and the center of the rectangle. 
    // This collapses the four quadrants down into one, so that the calculations do not have to be done four times. 
    F32 rectCenter_x = (min.x + max.x) / 2;
    F32 rectCenter_y = (min.y + max.y) / 2;

    F32 circleDistance_x = fabs(center.x - rectCenter_x);   
    F32 circleDistance_y = fabs(center.y - rectCenter_y);  

    // Eliminate the easy cases where the circle is far enough away from the rectangle (in either direction) that no intersection is possible
    if(circleDistance_x > (rectHalfWidth + radius)) { return false; }
    if(circleDistance_y > (rectHalfHeight + radius)) { return false; }

    // Eliminate the easy cases where the circle is close enough to the rectangle (in either direction) that an intersection is guaranteed
    if(circleDistance_x <= rectHalfWidth) { return true; } 
    if(circleDistance_y <= rectHalfHeight) { return true; }

    // Calculate the difficult case where the circle may intersect the corner of the rectangle. To solve, compute the distance from the 
    // center of the circle and the corner, and then verify that the distance is not more than the radius of the circle.
    F32 cornerDistance_sq = (circleDistance_x - rectHalfWidth)  * (circleDistance_x - rectHalfWidth) +
                            (circleDistance_y - rectHalfHeight) * (circleDistance_y - rectHalfHeight);

    return cornerDistance_sq <= (radius * radius);
}


#define INTIFY(a) (a) < 0 ? floor(a) : ceil(a) 

void Rect::expand(const Point &delta)
{
   min -= delta;
   max += delta;
}

void Rect::expandToInt(const Point &delta)
{
   expand(delta);
   min.set(INTIFY(min.x), INTIFY(min.y));
   max.set(INTIFY(max.x), INTIFY(max.y));
}

void Rect::offset(const Point &offset)
{
   min += offset;
   max += offset;
}

void Rect::toPoly(Vector<Point> &polyPoints)
{
   polyPoints.push_back(Point(max.x, max.y));
   polyPoints.push_back(Point(max.x, min.y));
   polyPoints.push_back(Point(min.x, min.y));
   polyPoints.push_back(Point(min.x, max.y));
}

F32 Rect::getWidth() const
{
   return max.x - min.x;
}

F32 Rect::getHeight() const
{
   return max.y - min.y;
}

Point Rect::getExtents() const
{
   return max - min;
}

string Rect::toString() const
{
   return min.toString() + " " + max.toString();
}

};	// namespace
