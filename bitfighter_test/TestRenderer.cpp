//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestRenderer.h"

#include <algorithm>
#include <cstring>

namespace Zap
{

namespace
{
   std::array<F32, 16> makeIdentity()
   {
      std::array<F32, 16> matrix{};
      matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
      return matrix;
   }
} // namespace

TestRenderer::TestRenderer()
   : mMatrixMode(MatrixType::ModelView),
   mViewportPos(0.0f, 0.0f),
   mViewportSize(0.0f, 0.0f),
   mScissorEnabled(false),
   mScissorPos(0.0f, 0.0f),
   mScissorSize(0.0f, 0.0f),
   mStencilEnabled(false),
   mStencilDrawOnly(false),
   mUseAndStencil(false),
   mAntialiasEnabled(false),
   mBlendingEnabled(false),
   mDepthTestEnabled(false),
   mCurrentColor{ 0.0f, 0.0f, 0.0f, 1.0f },
   mBoundTexture(0),
   mNextTextureHandle(0)
{
   mModelStack.push_back(makeIdentity());
   mProjectionStack.push_back(makeIdentity());
}

void TestRenderer::install()
{
   Renderer::shutdown();
   setInstance(std::unique_ptr<Renderer>(new TestRenderer()));
}

std::array<F32, 16> TestRenderer::identityMatrix()
{
   return makeIdentity();
}

std::vector<std::array<F32, 16>>& TestRenderer::activeStack()
{
   return (mMatrixMode == MatrixType::ModelView) ? mModelStack : mProjectionStack;
}

const std::vector<std::array<F32, 16>>& TestRenderer::stackFor(MatrixType type) const
{
   return (type == MatrixType::ModelView) ? mModelStack : mProjectionStack;
}

void TestRenderer::clear() {}
void TestRenderer::clearStencil() {}
void TestRenderer::clearDepth() {}

void TestRenderer::setClearColor(F32 r, F32 g, F32 b, F32 alpha)
{
   mCurrentColor = { r, g, b, alpha };
}

void TestRenderer::setColor(F32 r, F32 g, F32 b, F32 alpha)
{
   mCurrentColor = { r, g, b, alpha };
}

void TestRenderer::setLineWidth(F32) {}
void TestRenderer::setPointSize(F32) {}
void TestRenderer::enableAntialiasing() { mAntialiasEnabled = true; }
void TestRenderer::disableAntialiasing() { mAntialiasEnabled = false; }
void TestRenderer::enableBlending() { mBlendingEnabled = true; }
void TestRenderer::disableBlending() { mBlendingEnabled = false; }
void TestRenderer::useTransparentBlackBlending() {}
void TestRenderer::useSpyBugBlending() {}
void TestRenderer::useDefaultBlending() {}
void TestRenderer::enableDepthTest() { mDepthTestEnabled = true; }
void TestRenderer::disableDepthTest() { mDepthTestEnabled = false; }

void TestRenderer::enableStencil() { mStencilEnabled = true; }
void TestRenderer::disableStencil() { mStencilEnabled = false; }
void TestRenderer::useAndStencilTest() { mUseAndStencil = true; }
void TestRenderer::useNotStencilTest() { mUseAndStencil = false; }
void TestRenderer::enableStencilDrawOnly() { mStencilDrawOnly = true; }
void TestRenderer::disableStencilDraw() { mStencilDrawOnly = false; }

void TestRenderer::setViewport(S32 x, S32 y, S32 width, S32 height)
{
   mViewportPos.set(x, y);
   mViewportSize.set(width, height);
}

Point TestRenderer::getViewportPos()
{
   return mViewportPos;
}

Point TestRenderer::getViewportSize()
{
   return mViewportSize;
}

void TestRenderer::enableScissor() { mScissorEnabled = true; }
void TestRenderer::disableScissor() { mScissorEnabled = false; }
bool TestRenderer::isScissorEnabled() { return mScissorEnabled; }

void TestRenderer::setScissor(S32 x, S32 y, S32 width, S32 height)
{
   mScissorPos.set(x, y);
   mScissorSize.set(width, height);
}

Point TestRenderer::getScissorPos()
{
   return mScissorPos;
}

Point TestRenderer::getScissorSize()
{
   return mScissorSize;
}

void TestRenderer::scale(F32, F32, F32) {}
void TestRenderer::translate(F32, F32, F32) {}
void TestRenderer::rotate(F32, F32, F32, F32) {}

void TestRenderer::setMatrixMode(MatrixType type)
{
   mMatrixMode = type;
}

void TestRenderer::getMatrix(MatrixType type, F32* matrix)
{
   if (!matrix)
      return;

   const auto& stack = stackFor(type);
   std::copy(stack.back().begin(), stack.back().end(), matrix);
}

void TestRenderer::pushMatrix()
{
   activeStack().push_back(activeStack().back());
}

void TestRenderer::popMatrix()
{
   auto& stack = activeStack();
   if (stack.size() > 1)
      stack.pop_back();
}

void TestRenderer::loadMatrix(const F32* m)
{
   if (!m)
      return;

   auto& current = activeStack().back();
   std::copy(m, m + current.size(), current.begin());
}

void TestRenderer::loadMatrix(const F64* m)
{
   if (!m)
      return;

   auto& current = activeStack().back();
   for (size_t i = 0; i < current.size(); ++i)
      current[i] = static_cast<F32>(m[i]);
}

void TestRenderer::loadIdentity()
{
   activeStack().back() = identityMatrix();
}

void TestRenderer::projectOrtho(F32, F32, F32, F32, F32, F32) {}

U32 TestRenderer::generateTexture(bool)
{
   U32 handle = ++mNextTextureHandle;
   mTextures.insert(handle);
   return handle;
}

void TestRenderer::bindTexture(U32 textureHandle)
{
   mBoundTexture = textureHandle;
}

bool TestRenderer::isTexture(U32 textureHandle)
{
   return mTextures.find(textureHandle) != mTextures.end();
}

void TestRenderer::deleteTexture(U32 textureHandle)
{
   mTextures.erase(textureHandle);
   if (mBoundTexture == textureHandle)
      mBoundTexture = 0;
}

void TestRenderer::setTextureData(TextureFormat, DataType, U32, U32, const void*) {}
void TestRenderer::setSubTextureData(TextureFormat, DataType, S32, S32, U32, U32, const void*) {}

void TestRenderer::readFramebufferPixels(TextureFormat, DataType, S32, S32, S32, S32, void* data)
{
   if (data)
      std::memset(data, 0, 1); // ensure writes for determinism
}

void TestRenderer::renderVertexArray(const S8[], U32, RenderType, U32, U32, U32) {}
void TestRenderer::renderVertexArray(const S16[], U32, RenderType, U32, U32, U32) {}
void TestRenderer::renderVertexArray(const F32[], U32, RenderType, U32, U32, U32) {}

void TestRenderer::renderColored(const F32[], const F32[], U32,
   RenderType, U32, U32, U32) {
}

void TestRenderer::renderTextured(const F32[], const F32[], U32,
   RenderType, U32, U32, U32) {
}

void TestRenderer::renderColoredTexture(const F32[], const F32[], U32,
   RenderType, U32, U32, U32, bool) {
}

} // namespace Zap