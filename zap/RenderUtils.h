//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _RENDER_UTILS_H_
#define _RENDER_UTILS_H_

#ifdef ZAP_DEDICATED
#  error "RenderUtils.h shouldn't be included in dedicated build"
#endif

#include "RenderManager.h"
#include "Color.h"
#include "DisplayManager.h"
#include "FontContextEnum.h"
#include "Point.h"

#include "../nanovg/nanovg.h"

#include "tnlTypes.h"
#include "tnlVector.h"

#ifdef TNL_OS_WIN32 
#  include <windows.h>     // For ARRAYSIZE def
#endif

using namespace TNL;


namespace Zap {


class RenderUtils: RenderManager
{
   static char buffer[2048];     // Reusable buffer
#  define makeBuffer    va_list args; va_start(args, format); vsnprintf(buffer, sizeof(buffer), format, args); va_end(args);

public:
   RenderUtils();
   virtual ~RenderUtils();

   static F32 LINE_WIDTH_1;
   static F32 LINE_WIDTH_2;
   static F32 LINE_WIDTH_3;
   static F32 LINE_WIDTH_4;
   static F32 DEFAULT_LINE_WIDTH;

   static const S32 NUM_CIRCLE_SIDES;
   static const F32  CIRCLE_SIDE_THETA;

   static const U8 UR = 1;
   static const U8 LL = 2;

   /// Text-related methods
   ///
private:
   static void doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string);
   static S32 doDrawStringr(S32 x, S32 y, S32 size, const char *string);

public:
   // Draw string at given location (normal and formatted versions)
   // Note it is important that x be S32 because for longer strings, they are occasionally drawn starting off-screen
   // to the left, and better to have them partially appear than not appear at all, which will happen if they are U32
   static void drawString(F32 x, F32 y, F32 size, const char *string);
   static void drawString(F32 x, F32 y, S32 size, const char *string);
   static void drawString(S32 x, S32 y, S32 size, const char *string);



   static void drawStringf(S32 x, S32 y, S32 size, const char *format, ...);
   static void drawStringf(F32 x, F32 y, F32 size, const char *format, ...);
   static void drawStringf(F32 x, F32 y, S32 size, const char *format, ...);


   // Draw strings centered at point
   static S32 drawStringfc(F32 x, F32 y, F32 size, const char *format, ...);
   static S32 drawStringfc(F32 x, F32 y, F32 size, const Color &color, F32 alpha, const char *format, ...);


   // Draw strings right-aligned at point
   //static S32 drawStringfr(F32 x, F32 y, F32 size, const char *format, ...);
   static S32 drawStringfr(S32 x, S32 y, S32 size, const char *format, ...);
   static S32 drawStringfr(S32 x, S32 y, S32 size, const Color &color, const char *format, ...);
   static S32 drawStringfr_fixed(S32 x, S32 y, S32 size, const Color &color, const char *format, ...);

   static S32 drawStringr(S32 x, S32 y, S32 size, const Color &color, const char *string);

   // Draw string and get it's width
   static S32 drawStringAndGetWidth(S32 x, S32 y, S32 size, const char *string);
   static S32 drawStringAndGetWidth(S32 x, S32 y, S32 size, const Color &color, const char *string);

   static S32 drawStringAndGetWidth(F32 x, F32 y, S32 size, const char *string);
   static S32 drawStringAndGetWidthf(S32 x, S32 y, S32 size, const char *format, ...);
   static S32 drawStringAndGetWidthf(F32 x, F32 y, S32 size, const char *format, ...);
   static S32 drawStringAndGetWidthf(F32 x, F32 y, S32 size, const Color &color, const char *format, ...);


   template <typename T, typename U, typename V>
   static S32 drawStringAndGetWidth_fixed(T x, U y, V size, const Color &color, F32 alpha, const char *string)
   {
      drawString_fixed((F32)x, (F32)y, (F32)size, color, alpha, string);
      return getStringWidth(size, string);
   }


   template <typename T, typename U, typename V>
   static S32 drawStringAndGetWidth_fixed(T x, U y, V size, const Color &color, const char *string)
   {
      drawString_fixed(x, y, size, color, string);
      return getStringWidth(size, string);
   }


   template <typename T, typename U, typename V>
   static void drawStringf_fixed(T x, U y, V size, const Color &color, const char *format, ...)
   {
      makeBuffer;
      drawString_fixed(x, y, size, color, buffer);
   }


   template <typename T, typename U, typename V>
   static void drawStringf_fixed(T x, U y, V size, const Color &color, F32 alpha, const char *format, ...)
   {
      makeBuffer;
      drawString_fixed(x, y, size, color, alpha, buffer);
   }


   static S32 drawStringAndGetWidth_fixed(F32 x, F32 y, S32 size, const char *string);

   template <typename T, typename U, typename V>
   static S32 drawStringAndGetWidthf_fixed(T x, U y, V size, const Color &color, const char *format, ...)
   {
      makeBuffer;
      drawString_fixed(x, y + size, size, color, buffer);
      return getStringWidth(size, buffer);
   }



   // Original drawAngleString has a bug in positioning, but fixing it everywhere in the app would be a huge pain, so
   // we've created a new drawAngleString function without the bug, called xx_fixed.  Actual work now moved to doDrawAngleString,
   // which is marked private.  I think all usage of broken function has been removed, and _fixed can be renamed to something better.
   static void drawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string);
   static void drawAngleString(F32 x, F32 y, F32 size, const Color &color, F32 angle, const char *string);
   static void drawAngleString(F32 x, F32 y, F32 size, const Color &color, F32 alpha, F32 angle, const char *string);
   //static void drawAngleStringf(F32 x, F32 y, F32 size, F32 angle, const char *format, ...);

   // Center text between two points
   //static void drawStringf_2pt(Point p1, Point p2, F32 size, F32 vert_offset, const char *format, ...);

   static S32 drawCenteredString_fixed(S32 y, S32 size, const char *str);
   static S32 drawCenteredString_fixed(S32 y, S32 size, const Color &color, const char *str);
   static S32 drawCenteredString_fixed(S32 y, S32 size, const Color &color, F32 alpha, const char *str);

   static S32 drawCenteredString(S32 x, S32 y, S32 size, const char *str);
   static S32 drawCenteredString_fixed(F32 x, F32 y, S32 size, FontContext fontContext, const Color &color, const char *str);
   static S32 drawCenteredString_fixed(S32 x, S32 y, S32 size, const Color &color, const char *str);
   static S32 drawCenteredString_fixed(S32 x, S32 y, S32 size, const Color &color, F32 alpha, const char *str);

   template <typename T, typename U, typename V>
   static F32 drawCenteredString_fixed(T x, U y, V size, const char *string)
   {
      F32 xpos = x - getStringWidth((F32)size, string) / 2;
      drawString_fixed(xpos, y, size, string);
      return xpos;
   }


   //static F32 drawCenteredString(F32 x, F32 y, S32 size, const char *str);
   //static F32 drawCenteredString(F32 x, F32 y, F32 size, const char *str);
   //static S32 drawCenteredStringf(S32 y, S32 size, const char *format, ...);
   static S32 drawCenteredStringf_fixed(S32 y, S32 size, const Color &color, const char *format, ...);
   static S32 drawCenteredStringf_fixed(S32 y, S32 size, const Color &color, F32 alpha, const char *format, ...);
   //static S32 drawCenteredStringf(S32 x, S32 y, S32 size, const char *format, ...);
   static S32 drawCenteredStringf_fixed(S32 x, S32 y, S32 size, const Color &color, const char *format, ...);

   // Draw text centered on screen (normal and formatted versions)  --> now return starting location
   //template <typename T, typename U>
   //static F32 drawCenteredString(T y, U size, const char *str)
   //{
   //   return drawCenteredString((F32)DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2.0f, (F32)y, (F32)size, str);
   //}

   // Draw text at x,y --> fixes ye olde timee string rendering bug
   template <typename T, typename U, typename V>
   static void drawString_fixed(T x, U y, V size, const char *string)
   {
      drawAngleString(F32(x), F32(y), F32(size), 0, string);
   }

   template <typename T, typename U, typename V>
   static void drawString_fixed(T x, U y, V size, const Color &color, const char *string)
   {
      drawAngleString(F32(x), F32(y), F32(size), color, 0, string);
   }

   template <typename T, typename U, typename V>
   static void drawString_fixed(T x, U y, V size, const Color &color, F32 alpha, const char *string)
   {
      drawAngleString(F32(x), F32(y), F32(size), color, alpha, 0, string);
   }


   // fixed
   template <typename T, typename U, typename V>
   static S32 drawStringc(T x, U y, V size, const Color &color, const char *string)
   {
      F32 len = getStringWidth((F32)size, string);
      drawAngleString((F32)x - len / 2, (F32)y, (F32)size, color, 0, string);

      return (S32)len;
   }

   // fixed
   template <typename T, typename U, typename V>
   static S32 drawStringc(T x, U y, V size, const Color &color, F32 alpha, const char *string)
   {
      F32 len = getStringWidth((F32)size, string);
      drawAngleString((F32)x - len / 2, (F32)y, (F32)size, color, alpha, 0, string);

      return (S32)len;
   }

   // fixed
   template <typename T, typename U, typename V>
   static S32 drawStringc(T x, U y, V size, const char *string)
   {
      F32 len = getStringWidth((F32)size, string);
      drawAngleString((F32)x - len / 2.0f, (F32)y, (F32)size, 0, string);

      return (S32)len;
   }

   template <typename U>
   static S32 drawStringc(const Point &cen, U size, const Color &color, const char *string)
   {
      return drawStringc(cen.x, cen.y, (F32)size, color, string);
   }

   template <typename U>
   static S32 drawStringc(const Point &cen, U size, const Color &color, F32 alpha, const char *string)
   {
      return drawStringc(cen.x, cen.y, size, color, alpha, string);
   }


   static S32 drawStringc(const Point &cen, F32 size, const char *string);


   //static void drawCenteredString_highlightKeys(S32 y, S32 size, const string &str, const Color &bodyColor, const Color &keyColor);


   static S32 drawCenteredUnderlinedString(S32 y, S32 size, const Color &color, const char *string);

   static S32 drawStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                const char *leftStr, const char *rightStr);

   static S32 drawStringPair(S32 xpos, S32 ypos, S32 size, FontContext leftContext, FontContext rightContext, const Color &leftColor, const Color &rightColor,
                                const char *leftStr, const char *rightStr);

   static S32 drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                       const char *leftStr, const char *rightStr);
   static S32 drawCenteredStringPair(S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                       const char *leftStr, const char *rightStr);
   static S32 drawCenteredStringPair_fixed(S32 xpos, S32 ypos, S32 size, FontContext leftContext, FontContext rightContext, const Color &leftColor, const Color &rightColor,
                                           const char *leftStr, const char *rightStr);

   static S32 getStringPairWidth(S32 size, const char *leftStr, const char *rightStr);

   // Draw text centered in a left or right column (normal and formatted versions)  --> now return starting location
   static S32 drawCenteredString2Col(S32 y, S32 size, const Color &color, bool leftCol, const char *str);
   static S32 drawCenteredString2Colf(S32 y, S32 size, const Color &color, bool leftCol, const char *format, ...);
   static S32 drawCenteredStringPair2Colf(S32 y, S32 size, bool leftCol, const char *left, const char *right, ...);
   static S32 drawCenteredStringPair2Colf(S32 y, S32 size, bool leftCol, const Color &leftColor, const Color &rightColor,
         const char *left, const char *right, ...);

   static S32 drawCenteredStringPair2Col(S32 y, S32 size, bool leftCol, const Color &leftColor, const Color &rightColor,
         const char *left, const char *right);

   // Get info about where text will be draw
   static S32 get2ColStartingPos(bool leftCol);
   static S32 getCenteredStringStartingPos(S32 size, const char *string);
   static S32 getCenteredStringStartingPosf(S32 size, const char *format, ...);
   static S32 getCenteredString2ColStartingPos(S32 size, bool leftCol, const char *string);
   static S32 getCenteredString2ColStartingPosf(S32 size, bool leftCol, const char *format, ...);

   static void drawTime(S32 x, S32 y, S32 size, const Color &color, S32 timeInMs, const char *prefixString = "");

   // Return string rendering width (normal and formatted versions)
   static S32 getStringWidth(FontContext context, S32 size, const char *string);
   static F32 getStringWidth(FontContext context, F32 size, const char *string);

   static S32 getStringWidth(S32 size, const string &str);

   static F32 getStringWidth(F32 size, const char *str);
   static S32 getStringWidth(S32 size, const char *str);

   static F32 getStringWidthf(F32 size, const char *format, ...);
   static S32 getStringWidthf(S32 size, const char *format, ...);

   static S32 getStringPairWidth(S32 size, FontContext leftContext, FontContext rightContext, const char* leftStr, const char* rightStr);

   static void wrapString(const string &str, S32 wrapWidth, S32 fontSize, FontContext context, Vector<string> &lines);
   //Vector<string> wrapString(const string &str, S32 width, S32 fontSize, const string indentPrefix = "");

   static U32 drawWrapText(const string &msg, S32 xpos, S32 ypos, S32 width, S32 ypos_end,
                           S32 lineHeight, S32 fontSize, const Color &color, bool draw);

   static void drawLetter(char letter, const Point &pos, const Color &color, F32 alpha);


   /// Shape-related methods
   ///
   static void drawFilledRect(S32 x, S32 y, S32 w, S32 h, const Color &fillColor, const Color &outlineColor);
   static void drawFilledRect(S32 x, S32 y, S32 w, S32 h, const Color &fillColor, F32 fillAlpha, const Color &outlineColor);
   static void drawFilledRect(S32 x, S32 y, S32 w, S32 h, const Color &fillColor, F32 fillAlpha, const Color &outlineColor, F32 outlineAlpha);

   static void drawHollowRect(const Point &center, S32 width, S32 height, const Color &color, F32 alpha = 1.0);
   static void drawHollowRect(const Point &p1, const Point &p2, const Color &color, F32 alpha = 1.0);

   template<typename T, typename U, typename V, typename W>
   static void drawRect(T x, U y, V w, W h, const Color &color, F32 alpha = 1.0)
   {
      nvgBeginPath(nvg);
      nvgRect(nvg, static_cast<F32>(x), static_cast<F32>(y),
            static_cast<F32>(w), static_cast<F32>(h));
      nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
      nvgStroke(nvg);
   }

   template<typename T, typename U, typename V, typename W>
   static void drawFilledRect(T x, U y, V w, W h, const Color &color, F32 alpha = 1.0)
   {
      nvgBeginPath(nvg);
      nvgRect(nvg, static_cast<F32>(x), static_cast<F32>(y),
            static_cast<F32>(w), static_cast<F32>(h));
      nvgFillColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
      nvgFill(nvg);
   }


   template<typename T, typename U, typename V, typename W>
   static void drawHollowRect(T x, U y, V w, W h, const Color &color, F32 alpha = 1.0)
   {
      drawRect(static_cast<F32>(x), static_cast<F32>(y), static_cast<F32>(w), static_cast<F32>(h), color, alpha);
   }

   static void drawFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, U8 corners, const Color &color, F32 alpha = 1.0);
   static void drawFancyBox(F32 xLeft, F32 yTop, F32 xRight, F32 yBottom, F32 cornerInset, U8 corners, const Color &color, F32 alpha = 1.0);

   template<typename T, typename U, typename V, typename W, typename X>
   static void drawFancyBox(T xLeft, U yTop, V xRight, W yBottom, X cornerInset, const Color &color, F32 alpha = 1.0)
   {
      drawFancyBox(F32(xLeft), F32(yTop), F32(xRight), F32(yBottom), F32(cornerInset), LL|UR, color, alpha);
   }

   static void drawFilledFancyBox(F32 xLeft, F32 yTop, F32 xRight, F32 yBottom, F32 cornerInset, U8 corners, const Color &color, F32 alpha);

   static void drawHollowFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, const Color &color, F32 alpha = 1.0);
   static void drawFilledFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset,  
                                  const Color &fillColor, F32 fillAlpha, const Color &borderColor);
   static void drawHollowFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, U8 corners, const Color &color, F32 alpha = 1.0);
   static void drawFilledFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, U8 corners, 
                                  const Color &fillColor, F32 fillAlpha, const Color &borderColor);

   static void drawFilledCircle(const Point &pos, F32 radius, const Color &color, F32 alpha = 1.0);

   static void drawRoundedRect(const Point &pos, F32 width, F32 height, F32 radius, const Color &color, F32 alpha = 1.0);
   static void drawRoundedRect(const Point &pos, S32 width, S32 height, S32 radius, const Color &color, F32 alpha = 1.0);


   static void drawFilledRoundedRect(const Point &pos, S32 width, S32 height, const Color &fillColor,
                                     const Color &outlineColor, S32 radius, F32 alpha = 1.0);
   static void drawFilledRoundedRect(const Point &pos, F32 width, F32 height, const Color &fillColor,
                                     const Color &outlineColor, F32 radius, F32 alpha = 1.0);

   static void drawArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle, const Color &color, F32 alpha = 1.0);
   static void drawFilledArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle, const Color &color, F32 alpha = 1.0);

   static void drawEllipse(const Point &pos, F32 radiusX, F32 radiusY, const Color &color, F32 alpha = 1.0);
   static void drawEllipse(const Point &pos, S32 radiusX, S32 radiusY, const Color &color, F32 alpha = 1.0);

   static void drawFilledEllipse(const Point &pos, F32 radiusX, F32 radiusY, const Color &color, F32 alpha = 1.0);
   static void drawFilledEllipse(const Point &pos, S32 radiusX, S32 radiusY, const Color &color, F32 alpha = 1.0);

   static void drawPolygon(                  S32 sides, F32 radius, F32 angle, const Color &color, F32 alpha = 1.0);
   static void drawPolygon(const Point &pos, S32 sides, F32 radius, F32 angle, const Color &color, F32 alpha = 1.0);

   static void drawStar(const Point &pos, S32 points, F32 radius, F32 innerRadius, const Color &color, F32 alpha = 1.0);
   //static void drawFilledStar(const Point &pos, S32 points, F32 radius, F32 innerRadius);

   static void drawAngledRay(const Point &center, F32 innerRadius, F32 outerRadius, F32 angle, const Color &color);
   static void drawAngledRayCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 rayCount, F32 offsetAngle, const Color &color);
   static void drawAngledRayArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, S32 rayCount, F32 offsetAngle, const Color &color);
   static void drawDashedArc(const Point &center, F32 radius, F32 centralAngle, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle, const Color &color, F32 alpha = 1.0);
   static void drawDashedHollowCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle, const Color &color);
   static void drawDashedCircle(const Point &center, F32 radius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle, const Color &color, F32 alpha = 1.0);
   static void drawHollowArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, F32 offsetAngle, const Color &color);

   static void drawSquare(const Point &pos, F32 radius, const Color &color, bool filled = false);
   static void drawSquare(const Point &pos, F32 radius, const Color &color, F32 alpha, bool filled = false);
   static void drawSquare(const Point &pos, S32 radius, const Color &color, F32 alpha = 1.0, bool filled = false);

   static void drawHollowSquare(const Point &pos, F32 radius, const Color &color, F32 alpha = 1.0);
   static void drawFilledSquare(const Point &pos, F32 radius, const Color &color, F32 alpha = 1.0);

   static void drawCircle(const Point &center, F32 radius, const Color &color, F32 alpha = 1.0);
   static void drawCircle(F32 radius, const Color &color, F32 alpha = 1.0);

   static void drawLine(const Vector<Point> *points, const Color &color, F32 alpha = 1.0);
   static void drawLine(F32 x1, F32 x2, F32 y1, F32 y2, const Color &color, F32 alpha = 1.0);
   static void drawLineStrip(const F32 *points, U32 pointCount, const Color &color, F32 alpha = 1.0);
   static void drawLineStrip(const Vector<Point> *points, const Color &color, F32 alpha = 1.0);
   static void drawLineLoop(const S16 *points, U32 pointCount, const Color &color, F32 alpha = 1.0);
   static void drawLineLoop(const F32 *points, U32 pointCount, const Color &color, F32 alpha = 1.0);
   static void drawLineLoop(const Vector<Point> *points, const Color &color, F32 alpha = 1.0);
   static void drawLines(const F32 *points, U32 pointCount, const Color &color, F32 alpha = 1.0);
   static void drawLines(const Vector<Point> *points, const Color &color, F32 alpha = 1.0);

   static void drawLineGradient(F32 x1, F32 y1, F32 x2, F32 y2,
         const Color &color1, F32 alpha1, const Color &color2, F32 alpha2);

   static void drawRectHorizGradient(F32 x1, F32 y1, F32 w, F32 h,
         const Color &color1, F32 alpha1, const Color &color2, F32 alpha2);
   static void drawRectVertGradient(F32 x, F32 y, F32 w, F32 h,
         const Color &color1, F32 alpha1, const Color &color2, F32 alpha2);

   static void drawFilledLineLoop(const F32 *points, U32 pointCount, const Color &color, F32 alpha = 1.0);
   static void drawFilledLineLoop(const Vector<Point> *points, const Color &color, F32 alpha = 1.0);

   static void drawPoints(const F32 *points, U32 pointCount, const Color &color, F32 alpha = 1.0);
   static void drawPoints(const F32 *points, U32 pointCount, const Color &color, F32 alpha,
         F32 pointSize, F32 scale, F32 translateX, F32 translateY, F32 rotate);

   static void drawPointsColorArray(const F32 *points, const F32 *colors, U32 count, S32 stride = 0);
   static void drawLinesColorArray(const F32 *vertices, const F32 *colors, U32 count, S32 stride = 0);
   static void drawLineStripColorArray(const F32 *vertices, const F32 *colors, U32 count, S32 stride = 0);

   static void drawHorizLine(                S32 y, const Color &color, F32 alpha = 1.0);
   static void drawHorizLine(                U32 y, const Color &color, F32 alpha = 1.0);
   static void drawHorizLine(                F32 y, const Color &color, F32 alpha = 1.0);
   static void drawHorizLine(S32 x1, S32 x2, S32 y, const Color &color, F32 alpha = 1.0);
   static void drawHorizLine(F32 x1, F32 x2, F32 y, const Color &color, F32 alpha = 1.0); 
   
   static void drawVertLine (S32 x,                 const Color &color, F32 alpha = 1.0);
   static void drawVertLine (S32 x, S32 y1, S32 y2, const Color &color, F32 alpha = 1.0);
   static void drawVertLine (F32 x, F32 y1, F32 y2, const Color &color, F32 alpha = 1.0);

   static void setDefaultLineWidth(F32 width);
   static void lineWidth(F32 width);
};


};

#endif
