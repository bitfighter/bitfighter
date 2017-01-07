//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ScissorsManager.h"     // Class header
#include "DisplayManager.h"


namespace Zap
{
   
// Store previous scissors settings
void ScissorsManager::enable(bool enable, DisplayMode displayMode, F32 x, F32 y, F32 width, F32 height) 
{
   mManagerEnabled = enable;

   if(!enable)
      return;

   mGL->glGetValue(GLOPT::ScissorTest, &mScissorsWasEnabled);

   if(mScissorsWasEnabled)
      mGL->glGetValue(GLOPT::ScissorBox, &mScissorBox[0]);

   static Point p1, p2;
   p1 = DisplayManager::getScreenInfo()->convertCanvasToWindowCoord(x,     DisplayManager::getScreenInfo()->getGameCanvasHeight() - y - height, displayMode);
   p2 = DisplayManager::getScreenInfo()->convertCanvasToWindowCoord(width, height,                                         displayMode);

   mGL->scissor(S32(p1.x), S32(p1.y), S32(p2.x), S32(p2.y));

   mGL->enable(GLOPT::ScissorTest);
}


// Restore previous scissors settings
void ScissorsManager::disable()
{
   if(!mManagerEnabled)
      return;

   if(mScissorsWasEnabled)
      mGL->scissor(mScissorBox[0], mScissorBox[1], mScissorBox[2], mScissorBox[3]);
   else
      mGL->disable(GLOPT::ScissorTest);

   mManagerEnabled = false;
}


};
