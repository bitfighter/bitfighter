//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GLLegacyRenderer.h"
#include "DisplayManager.h"
#include "Color.h"
#include "Point.h"
#include "tnlVector.h"
#include "SDL_opengl.h" // Basic OpenGL support

namespace Zap
{

// Private
GLLegacyRenderer::GLLegacyRenderer()
{
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_SCISSOR_TEST);    // Turn on clipping
   glEnable(GL_BLEND);

   //glPixelStorei(GL_PACK_ALIGNMENT, 1);
   //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

GLLegacyRenderer::~GLLegacyRenderer()
{

}

U32 GLLegacyRenderer::getGLRenderType(RenderType type) const
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

   case RenderType::Quads:
      return GL_QUADS;

   default:
         return 0;
   }
}

U32 GLLegacyRenderer::getGLTextureFormat(TextureFormat format) const
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

U32 GLLegacyRenderer::getGLDataType(DataType type) const
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

// Static
void GLLegacyRenderer::create()
{
   setInstance(std::unique_ptr<Renderer>(new GLLegacyRenderer));
}

void GLLegacyRenderer::clear()
{
   glClear(GL_COLOR_BUFFER_BIT);
}

void GLLegacyRenderer::setClearColor(F32 r, F32 g, F32 b, F32 alpha)
{
   glClearColor(r, g, b, alpha);
}

void GLLegacyRenderer::setColor(F32 r, F32 g, F32 b, F32 alpha)
{
   glColor4f(r, g, b, alpha);
}

void GLLegacyRenderer::setLineWidth(F32 width)
{
   glLineWidth(width);
}

void GLLegacyRenderer::setPointSize(F32 size)
{
   glPointSize(size);
}

void GLLegacyRenderer::setViewport(S32 x, S32 y, S32 width, S32 height)
{
   glViewport(x, y, width, height);
}

Point GLLegacyRenderer::getViewportPos()
{
   GLint viewport[4];
   glGetIntegerv(GL_VIEWPORT, viewport);

   return Point(viewport[0], viewport[1]);
}

Point GLLegacyRenderer::getViewportSize()
{
   GLint viewport[4];
   glGetIntegerv(GL_VIEWPORT, viewport);

   return Point(viewport[2], viewport[3]);
}

void GLLegacyRenderer::scale(F32 x, F32 y, F32 z)
{
   glScalef(x, y, z);
}

void GLLegacyRenderer::translate(F32 x, F32 y, F32 z)
{
   glTranslatef(x, y, z);
}

void GLLegacyRenderer::rotate(F32 angle, F32 x, F32 y, F32 z)
{
   glRotatef(angle, x, y, z);
}

void GLLegacyRenderer::setMatrixMode(MatrixType type)
{
   switch (type)
   {
   case MatrixType::ModelView:
      glMatrixMode(GL_MODELVIEW);
      break;

   case MatrixType::Projection:
      glMatrixMode(GL_PROJECTION);
      break;

   default:
      break;
   }
}

void GLLegacyRenderer::getMatrix(MatrixType type, F32* matrix)
{
   switch (type)
   {
   case MatrixType::ModelView:
      glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
      break;

   case MatrixType::Projection:
      glGetFloatv(GL_PROJECTION_MATRIX, matrix);
      break;

   default:
      break;
   }
}

void GLLegacyRenderer::pushMatrix()
{
   glPushMatrix();
}

void GLLegacyRenderer::popMatrix()
{
   glPopMatrix();
}

void GLLegacyRenderer::loadMatrix(const F32* m)
{
   glLoadMatrixf(m);
}

void GLLegacyRenderer::loadMatrix(const F64* m)
{
   glLoadMatrixd(m);
}

void GLLegacyRenderer::loadIdentity()
{
   glLoadIdentity();
}

void GLLegacyRenderer::projectOrtho(F64 left, F64 right, F64 bottom, F64 top, F64 nearx, F64 farx)
{
   glOrtho(left, right, bottom, top, nearx, farx);
}

U32 GLLegacyRenderer::generateTexture()
{
   GLuint textureHandle;
   glGenTextures(1, &textureHandle);
   return textureHandle;
}

void GLLegacyRenderer::bindTexture(U32 textureHandle)
{
   glBindTexture(GL_TEXTURE_2D, textureHandle);
}

bool GLLegacyRenderer::isTexture(U32 textureHandle)
{
   return glIsTexture(textureHandle);
}

void GLLegacyRenderer::deleteTexture(U32 textureHandle)
{
   glDeleteTextures(1, &textureHandle);
}

void GLLegacyRenderer::setTextureData(TextureFormat format, DataType dataType, U32 width, U32 height, const void* data)
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

void GLLegacyRenderer::setSubTextureData(TextureFormat format, DataType dataType, S32 xOffset, S32 yOffset,
   U32 width, U32 height, const void* data)
{
   glTexSubImage2D(
      GL_TEXTURE_2D, 0,
      xOffset, yOffset,
      width, height,
      getGLTextureFormat(format),
      getGLDataType(dataType), data);
}

void GLLegacyRenderer::renderVertexArray(const S8 verts[], U32 vertCount, RenderType type, U32 start, U32 stride, U32 vertDimension)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(vertDimension, GL_BYTE, stride, verts);
   glDrawArrays(getGLRenderType(type), start, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLLegacyRenderer::renderVertexArray(const S16 verts[], U32 vertCount, RenderType type, U32 start, U32 stride, U32 vertDimension)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(vertDimension, GL_SHORT, stride, verts);
   glDrawArrays(getGLRenderType(type), start, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLLegacyRenderer::renderVertexArray(const F32 verts[], U32 vertCount, RenderType type, U32 start, U32 stride, U32 vertDimension)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(vertDimension, GL_FLOAT, stride, verts);
   glDrawArrays(getGLRenderType(type), start, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLLegacyRenderer::renderColored(const F32 verts[], const F32 colors[], U32 vertCount,
                                    RenderType type, U32 start, U32 stride, U32 vertDimension)
{
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   glVertexPointer(vertDimension, GL_FLOAT, stride, verts);
   glColorPointer(4, GL_FLOAT, 0, colors);
   glDrawArrays(getGLRenderType(type), start, vertCount);

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLLegacyRenderer::renderTextured(const F32 verts[], const F32 UVs[], U32 vertCount,
                                     RenderType type, U32 start, U32 stride, U32 vertDimension)
{
   // !Todo properly!
   glEnable(GL_TEXTURE_2D);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   glVertexPointer(vertDimension, GL_FLOAT, stride, verts);
   glTexCoordPointer(2, GL_FLOAT, stride, UVs);
   glDrawArrays(getGLRenderType(type), start, vertCount);

   glDisable(GL_TEXTURE_2D);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void GLLegacyRenderer::renderColoredTexture(const F32 verts[], const F32 UVs[], U32 vertCount,
                                           RenderType type, U32 start, U32 stride, U32 vertDimension)
{
   // !Todo properly!
   glEnable(GL_TEXTURE_2D);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   glVertexPointer(vertDimension, GL_FLOAT, stride, verts);
   glTexCoordPointer(2, GL_FLOAT, stride, UVs);
   glDrawArrays(getGLRenderType(type), start, vertCount);

   glDisable(GL_TEXTURE_2D);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

}