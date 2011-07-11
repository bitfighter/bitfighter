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
#include "config.h"

namespace Zap {


////////////////////////////////////////
////////////////////////////////////////

class ScreenInfo
{
private:
   static const TNL::S32 GAME_WIDTH = 800;
   static const TNL::S32 GAME_HEIGHT = 600;

   //static const F32 MIN_SCALING_FACTOR = 0.15;       // Limits minimum window size
   //... TODO: Fix error: error C2864: 'Zap::ScreenInfo::MIN_SCALING_FACTOR' : only static const integral data members can be initialized within a class

   Point mWindowMousePos, mCanvasMousePos;

   TNL::S32 mPhysicalScreenWidth, mPhysicalScreenHeight;
   TNL::S32 mGameCanvasWidth, mGameCanvasHeight;     // Size of screen; in game, will always be 800x600, but may be different in editor fullscreen
   TNL::S32 mWindowWidth, mWindowHeight;             // Window dimensions in physical pixels
   TNL::F32 mScalingRatio;                           // Ratio of physical pixels to virtual pixels
   bool mIsLandscape;                           // Is our screen landscape or portrait?
   bool mHardwareSurface;                       // Is our screen going to use a hardware surface?

public:
   ScreenInfo();      // Constructor

   TNL::F32 getMinScalingFactor(); //{return MIN_SCALING_FACTOR; }

   // Can't initialize until SDL has been set up
   void init(TNL::S32 physicalScreenWidth, TNL::S32 physicalScreenHeight);

   void setWindowSize(TNL::S32 width, TNL::S32 height);
   TNL::S32 getWindowWidth();
   TNL::S32 getWindowHeight();

   // The following methods return values in PHYSICAL pixels -- how large is the entire physical monitor?
   TNL::S32 getPhysicalScreenWidth();
   TNL::S32 getPhysicalScreenHeight();

   // Game canvas size in physical pixels, assuming full screen unstretched mode
   TNL::S32 getDrawAreaWidth();
   TNL::S32 getDrawAreaHeight();

   // Dimensions of black bars in physical pixels in full-screen unstretched mode.  Does not reflect current window mode
   TNL::S32 getHorizPhysicalMargin();
   TNL::S32 getVertPhysicalMargin();

   // Dimensions of black bars in physical pixes, based on current window mode
   TNL::S32 getHorizPhysicalMargin(DisplayMode mode);
   TNL::S32 getVertPhysicalMargin(DisplayMode mode);

   // The following methods return values in VIRTUAL pixels, not accurate in editor
   void setGameCanvasSize(TNL::S32 width, TNL::S32 height);
   void resetGameCanvasSize();
   TNL::S32 getGameCanvasWidth();
   TNL::S32 getGameCanvasHeight();

   // Dimensions of black bars in game-sized pixels
   TNL::S32 getHorizDrawMargin();
   TNL::S32 getVertDrawMargin();

   bool isLandscape();     // Whether physical screen is landscape, or at least more landscape than our game window
   bool isHardwareSurface();  // Whether we can use the opengl hardware surface
   void setHardwareSurface(bool isHardwareSurface);  // Whether we can use the opengl hardware surface

   // Convert physical window screen coordinates into virtual, in-game coordinate
   Point convertWindowToCanvasCoord(const Point &p, DisplayMode mode);
   Point convertWindowToCanvasCoord(S32 x, S32 y, DisplayMode mode);

   void setMousePos(TNL::S32 x, TNL::S32 y, DisplayMode mode);

   const Point *getMousePos();
   const Point *getWindowMousePos();

};

extern ScreenInfo gScreenInfo;

};

#endif /* SCREENINFO_H_ */
