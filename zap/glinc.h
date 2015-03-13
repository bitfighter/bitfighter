//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// This file is for various preprocessor logic required for bitfighter usage of
// OpenGL-related headers.  You should include this file instead of the OpenGL
// headers.

#if defined(TNL_OS_MOBILE) || defined(BF_USE_GLES)
#  include "SDL_opengles.h"
   // Needed for GLES compatibility
#  define glOrtho glOrthof
#else
#  include "SDL_opengl.h"
#endif
