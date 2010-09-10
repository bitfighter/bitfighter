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

#ifndef _UI_H_
#define _UI_H_

#include "keyCode.h"
#include "SharedConstants.h"
#include "timer.h"
#include "point.h"
#include "lineEditor.h"

#include "tnl.h"

#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{


#define ENABLE_ENGINEER 0

static const float gDefaultLineWidth = 2.0f;

const S32 gScreenHeight = 600;
const S32 gScreenWidth = 800;

const U32 MAX_GAME_NAME_LEN = 32;     // Any longer, and it won't fit on-screen
const U32 MAX_FILE_NAME_LEN = 32;     // Completely arbitrary
const U32 MAX_GAME_DESCR_LEN = 60;    // Any longer, and it won't fit on-screen


enum UIID {
   AdminPasswordEntryUI,
   ChatUI,
   CreditsUI,
   DiagnosticsScreenUI,
   EditorInstructionsUI,
   EditorUI,
   EditorMenuUI,
   ErrorMessageUI,
   GameMenuUI,
   GameParamsUI,
   GameUI,
   GenericUI,
   GlobalChatUI,
   SuspendedUI,
   HostingUI,
   InstructionsUI,
   KeyDefUI,
   LevelUI,
   LevelNameEntryUI,
   LevelChangePasswordEntryUI,
   LevelTypeUI,
   MainUI,
   NameEntryUI,
   OptionsUI,
   PasswordEntryUI,
   ReservedNamePasswordEntryUI,
   PlayerUI,
   TeamUI,
   QueryServersScreenUI,
   SplashUI,
   TeamDefUI,
   TextEntryUI,
   YesOrNoUI,
   InvalidUI,        // Not a valid UI
};


////////////////////////////////////////
////////////////////////////////////////

class UserInterface
{
private:
   UIID mInternalMenuID;                     // Unique interface ID
   static void doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string, bool fixed);
   static void doDrawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string, bool fixed);

public:
   static const S32 MAX_PASSWORD_LENGTH = 32;      // Arbitrary, doesn't matter, but needs to be _something_

   static UserInterface *current;            // Currently active menu
   static Vector<UserInterface *> prevUIs;   // Previously active menus

   static void dumpPreviousQueue();          // List all items in the previous list
   void setMenuID(UIID menuID);              // Set interface's name
   UIID getMenuID();                         // Retrieve interface's name
   UIID getPrevMenuID();                     // Retrieve previous interface's name

   static S32 windowWidth, windowHeight, canvasWidth, canvasHeight;
   static S32 vertMargin, horizMargin;
   static S32 chatMargin;

   static bool cameFrom(UIID menuID);        // Did we arrive at our current interface via the specified interface?

   static void renderCurrent();

   virtual void render();
   virtual void idle(U32 timeDelta);
   virtual void onActivate();
   virtual void onReactivate();

   void activate(bool save = true);
   virtual void reactivate();

   static void reactivatePrevUI();
   static void reactivateMenu(UserInterface target);

   // Input event handlers
   virtual void onKeyDown(KeyCode keyCode, char ascii);
   virtual void onKeyUp(KeyCode keyCode);
   virtual void onMouseMoved(S32 x, S32 y);
   virtual void onMouseDragged(S32 x, S32 y);

   static string itos(S32 i);     // Convert an integer to a string

   void renderMessageBox(const char *title, const char *instr, const char *message[], S32 msgLines);


   // Draw string at given location (normal and formatted versions)
   // Note it is important that x be S32 because for longer strings, they are occasionally drawn starting off-screen
   // to the left, and better to have them partially appear than not appear at all, which will happen if they are U32
   static void drawString(S32 x, S32 y, U32 size, const char *string);
   static void drawStringf(S32 x, S32 y, U32 size, const char *format, ...);

   static void drawStringf(F32 x, F32 y, U32 size, const char *format, ...);
   static void drawString(F32 x, F32 y, U32 size, const char *string);

   // Draw strings centered at point
   static void drawStringfc(F32 x, F32 y, U32 size, const char *format, ...);
   static void drawStringc(F32 x, F32 y, U32 size, const char *string);

   // Draw string and get it's width
   static S32 drawStringAndGetWidth(F32 x, F32 y, U32 size, const char *string);
   static S32 drawStringAndGetWidthf(F32 x, F32 y, U32 size, const char *format, ...);


   // Draw text at an angle...
   static void drawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string);
   static void drawAngleString(S32 x, S32 y, U32 size, F32 angle, const char *string);
   static void drawAngleStringf(S32 x, S32 y, F32 size, F32 angle, const char *format, ...);
   

   // Original drawAngleString has a bug in positioning, but fixing it everywhere in the app would be a huge pain, so
   // we've created a new drawAngleString function without the bug, called xx_fixed.  Actual work now moved to doDrawAngleString,
   // which is marked private.
   static void drawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string);
   static void drawAngleString_fixed(F32 x, F32 y, F32 size, F32 angle, const char *string);
   static void drawAngleString_fixed(S32 x, S32 y, F32 size, F32 angle, const char *string);
   static void drawAngleStringf(F32 x, F32 y, F32 size, F32 angle, const char *format, ...);
   static void drawAngleStringf_fixed(F32 x, F32 y, F32 size, F32 angle, const char *format, ...);

   // Center text between two points
   void drawStringf_2pt(Point p1, Point p2, F32 size, F32 vert_offset, const char *format, ...);

   // Draw text centered on screen (normal and formatted versions)  --> now return starting location
   static S32 drawCenteredString(S32 y, U32 size, const char *str);
   static S32 drawCenteredStringf(S32 y, U32 size, const char *format, ...);
   static S32 drawCenteredStringPair(S32 ypos, U32 size, const Color &leftColor, const Color &rightColor, 
                                     const char *leftStr, const char *rightStr);

   // Draw text centered in a left or right column (normal and formatted versions)  --> now return starting location
   static S32 drawCenteredString2Col(S32 y, U32 size, bool leftCol, const char *str);
   static S32 drawCenteredString2Colf(S32 y, U32 size, bool leftCol, const char *format, ...);
   static S32 drawCenteredStringPair2Colf(S32 y, U32 size, bool leftCol, const char *left, const char *right, ...);

   // Get info about where text will be draw
   static S32 get2ColStartingPos(bool leftCol);
   static S32 getCenteredStringStartingPos(U32 size, const char *string);
   static S32 getCenteredStringStartingPosf(U32 size, const char *format, ...);
   static S32 getCenteredString2ColStartingPos(U32 size, bool leftCol, const char *string);
   static S32 getCenteredString2ColStartingPosf(U32 size, bool leftCol, const char *format, ...);

   // Draw 4-column left-justified text
   static void drawString4Col(S32 y, U32 size, U32 col, const char *str);
   static void drawString4Colf(S32 y, U32 size, U32 col, const char *format, ...);

   // Return string rendering width (normal and formatted versions)
   static S32 getStringWidth(F32 size, const char *str);
   static S32 getStringWidthf(U32 size, const char *format, ...);

   Point convertWindowToCanvasCoord(Point p) { return Point(p.x * canvasWidth / windowWidth, p.y * canvasHeight / windowHeight); }


   static void playBoop();    // Make some noise!
};

};

#endif


