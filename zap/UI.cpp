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
#include "tnl.h"

using namespace TNL;

#include "UI.h"
#include "move.h"
#include "InputCode.h"
#include "UIMenus.h"
#include "UIDiagnostics.h"
#include "UIChat.h"
#include "input.h"               // For MaxJoystickButtons const
#include "config.h"
#include "ClientGame.h"
#include "Console.h"             // For console rendering
#include "Colors.h"
#include "OpenglUtils.h"
#include "ScreenInfo.h"
#include "Joystick.h"
#include "masterConnection.h"    // For MasterServerConnection def
#include "VideoSystem.h"

#include <string>
#include <stdarg.h>              // For va_args

#include <math.h>


using namespace std;
namespace Zap
{

#ifdef TNL_OS_XBOX
   S32 UserInterface::horizMargin = 50;
   S32 UserInterface::vertMargin = 38;
#else
   S32 UserInterface::horizMargin = 15;
   S32 UserInterface::vertMargin = 15;
#endif

extern S32 gLoadoutIndicatorHeight;

S32 UserInterface::messageMargin = UserInterface::vertMargin + gLoadoutIndicatorHeight + 5;

extern Vector<ClientGame *> gClientGames;

F32 gLineWidth1 = 1.0f;
F32 gDefaultLineWidth = 2.0f;
F32 gLineWidth3 = 3.0f;
F32 gLineWidth4 = 4.0f;


////////////////////////////////////////
////////////////////////////////////////

// Declare statics
bool UserInterface::mDisableShipKeyboardInput = true;


// Constructor
UserInterface::UserInterface(ClientGame *clientGame)
{
   mClientGame = clientGame;
   mTimeSinceLastInput = 0;
}


UserInterface::~UserInterface()
{
   // Do nothing
}


ClientGame *UserInterface::getGame()
{
   return mClientGame;
}


UIManager *UserInterface::getUIManager() const 
{ 
   TNLAssert(mClientGame, "mGame is NULL!");
   return mClientGame->getUIManager(); 
}


bool UserInterface::usesEditorScreenMode()
{
   return false;
}


void UserInterface::activate()
{
   onActivate(); 
}


void UserInterface::reactivate()
{
   //comingFrom = current;
   //current = this;
   onReactivate();
}


// Set interface's name.  This name is used internally only for debugging
// and to identify interfaces when searching for matches.  Each interface
// should have a unique id.

void UserInterface::setMenuID(UIID menuID)
{
   mInternalMenuID = menuID;
}


// Retrieve interface's id
UIID UserInterface::getMenuID() const
{
   return mInternalMenuID;
}


// Retrieve previous interface's id
UIID UserInterface::getPrevMenuID() const
{
   if(getUIManager()->hasPrevUI())
      return getUIManager()->getPrevUI()->mInternalMenuID;
   else
      return InvalidUI;
}


void UserInterface::onActivate()          { /* Do nothing */ }
void UserInterface::onReactivate()        { /* Do nothing */ }
void UserInterface::onDisplayModeChange() { /* Do nothing */ }


void UserInterface::onDeactivate(bool prevUIUsesEditorScreenMode) 
{
   if(prevUIUsesEditorScreenMode != usesEditorScreenMode())
      VideoSystem::actualizeScreenMode(getGame()->getSettings(), true, usesEditorScreenMode());
}


U32 UserInterface::getTimeSinceLastInput()
{
   return mTimeSinceLastInput;
}


extern const F32 radiansToDegreesConversion;

#define makeBuffer    va_list args; va_start(args, format); char buffer[2048]; vsnprintf(buffer, sizeof(buffer), format, args); va_end(args);


// Center text between two points, adjust angle so it's always right-side-up
void UserInterface::drawStringf_2pt(Point p1, Point p2, F32 size, F32 vert_offset, const char *format, ...)
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
void UserInterface::drawAngleStringf(F32 x, F32 y, F32 size, F32 angle, const char *format, ...)
{
   makeBuffer;
   doDrawAngleString((S32) x, (S32) y, size, angle, buffer);
}


// New, fixed version
void UserInterface::drawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string, bool autoLineWidth)
{
   doDrawAngleString(x, y, size, angle, string, autoLineWidth);
}


void UserInterface::doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string, bool autoLineWidth)
{
   static F32 modelview[16];
   glGetFloatv(GL_MODELVIEW_MATRIX, modelview);    // Fills modelview[]

   if(autoLineWidth)
   {
      F32 linewidth = MAX(MIN(size * gScreenInfo.getPixelRatio() * modelview[0] / 20, 1.0f), 0.5f)    // Clamp to range of 0.5 - 1
                                                                           * gDefaultLineWidth;       // then multiply by line width (2 by default)
      glLineWidth(linewidth);
   }

   F32 scaleFactor = size / 120.0f;
   glPushMatrix();
      glTranslatef(x, y, 0);
      glRotatef(angle * radiansToDegreesConversion, 0, 0, 1);
      glScalef(scaleFactor, -scaleFactor, 1);
      for(S32 i = 0; string[i]; i++)
         OpenglUtils::drawCharacter(string[i]);
   glPopMatrix();

   if(autoLineWidth)
      glLineWidth(gDefaultLineWidth);
}


// Same but accepts S32 args
void UserInterface::doDrawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string, bool autoLineWidth)
{
   doDrawAngleString(F32(x), F32(y), size, angle, string, autoLineWidth);
}


void UserInterface::drawString(S32 x, S32 y, S32 size, const char *string)
{
   y += size;     // TODO: Adjust all callers so we can get rid of this!
   drawAngleString(F32(x), F32(y), F32(size), 0, string);
}


void UserInterface::drawString(F32 x, F32 y, S32 size, const char *string)
{
   y += size;     // TODO: Adjust all callers so we can get rid of this!
   drawAngleString(x, y, F32(size), 0, string);
}


void UserInterface::drawString(F32 x, F32 y, F32 size, const char *string)
{
   y += size;     // TODO: Adjust all callers so we can get rid of this!
   drawAngleString(x, y, size, 0, string);
}


void UserInterface::drawStringf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
}


void UserInterface::drawStringf(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
}


void UserInterface::drawStringf(F32 x, F32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
}


void UserInterface::drawStringfc(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;
   drawStringc(x, y, (F32)size, buffer);
}


S32 UserInterface::drawStringfr(F32 x, F32 y, F32 size, const char *format, ...)
{
   makeBuffer;

   F32 len = getStringWidth(size, buffer);
   doDrawAngleString(x - len, y, size, 0, buffer);

   return S32(len);
}


S32 UserInterface::drawStringfr(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   return drawStringr(x, y, size, buffer);
}


S32 UserInterface::drawStringr(S32 x, S32 y, S32 size, const char *string)
{
   S32 len = getStringWidth(size, string);
   doDrawAngleString(x - len, y + size, (F32)size, 0, string);

   return len;
}

   
S32 UserInterface::drawStringAndGetWidth(S32 x, S32 y, S32 size, const char *string)
{
   drawString(x, y, size, string);
   return getStringWidth(size, string);
}


S32 UserInterface::drawStringAndGetWidth(F32 x, F32 y, S32 size, const char *string)
{
   drawString(x, y, size, string);
   return getStringWidth(size, string);
}


S32 UserInterface::drawStringAndGetWidthf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
   return getStringWidth(size, buffer);
}


S32 UserInterface::drawStringAndGetWidthf(F32 x, F32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
   return getStringWidth(size, buffer);
}


void UserInterface::drawStringc(S32 x, S32 y, S32 size, const char *string)
{
   drawStringc((F32)x, (F32)y, (F32)size, string);
}


void UserInterface::drawStringc(F32 x, F32 y, F32 size, const char *string, bool autoLineWidth)
{
   F32 len = getStringWidth(size, string);
   drawAngleString(x - len / 2, y, size, 0, string, autoLineWidth);
}


S32 UserInterface::drawCenteredString(S32 y, S32 size, const char *string)
{
   return drawCenteredString(gScreenInfo.getGameCanvasWidth() / 2, y, size, string);
}


extern void drawHorizLine(S32 x1, S32 x2, S32 y);

S32 UserInterface::drawCenteredUnderlinedString(S32 y, S32 size, const char *string)
{
   S32 x = gScreenInfo.getGameCanvasWidth() / 2;
   S32 xpos = drawCenteredString(x, y, size, string);
   drawHorizLine(xpos, gScreenInfo.getGameCanvasWidth() - xpos, y + size + 5);

   return xpos;
}


S32 UserInterface::drawCenteredString(S32 x, S32 y, S32 size, const char *string)
{
   S32 xpos = x - getStringWidth(size, string) / 2;
   drawString(xpos, y, size, string);
   return xpos;
}


F32 UserInterface::drawCenteredString(F32 x, F32 y, S32 size, const char *string)
{
   return drawCenteredString(x, y, F32(size), string);
}


F32 UserInterface::drawCenteredString(F32 x, F32 y, F32 size, const char *string)
{
   F32 xpos = x - getStringWidth(size, string) / 2;
   drawString(xpos, y, size, string);
   return xpos;
}


S32 UserInterface::drawCenteredStringf(S32 y, S32 size, const char *format, ...)
{
   makeBuffer; 
   return drawCenteredString(y, size, buffer);
}


S32 UserInterface::drawCenteredStringf(S32 x, S32 y, S32 size, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString(x, y, size, buffer);
}


// Figure out the first position of our CenteredString
S32 UserInterface::getCenteredStringStartingPos(S32 size, const char *string)
{
   S32 x = gScreenInfo.getGameCanvasWidth() / 2;      // x must be S32 in case it leaks off left side of screen
   x -= getStringWidth(size, string) / 2;

   return x;
}


S32 UserInterface::getCenteredStringStartingPosf(S32 size, const char *format, ...)
{
   makeBuffer;
   return getCenteredStringStartingPos(size, buffer);
}


// Figure out the first position of our 2ColCenteredString
S32 UserInterface::getCenteredString2ColStartingPos(S32 size, bool leftCol, const char *string)
{
   return get2ColStartingPos(leftCol) - getStringWidth(size, string) / 2;
}


S32 UserInterface::getCenteredString2ColStartingPosf(S32 size, bool leftCol, const char *format, ...)
{
   makeBuffer;
   return getCenteredString2ColStartingPos(size, leftCol, buffer);
}


S32 UserInterface::drawCenteredString2Col(S32 y, S32 size, bool leftCol, const char *string)
{
   S32 x = getCenteredString2ColStartingPos(size, leftCol, string);
   drawString(x, y, size, string);
   return x;
}


S32 UserInterface::drawCenteredString2Colf(S32 y, S32 size, bool leftCol, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString2Col(y, size, leftCol, buffer);
}

   
S32 UserInterface::get2ColStartingPos(bool leftCol)      // Must be S32 to avoid problems downstream
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   return leftCol ? (canvasWidth / 4) : (canvasWidth - (canvasWidth / 4));
}


//extern void glColor(const Color &c, float alpha = 1.0);

// Returns starting position of value, which is useful for positioning the cursor in an editable menu entry
S32 UserInterface::drawCenteredStringPair(S32 ypos, S32 size, const Color &leftColor, const Color &rightColor, 
                                          const char *leftStr, const char *rightStr)
{
   return drawCenteredStringPair(gScreenInfo.getGameCanvasWidth() / 2, ypos, size, leftColor, rightColor, leftStr, rightStr);
}


// Returns starting position of value, which is useful for positioning the cursor in an editable menu entry
S32 UserInterface::drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor, 
                                          const char *leftStr, const char *rightStr)
{
   S32 xpos2 = getCenteredStringStartingPosf(size, "%s %s", leftStr, rightStr) + xpos - gScreenInfo.getGameCanvasWidth() / 2;

   glColor(leftColor);
   xpos2 += UserInterface::drawStringAndGetWidthf((F32)xpos2, (F32)ypos, size, "%s ", leftStr);

   glColor(rightColor);
   UserInterface::drawString(xpos2, ypos, size, rightStr);

   return xpos2;
}


S32 UserInterface::getStringPairWidth(S32 size, const char *leftStr, const char *rightStr)
{
   return getStringWidthf(size, "%s %s", leftStr, rightStr);
}


//// Draws a string centered on the screen, with different parts colored differently
//S32 UserInterface::drawCenteredStringPair(S32 y, U32 size, const Color &col1, const Color &col2, const char *left, const char *right)
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


S32 UserInterface::drawCenteredStringPair2Colf(S32 y, S32 size, bool leftCol, const char *left, const char *format, ...)
{
   makeBuffer;
   return drawCenteredStringPair2Col(y, size, leftCol, Colors::white, Colors::cyan, left, buffer);
}


S32 UserInterface::drawCenteredStringPair2Colf(S32 y, S32 size, bool leftCol, const Color &leftColor, const Color &rightColor,
      const char *left, const char *format, ...)
{
   makeBuffer;
   return drawCenteredStringPair2Col(y, size, leftCol, leftColor, rightColor, left, buffer);
}

// Draws a string centered in the left or right half of the screen, with different parts colored differently
S32 UserInterface::drawCenteredStringPair2Col(S32 y, S32 size, bool leftCol, const Color &leftColor, const Color &rightColor,
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
void UserInterface::drawString4Col(S32 y, S32 size, U32 col, const char *string)
{
   drawString(horizMargin + ((gScreenInfo.getGameCanvasWidth() - 2 * horizMargin) / 4 * (col - 1)), y, size, string);
}


void UserInterface::drawString4Colf(S32 y, S32 size, U32 col, const char *format, ...)
{
   makeBuffer;
   drawString4Col(y, size, col, buffer);
}


void UserInterface::drawTime(S32 x, S32 y, S32 size, S32 timeInMs, const char *prefixString)
{
   F32 F32time = (F32)timeInMs;

   U32 minsRemaining = U32(F32time / (60 * 1000));
   U32 secsRemaining = U32((F32time - F32(minsRemaining * 60 * 1000)) / 1000);

   drawStringf(x, y, size, "%s%02d:%02d", prefixString, minsRemaining, secsRemaining);
}



#ifndef ZAP_DEDICATED
S32 UserInterface::getStringWidth(S32 size, const char *string)
{
   return OpenglUtils::getStringLength((const unsigned char *) string) * size / 120;
}


F32 UserInterface::getStringWidth(F32 size, const char *string)
{
   return F32( OpenglUtils::getStringLength((const unsigned char *) string) ) * size / 120.f;
}

#else

S32 UserInterface::getStringWidth(S32 size, const char *string)
{
   return 1;
}


F32 UserInterface::getStringWidth(F32 size, const char *string)
{
   return 1;
}
#endif


F32 UserInterface::getStringWidthf(F32 size, const char *format, ...)
{
   makeBuffer;
   return getStringWidth(size, buffer);
}


S32 UserInterface::getStringWidthf(S32 size, const char *format, ...)
{
   makeBuffer;
   return getStringWidth(size, buffer);
}

#undef makeBuffer


void UserInterface::playBoop()
{
   SoundSystem::playSoundEffect(SFXUIBoop, 1);
}


// Render master connection state if we're not connected
void UserInterface::renderMasterStatus()
{
   MasterServerConnection *conn = mClientGame->getConnectionToMaster();

   if(conn && conn->getConnectionState() != NetConnection::Connected)
   {
      glColor(Colors::white);
      UserInterface::drawStringf(10, 550, 15, "Master Server - %s", GameConnection::getConnectionStateString(conn->getConnectionState()));
   }
}


void UserInterface::renderConsole()
{
#ifndef BF_NO_CONSOLE
   // Temporarily disable scissors mode so we can use the full width of the screen
   // to show our console text, black bars be damned!
   bool scissorMode = glIsEnabled(GL_SCISSOR_TEST);

   if(scissorMode) 
      glDisable(GL_SCISSOR_TEST);

   gConsole.render();

   if(scissorMode) 
      glEnable(GL_SCISSOR_TEST);
#endif
}


//extern void glColor(const Color &c, float alpha = 1);
extern ScreenInfo gScreenInfo;

void UserInterface::renderMessageBox(const char *title, const char *instr, string message[], S32 msgLines, S32 vertOffset, S32 style)
{
   const S32 canvasWidth  = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   S32 inset = 100;                    // Inset for left and right edges of box
   const S32 titleSize = 30;           // Size of title
   const S32 titleGap = titleSize / 3; // Spacing between title and first line of text

   S32 textSize;

   if(style == 1)
      textSize = 18;            // Size of text and instructions
   else if(style == 2)
      textSize = 30;

   const S32 textGap = textSize / 3;   // Spacing between text lines
   const S32 instrGap = 15;            // Gap between last line of text and instruction line

   S32 titleSpace = titleSize + titleGap;
   S32 boxHeight = titleSpace + 2 * vertMargin + (msgLines + 1) * (textSize + textGap) + instrGap;

   if(strcmp(instr, "") == 0)
      boxHeight -= (instrGap + textSize);

   if(strcmp(title, "") == 0)
   {
      boxHeight -= titleSpace;
      titleSpace = 0;
   }

   S32 boxTop = (canvasHeight - boxHeight) / 2 + vertOffset;

   S32 maxLen = 0;
   for(S32 i = 0; i < msgLines; i++)
   {
      S32 len = getStringWidth(textSize, message[i].c_str()) + 20;     // 20 gives a little breathing room on the edges
      if(len > maxLen)
         maxLen = len;
   }

   if(canvasWidth - 2 * inset < maxLen)
      inset = (canvasWidth - maxLen) / 2;


   if(style == 1)       
   {
      // Solid red box with white border
      glDisable(GL_BLEND);
      drawFilledRect(inset, boxTop, canvasWidth - inset, boxTop + boxHeight, Colors::red30, Colors::white);
      glEnable(GL_BLEND);
      glColor(Colors::white);
   }
   else if(style == 2)
   {
      // Hollow blue box
      drawHollowRect(inset, boxTop, canvasWidth - inset, boxTop + boxHeight, Colors::blue);
      glColor(Colors::blue);
   }

   // Draw title, message, and footer
   drawCenteredString(boxTop + vertMargin, titleSize, title);

   for(S32 i = 0; i < msgLines; i++)
      drawCenteredString(boxTop + vertMargin + titleSpace + i * (textSize + textGap), textSize, message[i].c_str());

   drawCenteredString(boxTop + boxHeight - vertMargin - textSize, textSize, instr);
}


// This function could use some further cleaning; currently only used for the delayed spawn notification
void UserInterface::renderUnboxedMessageBox(const char *title, const char *instr, string message[], S32 msgLines, S32 vertOffset)
{
   dimUnderlyingUI();

   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   const S32 titleSize = 30;              // Size of title
   const S32 titleGap = titleSize / 3;    // Spacing between title and first line of text
   const S32 textSize = 36;               // Size of text and instructions
   const S32 textGap = textSize / 3;      // Spacing between text lines
   const S32 instrGap = 15;               // Gap between last line of text and instruction line

   S32 actualLines = 0;
   for(S32 i = 0; i < msgLines; i++)
      if(message[i] != "")
         actualLines = i + 1;

   S32 titleSpace = titleSize + titleGap;
   S32 boxHeight = titleSpace + actualLines * (textSize + textGap) + instrGap;

   if(strcmp(instr, "") == 0)
      boxHeight -= instrGap;

   if(strcmp(title, "") == 0)
   {
      boxHeight -= titleSpace;
      titleSpace = 0;
   }

   S32 boxTop = (canvasHeight - boxHeight) / 2;

   // Draw title, message, and footer
   glColor(Colors::blue);
   drawCenteredString(boxTop + vertMargin, titleSize, title);

   S32 boxWidth = 500;
   drawHollowRect((canvasWidth - boxWidth) / 2, boxTop - vertMargin, canvasWidth - ((canvasWidth - boxWidth) / 2), boxTop + boxHeight + vertMargin);

   for(S32 i = 0; i < msgLines; i++)
      drawCenteredString(boxTop + titleSpace + i * (textSize + textGap), textSize, message[i].c_str());

   drawCenteredString(boxTop + boxHeight / 2 - textSize, textSize, instr);
}


void UserInterface::dimUnderlyingUI()
{
   glColor(Colors::black, 0.75); 

   TNLAssert(glIsEnabled(GL_BLEND), "Blending should be enabled here!");

   drawFilledRect (0, 0, gScreenInfo.getGameCanvasWidth(), gScreenInfo.getGameCanvasHeight());
}


void UserInterface::drawMenuItemHighlight(S32 x1, S32 y1, S32 x2, S32 y2, bool disabled)
{
   if(disabled)
      drawFilledRect(x1, y1, x2, y2, Color(0.4), Color(0.8));
   else
      drawFilledRect(x1, y1, x2, y2, Colors::blue40, Colors::blue);
}


void UserInterface::drawRect(S32 x1, S32 y1, S32 x2, S32 y2, S32 mode)
{
   F32 vertices[] = { x1, y1,   x2, y1,   x2, y2,   x1, y2 };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, mode);
}


// Some functions (renderSpyBugVisibleRange) use this F32 version, this function has better accuracy
void UserInterface::drawRect(F32 x1, F32 y1, F32 x2, F32 y2, S32 mode)
{
   F32 vertices[] = { x1, y1,   x2, y1,   x2, y2,   x1, y2 };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, mode);
}


void UserInterface::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2)
{
   drawRect(x1, y1, x2, y2, GL_TRIANGLE_FAN);
}


void UserInterface::drawFilledRect(F32 x1, F32 y1, F32 x2, F32 y2)
{
   drawRect(x1, y1, x2, y2, GL_TRIANGLE_FAN);
}


void UserInterface::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, const Color &outlineColor)
{
   drawFilledRect(x1, y1, x2, y2, fillColor, 1, outlineColor);
}


void UserInterface::drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha, const Color &outlineColor)
{
   for(S32 i = 1; i >= 0; i--)
   {
      glColor(i ? fillColor : outlineColor, i ? fillAlpha : 1);
      drawRect(x1, y1, x2, y2, i ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
   }
}


void UserInterface::drawHollowRect(const Point &p1, const Point &p2)
{
   drawHollowRect((S32)p1.x, (S32)p1.y, (S32)p2.x, (S32)p2.y);
}


void UserInterface::drawHollowRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &outlineColor)
{
   glColor(outlineColor);
   drawRect(x1, y1, x2, y2, GL_LINE_LOOP);
}


void UserInterface::drawHollowRect(S32 x1, S32 y1, S32 x2, S32 y2)
{
   drawRect(x1, y1, x2, y2, GL_LINE_LOOP);
}


// Given a string, break it up such that no part is wider than width.  Prefix subsequent lines with indentPrefix.
Vector<string> UserInterface::wrapString(const string &str, S32 wrapWidth, S32 fontSize, const string &indentPrefix)
{
   string text = str;               // Make local working copy that we can alter

   U32 lineStartIndex = 0;
   U32 lineEndIndex = 0;
   U32 lineBreakCandidateIndex = 0;
   Vector<U32> separator;           // Collection of character indexes at which to split the message
   Vector<string> wrappedLines;

   while(lineEndIndex < text.length())
   {
      S32 indentWidth = UserInterface::getStringWidth(fontSize, indentPrefix.c_str());

      string substr = text.substr(lineStartIndex, lineEndIndex - lineStartIndex);

      // Does this char put us over the width limit?
      bool overWidthLimit = getStringWidth(fontSize, substr.c_str()) > (wrapWidth - indentWidth);

      // If this character is a space, keep track in case we need to split here
      if(text[lineEndIndex] == ' ')
         lineBreakCandidateIndex = lineEndIndex;

      if(overWidthLimit)
      {
         // If no spaces were found, we need to force a line break at this character.  We'll do this by inserting a space.
         if(lineBreakCandidateIndex == lineStartIndex)
            lineBreakCandidateIndex = lineEndIndex;

         separator.push_back(lineBreakCandidateIndex);  // Add this index to line split list
         if(text[lineBreakCandidateIndex] != ' ')
          text.insert(lineBreakCandidateIndex, 1, ' '); // Add a space if there's not already one there
         lineStartIndex = lineBreakCandidateIndex + 1;  // Skip a char which is a space
         lineBreakCandidateIndex = lineStartIndex;      // Reset line break index to start of list
      }

      lineEndIndex++;
   }

   // Handle lines that need to wrap
   lineStartIndex = 0;

   for(S32 i = 0; i < separator.size(); i++)
   {
      lineEndIndex = separator[i];

      if(i == 0)           
         wrappedLines.push_back(               text.substr(lineStartIndex, lineEndIndex - lineStartIndex));
      else
         wrappedLines.push_back(indentPrefix + text.substr(lineStartIndex, lineEndIndex - lineStartIndex));

      lineStartIndex = lineEndIndex + 1;  // Skip a char which is a space
   }

   // Handle any remaining characters
   if(lineStartIndex != lineEndIndex)
   {
      if(separator.size() == 0)           // Don't draw the extra margin if it is the first and only line
         wrappedLines.push_back(               text.substr(lineStartIndex));
      else
         wrappedLines.push_back(indentPrefix + text.substr(lineStartIndex));
   }

   return wrappedLines;
}


// Returns the number of lines our msg consumed during rendering
U32 UserInterface::drawWrapText(const string &msg, S32 xpos, S32 ypos, S32 width, S32 ypos_end,
                                S32 lineHeight, S32 fontSize, S32 multiLineIndentation, bool alignBottom, bool draw)
{
   string text = msg;               // Make local working copy that we can alter

   U32 lines = 0;
   U32 lineStartIndex = 0;
   U32 lineEndIndex = 0;
   U32 lineBreakCandidateIndex = 0;
   Vector<U32> separator;           // Collection of character indexes at which to split the message


   while(lineEndIndex < text.length())
   {
      bool overWidthLimit = UserInterface::getStringWidth(fontSize, text.substr(lineStartIndex, lineEndIndex - lineStartIndex).c_str()) > (width - multiLineIndentation);

      // If this character is a space, keep track in case we need to split here
      if(text[lineEndIndex] == ' ')
         lineBreakCandidateIndex = lineEndIndex;

      if(overWidthLimit)
      {
         // If no spaces were found, we need to force a line break at this character; game will freeze otherwise
         if(lineBreakCandidateIndex == lineStartIndex)
            lineBreakCandidateIndex = lineEndIndex;

         separator.push_back(lineBreakCandidateIndex);    // Add this index to line split list
         if(text[lineBreakCandidateIndex] != ' ')
            text.insert(lineBreakCandidateIndex, 1, ' '); // Add a space if there's not already one there
         lineStartIndex = lineBreakCandidateIndex + 1;    // Skip a char which is a space
         lineBreakCandidateIndex = lineStartIndex;        // Reset line break index to start of list
      }

      lineEndIndex++;
   }

   // Align the y position, if alignBottom is enabled
   if(alignBottom)
   {
      ypos -= separator.size() * lineHeight;  // Align according to number of wrapped lines
      if(lineStartIndex != lineEndIndex)      // Align the remaining line
         ypos -= lineHeight;
   }

   // Draw lines that need to wrap
   lineStartIndex = 0;
   for(S32 i = 0; i < separator.size(); i++)
   {
      lineEndIndex = separator[i];
      if(ypos >= ypos_end || !alignBottom)      // If there is room to draw some lines at top when aligned bottom
      {
         if(draw)
         {
            if(i == 0)                          // Don't draw the extra margin if it is the first line
               UserInterface::drawString(xpos, ypos, fontSize, text.substr(lineStartIndex, lineEndIndex - lineStartIndex).c_str());
            else
               UserInterface::drawString(xpos + multiLineIndentation, ypos, fontSize, text.substr(lineStartIndex, lineEndIndex - lineStartIndex).c_str());
         }
         lines++;
         if(ypos < ypos_end && !alignBottom)    // If drawing from align top, and ran out of room, then stop and return
         {
            return lines;
         }
      }
      ypos += lineHeight;

      lineStartIndex = lineEndIndex + 1;        // Skip a char which is a space
   }

   // Draw any remaining characters
   if(lineStartIndex != lineEndIndex)
   {
      if(ypos >= ypos_end || !alignBottom)
      {
         if(draw)
         {
            if (separator.size() == 0)          // Don't draw the extra margin if it is the only line
               UserInterface::drawString(xpos, ypos, fontSize, text.substr(lineStartIndex).c_str());
            else
               UserInterface::drawString(xpos + multiLineIndentation, ypos, fontSize, text.substr(lineStartIndex).c_str());
         }

         lines++;
      }
   }

   return lines;
}


// These will be overridden in child classes if needed
void UserInterface::render() 
{ 
   // Do nothing -- probably never even gets called
}


void UserInterface::idle(U32 timeDelta)
{ 
   mTimeSinceLastInput += timeDelta;
}


void UserInterface::onMouseMoved()                         
{ 
   mTimeSinceLastInput = 0;
}


void UserInterface::onMouseDragged()  { /* Do nothing */ }


InputCode UserInterface::getInputCode(GameSettings *settings, InputCodeManager::BindingName binding)
{
   return settings->getInputCodeManager()->getBinding(binding);
}


void UserInterface::setInputCode(GameSettings *settings, InputCodeManager::BindingName binding, InputCode inputCode)
{
   settings->getInputCodeManager()->setBinding(binding, inputCode);
}


bool UserInterface::checkInputCode(GameSettings *settings, InputCodeManager::BindingName binding, InputCode inputCode)
{
   return getInputCode(settings, binding) == settings->getInputCodeManager()->filterInputCode(inputCode);
}


const char *UserInterface::getInputCodeString(GameSettings *settings, InputCodeManager::BindingName binding)
{
   return InputCodeManager::inputCodeToString(getInputCode(settings, binding));
}

 
bool UserInterface::onKeyDown(InputCode inputCode)
{ 
   mTimeSinceLastInput = 0;

   bool handled = false;

   GameSettings *settings = getGame()->getSettings();
   UIManager *uiManager = getGame()->getUIManager();

   if(checkInputCode(settings, InputCodeManager::BINDING_DIAG, inputCode))              // Turn on diagnostic overlay
   { 
      if(uiManager->isCurrentUI(DiagnosticsScreenUI))
         return false;

      uiManager->activate(DiagnosticsScreenUI);

      playBoop();
      
      handled = true;
   }
   else if(checkInputCode(settings, InputCodeManager::BINDING_OUTGAMECHAT, inputCode))  // Turn on Global Chat overlay
   {
      // Don't activate if we're already in chat or if we're on the Name Entry
      // screen (since we don't have a nick yet)
      if(uiManager->isCurrentUI(GlobalChatUI) || uiManager->isCurrentUI(NameEntryUI))
         return false;

      getGame()->getUIManager()->activate(GlobalChatUI);
      playBoop();

      handled = true;
   }

   return handled;
}


void UserInterface::onKeyUp(InputCode inputCode) { /* Do nothing */ }
void UserInterface::onTextInput(char ascii)      { /* Do nothing */ }



// Dumps any keys and raw stick button inputs depressed to the screen when in diagnostic mode.
// This should make it easier to see what happens when users press joystick buttons.
void UserInterface::renderDiagnosticKeysOverlay()
{
   if(gClientGames[0]->getSettings()->getIniSettings()->diagnosticKeyDumpMode)
   {
     S32 vpos = gScreenInfo.getGameCanvasHeight() / 2;
     S32 hpos = horizMargin;

     glColor(Colors::white);

     // Key states
     for (U32 i = 0; i < MAX_INPUT_CODES; i++)
        if(InputCodeManager::getState((InputCode) i))
           hpos += drawStringAndGetWidth( hpos, vpos, 18, InputCodeManager::inputCodeToString((InputCode) i) );

      vpos += 23;
      hpos = horizMargin;
      glColor(Colors::magenta);

      for(U32 i = 0; i < Joystick::MaxSdlButtons; i++)
         if(Joystick::ButtonMask & (1 << i))
         {
            drawStringf( hpos, vpos, 18, "RawBut [%d]", i );
            hpos += getStringWidthf(18, "RawBut [%d]", i ) + 5;
         }
   }
}   


////////////////////////////////////////
////////////////////////////////////////


void UserInterfaceData::get()
{
   vertMargin = UserInterface::vertMargin;
   horizMargin = UserInterface::horizMargin;
   chatMargin = UserInterface::messageMargin;
}


void UserInterfaceData::set()
{
   UserInterface::vertMargin = vertMargin;
   UserInterface::horizMargin = horizMargin;
   UserInterface::messageMargin = chatMargin;
}


};

