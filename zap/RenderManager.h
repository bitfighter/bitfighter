//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef RENDERMANAGER_H_
#define RENDERMANAGER_H_

namespace Zap
{


class GL;

class RenderManager
{
private:
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
};
#endif


} /* namespace Zap */

#endif /* RENDERMANAGER_H_ */
