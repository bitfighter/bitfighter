//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Color.h"
#include "stringUtils.h"

namespace Zap
{

// Constructors
Color::Color(const Color &c)
{
   r = c.r;
   g = c.g;
   b = c.b;
}


Color::Color(const Color *c)
{
   // Protect against NULLs
   if(!c)
      return;

   r = c->r;
   g = c->g;
   b = c->b;
}


Color::Color(float grayScale)
{
   r = grayScale;
   g = grayScale;
   b = grayScale;
}


Color::Color(double grayScale)
{
   r = (F32)grayScale;
   g = (F32)grayScale;
   b = (F32)grayScale;
}


Color::Color(U32 rgbInt)
{
   r = F32(U8(rgbInt))       / 255;
   g = F32(U8(rgbInt >> 8))  / 255;
   b = F32(U8(rgbInt >> 16)) / 255;
}


Color::Color(const string &hex)
{
   if(hex.length() != 6)
   {
      r = 0;
      g = 0;
      b = 0;

      return;
   }

   r = strtol(hex.substr(0,2).c_str(), NULL, 16) / 255.0f;
   g = strtol(hex.substr(2,2).c_str(), NULL, 16) / 255.0f;
   b = strtol(hex.substr(4,2).c_str(), NULL, 16) / 255.0f;
}


void Color::read(const char **argv) 
{ 
   r = (F32) atof(argv[0]); 
   g = (F32) atof(argv[1]); 
   b = (F32) atof(argv[2]); 

}


void Color::interp(float t, const Color &c1, const Color &c2)
{
   float oneMinusT = 1.0f - t;
   r = c1.r * t  +  c2.r * oneMinusT;
   g = c1.g * t  +  c2.g * oneMinusT;
   b = c1.b * t  +  c2.b * oneMinusT;
}


void Color::set(const Color &c) { r = c.r;  g = c.g;  b = c.b;  }
void Color::set(const Color *c) { r = c->r; g = c->g; b = c->b; }

void Color::set(const string &s)
{
   Vector<string> list;
   parseString(s, list, ' ');

   if(list.size() < 3)
      parseString(s, list, ',');

   if(list.size() >= 3)
   {
      r = (F32)atof(list[0].c_str());
      g = (F32)atof(list[1].c_str());
      b = (F32)atof(list[2].c_str());
   }
}


Color Color::iniValToColor(const string &s)
{
   // If value begins with a "#", then we'll treat it as a hex
   if(s[0] == '#')
      return Color(s.substr(1));

   Color color;
   color.set(s);

   return color;
}


string Color::toRGBString() const 
{ 
   return ftos(r, 3) + " " + ftos(g, 3) + " " + ftos(b, 3); 
}


string Color::toHexString() const 
{ 
   char c[7]; 
   dSprintf(c, sizeof(c), "%.6X", U32(r * 0xFF) << 24 >> 8 | U32(g * 0xFF) << 24 >> 16 | (U32(b * 0xFF) & 0xFF));
   return c; 
}


string Color::toHexStringForIni() const 
{
   return string("#") + toHexString();
}


U32 Color::toU32() const
{ 
   return U32(r * 0xFF) | U32(g * 0xFF)<<8 | U32(b * 0xFF)<<16; 
}


//RangedU32<0, 0xFFFFFF> toRangedU32() { return RangedU32<0, 0xFFFFFF>(toU32()); }

};	// namespace

