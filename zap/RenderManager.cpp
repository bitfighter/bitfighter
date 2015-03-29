//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "RenderManager.h"
#include "Color.h"
#include "Point.h"

#include "tnlTypes.h"
#include "tnlLog.h"

namespace Zap
{


GL *RenderManager::mGL = NULL;

RenderManager::RenderManager()
{
   // Do nothing
}

RenderManager::~RenderManager()
{
   // Do nothing
}


void RenderManager::init()
{
   TNLAssert(mGL == NULL, "GL Renderer should only be created once!");

#ifdef BF_USE_GLES2
   mGL = new GLES2();
#else
   mGL = new GLES1();
#endif

   mGL->init();
}


void RenderManager::shutdown()
{
   TNLAssert(mGL != NULL, "GL Renderer should have been created!");
   delete mGL;
}


GL *RenderManager::getGL()
{
   TNLAssert(mGL != NULL, "GL Renderer should not be NULL!");
   return mGL;
}

////////////////////////////////////
////////////////////////////////////
// OpenGL API abstractions


GL::GL()
{
   // Do nothing
}


GL::~GL()
{
   // Do nothing
}


#ifdef BF_USE_GLES2

GLES2::GLES2()
{
   // Do nothing
}


GLES2::~GLES2()
{
   // Do nothing
}


void GLES2::init() {
   // TODO Shader initialization goes here
}

#else

GLES1::GLES1()
{
   // Do nothing
}


GLES1::~GLES1()
{
   // Do nothing
}

void GLES1::init() {
   // No initialization for the fixed-function pipeline!
}


// API methods

void GLES1::glColor(const Color &c, float alpha)
{
    glColor4f(c.r, c.g, c.b, alpha);
}


void GLES1::glColor(const Color *c, float alpha)
{
    glColor4f(c->r, c->g, c->b, alpha);
}


void GLES1::glColor(F32 c, float alpha)
{
   glColor4f(c, c, c, alpha);
}


void GLES1::glColor(F32 r, F32 g, F32 b)
{
   glColor4f(r, g, b, 1.0f);
}


void GLES1::glColor(F32 r, F32 g, F32 b, F32 alpha)
{
   glColor4f(r, g, b, alpha);
}


void GLES1::glScale(const Point &scaleFactor)
{
    glScalef(scaleFactor.x, scaleFactor.y, 1);
}


void GLES1::glScale(F32 scaleFactor)
{
    glScalef(scaleFactor, scaleFactor, 1);
}


void GLES1::glScale(F32 xScaleFactor, F32 yScaleFactor)
{
    glScalef(xScaleFactor, yScaleFactor, 1);
}


void GLES1::glTranslate(const Point &pos)
{
   glTranslatef(pos.x, pos.y, 0);
}


void GLES1::glTranslate(F32 x, F32 y)
{
   glTranslatef(x, y, 0);
}


void GLES1::glTranslate(F32 x, F32 y, F32 z)
{
   glTranslatef(x, y, z);
}


void GLES1::glRotate(F32 angle)
{
   glRotatef(angle, 0, 0, 1.0f);
}


void GLES1::setDefaultBlendFunction()
{
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void GLES1::renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_BYTE, 0, verts);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}


void GLES1::renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_SHORT, 0, verts);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}


void GLES1::renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType,
      S32 start, S32 stride)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, stride, verts);
   glDrawArrays(geomType, start, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}


void GLES1::renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType,
      S32 start, S32 stride)
{
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   // stride is the byte offset between consecutive vertices or colors
   glVertexPointer(2, GL_FLOAT, stride, vertices);
   glColorPointer(4, GL_FLOAT, stride, colors);
   glDrawArrays(geomType, start, vertCount);

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
}


// geomType: GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP, GL_TRIANGLES, GL_TRIANGLE_FAN, etc.
void GLES1::renderPointVector(const Vector<Point> *points, U32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, points->address());
   glDrawArrays(geomType, 0, points->size());

   glDisableClientState(GL_VERTEX_ARRAY);
}


void GLES1::renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType)
{
   glPushMatrix();
      glTranslate(offset);
      renderPointVector(points, geomType);
   glPopMatrix();
}


void GLES1::renderLine(const Vector<Point> *points)
{
   renderPointVector(points, GL_LINE_STRIP);
}

#endif


} /* namespace Zap */
