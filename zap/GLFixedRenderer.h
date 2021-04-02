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
   private:
      GLFixedRenderer();

      U32 getGLRenderType(RenderType type) const;

   public:
      ~GLFixedRenderer() override;

      static void create();

      void clear() override;
      void setClearColor(F32 r, F32 g, F32 b, F32 alpha = 1.0f) override;

      void setColor(F32 c, F32 alpha = 1.0f) override;
      void setColor(F32 r, F32 g, F32 b, F32 alpha = 1.0f) override;
      void setColor(const Color& c, F32 alpha = 1.0f) override;
    
      void setLineWidth(F32 width) override;
      void setPointSize(F32 size) override;
      void setViewport(S32 x, S32 y, S32 width, S32 height) override;

      void scale(F32 factor) override;
      void scale(F32 x, F32 y, F32 z = 1.0f) override;
      void scale(const Point& factor) override;
      
      void translate(F32 x, F32 y, F32 z = 0.0f) override;
      void translate(const Point& offset) override;

      void rotate(F32 angle) override;
      void rotate(F32 angle, F32 x, F32 y, F32 z) override;

      void setMatrixMode(MatrixType type) override;
      void getMatrix(MatrixType type, F32* matrix) override;
      void pushMatrix() override;
      void popMatrix() override;
      void loadMatrix(const F32* m) override;
      void loadMatrix(const F64* m) override;
      void loadIdentity() override;
      void projectOrtho(F64 left, F64 right, F64 bottom, F64 top, F64 nearx, F64 farx) override;

      void renderPointVector(const Vector<Point>* points, RenderType type) override;
      void renderPointVector(const Vector<Point>* points, const Point& offset, RenderType typee) override;

      void renderVertexArray(const S8 verts[], S32 vertCount, RenderType type) override;
      void renderVertexArray(const S16 verts[], S32 vertCount, RenderType type) override;
      void renderVertexArray(const F32 verts[], S32 vertCount, RenderType type) override;

      void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, RenderType type) override;
      void renderTexturedVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount,
         RenderType type, U32 start = 0, U32 stride = 0) override;
      void renderColoredTextureVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount,
         RenderType type, U32 start = 0, U32 stride = 0) override;

      void renderLine(const Vector<Point>* points) override;
   };


}

#endif /* _GLFIXEDRENDERER_H_ */