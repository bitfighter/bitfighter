//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "RenderUtils.h"

#include "UI.h"
#include "DisplayManager.h"

#include "Colors.h"
#include "FontManager.h"
#include "GeomUtils.h"
#include "MathUtils.h"     // For MIN/MAX def
#include "RenderManager.h"
#include "stringUtils.h"

#include "glinc.h"

#include <stdarg.h>        // For va_args
#include <stdio.h>         // For vsnprintf


namespace Zap {


// static members
F32 RenderUtils::LINE_WIDTH_1 = 1.0f;
F32 RenderUtils::DEFAULT_LINE_WIDTH = 2.0f;
F32 RenderUtils::LINE_WIDTH_3 = 3.0f;
F32 RenderUtils::LINE_WIDTH_4 = 4.0f;

char RenderUtils::buffer[2048];     // Reusable buffer


const S32 RenderUtils::NUM_CIRCLE_SIDES = 32;
const F32 RenderUtils::CIRCLE_SIDE_THETA = Float2Pi / NUM_CIRCLE_SIDES;


RenderUtils::RenderUtils()
{
   // Do nothing
}


RenderUtils::~RenderUtils()
{
   // Do nothing
}


// All text rendering flows through here
void RenderUtils::doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string)
{
      FontManager::renderString(x, y, size, angle, string);
}


// Same but accepts S32 args
//void doDrawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string, bool autoLineWidth)
//{
//   doDrawAngleString(F32(x), F32(y), size, angle, string, autoLineWidth);
//}


//// Center text between two points, adjust angle so it's always right-side-up
//void RenderUtils::drawStringf_2pt(Point p1, Point p2, F32 size, F32 vert_offset, const char *format, ...)
//{
//   F32 ang = p1.angleTo(p2);
//
//   // Make sure text is right-side-up
//   if(ang < -FloatHalfPi || ang > FloatHalfPi)
//   {
//      Point temp = p2;
//      p2 = p1;
//      p1 = temp;
//      ang = p1.angleTo(p2);
//   }
//
//   F32 cosang = cos(ang);
//   F32 sinang = sin(ang);
//
//   makeBuffer;
//   F32 len = getStringWidthf(size, buffer);
//   F32 offset = (p1.distanceTo(p2) - len) / 2;
//
//   doDrawAngleString(p1.x + cosang * offset + sinang * (size + vert_offset), 
//                     p1.y + sinang * offset - cosang * (size + vert_offset), 
//                     size, 
//                     ang, 
//                     buffer);
//}


//// New, fixed version
//void RenderUtils::drawAngleStringf(F32 x, F32 y, F32 size, F32 angle, const char *format, ...)
//{
//   makeBuffer;
//   doDrawAngleString(x, y, size, angle, buffer);
//}


// New, fixed version
void RenderUtils::drawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string)
{
   doDrawAngleString(x, y, size, angle, string);
}


void RenderUtils::drawAngleString(F32 x, F32 y, F32 size, const Color &color, F32 angle, const char *string)
{
   FontManager::setFontColor(color);
   doDrawAngleString(x, y, size, angle, string);
}


void RenderUtils::drawAngleString(F32 x, F32 y, F32 size, const Color &color, F32 alpha, F32 angle, const char *string)
{
   FontManager::setFontColor(color, alpha);
   doDrawAngleString(x, y, size, angle, string);
}



// Broken!
void RenderUtils::drawString(S32 x, S32 y, S32 size, const char *string)
{
   y += size;     // TODO: Adjust all callers so we can get rid of this!
   drawString_fixed(x, y, size, string);
}


// Broken!
void RenderUtils::drawString(F32 x, F32 y, S32 size, const char *string)
{
   y += size;     // TODO: Adjust all callers so we can get rid of this!
   drawAngleString(x, y, F32(size), 0, string);
}


// Broken!
void RenderUtils::drawString(F32 x, F32 y, F32 size, const char *string)
{
   y += size;     // TODO: Adjust all callers so we can get rid of this!
   drawAngleString(x, y, size, 0, string);
}


void RenderUtils::drawStringf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString_fixed(x, y + size, size, buffer);
}


void RenderUtils::drawStringf(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;
   drawString_fixed(x, y + size, size, buffer);
}


void RenderUtils::drawStringf(F32 x, F32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString_fixed(x, y + size, size, buffer);
}

// fixed
S32 RenderUtils::drawStringfc(F32 x, F32 y, F32 size, const Color &color, F32 alpha, const char *format, ...)
{
   makeBuffer;
   return drawStringc(x, y, (F32)size, color, alpha, buffer);
}


// fixed
S32 RenderUtils::drawStringfc(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;
   return drawStringc(x, y, (F32)size, buffer);
}


//S32 RenderUtils::drawStringfr(F32 x, F32 y, F32 size, const char *format, ...)
//{
//   makeBuffer;
//
//   F32 len = getStringWidth(size, buffer);
//   doDrawAngleString(x - len, y, size, 0, buffer);
//
//   return S32(len);
//}


S32 RenderUtils::drawStringfr(S32 x, S32 y, S32 size, const Color &color, const char *format, ...)
{
   makeBuffer;
   return drawStringr(x, y, size, color, buffer);
}


S32 RenderUtils::drawStringfr_fixed(S32 x, S32 y, S32 size, const Color &color, const char *format, ...)
{
   makeBuffer;
   return drawStringr(x, y - size, size, color, buffer);    // feeds into unfixed version
}


S32 RenderUtils::drawStringfr(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   return doDrawStringr(x, y, size, buffer);
}


S32 RenderUtils::drawStringr(S32 x, S32 y, S32 size, const Color &color, const char *string)
{
   FontManager::setFontColor(color);
   return doDrawStringr(x, y, size, string);
}


S32 RenderUtils::doDrawStringr(S32 x, S32 y, S32 size, const char *string)
{
   F32 len = getStringWidth((F32)size, string);
   doDrawAngleString((F32)x - len, (F32)y + size, (F32)size, 0, string);

   return (S32)len;
}


// Fixed
S32 RenderUtils::drawStringAndGetWidth(S32 x, S32 y, S32 size, const Color &color, const char *string)
{
   drawString_fixed(x, y + size, size, string);
   return getStringWidth(size, string);
}
   

S32 RenderUtils::drawStringAndGetWidth(S32 x, S32 y, S32 size, const char *string)
{
   drawString_fixed(x, y + size, size, string);
   return getStringWidth(size, string);
}


S32 RenderUtils::drawStringAndGetWidth(F32 x, F32 y, S32 size, const char *string)
{
   drawString_fixed(x, y + size, size, string);
   return getStringWidth(size, string);
}


S32 RenderUtils::drawStringAndGetWidth_fixed(F32 x, F32 y, S32 size, const char *string)
{
   drawString_fixed(x, y, size, string);
   return getStringWidth(size, string);
}


S32 RenderUtils::drawStringAndGetWidthf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString_fixed(x, y + size, size, buffer);
   return getStringWidth(size, buffer);
}


S32 RenderUtils::drawStringAndGetWidthf(F32 x, F32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString_fixed(x, y + size, size, buffer);
   return getStringWidth(size, buffer);
}


S32 RenderUtils::drawStringAndGetWidthf(F32 x, F32 y, S32 size, const Color &color, const char *format, ...)
{
   makeBuffer;
   drawString_fixed(x, y + size, size, color, buffer);
   return getStringWidth(size, buffer);
}


S32 RenderUtils::drawStringc(const Point &cen, F32 size, const char *string)
{
   return drawStringc(cen.x, cen.y, size, string);
}


S32 RenderUtils::drawCenteredString_fixed(S32 y, S32 size, const Color &color, const char *string)
{
   return drawCenteredString_fixed(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, y, size, color, string);
}


S32 RenderUtils::drawCenteredString_fixed(S32 y, S32 size, const Color &color, F32 alpha, const char *string)
{
   return drawCenteredString_fixed(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, y, size, color, alpha, string);
}


S32 RenderUtils::drawCenteredString_fixed(S32 y, S32 size, const char *string)
{
   return drawCenteredString_fixed(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, y, size, string);
}


//// For now, not very fault tolerant...  assumes well balanced []
//void RenderUtils::drawCenteredString_highlightKeys(S32 y, S32 size, const string &str, const Color &bodyColor, const Color &keyColor)
//{
//   S32 len = getStringWidth(size, str.c_str());
//   S32 x = DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2 - len / 2;
//   y += size;
//
//   std::size_t keyStart, keyEnd = 0;
//   S32 pos = 0;
//
//   keyStart = str.find("[");
//   while(keyStart != string::npos)
//   {
//      x += drawStringAndGetWidth_fixed(x, y, size, bodyColor, str.substr(pos, keyStart - pos).c_str());
//
//      keyEnd = str.find("]", keyStart) + 1;     // + 1 to include the "]" itself
//      
//      x += drawStringAndGetWidth_fixed(x, y, size, keyColor, str.substr(keyStart, keyEnd - keyStart).c_str());
//      pos = keyEnd;
//
//      keyStart = str.find("[", pos);
//   }
//   
//   // Draw any remaining bits of our string
//   drawString_fixed(x, y, size, bodyColor, str.substr(keyEnd).c_str());
//}


S32 RenderUtils::drawCenteredUnderlinedString(S32 y, S32 size, const Color &color, const char *string)
{
   S32 x = DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2;
   S32 xpos = drawCenteredString_fixed(x, y + size, size, color, string);
   drawHorizLine(xpos, DisplayManager::getScreenInfo()->getGameCanvasWidth() - xpos, y + size + 5, color);

   return xpos;
}


S32 RenderUtils::drawCenteredString(S32 x, S32 y, S32 size, const char *string)
{
   S32 xpos = x - getStringWidth(size, string) / 2;
   drawString(xpos, y, size, string);
   return xpos;
}


S32 RenderUtils::drawCenteredString_fixed(S32 x, S32 y, S32 size, const Color &color, const char *string)
{
   S32 xpos = x - getStringWidth(size, string) / 2;
   drawString_fixed(xpos, y, size, color, string);
   return xpos;   
}


S32 RenderUtils::drawCenteredString_fixed(S32 x, S32 y, S32 size, const Color &color, F32 alpha, const char *string)
{
   S32 xpos = x - getStringWidth(size, string) / 2;
   drawString_fixed(xpos, y, size, color, alpha, string);
   return xpos;   
}


S32 RenderUtils::drawCenteredString_fixed(F32 x, F32 y, S32 size, FontContext fontContext, const Color &color, const char *string)
{
   FontManager::pushFontContext(fontContext);

   F32 xpos = x - getStringWidth(size, string) / 2.0f;
   drawString_fixed(xpos, y, size, color, string);

   FontManager::popFontContext();

   return (S32)xpos;
}


//F32 RenderUtils::drawCenteredString(F32 x, F32 y, S32 size, const char *string)
//{
//   return drawCenteredString_fixed(x, y + size, F32(size), string);
//}


//F32 RenderUtils::drawCenteredString(F32 x, F32 y, F32 size, const char *string)
//{
//   F32 xpos = x - getStringWidth(size, string) / 2;
//   drawString(xpos, y, size, string);
//   return xpos;
//}


//S32 RenderUtils::drawCenteredStringf(S32 y, S32 size, const char *format, ...)
//{
//   makeBuffer; 
//   return (S32) drawCenteredString(y, size, buffer);
//}


S32 RenderUtils::drawCenteredStringf_fixed(S32 y, S32 size, const Color &color, const char *format, ...)
{
   makeBuffer;
   return (S32)drawCenteredString_fixed(y, size, color, buffer);
}


S32 RenderUtils::drawCenteredStringf_fixed(S32 y, S32 size, const Color &color, F32 alpha, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString_fixed(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, y, size, color, alpha, buffer);
}


//S32 RenderUtils::drawCenteredStringf(S32 x, S32 y, S32 size, const char *format, ...)
//{
//   makeBuffer;
//   return drawCenteredString(x, y, size, buffer);
//}


S32 RenderUtils::drawCenteredStringf_fixed(S32 x, S32 y, S32 size, const Color &color, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString_fixed(x, y, size, color, buffer);
}


// Figure out the first position of our CenteredString
S32 RenderUtils::getCenteredStringStartingPos(S32 size, const char *string)
{
   S32 x = DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2;      // x must be S32 in case it leaks off left side of screen
   x -= getStringWidth(size, string) / 2;

   return x;
}


S32 RenderUtils::getCenteredStringStartingPosf(S32 size, const char *format, ...)
{
   makeBuffer;
   return getCenteredStringStartingPos(size, buffer);
}


// Figure out the first position of our 2ColCenteredString
S32 RenderUtils::getCenteredString2ColStartingPos(S32 size, bool leftCol, const char *string)
{
   return get2ColStartingPos(leftCol) - getStringWidth(size, string) / 2;
}


S32 RenderUtils::getCenteredString2ColStartingPosf(S32 size, bool leftCol, const char *format, ...)
{
   makeBuffer;
   return getCenteredString2ColStartingPos(size, leftCol, buffer);
}


S32 RenderUtils::drawCenteredString2Col(S32 y, S32 size, const Color &color, bool leftCol, const char *string)
{
   S32 x = getCenteredString2ColStartingPos(size, leftCol, string);
   drawString_fixed(x, y, size, color, string);
   return x;
}


S32 RenderUtils::drawCenteredString2Colf(S32 y, S32 size, const Color &color, bool leftCol, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString2Col(y, size, color, leftCol, buffer);
}

   
S32 RenderUtils::get2ColStartingPos(bool leftCol)      // Must be S32 to avoid problems downstream
{
   const S32 canvasWidth = DisplayManager::getScreenInfo()->getGameCanvasWidth();
   return leftCol ? (canvasWidth / 4) : (canvasWidth - (canvasWidth / 4));
}


// Returns starting position of value, which is useful for positioning the cursor in an editable menu entry
S32 RenderUtils::drawCenteredStringPair(S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                        const char *leftStr, const char *rightStr)
{
   return drawCenteredStringPair(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, ypos, size, leftColor, rightColor, leftStr, rightStr);
}


// Returns starting position of value, which is useful for positioning the cursor in an editable menu entry
S32 RenderUtils::drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                        const char *leftStr, const char *rightStr)
{
   S32 xpos2 = getCenteredStringStartingPosf(size, "%s %s", leftStr, rightStr) + xpos - DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2;

   return drawStringPair(xpos2, ypos, size, leftColor, rightColor, leftStr, rightStr);
}


S32 RenderUtils::drawCenteredStringPair_fixed(S32 xpos, S32 ypos, S32 size, FontContext leftContext, FontContext rightContext, const Color &leftColor, const Color &rightColor,
                                              const char *leftStr, const char *rightStr)
{
   S32 width = getStringPairWidth(size, leftContext, rightContext, leftStr, rightStr);
   return drawStringPair(xpos - width / 2, ypos, size, leftContext, rightContext, leftColor, rightColor, string(leftStr).append(" ").c_str(), rightStr);
}


S32 RenderUtils::drawStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                const char *leftStr, const char *rightStr)
{
   // + 5 to compensate for fontStash bug calculating with of terminal spaces
   xpos += drawStringAndGetWidth_fixed(xpos, ypos, size, leftColor, leftStr) + 5;

   drawString_fixed(xpos, ypos, size, rightColor, rightStr);

   return xpos;
}


S32 RenderUtils::drawStringPair(S32 xpos, S32 ypos, S32 size, FontContext leftContext,
      FontContext rightContext, const Color& leftColor, const Color& rightColor,
      const char* leftStr, const char* rightStr)
{
   FontManager::pushFontContext(leftContext);

   FontManager::setFontColor(leftColor);
   xpos += drawStringAndGetWidth_fixed((F32)xpos, (F32)ypos, size, leftStr);
   FontManager::popFontContext();

   FontManager::pushFontContext(rightContext);
   FontManager::setFontColor(rightColor);

   drawString_fixed(xpos, ypos, size, rightStr);
   FontManager::popFontContext();

   return xpos;
}


S32 RenderUtils::getStringPairWidth(S32 size, const char *leftStr, const char *rightStr)
{
   return getStringWidthf(size, "%s %s", leftStr, rightStr);
}


//// Draws a string centered on the screen, with different parts colored differently
//S32 drawCenteredStringPair(S32 y, U32 size, const Color &col1, const Color &col2, const char *left, const char *right)
//{
//   S32 offset = getStringWidth(size, left) + getStringWidth(size, " ");
//   S32 width = offset + getStringWidth(size, buffer);
//   S32 x = (S32)((S32) canvasWidth - (getStringWidth(size, left) + getStringWidth(size, buffer))) / 2;
//
//   mGL->glColor(col1);
//   drawString(x, y, size, left);
//   mGL->glColor(col2);
//   drawString(x + offset, y, size, buffer);
//
//   return x;
//}


S32 RenderUtils::drawCenteredStringPair2Colf(S32 y, S32 size, bool leftCol, const char *left, const char *format, ...)
{
   makeBuffer;
   return drawCenteredStringPair2Col(y, size, leftCol, Colors::white, Colors::cyan, left, buffer);
}


S32 RenderUtils::drawCenteredStringPair2Colf(S32 y, S32 size, bool leftCol, const Color &leftColor, const Color &rightColor,
      const char *left, const char *format, ...)
{
   makeBuffer;
   return drawCenteredStringPair2Col(y, size, leftCol, leftColor, rightColor, left, buffer);
}

// Draws a string centered in the left or right half of the screen, with different parts colored differently
S32 RenderUtils::drawCenteredStringPair2Col(S32 y, S32 size, bool leftCol, const Color &leftColor, const Color &rightColor,
      const char *left, const char *right)
{
   S32 offset = getStringWidth(size, left) + getStringWidth(size, " ");
   S32 width = offset + getStringWidth(size, right);
   S32 x = get2ColStartingPos(leftCol) - width / 2;         // x must be S32 in case it leaks off left side of screen

   y += size;

   drawString_fixed(x,          y, size, leftColor,  left);
   drawString_fixed(x + offset, y, size, rightColor, right);

   return x;
}


void RenderUtils::drawTime(S32 x, S32 y, S32 size, const Color &color, S32 timeInMs, const char *prefixString)
{
   F32 F32time = (F32)timeInMs;

   U32 minsRemaining = U32(F32time / (60 * 1000));
   U32 secsRemaining = U32((F32time - F32(minsRemaining * 60 * 1000)) / 1000);

   drawStringf_fixed(x, y, size, color, "%s%02d:%02d", prefixString, minsRemaining, secsRemaining);
}


S32 RenderUtils::getStringWidth(FontContext fontContext, S32 size, const char *string)
{
   return (S32)getStringWidth(fontContext, (F32)size, string);
}


F32 RenderUtils::getStringWidth(FontContext fontContext, F32 size, const char *string)
{
   FontManager::pushFontContext(fontContext);
   F32 width = getStringWidth(size, string);
   FontManager::popFontContext();

   return width;
}


S32 RenderUtils::getStringWidth(S32 size, const string &str)
{
   return getStringWidth(size, str.c_str());
}


S32 RenderUtils::getStringWidth(S32 size, const char *string)
{
   return (S32)getStringWidth((F32)size, string);
}


F32 RenderUtils::getStringWidth(F32 size, const char *string)
{
   if(strcmp(string, "QYZX") == 0)
      printf("Width: %2.2f (%2.2f)\n",  FontManager::getStringLength(string) * size / 120, size);//xyzzy

   return FontManager::getStringLength(string) * size / 120;
}


F32 RenderUtils::getStringWidthf(F32 size, const char *format, ...)
{
   makeBuffer;
   return getStringWidth(size, buffer);
}


S32 RenderUtils::getStringWidthf(S32 size, const char *format, ...)
{
   makeBuffer;
   return getStringWidth(size, buffer);
}

#undef makeBuffer


// Given a string, break it up such that no part is wider than width.
void RenderUtils::wrapString(const string &str, S32 wrapWidth, S32 fontSize, FontContext context, Vector<string> &lines)
{
   FontManager::pushFontContext(context);
   Vector<string> wrapped = Zap::wrapString(str, wrapWidth, fontSize);
   FontManager::popFontContext();

   for(S32 i = 0; i < wrapped.size(); i++)
      lines.push_back(wrapped[i]);
}


S32 RenderUtils::getStringPairWidth(S32 size, 
                                    FontContext leftContext,
                                    FontContext rightContext, 
                                    const char* leftStr, 
                                    const char* rightStr)
{
   S32 leftWidth  = getStringWidth(leftContext,  size, leftStr);
   S32 spaceWidth = getStringWidth(leftContext,  size, " ");
   S32 rightWidth = getStringWidth(rightContext, size, rightStr);

   return leftWidth + spaceWidth + rightWidth;
}


// Returns the number of lines our msg consumed during rendering
U32 RenderUtils::drawWrapText(const string &msg, S32 xpos, S32 ypos, S32 width, S32 ypos_end, S32 lineHeight, S32 fontSize, const Color &color, bool draw)
{
   S32 linesDrawn = 0;
   Vector<string> lines = Zap::wrapString(msg, width, fontSize);

   // Align the y position, if alignBottom is enabled
   ypos -= lines.size() * lineHeight - fontSize;     // Align according to number of wrapped lines

   // Draw lines that need to wrap
   for(S32 i = 0; i < lines.size(); i++)
   {
      if(ypos >= ypos_end)      // If there is room to draw some lines at top when aligned bottom
      {
         if(draw)
            drawString_fixed(xpos, ypos, fontSize, color, lines[i].c_str());

         linesDrawn++;
      }
      ypos += lineHeight;
   }

   return linesDrawn;
}


void RenderUtils::drawLetter(char letter, const Point &pos, const Color &color, F32 alpha)
{
   // Mark the item with a letter, unless we're showing the reference ship
   F32 vertOffset = 7;
   if (letter >= 'a' && letter <= 'z')    // Better positioning for lowercase letters
      vertOffset = 5;

   drawStringfc(pos.x, pos.y + vertOffset, 15, color, alpha, "%c", letter);
}


void RenderUtils::drawLineStrip(const Vector<Point> *points, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   nvgMoveTo(nvg, points->get(0).x, points->get(0).y);
   for(S32 i = 1; i < points->size(); i++)
      nvgLineTo(nvg, points->get(i).x, points->get(i).y);

   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


void RenderUtils::drawLineStrip(const F32 *points, U32 pointCount, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   nvgMoveTo(nvg, points[0], points[1]);
   for(U32 i = 2; i < 2 * pointCount; i = i + 2)
      nvgLineTo(nvg, points[i], points[i+1]);

   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


void RenderUtils::drawLineLoop(const Vector<Point> *points, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   nvgMoveTo(nvg, points->get(0).x, points->get(0).y);
   for(S32 i = 1; i < points->size(); i++)
      nvgLineTo(nvg, points->get(i).x, points->get(i).y);
   nvgClosePath(nvg);  // Finish loop

   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


void RenderUtils::drawLineLoop(const F32 *points, U32 pointCount, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   nvgMoveTo(nvg, points[0], points[1]);
   for(U32 i = 2; i < 2 * pointCount; i = i + 2)
      nvgLineTo(nvg, points[i], points[i+1]);
   nvgClosePath(nvg);  // Finish loop

   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


void RenderUtils::drawLineLoop(const S16 *points, U32 pointCount, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   nvgMoveTo(nvg, (F32)points[0], (F32)points[1]);
   for(U32 i = 2; i < 2 * pointCount; i = i + 2)
      nvgLineTo(nvg, (F32)points[i], (F32)points[i+1]);
   nvgClosePath(nvg);  // Finish loop

   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


void RenderUtils::drawFilledLineLoop(const F32 *points, U32 pointCount, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   nvgMoveTo(nvg, points[0], points[1]);
   for(U32 i = 2; i < 2 * pointCount; i = i + 2)
      nvgLineTo(nvg, points[i], points[i+1]);
   nvgClosePath(nvg);  // Finish loop

   nvgFillColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgFill(nvg);
}


void RenderUtils::drawFilledLineLoop(const Vector<Point> *points, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   nvgMoveTo(nvg, points->get(0).x, points->get(0).y);
   for(S32 i = 1; i < points->size(); i++)
      nvgLineTo(nvg, points->get(i).x, points->get(i).y);
   nvgClosePath(nvg);  // Finish loop

   nvgFillColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgFill(nvg);
}


// Operates like GL_LINES, drawing individual 2-point line segments
void RenderUtils::drawLines(const F32 *points, U32 pointCount, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   // 1 segment == 2 points == 4 F32s
   for(U32 i = 0; i < 2 * pointCount; i = i + 4)
   {
      nvgMoveTo(nvg, points[i],   points[i+1]);
      nvgLineTo(nvg, points[i+2], points[i+3]);
   }

   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


// Operates like GL_LINES, drawing individual 2-point line segments
void RenderUtils::drawLines(const Vector<Point> *points, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   // 1 segment == 2 points == 4 F32s
   for(S32 i = 0; i < points->size(); i = i + 2)
   {
      nvgMoveTo(nvg, points->get(i).x, points->get(i).y);
      nvgLineTo(nvg, points->get(i+1).x, points->get(i+1).y);
   }

   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


void RenderUtils::drawLine(F32 x1, F32 y1, F32 x2, F32 y2, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);

   nvgMoveTo(nvg, x1, y1);
   nvgLineTo(nvg, x2, y2);

   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


void RenderUtils::drawLineGradient(F32 x1, F32 y1, F32 x2, F32 y2,
      const Color &color1, F32 alpha1, const Color &color2, F32 alpha2)
{
   NVGpaint gradient = nvgLinearGradient(nvg, x1, y1, x2, y2,
         nvgRGBAf(color1.r, color1.g, color1.b, alpha1),
         nvgRGBAf(color2.r, color2.g, color2.b, alpha2));

   nvgBeginPath(nvg);

   nvgMoveTo(nvg, x1, y1);
   nvgLineTo(nvg, x2, y2);

   nvgStrokePaint(nvg, gradient);
   nvgStroke(nvg);
}


void RenderUtils::drawRectHorizGradient(F32 x, F32 y, F32 w, F32 h,
      const Color &color1, F32 alpha1, const Color &color2, F32 alpha2)
{
   NVGpaint gradient = nvgLinearGradient(nvg, x, y, x+w, y,
         nvgRGBAf(color1.r, color1.g, color1.b, alpha1),
         nvgRGBAf(color2.r, color2.g, color2.b, alpha2));

   nvgBeginPath(nvg);

   nvgRect(nvg, x, y, w, h);

   nvgFillPaint(nvg, gradient);
   nvgFill(nvg);
}


void RenderUtils::drawRectVertGradient(F32 x, F32 y, F32 w, F32 h,
      const Color &color1, F32 alpha1, const Color &color2, F32 alpha2)
{
   NVGpaint gradient = nvgLinearGradient(nvg, x, y, x, y+h,
         nvgRGBAf(color1.r, color1.g, color1.b, alpha1),
         nvgRGBAf(color2.r, color2.g, color2.b, alpha2));

   nvgBeginPath(nvg);

   nvgRect(nvg, x, y, w, h);

   nvgFillPaint(nvg, gradient);
   nvgFill(nvg);
}


void RenderUtils::drawPoints(const F32 *points, U32 pointCount, const Color &color, F32 alpha)
{
   // FIXME NANOVG
#ifdef BF_USE_GLES2
   // TODO
#else
   // This is GL 1.x/2.x only
   glColor4f(color.r, color.g, color.b, alpha);

   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, points);
   glDrawArrays(GL_POINTS, 0, pointCount);

   glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


void RenderUtils::drawPoints(const F32 *points, U32 pointCount, const Color &color, F32 alpha,
      F32 pointSize, F32 scale, F32 translateX, F32 translateY, F32 rotate)
{
   // FIXME NANOVG
#ifdef BF_USE_GLES2
   // TODO
#else
   glPointSize(pointSize);  // Might need to do inside matrix?

   glPushMatrix();
      glScalef(scale, scale, 1);
      glTranslatef(translateX, translateY, 0.0f);
      glRotatef(rotate, 0.0f, 0.0f, 1.0f);

      drawPoints(points, pointCount, color, alpha);
   glPopMatrix();
#endif
}


void RenderUtils::drawPointsColorArray(const F32 *points, const F32 *colors, U32 count, S32 stride)
{
   // FIXME NANOVG
#ifdef BF_USE_GLES2
   // TODO
#else
   glPointSize(DEFAULT_LINE_WIDTH);

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   // stride is the byte offset between consecutive vertices or colors
   glVertexPointer(2, GL_FLOAT, stride, points);
   glColorPointer(4, GL_FLOAT, stride, colors);
   glDrawArrays(GL_POINTS, 0, count);

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


void RenderUtils::drawLinesColorArray(const F32 *vertices, const F32 *colors, U32 count, S32 stride)
{
   // FIXME NANOVG
#ifdef BF_USE_GLES2
   // TODO
#else
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   // stride is the byte offset between consecutive vertices or colors
   glVertexPointer(2, GL_FLOAT, stride, vertices);
   glColorPointer(4, GL_FLOAT, stride, colors);
   glDrawArrays(GL_LINES, 0, count);

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


void RenderUtils::drawLineStripColorArray(const F32 *vertices, const F32 *colors, U32 count, S32 stride)
{
   // FIXME NANOVG
#ifdef BF_USE_GLES2
   // TODO
#else
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   // stride is the byte offset between consecutive vertices or colors
   glVertexPointer(2, GL_FLOAT, stride, vertices);
   glColorPointer(4, GL_FLOAT, stride, colors);
   glDrawArrays(GL_LINE_STRIP, 0, count);

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


void RenderUtils::drawLine(const Vector<Point> *points, const Color &color, F32 alpha)
{
   drawLineStrip(points, color, alpha);
}


void RenderUtils::drawHorizLine(F32 y, const Color &color, F32 alpha)
{
   drawHorizLine((S32)y, color, alpha);
}


void RenderUtils::drawHorizLine(U32 y, const Color &color, F32 alpha)
{
   drawHorizLine(0, DisplayManager::getScreenInfo()->getGameCanvasWidth(), y, color, alpha);
}


void RenderUtils::drawHorizLine(S32 y, const Color &color, F32 alpha)
{
   drawHorizLine(0, DisplayManager::getScreenInfo()->getGameCanvasWidth(), y, color, alpha);
}


void RenderUtils::drawHorizLine(S32 x1, S32 x2, S32 y, const Color &color, F32 alpha)
{
   drawHorizLine((F32)x1, (F32)x2, (F32)y, color, alpha);
}


void RenderUtils::drawHorizLine(F32 x1, F32 x2, F32 y, const Color &color, F32 alpha)
{
   drawLine(x1, y, x2, y, color, alpha);
}


void RenderUtils::drawVertLine(S32 x, const Color &color, F32 alpha)
{
   drawVertLine((F32)x, (F32)0, (F32)DisplayManager::getScreenInfo()->getGameCanvasHeight(), color, alpha);
}


void RenderUtils::drawVertLine(S32 x, S32 y1, S32 y2, const Color &color, F32 alpha)
{
   drawVertLine((F32)x, (F32)y1, (F32)y2, color, alpha);
}


void RenderUtils::drawVertLine(F32 x, F32 y1, F32 y2, const Color &color, F32 alpha)
{
   drawLine(x, y1, x, y2, color, alpha);
}


void RenderUtils::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, const Color &outlineColor)
{
   drawFilledRect(x1, y1, x2-x1, y2-y1, fillColor, 1, outlineColor, 1);
}


void RenderUtils::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha, const Color &outlineColor)
{
   drawFilledRect(x1, y1, x2-x1, y2-y1, fillColor, fillAlpha, outlineColor, 1);
}


void RenderUtils::drawFilledRect(S32 x, S32 y, S32 w, S32 h, const Color &fillColor, F32 fillAlpha, const Color &outlineColor, F32 outlineAlpha)
{
   drawFilledRect(x, y, w, h, fillColor, fillAlpha);
   drawRect(x, y, w, h, outlineColor, outlineAlpha);
}


void RenderUtils::drawHollowRect(const Point &center, S32 width, S32 height, const Color &color, F32 alpha)
{
   drawHollowRect(center.x - (F32)width / 2, center.y - (F32)height / 2,
         width, height, color, alpha);
}


void RenderUtils::drawHollowRect(const Point &p1, const Point &p2, const Color &color, F32 alpha)
{
   drawHollowRect(p1.x, p1.y, p2.x-p1.x, p2.y-p1.y, color, alpha);
}


void RenderUtils::drawFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, U8 corners, const Color &color, F32 alpha)
{
   drawFancyBox((F32)xLeft, (F32)yTop, (F32)xRight, (F32)yBottom, (F32)cornerInset, corners, color, alpha);
}


void RenderUtils::drawFancyBox(F32 xLeft, F32 yTop, F32 xRight, F32 yBottom, F32 cornerInset, U8 corners, const Color &color, F32 alpha)
{
   F32 vertices[] = {
         xLeft, yTop,                   // Top
         xRight - (corners & UR ? cornerInset : 0), yTop,
         xRight, yTop + cornerInset,    // Edge
         xRight, yBottom,               // Bottom
         xLeft + (corners & LL ? cornerInset : 0), yBottom,
         xLeft, yBottom - cornerInset   // Edge
   };

   drawLineLoop(vertices, ARRAYSIZE(vertices)/2, color, alpha);
}

void RenderUtils::drawFilledFancyBox(F32 xLeft, F32 yTop, F32 xRight, F32 yBottom, F32 cornerInset, U8 corners, const Color &color, F32 alpha)
{
   F32 vertices[] = {
         xLeft, yTop,                   // Top
         xRight - (corners & UR ? cornerInset : 0), yTop,
         xRight, yTop + cornerInset,    // Edge
         xRight, yBottom,               // Bottom
         xLeft + (corners & LL ? cornerInset : 0), yBottom,
         xLeft, yBottom - cornerInset   // Edge
   };

   drawFilledLineLoop(vertices, ARRAYSIZE(vertices)/2, color, alpha);
}


void RenderUtils::drawHollowFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, const Color &color, F32 alpha)
{
   drawHollowFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, LL|UR, color, alpha);
}


void RenderUtils::drawHollowFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, U8 corners, const Color &color, F32 alpha)
{
   drawFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, corners, color, alpha);
}


void RenderUtils::drawFilledFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, const Color &fillColor, F32 fillAlpha, const Color &borderColor)
{
   drawFilledFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, LL|UR, fillColor, fillAlpha, borderColor);
}


void RenderUtils::drawFilledFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, U8 corners, const Color &fillColor, F32 fillAlpha, const Color &borderColor)
{
   // Fill
   drawFilledFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, corners, fillColor, fillAlpha);

   // Border
   drawFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, corners, borderColor);
}


// Draw arc centered on pos, with given radius, from startAngle to endAngle.  0 is East, increasing CW
void RenderUtils::drawArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle, const Color &color, F32 alpha)
{
   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);

   nvgBeginPath(nvg);
   nvgArc(nvg, 0, 0, radius, startAngle, endAngle, NVG_CW);  // Clockwise
   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);

   nvgRestore(nvg);
}


void RenderUtils::drawDashedArc(const Point &center, F32 radius, F32 arcTheta, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle, const Color &color, F32 alpha)
{
   F32 interimAngle = arcTheta / dashCount;

   for(S32 i = 0; i < dashCount; i++)
      drawArc(center, radius, interimAngle * i + offsetAngle, (interimAngle * (i + 1)) - dashSpaceCentralAngle + offsetAngle, color, alpha);
}


void RenderUtils::drawDashedCircle(const Point &center, F32 radius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle, const Color &color, F32 alpha)
{
   drawDashedArc(center, radius, FloatTau, dashCount, dashSpaceCentralAngle, offsetAngle, color, alpha);
}


void RenderUtils::drawAngledRay(const Point &center, F32 innerRadius, F32 outerRadius, F32 angle, const Color &color)
{
   F32 x1 = center.x + cosf(angle) * innerRadius;
   F32 y1 = center.y + sinf(angle) * innerRadius;
   F32 x2 = center.x + cosf(angle) * outerRadius;
   F32 y2 = center.y + sinf(angle) * outerRadius;

   drawLine(x1, y1, x2, y2, color);
}


void RenderUtils::drawAngledRayCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 rayCount, F32 offsetAngle, const Color &color)
{
   drawAngledRayArc(center, innerRadius, outerRadius, FloatTau, rayCount, offsetAngle, color);
}


void RenderUtils::drawAngledRayArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, S32 rayCount, F32 offsetAngle, const Color &color)
{
   F32 interimAngle = centralAngle / rayCount;

   for(S32 i = 0; i < rayCount; i++)
      drawAngledRay(center, innerRadius, outerRadius, interimAngle * i + offsetAngle, color);
}


void RenderUtils::drawDashedHollowCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle, const Color &color)
{
   // Draw the dashed circles
   drawDashedCircle(center, innerRadius, dashCount, dashSpaceCentralAngle, offsetAngle, color);
   drawDashedCircle(center, outerRadius, dashCount, dashSpaceCentralAngle, offsetAngle, color);

   // Now connect them
   drawAngledRayCircle(center, innerRadius,  outerRadius, dashCount, offsetAngle,                         color);
   drawAngledRayCircle(center, innerRadius,  outerRadius, dashCount, offsetAngle - dashSpaceCentralAngle, color);
}


void RenderUtils::drawHollowArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, F32 offsetAngle, const Color &color)
{
   drawAngledRay(center, innerRadius, outerRadius, offsetAngle,                color);
   drawAngledRay(center, innerRadius, outerRadius, offsetAngle + centralAngle, color);

   drawArc(center, innerRadius, offsetAngle, offsetAngle + centralAngle, color);
   drawArc(center, outerRadius, offsetAngle, offsetAngle + centralAngle, color);
}


void RenderUtils::drawRoundedRect(const Point &pos, S32 width, S32 height, S32 rad, const Color &color, F32 alpha)
{
   drawRoundedRect(pos, (F32)width, (F32)height, (F32)rad, color, alpha);
}


// Draw rounded rectangle centered on pos
void RenderUtils::drawRoundedRect(const Point &pos, F32 width, F32 height, F32 rad, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);
   nvgRoundedRect(nvg, pos.x - width/2, pos.y - height/2, width, height, rad);
   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


void RenderUtils::drawFilledArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);
   nvgArc(nvg, pos.x, pos.y, radius, startAngle, endAngle, NVG_CCW);  // Counter-clockwise
   nvgFillColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgFill(nvg);
}


void RenderUtils::drawFilledRoundedRect(const Point &pos, S32 width, S32 height, const Color &fillColor, const Color &outlineColor, S32 radius, F32 alpha)
{
   drawFilledRoundedRect(pos, (F32)width, (F32)height, fillColor, outlineColor, (F32)radius, alpha);
}


// This includes an outline on top
void RenderUtils::drawFilledRoundedRect(const Point &pos, F32 width, F32 height, const Color &fillColor, const Color &outlineColor, F32 radius, F32 alpha)
{
   // Draw fill
   nvgBeginPath(nvg);
   nvgRoundedRect(nvg, pos.x - width/2, pos.y - height/2, width, height, radius);
   nvgFillColor(nvg, nvgRGBAf(fillColor.r, fillColor.g, fillColor.b, alpha));
   nvgFill(nvg);

   // Draw outline
   drawRoundedRect(pos, width, height, radius, outlineColor, alpha);
}


// Draw an n-sided polygon
void RenderUtils::drawPolygon(S32 sides, F32 radius, F32 angle, const Color &color, F32 alpha)
{
   Vector<Point> points;
   generatePointsInACurve(angle, angle + FloatTau, sides + 1, radius, points);   // +1 so we can "close the loop"

   drawLineStrip(&points, color, alpha);
}


// Draw an n-sided polygon
void RenderUtils::drawPolygon(const Point &pos, S32 sides, F32 radius, F32 angle, const Color &color, F32 alpha)
{
   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);

   drawPolygon(sides, radius, angle, color, alpha);

   nvgRestore(nvg);
}


void RenderUtils::drawStar(const Point &pos, S32 points, F32 radius, F32 innerRadius, const Color &color, F32 alpha)
{
   F32 ang = FloatTau / F32(points * 2);
   F32 a = -ang / 2;
   F32 r = radius;
   bool inout = true;

   Point p;

   Vector<Point> pts;
   for(S32 i = 0; i < points * 2; i++)
   {
      p.set(r * cos(a), r * sin(a));
      pts.push_back(p + pos);

      a += ang;
      inout = !inout;
      r = inout ? radius : innerRadius;
   }

   drawLineLoop(&pts, color, alpha);
}


//void RenderUtils::drawFilledStar(const Point &pos, S32 points, F32 radius, F32 innerRadius)
//{
//   F32 ang = FloatTau / F32(points * 2);
//   F32 a = ang / 2;
//   F32 r = innerRadius;
//   bool inout = false;
//
//   static Point p;
//   static Point first;
//
//   static Vector<Point> pts;
//   static Vector<Point> core;
//   static Vector<Point> outline;
//
//   pts.clear();
//   core.clear();
//   outline.clear();
//
//   for(S32 i = 0; i < points * 2; i++)
//   {
//      p.set(r * cos(a) + pos.x, r * sin(a) + pos.y);
//
//      outline.push_back(p);
//
//      if(i == 0)
//      {
//         first = p;
//         core.push_back(p);
//      }
//      else if(i % 2 == 0)
//      {
//         pts.push_back(p);
//         core.push_back(p);
//      }
//
//      pts.push_back(p);
//
//      a += ang;
//      inout = !inout;
//      r = inout ? radius : innerRadius;
//   }
//
//   pts.push_back(first);
//
//   mGL->renderPointVector(&pts, GLOPT::Triangles);       // Points
//   mGL->renderPointVector(&core, GLOPT::TriangleFan);        // Inner pentagon
//   mGL->renderPointVector(&outline, GLOPT::LineLoop);   // Outline to make things look smoother, at least when star is small
//}


// Draw an ellipse at pos, with axes width and height, canted at angle
void RenderUtils::drawEllipse(const Point &pos, F32 radiusX, F32 radiusY, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);
   nvgEllipse(nvg, pos.x, pos.y, radiusX, radiusY);
   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


// Draw an ellipse at pos, with axes width and height, canted at angle
void RenderUtils::drawEllipse(const Point &pos, S32 radiusX, S32 radiusY, const Color &color, F32 alpha)
{
   drawEllipse(pos, (F32)radiusX, (F32)radiusY, color, alpha);
}


// Well...  draws a filled ellipse, much as you'd expect
void RenderUtils::drawFilledEllipse(const Point &pos, F32 radiusX, F32 radiusY, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);
   nvgEllipse(nvg, pos.x, pos.y, radiusX, radiusY);
   nvgFillColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgFill(nvg);
}


void RenderUtils::drawFilledEllipse(const Point &pos, S32 radiusX, S32 radiusY, const Color &color, F32 alpha)
{
   drawFilledEllipse(pos, (F32)radiusX, (F32)radiusY, color, alpha);
}


void RenderUtils::drawFilledCircle(const Point &pos, F32 radius, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);
   nvgCircle(nvg, pos.x, pos.y, radius);
   nvgFillColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgFill(nvg);
}


// pos is the square's center
void RenderUtils::drawSquare(const Point &pos, F32 radius, const Color &color, bool filled)
{
   if(filled)
      drawFilledRect(pos.x - radius, pos.y - radius, 2*radius, 2*radius, color);
   else
      drawRect(pos.x - radius, pos.y - radius, 2*radius, 2*radius, color);
}


// pos is the square's center
void RenderUtils::drawSquare(const Point &pos, F32 radius, const Color &color, F32 alpha, bool filled)
{
   if(filled)
      drawFilledRect(pos.x - radius, pos.y - radius, 2*radius, 2*radius, color, alpha);
   else
      drawRect(pos.x - radius, pos.y - radius, 2*radius, 2*radius, color, alpha);
}


// Hollow by default
void RenderUtils::drawSquare(const Point &pos, S32 radius, const Color &color, F32 alpha, bool filled)
{
   drawSquare(pos, (F32)radius, color, alpha, filled);
}


void RenderUtils::drawHollowSquare(const Point &pos, F32 radius, const Color &color, F32 alpha)
{
   drawSquare(pos, radius, color, alpha, false);
}


void RenderUtils::drawFilledSquare(const Point &pos, F32 radius, const Color &color, F32 alpha)
{
   drawSquare(pos, radius, color, alpha, true);
}


void RenderUtils::drawCircle(const Point &center, F32 radius, const Color &color, F32 alpha)
{
   nvgSave(nvg);
   nvgTranslate(nvg, center.x, center.y);
   drawCircle(radius, color, alpha);
   nvgRestore(nvg);
}


void RenderUtils::drawCircle(F32 radius, const Color &color, F32 alpha)
{
   nvgBeginPath(nvg);
   nvgCircle(nvg, 0, 0, radius);
   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgStroke(nvg);
}


void RenderUtils::setDefaultLineWidth(F32 width)
{
   RenderUtils::DEFAULT_LINE_WIDTH = width;
   RenderUtils::LINE_WIDTH_1 = RenderUtils::DEFAULT_LINE_WIDTH * 0.5f;
   RenderUtils::LINE_WIDTH_3 = RenderUtils::DEFAULT_LINE_WIDTH * 1.5f;
   RenderUtils::LINE_WIDTH_4 = RenderUtils::DEFAULT_LINE_WIDTH * 2;
}


void RenderUtils::lineWidth(F32 width)
{
   // TODO manage width with respect to current scaling matrix
   nvgStrokeWidth(nvg, width);
}


};

