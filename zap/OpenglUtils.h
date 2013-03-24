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
#include "FontManager.h"

#ifdef TNL_OS_MOBILE
#  include "SDL_opengles.h"
#else
#  include "SDL_opengl.h"
#endif


namespace TNL {
   template<class T> class Vector;
};

using namespace TNL;

namespace Zap {

class Color;
class Point;

extern void glColor(const Color &c, float alpha = 1.0);
extern void glColor(const Color *c, float alpha = 1.0);
extern void glColor(F32 c, float alpha = 1.0);

extern void renderPointVector(const Vector<Point> *points, U32 geomType);
extern void renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType);  // Same, but with points offset some distance
extern void renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType);
extern void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType);
extern void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType);
extern void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType);
extern void renderLine(const Vector<Point> *points);

extern void setFont(FontManager::FontId fontId);

extern void glScale(F32 scaleFactor);
extern void glTranslate(const Point &pos);
extern void setDefaultBlendFunction();

template<class T, class U, class V>
      static void glColor(T in_r, U in_g, V in_b) { glColor4f(static_cast<F32>(in_r), static_cast<F32>(in_g), static_cast<F32>(in_b), 1.0f); }

template<class T, class U, class V>
      static void glTranslate(T in_x, U in_y, V in_z) { glTranslatef(static_cast<F32>(in_x), static_cast<F32>(in_y), static_cast<F32>(in_z)); }


}

#endif /* OPENGLUTILS_H_ */
