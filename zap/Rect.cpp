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

using namespace TNL;

namespace Zap
{

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

Rect::Rect(const Point &p, member_type size)
{
   set(p, size);
}

// Construct as a bounding box around multiple points
Rect::Rect(const TNL::Vector<Point> &p)
{
   set(p);
}

Point Rect::getCenter()
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
void Rect::set(const TNL::Vector<Point> &p)
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

// Takes point and "radius"
void Rect::set(const Point &p, member_type size)
{
   F32 sizeDiv2 = size / 2;
   min.x = p.x - sizeDiv2;
   max.x = p.x + sizeDiv2;
   min.y = p.y - sizeDiv2;
   max.y = p.y + sizeDiv2;
}

// Rect contains the point
bool Rect::contains(const Point &p)
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
bool Rect::intersects(const Point &p1, const Point &p2)
{
   return ( segmentsIntersect(p1, p2, Point(min.x, min.y), Point(min.x, max.y)) ||
         segmentsIntersect(p1, p2, Point(min.x, max.y), Point(max.x, max.y)) ||
         segmentsIntersect(p1, p2, Point(max.x, max.y), Point(max.x, min.y)) ||
         segmentsIntersect(p1, p2, Point(max.x, min.y), Point(min.x, min.y)) ||
         contains(p1) || contains(p2)
   );
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

F32 Rect::getWidth()
{
   return max.x - min.x;
}

F32 Rect::getHeight()
{
   return max.y - min.y;
}

Point Rect::getExtents()
{
   return max - min;
}

};	// namespace
