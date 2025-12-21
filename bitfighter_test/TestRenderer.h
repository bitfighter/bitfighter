//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#pragma once

#include "Renderer.h"
#include "Point.h"

#include <array>
#include <memory>
#include <unordered_set>
#include <vector>

namespace Zap
{

// A rendering stub to be used when we're testing things that need a renderer.
class TestRenderer final : public Renderer
{
public:
   static void install();

   ~TestRenderer() override = default;

   void clear() override;
   void clearStencil() override;
   void clearDepth() override;
   void setClearColor(F32 r, F32 g, F32 b, F32 alpha = 1.0f) override;
   void setColor(F32 r, F32 g, F32 b, F32 alpha = 1.0f) override;

   void setLineWidth(F32 width) override;
   void setPointSize(F32 size) override;
   void enableAntialiasing() override;
   void disableAntialiasing() override;
   void enableBlending() override;
   void disableBlending() override;
   void useTransparentBlackBlending() override;
   void useSpyBugBlending() override;
   void useDefaultBlending() override;
   void enableDepthTest() override;
   void disableDepthTest() override;

   void enableStencil() override;
   void disableStencil() override;
   void useAndStencilTest() override;
   void useNotStencilTest() override;
   void enableStencilDrawOnly() override;
   void disableStencilDraw() override;

   void setViewport(S32 x, S32 y, S32 width, S32 height) override;
   Point getViewportPos() override;
   Point getViewportSize() override;

   void enableScissor() override;
   void disableScissor() override;
   bool isScissorEnabled() override;
   void setScissor(S32 x, S32 y, S32 width, S32 height) override;
   Point getScissorPos() override;
   Point getScissorSize() override;

   void scale(F32 x, F32 y, F32 z = 1.0f) override;
   void translate(F32 x, F32 y, F32 z = 0.0f) override;
   void rotate(F32 degAngle, F32 x, F32 y, F32 z) override;

   void setMatrixMode(MatrixType type) override;
   void getMatrix(MatrixType type, F32* matrix) override;
   void pushMatrix() override;
   void popMatrix() override;
   void loadMatrix(const F32* m) override;
   void loadMatrix(const F64* m) override;
   void loadIdentity() override;
   void projectOrtho(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ) override;

   U32 generateTexture(bool useLinearFiltering = true) override;
   void bindTexture(U32 textureHandle) override;
   bool isTexture(U32 textureHandle) override;
   void deleteTexture(U32 textureHandle) override;
   void setTextureData(TextureFormat format, DataType dataType, U32 width, U32 height, const void* data) override;
   void setSubTextureData(TextureFormat format, DataType dataType, S32 xOffset, S32 yOffset,
      U32 width, U32 height, const void* data) override;

   void readFramebufferPixels(TextureFormat format, DataType dataType, S32 x, S32 y, S32 width, S32 height, void* data) override;

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

   void renderColoredTexture(const F32 verts[], const F32 UVs[], U32 vertCount, RenderType type,
      U32 start = 0, U32 stride = 0, U32 vertDimension = 2, bool isAlphaTexture = false) override;

private:
   TestRenderer();

   static std::array<F32, 16> identityMatrix();
   std::vector<std::array<F32, 16>>& activeStack();
   const std::vector<std::array<F32, 16>>& stackFor(MatrixType type) const;

   std::vector<std::array<F32, 16>> mModelStack;
   std::vector<std::array<F32, 16>> mProjectionStack;
   MatrixType mMatrixMode;

   Point mViewportPos;
   Point mViewportSize;

   bool mScissorEnabled;
   Point mScissorPos;
   Point mScissorSize;

   bool mStencilEnabled;
   bool mStencilDrawOnly;
   bool mUseAndStencil;

   bool mAntialiasEnabled;
   bool mBlendingEnabled;
   bool mDepthTestEnabled;

   std::array<F32, 4> mCurrentColor;

   U32 mBoundTexture;
   U32 mNextTextureHandle;
   std::unordered_set<U32> mTextures;
};

} // namespace Zap
