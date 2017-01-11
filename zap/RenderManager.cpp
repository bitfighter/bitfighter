//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "RenderManager.h"
#include "DisplayManager.h"

#include "glinc.h"

#ifdef BF_USE_GLES
#  define NANOVG_GLES2_IMPLEMENTATION
#else
#  define NANOVG_GL2_IMPLEMENTATION
#endif
#include "../nanovg/nanovg.h"
#include "../nanovg/nanovg_gl.h"

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

#ifndef BF_USE_GLES2
const U32 GLOPT::LineSmooth = GL_LINE_SMOOTH;
#endif

const U32 GLOPT::LineStrip = GL_LINE_STRIP;
const U32 GLOPT::Lines = GL_LINES;

#ifndef BF_USE_GLES2
const U32 GLOPT::Modelview = GL_MODELVIEW;
const U32 GLOPT::ModelviewMatrix = GL_MODELVIEW_MATRIX;
#endif

const U32 GLOPT::One = GL_ONE;
const U32 GLOPT::OneMinusDstColor = GL_ONE_MINUS_DST_COLOR;
const U32 GLOPT::PackAlignment = GL_PACK_ALIGNMENT;
const U32 GLOPT::Points = GL_POINTS;

#ifndef BF_USE_GLES2
const U32 GLOPT::Projection = GL_PROJECTION;
#endif

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
NVGcontext *RenderManager::nvg = NULL;
NVGcontext *GL::nvg = NULL;

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
   TNLAssert(nvg == NULL, "NanoVG GL context should only be created once!");

//   int nvgFlags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
//   int nvgFlags = NVG_ANTIALIAS;
   int nvgFlags = 0;

#ifdef BF_USE_GLES
   nvg = nvgCreateGLES2(nvgFlags);
#else
   nvg = nvgCreateGL2(nvgFlags);
#endif

   mGL = new GL();
   mGL->init();
}


void RenderManager::shutdown()
{
   TNLAssert(mGL != NULL, "GL Renderer should have been created; never called RenderManager::init()?");
   delete mGL;

#ifdef BF_USE_GLES
   nvgDeleteGLES2(nvg);
#else
   nvgDeleteGL2(nvg);
#endif
}


GL *RenderManager::getGL()
{
   TNLAssert(mGL != NULL, "GL Renderer should not be NULL!  Run RenderManager::init() before calling this!");
   return mGL;
}


NVGcontext *RenderManager::getNVG()
{
   TNLAssert(nvg != NULL, "GL Renderer should not be NULL!  Run RenderManager::init() before calling this!");
   return nvg;
}



////////////////////////////////////
////////////////////////////////////
/// OpenGL API abstraction
///
/// Each method has a GL/GLES 1 and GLES 2 implementation


GL::GL()
{
   // Do nothing
}


GL::~GL()
{
   // Do nothing
}


void GL::init() {
#ifdef BF_USE_GLES2
   // TODO - Shaders and stuff
#else
   nvg = RenderManager::getNVG();
   TNLAssert(nvg != NULL, "NanoVG GL context is NULL!");
   // No initialization for the fixed-function pipeline!
#endif
}


// API methods

void GL::glColor(const Color &c, float alpha)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glColor4f(c.r, c.g, c.b, alpha);
#endif
}


void GL::glColor(const Color *c, float alpha)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glColor4f(c->r, c->g, c->b, alpha);
#endif
}


void GL::glColor(F32 c, float alpha)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glColor4f(c, c, c, alpha);
#endif
}


void GL::glColor(F32 r, F32 g, F32 b)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glColor4f(r, g, b, 1.0f);
#endif
}


void GL::glColor(F32 r, F32 g, F32 b, F32 alpha)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glColor4f(r, g, b, alpha);
#endif
}


void GL::glScale(const Point &scaleFactor)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glScalef(scaleFactor.x, scaleFactor.y, 1);
#endif
}


void GL::glScale(F32 scaleFactor)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glScalef(scaleFactor, scaleFactor, 1);
#endif
}


void GL::glScale(F32 xScaleFactor, F32 yScaleFactor)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glScalef(xScaleFactor, yScaleFactor, 1);
#endif
}


void GL::glTranslate(const Point &pos)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glTranslatef(pos.x, pos.y, 0);
#endif
}


void GL::glTranslate(S32 x, S32 y)
{
   glTranslate((F32)x, (F32)y);
}


void GL::glTranslate(F32 x, S32 y)
{
   glTranslate(x, (F32)y);
}


void GL::glTranslate(S32 x, F32 y)
{
   glTranslate((F32)x, y);
}


void GL::glTranslate(F32 x, F32 y)
{
   glTranslatef(x, y, 0.0f);
}


void GL::glTranslate(S32 x, S32 y, S32 z)
{
   glTranslatef((F32)x, (F32)y, (F32)z);
}


void GL::glTranslate(S32 x, S32 y, F32 z)
{
   glTranslatef((F32)x, (F32)y, z);
}


void GL::glTranslate(F32 x, F32 y, F32 z)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glTranslatef(x, y, z);
#endif
}


void GL::glRotate(F32 angle)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glRotatef(angle, 0, 0, 1.0f);
#endif
}


void GL::lineWidth(F32 angle)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glLineWidth(angle);
#endif
}


void GL::viewport(S32 x, S32 y, U32 width, U32 height)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glViewport(x, y, width, height);
#endif
}


void GL::scissor(S32 x, S32 y, S32 width, S32 height)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glScissor(x, y, width, height);

   nvgScissor(nvg, x, y, width, height);
#endif
}


void GL::pointSize(F32 size)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glPointSize(size);
#endif
}


void GL::loadIdentity()
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glLoadIdentity();
#endif
}


void GL::ortho(F64 left, F64 right, F64 bottom, F64 top, F64 nearx, F64 farx)
{
#ifdef BF_USE_GLES2
   // TODO
#else
#  ifdef BF_USE_GLES
   ::glOrthof(left, right, bottom, top, nearx, farx);
#  else
   ::glOrtho(left, right, bottom, top, nearx, farx);
#  endif
#endif
}


void GL::clear(U32 mask)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glClear(mask);
#endif
}


void GL::clearColor(F32 red, F32 green, F32 blue, F32 alpha)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glClearColor(red, green, blue, alpha);
#endif
}


void GL::pixelStore(U32 name, S32 param)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glPixelStorei(name, param);
#endif
}


void GL::readPixels(S32 x, S32 y, U32 width, U32 height, U32 format, U32 type, void *data)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glReadPixels(x, y, width, height, format, type, data);
#endif
}


void GL::setDefaultBlendFunction()
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}


void GL::blendFunc(U32 sourceFactor, U32 destFactor)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glBlendFunc(sourceFactor, destFactor);
#endif
}


void GL::depthFunc(U32 function)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glDepthFunc(function);
#endif
}


void GL::renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType, S32 start, S32 stride)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_BYTE, stride, verts);
   glDrawArrays(geomType, start, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


void GL::renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType, const Color &color, S32 start, S32 stride)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glColor(color);
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_SHORT, stride, verts);
   glDrawArrays(geomType, start, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


void GL::renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType, const Color &color, S32 start, S32 stride)
{
   glColor(color);

   renderVertexArray(verts, vertCount, geomType, start, stride);
}


void GL::renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType, const Color &color, F32 alpha, S32 start, S32 stride)
{
   glColor(color, alpha);

   renderVertexArray(verts, vertCount, geomType, start, stride);
}


void GL::renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType, S32 start, S32 stride)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, stride, verts);
   glDrawArrays(geomType, start, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


void GL::renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType, S32 start, S32 stride)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   // stride is the byte offset between consecutive vertices or colors
   glVertexPointer(2, GL_FLOAT, stride, vertices);
   glColorPointer(4, GL_FLOAT, stride, colors);
   glDrawArrays(geomType, start, vertCount);

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


// geomType: GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP, GL_TRIANGLES, GL_TRIANGLE_FAN, etc.
void GL::renderPointVector(const Vector<Point> *points, U32 geomType, const Color &color, F32 alpha)
{
   glColor(color, alpha);
   renderPointVector(points, geomType);
}


void GL::renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType, const Color &color, F32 alpha)
{
   glColor(color, alpha);
   glPushMatrix();
   glTranslate(offset);
   renderPointVector(points, geomType);
   glPopMatrix();
}


void GL::renderPointVector(const Vector<Point> *points, U32 geomType)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, points->address());
   glDrawArrays(geomType, 0, points->size());

   glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


void GL::renderLine(const Vector<Point> *points)
{
   renderPointVector(points, GL_LINE_STRIP);
}


void GL::glGetValue(U32 name, U8 *fill)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glGetBooleanv(name, fill);
#endif
}


void GL::glGetValue(U32 name, S32 *fill)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glGetIntegerv(name, fill);
#endif
}


void GL::glGetValue(U32 name, F32 *fill)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glGetFloatv(name, fill);
#endif
}


void GL::pushMatrix()
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glPushMatrix();
#endif
}


void GL::popMatrix()
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glPopMatrix();
#endif
}


void GL::matrixMode(U32 mode)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glMatrixMode(mode);
#endif
}


void GL::enable(U32 option)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glEnable(option);
#endif
}


void GL::disable(U32 option)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   ::glDisable(option);
#endif
}


bool GL::isEnabled(U32 option)
{
#ifdef BF_USE_GLES2
   // TODO
#else
   // Returns GL_TRUE == 1, or GL_FALSE == 0, so cast to bool works
   return ::glIsEnabled(option);
#endif
}


} /* namespace Zap */
