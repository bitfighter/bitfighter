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

#ifndef SCREENINFO_H_
#define SCREENINFO_H_

#include "tnlTypes.h"
#include "Point.h"
#include "ConfigEnum.h"

#ifndef ZAP_DEDICATED
#include "SDL.h"
#endif

using namespace TNL;

namespace Zap {


////////////////////////////////////////
////////////////////////////////////////

class ScreenInfo
{
private:
   static const S32 GAME_WIDTH  = 800;
   static const S32 GAME_HEIGHT = 600;

   F32 MIN_SCALING_FACTOR;       // Limits minimum window size

   Point mWindowMousePos, mCanvasMousePos;

   S32 mPhysicalScreenWidth, mPhysicalScreenHeight;
   S32 mGameCanvasWidth, mGameCanvasHeight;     // Size of screen; in game, will always be 800x600, but may be different in editor fullscreen
   S32 mPrevCanvasWidth, mPrevCanvasHeight;     // Previous size of screen
   S32 mWindowWidth, mWindowHeight;             // Window dimensions in physical pixels
   F32 mScalingRatioX, mScalingRatioY;          // Ratio of physical pixels to virtual pixels
   bool mIsLandscape;                           // Is our screen landscape or portrait?

   bool mActualized;                            // True once screen has been actualized

   F32 mPixelRatio;                             // Number of physical pixels that are used to draw a single virtual pixel
   void calcPixelRatio();

public:
   ScreenInfo();      // Constructor

   F32 getMinScalingFactor();

   // Can't initialize until SDL has been set up
   void init(S32 physicalScreenWidth, S32 physicalScreenHeight);

   void setWindowSize(S32 width, S32 height);
   S32 getWindowWidth();
   S32 getWindowHeight();

   // The following methods return values in PHYSICAL pixels -- how large is the entire physical monitor?
   S32 getPhysicalScreenWidth();
   S32 getPhysicalScreenHeight();

   // Game canvas size in physical pixels, assuming full screen unstretched mode
   S32 getDrawAreaWidth();
   S32 getDrawAreaHeight();

   // This is the number of physical pixels that are used to draw a single virtual pixel -- larger windows will have larger pixelRaios
   F32 getPixelRatio();

   F32 getScalingRatio();

   // Dimensions of black bars in physical pixels in full-screen unstretched mode.  Does not reflect current window mode
   S32 getHorizPhysicalMargin();
   S32 getVertPhysicalMargin();

   // Dimensions of black bars in physical pixes, based on current window mode
   S32 getHorizPhysicalMargin(DisplayMode mode);
   S32 getVertPhysicalMargin (DisplayMode mode);

   // The following methods return values in VIRTUAL pixels, not accurate in editor
   void setGameCanvasSize(S32 width, S32 height);
   void resetGameCanvasSize();
   S32 getGameCanvasWidth();
   S32 getGameCanvasHeight();

   S32 getPrevCanvasWidth();
   S32 getPrevCanvasHeight();

   // Dimensions of black bars in game-sized pixels
   S32 getHorizDrawMargin();
   S32 getVertDrawMargin();

   bool isLandscape();     // Whether physical screen is landscape, or at least more landscape than our game window

   // Convert physical window screen coordinates into virtual, in-game coordinate
   Point convertWindowToCanvasCoord(const Point &p, DisplayMode mode);
   Point convertWindowToCanvasCoord(S32 x, S32 y, DisplayMode mode);

   Point convertCanvasToWindowCoord(S32 x, S32 y, DisplayMode mode);

   void setMousePos      (S32 x, S32 y, DisplayMode mode);
   void setCanvasMousePos(S32 x, S32 y, DisplayMode mode);

   const Point *getMousePos();
   const Point *getWindowMousePos();

   bool isActualized();
   void setActualized();

   // SDL information
#ifndef ZAP_DEDICATED
#  if SDL_VERSION_ATLEAST(2,0,0)
      SDL_Window *sdlWindow;
      SDL_GLContext *sdlGlContext;
#  endif
#endif

};

extern ScreenInfo gScreenInfo;

};

#endif /* SCREENINFO_H_ */
