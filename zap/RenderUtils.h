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

#include "tnlTypes.h"
#include "tnlVector.h"

#ifdef TNL_OS_WIN32 
#  include <windows.h>     // For ARRAYSIZE def
#endif

using namespace TNL;


namespace Zap {


class RenderUtils: RenderManager
{
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

   /// Generic render methods
   ///
   // This method is probably wholly incompatible with GLES 2
   static void glColor(const Color &color, float alpha = 1.0f);

   /// Text-related methods
   ///
private:
   static void doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string);

public:
   // Draw string at given location (normal and formatted versions)
   // Note it is important that x be S32 because for longer strings, they are occasionally drawn starting off-screen
   // to the left, and better to have them partially appear than not appear at all, which will happen if they are U32
   static void drawString(S32 x, S32 y, F32 size, const char *string);
   static void drawString(F32 x, F32 y, F32 size, const char *string);
   static void drawString(F32 x, F32 y, S32 size, const char *string);
   static void drawString(S32 x, S32 y, S32 size, const char *string);
   static void drawString(const Point &left, S32 size, const char *string);

   static void drawStringf(S32 x, S32 y, S32 size, const char *format, ...);
   static void drawStringf(F32 x, F32 y, F32 size, const char *format, ...);
   static void drawStringf(F32 x, F32 y, S32 size, const char *format, ...);

   // Draw strings centered at point
   static S32 drawStringfc(F32 x, F32 y, F32 size, const char *format, ...);
   static S32 drawStringc (F32 x, F32 y, F32 size, const char *string);
   static S32 drawStringc (S32 x, S32 y, S32 size, const char *string);
   static S32 drawStringc(const Point &cen, F32 size, const char *string);

   // Draw strings right-aligned at point
   static S32 drawStringfr(F32 x, F32 y, F32 size, const char *format, ...);
   static S32 drawStringfr(S32 x, S32 y, S32 size, const char *format, ...);
   static S32 drawStringr(S32 x, S32 y, S32 size, const char *string);

   // Draw string and get it's width
   static S32 drawStringAndGetWidth(S32 x, S32 y, S32 size, const char *string);
   static S32 drawStringAndGetWidth(F32 x, F32 y, S32 size, const char *string);
   static S32 drawStringAndGetWidthf(S32 x, S32 y, S32 size, const char *format, ...);
   static S32 drawStringAndGetWidthf(F32 x, F32 y, S32 size, const char *format, ...);


   // Original drawAngleString has a bug in positioning, but fixing it everywhere in the app would be a huge pain, so
   // we've created a new drawAngleString function without the bug, called xx_fixed.  Actual work now moved to doDrawAngleString,
   // which is marked private.  I think all usage of broken function has been removed, and _fixed can be renamed to something better.
   static void drawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string);
   static void drawAngleString_fixed(F32 x, F32 y, F32 size, F32 angle, const char *string);
   static void drawAngleStringf(F32 x, F32 y, F32 size, F32 angle, const char *format, ...);

   // Center text between two points
   static void drawStringf_2pt(Point p1, Point p2, F32 size, F32 vert_offset, const char *format, ...);

   static S32 drawCenteredString_fixed(S32 y, S32 size, const char *str);
   static S32 drawCenteredString(S32 x, S32 y, S32 size, const char *str);
   static S32 drawCenteredString_fixed(S32 x, S32 y, S32 size, const char *str);
   static S32 drawCenteredString_fixed(F32 x, F32 y, S32 size, FontContext fontContext, const char *str);
   static F32 drawCenteredString_fixed(F32 x, F32 y, S32 size, const char *str);

   static F32 drawCenteredString(F32 x, F32 y, S32 size, const char *str);
   static F32 drawCenteredString(F32 x, F32 y, F32 size, const char *str);
   static S32 drawCenteredStringf(S32 y, S32 size, const char *format, ...);
   static S32 drawCenteredStringf(S32 x, S32 y, S32 size, const char *format, ...);

   // Draw text centered on screen (normal and formatted versions)  --> now return starting location
   template <typename T, typename U>
   static F32 drawCenteredString(T y, U size, const char *str)
   {
      return drawCenteredString((F32)DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2.0f, (F32)y, (F32)size, str);
   }

   // Draw text at x,y --> fixes ye olde timee string rendering bug
   template <typename T, typename U, typename V>
   static void drawString_fixed(T x, U y, V size, const char *string)
   {
      drawAngleString(F32(x), F32(y), F32(size), 0, string);
   }

   static void drawCenteredString_highlightKeys(S32 y, S32 size, const string &str, const Color &bodyColor, const Color &keyColor);


   static S32 drawCenteredUnderlinedString(S32 y, S32 size, const char *string);

   static S32 drawStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                const char *leftStr, const char *rightStr);

   static S32 drawStringPair(S32 xpos, S32 ypos, S32 size, FontContext leftContext, FontContext rightContext, const Color &leftColor, const Color &rightColor,
                                const char *leftStr, const char *rightStr);

   static S32 drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                       const char *leftStr, const char *rightStr);
   static S32 drawCenteredStringPair(S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                       const char *leftStr, const char *rightStr);
   static S32 drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, FontContext leftContext, FontContext rightContext, const Color &leftColor, const Color &rightColor,
                                       const char *leftStr, const char *rightStr);

   static S32 getStringPairWidth(S32 size, const char *leftStr, const char *rightStr);

   // Draw text centered in a left or right column (normal and formatted versions)  --> now return starting location
   static S32 drawCenteredString2Col(S32 y, S32 size, bool leftCol, const char *str);
   static S32 drawCenteredString2Colf(S32 y, S32 size, bool leftCol, const char *format, ...);
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

   // Draw 4-column left-justified text
   static void drawString4Col(S32 y, S32 size, U32 col, const char *str);
   static void drawString4Colf(S32 y, S32 size, U32 col, const char *format, ...);

   static void drawTime(S32 x, S32 y, S32 size, S32 timeInMs, const char *prefixString = "");

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
         S32 lineHeight, S32 fontSize, bool draw = true);

   static void drawLetter(char letter, const Point &pos, const Color &color, F32 alpha);


   /// Shape-related methods
   ///
   static void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor);
   static void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, const Color &outlineColor);
   static void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha);
   static void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha, const Color &outlineColor);
   static void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha, const Color &outlineColor, F32 outlineAlpha);

   static void drawHollowRect(const Point &center, S32 width, S32 height);
   static void drawHollowRect(const Point &p1, const Point &p2);

   template<typename T, typename U, typename V, typename W>
   static void drawRect(T x1, U y1, V x2, W y2, S32 mode)
   {
      F32 vertices[] =
      {
            static_cast<F32>(x1), static_cast<F32>(y1),
            static_cast<F32>(x2), static_cast<F32>(y1),
            static_cast<F32>(x2), static_cast<F32>(y2),
            static_cast<F32>(x1), static_cast<F32>(y2)
      };

      mGL->renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, mode);
   }

   //
   template<typename T, typename U, typename V, typename W>
   static void drawFilledRect(T x1, U y1, V x2, W y2)
   {
      drawRect(static_cast<F32>(x1), static_cast<F32>(y1), static_cast<F32>(x2), static_cast<F32>(y2), GLOPT::TriangleFan);
   }


   template<typename T, typename U, typename V, typename W>
   static void drawHollowRect(T x1, U y1, V x2, W y2)
   {
      drawRect(static_cast<F32>(x1), static_cast<F32>(y1), static_cast<F32>(x2), static_cast<F32>(y2), GLOPT::LineLoop);
   }


   template<typename T, typename U, typename V, typename W>
   static void drawHollowRect(T x1, U y1, V x2, W y2, const Color &outlineColor)
   {
      mGL->glColor(outlineColor.r, outlineColor.g, outlineColor.b, 1.0);
      drawHollowRect(x1, y1, x2, y2);
   }

   static void drawFancyBox(F32 xLeft, F32 yTop, F32 xRight, F32 yBottom, F32 cornerInset, S32 mode);

   template<typename T, typename U, typename V, typename W, typename X>
   static void drawFancyBox(T xLeft, U yTop, V xRight, W yBottom, X cornerInset, S32 mode)
   {
      drawFancyBox(F32(xLeft), F32(yTop), F32(xRight), F32(yBottom), F32(cornerInset), mode);
   }


   static void drawHollowFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset);
   static void drawFilledFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, const Color &fillColor, F32 fillAlpha, const Color &borderColor);

   static void drawFilledCircle(const Point &pos, F32 radius);
   static void drawFilledCircle(const Point &pos, F32 radius, const Color &color);
   static void drawFilledSector(const Point &pos, F32 radius, F32 start, F32 end);

   static void drawRoundedRect(const Point &pos, F32 width, F32 height, F32 radius);
   static void drawRoundedRect(const Point &pos, S32 width, S32 height, S32 radius);


   static void drawFilledRoundedRect(const Point &pos, S32 width, S32 height, const Color &fillColor,
                                     const Color &outlineColor, S32 radius, F32 alpha = 1.0);
   static void drawFilledRoundedRect(const Point &pos, F32 width, F32 height, const Color &fillColor,
                                     const Color &outlineColor, F32 radius, F32 alpha = 1.0);

   static void drawArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle);
   static void drawFilledArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle);

   static void drawEllipse(const Point &pos, F32 width, F32 height, F32 angle);
   static void drawEllipse(const Point &pos, S32 width, S32 height, F32 angle);

   static void drawFilledEllipse(const Point &pos, F32 width, F32 height, F32 angle);
   static void drawFilledEllipse(const Point &pos, S32 width, S32 height, F32 angle);

   static void drawFilledEllipseUtil(const Point &pos, F32 width, F32 height, F32 angle, U32 glStyle);

   static void drawPolygon(const Point &pos, S32 sides, F32 radius, F32 angle);
   static void drawPolygon(S32 sides, F32 radius, F32 angle);

   static void drawStar(const Point &pos, S32 points, F32 radius, F32 innerRadius);
   static void drawFilledStar(const Point &pos, S32 points, F32 radius, F32 innerRadius);

   static void drawAngledRay(const Point &center, F32 innerRadius, F32 outerRadius, F32 angle);
   static void drawAngledRayCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 rayCount, F32 offsetAngle);
   static void drawAngledRayArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, S32 rayCount, F32 offsetAngle);
   static void drawDashedArc(const Point &center, F32 radius, F32 centralAngle, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle);
   static void drawDashedHollowCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle);
   static void drawDashedCircle(const Point &center, F32 radius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle);
   static void drawHollowArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, F32 offsetAngle);

   static void drawSquare(const Point &pos, F32 radius, bool filled = false);
   static void drawSquare(const Point &pos, S32 radius, bool filled = false);

   static void drawHollowSquare(const Point &pos, F32 radius, const Color &color);
   static void drawHollowSquare(const Point &pos, F32 radius);

   static void drawFilledSquare(const Point &pos, F32 radius, const Color &color);
   static void drawFilledSquare(const Point &pos, F32 radius);

   static void drawCircle(const Point &center, F32 radius, const Color *color = NULL, F32 alpha = 1.0);
   static void drawCircle(F32 radius, const Color *color = NULL, F32 alpha = 1.0);

   static void drawLine(const Vector<Point> *points);
   static void drawLine(const Vector<Point> *points, const Color &color);

   static void drawHorizLine(S32 x1, S32 x2, S32 y);
   static void drawVertLine (S32 x,  S32 y1, S32 y2);
   static void drawHorizLine(F32 x1, F32 x2, F32 y);
   static void drawVertLine (F32 x,  F32 y1, F32 y2);

   static void drawFadingHorizontalLine(S32 x1, S32 x2, S32 yPos, const Color &color);
};


};

#endif
