//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef RENDERMANAGER_H_
#define RENDERMANAGER_H_

#ifdef ZAP_DEDICATED
#  error "RenderManager.h should not be included in dedicated build"
#endif

#include "tnlTypes.h"
#include "tnlVector.h"

using namespace TNL;

namespace Zap
{


// Here we abstract all GL_* options so no class will require the OpenGL
// headers
class GLOPT {
public:
   static const U32 Back;
   static const U32 Blend;
   static const U32 ColorBufferBit;
   static const U32 DepthBufferBit;
   static const U32 DepthTest;
   static const U32 DepthWritemask;
   static const U32 Float;
   static const U32 Front;
   static const U32 Less;
   static const U32 LineLoop;
   static const U32 LineSmooth;
   static const U32 LineStrip;
   static const U32 Lines;
   static const U32 Modelview;
   static const U32 ModelviewMatrix;
   static const U32 One;
   static const U32 OneMinusDstColor;
   static const U32 PackAlignment;
   static const U32 Points;
   static const U32 Projection;
   static const U32 Rgb;
   static const U32 ScissorBox;
   static const U32 ScissorTest;
   static const U32 Short;
   static const U32 TriangleFan;
   static const U32 TriangleStrip;
   static const U32 Triangles;
   static const U32 UnsignedByte;
   static const U32 Viewport;
};


class GL;
class Color;
class Point;

// Render classes can sub-class this to gain access to the GL* object
class RenderManager
{
protected:
   static GL *mGL;

public:
   RenderManager();
   virtual ~RenderManager();

   static void init();
   static void shutdown();

   static GL *getGL();
};


// This class is the interface layer for all OpenGL calls.  Each method must
// be implemented in a child class
class GL
{
public:
   GL();          // Constructor
   virtual ~GL(); // Destructor

   // Interface methods
   virtual void init() = 0;


   virtual void glColor(const Color &c, float alpha = 1.0) = 0;
   virtual void glColor(const Color *c, float alpha = 1.0) = 0;
   virtual void glColor(F32 c, float alpha = 1.0) = 0;
   virtual void glColor(F32 r, F32 g, F32 b) = 0;
   virtual void glColor(F32 r, F32 g, F32 b, F32 alpha) = 0;

   virtual void renderPointVector(const Vector<Point> *points, U32 geomType) = 0;
   virtual void renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType) = 0;  // Same, but with points offset some distance
   virtual void renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0) = 0;
   virtual void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0) = 0;
   virtual void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0) = 0;
   virtual void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount,
         S32 geomType, S32 start = 0, S32 stride = 0) = 0;
   virtual void renderLine(const Vector<Point> *points) = 0;

   virtual void glScale(const Point &scaleFactor) = 0;
   virtual void glScale(F32 scaleFactor) = 0;
   virtual void glScale(F32 xScaleFactor, F32 yScaleFactor) = 0;
   virtual void glTranslate(const Point &pos) = 0;
   virtual void glTranslate(F32 x, F32 y) = 0;
   virtual void glTranslate(F32 x, F32 y, F32 z) = 0;
   virtual void glRotate(F32 angle) = 0;
   virtual void glLineWidth(F32 width) = 0;
   virtual void glViewport(S32 x, S32 y, S32 width, S32 height) = 0;
   virtual void glScissor(S32 x, S32 y, S32 width, S32 height) = 0;
   virtual void glPointSize(F32 size) = 0;
   virtual void glLoadIdentity() = 0;
   virtual void glOrtho(F64 left, F64 right, F64 bottom, F64 top, F64 near, F64 far) = 0;
   virtual void glClear(U32 mask) = 0;
   virtual void glClearColor(F32 red, F32 green, F32 blue, F32 alpha) = 0;
   virtual void glPixelStore(U32 name, S32 param) = 0;
   virtual void glReadBuffer(U32 mode) = 0;
   virtual void glReadPixels(S32 x, S32 y, U32 width, U32 height, U32 format, U32 type, void *data) = 0;
   virtual void glViewport(S32 x, S32 y, U32 width, U32 height) = 0;

   virtual void glBlendFunc(U32 sourceFactor, U32 destFactor) = 0;
   virtual void setDefaultBlendFunction() = 0;
   virtual void glDepthFunc(U32 func) = 0;

   virtual void glGetValue(U32 name, U8 *gl_bool) = 0;
   virtual void glGetValue(U32 name, S32 *gl_int) = 0;
   virtual void glGetValue(U32 name, F32 *gl_float) = 0;

   virtual void glPushMatrix() = 0;
   virtual void glPopMatrix() = 0;
   virtual void glMatrixMode(U32 mode) = 0;

   virtual void glEnable(U32 option) = 0;
   virtual void glDisable(U32 option) = 0;
   virtual bool glIsEnabled(U32 option) = 0;
};


#ifdef BF_USE_GLES2

class GLES2: public GL
{
public:
   GLES2();          // Constructor
   virtual ~GLES2(); // Destructor

   void init();

   // Ideas on reimplementing the fixed-function pipeline.
   //
   // Matrix stack:
   //    https://stackoverflow.com/a/21216491
};
#else
// This implementation is for using the OpenGL ES 1.1 API (which is a subset
// of desktop OpenGL 1.1 compatible [a subset]).
class GLES1: public GL
{
public:
   GLES1();          // Constructor
   virtual ~GLES1(); // Destructor

   void init();


   // GL methods
   void glColor(const Color &c, float alpha = 1.0);
   void glColor(const Color *c, float alpha = 1.0);
   void glColor(F32 c, float alpha = 1.0);
   void glColor(F32 r, F32 g, F32 b);
   void glColor(F32 r, F32 g, F32 b, F32 alpha);

   void renderPointVector(const Vector<Point> *points, U32 geomType);
   void renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType);  // Same, but with points offset some distance
   void renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0);
   void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0);
   void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0);
   void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount,
         S32 geomType, S32 start = 0, S32 stride = 0);
   void renderLine(const Vector<Point> *points);

   void glScale(const Point &scaleFactor);
   void glScale(F32 scaleFactor);
   void glScale(F32 xScaleFactor, F32 yScaleFactor);
   void glTranslate(const Point &pos);
   void glTranslate(F32 x, F32 y);
   void glTranslate(F32 x, F32 y, F32 z);
   void glRotate(F32 angle);
   void glLineWidth(F32 width);
   void glViewport(S32 x, S32 y, S32 width, S32 height);
   void glScissor(S32 x, S32 y, S32 width, S32 height);
   void glPointSize(F32 size);
   void glLoadIdentity();
   void glOrtho(F64 left, F64 right, F64 bottom, F64 top, F64 near, F64 far);
   void glClear(U32 mask);
   void glClearColor(F32 red, F32 green, F32 blue, F32 alpha);
   void glPixelStore(U32 name, S32 param);
   void glReadBuffer(U32 mode);
   void glReadPixels(S32 x, S32 y, U32 width, U32 height, U32 format, U32 type, void *data);
   void glViewport(S32 x, S32 y, U32 width, U32 height);

   void glBlendFunc(U32 sourceFactor, U32 destFactor);
   void setDefaultBlendFunction();
   void glDepthFunc(U32 func);

   void glGetValue(U32 name, U8 *fill);
   void glGetValue(U32 name, S32 *fill);
   void glGetValue(U32 name, F32 *fill);

   void glPushMatrix();
   void glPopMatrix();
   void glMatrixMode(U32 mode);

   void glEnable(U32 option);
   void glDisable(U32 option);
   bool glIsEnabled(U32 option);
};
#endif


} /* namespace Zap */

#endif /* RENDERMANAGER_H_ */
