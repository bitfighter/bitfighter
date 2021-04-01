//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "tnlTypes.h"

namespace TNL {
   template<class T> class Vector;
};

using namespace TNL;

namespace Zap
{

class Color;
class Point;


class Renderer
{
public:
   virtual ~Renderer() = default;

   virtual void setColor(const Color& c, float alpha = 1.0) = 0;
   virtual void scale(F32 scaleFactor) = 0;
   virtual void translate(const Point& pos) = 0;

   virtual void renderPointVector(const Vector<Point>* points, U32 geomType) = 0;
   virtual void renderPointVector(const Vector<Point>* points, const Point& offset, U32 geomType) = 0;

   virtual void renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType) = 0;
   virtual void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType) = 0;
   virtual void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType) = 0;

   virtual void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType) = 0;
   virtual void renderTexturedVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount,
      U32 geomType, U32 start = 0, U32 stride = 0) = 0;
   virtual void renderColoredTextureVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount,
      U32 geomType, U32 start = 0, U32 stride = 0) = 0;

   virtual void renderLine(const Vector<Point>* points) = 0;
};


}

#endif /* _RENDERER_H_ */