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

#include "RenderUtils.h"

#include "UI.h"
#include "ScreenInfo.h"

#include "MathUtils.h"     // For MIN/MAX def
#include "OpenglUtils.h"
#include "FontManager.h"
#include "Colors.h"

#include <stdarg.h>        // For va_args
#include <stdio.h>         // For vsnprintf


namespace Zap {

static char buffer[2048];     // Reusable buffer
#define makeBuffer    va_list args; va_start(args, format); vsnprintf(buffer, sizeof(buffer), format, args); va_end(args);


F32 gLineWidth1 = 1.0f;
F32 gDefaultLineWidth = 2.0f;
F32 gLineWidth3 = 3.0f;
F32 gLineWidth4 = 4.0f;

extern ScreenInfo gScreenInfo;


void doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string)
{
   glPushMatrix();
      glTranslatef(x, y, 0);
      glRotatef(angle * RADIANS_TO_DEGREES, 0, 0, 1);

      FontManager::renderString(size, string);

   glPopMatrix();
}


// Same but accepts S32 args
//void doDrawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string, bool autoLineWidth)
//{
//   doDrawAngleString(F32(x), F32(y), size, angle, string, autoLineWidth);
//}


// Center text between two points, adjust angle so it's always right-side-up
void drawStringf_2pt(Point p1, Point p2, F32 size, F32 vert_offset, const char *format, ...)
{
   F32 ang = p1.angleTo(p2);

   // Make sure text is right-side-up
   if(ang < -FloatHalfPi || ang > FloatHalfPi)
   {
      Point temp = p2;
      p2 = p1;
      p1 = temp;
      ang = p1.angleTo(p2);
   }

   F32 cosang = cos(ang);
   F32 sinang = sin(ang);

   makeBuffer;
   F32 len = getStringWidthf(size, buffer);
   F32 offset = (p1.distanceTo(p2) - len) / 2;

   doDrawAngleString(p1.x + cosang * offset + sinang * (size + vert_offset), p1.y + sinang * offset - cosang * (size + vert_offset), size, ang, buffer);
}


// New, fixed version
void drawAngleStringf(F32 x, F32 y, F32 size, F32 angle, const char *format, ...)
{
   makeBuffer;
   doDrawAngleString(x, y, size, angle, buffer);
}


// New, fixed version
void drawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string)
{
   doDrawAngleString(x, y, size, angle, string);
}


// Broken!
void drawString(S32 x, S32 y, S32 size, const char *string)
{
   y += size;     // TODO: Adjust all callers so we can get rid of this!
   drawString_fixed(x, y, size, string);
}


// Broken!
void drawString(F32 x, F32 y, S32 size, const char *string)
{
   y += size;     // TODO: Adjust all callers so we can get rid of this!
   drawAngleString(x, y, F32(size), 0, string);
}


// Broken!
void drawString(F32 x, F32 y, F32 size, const char *string)
{
   y += size;     // TODO: Adjust all callers so we can get rid of this!
   drawAngleString(x, y, size, 0, string);
}


void drawStringf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
}


void drawStringf(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
}


void drawStringf(F32 x, F32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
}


S32 drawStringfc(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;
   return drawStringc(x, y, (F32)size, buffer);
}


S32 drawStringfr(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;

   F32 len = getStringWidth(size, buffer);
   doDrawAngleString(x - len, y, size, 0, buffer);

   return S32(len);
}


S32 drawStringfr(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   return drawStringr(x, y, size, buffer);
}


S32 drawStringr(S32 x, S32 y, S32 size, const char *string)
{
   F32 len = getStringWidth((F32)size, string);
   doDrawAngleString((F32)x - len, (F32)y + size, (F32)size, 0, string);

   return (S32)len;
}

   
S32 drawStringAndGetWidth(S32 x, S32 y, S32 size, const char *string)
{
   drawString(x, y, size, string);
   return getStringWidth(size, string);
}


S32 drawStringAndGetWidth(F32 x, F32 y, S32 size, const char *string)
{
   drawString(x, y, size, string);
   return getStringWidth(size, string);
}


S32 drawStringAndGetWidthf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
   return getStringWidth(size, buffer);
}


S32 drawStringAndGetWidthf(F32 x, F32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
   return getStringWidth(size, buffer);
}


S32 drawStringc(S32 x, S32 y, S32 size, const char *string)
{
   return drawStringc((F32)x, (F32)y, (F32)size, string);
}


// Uses fixed drawAngleString()
S32 drawStringc(F32 x, F32 y, F32 size, const char *string)
{
   F32 len = getStringWidth(size, string);
   drawAngleString(x - len / 2, y, size, 0, string);

   return (S32)len;
}


S32 drawStringc(const Point &cen, F32 size, const char *string)
{
   return drawStringc(cen.x, cen.y, size, string);
}


S32 drawCenteredString_fixed(S32 y, S32 size, const char *string)
{
   return drawCenteredString_fixed(gScreenInfo.getGameCanvasWidth() / 2, y, size, string);
}


// For now, not very fault tolerant...  assumes well balanced []
void drawCenteredString_highlightKeys(S32 y, S32 size, const string &str, const Color &bodyColor, const Color &keyColor)
{
   S32 len = getStringWidth(size, str.c_str());
   S32 x = gScreenInfo.getGameCanvasWidth() / 2 - len / 2;

   std::size_t keyStart, keyEnd = 0;
   S32 pos = 0;

   keyStart = str.find("[");
   while(keyStart != string::npos)
   {
      glColor(bodyColor);
      x += drawStringAndGetWidth(x, y, size, str.substr(pos, keyStart - pos).c_str());

      keyEnd = str.find("]", keyStart) + 1;     // + 1 to include the "]" itself
      glColor(keyColor);
      x += drawStringAndGetWidth(x, y, size, str.substr(keyStart, keyEnd - keyStart).c_str());
      pos = keyEnd;

      keyStart = str.find("[", pos);
   }
   
   // Draw any remaining bits of our string
   glColor(bodyColor);
   drawString(x, y, size, str.substr(keyEnd).c_str());
}


extern void drawHorizLine(S32 x1, S32 x2, S32 y);

S32 drawCenteredUnderlinedString(S32 y, S32 size, const char *string)
{
   S32 x = gScreenInfo.getGameCanvasWidth() / 2;
   S32 xpos = drawCenteredString(x, y, size, string);
   drawHorizLine(xpos, gScreenInfo.getGameCanvasWidth() - xpos, y + size + 5);

   return xpos;
}


S32 drawCenteredString(S32 x, S32 y, S32 size, const char *string)
{
   S32 xpos = x - getStringWidth(size, string) / 2;
   drawString(xpos, y, size, string);
   return xpos;
}


S32 drawCenteredString_fixed(S32 x, S32 y, S32 size, const char *string)
{
   S32 xpos = x - getStringWidth(size, string) / 2;
   drawString_fixed(xpos, y, size, string);
   return xpos;
}


S32 drawCenteredString_fixed(F32 x, F32 y, S32 size, FontContext fontContext, const char *string)
{
   FontManager::pushFontContext(fontContext);

   F32 xpos = x - getStringWidth(size, string) / 2.0f;
   drawString_fixed(xpos, y, size, string);

   FontManager::popFontContext();

   return (S32)xpos;
}


F32 drawCenteredString(F32 x, F32 y, S32 size, const char *string)
{
   return drawCenteredString(x, y, F32(size), string);
}


F32 drawCenteredString(F32 x, F32 y, F32 size, const char *string)
{
   F32 xpos = x - getStringWidth(size, string) / 2;
   drawString(xpos, y, size, string);
   return xpos;
}


S32 drawCenteredStringf(S32 y, S32 size, const char *format, ...)
{
   makeBuffer; 
   return drawCenteredString(y, size, buffer);
}


S32 drawCenteredStringf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString(x, y, size, buffer);
}


// Figure out the first position of our CenteredString
S32 getCenteredStringStartingPos(S32 size, const char *string)
{
   S32 x = gScreenInfo.getGameCanvasWidth() / 2;      // x must be S32 in case it leaks off left side of screen
   x -= getStringWidth(size, string) / 2;

   return x;
}


S32 getCenteredStringStartingPosf(S32 size, const char *format, ...)
{
   makeBuffer;
   return getCenteredStringStartingPos(size, buffer);
}


// Figure out the first position of our 2ColCenteredString
S32 getCenteredString2ColStartingPos(S32 size, bool leftCol, const char *string)
{
   return get2ColStartingPos(leftCol) - getStringWidth(size, string) / 2;
}


S32 getCenteredString2ColStartingPosf(S32 size, bool leftCol, const char *format, ...)
{
   makeBuffer;
   return getCenteredString2ColStartingPos(size, leftCol, buffer);
}


S32 drawCenteredString2Col(S32 y, S32 size, bool leftCol, const char *string)
{
   S32 x = getCenteredString2ColStartingPos(size, leftCol, string);
   drawString(x, y, size, string);
   return x;
}


S32 drawCenteredString2Colf(S32 y, S32 size, bool leftCol, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString2Col(y, size, leftCol, buffer);
}

   
S32 get2ColStartingPos(bool leftCol)      // Must be S32 to avoid problems downstream
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   return leftCol ? (canvasWidth / 4) : (canvasWidth - (canvasWidth / 4));
}


//extern void glColor(const Color &c, float alpha = 1.0);

// Returns starting position of value, which is useful for positioning the cursor in an editable menu entry
S32 drawCenteredStringPair(S32 ypos, S32 size, const Color &leftColor, const Color &rightColor, 
                                          const char *leftStr, const char *rightStr)
{
   return drawCenteredStringPair(gScreenInfo.getGameCanvasWidth() / 2, ypos, size, leftColor, rightColor, leftStr, rightStr);
}


// Returns starting position of value, which is useful for positioning the cursor in an editable menu entry
S32 drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor, 
                                          const char *leftStr, const char *rightStr)
{
   S32 xpos2 = getCenteredStringStartingPosf(size, "%s %s", leftStr, rightStr) + xpos - gScreenInfo.getGameCanvasWidth() / 2;

   return drawStringPair(xpos2, ypos, size, leftColor, rightColor, leftStr, rightStr);
}

S32 drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, FontContext leftContext, FontContext rightContext, const Color &leftColor, const Color &rightColor,
                                          const char *leftStr, const char *rightStr)
{
   S32 width = getStringPairWidth(size, leftContext, rightContext, leftStr, rightStr);
   return drawStringPair(xpos - width / 2, ypos, size, leftContext, rightContext, leftColor, rightColor, string(leftStr).append(" ").c_str(), rightStr);
}


S32 drawStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor, 
                                         const char *leftStr, const char *rightStr)
{
   glColor(leftColor);

   // Use crazy width calculation to compensate for fontStash bug calculating with of terminal spaces
   xpos += drawStringAndGetWidth((F32)xpos, (F32)ypos, size, leftStr) + 5; //getStringWidth(size, "X X") - getStringWidth(size, "XX");

   glColor(rightColor);
   drawString(xpos, ypos, size, rightStr);

   return xpos;
}


S32 drawStringPair(S32 xpos, S32 ypos, S32 size, FontContext leftContext,
      FontContext rightContext, const Color& leftColor, const Color& rightColor,
      const char* leftStr, const char* rightStr)
{
   FontManager::pushFontContext(leftContext);
   glColor(leftColor);
   xpos += drawStringAndGetWidth((F32)xpos, (F32)ypos, size, leftStr);
   FontManager::popFontContext();

   FontManager::pushFontContext(rightContext);
   glColor(rightColor);
   drawString(xpos, ypos, size, rightStr);
   FontManager::popFontContext();

   return xpos;
}


S32 getStringPairWidth(S32 size, const char *leftStr, const char *rightStr)
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
//   glColor(col1);
//   drawString(x, y, size, left);
//   glColor(col2);
//   drawString(x + offset, y, size, buffer);
//
//   return x;
//}


S32 drawCenteredStringPair2Colf(S32 y, S32 size, bool leftCol, const char *left, const char *format, ...)
{
   makeBuffer;
   return drawCenteredStringPair2Col(y, size, leftCol, Colors::white, Colors::cyan, left, buffer);
}


S32 drawCenteredStringPair2Colf(S32 y, S32 size, bool leftCol, const Color &leftColor, const Color &rightColor,
      const char *left, const char *format, ...)
{
   makeBuffer;
   return drawCenteredStringPair2Col(y, size, leftCol, leftColor, rightColor, left, buffer);
}

// Draws a string centered in the left or right half of the screen, with different parts colored differently
S32 drawCenteredStringPair2Col(S32 y, S32 size, bool leftCol, const Color &leftColor, const Color &rightColor,
      const char *left, const char *right)
{
   S32 offset = getStringWidth(size, left) + getStringWidth(size, " ");
   S32 width = offset + getStringWidth(size, right);
   S32 x = get2ColStartingPos(leftCol) - width / 2;         // x must be S32 in case it leaks off left side of screen

   glColor(leftColor);
   drawString(x, y, size, left);

   glColor(rightColor);
   drawString(x + offset, y, size, right);

   return x;
}


// Draw a left-justified string at column # (1-4)
void drawString4Col(S32 y, S32 size, U32 col, const char *string)
{
   drawString(UserInterface::horizMargin + ((gScreenInfo.getGameCanvasWidth() - 2 * UserInterface::horizMargin) / 4 * (col - 1)), y, size, string);
}


void drawString4Colf(S32 y, S32 size, U32 col, const char *format, ...)
{
   makeBuffer;
   drawString4Col(y, size, col, buffer);
}


void drawTime(S32 x, S32 y, S32 size, S32 timeInMs, const char *prefixString)
{
   F32 F32time = (F32)timeInMs;

   U32 minsRemaining = U32(F32time / (60 * 1000));
   U32 secsRemaining = U32((F32time - F32(minsRemaining * 60 * 1000)) / 1000);

   drawStringf(x, y, size, "%s%02d:%02d", prefixString, minsRemaining, secsRemaining);
}


S32 getStringWidth(FontContext fontContext, S32 size, const char *string)
{
   FontManager::pushFontContext(fontContext);
   S32 width = getStringWidth(size, string);
   FontManager::popFontContext();

   return width;
}


F32 getStringWidth(FontContext fontContext, F32 size, const char *string)
{
   FontManager::pushFontContext(fontContext);
   F32 width = getStringWidth(size, string);
   FontManager::popFontContext();

   return width;
}


S32 getStringWidth(S32 size, const char *string)
{
   return FontManager::getStringLength(string) * size / 120;
}


F32 getStringWidth(F32 size, const char *string)
{
   return F32(FontManager::getStringLength(string)) * size / 120.0f;
}


F32 getStringWidthf(F32 size, const char *format, ...)
{
   makeBuffer;
   return getStringWidth(size, buffer);
}


S32 getStringWidthf(S32 size, const char *format, ...)
{
   makeBuffer;
   return getStringWidth(size, buffer);
}

#undef makeBuffer


void drawRect(S32 x1, S32 y1, S32 x2, S32 y2, S32 mode)
{
   F32 vertices[] = { (F32)x1, (F32)y1,   (F32)x2, (F32)y1,   (F32)x2, (F32)y2,   (F32)x1, (F32)y2 };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, mode);
}


// Some functions (renderSpyBugVisibleRange) use this F32 version, this function has better accuracy
void drawRect(F32 x1, F32 y1, F32 x2, F32 y2, S32 mode)
{
   F32 vertices[] = { x1, y1,   x2, y1,   x2, y2,   x1, y2 };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, mode);
}


void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2)
{
   drawRect(x1, y1, x2, y2, GL_TRIANGLE_FAN);
}


void drawFilledRect(F32 x1, F32 y1, F32 x2, F32 y2)
{
   drawRect(x1, y1, x2, y2, GL_TRIANGLE_FAN);
}


void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor)
{
   glColor(fillColor);
   drawFilledRect(x1, y1, x2, y2);
}


void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha)
{
   glColor(fillColor, fillAlpha);
   drawFilledRect(x1, y1, x2, y2);
}


void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, const Color &outlineColor)
{
   drawFilledRect(x1, y1, x2, y2, fillColor, 1, outlineColor);
}


void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha, const Color &outlineColor)
{
   glColor(fillColor, fillAlpha);
   drawRect(x1, y1, x2, y2, GL_TRIANGLE_FAN);

   glColor(outlineColor, 1);
   drawRect(x1, y1, x2, y2, GL_LINE_LOOP);
}


void drawHollowRect(const Point &center, S32 width, S32 height)
{
   drawHollowRect(center.x - (F32)width / 2, center.y - (F32)height / 2,
                  center.x + (F32)width / 2, center.y + (F32)height / 2);
}


void drawHollowRect(const Point &p1, const Point &p2)
{
   drawHollowRect(p1.x, p1.y, p2.x, p2.y);
}


void drawFancyBox(F32 xLeft, F32 yTop, F32 xRight, F32 yBottom, F32 cornerInset, S32 mode)
{
   F32 vertices[] = {
         xLeft, yTop,                   // Top
         xRight - cornerInset, yTop,
         xRight, yTop + cornerInset,    // Edge
         xRight, yBottom,               // Bottom
         xLeft + cornerInset, yBottom,
         xLeft, yBottom - cornerInset   // Edge
   };

   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, mode);
}


void drawHollowFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset)
{
   drawFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, GL_LINE_LOOP);
}


void drawFilledFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, const Color &fillColor, F32 fillAlpha, const Color &borderColor)
{
   // Fill
   glColor(fillColor, fillAlpha);
   drawFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, GL_TRIANGLE_FAN);

   // Border
   glColor(borderColor, 1.f);
   drawFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, GL_LINE_LOOP);
}


void renderUpArrow(const Point &center, S32 size)
{
   F32 offset = (F32)size / 2;

   F32 top = center.y - offset;
   F32 bot = center.y + offset;
   F32 capHeight = size * 0.39f;    // An artist need provide no explanation

   F32 vertices[] = { center.x, top,     center.x,             bot, 
                      center.x, top,     center.x - capHeight, top + capHeight,
                      center.x, top,     center.x + capHeight, top + capHeight
                    };

   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINES);
}


void renderDownArrow(const Point &center, S32 size)
{
   F32 offset = (F32)size / 2;
   F32 top = center.y - offset;
   F32 bot = center.y + offset;
   F32 capHeight = size * 0.39f;    // An artist need provide no explanation

   F32 vertices[] = { center.x, top,     center.x,             bot, 
                      center.x, bot,     center.x - capHeight, bot - capHeight,
                      center.x, bot,     center.x + capHeight, bot - capHeight
                    };

   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINES);
}


void renderLeftArrow(const Point &center, S32 size)
{
   F32 offset = (F32)size / 2;
   F32 left = center.x - offset;
   F32 right = center.x + offset;
   F32 capHeight = size * 0.39f;    // An artist need provide no explanation

   F32 vertices[] = { left, center.y,     right,            center.y, 
                      left, center.y,     left + capHeight, center.y - capHeight,
                      left, center.y,     left + capHeight, center.y + capHeight
                    };

   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINES);
}


void renderRightArrow(const Point &center, S32 size)
{
   F32 offset = (F32)size / 2;
   F32 left = center.x - offset;
   F32 right = center.x + offset;
   F32 capHeight = size * 0.39f;    // An artist need provide no explanation

   F32 vertices[] = { left,  center.y,     right,             center.y, 
                      right, center.y,     right - capHeight, center.y - capHeight,
                      right, center.y,     right - capHeight, center.y + capHeight
                    };

   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINES);
}


// Given a string, break it up such that no part is wider than width.  
void wrapString(const string &str, S32 wrapWidth, S32 fontSize, FontContext context, Vector<string> &lines)
{
   FontManager::pushFontContext(context);
   Vector<string> wrapped = wrapString(str, wrapWidth, fontSize);
   FontManager::popFontContext();

   for(S32 i = 0; i < wrapped.size(); i++)
      lines.push_back(wrapped[i]);
}


// Given a string, break it up such that no part is wider than width.  Prefix subsequent lines with indentPrefix.
Vector<string> wrapString(const string &str, S32 wrapWidth, S32 fontSize, const string &indentPrefix)
{
   Vector<string> wrappedLines;

   if(str == "")
      return wrappedLines;

   S32 indent = 0;
   string prefix = "";

   S32 start = 0;
   S32 potentialBreakPoint = start;

   for(U32 i = 0; i < str.length(); i++)
   {
      if(str[i] == '\n')
      {
         wrappedLines.push_back((wrappedLines.size() > 0 ? indentPrefix : "") + str.substr(start, i - start));
         start = i + 1;
         potentialBreakPoint = start + 1;  
      }
      else if(str[i] == ' ')
         potentialBreakPoint = i;
      else if(getStringWidth(fontSize, str.substr(start, i - start + 1).c_str()) > wrapWidth - (wrappedLines.size() > 0 ? indent : 0))
      {
         if(potentialBreakPoint == start)    // No breakpoints were found before string grew too long... will just break here
         {
            wrappedLines.push_back((wrappedLines.size() > 0 ? indentPrefix : "") + str.substr(start, i - start));
            start = i;
            potentialBreakPoint = start;
         }
         else
         {
            wrappedLines.push_back((wrappedLines.size() > 0 ? indentPrefix : "") + str.substr(start, potentialBreakPoint - start));
            potentialBreakPoint++;
            start = potentialBreakPoint;
         }
      }
   }

   if(start != str.length())
      wrappedLines.push_back((wrappedLines.size() > 0 ? indentPrefix : "") + str.substr(start));

   return wrappedLines;
}


S32 getStringPairWidth(S32 size, FontContext leftContext,
      FontContext rightContext, const char* leftStr, const char* rightStr)
{
   S32 leftWidth = getStringWidth(leftContext, size, leftStr);
   S32 spaceWidth = getStringWidth(leftContext, size, " ");
   S32 rightWidth = getStringWidth(rightContext, size, rightStr);

   return leftWidth + spaceWidth + rightWidth;
}


// Returns the number of lines our msg consumed during rendering
U32 drawWrapText(const string &msg, S32 xpos, S32 ypos, S32 width, S32 ypos_end, S32 lineHeight, S32 fontSize, bool draw)
{
   S32 linesDrawn = 0;
   Vector<string> lines = wrapString(msg, width, fontSize);

   // Align the y position, if alignBottom is enabled
   ypos -= lines.size() * lineHeight;     // Align according to number of wrapped lines

   // Draw lines that need to wrap
   for(S32 i = 0; i < lines.size(); i++)
   {
      if(ypos >= ypos_end)      // If there is room to draw some lines at top when aligned bottom
      {
         if(draw)
            drawString(xpos, ypos, fontSize, lines[i].c_str());

         linesDrawn++;
      }
      ypos += lineHeight;
   }

   return linesDrawn;
}


};


