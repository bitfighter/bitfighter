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

#include "Point.h"

#include "tnlPlatform.h"
#include "tnlBitStream.h"
#include "stringUtils.h"

#include <math.h>
#include <cstdlib>


namespace Zap
{

// Constructors...
Point::Point()
{
   x = 0;
   y = 0;
}

Point::Point(const Point& pt)
{
   x = pt.x;
   y = pt.y;
}

void Point::set(const Point &pt)
{
   x = pt.x;
   y = pt.y;
}

void Point::set(const Point *pt)
{
   x = pt->x;
   y = pt->y;
}

// Distance from (0,0)
F32 Point::len() const
{
   return (F32) sqrt(x * x + y * y);
}

F32 Point::lenSquared() const
{
   return x * x + y * y;
}

void Point::normalize()
{
   F32 l = len();

   if(l == 0)
   {
      x = 1;
      y = 0;
   }
   else
   {
      l = 1 / l;
      x *= l;
      y *= l;
   }
}

void Point::normalize(float newLen)
{
   F32 l = len();

   if(l == 0)
   {
      x = newLen;
      y = 0;
   }
   else
   {
      l = newLen / l;
      x *= l;
      y *= l;
   }
}


void Point::interp(float t, const Point &p1, const Point &p2)
{
   float oneMinusT = 1.0f - t;
   x = p1.x * t + p2.x * oneMinusT;
   y = p1.y * t + p2.y * oneMinusT;
}


F32 Point::ATAN2() const
{
   return atan2(y, x);
}

F32 Point::distanceTo(const Point &pt) const
{
   return sqrt( (x-pt.x) * (x-pt.x) + (y-pt.y) * (y-pt.y) );
}

F32 Point::distSquared(const Point &pt) const
{
   return((x-pt.x) * (x-pt.x) + (y-pt.y) * (y-pt.y));
}

F32 Point::angleTo(const Point &p) const
{
   return atan2(p.y-y, p.x-x);
}

Point Point::rotate(F32 ang)
{
   F32 sina = sin(ang);
   F32 cosa = cos(ang);

   return Point(x * sina + y * cosa, y * sina - x * cosa);
}

void Point::setAngle(const F32 ang)
{
   setPolar(len(), ang);
}

void Point::setPolar(const F32 l, const F32 ang)
{
   x = cos(ang) * l;
   y = sin(ang) * l;
}

F32 Point::determinant(const Point &p)
{
   return (x * p.y - y * p.x);
}

void Point::scaleFloorDiv(float scaleFactor, float divFactor)
{
   x = (F32) floor(x * scaleFactor + 0.5) * divFactor;
   y = (F32) floor(y * scaleFactor + 0.5) * divFactor;
}

F32 Point::dot(const Point &p) const
{
   return x * p.x + y * p.y;
}

void Point::read(const char **argv)
{
   x = (F32) atof(argv[0]);
   y = (F32) atof(argv[1]);
}


void Point::read(BitStream *stream)
{
   stream->read(&x);
   stream->read(&y);
}


void Point::write(BitStream *stream) const
{
   stream->write(x);
   stream->write(y);
}


string Point::toString() const
{
   return ftos(x) + " " + ftos(y);
}

};	// namespace


namespace Types
{
void read(TNL::BitStream &s, Zap::Point *val)
{
   val->read(&s);
}

void write(TNL::BitStream &s, const Zap::Point &val)
{
   val.write(&s);
}
};
