//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "RenderManager.h"
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

#endif


} /* namespace Zap */
