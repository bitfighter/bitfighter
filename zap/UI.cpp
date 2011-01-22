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
#include "sfx.h"
#include "move.h"
#include "keyCode.h"
#include "UIMenus.h"
#include "input.h"      // For MaxJoystickButtons const
#include "config.h"
#include "game.h"
#include "oglconsole.h"    // For console rendering
//#include "UIEditor.h"      // <--- we need to get rid of this one!
//#include "UICredits.h"     // <--- don't want this one either

#include "../glut/glutInclude.h"

#include <string>
#include <stdarg.h>     // For va_args


#ifdef ZAP_DEDICATED
void *glutStrokeRoman;
#endif

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

S32 UserInterface::chatMargin = UserInterface::vertMargin + gLoadoutIndicatorHeight + 5;

UserInterface *UserInterface::current = NULL;
Vector<UserInterface *> UserInterface::prevUIs;    // List of peviously displayed UIs


float gLineWidth1 = 1.0f;
float gDefaultLineWidth = 2.0f;
float gLineWidth3 = 3.0f;
float gLineWidth4 = 4.0f;


////////////////////////////////////////
////////////////////////////////////////


void UserInterface::activate(bool save)
{
   UserInterface *prev = current;

   if(current && save)        // Current is not really current any more... it's actually the previously active UI
       prevUIs.push_back(current);

   current = this;            // Now it is current

   if(prev)
      prev->onDeactivate();

   onActivate();              // Activate the now current current UI
}


void UserInterface::reactivate()
{
   current = this;
   onReactivate();
}

// Dump all items in the prevUIs vector for debugging purposes
void UserInterface::dumpPreviousQueue()
{
   logprintf("Previous UI/menu tree (%d elements):", prevUIs.size());
   for(S32 i = 0; i < prevUIs.size(); i++)
   {
      UserInterface *prev = prevUIs[i];
      logprintf("    ->%d", prev->getMenuID());
   }
}


// Did we arrive at our current interface via the Editor?
// If the editor is in prevUIs, then we came from there
bool UserInterface::cameFrom(UIID menuID)
{
   for(S32 i = 0; i < prevUIs.size(); i++)
      if(prevUIs[i]->getMenuID() == menuID)
         return true;

   return false;
}


// Set interface's name.  This name is used internally only for debugging
// and to identify interfaces when searching for matches.  Each interface
// should have a unique id.

void UserInterface::setMenuID(UIID menuID)
{
   mInternalMenuID = menuID;
}


// Retrieve interface's id
UIID UserInterface::getMenuID()
{
   return mInternalMenuID;
}


// Retrieve previous interface's name
UIID UserInterface::getPrevMenuID()
{
   if(prevUIs.size())
      return prevUIs.last()->mInternalMenuID;
   else
      return InvalidUI;
}


// Reactivate previous interface, going to fallback if there is none
void UserInterface::reactivatePrevUI()
{
   TNLAssert(prevUIs.size(), "Trying to reactivate a non-existant UI!");

   UserInterface *prev = prevUIs.last();
   prevUIs.pop_back();
   prev->reactivate();
}


// Like above, except we specify a target menu to go to.
void UserInterface::reactivateMenu(UserInterface target)
{
   // Keep discarding menus until we find the one we want
   while( prevUIs.size() && (prevUIs.last()->getMenuID() != target.getMenuID()) )
      prevUIs.pop_back();

   if(!prevUIs.size())
      gMainMenuUserInterface.reactivate();      // Fallback if everything else has failed
   else
      // Now that the next one is our target, when we reactivate, we'll be where we want to be
      reactivatePrevUI();
}


void UserInterface::onActivate()   { /* Do nothing */ }
void UserInterface::onReactivate() { /* Do nothing */ }


// It will be simpler if we translate joystick controls into keyboard actions here rather than check for them elsewhere.  
// This is possibly marginally less efficient, but will reduce maintenance burdens over time.
KeyCode UserInterface::convertJoystickToKeyboard(KeyCode keyCode)
{
   //if(menuSitck == JOYSTICK_DPAD)
   //{
      if(keyCode == BUTTON_DPAD_LEFT) 
         return KEY_LEFT;
      if(keyCode == BUTTON_DPAD_RIGHT) 
         return KEY_RIGHT;
      if(keyCode == BUTTON_DPAD_UP) 
         return KEY_UP;
      if(keyCode == BUTTON_DPAD_DOWN) 
         return KEY_DOWN;
   //}

   //if(menuStick == JOYSTICK_STICK1)
   //{
      if(keyCode == STICK_1_LEFT) 
         return KEY_LEFT;
      if(keyCode == STICK_1_RIGHT) 
         return KEY_RIGHT;
      if(keyCode == STICK_1_UP) 
         return KEY_UP;
      if(keyCode == STICK_1_DOWN) 
         return KEY_DOWN;
   //}

   //if(menuStick == JOYSTICK_STICK2)
   //{
      if(keyCode == STICK_2_LEFT) 
         return KEY_LEFT;
      if(keyCode == STICK_2_RIGHT) 
         return KEY_RIGHT;
      if(keyCode == STICK_2_UP) 
         return KEY_UP;
      if(keyCode == STICK_2_DOWN) 
         return KEY_DOWN;
   //}


   if(keyCode == BUTTON_START) 
      return KEY_ENTER;
   if(keyCode == BUTTON_BACK) 
      return KEY_ESCAPE;
   if(keyCode == BUTTON_1)    // Some game pads might not have a START button
         return KEY_ENTER;

   return keyCode;
}


extern U32 gRawJoystickButtonInputs;
extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;
extern ServerGame *gServerGame;

// Clean up and get ready to render
void UserInterface::renderCurrent()    // static
{
   // Clear screen -- force clear of "black bars" area to avoid flickering on some video cards
   bool scissorMode = glIsEnabled(GL_SCISSOR_TEST);

   if(scissorMode) 
      glDisable(GL_SCISSOR_TEST);

   glClearColor(0, 0, 0, 1.0);
   glClear(GL_COLOR_BUFFER_BIT);

   if(scissorMode) 
      glEnable(GL_SCISSOR_TEST);
   
   glViewport(0, 0, gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight());

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // Run the active UI renderer
   if(current)
      current->render();

   // By putting this here, it will always get rendered, regardless of which UI (if any) is active (kind of ugly)
   // This block will dump any keys and raw stick button inputs depressed to the screen when in diagnostic mode
   // This should make it easier to see what happens when users press joystick buttons
   if(gIniSettings.diagnosticKeyDumpMode)
   {
     glColor3f(1, 1, 1);
     S32 vpos = gScreenInfo.getGameCanvasHeight() / 2;
     S32 hpos = horizMargin;

      glColor3f(1, 1, 1);
      // Key states
     for (U32 i = 0; i < MAX_KEYS; i++)
        if(getKeyState((KeyCode) i))
        {
        drawString( hpos, vpos, 18, keyCodeToString((KeyCode) i) );
        hpos += getStringWidth(18, keyCodeToString( (KeyCode) i) ) + 5;
        }

      glColor3f(1, 0, 1);
      vpos += 23;
      hpos = horizMargin;
      for(S32 i = 0; i < MaxJoystickButtons; i++)
         if(gRawJoystickButtonInputs & (1 << i))
         {
            drawStringf( hpos, vpos, 18, "RawBut [%d]", i );
            hpos += getStringWidthf(18, "RawBut [%d]", i ) + 5;
         }
   }
   // End diagnostic key dump mode
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
   S32 len = getStringWidthf((U32)size, buffer);
   F32 offset = (p1.distanceTo(p2) - len) / 2;

   drawAngleString_fixed(p1.x + cosang * offset + sinang * (size + vert_offset), p1.y + sinang * offset - cosang * (size + vert_offset), size, ang, buffer);
}


// New, fixed version
void UserInterface::drawAngleStringf_fixed(F32 x, F32 y, F32 size, F32 angle, const char *format, ...)
{
   makeBuffer;
   doDrawAngleString((S32) x, (S32) y, size, angle, buffer, true);
}


// New, fixed version
void UserInterface::drawAngleString_fixed(S32 x, S32 y, F32 size, F32 angle, const char *string)
{
   doDrawAngleString(x, y, size, angle, string, true);
}


// New, fixed version
void UserInterface::drawAngleString_fixed(F32 x, F32 y, F32 size, F32 angle, const char *string)
{
   doDrawAngleString(x, y, size, angle, string, true);
}


// Old, broken version
void UserInterface::drawAngleStringf(F32 x, F32 y, F32 size, F32 angle, const char *format, ...)
{
   makeBuffer;
   doDrawAngleString((S32) x, (S32) y, size, angle, buffer, false);
}


void UserInterface::drawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string)
{
   doDrawAngleString(x, y, size, angle, string, false);
}


void UserInterface::drawAngleString(S32 x, S32 y, U32 size, F32 angle, const char *string)
{
   doDrawAngleString(x, y, (F32) size, angle, string, false);
}


void UserInterface::doDrawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string, bool fix)
{
   F32 scaleFactor = size / 120.0f;
   glPushMatrix();
   glTranslatef(x, y + (fix ? 0 : size), 0);
   glRotatef(angle * radiansToDegreesConversion, 0, 0, 1);
   glScalef(scaleFactor, -scaleFactor, 1);
   for(S32 i = 0; string[i]; i++)
      glutStrokeCharacter(GLUT_STROKE_ROMAN, string[i]);
   glPopMatrix();
}

// Same but accepts F32 args
void UserInterface::doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string, bool fix)
{
   doDrawAngleString((S32) x, (S32) y, size, angle, string, fix);
}


//void UserInterface::drawAngleString(S32 x, S32 y, U32 size, F32 angle, const char *string)
//{
//   drawAngleString(x, y, (F32) size, angle, string);
//}
//
//void UserInterface::drawAngleStringf(S32 x, S32 y, U32 size, F32 angle, const char *format, ...)
//{
//   va_list args;
//   va_start(args, format);
//   char buffer[2048];
//
//   dVsprintf(buffer, sizeof(buffer), format, args);
//   drawAngleString(x, y, (F32) size, angle, buffer);
//}


void UserInterface::drawString(S32 x, S32 y, U32 size, const char *string)
{
   drawAngleString(x, y, size, 0, string);
}


void UserInterface::drawString(F32 x, F32 y, U32 size, const char *string)
{
   drawAngleString((S32) x, (S32) y, size, 0, string);
}


void UserInterface::drawStringf(S32 x, S32 y, U32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
}


void UserInterface::drawStringf(F32 x, F32 y, U32 size, const char *format, ...)
{
   makeBuffer;
   drawString((S32) x, (S32) y, size, buffer);
}


void UserInterface::drawStringfc(F32 x, F32 y, U32 size, const char *format, ...)
{
   makeBuffer;
   drawStringc(x, y, size, buffer);
}


void UserInterface::drawStringfr(F32 x, F32 y, U32 size, const char *format, ...)
{
   makeBuffer;
   S32 pos = getStringWidth(size, buffer);
   drawStringc(x - pos, y, size, buffer);
}

   
S32 UserInterface::drawStringAndGetWidth(F32 x, F32 y, U32 size, const char *string)
{
   drawString((S32) x, (S32) y, size, string);
   return getStringWidth(size, string);
}


S32 UserInterface::drawStringAndGetWidthf(F32 x, F32 y, U32 size, const char *format, ...)
{
   makeBuffer;
   drawString(x, y, size, buffer);
   return getStringWidth(size, buffer);
}


void UserInterface::drawStringc(F32 x, F32 y, U32 size, const char *string)
{
   S32 len = getStringWidth(size, string);
   drawAngleString((S32) x - len / 2, (S32) y, size, 0, string);
}


S32 UserInterface::drawCenteredString(S32 y, U32 size, const char *string)
{
   S32 x = (gScreenInfo.getGameCanvasWidth() - getStringWidth(size, string)) / 2;
   drawString(x, y, size, string);
   return x;
}


S32 UserInterface::drawCenteredStringf(S32 y, U32 size, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString(y, size, buffer);
}


// Figure out the first position of our CenteredString
S32 UserInterface::getCenteredStringStartingPos(U32 size, const char *string)
{
   S32 x = gScreenInfo.getGameCanvasWidth() / 2;      // x must be S32 in case it leaks off left side of screen
   x -= getStringWidth(size, string) / 2;

   return x;
}


S32 UserInterface::getCenteredStringStartingPosf(U32 size, const char *format, ...)
{
   makeBuffer;
   return getCenteredStringStartingPos(size, buffer);
}


// Figure out the first position of our 2ColCenteredString
S32 UserInterface::getCenteredString2ColStartingPos(U32 size, bool leftCol, const char *string)
{
   return get2ColStartingPos(leftCol) - getStringWidth(size, string) / 2;
}


S32 UserInterface::getCenteredString2ColStartingPosf(U32 size, bool leftCol, const char *format, ...)
{
   makeBuffer;
   return getCenteredString2ColStartingPos(size, leftCol, buffer);
}


S32 UserInterface::drawCenteredString2Col(S32 y, U32 size, bool leftCol, const char *string)
{
   S32 x = getCenteredString2ColStartingPos(size, leftCol, string);
   drawString(x, y, size, string);
   return x;
}


S32 UserInterface::drawCenteredString2Colf(S32 y, U32 size, bool leftCol, const char *format, ...)
{
   makeBuffer;
   return drawCenteredString2Col(y, size, leftCol, buffer);
}

   
S32 UserInterface::get2ColStartingPos(bool leftCol)      // Must be S32 to avoid problems downstream
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   return leftCol ? (canvasWidth / 4) : (canvasWidth - (canvasWidth / 4));
}


extern void glColor(const Color &c, float alpha = 1.0);

// Returns starting position of value, which is useful for positioning the cursor in an editable menu entry
S32 UserInterface::drawCenteredStringPair(S32 ypos, U32 size, const Color &leftColor, const Color &rightColor, 
                                          const char *leftStr, const char *rightStr)
{
   S32 xpos = getCenteredStringStartingPosf(size, "%s %s", leftStr, rightStr);

   glColor(leftColor);
   xpos += UserInterface::drawStringAndGetWidthf(xpos, ypos, size, "%s ", leftStr);

   glColor(rightColor);
   UserInterface::drawString(xpos, ypos, size, rightStr);

   return xpos;
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


// Draws a string centered in the left or right half of the screen, with different parts colored differently
S32 UserInterface::drawCenteredStringPair2Colf(S32 y, U32 size, bool leftCol, const char *left, const char *right, ...)
{
   va_list args;
   va_start(args, right);
   char buffer[2048];
   vsnprintf(buffer, sizeof(buffer), right, args);
   va_end(args);

   S32 offset = getStringWidth(size, left) + getStringWidth(size, " ");
   S32 width = offset + getStringWidth(size, buffer);
   S32 x = get2ColStartingPos(leftCol) - width / 2;         // x must be S32 in case it leaks off left side of screen

   glColor3f(1,1,1);
   drawString(x, y, size, left);
   glColor3f(0,1,1);
   drawString(x + offset, y, size, buffer);

   return x;
}


// Draw a left-justified string at column # (1-4)
void UserInterface::drawString4Col(S32 y, U32 size, U32 col, const char *string)
{
   drawString(horizMargin + ((gScreenInfo.getGameCanvasWidth() - 2 * horizMargin) / 4 * (col - 1)), y, size, string);
}


void UserInterface::drawString4Colf(S32 y, U32 size, U32 col, const char *format, ...)
{
   makeBuffer;
   drawString4Col(y, size, col, buffer);
}

   

S32 UserInterface::getStringWidth(F32 size, const char *string)
{
#ifndef ZAP_DEDICATED
   return (S32) glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char *) string) * size / 120.0;
#else
   return 1;
#endif
}


S32 UserInterface::getStringWidthf(U32 size, const char *format, ...)
{
   makeBuffer;
   return getStringWidth(size, buffer);
}

#undef makeBuffer


void UserInterface::playBoop()
{
   SFXObject::play(SFXUIBoop, 1);
}


void UserInterface::renderConsole()
{
   // Temporarily disable scissors mode so we can use the full width of the screen
   // to show our console text, black bars be damned!
   bool scissorMode = glIsEnabled(GL_SCISSOR_TEST);

   if(scissorMode) 
      glDisable(GL_SCISSOR_TEST);

   OGLCONSOLE_Draw();   

   if(scissorMode) 
      glEnable(GL_SCISSOR_TEST);
}


//extern void glColor(const Color &c, float alpha = 1);

void UserInterface::renderMessageBox(const char *title, const char *instr, const char *message[], S32 msgLines, S32 vertOffset)
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   S32 inset = 100;              // Inset for left and right edges of box
   const S32 titleSize = 30;     // Size of title
   const S32 titleGap = 10;      // Spacing between title and first line of text
   const S32 textSize = 18;      // Size of text and instructions
   const S32 textGap = 6;        // Spacing between text lines
   const S32 instrGap = 15;      // Gap between last line of text and instruction line

   S32 boxHeight = titleSize + titleGap + 2 * vertMargin + (msgLines + 1) * (textSize + textGap) + instrGap;

   if(!strcmp(instr, ""))
      boxHeight -= (instrGap + textSize);

   S32 boxTop = (canvasHeight - boxHeight) / 2 + vertOffset;

   S32 maxLen = 0;
   for(S32 i = 0; i < msgLines; i++)
   {
      S32 len = getStringWidth(textSize, message[i]) + 20;     // 20 gives a little breathing room on the edges
      if(len > maxLen)
         maxLen = len;
   }

   if(canvasWidth - 2 * inset < maxLen)
      inset = (canvasWidth - maxLen) / 2;

   glDisableBlendfromLineSmooth;
   for(S32 i = 1; i >= 0; i--)
   {
      if(i == 0) 
         glEnableBlendfromLineSmooth;

      glColor(i ? Color(.3,0,0) : Color(1,1,1));        // Draw the box
      
      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f(inset, boxTop);
         glVertex2f(canvasWidth - inset, boxTop);
         glVertex2f(canvasWidth - inset, boxTop + boxHeight);
         glVertex2f(inset, boxTop + boxHeight);
      glEnd();
   
   }

   // Draw title, message, and footer
   glColor3f(1,1,1);
   drawCenteredString(boxTop + vertMargin, titleSize, title);

   for(S32 i = 0; i < msgLines; i++)
      drawCenteredString(boxTop + vertMargin + titleSize + titleGap + i * (textSize + textGap), textSize, message[i]);

   drawCenteredString(boxTop + boxHeight - vertMargin - textSize, textSize, instr);
}


// These will be overridden in child classes if needed
void UserInterface::render()                               { /* Do nothing */ }
void UserInterface::idle(U32 timeDelta)                    { /* Do nothing */ }
void UserInterface::onMouseMoved(S32 x, S32 y)             { /* Do nothing */ }
void UserInterface::onMouseDragged(S32 x, S32 y)           { /* Do nothing */ }
void UserInterface::onKeyDown(KeyCode keyCode, char ascii) { /* Do nothing */ }
void UserInterface::onKeyUp(KeyCode keyCode)               { /* Do nothing */ }

};

