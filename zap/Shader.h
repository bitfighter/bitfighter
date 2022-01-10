//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SHADER_H_
#define _SHADER_H_

#include "Matrix4.h"
#include "Color.h"
#include "tnlTypes.h"
#include <string>

using namespace TNL;

namespace Zap
{

enum class UniformName
{
   MVP = 0,
   Color,
   PointSize,
   TextureSampler,
   IsAlphaTexture,
   Time,
   UniformName_LAST // Keep this at the end
};

enum class AttributeName
{
   VertexPosition = 0,
   VertexColor,
   VertexUV,
   AttributeName_LAST // Keep this at the end
};

class Shader
{
private:
   std::string mName;
   U32 mId;

   S32 mUniformLocations[static_cast<unsigned>(UniformName::UniformName_LAST)];
   S32 mAttributeLocations[static_cast<unsigned>(AttributeName::AttributeName_LAST)];

   Color mLastColor;
   F32 mLastAlpha;
   F32 mLastPointSize;
   U32 mLastTime;
   bool mLastIsAlphaTexture;
   U32 mLastTextureSampler;

   static std::string getShaderSource(const std::string &fileName);
   static U32 compileShader(const std::string& shaderPath, const std::string& shaderCode, U32 type);
   static U32 linkShader(const std::string& shaderProgramName, U32 vertexShader, U32 fragmentShader);

   void buildProgram(const std::string &vertexShaderFile, const std::string &fragmentShaderFile);
   void registerUniforms();
   void registerAttributes();

public:
   Shader(const std::string &name, const std::string &vertexShaderFile, const std::string &fragmentShaderFile);
   ~Shader();

   std::string getName() const;
   U32 getId() const;

   S32 getUniformLocation(UniformName uniformName) const;
   S32 getAttributeLocation(AttributeName attributeName) const;

   // Defined for all shaders, even if unused.
   // Shader must be active when called!
   void setMVP(const Matrix4 &MVP);
   void setColor(const Color &color, F32 alpha);
   void setPointSize(F32 size);
   void setTime(U32 time);
   void setIsAlphaTexture(bool isAlphaTexture);
   void setTextureSampler(U32 textureSampler);
};

}

#endif // _SHADER_H_
