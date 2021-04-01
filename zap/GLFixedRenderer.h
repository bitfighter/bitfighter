//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GLFIXEDRENDERER_H_
#define _GLFIXEDRENDERER_H_

#include "Renderer.h"

namespace Zap
{

   class GLFixedRenderer: public Renderer
   {
   public:
      GLFixedRenderer();
      ~GLFixedRenderer() override;

      void setColor(const Color& c, float alpha = 1.0) override;
      void scale(F32 scaleFactor) override;
      void translate(const Point& pos) override;

      void renderPointVector(const Vector<Point>* points, U32 geomType) override;
      void renderPointVector(const Vector<Point>* points, const Point& offset, U32 geomType) override;

      void renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType) override;
      void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType) override;
      void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType) override;

      void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType) override;
      void renderTexturedVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount,
         U32 geomType, U32 start = 0, U32 stride = 0) override;
      void renderColoredTextureVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount,
         U32 geomType, U32 start = 0, U32 stride = 0) override;

      void renderLine(const Vector<Point>* points) override;
   };


}

#endif /* _GLFIXEDRENDERER_H_ */