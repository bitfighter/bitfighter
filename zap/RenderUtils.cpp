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

#include <stdarg.h>        // For va_args
#include <stdio.h>         // For vsnprintf


namespace Zap {

static char buffer[2048];     // Reusable buffer
#define makeBuffer    va_list args; va_start(args, format); vsnprintf(buffer, sizeof(buffer), format, args); va_end(args);

// static members
F32 RenderUtils::LINE_WIDTH_1 = 1.0f;
F32 RenderUtils::DEFAULT_LINE_WIDTH = 2.0f;
F32 RenderUtils::LINE_WIDTH_3 = 3.0f;
F32 RenderUtils::LINE_WIDTH_4 = 4.0f;

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


void RenderUtils::glColor(const Color &color, float alpha)
{
   mGL->glColor(color, alpha);
}


void RenderUtils::doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string)
{
   mGL->glPushMatrix();
      mGL->glTranslate(x, y);
      mGL->glRotate(angle * RADIANS_TO_DEGREES);

      FontManager::renderString(size, string);

   mGL->glPopMatrix();
}


// Same but accepts S32 args
//void doDrawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string, bool autoLineWidth)
//{
//   doDrawAngleString(F32(x), F32(y), size, angle, string, autoLineWidth);
//}


// Center text between two points, adjust angle so it's always right-side-up
void RenderUtils::drawStringf_2pt(Point p1, Point p2, F32 size, F32 vert_offset, const char *format, ...)
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
void RenderUtils::drawAngleStringf(F32 x, F32 y, F32 size, F32 angle, const char *format, ...)
{
   makeBuffer;
   doDrawAngleString(x, y, size, angle, buffer);
}


// New, fixed version
void RenderUtils::drawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string)
{
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
   drawString(x, y, size, buffer);
}


void RenderUtils::drawStringf(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
}


void RenderUtils::drawStringf(F32 x, F32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
}


S32 RenderUtils::drawStringfc(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;
   return drawStringc(x, y, (F32)size, buffer);
}


S32 RenderUtils::drawStringfr(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;

   F32 len = getStringWidth(size, buffer);
   doDrawAngleString(x - len, y, size, 0, buffer);

   return S32(len);
}


S32 RenderUtils::drawStringfr(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   return drawStringr(x, y, size, buffer);
}


S32 RenderUtils::drawStringr(S32 x, S32 y, S32 size, const char *string)
{
   F32 len = getStringWidth((F32)size, string);
   doDrawAngleString((F32)x - len, (F32)y + size, (F32)size, 0, string);

   return (S32)len;
}

   
S32 RenderUtils::drawStringAndGetWidth(S32 x, S32 y, S32 size, const char *string)
{
   drawString(x, y, size, string);
   return getStringWidth(size, string);
}


S32 RenderUtils::drawStringAndGetWidth(F32 x, F32 y, S32 size, const char *string)
{
   drawString(x, y, size, string);
   return getStringWidth(size, string);
}


S32 RenderUtils::drawStringAndGetWidthf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
   return getStringWidth(size, buffer);
}


S32 RenderUtils::drawStringAndGetWidthf(F32 x, F32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
   return getStringWidth(size, buffer);
}


S32 RenderUtils::drawStringc(S32 x, S32 y, S32 size, const char *string)
{
   return drawStringc((F32)x, (F32)y, (F32)size, string);
}


// Uses fixed drawAngleString()
S32 RenderUtils::drawStringc(F32 x, F32 y, F32 size, const char *string)
{
   F32 len = getStringWidth(size, string);
   drawAngleString(x - len / 2, y, size, 0, string);

   return (S32)len;
}


S32 RenderUtils::drawStringc(const Point &cen, F32 size, const char *string)
{
   return drawStringc(cen.x, cen.y, size, string);
}


S32 RenderUtils::drawCenteredString_fixed(S32 y, S32 size, const char *string)
{
   return drawCenteredString_fixed(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, y, size, string);
}


// For now, not very fault tolerant...  assumes well balanced []
void RenderUtils::drawCenteredString_highlightKeys(S32 y, S32 size, const string &str, const Color &bodyColor, const Color &keyColor)
{
   S32 len = getStringWidth(size, str.c_str());
   S32 x = DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2 - len / 2;

   std::size_t keyStart, keyEnd = 0;
   S32 pos = 0;

   keyStart = str.find("[");
   while(keyStart != string::npos)
   {
      mGL->glColor(bodyColor);
      x += drawStringAndGetWidth(x, y, size, str.substr(pos, keyStart - pos).c_str());

      keyEnd = str.find("]", keyStart) + 1;     // + 1 to include the "]" itself
      mGL->glColor(keyColor);
      x += drawStringAndGetWidth(x, y, size, str.substr(keyStart, keyEnd - keyStart).c_str());
      pos = keyEnd;

      keyStart = str.find("[", pos);
   }
   
   // Draw any remaining bits of our string
   mGL->glColor(bodyColor);
   drawString(x, y, size, str.substr(keyEnd).c_str());
}


S32 RenderUtils::drawCenteredUnderlinedString(S32 y, S32 size, const char *string)
{
   S32 x = DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2;
   S32 xpos = drawCenteredString(x, y, size, string);
   drawHorizLine(xpos, DisplayManager::getScreenInfo()->getGameCanvasWidth() - xpos, y + size + 5);

   return xpos;
}


S32 RenderUtils::drawCenteredString(S32 x, S32 y, S32 size, const char *string)
{
   S32 xpos = x - getStringWidth(size, string) / 2;
   drawString(xpos, y, size, string);
   return xpos;
}


S32 RenderUtils::drawCenteredString_fixed(S32 x, S32 y, S32 size, const char *string)
{
   S32 xpos = x - getStringWidth(size, string) / 2;
   drawString_fixed(xpos, y, size, string);
   return xpos;
}


F32 RenderUtils::drawCenteredString_fixed(F32 x, F32 y, S32 size, const char *string)
{
   F32 xpos = x - getStringWidth((F32)size, string) / 2;
   drawString_fixed(xpos, y, size, string);
   return xpos;
}


S32 RenderUtils::drawCenteredString_fixed(F32 x, F32 y, S32 size, FontContext fontContext, const char *string)
{
   FontManager::pushFontContext(fontContext);

   F32 xpos = x - getStringWidth(size, string) / 2.0f;
   drawString_fixed(xpos, y, size, string);

   FontManager::popFontContext();

   return (S32)xpos;
}


F32 RenderUtils::drawCenteredString(F32 x, F32 y, S32 size, const char *string)
{
   return drawCenteredString(x, y, F32(size), string);
}


F32 RenderUtils::drawCenteredString(F32 x, F32 y, F32 size, const char *string)
{
   F32 xpos = x - getStringWidth(size, string) / 2;
   drawString(xpos, y, size, string);
   return xpos;
}


S32 RenderUtils::drawCenteredStringf(S32 y, S32 size, const char *format, ...)
{
   makeBuffer; 
   return (S32) drawCenteredString(y, size, buffer);
}


S32 RenderUtils::drawCenteredStringf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString(x, y, size, buffer);
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


S32 RenderUtils::drawCenteredString2Col(S32 y, S32 size, bool leftCol, const char *string)
{
   S32 x = getCenteredString2ColStartingPos(size, leftCol, string);
   drawString(x, y, size, string);
   return x;
}


S32 RenderUtils::drawCenteredString2Colf(S32 y, S32 size, bool leftCol, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString2Col(y, size, leftCol, buffer);
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

S32 RenderUtils::drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, FontContext leftContext, FontContext rightContext, const Color &leftColor, const Color &rightColor,
                                          const char *leftStr, const char *rightStr)
{
   S32 width = getStringPairWidth(size, leftContext, rightContext, leftStr, rightStr);
   return drawStringPair(xpos - width / 2, ypos, size, leftContext, rightContext, leftColor, rightColor, string(leftStr).append(" ").c_str(), rightStr);
}


S32 RenderUtils::drawStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor,
                                         const char *leftStr, const char *rightStr)
{
   mGL->glColor(leftColor);

   // Use crazy width calculation to compensate for fontStash bug calculating with of terminal spaces
   xpos += drawStringAndGetWidth((F32)xpos, (F32)ypos, size, leftStr) + 5; //getStringWidth(size, "X X") - getStringWidth(size, "XX");

   mGL->glColor(rightColor);
   drawString(xpos, ypos, size, rightStr);

   return xpos;
}


S32 RenderUtils::drawStringPair(S32 xpos, S32 ypos, S32 size, FontContext leftContext,
      FontContext rightContext, const Color& leftColor, const Color& rightColor,
      const char* leftStr, const char* rightStr)
{
   FontManager::pushFontContext(leftContext);
   mGL->glColor(leftColor);
   xpos += drawStringAndGetWidth((F32)xpos, (F32)ypos, size, leftStr);
   FontManager::popFontContext();

   FontManager::pushFontContext(rightContext);
   mGL->glColor(rightColor);
   drawString(xpos, ypos, size, rightStr);
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

   mGL->glColor(leftColor);
   drawString(x, y, size, left);

   mGL->glColor(rightColor);
   drawString(x + offset, y, size, right);

   return x;
}


// Draw a left-justified string at column # (1-4)
void RenderUtils::drawString4Col(S32 y, S32 size, U32 col, const char *string)
{
   drawString(UserInterface::horizMargin + ((DisplayManager::getScreenInfo()->getGameCanvasWidth() - 2 * UserInterface::horizMargin) / 4 * (col - 1)), y, size, string);
}


void RenderUtils::drawString4Colf(S32 y, S32 size, U32 col, const char *format, ...)
{
   makeBuffer;
   drawString4Col(y, size, col, buffer);
}


void RenderUtils::drawTime(S32 x, S32 y, S32 size, S32 timeInMs, const char *prefixString)
{
   F32 F32time = (F32)timeInMs;

   U32 minsRemaining = U32(F32time / (60 * 1000));
   U32 secsRemaining = U32((F32time - F32(minsRemaining * 60 * 1000)) / 1000);

   drawStringf(x, y, size, "%s%02d:%02d", prefixString, minsRemaining, secsRemaining);
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


//// Given a string, break it up such that no part is wider than width.  Prefix subsequent lines with indentPrefix.
//Vector<string> wrapString(const string &str, S32 wrapWidth, S32 fontSize, const string indentPrefix)
//{
//   Vector<string> wrappedLines;
//
//   if(str == "")
//      return wrappedLines;
//
//   S32 indent = 0;
//   string prefix = "";
//
//   S32 start = 0;
//   S32 potentialBreakPoint = start;
//
//   for(U32 i = 0; i < str.length(); i++)
//   {
//      if(str[i] == '\n')
//      {
//         wrappedLines.push_back((wrappedLines.size() > 0 ? indentPrefix : "") + str.substr(start, i - start));
//         start = i + 1;
//         potentialBreakPoint = start + 1;
//      }
//      else if(str[i] == ' ')
//         potentialBreakPoint = i;
//      else if(getStringWidth(fontSize, str.substr(start, i - start + 1).c_str()) > wrapWidth - (wrappedLines.size() > 0 ? indent : 0))
//      {
//         if(potentialBreakPoint == start)    // No breakpoints were found before string grew too long... will just break here
//         {
//            wrappedLines.push_back((wrappedLines.size() > 0 ? indentPrefix : "") + str.substr(start, i - start));
//            start = i;
//            potentialBreakPoint = start;
//         }
//         else
//         {
//            wrappedLines.push_back((wrappedLines.size() > 0 ? indentPrefix : "") + str.substr(start, potentialBreakPoint - start));
//            potentialBreakPoint++;
//            start = potentialBreakPoint;
//         }
//      }
//   }
//
//   if(start != (S32)str.length())
//      wrappedLines.push_back((wrappedLines.size() > 0 ? indentPrefix : "") + str.substr(start));
//
//   return wrappedLines;
//}


S32 RenderUtils::getStringPairWidth(S32 size, FontContext leftContext,
      FontContext rightContext, const char* leftStr, const char* rightStr)
{
   S32 leftWidth = getStringWidth(leftContext, size, leftStr);
   S32 spaceWidth = getStringWidth(leftContext, size, " ");
   S32 rightWidth = getStringWidth(rightContext, size, rightStr);

   return leftWidth + spaceWidth + rightWidth;
}


// Returns the number of lines our msg consumed during rendering
U32 RenderUtils::drawWrapText(const string &msg, S32 xpos, S32 ypos, S32 width, S32 ypos_end, S32 lineHeight, S32 fontSize, bool draw)
{
   S32 linesDrawn = 0;
   Vector<string> lines = Zap::wrapString(msg, width, fontSize);

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


void RenderUtils::drawLetter(char letter, const Point &pos, const Color &color, F32 alpha)
{
   // Mark the item with a letter, unless we're showing the reference ship
   F32 vertOffset = 8;
   if (letter >= 'a' && letter <= 'z')    // Better positioning for lowercase letters
      vertOffset = 10;

   mGL->glColor(color, alpha);
   F32 xpos = pos.x - getStringWidthf(15, "%c", letter) / 2;

   drawStringf(xpos, pos.y - vertOffset, 15, "%c", letter);
}


void RenderUtils::drawLine(const Vector<Point> *points)
{
   mGL->renderPointVector(points, GLOPT::LineStrip);
}


void RenderUtils::drawLine(const Vector<Point> *points, const Color &color)
{
   mGL->glColor(color);
   mGL->renderPointVector(points, GLOPT::LineStrip);
}


void RenderUtils::drawHorizLine(S32 x1, S32 x2, S32 y)
{
   drawHorizLine((F32)x1, (F32)x2, (F32)y);
}


void RenderUtils::drawVertLine(S32 x, S32 y1, S32 y2)
{
   drawVertLine((F32)x, (F32)y1, (F32)y2);
}


void RenderUtils::drawHorizLine(F32 x1, F32 x2, F32 y)
{
   F32 vertices[] = { x1, y,   x2, y };
   mGL->renderVertexArray(vertices, 2, GLOPT::Lines);
}


void RenderUtils::drawVertLine(F32 x, F32 y1, F32 y2)
{
   F32 vertices[] = { x, y1,   x, y2 };
   mGL->renderVertexArray(vertices, 2, GLOPT::Lines);
}


void RenderUtils::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor)
{
   mGL->glColor(fillColor);
   drawFilledRect(x1, y1, x2, y2);
}


void RenderUtils::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha)
{
   mGL->glColor(fillColor, fillAlpha);
   drawFilledRect(x1, y1, x2, y2);
}


void RenderUtils::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, const Color &outlineColor)
{
   drawFilledRect(x1, y1, x2, y2, fillColor, 1, outlineColor, 1);
}


void RenderUtils::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha, const Color &outlineColor)
{
   drawFilledRect(x1, y1, x2, y2, fillColor, fillAlpha, outlineColor, 1);
}


void RenderUtils::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha, const Color &outlineColor, F32 outlineAlpha)
{
   mGL->glColor(fillColor, fillAlpha);
   drawRect(x1, y1, x2, y2, GLOPT::TriangleFan);

   mGL->glColor(outlineColor, outlineAlpha);
   drawRect(x1, y1, x2, y2, GLOPT::LineLoop);
}


void RenderUtils::drawHollowRect(const Point &center, S32 width, S32 height)
{
   drawHollowRect(center.x - (F32)width / 2, center.y - (F32)height / 2,
                  center.x + (F32)width / 2, center.y + (F32)height / 2);
}


void RenderUtils::drawHollowRect(const Point &p1, const Point &p2)
{
   drawHollowRect(p1.x, p1.y, p2.x, p2.y);
}


void RenderUtils::drawFancyBox(F32 xLeft, F32 yTop, F32 xRight, F32 yBottom, F32 cornerInset, S32 mode)
{
   F32 vertices[] = {
         xLeft, yTop,                   // Top
         xRight - cornerInset, yTop,
         xRight, yTop + cornerInset,    // Edge
         xRight, yBottom,               // Bottom
         xLeft + cornerInset, yBottom,
         xLeft, yBottom - cornerInset   // Edge
   };

   mGL->renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, mode);
}


void RenderUtils::drawHollowFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset)
{
   drawFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, GLOPT::LineLoop);
}


void RenderUtils::drawFilledFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, const Color &fillColor, F32 fillAlpha, const Color &borderColor)
{
   // Fill
   mGL->glColor(fillColor, fillAlpha);
   drawFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, GLOPT::TriangleFan);

   // Border
   mGL->glColor(borderColor, 1.f);
   drawFancyBox(xLeft, yTop, xRight, yBottom, cornerInset, GLOPT::LineLoop);
}


// Draw arc centered on pos, with given radius, from startAngle to endAngle.  0 is East, increasing CW
void RenderUtils::drawArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle)
{
   static Vector<Point> points;     // Reusable container

   // The +1 just makes the curves I've looked at appear nicer
   S32 numPoints = (S32)ceil(NUM_CIRCLE_SIDES * (endAngle - startAngle) / FloatTau) + 1;

   TNLAssert(numPoints >= 0, "Negative points???");

   generatePointsInACurve(startAngle, endAngle, numPoints, radius, points);

   mGL->glPushMatrix();
      mGL->glTranslate(pos);
      mGL->renderPointVector(&points, GLOPT::LineStrip);
   mGL->glPopMatrix();
}


void RenderUtils::drawDashedArc(const Point &center, F32 radius, F32 arcTheta, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle)
{
   F32 interimAngle = arcTheta / dashCount;

   for(S32 i = 0; i < dashCount; i++)
      drawArc(center, radius, interimAngle * i + offsetAngle, (interimAngle * (i + 1)) - dashSpaceCentralAngle + offsetAngle);
}


void RenderUtils::drawDashedCircle(const Point &center, F32 radius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle)
{
   drawDashedArc(center, radius, FloatTau, dashCount, dashSpaceCentralAngle, offsetAngle);
}


void RenderUtils::drawAngledRay(const Point &center, F32 innerRadius, F32 outerRadius, F32 angle)
{
   F32 vertices[] = {
         center.x + cos(angle) * innerRadius, center.y + sin(angle) * innerRadius,
         center.x + cos(angle) * outerRadius, center.y + sin(angle) * outerRadius,
   };

   mGL->renderVertexArray(vertices, 2, GLOPT::LineStrip);
}


void RenderUtils::drawAngledRayCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 rayCount, F32 offsetAngle)
{
   drawAngledRayArc(center, innerRadius, outerRadius, FloatTau, rayCount, offsetAngle);
}


void RenderUtils::drawAngledRayArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, S32 rayCount, F32 offsetAngle)
{
   F32 interimAngle = centralAngle / rayCount;

   for(S32 i = 0; i < rayCount; i++)
      drawAngledRay(center, innerRadius, outerRadius, interimAngle * i + offsetAngle);
}


void RenderUtils::drawDashedHollowCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle)
{
   // Draw the dashed circles
   drawDashedCircle(center, innerRadius, dashCount, dashSpaceCentralAngle, offsetAngle);
   drawDashedCircle(center, outerRadius, dashCount, dashSpaceCentralAngle, offsetAngle);

   // Now connect them
   drawAngledRayCircle(center, innerRadius,  outerRadius, dashCount, offsetAngle);
   drawAngledRayCircle(center, innerRadius,  outerRadius, dashCount, offsetAngle - dashSpaceCentralAngle);
}


void RenderUtils::drawHollowArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, F32 offsetAngle)
{
   drawAngledRay(center, innerRadius, outerRadius, offsetAngle);
   drawAngledRay(center, innerRadius, outerRadius, offsetAngle + centralAngle);

   drawArc(center, innerRadius, offsetAngle, offsetAngle + centralAngle);
   drawArc(center, outerRadius, offsetAngle, offsetAngle + centralAngle);
}


void RenderUtils::drawRoundedRect(const Point &pos, S32 width, S32 height, S32 rad)
{
   drawRoundedRect(pos, (F32)width, (F32)height, (F32)rad);
}


// Draw rounded rectangle centered on pos
void RenderUtils::drawRoundedRect(const Point &pos, F32 width, F32 height, F32 rad)
{
   Point p;

   // First the main body of the rect, start in UL, proceed CW
   F32 width2  = width  / 2;
   F32 height2 = height / 2;

   F32 vertices[] = {
         pos.x - width2 + rad, pos.y - height2,
         pos.x + width2 - rad, pos.y - height2,

         pos.x + width2, pos.y - height2 + rad,
         pos.x + width2, pos.y + height2 - rad,

         pos.x + width2 - rad, pos.y + height2,
         pos.x - width2 + rad, pos.y + height2,

         pos.x - width2, pos.y + height2 - rad,
         pos.x - width2, pos.y - height2 + rad
   };

   mGL->renderVertexArray(vertices, 8, GLOPT::Lines);

   // Now add some quarter-rounds in the corners, start in UL, proceed CW
   p.set(pos.x - width2 + rad, pos.y - height2 + rad);
   drawArc(p, rad, -FloatPi, -FloatHalfPi);

   p.set(pos.x + width2 - rad, pos.y - height2 + rad);
   drawArc(p, rad, -FloatHalfPi, 0);

   p.set(pos.x + width2 - rad, pos.y + height2 - rad);
   drawArc(p, rad, 0, FloatHalfPi);

   p.set(pos.x - width2 + rad, pos.y + height2 - rad);
   drawArc(p, rad, FloatHalfPi, FloatPi);
}


void RenderUtils::drawFilledArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle)
{
   // With theta delta of 0.2, that means maximum 32 points + 2 at the end
   const S32 MAX_POINTS = 32 + 2;
   static F32 filledArcVertexArray[MAX_POINTS * 2];      // 2 components per point

   U32 count = 0;

   for(F32 theta = startAngle; theta < endAngle; theta += CIRCLE_SIDE_THETA)
   {
      filledArcVertexArray[2*count]       = pos.x + cos(theta) * radius;
      filledArcVertexArray[(2*count) + 1] = pos.y + sin(theta) * radius;
      count++;
   }

   // Make sure arc makes it all the way to endAngle...  rounding errors look terrible!
   filledArcVertexArray[2*count]       = pos.x + cos(endAngle) * radius;
   filledArcVertexArray[(2*count) + 1] = pos.y + sin(endAngle) * radius;
   count++;

   filledArcVertexArray[2*count]       = pos.x;
   filledArcVertexArray[(2*count) + 1] = pos.y;
   count++;

   mGL->renderVertexArray(filledArcVertexArray, count, GLOPT::TriangleFan);
}


void RenderUtils::drawFilledRoundedRect(const Point &pos, S32 width, S32 height, const Color &fillColor, const Color &outlineColor, S32 radius, F32 alpha)
{
   drawFilledRoundedRect(pos, (F32)width, (F32)height, fillColor, outlineColor, (F32)radius, alpha);
}


void RenderUtils::drawFilledRoundedRect(const Point &pos, F32 width, F32 height, const Color &fillColor, const Color &outlineColor, F32 radius, F32 alpha)
{
   mGL->glColor(fillColor, alpha);

   drawFilledArc(Point(pos.x - width / 2 + radius, pos.y - height / 2 + radius), radius,      FloatPi, FloatPi + FloatHalfPi);
   drawFilledArc(Point(pos.x + width / 2 - radius, pos.y - height / 2 + radius), radius, -FloatHalfPi, 0);
   drawFilledArc(Point(pos.x + width / 2 - radius, pos.y + height / 2 - radius), radius,            0, FloatHalfPi);
   drawFilledArc(Point(pos.x - width / 2 + radius, pos.y + height / 2 - radius), radius,  FloatHalfPi, FloatPi);

   drawRect(pos.x - width / 2, pos.y - height / 2 + radius,
            pos.x + width / 2, pos.y + height / 2 - radius, GLOPT::TriangleFan);

   drawRect(pos.x - width / 2 + radius, pos.y - height / 2,
            pos.x + width / 2 - radius, pos.y - height / 2 + radius, GLOPT::TriangleFan);

   drawRect(pos.x - width / 2 + radius, pos.y + height / 2,
            pos.x + width / 2 - radius, pos.y + height / 2 - radius, GLOPT::TriangleFan);

   mGL->glColor(outlineColor, alpha);
   drawRoundedRect(pos, width, height, radius);
}


// Actually draw the ellipse
void RenderUtils::drawFilledEllipseUtil(const Point &pos, F32 width, F32 height, F32 angle, U32 glStyle)
{
   F32 sinbeta = sin(angle);
   F32 cosbeta = cos(angle);

   // 32 vertices to fake our ellipse
   F32 vertexArray[64];
   U32 count = 0;
   for(F32 theta = 0; theta < FloatTau; theta += CIRCLE_SIDE_THETA)
   {
      F32 sinalpha = sin(theta);
      F32 cosalpha = cos(theta);

      vertexArray[2*count]     = pos.x + (width * cosalpha * cosbeta - height * sinalpha * sinbeta);
      vertexArray[(2*count)+1] = pos.y + (width * cosalpha * sinbeta + height * sinalpha * cosbeta);
      count++;
   }

   mGL->renderVertexArray(vertexArray, ARRAYSIZE(vertexArray) / 2, glStyle);
}


// Draw an n-sided polygon
void RenderUtils::drawPolygon(S32 sides, F32 radius, F32 angle)
{
   Vector<Point> points;
   generatePointsInACurve(angle, angle + FloatTau, sides + 1, radius, points);   // +1 so we can "close the loop"

   mGL->renderPointVector(&points, GLOPT::LineStrip);
}


// Draw an n-sided polygon
void RenderUtils::drawPolygon(const Point &pos, S32 sides, F32 radius, F32 angle)
{
   mGL->glPushMatrix();
      mGL->glTranslate(pos);
      drawPolygon(sides, radius, angle);
   mGL->glPopMatrix();
}


void RenderUtils::drawStar(const Point &pos, S32 points, F32 radius, F32 innerRadius)
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

   mGL->renderPointVector(&pts, GLOPT::LineLoop);
}


void RenderUtils::drawFilledStar(const Point &pos, S32 points, F32 radius, F32 innerRadius)
{
   F32 ang = FloatTau / F32(points * 2);
   F32 a = ang / 2;
   F32 r = innerRadius;
   bool inout = false;

   static Point p;
   static Point first;

   static Vector<Point> pts;
   static Vector<Point> core;
   static Vector<Point> outline;

   pts.clear();
   core.clear();
   outline.clear();

   for(S32 i = 0; i < points * 2; i++)
   {
      p.set(r * cos(a) + pos.x, r * sin(a) + pos.y);

      outline.push_back(p);

      if(i == 0)
      {
         first = p;
         core.push_back(p);
      }
      else if(i % 2 == 0)
      {
         pts.push_back(p);
         core.push_back(p);
      }

      pts.push_back(p);

      a += ang;
      inout = !inout;
      r = inout ? radius : innerRadius;
   }

   pts.push_back(first);

   mGL->renderPointVector(&pts, GLOPT::Triangles);       // Points
   mGL->renderPointVector(&core, GLOPT::TriangleFan);        // Inner pentagon
   mGL->renderPointVector(&outline, GLOPT::LineLoop);   // Outline to make things look smoother, at least when star is small
}


// Draw an ellipse at pos, with axes width and height, canted at angle
void RenderUtils::drawEllipse(const Point &pos, F32 width, F32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, width, height, angle, GLOPT::LineLoop);
}


// Draw an ellipse at pos, with axes width and height, canted at angle
void RenderUtils::drawEllipse(const Point &pos, S32 width, S32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, (F32)width, (F32)height, angle, GLOPT::LineLoop);
}


// Well...  draws a filled ellipse, much as you'd expect
void RenderUtils::drawFilledEllipse(const Point &pos, F32 width, F32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, width, height, angle, GLOPT::TriangleFan);
}


void RenderUtils::drawFilledEllipse(const Point &pos, S32 width, S32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, (F32)width, (F32)height, angle, GLOPT::TriangleFan);
}


void RenderUtils::drawFilledCircle(const Point &pos, F32 radius)
{
   drawFilledSector(pos, radius, 0, FloatTau);
}


void RenderUtils::drawFilledCircle(const Point &pos, F32 radius, const Color &color)
{
   mGL->glColor(color);
   drawFilledCircle(pos, radius);
}


void RenderUtils::drawFilledSector(const Point &pos, F32 radius, F32 start, F32 end)
{
   // With theta delta of 0.2, that means maximum 32 points
   static const S32 MAX_POINTS = 32;
   static F32 filledSectorVertexArray[MAX_POINTS * 2];      // 2 components per point

   U32 count = 0;

   for(F32 theta = start; theta < end; theta += CIRCLE_SIDE_THETA)
   {
      filledSectorVertexArray[2 * count]     = pos.x + cos(theta) * radius;
      filledSectorVertexArray[2 * count + 1] = pos.y + sin(theta) * radius;
      count++;
   }

   mGL->renderVertexArray(filledSectorVertexArray, count, GLOPT::TriangleFan);
}


// Pos is the square's center
void RenderUtils::drawSquare(const Point &pos, F32 radius, bool filled)
{
   drawRect(pos.x - radius, pos.y - radius, pos.x + radius, pos.y + radius, filled ? GLOPT::TriangleFan : GLOPT::LineLoop);
}


void RenderUtils::drawSquare(const Point &pos, S32 radius, bool filled)
{
    drawSquare(pos, F32(radius), filled);
}


void RenderUtils::drawHollowSquare(const Point &pos, F32 radius)
{
   drawSquare(pos, radius, false);
}


void RenderUtils::drawHollowSquare(const Point &pos, F32 radius, const Color &color)
{
   mGL->glColor(color);
   drawHollowSquare(pos, radius);
}


void RenderUtils::drawFilledSquare(const Point &pos, F32 radius)
{
   drawSquare(pos, radius, true);

}


void RenderUtils::drawFilledSquare(const Point &pos, F32 radius, const Color &color)
{
   mGL->glColor(color);
   drawFilledSquare(pos, radius);
}


void RenderUtils::drawCircle(const Point &center, F32 radius, const Color *color, F32 alpha)
{
   mGL->glPushMatrix();
      mGL->glTranslate(center);
      drawCircle(radius, color, alpha);
   mGL->glPopMatrix();
}


// Circle drawing now precalculates a set of points around the circle, then renders them using mGL->glScale()
// to get the radius right.  This eliminates most point calculations during rendering, making it much
// more efficient.
void RenderUtils::drawCircle(F32 radius, const Color *color, F32 alpha)
{
   static Vector<Point> points;

   // Lazily initialize point array
   if(points.size() == 0)
      generatePointsInACircle(NUM_CIRCLE_SIDES, 1.0, points);

   if(color)
      mGL->glColor(color, alpha);

   mGL->glPushMatrix();
      mGL->glScale(radius);
      mGL->renderPointVector(&points, GLOPT::LineStrip);
   mGL->glPopMatrix();
}


void RenderUtils::drawFadingHorizontalLine(S32 x1, S32 x2, S32 yPos, const Color &color)
{
   F32 vertices[] = {
         (F32)x1, (F32)yPos,
         (F32)x2, (F32)yPos
   };

   F32 colors[] = {
         color.r, color.g, color.b, 1,
         color.r, color.g, color.b, 0,
   };

   mGL->renderColorVertexArray(vertices, colors, ARRAYSIZE(vertices) / 2, GLOPT::Lines);
}


};

