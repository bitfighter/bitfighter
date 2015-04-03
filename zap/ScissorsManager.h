//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SCISSORS_MANAGER_H_
#define _SCISSORS_MANAGER_H_

#include "tnlTypes.h"

#include "Point.h"
#include "ConfigEnum.h"          // For DisplayMode def
#include "RenderManager.h"

using namespace TNL; 


namespace Zap
{

class ClientGame;

// Class for managing scissor settings and reducing repeated code

class ScissorsManager: RenderManager
{
private:
   U8 mScissorsWasEnabled;
   S32 mScissorBox[4];
   bool mManagerEnabled;

public:
   void enable(bool enable, DisplayMode displayMode, F32 x, F32 y, F32 width, F32 height);  // Store previous scissors settings
   void disable();                                                                          // Restore previous scissors settings
};


};

#endif

