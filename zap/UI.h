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

#ifdef ZAP_DEDICATED
#error "UI.h shouldn't be included in dedicated build"
#endif

#include "UIManager.h"
#include "InputCode.h"
#include "SharedConstants.h"
#include "Timer.h"
#include "Point.h"
#include "lineEditor.h"
#include "config.h"           // For DisplayMode enum
#include "stringUtils.h"      // For itos

#include "tnl.h"
#include "tnlLog.h"

#ifdef TNL_OS_MOBILE
#include "SDL_opengles.h"
#else
#include "SDL_opengl.h"
#endif

#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{

extern float gLineWidth1;
//float gLineWidth2 = 2.0f;
extern float gDefaultLineWidth;
extern float gLineWidth3;
extern float gLineWidth4;


////////////////////////////////////////
////////////////////////////////////////

class Game;
class ClientGame;
class UIManager;

class UserInterface
{
   friend class UIManager;    // Give UIManager access to private and protected members

private:
   UIID mInternalMenuID;                     // Unique interface ID
   static void doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string, bool autoLineWidth = true);
   static void doDrawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string, bool autoLineWidth = true);

   ClientGame *mClientGame;

   U32 mTimeSinceLastInput;

   // Activate menus via the UIManager, please!
   void activate();
   void reactivate();

protected:
   static bool mDisableShipKeyboardInput;    // Disable ship movement while user is in menus
   void setMenuID(UIID menuID);              // Set interface's name

public:
   UserInterface(ClientGame *game);                // Constructor
   virtual ~UserInterface();                       // Destructor
   static const S32 MAX_PASSWORD_LENGTH = 32;      // Arbitrary, doesn't matter, but needs to be _something_

   UIID getMenuID() const;                   // Retrieve interface's name
   UIID getPrevMenuID() const;               // Retrieve previous interface's name

   ClientGame *getGame();

   UIManager *getUIManager() const;

   static S32 vertMargin, horizMargin;
   static S32 messageMargin;

   U32 getTimeSinceLastInput();

   virtual void render();
   virtual void idle(U32 timeDelta);
   virtual void onActivate();
   virtual void onDeactivate(bool usesEditorScreenMode);
   virtual void onReactivate();
   virtual void onDisplayModeChange();


   virtual bool usesEditorScreenMode();   // Returns true if the UI attempts to use entire screen like editor, false otherwise

   void renderConsole();                  // Render game console
   virtual void renderMasterStatus();     // Render master server connection status

   // Helpers to simplify dealing with key bindings
   InputCode getInputCode(GameSettings *settings, InputCodeManager::BindingName binding);
   void setInputCode(GameSettings *settings, InputCodeManager::BindingName binding, InputCode inputCode);
   bool checkInputCode(GameSettings *settings, InputCodeManager::BindingName, InputCode inputCode);
   const char *getInputCodeString(GameSettings *settings, InputCodeManager::BindingName binding);


   // Input event handlers
   virtual bool onKeyDown(InputCode inputCode);
   virtual void onKeyUp(InputCode inputCode);
   virtual void onTextInput(char ascii);
   virtual void onMouseMoved();
   virtual void onMouseDragged();

   void renderMessageBox(const char *title, const char *instr, string message[], S32 msgLines, S32 vertOffset = 0, S32 style = 1);
   void renderUnboxedMessageBox(const char *title, const char *instr, string message[], S32 msgLines, S32 vertOffset = 0);

   void dimUnderlyingUI();

   static void renderDiagnosticKeysOverlay();

   static void drawMenuItemHighlight(S32 x1, S32 y1, S32 x2, S32 y2, bool disabled = false);
   static void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2);
   static void drawFilledRect(F32 x1, F32 y1, F32 x2, F32 y2);

   static void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, const Color &outlineColor);
   static void drawFilledRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &fillColor, F32 fillAlpha, const Color &outlineColor);

   static void drawHollowRect(const Point &p1, const Point &p2);
   static void drawHollowRect(S32 x1, S32 y1, S32 x2, S32 y2);
   static void drawHollowRect(S32 x1, S32 y1, S32 x2, S32 y2, const Color &outlineColor);

   static void drawRect(S32 x1, S32 y1, S32 x2, S32 y2, S32 mode);
   static void drawRect(F32 x1, F32 y1, F32 x2, F32 y2, S32 mode);


   // Draw string at given location (normal and formatted versions)
   // Note it is important that x be S32 because for longer strings, they are occasionally drawn starting off-screen
   // to the left, and better to have them partially appear than not appear at all, which will happen if they are U32
   static void drawString(S32 x, S32 y, F32 size, const char *string);
   static void drawString(F32 x, F32 y, F32 size, const char *string);
   static void drawString(F32 x, F32 y, S32 size, const char *string);
   static void drawString(S32 x, S32 y, S32 size, const char *string);

   static void drawStringf(S32 x, S32 y, S32 size, const char *format, ...);
   static void drawStringf(F32 x, F32 y, F32 size, const char *format, ...);
   static void drawStringf(F32 x, F32 y, S32 size, const char *format, ...);


   // Draw strings centered at point
   static void drawStringfc(F32 x, F32 y, F32 size, const char *format, ...);
   static void drawStringc(F32 x, F32 y, F32 size, const char *string, bool autoSize = true);
   static void drawStringc(S32 x, S32 y, S32 size, const char *string);


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
   static void drawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string, bool autoLineWidth = true);
   static void drawAngleStringf(F32 x, F32 y, F32 size, F32 angle, const char *format, ...);

   // Center text between two points
   static void drawStringf_2pt(Point p1, Point p2, F32 size, F32 vert_offset, const char *format, ...);

   // Draw text centered on screen (normal and formatted versions)  --> now return starting location
   static S32 drawCenteredString(S32 y, S32 size, const char *str);
   static S32 drawCenteredString(S32 x, S32 y, S32 size, const char *str);
   static F32 drawCenteredString(F32 x, F32 y, S32 size, const char *str);
   static F32 drawCenteredString(F32 x, F32 y, F32 size, const char *str);
   static S32 drawCenteredStringf(S32 y, S32 size, const char *format, ...);
   static S32 drawCenteredStringf(S32 x, S32 y, S32 size, const char *format, ...);

   static S32 drawCenteredUnderlinedString(S32 y, S32 size, const char *string);


   static S32 drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor, 
                                     const char *leftStr, const char *rightStr);

   static S32 drawCenteredStringPair(S32 ypos, S32 size, const Color &leftColor, const Color &rightColor, 
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
   static F32 getStringWidth(F32 size, const char *str);
   static S32 getStringWidth(S32 size, const char *str);

   static F32 getStringWidthf(F32 size, const char *format, ...);
   static S32 getStringWidthf(S32 size, const char *format, ...);

   static Vector<string> wrapString(const string &str, S32 width, S32 fontSize, const string &indentPrefix);

   static U32 drawWrapText(const string &msg, S32 xpos, S32 ypos, S32 width, S32 ypos_end,
         S32 lineHeight, S32 fontSize, S32 multiLineIndentation = 0, bool alignBottom = false, bool draw = true);

   static void playBoop();    // Make some noise!
};



// Used only for multiple mClientGame in one instance
struct UserInterfaceData
{
   S32 vertMargin, horizMargin;
   S32 chatMargin;

   void get();
   void set();
};

};

#endif


