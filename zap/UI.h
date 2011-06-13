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
#include "Point.h"
#include "lineEditor.h"
#include "config.h"           // For DisplayMode enum
#include "stringUtils.h"      // For itos

#include "tnl.h"

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

extern IniSettings gIniSettings;    // For linesmoothing settings


#define glEnableBlend { if(!gIniSettings.useLineSmoothing) glEnable(GL_BLEND); }
#define glDisableBlend { if(!gIniSettings.useLineSmoothing) glDisable(GL_BLEND); }
#define glEnableBlendfromLineSmooth { if(gIniSettings.useLineSmoothing) glEnable(GL_BLEND); }
#define glDisableBlendfromLineSmooth { if(gIniSettings.useLineSmoothing) glDisable(GL_BLEND); }


const U32 MAX_GAME_NAME_LEN = 32;     // Any longer, and it won't fit on-screen
const U32 MAX_FILE_NAME_LEN = 128;     // Completely arbitrary
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
   GoFastAttributeEditorUI,
   TextItemAttributeEditorUI,
   InvalidUI,        // Not a valid UI
};

extern void glColor(const Color &c, float alpha = 1.0);
extern void glColor(const Color *c, float alpha = 1.0);

////////////////////////////////////////
////////////////////////////////////////

class ScreenInfo
{
private:
   static const S32 GAME_WIDTH = 800;
   static const S32 GAME_HEIGHT = 600;

   static const F32 MIN_SCALING_FACTOR = 0.15;       // Limits minimum window size

   Point mWindowMousePos, mCanvasMousePos;    

   S32 mPhysicalScreenWidth, mPhysicalScreenHeight;
   S32 mGameCanvasWidth, mGameCanvasHeight;     // Size of screen; in game, will always be 800x600, but may be different in editor fullscreen
   S32 mWindowWidth, mWindowHeight;             // Window dimensions in physical pixels
   F32 mScalingRatio;                           // Ratio of physical pixels to virtual pixels
   bool mIsLandscape;                           // Is our screen landscape or portrait?
   bool mHardwareSurface;                       // Is our screen going to use a hardware surface?

public:
   ScreenInfo()      // Constructor
   { 
      resetGameCanvasSize();        // Initialize GameCanvasSize vars
      setWindowSize(GAME_WIDTH, GAME_HEIGHT);      // In case these are used in a calculation before they're set... avoids spurious divide by 0
      mWindowMousePos.set(-1,-1);   // -1 is used to indicate initial run
   }     

   // Can't initialize until SDL has been set up
   void init(S32 physicalScreenWidth, S32 physicalScreenHeight)
   {
      mPhysicalScreenWidth = physicalScreenWidth;
      mPhysicalScreenHeight = physicalScreenHeight;

      F32 physicalScreenRatio = (F32)mPhysicalScreenWidth / (F32)mPhysicalScreenHeight;     
      F32 gameCanvasRatio = (F32)mGameCanvasWidth / (F32)mGameCanvasHeight;

      mIsLandscape = physicalScreenRatio >= gameCanvasRatio;
      mScalingRatio = mIsLandscape ? (F32)mPhysicalScreenHeight / (F32)mGameCanvasHeight : (F32)mPhysicalScreenWidth / (F32)mGameCanvasWidth;

      mHardwareSurface = false;
   }

   void setWindowSize(S32 width, S32 height) { mWindowWidth = width; mWindowHeight = height; }
   S32 getWindowWidth() { return mWindowWidth; }
   S32 getWindowHeight() { return mWindowHeight; }

   // The following methods return values in PHYSICAL pixels -- how large is the entire physical monitor?
   S32 getPhysicalScreenWidth() { return mPhysicalScreenWidth; }        
   S32 getPhysicalScreenHeight() { return mPhysicalScreenHeight; }      

   // Game canvas size in physical pixels, assuming full screen unstretched mode
   S32 getDrawAreaWidth()  { return mIsLandscape ? S32((F32)mGameCanvasWidth * mScalingRatio) : mPhysicalScreenWidth; }
   S32 getDrawAreaHeight() { return mIsLandscape ? mPhysicalScreenHeight : S32((F32)mGameCanvasHeight * mScalingRatio); }

   // Dimensions of black bars in physical pixels in full-screen unstretched mode.  Does not reflect current window mode
   S32 getHorizPhysicalMargin() { return mIsLandscape ? (mPhysicalScreenWidth - getDrawAreaWidth()) / 2 : 0; }
   S32 getVertPhysicalMargin()  { return mIsLandscape ? 0 : (mPhysicalScreenHeight - getDrawAreaHeight()) / 2; }

   // Dimensions of black bars in physical pixes, based on current window mode
   S32 getHorizPhysicalMargin(DisplayMode mode) { return mode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED ? getHorizPhysicalMargin() : 0; }
   S32 getVertPhysicalMargin(DisplayMode mode)  { return mode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED ? getVertPhysicalMargin() : 0; }

   // The following methods return values in VIRTUAL pixels, not accurate in editor
   void setGameCanvasSize(S32 width, S32 height) { mGameCanvasWidth = width; mGameCanvasHeight = height; }
   void resetGameCanvasSize() { mGameCanvasWidth = GAME_WIDTH; mGameCanvasHeight = GAME_HEIGHT; }
   S32 getGameCanvasWidth() { return mGameCanvasWidth; }       // canvasWidth, usually 800
   S32 getGameCanvasHeight() { return mGameCanvasHeight; }     // canvasHeight, usually 600

   // Dimensions of black bars in game-sized pixels
   S32 getHorizDrawMargin() { return mIsLandscape ? S32(getHorizPhysicalMargin() / mScalingRatio) : 0; }
   S32 getVertDrawMargin()  { return mIsLandscape ? 0 : S32(getVertPhysicalMargin() / mScalingRatio); }

   bool isLandscape() { return mIsLandscape; }     // Whether physical screen is landscape, or at least more landscape than our game window
   bool isHardwareSurface() { return mHardwareSurface; }  // Whether we can use the opengl hardware surface
   void setHardwareSurface(bool isHardwareSurface) { mHardwareSurface = isHardwareSurface; }  // Whether we can use the opengl hardware surface

   // Convert physical window screen coordinates into virtual, in-game coordinate
   Point convertWindowToCanvasCoord(const Point &p, DisplayMode mode) { return convertWindowToCanvasCoord((S32)p.x, (S32)p.y, mode); }
   Point convertWindowToCanvasCoord(S32 x, S32 y, DisplayMode mode) { 
      //printf("Point: %d, %d || canvas %2.0f, %2.0f ||margin h/v: %d/%d || window w/h: %d,%d || canvas w/h %d,%d ||physicalMargin: %d,%d\n",
      //         x, y, mCanvasMousePos.x, mCanvasMousePos.y, getHorizPhysicalMargin(mode),
      //        getVertPhysicalMargin(mode),getWindowWidth(),getWindowHeight(),
      //        mGameCanvasWidth, mGameCanvasHeight,
      //        getHorizPhysicalMargin(mode),getVertPhysicalMargin(mode));

         return Point((x - getHorizPhysicalMargin(mode)) * getGameCanvasWidth()  / (getWindowWidth()  - 2 * getHorizPhysicalMargin(mode)), 
                      (y - getVertPhysicalMargin(mode))  * getGameCanvasHeight() / (getWindowHeight() - 2 * getVertPhysicalMargin(mode))); }

   void setMousePos(S32 x, S32 y, DisplayMode mode) { mWindowMousePos.set(x,y); mCanvasMousePos.set(convertWindowToCanvasCoord(x,y,mode)); }

   const Point *getMousePos() { return &mCanvasMousePos; }
   const Point *getWindowMousePos() { return &mWindowMousePos; }

   static F32 getMinScalingFactor() {return MIN_SCALING_FACTOR; }
};

extern ScreenInfo gScreenInfo;


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

   static S32 vertMargin, horizMargin;
   static S32 messageMargin;
   static S32 chatMessageMargin;

   static bool cameFrom(UIID menuID);        // Did we arrive at our current interface via the specified interface?

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

   static void reactivatePrevUI();
   static void reactivateMenu(UserInterface target);

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
   static void drawStringfc(F32 x, F32 y, S32 size, const char *format, ...);
   static void drawStringc(F32 x, F32 y, F32 size, const char *string);

   // Draw strings right-aligned at point
   static void drawStringfr(F32 x, F32 y, S32 size, const char *format, ...);

   // Draw string and get it's width
   static S32 drawStringAndGetWidth(F32 x, F32 y, S32 size, const char *string);
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

   static F32 getStringWidthF32(F32 size, const char *string);    // TODO: make functions above return F32 and get rid of this one

   static U32 drawWrapText(char *text, S32 xpos, S32 ypos, S32 width, S32 ypos_end, S32 lineHeight, S32 fontSize, bool alignBottom = false, bool draw = true);

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


