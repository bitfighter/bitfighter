//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Color.h"

#include "MathUtils.h"     // For min/max
#include "stringUtils.h"

#include <cmath>

namespace Zap
{

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


Color::Color(string hex)      // Pass by val so we can modify the string if need be
{
   if(hex.length() == 0)
   {
      r = g = b = 0;
   }
   else if(hex.length() == 1)
   {
      r = strtol(hex.c_str(), NULL, 16) / 15.0f;
      g = b = r;
   }
   else if(hex.length() == 2)
   {
      r = strtol(hex.c_str(), NULL, 16) / 255.0f;
      g = b = r;
   }
   else if(hex.length() == 3)
   {
      r = strtol(hex.substr(0, 1).c_str(), NULL, 16) / 15.0f;
      g = strtol(hex.substr(1, 1).c_str(), NULL, 16) / 15.0f;
      b = strtol(hex.substr(2, 1).c_str(), NULL, 16) / 15.0f;
   }
   else
   {
      if(hex.length() < 6)
         hex.append(6 - hex.length(), '0');

      r = strtol(hex.substr(0, 2).c_str(), NULL, 16) / 255.0f;
      g = strtol(hex.substr(2, 2).c_str(), NULL, 16) / 255.0f;
      b = strtol(hex.substr(4, 2).c_str(), NULL, 16) / 255.0f;
   }
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
      F32 p;

      p = (F32)atof(list[0].c_str());
      r = CLAMP(p, 0, 1);

      p = (F32)atof(list[1].c_str());
      g = CLAMP(p, 0, 1);

      p = (F32)atof(list[2].c_str());
      b = CLAMP(p, 0, 1);
   }
}


Color Color::iniValToColor(const string &s)
{
   // If value begins with a "#", then we'll treat it as a hex
   if(s[0] == '#')
      return Color(s.substr(1));

   Color color(0.0f);
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


NVGcolor Color::toNvg() const
{
   return nvgRGBf(r, g, b);
}


NVGcolor Color::toNvg(F32 alpha) const
{
   return nvgRGBAf(r, g, b, alpha);
}



U32 Color::toU32() const
{ 
   return U32(r * 0xFF) | U32(g * 0xFF)<<8 | U32(b * 0xFF)<<16; 
}


static const F32 Pr = .299;
static const F32 Pg = .587;
static const F32 Pb = .114;

//  public domain function by Darel Rex Finley, 2006
//
//  This function expects the passed-in values to be on a scale
//  of 0 to 1, and uses that same scale for the return values.
//
//  See description/examples at http://alienryderflex.com/hsp.html
//
// I'm leaving the formatting as-is (except the signature) because
// the code author wrote a whole set of HTML pages to defend it.
// See http://alienryderflex.com/brace_style/brace01.html
void RGBtoHSP(F32  R, F32  G, F32  B,
              F32 *H, F32 *S, F32 *P) {

   //  Calculate the Perceived brightness.
   *P=sqrt(R*R*Pr+G*G*Pg+B*B*Pb);

   //  Calculate the Hue and Saturation.  (This part works
   //  the same way as in the HSV/B and HSL systems???.)
   if      (R==G && R==B) {
      *H=0.; *S=0.; return; }
   if      (R>=G && R>=B) {   //  R is largest
      if    (B>=G) {
         *H=6./6.-1./6.*(B-G)/(R-G); *S=1.-G/R; }
      else         {
         *H=0./6.+1./6.*(G-B)/(R-B); *S=1.-B/R; }}
   else if (G>=R && G>=B) {   //  G is largest
      if    (R>=B) {
         *H=2./6.-1./6.*(R-B)/(G-B); *S=1.-B/G; }
      else         {
         *H=2./6.+1./6.*(B-R)/(G-R); *S=1.-R/G; }}
   else                   {   //  B is largest
      if    (G>=R) {
         *H=4./6.-1./6.*(G-R)/(B-R); *S=1.-R/B; }
      else         {
         *H=4./6.+1./6.*(R-G)/(B-G); *S=1.-G/B; }}
}


//  This function expects the passed-in values to be on a scale
//  of 0 to 1, and uses that same scale for the return values.
//
//  Note that some combinations of HSP, even if in the scale
//  0-1, may return RGB values that exceed a value of 1.  For
//  example, if you pass in the HSP color 0,1,1, the result
//  will be the RGB color 2.037,0,0.
//
//  See description/examples at http://alienryderflex.com/hsp.html
void HSPtoRGB(F32  H, F32  S, F32  P,
              F32 *R, F32 *G, F32 *B) {

   F32  part, minOverMax=1.-S ;

   if (minOverMax>0.) {
      if      ( H<1./6.) {   //  R>G>B
         H= 6.*( H-0./6.); part=1.+H*(1./minOverMax-1.);
         *B=P/sqrt(Pr/minOverMax/minOverMax+Pg*part*part+Pb);
         *R=(*B)/minOverMax; *G=(*B)+H*((*R)-(*B)); }
      else if ( H<2./6.) {   //  G>R>B
         H= 6.*(-H+2./6.); part=1.+H*(1./minOverMax-1.);
         *B=P/sqrt(Pg/minOverMax/minOverMax+Pr*part*part+Pb);
         *G=(*B)/minOverMax; *R=(*B)+H*((*G)-(*B)); }
      else if ( H<3./6.) {   //  G>B>R
         H= 6.*( H-2./6.); part=1.+H*(1./minOverMax-1.);
         *R=P/sqrt(Pg/minOverMax/minOverMax+Pb*part*part+Pr);
         *G=(*R)/minOverMax; *B=(*R)+H*((*G)-(*R)); }
      else if ( H<4./6.) {   //  B>G>R
         H= 6.*(-H+4./6.); part=1.+H*(1./minOverMax-1.);
         *R=P/sqrt(Pb/minOverMax/minOverMax+Pg*part*part+Pr);
         *B=(*R)/minOverMax; *G=(*R)+H*((*B)-(*R)); }
      else if ( H<5./6.) {   //  B>R>G
         H= 6.*( H-4./6.); part=1.+H*(1./minOverMax-1.);
         *G=P/sqrt(Pb/minOverMax/minOverMax+Pr*part*part+Pg);
         *B=(*G)/minOverMax; *R=(*G)+H*((*B)-(*G)); }
      else               {   //  R>B>G
         H= 6.*(-H+6./6.); part=1.+H*(1./minOverMax-1.);
         *G=P/sqrt(Pr/minOverMax/minOverMax+Pb*part*part+Pg);
         *R=(*G)/minOverMax; *B=(*G)+H*((*R)-(*G)); }}
   else {
      if      ( H<1./6.) {   //  R>G>B
         H= 6.*( H-0./6.); *R=sqrt(P*P/(Pr+Pg*H*H)); *G=(*R)*H; *B=0.; }
      else if ( H<2./6.) {   //  G>R>B
         H= 6.*(-H+2./6.); *G=sqrt(P*P/(Pg+Pr*H*H)); *R=(*G)*H; *B=0.; }
      else if ( H<3./6.) {   //  G>B>R
         H= 6.*( H-2./6.); *G=sqrt(P*P/(Pg+Pb*H*H)); *B=(*G)*H; *R=0.; }
      else if ( H<4./6.) {   //  B>G>R
         H= 6.*(-H+4./6.); *B=sqrt(P*P/(Pb+Pg*H*H)); *G=(*B)*H; *R=0.; }
      else if ( H<5./6.) {   //  B>R>G
         H= 6.*( H-4./6.); *B=sqrt(P*P/(Pb+Pr*H*H)); *R=(*B)*H; *G=0.; }
      else               {   //  R>B>G
         H= 6.*(-H+6./6.); *R=sqrt(P*P/(Pr+Pb*H*H)); *B=(*R)*H; *G=0.; }}
}


void Color::ensureMinimumBrightness()
{
   F32 H, S, P;
   RGBtoHSP(r, g, b, &H, &S, &P);

   F32 minBrightness = .20;

   if(P < minBrightness)
      HSPtoRGB(H, S, minBrightness, &r, &g, &b);
}

//RangedU32<0, 0xFFFFFF> toRangedU32() { return RangedU32<0, 0xFFFFFF>(toU32()); }

};	// namespace

