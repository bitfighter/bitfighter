//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef BF_USE_LEGACY_GL

#include "Shader.h"
#include "GameSettings.h"

#include "glad/glad.h"
#include "tnlAssert.h"
#include "tnlLog.h"
#include "stringUtils.h"
#include <limits>
#include <array>

namespace Zap
{

std::string getGLShaderDebugLog(U32 object, PFNGLGETSHADERIVPROC glGet_iv, PFNGLGETSHADERINFOLOGPROC glGet_InfoLog)
{
   GLint logLength;
   std::string log;
   
   // Resize string first
   glGet_iv(static_cast<GLuint>(object), GL_INFO_LOG_LENGTH, &logLength);
   log.resize(logLength);

   if(logLength)
      glGet_InfoLog(object, logLength, NULL, &log[0]);

   // Remove null terminator added by OpenGL; std::string doesn't need this!
   log.pop_back();
   return "\n----------- GL DEBUG LOG -----------\n" + log;
}

Shader::Shader(const std::string& name, const std::string& vertexShaderFile, const std::string& fragmentShaderFile)
   : mName(name)
   , mId(0)
   , mUniformLocations()
   , mLastAlpha(0)
   , mLastPointSize(0)
   , mLastTime(0)
   , mLastIsAlphaTexture(false)
   , mLastTextureSampler(0)
{
   std::string vertexShaderCode = getShaderSource(vertexShaderFile);
   std::string fragmentShaderCode = getShaderSource(fragmentShaderFile);

   U32 vertexShader = compileShader(vertexShaderFile, vertexShaderCode, GL_VERTEX_SHADER);
   U32 fragmentShader = compileShader(fragmentShaderFile, fragmentShaderCode, GL_FRAGMENT_SHADER);

   if(vertexShader != 0 && fragmentShader != 0)
   {
      mShaders.push_back(vertexShader);
      mShaders.push_back(fragmentShader);
      mId = linkShader(mName, static_cast<GLuint>(vertexShader), static_cast<GLuint>(fragmentShader));
   }

   registerUniforms();
   registerAttributes();

   // Set initial uniform values
   glUseProgram(mId);
   setMVP(Matrix4());
   setColor(Color(), 1.0f);
   setPointSize(1.0f);
   setTime(0);
   setIsAlphaTexture(false);
   setTextureSampler(0);
}

Shader::~Shader()
{
   for(U32 shader : mShaders)
      glDeleteShader(static_cast<GLuint>(shader));

   glDeleteProgram(static_cast<GLuint>(mId));
}

// Static
std::string Shader::getShaderSource(const std::string &fileName)
{
   std::string filePath = GameSettings::getFolderManager()->findShaderFile(fileName);
   TNLAssert(filePath != "", "Expected a shader path here!");

   return readFile(filePath);
}

// Static
U32 Shader::compileShader(const std::string& shaderPath, const std::string& shaderCode, U32 type)
{
   GLint shaderLength = static_cast<GLint>(shaderCode.length());
   GLuint shader = glCreateShader(static_cast<GLuint>(type));

   // Verify shader code
   TNLAssert(shaderCode.length() < std::numeric_limits<int>::max(),
      ("Overflow! Shader '" + shaderPath + "' too long! How is this possible?!").c_str());
   TNLAssert(shaderLength > 0, ("Shader '" + shaderPath + "' is empty!").c_str());

   const char *shaderFiles[] = { shaderCode.c_str() };
   const GLint shaderFilesLength[] = { shaderLength };
   
   // Compile
   glShaderSource(shader, 1, shaderFiles, shaderFilesLength);
   glCompileShader(shader);

   // Verify compilation
   GLint shaderOk;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderOk);
   if(!shaderOk)
   {
      std::string shaderLog = getGLShaderDebugLog(shader, glGetShaderiv, glGetShaderInfoLog);
      glDeleteShader(shader);

      logprintf(shaderLog.c_str());
      TNLAssert(false, ("Failed to compile shader at '" + shaderPath + "'.").c_str());
   }

   return static_cast<U32>(shader);
}

// Static
U32 Shader::linkShader(const std::string& shaderProgramName, U32 vertexShader, U32 fragmentShader)
{
   GLint programOk;

   GLuint program = glCreateProgram();
   glAttachShader(program, static_cast<GLuint>(vertexShader));
   glAttachShader(program, static_cast<GLuint>(fragmentShader));
   glLinkProgram(program);

   glGetProgramiv(program, GL_LINK_STATUS, &programOk);

   if(!programOk)
   {
      std::string shaderLog = getGLShaderDebugLog(program, glGetProgramiv, glGetProgramInfoLog);
      glDeleteProgram(program);

      logprintf(shaderLog.c_str());
      TNLAssert(false, ("Failed to link shader program '" + shaderProgramName + "'.").c_str());
      return 0;
   }

   return program;
}

void Shader::registerUniforms()
{
   // glGetUniformLocation returns -1 if uniform was not found
   mUniformLocations[static_cast<unsigned>(UniformName::MVP)] = glGetUniformLocation(mId, "MVP");
   mUniformLocations[static_cast<unsigned>(UniformName::Color)] = glGetUniformLocation(mId, "color");
   mUniformLocations[static_cast<unsigned>(UniformName::PointSize)] = glGetUniformLocation(mId, "pointSize");
   mUniformLocations[static_cast<unsigned>(UniformName::TextureSampler)] = glGetUniformLocation(mId, "textureSampler");
   mUniformLocations[static_cast<unsigned>(UniformName::IsAlphaTexture)] = glGetUniformLocation(mId, "isAlphaTexture");
   mUniformLocations[static_cast<unsigned>(UniformName::Time)] = glGetUniformLocation(mId, "time");
}

void Shader::registerAttributes()
{
   // glGetAttribLocation returns -1 if attribute was not found
   mAttributeLocations[static_cast<unsigned>(AttributeName::VertexPosition)] = glGetAttribLocation(mId, "vertexPosition_modelspace");
   mAttributeLocations[static_cast<unsigned>(AttributeName::VertexColor)] = glGetAttribLocation(mId, "vertexColor");
   mAttributeLocations[static_cast<unsigned>(AttributeName::VertexUV)] = glGetAttribLocation(mId, "vertexUV");

   // Enable all used attributes
   for(unsigned i = 0; i < static_cast<unsigned>(AttributeName::AttributeName_LAST); ++i)
   {
      if(mAttributeLocations[i] != -1)
         glEnableVertexAttribArray(mAttributeLocations[i]);
   }
}

std::string Shader::getName() const
{
   return mName;
}

U32 Shader::getId() const
{
   return mId;
}

S32 Shader::getUniformLocation(UniformName uniformName) const
{
   return mUniformLocations[static_cast<unsigned>(uniformName)];
}

S32 Shader::getAttributeLocation(AttributeName attributeName) const
{
   return mAttributeLocations[static_cast<unsigned>(attributeName)];
}

void Shader::setMVP(const Matrix4 &MVP)
{
   glUniformMatrix4fv(getUniformLocation(UniformName::MVP), 1, GL_FALSE, MVP.getData());
}

void Shader::setColor(const Color &color, F32 alpha)
{
   if(color != mLastColor || alpha != mLastAlpha)
   {
      glUniform4f(getUniformLocation(UniformName::Color), color.r, color.g, color.b, alpha);
      mLastColor = color;
      mLastAlpha = alpha;
   }
}

void Shader::setPointSize(F32 size)
{
   if(size != mLastPointSize)
   {
      glUniform1f(getUniformLocation(UniformName::PointSize), size);
      mLastPointSize = size;
   }
}

void Shader::setTime(U32 time)
{
   if(time != mLastTime)
   {
      glUniform1i(getUniformLocation(UniformName::Time), time);
      mLastTime = time;
   }
}

void Shader::setIsAlphaTexture(bool isAlphaTexture)
{
   if(isAlphaTexture != mLastIsAlphaTexture)
   {
      glUniform1i(getUniformLocation(UniformName::IsAlphaTexture), isAlphaTexture ? 1 : 0);
      mLastIsAlphaTexture = isAlphaTexture;
   }
}

void Shader::setTextureSampler(U32 textureSampler)
{
   if(textureSampler != mLastTextureSampler)
   {
      glUniform1i(getUniformLocation(UniformName::TextureSampler), textureSampler);
      mLastTextureSampler = textureSampler;
   }
}

}

#endif // BF_USE_LEGACY_GL