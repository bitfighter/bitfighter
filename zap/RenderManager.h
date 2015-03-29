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

#include "glinc.h"

using namespace TNL;

namespace Zap
{

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
   virtual void renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType) = 0;
   virtual void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType) = 0;
   virtual void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0) = 0;
   virtual void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0) = 0;
   virtual void renderLine(const Vector<Point> *points) = 0;

   virtual void glScale(const Point &scaleFactor) = 0;
   virtual void glScale(F32 scaleFactor) = 0;
   virtual void glScale(F32 xScaleFactor, F32 yScaleFactor) = 0;
   virtual void glTranslate(const Point &pos) = 0;
   virtual void glTranslate(F32 x, F32 y) = 0;
   virtual void glTranslate(F32 x, F32 y, F32 z) = 0;
   virtual void glRotate(F32 angle) = 0;

   virtual void setDefaultBlendFunction() = 0;
};


#ifdef BF_USE_GLES2

class GLES2: public GL
{
public:
   GLES2();          // Constructor
   virtual ~GLES2(); // Destructor

   void init();
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
   void renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType);
   void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType);
   void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0);
   void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType,
         S32 start = 0, S32 stride = 0);
   void renderLine(const Vector<Point> *points);

   void glScale(const Point &scaleFactor);
   void glScale(F32 scaleFactor);
   void glScale(F32 xScaleFactor, F32 yScaleFactor);
   void glTranslate(const Point &pos);
   void glTranslate(F32 x, F32 y);
   void glTranslate(F32 x, F32 y, F32 z);
   void glRotate(F32 angle);

   void setDefaultBlendFunction();
};
#endif


} /* namespace Zap */

#endif /* RENDERMANAGER_H_ */
