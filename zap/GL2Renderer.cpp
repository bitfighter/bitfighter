//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef BF_USE_LEGACY_GL

#include "GL2Renderer.h"
#include "simple-opengl-loader.h"
#include <memory>

namespace Zap
{

GL2Renderer::GL2Renderer()
{
   sogl_loadOpenGL();
}

GL2Renderer::~GL2Renderer()
{
   // Do nothing
}

// Static
void GL2Renderer::create()
{
   setInstance(std::unique_ptr<Renderer>(new GL2Renderer));
}

}

#endif // BF_USE_LEGACY_GL