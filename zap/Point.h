//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _POINT_H_
#define _POINT_H_

#include <string>

// forward declarations
namespace TNL {
   typedef float F32;
   class BitStream;
};


using namespace TNL;
using namespace std;

namespace Zap
{

class Point
{
public:
   F32 x;
   F32 y;

   // Constructors...
   Point();
   Point(const Point& pt);

   // Thanks, Ben & Mike!
   // Templates required to be in headers
   template<class T, class U>
   Point(T in_x, U in_y) { x = static_cast<F32>(in_x); y = static_cast<F32>(in_y); }

   template<class T, class U>
   void set(T ix, U iy) { x = (F32)ix; y = (F32)iy; }

   void set(const Point &pt);
   void set(const Point *pt);

   F32 len() const;		// Distance from (0,0)
   F32 lenSquared() const;
   void normalize();
   void normalize(float newLen);
   void interp(float t, const Point &p1, const Point &p2);

   F32 ATAN2() const;
   F32 distanceTo(const Point &pt) const;
   F32 distSquared(const Point &pt) const;

   F32 angleTo(const Point &p) const;

   Point rotate(F32 ang);

   void setAngle(const F32 ang);
   void setPolar(const F32 l, const F32 ang);

   F32 determinant(const Point &p);

   void scaleFloorDiv(float scaleFactor, float divFactor);

   F32 dot(const Point &p) const;
   void read(const char **argv);
   void read(BitStream *stream);
   void write(BitStream *stream) const;

   string toString() const;
   string toLevelCode() const;
   
   // inlines  need to be in header, too
   inline Point operator+(const Point &pt) const
   {
      return Point (x + pt.x, y + pt.y);
   }

   inline Point operator-(const Point &pt) const
   {
      return Point (x - pt.x, y - pt.y);
   }

   inline Point operator-() const
   {
      return Point(-x, -y);
   }

   inline Point& operator+=(const Point &pt)
   {
      x += pt.x;
      y += pt.y;
      return *this;
   }

   inline Point& operator-=(const Point &pt)
   {
      x -= pt.x;
      y -= pt.y;
      return *this;
   }

   inline Point operator*(const F32 f) const
   {
      return Point (x * f, y * f);
   }

   inline Point operator/(const F32 f)
   {
      return Point (x / f, y / f);
   }

   inline Point& operator*=(const F32 f)
   {
      x *= f;
      y *= f;
      return *this;
   }

   inline Point& operator/=(const F32 f)
   {
      x /= f;
      y /= f;
      return *this;
   }

   inline Point operator*(const Point &pt)
   {
      return Point(x * pt.x, y * pt.y);
   }

   inline Point operator/(const Point &pt)
   {
      return Point(x / pt.x, y / pt.y);
   }

   // Performance equivalent to set
   inline Point& operator=(const Point &pt)
   {
      x = pt.x;
      y = pt.y;
      return *this;
   }

   inline bool operator==(const Point &pt) const
   {
      return x == pt.x && y == pt.y;
   }

   inline bool operator!=(const Point &pt) const
   {
      return x != pt.x || y != pt.y;
   }
}; // class


};	// namespace

namespace Types
{
   extern void read(TNL::BitStream &s, Zap::Point *val);
   extern void write(TNL::BitStream &s, const Zap::Point &val);
}


#endif

