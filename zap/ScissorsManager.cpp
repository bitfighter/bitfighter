//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ScissorsManager.h"     // Class header
#include "ScreenInfo.h"


namespace Zap
{
   
extern ScreenInfo gScreenInfo;

// Store previous scissors settings
void ScissorsManager::enable(bool enable, DisplayMode displayMode, F32 x, F32 y, F32 width, F32 height) 
{
   mManagerEnabled = enable;

   if(!enable)
      return;

   glGetBooleanv(GL_SCISSOR_TEST, &mScissorsWasEnabled);

   if(mScissorsWasEnabled)
      glGetIntegerv(GL_SCISSOR_BOX, &mScissorBox[0]);

   static Point p1, p2;
   p1 = gScreenInfo.convertCanvasToWindowCoord(x,     gScreenInfo.getGameCanvasHeight() - y - height, displayMode);
   p2 = gScreenInfo.convertCanvasToWindowCoord(width, height,                                         displayMode);

   glScissor(GLint(p1.x), GLint(p1.y), GLsizei(p2.x), GLsizei(p2.y));

   glEnable(GL_SCISSOR_TEST);
}


// Restore previous scissors settings
void ScissorsManager::disable()
{
   if(!mManagerEnabled)
      return;

   if(mScissorsWasEnabled)
      glScissor(mScissorBox[0], mScissorBox[1], mScissorBox[2], mScissorBox[3]);
   else
      glDisable(GL_SCISSOR_TEST);

   mManagerEnabled = false;
}


};