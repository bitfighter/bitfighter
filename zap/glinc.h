//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// This file is for various preprocessor logic required for bitfighter usage of
// OpenGL-related headers.  You should include this file instead of the OpenGL
// headers.

#ifndef _GLINC_H_
#define _GLINC_H_
#if defined(BF_USE_GLES2)
#  include "SDL_opengles2.h"
#elif defined(BF_USE_GLES)
#  include "SDL_opengles.h"
   // Needed for GLES compatibility
#  define glOrtho glOrthof
#else
//#  include "SDL_opengl.h"
#  include "../lib/include/libsdl/SDL2/SDL_opengl.h"

#endif

#endif // _GLINC_H_
