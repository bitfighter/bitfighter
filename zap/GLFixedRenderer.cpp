//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GLFixedRenderer.h"
#include "Color.h"
#include "Point.h"
#include "tnlVector.h"
#include "SDL_opengl.h" // Basic OpenGL support

namespace Zap
{

// Private
GLFixedRenderer::GLFixedRenderer()
{
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

GLFixedRenderer::~GLFixedRenderer()
{
   
}

U32 GLFixedRenderer::getGLRenderType(RenderType type) const
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

// Static
void GLFixedRenderer::create()
{
   setInstance(std::unique_ptr<Renderer>(new GLFixedRenderer));
}

void GLFixedRenderer::clear()
{
   glClear(GL_COLOR_BUFFER_BIT);
}

void GLFixedRenderer::setClearColor(F32 r, F32 g, F32 b, F32 alpha)
{
   glClearColor(r, g, b, alpha);
}

void GLFixedRenderer::setColor(F32 c, F32 alpha)
{
   glColor4f(c, c, c, alpha);
}

void GLFixedRenderer::setColor(F32 r, F32 g, F32 b, F32 alpha)
{
   glColor4f(r, g, b, alpha);
}

void GLFixedRenderer::setColor(const Color& c, F32 alpha)
{
   glColor4f(c.r, c.g, c.b, alpha);
}

void GLFixedRenderer::setLineWidth(F32 width)
{
   glLineWidth(width);
}

void GLFixedRenderer::setPointSize(F32 size)
{
   glPointSize(size);
}

void GLFixedRenderer::setViewport(S32 x, S32 y, S32 width, S32 height)
{
   glViewport(x, y, width, height);
}

void GLFixedRenderer::scale(F32 factor)
{
   glScalef(factor, factor, factor);
}

void GLFixedRenderer::scale(F32 x, F32 y, F32 z)
{
   glScalef(x, y, z);
}

void GLFixedRenderer::scale(const Point& factor)
{
   glScalef(factor.x, factor.y, 1.0f);
}

void GLFixedRenderer::translate(F32 x, F32 y, F32 z)
{
   glTranslatef(x, y, z);
}

void GLFixedRenderer::translate(const Point& offset)
{
   glTranslatef(offset.x, offset.y, 0.0f);
}

void GLFixedRenderer::rotate(F32 angle)
{
   glRotatef(angle, 0.0f, 0.0f, 1.0f);
}

void GLFixedRenderer::rotate(F32 angle, F32 x, F32 y, F32 z)
{
   glRotatef(angle, x, y, z);
}

// TEMP
void GLFixedRenderer::t_enable(U32 option)
{
   glEnable(option);
}

void GLFixedRenderer::getMatrix(MatrixType type, F32* matrix)
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

void GLFixedRenderer::pushMatrix()
{
   glPushMatrix();
}

void GLFixedRenderer::popMatrix()
{
   glPopMatrix();
}

void GLFixedRenderer::loadMatrix(const F32* m)
{
   glLoadMatrixf(m);
}

void GLFixedRenderer::loadMatrix(const F64* m)
{
   glLoadMatrixd(m);
}

void GLFixedRenderer::loadIdentity()
{
   glLoadIdentity();
}

void GLFixedRenderer::projectOrtho(F64 left, F64 right, F64 bottom, F64 top, F64 nearx, F64 farx)
{
   glOrtho(left, right, bottom, top, nearx, farx);
}

void GLFixedRenderer::renderPointVector(const Vector<Point>* points, RenderType type)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, sizeof(Point), points->address());
   glDrawArrays(getGLRenderType(type), 0, points->size());

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderPointVector(const Vector<Point>* points, const Point& offset, RenderType type)
{
   glPushMatrix();
   glTranslatef(offset.x, offset.y, 0);
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(2, GL_FLOAT, 0, points->address());
   glDrawArrays(getGLRenderType(type), 0, points->size());
   glDisableClientState(GL_VERTEX_ARRAY);
   glPopMatrix();
}

void GLFixedRenderer::renderVertexArray(const S8 verts[], S32 vertCount, RenderType type)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_BYTE, 0, verts);
   glDrawArrays(getGLRenderType(type), 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderVertexArray(const S16 verts[], S32 vertCount, RenderType type)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_SHORT, 0, verts);
   glDrawArrays(getGLRenderType(type), 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderVertexArray(const F32 verts[], S32 vertCount, RenderType type)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, verts);
   glDrawArrays(getGLRenderType(type), 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, RenderType type)
{
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, vertices);
   glColorPointer(4, GL_FLOAT, 0, colors);
   glDrawArrays(getGLRenderType(type), 0, vertCount);

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderTexturedVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount, RenderType type, U32 start, U32 stride)
{
   // Todo
}

void GLFixedRenderer::renderColoredTextureVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount, RenderType type, U32 start, U32 stride)
{
   // Todo properly
   glEnable(GL_TEXTURE_2D);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glVertexPointer(2, GL_FLOAT, stride, vertices);
   glTexCoordPointer(2, GL_FLOAT, stride, UVs);
   glDrawArrays(GL_TRIANGLES, 0, vertCount);
   glDisable(GL_TEXTURE_2D);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void GLFixedRenderer::renderLine(const Vector<Point>* points)
{
   renderPointVector(points, RenderType::LineStrip);
}


}