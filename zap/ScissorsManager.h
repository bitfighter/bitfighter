//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SCISSORS_MANAGER_H_
#define _SCISSORS_MANAGER_H_

#include "Point.h"
#include "ConfigEnum.h"          // For DisplayMode def
#include "tnlTypes.h"

// Need this ugliness to get GLint and GLboolean
#if defined(TNL_OS_MOBILE) || defined(BF_USE_GLES)
#  include "SDL_opengles.h"
#else
#  include "SDL_opengl.h"
#endif

using namespace TNL; 


namespace Zap
{

class ClientGame;

// Class for managing scissor settings and reducing repeated code

class ScissorsManager
{
private:
   GLboolean mScissorsWasEnabled;
   GLint mScissorBox[4];
   bool mManagerEnabled;

public:
   void enable(bool enable, DisplayMode displayMode, F32 x, F32 y, F32 width, F32 height);  // Store previous scissors settings
   void disable();                                                                          // Restore previous scissors settings
};


};

#endif

