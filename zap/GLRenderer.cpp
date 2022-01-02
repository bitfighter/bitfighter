//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GLRenderer.h"
#include "DisplayManager.h"
#include "Color.h"
#include "Point.h"
#include "tnlVector.h"

#ifdef BF_USE_LEGACY_GL
#  ifdef BF_USE_GLES
#     include "SDL_opengles.h"
#  else
#     include "SDL_opengl.h"
#  endif
#else
#  include "glad/glad.h"
#endif


namespace Zap
{

GLRenderer::GLRenderer()
 : mUsingAndStencilTest(0)
{
#ifndef BF_USE_LEGACY_GL
   TNLAssert(gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress), "Unable to load GL functions!");
#endif

   glDepthFunc(GL_LESS);
   glDepthMask(true);   // Always enable writing to depth buffer, needed for glClearing depth buffer

   glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
   glStencilMask(0xFF); // Always enable writing to stencil buffer, needed for glClearing stencil buffer

   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

GLRenderer::~GLRenderer()
{
   // Do nothing
}

U32 GLRenderer::getGLRenderType(RenderType type) const
{
   switch(type)
   {
   case RenderType::Points:
      return GL_POINTS;

   case RenderType::Lines:
      return GL_LINES;

   case RenderType::LineStrip:
      return GL_LINE_STRIP;

   case RenderType::LineLoop:
      return GL_LINE_LOOP;

   case RenderType::Triangles:
      return GL_TRIANGLES;

   case RenderType::TriangleStrip:
      return GL_TRIANGLE_STRIP;

   case RenderType::TriangleFan:
      return GL_TRIANGLE_FAN;

   default:
         return 0;
   }
}

U32 GLRenderer::getGLTextureFormat(TextureFormat format) const
{
   switch(format)
   {
   case TextureFormat::RGB:
      return GL_RGB;

   case TextureFormat::RGBA:
      return GL_RGBA;

   case TextureFormat::Alpha:
      return GL_ALPHA;

   default:
      return 0;
   }
}

U32 GLRenderer::getGLDataType(DataType type) const
{
   switch(type)
   {
   case DataType::UnsignedByte:
      return GL_UNSIGNED_BYTE;

   case DataType::Byte:
      return GL_BYTE;

   case DataType::UnsignedShort:
      return GL_UNSIGNED_SHORT;

   case DataType::Short:
      return GL_SHORT;

   case DataType::UnsignedInt:
      return GL_UNSIGNED_INT;

   case DataType::Int:
      return GL_INT;

   case DataType::Float:
      return GL_FLOAT;

   default:
      return 0;
   }
}

void GLRenderer::clear()
{
   glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLRenderer::clearStencil()
{
   glClear(GL_STENCIL_BUFFER_BIT);
}

void GLRenderer::clearDepth()
{
   glClear(GL_DEPTH_BUFFER_BIT);
}

void GLRenderer::setClearColor(F32 r, F32 g, F32 b, F32 alpha)
{
   glClearColor(r, g, b, alpha);
}

void GLRenderer::setLineWidth(F32 width)
{
   glLineWidth(width);
}

void GLRenderer::setPointSize(F32 size)
{
   glPointSize(size);
}

void GLRenderer::enableAntialiasing()
{
   glEnable(GL_LINE_SMOOTH);
   // glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

void GLRenderer::disableAntialiasing()
{
   glDisable(GL_LINE_SMOOTH);
}

void GLRenderer::enableBlending()
{
   glEnable(GL_BLEND);
}

void GLRenderer::disableBlending()
{
   glDisable(GL_BLEND);
}

void GLRenderer::useSpyBugBlending()
{
   // This blending works like this, source(SRC) * GL_ONE_MINUS_DST_COLOR + destination(DST) * GL_ONE
   glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
}

void GLRenderer::useDefaultBlending()
{
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GLRenderer::enableDepthTest()
{
   glEnable(GL_DEPTH_TEST);
}

void GLRenderer::disableDepthTest()
{
   glDisable(GL_DEPTH_TEST);
}

/// Stencils
void GLRenderer::enableStencil()
{
   glEnable(GL_STENCIL_TEST);
}

void GLRenderer::disableStencil()
{
   // Enable writing to stencil in case we disabled it, needed for clearing buffer
   glStencilMask(0xFF);
   glDisable(GL_STENCIL_TEST);
}

void GLRenderer::useAndStencilTest()
{
   // Render if stencil value == 1
   glStencilFunc(GL_EQUAL, 1, 0xFF);
   mUsingAndStencilTest = true;
}

void GLRenderer::useNotStencilTest()
{
   // Render if stencil value != 1
   glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
   mUsingAndStencilTest = false;
}

void GLRenderer::enableStencilDrawOnly()
{
   // Always draw to stencil buffer; we don't care what what's in there already
   glStencilFunc(GL_ALWAYS, 1, 0xFF);
   glStencilMask(0xFF);                                 // Draw 1s everywhere in stencil buffer
   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Don't draw to color buffer!
}

// Temporarily disable drawing to stencil
void GLRenderer::disableStencilDraw()
{
   glStencilMask(0x00);                             // Don't draw anything in the stencil buffer
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Feel free to draw in the color buffer tho!

   // Restore stencil test, it was probably modified by enableStencilDrawOnly()
   if(mUsingAndStencilTest)
      useAndStencilTest();
   else
      useNotStencilTest();
}

void GLRenderer::setViewport(S32 x, S32 y, S32 width, S32 height)
{
   glViewport(x, y, width, height);
}

Point GLRenderer::getViewportPos()
{
   GLint viewport[4];
   glGetIntegerv(GL_VIEWPORT, viewport);

   return Point(viewport[0], viewport[1]);
}

Point GLRenderer::getViewportSize()
{
   GLint viewport[4];
   glGetIntegerv(GL_VIEWPORT, viewport);

   return Point(viewport[2], viewport[3]);
}

void GLRenderer::enableScissor()
{
   glEnable(GL_SCISSOR_TEST);
}

void GLRenderer::disableScissor()
{
   glDisable(GL_SCISSOR_TEST);
}

bool GLRenderer::isScissorEnabled()
{
   GLboolean scissorEnabled;
   glGetBooleanv(GL_SCISSOR_TEST, &scissorEnabled);

   return scissorEnabled;
}

void GLRenderer::setScissor(S32 x, S32 y, S32 width, S32 height)
{
   glScissor(x, y, width, height);
}

Point GLRenderer::getScissorPos()
{
   GLint scissor[4];
   glGetIntegerv(GL_SCISSOR_BOX, scissor);

   return Point(scissor[0], scissor[1]);
}

Point GLRenderer::getScissorSize()
{
   GLint scissor[4];
   glGetIntegerv(GL_SCISSOR_BOX, scissor);

   return Point(scissor[2], scissor[3]);
}

U32 GLRenderer::generateTexture()
{
   GLuint textureHandle;
   glGenTextures(1, &textureHandle);
   return textureHandle;
}

void GLRenderer::bindTexture(U32 textureHandle)
{
   glBindTexture(GL_TEXTURE_2D, textureHandle);
}

bool GLRenderer::isTexture(U32 textureHandle)
{
   return glIsTexture(textureHandle);
}

void GLRenderer::deleteTexture(U32 textureHandle)
{
   glDeleteTextures(1, &textureHandle);
}

void GLRenderer::setTextureData(TextureFormat format, DataType dataType, U32 width, U32 height, const void* data)
{
   U32 textureFormat = getGLTextureFormat(format);

   glTexImage2D(
      GL_TEXTURE_2D, 0, textureFormat,
      width, height, 0,
      textureFormat, getGLDataType(dataType), data);

   // Set filtering
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void GLRenderer::setSubTextureData(TextureFormat format, DataType dataType, S32 xOffset, S32 yOffset,
   U32 width, U32 height, const void* data)
{
   glTexSubImage2D(
      GL_TEXTURE_2D, 0,
      xOffset, yOffset,
      width, height,
      getGLTextureFormat(format),
      getGLDataType(dataType), data);
}

// Fairly slow operation
void GLRenderer::readFramebufferPixels(TextureFormat format, DataType dataType, S32 x, S32 y, S32 width, S32 height, void* data)
{
   glReadBuffer(GL_BACK);
   glReadPixels(
      x, y, width, height,
      getGLTextureFormat(format),
      getGLDataType(dataType),
      data);
}

}