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

GLFixedRenderer::GLFixedRenderer()
{

}

GLFixedRenderer::~GLFixedRenderer()
{

}

void GLFixedRenderer::setColor(const Color& c, float alpha)
{
   glColor4f(c.r, c.g, c.b, alpha);
}

void GLFixedRenderer::scale(F32 scaleFactor)
{
   glScalef(scaleFactor, scaleFactor, 1);
}

void GLFixedRenderer::translate(const Point& pos)
{
   glTranslatef(pos.x, pos.y, 0);
}

void GLFixedRenderer::renderPointVector(const Vector<Point>* points, U32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, sizeof(Point), points->address());
   glDrawArrays(geomType, 0, points->size());

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderPointVector(const Vector<Point>* points, const Point& offset, U32 geomType)
{
   glPushMatrix();
   glTranslatef(offset.x, offset.y, 0);
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(2, GL_FLOAT, 0, points->address());
   glDrawArrays(geomType, 0, points->size());
   glDisableClientState(GL_VERTEX_ARRAY);
   glPopMatrix();
}

void GLFixedRenderer::renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_BYTE, 0, verts);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_SHORT, 0, verts);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, verts);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, vertices);
   glColorPointer(4, GL_FLOAT, 0, colors);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
}

void GLFixedRenderer::renderTexturedVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount, U32 geomType, U32 start, U32 stride)
{
   // Todo
}

void GLFixedRenderer::renderColoredTextureVertexArray(const F32 vertices[], const F32 UVs[], U32 vertCount, U32 geomType, U32 start, U32 stride)
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
   renderPointVector(points, GL_LINE_STRIP);
}


}