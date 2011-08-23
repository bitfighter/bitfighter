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
#include "keyCode.h"
#include "SharedConstants.h"
#include "Timer.h"
#include "Point.h"
#include "lineEditor.h"
#include "config.h"           // For DisplayMode enum
#include "stringUtils.h"      // For itos

#include "tnl.h"
#include "tnlLog.h"

#include "SDL/SDL_opengl.h"
#include "boost/shared_ptr.hpp"

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



#define glEnableBlend { if(!gIniSettings.useLineSmoothing) glEnable(GL_BLEND); }
#define glDisableBlend { if(!gIniSettings.useLineSmoothing) glDisable(GL_BLEND); }
#define glEnableBlendfromLineSmooth { if(gIniSettings.useLineSmoothing) glEnable(GL_BLEND); }
#define glDisableBlendfromLineSmooth { if(gIniSettings.useLineSmoothing) glDisable(GL_BLEND); }


extern void glColor(const Color &c, float alpha = 1.0);
extern void glColor(const Color *c, float alpha = 1.0);

////////////////////////////////////////
////////////////////////////////////////

template<class T, class U, class V>
      static void glColor(T in_r, U in_g, V in_b) { glColor3f(static_cast<F32>(in_r), static_cast<F32>(in_g), static_cast<F32>(in_b)); }

template<class T, class U>
      static void glVertex(T in_x, U in_y) { glVertex2f(static_cast<F32>(in_x), static_cast<F32>(in_y)); }

template<class T, class U, class V>
      static void glTranslate(T in_x, U in_y, V in_z) { glTranslatef(static_cast<F32>(in_x), static_cast<F32>(in_y), static_cast<F32>(in_z)); }


class Game;
class ClientGame;
class UIManager;

class UserInterface
{
private:
   UIID mInternalMenuID;                     // Unique interface ID
   static void doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string, bool fixed);
   static void doDrawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string, bool fixed);

   ClientGame *mClientGame;

public:
   UserInterface(ClientGame *game);                // Constructor
   virtual ~UserInterface();                       // Destructor
   static const S32 MAX_PASSWORD_LENGTH = 32;      // Arbitrary, doesn't matter, but needs to be _something_

   static UserInterface *current;            // Currently active menu

   void setMenuID(UIID menuID);              // Set interface's name
   UIID getMenuID() const;                   // Retrieve interface's name
   UIID getPrevMenuID() const;               // Retrieve previous interface's name

   ClientGame *getGame() { return mClientGame; }

   UIManager *getUIManager() const;

   static S32 vertMargin, horizMargin;
   static S32 messageMargin;
   static S32 chatMessageMargin;

   static void renderCurrent();

   virtual void render();
   virtual void idle(U32 timeDelta);
   virtual void onActivate();
   virtual void onDeactivate() { /* Do nothing */ }
   virtual void onReactivate();
   virtual void onPreDisplayModeChange() { /* Do nothing */ }
   virtual void onDisplayModeChange() { /* Do nothing */ }

   void activate(bool save = true);
   virtual void reactivate();

   void renderConsole();      // Render game console

   KeyCode convertJoystickToKeyboard(KeyCode keyCode);

   // Input event handlers
   virtual void onKeyDown(KeyCode keyCode, char ascii);
   virtual void onKeyUp(KeyCode keyCode);
   virtual void onMouseMoved(S32 x, S32 y);
   virtual void onMouseDragged(S32 x, S32 y);

   void renderMessageBox(const char *title, const char *instr, const char *message[], S32 msgLines, S32 vertOffset = 0);


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
   static void drawStringc(F32 x, F32 y, F32 size, const char *string);

   // Draw strings right-aligned at point
   static void drawStringfr(F32 x, F32 y, F32 size, const char *format, ...);

   // Draw string and get it's width
   static S32 drawStringAndGetWidth(S32 x, S32 y, S32 size, const char *string);
   static S32 drawStringAndGetWidth(F32 x, F32 y, S32 size, const char *string);
   static S32 drawStringAndGetWidthf(S32 x, S32 y, S32 size, const char *format, ...);
   static S32 drawStringAndGetWidthf(F32 x, F32 y, S32 size, const char *format, ...);


   // Draw text at an angle...
   static void drawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string);
   static void drawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string);
   

   // Original drawAngleString has a bug in positioning, but fixing it everywhere in the app would be a huge pain, so
   // we've created a new drawAngleString function without the bug, called xx_fixed.  Actual work now moved to doDrawAngleString,
   // which is marked private.  I think all usage of broken function has been removed, and _fixed can be renamed to something better.
   static void drawAngleString_fixed(F32 x, F32 y, F32 size, F32 angle, const char *string);
   static void drawAngleString_fixed(S32 x, S32 y, F32 size, F32 angle, const char *string);
   static void drawAngleStringf_fixed(F32 x, F32 y, F32 size, F32 angle, const char *format, ...);

   // Center text between two points
   static void drawStringf_2pt(Point p1, Point p2, F32 size, F32 vert_offset, const char *format, ...);

   // Draw text centered on screen (normal and formatted versions)  --> now return starting location
   static S32 drawCenteredString(S32 y, S32 size, const char *str);
   static S32 drawCenteredString(S32 x, S32 y, S32 size, const char *str);
   static F32 drawCenteredString(F32 x, F32 y, S32 size, const char *str);
   static S32 drawCenteredStringf(S32 y, S32 size, const char *format, ...);
   static S32 drawCenteredStringf(S32 x, S32 y, S32 size, const char *format, ...);

   static S32 drawCenteredStringPair(S32 xpos, S32 ypos, S32 size, const Color &leftColor, const Color &rightColor, 
                                     const char *leftStr, const char *rightStr);

   static S32 drawCenteredStringPair(S32 ypos, S32 size, const Color &leftColor, const Color &rightColor, 
                                     const char *leftStr, const char *rightStr);

   // Draw text centered in a left or right column (normal and formatted versions)  --> now return starting location
   static S32 drawCenteredString2Col(S32 y, S32 size, bool leftCol, const char *str);
   static S32 drawCenteredString2Colf(S32 y, S32 size, bool leftCol, const char *format, ...);
   static S32 drawCenteredStringPair2Colf(S32 y, S32 size, bool leftCol, const char *left, const char *right, ...);

   // Get info about where text will be draw
   static S32 get2ColStartingPos(bool leftCol);
   static S32 getCenteredStringStartingPos(S32 size, const char *string);
   static S32 getCenteredStringStartingPosf(S32 size, const char *format, ...);
   static S32 getCenteredString2ColStartingPos(S32 size, bool leftCol, const char *string);
   static S32 getCenteredString2ColStartingPosf(S32 size, bool leftCol, const char *format, ...);

   // Draw 4-column left-justified text
   static void drawString4Col(S32 y, S32 size, U32 col, const char *str);
   static void drawString4Colf(S32 y, S32 size, U32 col, const char *format, ...);

   // Return string rendering width (normal and formatted versions)
   static F32 getStringWidth(F32 size, const char *str);
   static S32 getStringWidth(S32 size, const char *str);

   static F32 getStringWidthf(F32 size, const char *format, ...);
   static S32 getStringWidthf(S32 size, const char *format, ...);

   static U32 drawWrapText(char *text, S32 xpos, S32 ypos, S32 width, S32 ypos_end,
         S32 lineHeight, S32 fontSize, S32 multiLineIndentation = 0, bool alignBottom = false, bool draw = true);

   static void playBoop();    // Make some noise!
};



// Used only for multiple mClientGame in one instance
struct UserInterfaceData
{
   UserInterface *current;            // Currently active menu
   Vector<UserInterface *> prevUIs;   // Previously active menus
   S32 vertMargin, horizMargin;
   S32 chatMargin;
   UserInterfaceData();
   void get();
   void set();
};

};

#endif


