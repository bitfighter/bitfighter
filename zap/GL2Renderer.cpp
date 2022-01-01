//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef BF_USE_LEGACY_GL

#include "GL2Renderer.h"
#include "glad/glad.h"
#include <memory>

namespace Zap
{

GL2Renderer::GL2Renderer()
{
   // Do nothing
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

void GL2Renderer::setColor(F32 r, F32 g, F32 b, F32 alpha)
{
}

void GL2Renderer::scale(F32 x, F32 y, F32 z)
{
}

void GL2Renderer::translate(F32 x, F32 y, F32 z)
{
}

void GL2Renderer::rotate(F32 angle, F32 x, F32 y, F32 z)
{
}


void GL2Renderer::setMatrixMode(MatrixType type)
{
}

void GL2Renderer::getMatrix(MatrixType type, F32 *matrix)
{
}

void GL2Renderer::pushMatrix()
{
}

void GL2Renderer::popMatrix()
{
}

void GL2Renderer::loadMatrix(const F32 *m)
{
}

void GL2Renderer::loadMatrix(const F64 *m)
{
}

void GL2Renderer::loadIdentity()
{
}

void GL2Renderer::projectOrtho(F64 left, F64 right, F64 bottom, F64 top, F64 nearx, F64 farx)
{
}


void GL2Renderer::renderVertexArray(const S8 verts[], U32 vertCount, RenderType type,
   U32 start, U32 stride, U32 vertDimension)
{
}

void GL2Renderer::renderVertexArray(const S16 verts[], U32 vertCount, RenderType type,
   U32 start, U32 stride, U32 vertDimension)
{
}

void GL2Renderer::renderVertexArray(const F32 verts[], U32 vertCount, RenderType type,
   U32 start, U32 stride, U32 vertDimension)
{
}


void GL2Renderer::renderColored(const F32 verts[], const F32 colors[], U32 vertCount,
   RenderType type, U32 start, U32 stride, U32 vertDimension)
{
}


void GL2Renderer::renderTextured(const F32 verts[], const F32 UVs[], U32 vertCount,
   RenderType type, U32 start, U32 stride, U32 vertDimension)
{
}


// Render a texture colored by the current color:
void GL2Renderer::renderColoredTexture(const F32 verts[], const F32 UVs[], U32 vertCount,
   RenderType type, U32 start, U32 stride, U32 vertDimension)
{
}


}

#endif // BF_USE_LEGACY_GL