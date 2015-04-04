//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "RenderManager.h"

#define BF_ALLOW_GLHEADER 1
#include "glinc.h"
#undef BF_ALLOW_GLHEADER

#include "Color.h"
#include "Point.h"

#include "tnlTypes.h"
#include "tnlLog.h"

namespace Zap
{


const U32 GLOPT::Back = GL_BACK;
const U32 GLOPT::Blend = GL_BLEND;
const U32 GLOPT::ColorBufferBit = GL_COLOR_BUFFER_BIT;
const U32 GLOPT::DepthBufferBit = GL_DEPTH_BUFFER_BIT;
const U32 GLOPT::DepthTest = GL_DEPTH_TEST;
const U32 GLOPT::DepthWritemask = GL_DEPTH_WRITEMASK;
const U32 GLOPT::Float = GL_FLOAT;
const U32 GLOPT::Front = GL_FRONT;
const U32 GLOPT::Less = GL_LESS;
const U32 GLOPT::LineLoop = GL_LINE_LOOP;
const U32 GLOPT::LineSmooth = GL_LINE_SMOOTH;
const U32 GLOPT::LineStrip = GL_LINE_STRIP;
const U32 GLOPT::Lines = GL_LINES;
const U32 GLOPT::Modelview = GL_MODELVIEW;
const U32 GLOPT::ModelviewMatrix = GL_MODELVIEW_MATRIX;
const U32 GLOPT::One = GL_ONE;
const U32 GLOPT::OneMinusDstColor = GL_ONE_MINUS_DST_COLOR;
const U32 GLOPT::PackAlignment = GL_PACK_ALIGNMENT;
const U32 GLOPT::Points = GL_POINTS;
const U32 GLOPT::Projection = GL_PROJECTION;
const U32 GLOPT::Rgb = GL_RGB;
const U32 GLOPT::ScissorBox = GL_SCISSOR_BOX;
const U32 GLOPT::ScissorTest = GL_SCISSOR_TEST;
const U32 GLOPT::Short = GL_SHORT;
const U32 GLOPT::TriangleFan = GL_TRIANGLE_FAN;
const U32 GLOPT::TriangleStrip = GL_TRIANGLE_STRIP;
const U32 GLOPT::Triangles = GL_TRIANGLES;
const U32 GLOPT::UnsignedByte = GL_UNSIGNED_BYTE;
const U32 GLOPT::Viewport = GL_VIEWPORT;


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
   TNLAssert(mGL != NULL, "GL Renderer should have been created; never called RenderManager::init()?");
   delete mGL;
}


GL *RenderManager::getGL()
{
   TNLAssert(mGL != NULL, "GL Renderer should not be NULL!  Run RenderManager::init() before calling this!");
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


void GLES1::glLineWidth(F32 angle)
{
   ::glLineWidth(angle);
}


void GLES1::glViewport(S32 x, S32 y, S32 width, S32 height)
{
   ::glViewport(x, y, width, height);
}


void GLES1::glScissor(S32 x, S32 y, S32 width, S32 height)
{
   ::glScissor(x, y, width, height);
}


void GLES1::glPointSize(F32 size)
{
   ::glPointSize(size);
}


void GLES1::glLoadIdentity()
{
   ::glLoadIdentity();
}


void GLES1::glOrtho(F64 left, F64 right, F64 bottom, F64 top, F64 nearx, F64 farx)
{
   ::glOrtho(left, right, bottom, top, nearx, farx);
}


void GLES1::glClear(U32 mask)
{
   ::glClear(mask);
}


void GLES1::glClearColor(F32 red, F32 green, F32 blue, F32 alpha)
{
   ::glClearColor(red, green, blue, alpha);
}


void GLES1::glPixelStore(U32 name, S32 param)
{
   ::glPixelStorei(name, param);
}


void GLES1::glReadBuffer(U32 mode)
{
   ::glReadBuffer(mode);
}


void GLES1::glReadPixels(S32 x, S32 y, U32 width, U32 height, U32 format, U32 type, void *data)
{
   ::glReadPixels(x, y, width, height, format, type, data);
}


void GLES1::glViewport(S32 x, S32 y, U32 width, U32 height)
{
   ::glViewport(x, y, width, height);
}


void GLES1::setDefaultBlendFunction()
{
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void GLES1::glBlendFunc(U32 sourceFactor, U32 destFactor)
{
   ::glBlendFunc(sourceFactor, destFactor);
}


void GLES1::glDepthFunc(U32 function)
{
   ::glDepthFunc(function);
}


void GLES1::renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType,
      S32 start, S32 stride)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_BYTE, stride, verts);
   glDrawArrays(geomType, start, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}


void GLES1::renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType,
      S32 start, S32 stride)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_SHORT, stride, verts);
   glDrawArrays(geomType, start, vertCount);

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


void GLES1::renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount,
      S32 geomType, S32 start, S32 stride)
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
   ::glPushMatrix();
      glTranslate(offset);
      renderPointVector(points, geomType);
   ::glPopMatrix();
}


void GLES1::renderLine(const Vector<Point> *points)
{
   renderPointVector(points, GL_LINE_STRIP);
}


void GLES1::glGetValue(U32 name, U8 *fill)
{
   ::glGetBooleanv(name, fill);
}


void GLES1::glGetValue(U32 name, S32 *fill)
{
   ::glGetIntegerv(name, fill);
}


void GLES1::glGetValue(U32 name, F32 *fill)
{
   ::glGetFloatv(name, fill);
}


void GLES1::glPushMatrix()
{
   ::glPushMatrix();
}


void GLES1::glPopMatrix()
{
   ::glPopMatrix();
}


void GLES1::glMatrixMode(U32 mode)
{
   ::glMatrixMode(mode);
}


void GLES1::glEnable(U32 option)
{
   ::glEnable(option);
}


void GLES1::glDisable(U32 option)
{
   ::glDisable(option);
}


bool GLES1::glIsEnabled(U32 option)
{
   // Returns GL_TRUE == 1, or GL_FALSE == 0, so cast to bool works
   return ::glIsEnabled(option);
}


#endif


} /* namespace Zap */
