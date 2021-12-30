//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ScissorsManager.h"     // Class header
#include "DisplayManager.h"
#include "Renderer.h"


namespace Zap
{
   
// Store previous scissors settings
void ScissorsManager::enable(bool enable, DisplayMode displayMode, F32 x, F32 y, F32 width, F32 height) 
{
   mManagerEnabled = enable;

   Renderer& r = Renderer::get();
   if(!enable)
      return;

   mScissorsWasEnabled = r.isScissorEnabled();
   if (mScissorsWasEnabled)
   {
      Point scissorPos = r.getScissorPos();
      Point scissorSize = r.getScissorSize();

      mScissorBox[0] = scissorPos.x;
      mScissorBox[1] = scissorPos.y;
      mScissorBox[2] = scissorSize.x;
      mScissorBox[3] = scissorSize.y;
   }

   static Point p1, p2;
   p1 = DisplayManager::getScreenInfo()->convertCanvasToWindowCoord(x,     DisplayManager::getScreenInfo()->getGameCanvasHeight() - y - height, displayMode);
   p2 = DisplayManager::getScreenInfo()->convertCanvasToWindowCoord(width, height,                                         displayMode);

   r.setScissor(p1.x, p1.y, p2.x, p2.y);
   r.enableScissor();
}


// Restore previous scissors settings
void ScissorsManager::disable()
{
   Renderer& r = Renderer::get();
   if(!mManagerEnabled)
      return;

   if(mScissorsWasEnabled)
      r.setScissor(mScissorBox[0], mScissorBox[1], mScissorBox[2], mScissorBox[3]);
   else
      r.disableScissor();

   mManagerEnabled = false;
}


};
