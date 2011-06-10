/*
 * OpenglUtils.h
 *
 *  Created on: Jun 8, 2011
 *
 *  Most of this was taken directly from freeglut sources
 */

#ifndef OPENGLUTILS_H_
#define OPENGLUTILS_H_

#include "tnlTypes.h"
#include "SDL/SDL_opengl.h"

namespace Zap {

class OpenglUtils {
public:
   static void drawCharacter(TNL::S32 character);
   static TNL::S32 getStringLength(const unsigned char* string);
};

}

#endif /* OPENGLUTILS_H_ */
