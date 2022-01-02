//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef BF_USE_LEGACY_GL

#include "GL2Renderer.h"
#include "glad/glad.h"
#include <memory>

#define MAX_NUMBER_OF_VERTICES 300000

namespace Zap
{

GL2Renderer::GL2Renderer()
   : mStaticShader("static", "static.v.glsl", "static.f.glsl")
   , mDynamicShader("dynamic", "dynamic.v.glsl", "dynamic.f.glsl")
   , mTexturedShader("textured", "textured.v.glsl", "textured.f.glsl")
   , mColoredTextureShader("coloredTexture", "coloredTexture.v.glsl", "coloredTexture.f.glsl")
   , mPositionBuffer(0)
   , mColorBuffer(0)
   , mUVBuffer(0)
   , mTextureEnabled(false)
   , mAlpha(1.0f)
{
	// Give each stack one identity matrix
	mModelViewMatrixStack.push(glm::mat4(1.0f));
	mProjectionMatrixStack.push(glm::mat4(1.0f));
	mMatrixMode = MatrixType::ModelView;

	initBuffers();
	initRenderer();
}

GL2Renderer::~GL2Renderer()
{
	U32 buffers[] = { mPositionBuffer, mColorBuffer, mUVBuffer };
	glDeleteBuffers(ARRAYSIZE(buffers), buffers);
}

void GL2Renderer::initBuffers()
{
	// Generate big, reusable buffers for our vertex data
	// Position buffer
	glGenBuffers(1, &mPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_NUMBER_OF_VERTICES * 4 * 2, nullptr, GL_DYNAMIC_DRAW); // 2D

	// Color buffer
	glGenBuffers(1, &mColorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mColorBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_NUMBER_OF_VERTICES * 4 * 4, nullptr, GL_DYNAMIC_DRAW); // 4D

	// UV buffer
	glGenBuffers(1, &mUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_NUMBER_OF_VERTICES * 4 * 2, nullptr, GL_DYNAMIC_DRAW); // 2D
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