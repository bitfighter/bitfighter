//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ScissorsManager.h"     // Class header
#include "DisplayManager.h"
#include "RenderUtils.h"


namespace Zap
{
   
// Store previous scissors settings
void ScissorsManager::enable(bool enable, DisplayMode displayMode, F32 x, F32 y, F32 width, F32 height) 
{
   mManagerEnabled = enable;

   if(!enable)
      return;

   // I think this is all that is needed here.  Maybe nvgSave (with nvgRestore
   // in disable() ) might be needed in certain cases
   nvgScissor(nvg, x, y, width, height);
}


// Restore previous scissors settings
void ScissorsManager::disable()
{
   if(!mManagerEnabled)
      return;

   nvgResetScissor(nvg);

   mManagerEnabled = false;
}


};
