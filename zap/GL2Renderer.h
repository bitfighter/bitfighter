//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GL2RENDERER_H
#define _GL2RENDERER_H

#include "GLRenderer.h"

namespace Zap
{

class GL2Renderer : public GLRenderer
{
private:
   GL2Renderer();

public:
   ~GL2Renderer() override;
   static void create();
};

}

#endif /* _GL2RENDERER_H */