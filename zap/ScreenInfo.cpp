//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ScreenInfo.h"

namespace Zap {


ScreenInfo::ScreenInfo()
{
   MIN_SCALING_FACTOR = 0.15f;
   resetGameCanvasSize();        // Initialize GameCanvasSize vars
   setWindowSize(GAME_WIDTH, GAME_HEIGHT);      // In case these are used in a calculation before they're set... avoids spurious divide by 0
   mPrevCanvasWidth  = GAME_WIDTH;
   mPrevCanvasHeight = GAME_HEIGHT;
   mWindowMousePos.set(-1,-1);   // -1 is used to indicate initial run
   mActualized = false;
}

// Destructor
ScreenInfo::~ScreenInfo()
{
   // Do nothing
}


// Can't initialize until SDL has been set up
void ScreenInfo::init(S32 physicalScreenWidth, S32 physicalScreenHeight)
{
   mPhysicalScreenWidth = physicalScreenWidth;
   mPhysicalScreenHeight = physicalScreenHeight;

   F32 physicalScreenRatio = (F32)mPhysicalScreenWidth / (F32)mPhysicalScreenHeight;
   F32 gameCanvasRatio = (F32)mGameCanvasWidth / (F32)mGameCanvasHeight;

   mIsLandscape = physicalScreenRatio >= gameCanvasRatio;

   mScalingRatioX = mIsLandscape ? (F32)mPhysicalScreenWidth  / (F32)mGameCanvasWidth  : (F32)mPhysicalScreenHeight / (F32)mGameCanvasHeight;
   mScalingRatioY = mIsLandscape ? (F32)mPhysicalScreenHeight / (F32)mGameCanvasHeight : (F32)mPhysicalScreenWidth  / (F32)mGameCanvasWidth;

//   logprintf("mIsLandscape: %d", mIsLandscape);
//
//   logprintf("physicalScreenWidth: %d", physicalScreenWidth);
//   logprintf("physicalScreenHeight: %d", physicalScreenHeight);
//   logprintf("physicalScreenRatio: %f", physicalScreenRatio);
//   logprintf("gameCanvasRatio: %f", gameCanvasRatio);
//
//   logprintf("mPhysicalScreenWidth: %d", mPhysicalScreenWidth);
//   logprintf("mPhysicalScreenHeight: %d", mPhysicalScreenHeight);
//   logprintf("mGameCanvasWidth: %d", mGameCanvasWidth);
//   logprintf("mGameCanvasHeight: %d", mGameCanvasHeight);
//
//   logprintf("mScalingRatioX: %f", mScalingRatioX);
//   logprintf("mScalingRatioY: %f", mScalingRatioY);
}


F32 ScreenInfo::getMinScalingFactor() { return MIN_SCALING_FACTOR; }

void ScreenInfo::setWindowSize(S32 width, S32 height) 
{ 
   mWindowWidth  = width; 
   mWindowHeight = height; 

   calcPixelRatio();
}


void ScreenInfo::calcPixelRatio()
{
   mPixelRatio = (F32)mWindowHeight / (F32)mGameCanvasHeight;
}


S32 ScreenInfo::getWindowWidth()  const { return mWindowWidth;  }
S32 ScreenInfo::getWindowHeight() const { return mWindowHeight; }

// The following methods return values in PHYSICAL pixels -- how large is the entire physical monitor?
S32 ScreenInfo::getPhysicalScreenWidth()  const { return mPhysicalScreenWidth;  }
S32 ScreenInfo::getPhysicalScreenHeight() const { return mPhysicalScreenHeight; }


// How many physical pixels make up a virtual one?
F32 ScreenInfo::getPixelRatio() const { return mPixelRatio; }

F32 ScreenInfo::getScalingRatio() const { return mScalingRatioY; }


// Game canvas size in physical pixels, assuming full screen unstretched mode
S32 ScreenInfo::getDrawAreaWidth() const
{
   return mIsLandscape ? S32((F32)mGameCanvasWidth * mScalingRatioY) : mPhysicalScreenWidth;
}


S32 ScreenInfo::getDrawAreaHeight() const
{
   return mIsLandscape ? mPhysicalScreenHeight : S32((F32)mGameCanvasHeight * mScalingRatioX);
}


// Dimensions of black bars in physical pixels in full-screen unstretched mode.  Does not reflect current window mode
S32 ScreenInfo::getHorizPhysicalMargin() const
{
   return mIsLandscape ? (mPhysicalScreenWidth - getDrawAreaWidth()) / 2 : 0;
}


S32 ScreenInfo::getVertPhysicalMargin() const
{
   return mIsLandscape ? 0 : (mPhysicalScreenHeight - getDrawAreaHeight()) / 2;
}


// Dimensions of black bars in physical pixes, based on current window mode
S32 ScreenInfo::getHorizPhysicalMargin(DisplayMode mode) const
{
   return mode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED ? getHorizPhysicalMargin() : 0;
}


S32 ScreenInfo::getVertPhysicalMargin(DisplayMode mode) const
{
   return mode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED ? getVertPhysicalMargin() : 0;
}


// The following methods return values in VIRTUAL pixels, not accurate in editor
void ScreenInfo::setGameCanvasSize(S32 width, S32 height)
{
   mPrevCanvasWidth  = mGameCanvasWidth;
   mPrevCanvasHeight = mGameCanvasHeight;

   mGameCanvasWidth  = width;
   mGameCanvasHeight = height;

   calcPixelRatio();
}

void ScreenInfo::resetGameCanvasSize()
{
   setGameCanvasSize(GAME_WIDTH, GAME_HEIGHT);
}  

S32 ScreenInfo::getDefaultCanvasWidth()  const { return GAME_WIDTH;  }
S32 ScreenInfo::getDefaultCanvasHeight() const { return GAME_HEIGHT; }

// These only change from the default when in the editor
S32 ScreenInfo::getGameCanvasWidth()  const { return mGameCanvasWidth;  }     // canvasWidth, usually 800
S32 ScreenInfo::getGameCanvasHeight() const { return mGameCanvasHeight; }     // canvasHeight, usually 600
                                      
S32 ScreenInfo::getPrevCanvasWidth()  const { return mPrevCanvasWidth;  }       
S32 ScreenInfo::getPrevCanvasHeight() const { return mPrevCanvasHeight; }     


// Dimensions of black bars in game-sized pixels
S32 ScreenInfo::getHorizDrawMargin() const
{
   return mIsLandscape ? S32(getHorizPhysicalMargin() / mScalingRatioY) : 0;
}

S32 ScreenInfo::getVertDrawMargin() const
{
   return mIsLandscape ? 0 : S32(getVertPhysicalMargin() / mScalingRatioX);
}

bool ScreenInfo::isLandscape() const { return mIsLandscape; }     // Whether physical screen is landscape, or at least more landscape than our game window


Point ScreenInfo::convertWindowToCanvasCoord(S32 x, S32 y, DisplayMode mode)
{
//   logprintf("Point: %d, %d || canvas %2.0f, %2.0f ||margin h/v: %d/%d || window w/h: %d,%d || canvas w/h %d,%d\n",
//         x, y,
//         mCanvasMousePos.x, mCanvasMousePos.y,
//         getHorizPhysicalMargin(mode), getVertPhysicalMargin(mode),
//         getWindowWidth(),getWindowHeight(),
//         mGameCanvasWidth, mGameCanvasHeight
//         );

   return Point((x - getHorizPhysicalMargin(mode)) * getGameCanvasWidth()  / F32(getWindowWidth()  - 2 * getHorizPhysicalMargin(mode)),
                (y - getVertPhysicalMargin(mode))  * getGameCanvasHeight() / F32(getWindowHeight() - 2 * getVertPhysicalMargin(mode)));
}


Point ScreenInfo::convertCanvasToWindowCoord(S32 x, S32 y, DisplayMode mode) const
{
   return convertCanvasToWindowCoord((F32)x, (F32)y, mode);
}


Point ScreenInfo::convertCanvasToWindowCoord(F32 x, F32 y, DisplayMode mode) const
{
   return Point(x * (getWindowWidth()  - 2 * getHorizPhysicalMargin(mode)) / getGameCanvasWidth()  + getHorizPhysicalMargin(mode),
                y * (getWindowHeight() - 2 * getVertPhysicalMargin(mode))  / getGameCanvasHeight() + getVertPhysicalMargin(mode));
}


void ScreenInfo::setMousePos(S32 x, S32 y, DisplayMode mode)
{
   mWindowMousePos.set(x, y);
   mCanvasMousePos.set(convertWindowToCanvasCoord(x, y, mode));
}


void ScreenInfo::setCanvasMousePos(S32 x, S32 y, DisplayMode mode)
{
   mWindowMousePos.set(convertCanvasToWindowCoord(x, y, mode));
   mCanvasMousePos.set(x, y);
}


bool ScreenInfo::isActualized()  { return mActualized; }
void ScreenInfo::setActualized() { mActualized = true; }


const Point *ScreenInfo::getMousePos() { return &mCanvasMousePos; }
const Point *ScreenInfo::getWindowMousePos() { return &mWindowMousePos; }

} // namespace Zap
