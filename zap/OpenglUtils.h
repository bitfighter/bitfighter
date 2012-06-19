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
#include "SDL_opengl.h"


using namespace TNL;

namespace Zap {

class OpenglUtils {
public:
   static void drawCharacter(S32 character);
   static S32 getStringLength(const unsigned char* string);
};

class Color;
extern void glColor(const Color &c, float alpha = 1.0);
extern void glColor(const Color *c, float alpha = 1.0);

template<class T, class U, class V>
      static void glColor(T in_r, U in_g, V in_b) { glColor3f(static_cast<F32>(in_r), static_cast<F32>(in_g), static_cast<F32>(in_b)); }

template<class T, class U>
      static void glVertex(T in_x, U in_y) { glVertex2f(static_cast<F32>(in_x), static_cast<F32>(in_y)); }

template<class T, class U, class V>
      static void glTranslate(T in_x, U in_y, V in_z) { glTranslatef(static_cast<F32>(in_x), static_cast<F32>(in_y), static_cast<F32>(in_z)); }


}

#endif /* OPENGLUTILS_H_ */
