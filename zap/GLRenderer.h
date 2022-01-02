//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GLRENDERER_H_
#define _GLRENDERER_H_

#include "Renderer.h"

namespace Zap
{

class GLRenderer: public Renderer
{
private:
   bool mUsingAndStencilTest;

protected:
   GLRenderer();
   U32 getGLRenderType(RenderType type) const;
   U32 getGLTextureFormat(TextureFormat format) const;
   U32 getGLDataType(DataType type) const;

public:
   ~GLRenderer() override;

   void clear() override;
   void clearStencil() override;
   void clearDepth() override;
   void setClearColor(F32 r, F32 g, F32 b, F32 alpha = 1.0f) override;
    
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

   U32 generateTexture(bool useLinearFiltering = true) override;
   void bindTexture(U32 textureHandle) override;
   bool isTexture(U32 textureHandle) override;
   void deleteTexture(U32 textureHandle) override;
   void setTextureData(TextureFormat format, DataType dataType, U32 width, U32 height, const void* data) override;
   void setSubTextureData(TextureFormat format, DataType dataType, S32 xOffset, S32 yOffset,
      U32 width, U32 height, const void* data) override;

   void readFramebufferPixels(TextureFormat format, DataType dataType, S32 x, S32 y, S32 width, S32 height, void* data) override;
};


}

#endif /* _GLRENDERER_H_ */