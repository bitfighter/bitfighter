//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GL2RENDERER_H
#define _GL2RENDERER_H

#include "GLRenderer.h"
#include "Shader.h"
#include "Matrix.h"
#include "Color.h"
#include <stack>

namespace Zap
{

class GL2Renderer : public GLRenderer
{
private:
   // Shaders
   Shader mStaticShader;
   Shader mDynamicShader;
   Shader mTexturedShader;
   Shader mColoredTextureShader;

   // Reusable buffers for vertex data
   U32 mPositionBuffer;
   U32 mColorBuffer;
   U32 mUVBuffer;

   bool mTextureEnabled;
   Color mColor;
   float mAlpha;

   std::stack<Matrix<4>> mModelViewMatrixStack;
   std::stack<Matrix<4>> mProjectionMatrixStack;
   MatrixType mMatrixMode;

   GL2Renderer();
   void initBuffers();

   template<typename T>
   void renderGenericVertexArray(DataType dataType, const T verts[], U32 vertCount, RenderType type,
      U32 start, U32 stride, U32 vertDimension);

public:
   ~GL2Renderer() override;
   static void create();

   void setColor(F32 r, F32 g, F32 b, F32 alpha = 1.0f) override;
   void scale(F32 x, F32 y, F32 z = 1.0f) override;
   void translate(F32 x, F32 y, F32 z = 0.0f) override;
   void rotate(F32 degAngle, F32 x, F32 y, F32 z) override;

   void setMatrixMode(MatrixType type) override;
   void getMatrix(MatrixType type, F32 *matrix) override;
   void pushMatrix() override;
   void popMatrix() override;
   void loadMatrix(const F32 *m) override;
   void loadMatrix(const F64 *m) override;
   void loadIdentity() override;
   void projectOrtho(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ) override;

   void renderVertexArray(const S8 verts[], U32 vertCount, RenderType type,
      U32 start = 0, U32 stride = 0, U32 vertDimension = 2) override;
   void renderVertexArray(const S16 verts[], U32 vertCount, RenderType type,
      U32 start = 0, U32 stride = 0, U32 vertDimension = 2) override;
   void renderVertexArray(const F32 verts[], U32 vertCount, RenderType type,
      U32 start = 0, U32 stride = 0, U32 vertDimension = 2) override;

   void renderColored(const F32 verts[], const F32 colors[], U32 vertCount,
      RenderType type, U32 start = 0, U32 stride = 0, U32 vertDimension = 2) override;

   void renderTextured(const F32 verts[], const F32 UVs[], U32 vertCount,
      RenderType type, U32 start = 0, U32 stride = 0, U32 vertDimension = 2) override;

   // Render a texture colored by the current color:
   void renderColoredTexture(const F32 verts[], const F32 UVs[], U32 vertCount, RenderType type,
      U32 start = 0, U32 stride = 0, U32 vertDimension = 2, bool isAlphaTexture = false) override;
};

}

#endif /* _GL2RENDERER_H */