//// Copyright 2016 Carl Hewett
////
//// This file is part of SDL3D.
////
//// SDL3D is free software: you can redistribute it and/or modify
//// it under the terms of the GNU General Public License as published by
//// the Free Software Foundation, either version 3 of the License, or
//// (at your option) any later version.
////
//// SDL3D is distributed in the hope that it will be useful,
//// but WITHOUT ANY WARRANTY; without even the implied warranty of
//// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// GNU General Public License for more details.
////
//// You should have received a copy of the GNU General Public License
//// along with SDL3D. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifndef SHADER_HPP
#define SHADER_HPP

#include "tnlTypes.h"

#include <vector>
#include <string>

using namespace TNL;

enum class UniformName
{
	MVP = 0,
	Color,
	TextureSampler,
	IsAlphaTexture,
	Time,
	UniformName_LAST // Keep this at the end
};

class Shader
{
private:
	std::string mName;
	U32 mId;
	std::vector<U32> mShaders;
	S32 mUniformLocations[static_cast<unsigned>(UniformName::UniformName_LAST)];

	static std::string getShaderSource(const std::string &fileName);
	static U32 compileShader(const std::string& shaderPath, const std::string& shaderCode, U32 type);
	static U32 linkShader(const std::string& shaderProgramName, U32 vertexShader, U32 fragmentShader);
	void registerUniforms();

public:
	Shader(const std::string &name, const std::string &vertexShaderFile, const std::string &fragmentShaderFile);
	~Shader();

	std::string getName() const;
	U32 getId() const;
	S32 getUniform(UniformName uniformName) const;
};

#endif /* SHADER_HPP */
