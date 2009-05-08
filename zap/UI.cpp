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

#include "../tnl/tnl.h"
using namespace TNL;

#include "UI.h"
#include "sfx.h"
#include "move.h"
#include "keyCode.h"
#include "UIMenus.h"
#include "UIEditor.h"
#include "input.h"      // For MaxJoystickButtons const
#include "config.h"
#include "game.h"

#include "../tnl/tnlLog.h"

#include "../glut/glutInclude.h"

#include <string>
#include <stdarg.h>


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

S32 UserInterface::chatMargin = UserInterface::vertMargin + gLoadoutIndicatorHeight + 3;

extern const S32 gScreenHeight;
extern const S32 gScreenWidth;

S32 UserInterface::canvasWidth = gScreenWidth;
S32 UserInterface::canvasHeight = gScreenHeight;
S32 UserInterface::windowWidth = gScreenWidth;
S32 UserInterface::windowHeight = gScreenHeight;

UserInterface *UserInterface::current = NULL;
Vector<UserInterface *> UserInterface::prevUIs;    // List of peviously displayed UIs


void UserInterface::activate(bool save)
{
   mBlinkTimer.reset(gCursorBlinkTime);

   if (current && save)       // Current is not really current any more... it's actually the previously active UI
      prevUIs.push_back(current);

   current = this;            // Now it is current
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
   TNL::logprintf("Previous UI/menu tree (%d elements):", prevUIs.size());
   for(S32 i = 0; i < prevUIs.size(); i++)
   {
      UserInterface *prev = prevUIs[i];
      TNL::logprintf("    ->%d", prev->getMenuID());
   }
}

// Did we arrive at our current interface via the Editor?
// If the editor is in prevUIs, then we came from there
bool UserInterface::cameFromEditor()
{
   for(S32 i = 0; i < prevUIs.size(); i++)
      if (prevUIs[i] == &gEditorUserInterface)
         return true;

   return false;
}

// Update our global cursor blinkenlight
// To implement a flashing cursor, call this routine from a UI's local idle routine,
// then draw the cursor when cursorBlink is true
void UserInterface::updateCursorBlink(U32 timeDelta)
{
   if(mBlinkTimer.update(timeDelta))
   {
      mBlinkTimer.reset(gCursorBlinkTime);
      cursorBlink = !cursorBlink;
   }
}


// Set interface's name.  This name is used internally only for debugging
// and to identify interfaces when searching for matches.  Each interface
// should have a unique name.

void UserInterface::setMenuID(UIID menuID)
{
   mInternalMenuID = menuID;
}

// Retrieve interface's name
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
   if(prevUIs.size())
   {
      UserInterface *prev = prevUIs.last();
      prevUIs.pop_back();
      prev->reactivate();
   }
   else
      throw("No previous menu to go to!!");
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

extern U32 gRawJoystickButtonInputs;
extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;
extern ServerGame *gServerGame;


// Clean up and get ready to render 
void UserInterface::renderCurrent()    // static
{
   glViewport(0, 0, windowWidth, windowHeight);

   glClearColor(0, 0, 0, 1.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
     S32 vpos = gScreenHeight / 2;
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

#define makeBuffer    va_list args; va_start(args, format); char buffer[2048]; dVsprintf(buffer, sizeof(buffer), format, args);


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

#undef makeBuffer

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
   va_list args;
   va_start(args, format);
   char buffer[2048];

   dVsprintf(buffer, sizeof(buffer), format, args);
   drawString(x, y, size, buffer);
}


void UserInterface::drawStringf(F32 x, F32 y, U32 size, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   dVsprintf(buffer, sizeof(buffer), format, args);
   drawString((S32) x, (S32) y, size, buffer);
}


void UserInterface::drawCenteredString(S32 y, U32 size, const char *string)
{
   S32 x = (S32)((S32) canvasWidth - getStringWidth(size, string)) / 2;
   drawString(x, y, size, string);
}


void UserInterface::drawCenteredStringf(S32 y, U32 size, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   dVsprintf(buffer, sizeof(buffer), format, args);
   drawCenteredString(y, size, buffer);
}


// Figure out the first position of our CenteredString
S32 UserInterface::getCenteredStringStartingPos(U32 size, const char *string)
{
   S32 x = canvasWidth / 2;                  // x must be S32 in case it leaks off left side of screen
   x -= getStringWidth(size, string) / 2;

   return x;
}

S32 UserInterface::getCenteredStringStartingPosf(U32 size, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   dVsprintf(buffer, sizeof(buffer), format, args);
   return getCenteredStringStartingPos(size, buffer);
}


// Figure out the first position of our 2ColCenteredString
S32 UserInterface::getCenteredString2ColStartingPos(U32 size, bool leftCol, const char *string)
{
   return get2ColStartingPos(leftCol) - getStringWidth(size, string) / 2;
}


S32 UserInterface::getCenteredString2ColStartingPosf(U32 size, bool leftCol, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   dVsprintf(buffer, sizeof(buffer), format, args);
   return getCenteredString2ColStartingPos(size, leftCol, buffer);
}


void UserInterface::drawCenteredString2Col(S32 y, U32 size, bool leftCol, const char *string)
{
   S32 x = getCenteredString2ColStartingPos(size, leftCol, string);
   drawString(x, y, size, string);
}


void UserInterface::drawCenteredString2Colf(S32 y, U32 size, bool leftCol, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   dVsprintf(buffer, sizeof(buffer), format, args);
   drawCenteredString2Col(y, size, leftCol, buffer);
}

S32 UserInterface::get2ColStartingPos(bool leftCol)      // Must be S32 to avoid problems downstream
{
   return leftCol ? (canvasWidth / 4) : (canvasWidth - (canvasWidth / 4));
}

// Draws a string centered in the left or right half of the screen, with different parts colored differently
void UserInterface::drawCenteredStringPair2Colf(S32 y, U32 size, bool leftCol, const char *left, const char *right, ...)
{
   va_list args;
   va_start(args, right);
   char buffer[2048];
   dVsprintf(buffer, sizeof(buffer), right, args);

   S32 offset = getStringWidth(size, left) + getStringWidth(size, " ");
   S32 width = offset + getStringWidth(size, buffer);
   S32 x = get2ColStartingPos(leftCol) - width / 2;         // x must be S32 in case it leaks off left side of screen

   glColor3f(1,1,1);
   drawString(x, y, size, left);
   glColor3f(0,1,1);
   drawString(x + offset, y, size, buffer);
}


// Draw a left-justified string at column # (1-4)
void UserInterface::drawString4Col(S32 y, U32 size, U32 col, const char *string)
{
   drawString(horizMargin + ((canvasWidth - 2 * horizMargin) / 4 * (col - 1)), y, size, string);
}

void UserInterface::drawString4Colf(S32 y, U32 size, U32 col, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   dVsprintf(buffer, sizeof(buffer), format, args);
   drawString4Col(y, size, col, buffer);
}


S32 UserInterface::getStringWidth(U32 size, const char *string, U32 len)
{
   U32 width = 0;
   if(!len)
      len = (U32) strlen(string);
   while(len--)
   {
      width += glutStrokeWidth(GLUT_STROKE_ROMAN, *string);
      string++;
   }
   return U32((width * size) / 120.0f);
}

S32 UserInterface::getStringWidthf(U32 size, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   dVsprintf(buffer, sizeof(buffer), format, args);
   return getStringWidth(size, buffer);
}

void UserInterface::playBoop()
{
   SFXObject::play(SFXUIBoop, 1);
}

void UserInterface::render()
{
   // Do nothing
}

void UserInterface::idle(U32 timeDelta)
{
   // Do nothing
}

void UserInterface::onMouseMoved(S32 x, S32 y)
{
   // Do nothing
}

void UserInterface::onMouseDragged(S32 x, S32 y)
{
   // Do nothing
}

void UserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   // Do nothing
}

void UserInterface::onKeyUp(KeyCode keyCode)
{
   // Do nothing
}

};
