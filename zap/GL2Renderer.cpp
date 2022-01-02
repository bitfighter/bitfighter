//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef BF_USE_LEGACY_GL

#include "GL2Renderer.h"
#include "glad/glad.h"
#include "SDL.h"
#include "glm/gtc/matrix_transform.hpp" // For matrix transformations
#include "glm/gtc/type_ptr.hpp" // glm to array conversions
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

// Uses static shader
template<typename T>
void GL2Renderer::renderGenericVertexArray(DataType dataType, const T verts[], U32 vertCount, RenderType type,
	U32 start, U32 stride, U32 vertDimension)
{
	GLuint shaderId = mStaticShader.getId();
	glUseProgram(shaderId);

	glm::mat4 MVP = mProjectionMatrixStack.top() * mModelViewMatrixStack.top();
	glUniformMatrix4fv(mStaticShader.findUniform("MVP"), 1, GL_FALSE, &MVP[0][0]);
	glUniform4f(mStaticShader.findUniform("color"), mColor.r, mColor.g, mColor.b, mAlpha);
	glUniform1i(mStaticShader.findUniform("time"), static_cast<int>(SDL_GetTicks())); // Give time, it's always useful!

	// Get the vertex position attribute location in the shader
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
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	glm::mat4 newMatrix = glm::scale(stack.top(), glm::vec3(x, y, z));
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::translate(F32 x, F32 y, F32 z)
{
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	glm::mat4 newMatrix = glm::translate(stack.top(), glm::vec3(x, y, z));
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::rotate(F32 degAngle, F32 x, F32 y, F32 z)
{
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	glm::mat4 newMatrix = glm::rotate(stack.top(), glm::radians(degAngle), glm::vec3(x, y, z));
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::setMatrixMode(MatrixType type)
{
	mMatrixMode = type;
}

void GL2Renderer::getMatrix(MatrixType type, F32 *matrix)
{
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	const F32 *sourceMatrix = static_cast<const F32 *>(glm::value_ptr(stack.top()));

	for(int i = 0; i < 16; ++i)
		matrix[i] = sourceMatrix[i];
}

void GL2Renderer::pushMatrix()
{
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	// Duplicate the top matrix on top of the stack
	glm::mat4 currentMatrix = stack.top();
	stack.push(currentMatrix);
}

void GL2Renderer::popMatrix()
{
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;
	stack.pop();
}

// m is column-major
void GL2Renderer::loadMatrix(const F32 *m)
{
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	// Replace top matrix
	stack.pop();
	stack.push(glm::make_mat4(m));
}

// Results in loss of precision!
void GL2Renderer::loadMatrix(const F64 *m)
{
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	// Replace top matrix
	stack.pop();
	stack.push(static_cast<glm::fmat4>(glm::make_mat4(m))); // Conversion exists between matrix types
}

void GL2Renderer::loadIdentity()
{
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	// Replace the top matrix with an identity matrix
	glm::mat4 newMatrix = glm::mat4(1.0f);
	stack.pop();
	stack.push(newMatrix);
}

void GL2Renderer::projectOrtho(F64 left, F64 right, F64 bottom, F64 top, F64 nearx, F64 farx)
{
	std::stack<glm::mat4> &stack = (mMatrixMode == MatrixType::ModelView) ? mModelViewMatrixStack : mProjectionMatrixStack;

	// Multiply the top matrix with an ortho matrix
	glm::mat4 ortho = glm::ortho(left, right, bottom, top, nearx, farx);
	glm::mat4 topMatrix = stack.top();

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
	//renderGenericVertexArray(DataType::Short, verts, vertCount, type, start, stride, vertDimension);
}

void GL2Renderer::renderVertexArray(const F32 verts[], U32 vertCount, RenderType type,
   U32 start, U32 stride, U32 vertDimension)
{
	renderGenericVertexArray(DataType::Float, verts, vertCount, type, start, stride, vertDimension);
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