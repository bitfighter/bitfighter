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

#ifndef _POINT_H_
#define _POINT_H_

#include "tnlTypes.h"
#include "tnlVector.h"

#include <math.h>
#include <stdlib.h>

using namespace TNL;

namespace Zap
{

struct Point
{
   F32 x;
   F32 y;

   // Constructors...
   Point() { x = 0; y = 0; }
   Point(const Point& pt) { x = pt.x; y = pt.y; }
   Point(F32 in_x, F32 in_y) { x = in_x; y = in_y; }

   void set(F32 ix, F32 iy) { x = ix; y = iy; }
   void set(const Point &pt) { x = pt.x; y = pt.y; }

   Point operator+(const Point &pt) const { return Point (x + pt.x, y + pt.y); }
   Point operator-(const Point &pt) const { return Point (x - pt.x, y - pt.y); }
   Point operator-() const { return Point(-x, -y); }
   Point &operator+=(const Point &pt) { x += pt.x; y += pt.y; return *this; }
   Point &operator-=(const Point &pt) { x -= pt.x; y -= pt.y; return *this; }

   Point operator*(const F32 f) { return Point (x * f, y * f); }
   Point &operator*=(const F32 f) { x *= f; y *= f; return *this; }
   Point &operator/=(const F32 f) { x /= f; y /= f; return *this; }

   Point operator*(const Point &pt) { return Point(x * pt.x, y * pt.y); }

   Point &operator=(const Point &pt) { x = pt.x; y = pt.y; return *this; }
   bool operator==(const Point &pt) const { return x == pt.x && y == pt.y; }
   bool operator!=(const Point &pt) const { return x != pt.x || y != pt.y; }

   F32 len() const { return (F32) sqrt(x * x + y * y); }		// Distance from (0,0)
   F32 lenSquared() const { return x * x + y * y; }
   void normalize() { F32 l = len(); if(l == 0) { x = 1; y = 0; } else { l = 1 / l; x *= l; y *= l; } }
   void normalize(float newLen) { F32 l = len(); if(l == 0) { x = newLen; y = 0; } else { l = newLen / l; x *= l; y *= l; } }
   F32 ATAN2() const { return atan2(y, x); }
   F32 distanceTo(const Point &pt) const { return sqrt( (x-pt.x) * (x-pt.x) + (y-pt.y) * (y-pt.y) ); }
   F32 distSquared(const Point &pt) const { return((x-pt.x) * (x-pt.x) + (y-pt.y) * (y-pt.y)); }

   F32 angleTo(const Point &p) const { return atan2(p.y-y, p.x-x); }

   Point rotate(F32 ang) { F32 sina = sin(ang); F32 cosa = cos(ang); return Point(x * sina + y * cosa, y * sina - x * cosa); }

   void setAngle(const F32 ang) { setPolar(len(), ang); }
   void setPolar(const F32 l, const F32 ang) { x = cos(ang) * l; y = sin(ang) * l; }

   F32 determinant(const Point &p) { return (x * p.y - y * p.x); }

   void scaleFloorDiv(float scaleFactor, float divFactor)
   {
      x = (F32) floor(x * scaleFactor + 0.5) * divFactor;
      y = (F32) floor(y * scaleFactor + 0.5) * divFactor;
   }
   F32 dot(const Point &p) const { return x * p.x + y * p.y; }
   void read(const char **argv) { x = (F32) atof(argv[0]); y = (F32) atof(argv[1]); }
};


struct Color
{
   float r, g, b;

   Color(const Color &c) { r = c.r; g = c.g; b = c.b; }
   Color(float red = 1, float green = 1, float blue = 1) { r = red; g = green; b = blue; }
   void read(const char **argv) { r = (float) atof(argv[0]); g = (float) atof(argv[1]); b = (float) atof(argv[2]); }

   void interp(float t, const Color &c1, const Color &c2)
   {
      float oneMinusT = 1.0f - t;
      r = c1.r * t + c2.r * oneMinusT;
      g = c1.g * t + c2.g * oneMinusT;
      b = c1.b * t + c2.b * oneMinusT;
   }
   void set(float _r, float _g, float _b) { r = _r; g = _g; b = _b; }
   void set(const Color &c) { r = c.r; g = c.g; b = c.b; }

   Color operator+(const Color &c) const { return Color (r + c.r, g + c.g, b + c.b); }
   Color operator-(const Color &c) const { return Color (r - c.r, g - c.g, b - c.b); }
   Color operator-() const { return Color(-r, -g, -b); }
   Color &operator+=(const Color &c) { r += c.r; g += c.g; b += c.b; return *this; }
   Color &operator-=(const Color &c) { r -= c.r; g -= c.g; b -= c.b; return *this; }

   Color operator*(const float f) { return Color (r * f, g * f, b * f); }
   Color &operator*=(const float f) { r *= f; g *= f; b *= f; return *this; }

   bool operator==(const Color &col) const { return r == col.r && g == col.g && b == col.b; }
   bool operator!=(const Color &col) const { return r != col.r || g != col.g || b != col.b; }

};


extern bool segmentsIntersect(const Point &p1, const Point &p2, const Point &p3, const Point &p4);

struct Rect
{
   typedef float member_type;
   Point min, max;

   Rect() { set(Point(), Point()); }                          // Constuctor
   Rect(const Point &p1, const Point &p2) { set(p1, p2); }    // Constuctor
   Rect(F32 x1, F32 y1, F32 x2, F32 y2) { set(Point(x1, y1), Point(x2, y2)); }  // Constuctor
   Rect(const Point &p, member_type size) {                   // Constuctor, takes point and "radius"
      min.x = p.x - size/2;
      max.x = p.x + size/2;
      min.y = p.y - size/2;
      max.y = p.y + size/2;
   }

   Rect(const TNL::Vector<Point> &p)                          // Construct as a bounding box
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

   Point getCenter() { return (max + min) * 0.5; }

   void set(const Point &p1, const Point &p2)
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

   void set(const TNL::Vector<Point> &p)     // Set to bounding box
   {
      min = p[0];
      max = p[0];

      for(int i = 1; i < p.size(); i++)
         unionPoint(p[i]);
   }

   void set(const Rect &r)
   {
      min.x = r.min.x;
      min.y = r.min.y;

      max.x = r.max.x;
      max.y = r.max.y;
   }

   bool contains(const Point &p)    // Rect contains the point
   {
      return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
   }

   void unionPoint(const Point &p)
   {
      if(p.x < min.x)        min.x = p.x;
      else if(p.x > max.x)   max.x = p.x;
      if(p.y < min.y)        min.y = p.y;
      else if(p.y > max.y)   max.y = p.y;
   }

   void unionRect(const Rect &r)
   {
      if(r.min.x < min.x)    min.x = r.min.x;
      if(r.max.x > max.x)    max.x = r.max.x;
      if(r.min.y < min.y)    min.y = r.min.y;
      if(r.max.y > max.y)    max.y = r.max.y;
   }

   // Does rect interset rect r?
   bool intersects(const Rect &r)
   {
      return min.x < r.max.x && min.y < r.max.y &&
             max.x > r.min.x && max.y > r.min.y;
   }
   
   // Does rect interset or border on rect r?
   bool intersectsOrBorders(const Rect &r)
   {
      F32 littleBit = 0.001f;
      return min.x <= r.max.x + littleBit && min.y <= r.max.y + littleBit &&
             max.x >= r.min.x - littleBit && max.y >= r.min.y - littleBit;
   }


   // Does rect intersect line defined by p1 and p2?
   bool intersects(const Point &p1, const Point &p2)
   {
      return ( segmentsIntersect(p1, p2, Point(min.x, min.y), Point(min.x, max.y)) ||
               segmentsIntersect(p1, p2, Point(min.x, max.y), Point(max.x, max.y)) ||
               segmentsIntersect(p1, p2, Point(max.x, max.y), Point(max.x, min.y)) ||
               segmentsIntersect(p1, p2, Point(max.x, min.y), Point(min.x, min.y)) || 
                                     contains(p1) || contains(p2)
              );
   }

   void expand(Point delta) { min -= delta; max += delta; }

   Point getExtents()
   {
      return max - min;
   }
};	// struct

};	// namespace

#endif

