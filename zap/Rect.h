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

#ifndef _RECT_H_
#define _RECT_H_

#include "tnlTypes.h"
#include "tnlVector.h"
#include "Point.h"

using namespace TNL;

namespace Zap
{

class Rect
{

public:
   typedef float member_type;
   Point min, max;

   Rect();                                     // Constuctor
   Rect(const Point &p1, const Point &p2);    // Constuctor
   Rect(F32 x1, F32 y1, F32 x2, F32 y2);       // Constuctor
   Rect(const Point &p, member_type size);    // Constuctor, takes point and "radius"

   Rect(const TNL::Vector<Point> &p);         // Construct as a bounding box around multiple points

   Point getCenter();

   void set(const Point &p1, const Point &p2);

   void set(const TNL::Vector<Point> &p);     // Set to bounding box around multiple points

   void set(const Rect &r);

   void set(const Point &p, member_type size);   // Takes point and "radius"

   bool contains(const Point &p);    // Rect contains the point

   void unionPoint(const Point &p);

   void unionRect(const Rect &r);

   // Does rect interset rect r?
   bool intersects(const Rect &r);
   
   // Does rect interset or border on rect r?
   bool intersectsOrBorders(const Rect &r);

   // Does rect intersect line defined by p1 and p2?
   bool intersects(const Point &p1, const Point &p2);

   void expand(const Point &delta);
   void expandToInt(const Point &delta);

   void offset(const Point &offset);

   F32 getWidth();
   F32 getHeight();

   Point getExtents();

   // inlines must stay in headers
   inline Rect& operator=(const Rect &r)  // Performance equivalent to set()
   {
      set(r);
      return *this;
   }
};

};	// namespace

#endif // _RECT_H_

