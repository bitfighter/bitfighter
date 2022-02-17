//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef BF_USE_LEGACY_GL

#include "GL2Renderer.h"
#include "MathUtils.h"
#include "glad/glad.h"
#include "SDL.h"

#include <memory>
#include <cstddef> // For size_t

namespace Zap
{

GL2Renderer::GL2Renderer()
   : mStaticShader("static", "static.v.glsl", "static.f.glsl")
   , mDynamicShader("dynamic", "dynamic.v.glsl", "dynamic.f.glsl")
   , mTexturedShader("textured", "textured.v.glsl", "textured.f.glsl")
   , mColoredTextureShader("coloredTexture", "coloredTexture.v.glsl", "coloredTexture.f.glsl")
   , mTextureEnabled(false)
   , mAlpha(1.0f)
   , mPointSize(1.0f)
   , mCurrentShaderId(0)
   , mMatrixMode(MatrixType::ModelView)
{
   // Give each stack an identity matrix
   mModelViewMatrixStack.push(Matrix4());
   mProjectionMatrixStack.push(Matrix4());
	initRenderer();
}

GL2Renderer::~GL2Renderer()
{
	// Do nothing
}

void GL2Renderer::useShader(const Shader &shader)
{
   if(mCurrentShaderId != shader.getId())
   {
      glUseProgram(shader.getId());
      mCurrentShaderId = shader.getId();
   }
}

// Static
void GL2Renderer::create()
{
   setInstance(std::unique_ptr<Renderer>(new GL2Renderer));
}

// Uses static shader
template<typename T>
void GL2Renderer::renderGenericVertexArray(DataType dataType, const T verts[], U32 vertCount, RenderType type,
	U32 start, U32 stride, U32 vertDimension)
{
   useShader(mStaticShader);

	Matrix4 MVP = mProjectionMatrixStack.top() * mModelViewMatrixStack.top();
   mStaticShader.setMVP(MVP);
   mStaticShader.setColor(mColor, mAlpha);
   mStaticShader.setPointSize(mPointSize);
   mStaticShader.setTime(static_cast<GLuint>(SDL_GetTicks())); // Give time, it's always useful!

	// Get the position attribute location in the shader
	GLint attribLocation = mStaticShader.getAttributeLocation(AttributeName::VertexPosition);

	// Give position data to the shader, and deal with stride
   // Positions
   U32 bytesPerCoord = sizeof(T) * vertDimension;
   if(stride > bytesPerCoord) // Should never be less than
      bytesPerCoord = stride;

   mPositionBuffer.bind();
   std::size_t positionOffset = mPositionBuffer.insertData((U8 *)verts + (start * bytesPerCoord), bytesPerCoord * vertCount);

	glVertexAttribPointer(
		attribLocation,	       // Attribute index
		vertDimension,				 // Number of values per vertex
		getGLDataType(dataType), // Data type
		GL_FALSE,			       // Normalized?
		stride,				       // Stride
		(void *)positionOffset	 // Array buffer offset
	);

	// Draw!
	glDrawArrays(getGLRenderType(type), 0, vertCount);
}

void GL2Renderer::setColor(F32 r, F32 g, F32 b, F32 alpha)
{
	mColor = Color(r, g, b);
	mAlpha = alpha;
}

void GL2Renderer::setPointSize(F32 size)
{
   mPointSize = size;

#ifndef BF_USE_GLES
   // GL2 does not support vertex shader gl_PointSize, while GLES2 does not suport glPointSize().
   glPointSize(size);
#endif
}

void GL2Renderer::scale(F32 x, F32 y, F32 z)
{
	// Choose correct stack
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	Matrix4 newMatrix = stack.top().scale(x, y, z);
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::translate(F32 x, F32 y, F32 z)
{
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	Matrix4 newMatrix = stack.top().translate(x, y, z);
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::rotate(F32 degAngle, F32 x, F32 y, F32 z)
{
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	Matrix4 newMatrix = stack.top().rotate(degreesToRadians(degAngle), x, y, z);
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::setMatrixMode(MatrixType type)
{
	mMatrixMode = type;
}

void GL2Renderer::getMatrix(MatrixType type, F32 *matrix)
{
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	const F32 *sourceMatrix = stack.top().getData();

	for(int i = 0; i < 16; ++i)
		matrix[i] = sourceMatrix[i];
}

void GL2Renderer::pushMatrix()
{
	// Duplicate the top matrix on top of the stack
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.push(stack.top());
}

void GL2Renderer::popMatrix()
{
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.pop();
}

// m is column-major
void GL2Renderer::loadMatrix(const F32 *m)
{
	// Replace top matrix
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.pop();
	stack.push(Matrix4(m));
}

// Results in loss of precision!
void GL2Renderer::loadMatrix(const F64 *m)
{
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.pop();
	stack.push(Matrix4(m));
}

void GL2Renderer::loadIdentity()
{
	// Replace the top matrix with an identity matrix
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.pop();
	stack.push(Matrix4());
}

void GL2Renderer::projectOrtho(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ)
{
	// Multiply the top matrix with an ortho matrix
   MatrixStack &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	Matrix4 topMatrix = stack.top();
	Matrix4 ortho = Matrix4::getOrthoProjection(left, right, bottom, top, nearZ, farZ);

	stack.pop();
	stack.push(ortho * topMatrix);
}


void GL2Renderer::renderVertexArray(const S8 verts[], U32 vertCount, RenderType type,
   U32 start, U32 stride, U32 vertDimension)
{
	renderGenericVertexArray(DataType::Byte, verts, vertCount, type, start, stride, vertDimension);
}

void GL2Renderer::renderVertexArray(const S16 verts[], U32 vertCount, RenderType type,
   U32 start, U32 stride, U32 vertDimension)
{
	renderGenericVertexArray(DataType::Short, verts, vertCount, type, start, stride, vertDimension);
}

void GL2Renderer::renderVertexArray(const F32 verts[], U32 vertCount, RenderType type,
   U32 start, U32 stride, U32 vertDimension)
{
	renderGenericVertexArray(DataType::Float, verts, vertCount, type, start, stride, vertDimension);
}

void GL2Renderer::renderColored(const F32 verts[], const F32 colors[], U32 vertCount,
   RenderType type, U32 start, U32 stride, U32 vertDimension)
{
   useShader(mDynamicShader);

	Matrix4 MVP = mProjectionMatrixStack.top() * mModelViewMatrixStack.top();
   mDynamicShader.setMVP(MVP);
   mDynamicShader.setPointSize(mPointSize);
   mDynamicShader.setTime(static_cast<GLuint>(SDL_GetTicks()));

	// Attribute locations
	GLint vertexPositionAttrib = mDynamicShader.getAttributeLocation(AttributeName::VertexPosition);
	GLint colorAttrib = mDynamicShader.getAttributeLocation(AttributeName::VertexColor);

	// Positions
	U32 bytesPerCoord = sizeof(F32) * vertDimension;
   if(stride > bytesPerCoord) // Should never be less than
      bytesPerCoord = stride;

   mPositionBuffer.bind();
   std::size_t positionOffset = mPositionBuffer.insertData((U8 *)verts + (start * bytesPerCoord), bytesPerCoord * vertCount);

	glVertexAttribPointer(
		vertexPositionAttrib,  // Attribute index
		vertDimension,			  // Number of values per vertex
		GL_FLOAT,			     // Data type
		GL_FALSE,			     // Normalized?
		stride,				     // Stride
		(void *)positionOffset // Array buffer offset
	);

	// Colors
	bytesPerCoord = sizeof(F32) * 4;
   if(stride > bytesPerCoord) // Should never be less than
      bytesPerCoord = stride;

   mColorBuffer.bind();
   std::size_t colorOffset = mColorBuffer.insertData((U8 *)colors + (start * bytesPerCoord), bytesPerCoord * vertCount);

	glVertexAttribPointer(
		colorAttrib,          // Attribute index
		4,				          // Number of values per color
		GL_FLOAT,				 // Data type
		GL_FALSE,				 // Normalized?
		stride,					 // Stride
		(void *)colorOffset	 // Array buffer offset
	);

	// Draw!
	glDrawArrays(getGLRenderType(type), 0, vertCount);
}

void GL2Renderer::renderTextured(const F32 verts[], const F32 UVs[], U32 vertCount,
   RenderType type, U32 start, U32 stride, U32 vertDimension)
{
   useShader(mTexturedShader);

	Matrix4 MVP = mProjectionMatrixStack.top() * mModelViewMatrixStack.top();
   mTexturedShader.setMVP(MVP);
   mTexturedShader.setTextureSampler(0); // Default texture unit
   mTexturedShader.setTime(static_cast<GLuint>(SDL_GetTicks()));

	// Attribute locations
	GLint vertexPositionAttrib = mTexturedShader.getAttributeLocation(AttributeName::VertexPosition);
	GLint UVAttrib = mTexturedShader.getAttributeLocation(AttributeName::VertexUV);

   // Positions
   U32 bytesPerCoord = sizeof(F32) * vertDimension;
   if(stride > bytesPerCoord)
      bytesPerCoord = stride;

   mPositionBuffer.bind();
   std::size_t positionOffset = mPositionBuffer.insertData((U8 *)verts + (start * bytesPerCoord), bytesPerCoord * vertCount);

	glVertexAttribPointer(
		vertexPositionAttrib,  // Attribute index
		vertDimension,			  // Number of values per vertex
		GL_FLOAT,			     // Data type
		GL_FALSE,			     // Normalized?
		stride,				     // Stride
		(void *)positionOffset // Array buffer offset
	);

	// UV-coords
   bytesPerCoord = sizeof(F32) * 2;
   if(stride > bytesPerCoord)
      bytesPerCoord = stride;

   mUVBuffer.bind();
   std::size_t UVOffset = mUVBuffer.insertData((U8 *)UVs + (start * bytesPerCoord), bytesPerCoord * vertCount);

	glVertexAttribPointer(
		UVAttrib,			    // Attribute index
		2,				          // Number of values per coord
		GL_FLOAT,			    // Data type
		GL_FALSE,			    // Normalized?
		stride,				    // Stride
		(void *)UVOffset	    // Array buffer offset
	);

	// Draw!
	glDrawArrays(getGLRenderType(type), 0, vertCount);
}

// Render a texture colored by the current color:
void GL2Renderer::renderColoredTexture(const F32 verts[], const F32 UVs[], U32 vertCount,
   RenderType type, U32 start, U32 stride, U32 vertDimension, bool isAlphaTexture)
{
   useShader(mColoredTextureShader);

	// Uniforms
	Matrix4 MVP = mProjectionMatrixStack.top() * mModelViewMatrixStack.top();
   mColoredTextureShader.setMVP(MVP);
   mColoredTextureShader.setColor(mColor, mAlpha);
   mColoredTextureShader.setIsAlphaTexture(isAlphaTexture);
   mColoredTextureShader.setTextureSampler(0); // Default texture unit
   mColoredTextureShader.setTime(static_cast<GLuint>(SDL_GetTicks()));

	// Attribute locations
	GLint vertexPositionAttrib = mColoredTextureShader.getAttributeLocation(AttributeName::VertexPosition);
	GLint UVAttrib = mColoredTextureShader.getAttributeLocation(AttributeName::VertexUV);

	// Positions
   U32 bytesPerCoord = sizeof(F32) * vertDimension;
   if(stride > bytesPerCoord)
      bytesPerCoord = stride;

   mPositionBuffer.bind();
   std::size_t positionOffset = mPositionBuffer.insertData((U8*)verts + (start * bytesPerCoord), bytesPerCoord * vertCount);

	glVertexAttribPointer(
		vertexPositionAttrib,  // Attribute index
		vertDimension,			  // Number of values per vertex
		GL_FLOAT,			     // Data type
		GL_FALSE,			     // Normalized?
		stride,				     // Stride
		(void *)positionOffset // Array buffer offset
	);

   // UV-coords
   bytesPerCoord = sizeof(F32) * 2;
   if(stride > bytesPerCoord)
      bytesPerCoord = stride;

   mUVBuffer.bind();
   std::size_t UVOffset = mUVBuffer.insertData((U8 *)UVs + (start * bytesPerCoord), bytesPerCoord * vertCount);

	glVertexAttribPointer(
		UVAttrib,			    // Attribute index
		2,				          // Number of values per coord
		GL_FLOAT,			    // Data type
		GL_FALSE,			    // Normalized?
		stride,				    // Stride
		(void *)UVOffset		 // Array buffer offset
	);

	// Draw!
	glDrawArrays(getGLRenderType(type), 0, vertCount);
}

}

#endif // BF_USE_LEGACY_GL