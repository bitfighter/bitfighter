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
	mModelViewMatrixStack.push(Matrix<4>());
	mProjectionMatrixStack.push(Matrix<4>());
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

// Uses static shader
template<typename T>
void GL2Renderer::renderGenericVertexArray(DataType dataType, const T verts[], U32 vertCount, RenderType type,
	U32 start, U32 stride, U32 vertDimension)
{
	GLuint shaderId = mStaticShader.getId();
	glUseProgram(shaderId);

	Matrix<4> MVP = mProjectionMatrixStack.top() * mModelViewMatrixStack.top();
	glUniformMatrix4fv(mStaticShader.findUniform("MVP"), 1, GL_FALSE, MVP.getData());
	glUniform4f(mStaticShader.findUniform("color"), mColor.r, mColor.g, mColor.b, mAlpha);
	glUniform1i(mStaticShader.findUniform("time"), static_cast<int>(SDL_GetTicks())); // Give time, it's always useful!

	// Get the position attribute location in the shader
	GLint attribLocation = glGetAttribLocation(shaderId, "vertexPosition_modelspace");

	// Give position data to the shader, and deal with stride
	glBindBuffer(GL_ARRAY_BUFFER, mPositionBuffer);
	U32 extraBytesPerVert = 0;
	if(stride > sizeof(F32) * vertDimension) // Should never be less than
		extraBytesPerVert = stride - sizeof(F32) * vertDimension;

	glBufferSubData(GL_ARRAY_BUFFER, 0,
		(sizeof(T) * vertCount * vertDimension) + (extraBytesPerVert * vertCount),
		verts + (start * vertDimension)
	);

	// Set the attribute to point to the buffer data
	glEnableVertexAttribArray(attribLocation);
	glVertexAttribPointer(
		attribLocation,	       // Attribute index
		vertDimension,				 // Number of values per vertex
		getGLDataType(dataType), // Data type
		GL_FALSE,			       // Normalized?
		stride,				       // Stride
		(void *)0			       // Array buffer offset
	);

	// Draw!
	glDrawArrays(getGLRenderType(type), 0, vertCount);
	glDisableVertexAttribArray(attribLocation);
}

void GL2Renderer::setColor(F32 r, F32 g, F32 b, F32 alpha)
{
	mColor = Color(r, g, b);
	mAlpha = alpha;
}

void GL2Renderer::scale(F32 x, F32 y, F32 z)
{
	// Choose correct stack
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	Matrix<4> newMatrix = stack.top().scale(x, y, z);
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::translate(F32 x, F32 y, F32 z)
{
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	Matrix<4> newMatrix = stack.top().translate(x, y, z);
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::rotate(F32 degAngle, F32 x, F32 y, F32 z)
{
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	Matrix<4> newMatrix = stack.top().rotate(degreesToRadians(degAngle), x, y, z);
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::setMatrixMode(MatrixType type)
{
	mMatrixMode = type;
}

void GL2Renderer::getMatrix(MatrixType type, F32 *matrix)
{
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	const F32 *sourceMatrix = stack.top().getData();

	for(int i = 0; i < 16; ++i)
		matrix[i] = sourceMatrix[i];
}

void GL2Renderer::pushMatrix()
{
	// Duplicate the top matrix on top of the stack
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.push(stack.top());
}

void GL2Renderer::popMatrix()
{
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.pop();
}

// m is column-major
void GL2Renderer::loadMatrix(const F32 *m)
{
	// Replace top matrix
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.pop();
	stack.push(Matrix<4>(m));
}

// Results in loss of precision!
void GL2Renderer::loadMatrix(const F64 *m)
{
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.pop();
	stack.push(Matrix<4>(m));
}

void GL2Renderer::loadIdentity()
{
	// Replace the top matrix with an identity matrix
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.pop();
	stack.push(Matrix<4>());
}

void GL2Renderer::projectOrtho(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ)
{
	// Multiply the top matrix with an ortho matrix
	std::stack<Matrix<4>> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	Matrix<4> topMatrix = stack.top();
	Matrix<4> ortho = Matrix<4>::orthoProjection(left, right, bottom, top, nearZ, farZ);

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
	GLint shaderID = mDynamicShader.getId();
	glUseProgram(mDynamicShader.getId());

	Matrix<4> MVP = mProjectionMatrixStack.top() * mModelViewMatrixStack.top();
	glUniformMatrix4fv(mDynamicShader.findUniform("MVP"), 1, GL_FALSE, MVP.getData());
	glUniform1i(mDynamicShader.findUniform("time"), static_cast<int>(SDL_GetTicks()));

	// Attribute locations
	GLint vertexPositionAttrib = glGetAttribLocation(shaderID, "vertexPosition_modelspace");
	GLint colorAttrib = glGetAttribLocation(shaderID, "vertexColor");

	// Positions
	U32 extraBytesPerVert = 0;
	if(stride > sizeof(F32) * vertDimension) // Should never be less than
		extraBytesPerVert = stride - sizeof(F32) * vertDimension;

	glBindBuffer(GL_ARRAY_BUFFER, mPositionBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		(sizeof(F32) * vertCount * vertDimension) + (extraBytesPerVert * vertCount),
		verts + (start * vertDimension));

	glEnableVertexAttribArray(vertexPositionAttrib);
	glVertexAttribPointer(
		vertexPositionAttrib, // Attribute index
		vertDimension,			 // Number of values per vertex
		GL_FLOAT,			    // Data type
		GL_FALSE,			    // Normalized?
		stride,				    // Stride
		(void *)0			    // Array buffer offset
	);

	// Colors
	U32 extraBytesPerColorVert = 0;
	if(stride > sizeof(F32) * 4) // Should never be less than
		extraBytesPerColorVert = stride - sizeof(F32) * 4;

	glBindBuffer(GL_ARRAY_BUFFER, mColorBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		(sizeof(F32) * vertCount * 4) + (extraBytesPerColorVert * vertCount),
		colors + (start * 4));

	glEnableVertexAttribArray(colorAttrib);
	glVertexAttribPointer(
		colorAttrib,          // Attribute index
		4,				          // Number of values per color
		GL_FLOAT,				 // Data type
		GL_FALSE,				 // Normalized?
		stride,					 // Stride
		(void *)0			    // Array buffer offset
	);

	// Draw!
	glDrawArrays(getGLRenderType(type), 0, vertCount);
	glDisableVertexAttribArray(vertexPositionAttrib);
	glDisableVertexAttribArray(colorAttrib);
}

void GL2Renderer::renderTextured(const F32 verts[], const F32 UVs[], U32 vertCount,
   RenderType type, U32 start, U32 stride, U32 vertDimension)
{
	GLint shaderID = mTexturedShader.getId();
	glUseProgram(mTexturedShader.getId());

	Matrix<4> MVP = mProjectionMatrixStack.top() * mModelViewMatrixStack.top();
	glUniformMatrix4fv(mTexturedShader.findUniform("MVP"), 1, GL_FALSE, MVP.getData());

	// Uniforms
	GLint activeTexture = 0;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture); // Get active texture unit
	glUniform1i(mTexturedShader.findUniform("textureSampler"), activeTexture);
	glUniform1i(mDynamicShader.findUniform("time"), static_cast<int>(SDL_GetTicks()));

	// Attribute locations
	GLint vertexPositionAttrib = glGetAttribLocation(shaderID, "vertexPosition_modelspace");
	GLint UVAttrib = glGetAttribLocation(shaderID, "vertexUV");

	// Positions
	U32 extraBytesPerVert = 0;
	if(stride > sizeof(F32) * vertDimension) // Should never be less than
		extraBytesPerVert = stride - sizeof(F32) * vertDimension;

	glBindBuffer(GL_ARRAY_BUFFER, mPositionBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		(sizeof(F32) * vertCount * vertDimension) + (extraBytesPerVert * vertCount),
		verts + (start * vertDimension));

	glEnableVertexAttribArray(vertexPositionAttrib);
	glVertexAttribPointer(
		vertexPositionAttrib, // Attribute index
		vertDimension,			 // Number of values per vertex
		GL_FLOAT,			    // Data type
		GL_FALSE,			    // Normalized?
		stride,				    // Stride
		(void *)0			    // Array buffer offset
	);

	// UV-coords
	glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		(sizeof(F32) * vertCount * 2) + (extraBytesPerVert * vertCount),
		UVs + (start * 2));

	glEnableVertexAttribArray(UVAttrib);
	glVertexAttribPointer(
		UVAttrib,			    // Attribute index
		2,				          // Number of values per coord
		GL_FLOAT,			    // Data type
		GL_FALSE,			    // Normalized?
		stride,				    // Stride
		(void *)0			    // Array buffer offset
	);

	// Draw!
	glDrawArrays(getGLRenderType(type), 0, vertCount);
	glDisableVertexAttribArray(vertexPositionAttrib);
	glDisableVertexAttribArray(UVAttrib);
}

// Render a texture colored by the current color:
void GL2Renderer::renderColoredTexture(const F32 verts[], const F32 UVs[], U32 vertCount,
   RenderType type, U32 start, U32 stride, U32 vertDimension)
{
	GLint shaderID = mColoredTextureShader.getId();
	glUseProgram(mColoredTextureShader.getId());

	// Uniforms
	Matrix<4> MVP = mProjectionMatrixStack.top() * mModelViewMatrixStack.top();
	glUniformMatrix4fv(mColoredTextureShader.findUniform("MVP"), 1, GL_FALSE, MVP.getData());
	glUniform4f(mColoredTextureShader.findUniform("color"), mColor.r, mColor.g, mColor.b, mAlpha);
	glUniform1i(mDynamicShader.findUniform("time"), static_cast<int>(SDL_GetTicks()));

	GLint activeTexture = 0;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture); // Get active texture unit
	glUniform1i(mColoredTextureShader.findUniform("textureSampler"), activeTexture);

	// Attribute locations
	GLint vertexPositionAttrib = glGetAttribLocation(shaderID, "vertexPosition_modelspace");
	GLint UVAttrib = glGetAttribLocation(shaderID, "vertexUV");

	// Positions
	U32 extraBytesPerVert = 0;
	if(stride > sizeof(F32) * vertDimension) // Should never be less than
		extraBytesPerVert = stride - sizeof(F32) * vertDimension;

	glBindBuffer(GL_ARRAY_BUFFER, mPositionBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		(sizeof(F32) * vertCount * vertDimension) + (extraBytesPerVert * vertCount),
		verts + (start * vertDimension));

	glEnableVertexAttribArray(vertexPositionAttrib);
	glVertexAttribPointer(
		vertexPositionAttrib, // Attribute index
		vertDimension,			 // Number of values per vertex
		GL_FLOAT,			    // Data type
		GL_FALSE,			    // Normalized?
		stride,				    // Stride
		(void *)0			    // Array buffer offset
	);

	// UV-coords
	glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		(sizeof(F32) * vertCount * 2) + (extraBytesPerVert * vertCount),
		UVs + (start * 2));

	glEnableVertexAttribArray(UVAttrib);
	glVertexAttribPointer(
		UVAttrib,			    // Attribute index
		2,				          // Number of values per coord
		GL_FLOAT,			    // Data type
		GL_FALSE,			    // Normalized?
		stride,				    // Stride
		(void *)0			    // Array buffer offset
	);

	// Draw!
	glDrawArrays(getGLRenderType(type), 0, vertCount);
	glDisableVertexAttribArray(vertexPositionAttrib);
	glDisableVertexAttribArray(UVAttrib);
}

}

#endif // BF_USE_LEGACY_GL