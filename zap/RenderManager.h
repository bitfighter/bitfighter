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


// This implementation is for using the OpenGL ES 1.1 API (which is a subset
// of desktop OpenGL 1.1 compatible [a subset]).
class GL
{
public:
   GL();          // Constructor
   virtual ~GL(); // Destructor

   void init();

   // GL methods
   static void glColor(const Color &c, float alpha = 1.0);
   static void glColor(const Color *c, float alpha = 1.0);
   static void glColor(F32 c, float alpha = 1.0);
   static void glColor(F32 r, F32 g, F32 b);
   static void glColor(F32 r, F32 g, F32 b, F32 alpha);

   static void renderPointVector(const Vector<Point> *points, U32 geomType);
   static void renderPointVector(const Vector<Point> *points, U32 geomType, const Color &color, F32 alpha = 1.0);
   //static void renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType);  // Same, but with points offset some distance
   static void renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType, const Color &color, F32 alpha = 1.0);
   
   static void renderVertexArray(const S8  verts[], S32 vertCount, S32 geomType, S32 start = 0, S32 stride = 0);
   //static void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType, S32 start = 0, S32 stride = 0);
   static void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType, const Color &color, S32 start = 0, S32 stride = 0);
   static void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType, S32 start = 0, S32 stride = 0);
   static void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType, const Color &color, S32 start = 0, S32 stride = 0);
   static void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType, const Color &color, F32 alpha, S32 start = 0, S32 stride = 0);

   static void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType, S32 start = 0, S32 stride = 0);
   static void renderLine(const Vector<Point> *points);

   static void glScale(const Point &scaleFactor);
   static void glScale(F32 scaleFactor);
   static void glScale(F32 xScaleFactor, F32 yScaleFactor);
   static void glTranslate(const Point &pos);

   static void glTranslate(F32 x, F32 y);
   static void glTranslate(S32 x, S32 y);
   static void glTranslate(F32 x, S32 y);
   static void glTranslate(S32 x, F32 y);

   static void glTranslate(S32 x, S32 y, S32 z);
   static void glTranslate(F32 x, F32 y, F32 z);
   static void glTranslate(S32 x, S32 y, F32 z);

   static void glRotate(F32 angle);
   static void glLineWidth(F32 width);
   static void glViewport(S32 x, S32 y, S32 width, S32 height);
   static void glScissor(S32 x, S32 y, S32 width, S32 height);
   static void glPointSize(F32 size);
   static void glLoadIdentity();
   static void glOrtho(F64 left, F64 right, F64 bottom, F64 top, F64 near, F64 far);
   static void glClear(U32 mask);
   static void glClearColor(F32 red, F32 green, F32 blue, F32 alpha);
   static void glPixelStore(U32 name, S32 param);
   static void glReadPixels(S32 x, S32 y, U32 width, U32 height, U32 format, U32 type, void *data);
   static void glViewport(S32 x, S32 y, U32 width, U32 height);

   void glBlendFunc(U32 sourceFactor, U32 destFactor);
   void setDefaultBlendFunction();
   static void glDepthFunc(U32 func);

   static void glGetValue(U32 name, U8 *fill);
   void glGetValue(U32 name, S32 *fill);
   void glGetValue(U32 name, F32 *fill);

   static void glPushMatrix();
   static void glPopMatrix();
   void glMatrixMode(U32 mode);

   void glEnable(U32 option);
   void glDisable(U32 option);
   bool glIsEnabled(U32 option);
};


} /* namespace Zap */

#endif /* RENDERMANAGER_H_ */
