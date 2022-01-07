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
#include <unordered_map>
#include <string>

using namespace TNL;

class Shader
{
private:
	// <uniform, uniformLocation>
	using UniformMap = std::unordered_map<std::string, S32>;
	using UniformPair = std::pair<std::string, S32>;

	std::string mName;
	U32 mId;
	std::vector<U32> mShaders;
	UniformMap mUniformMap;

	static std::string getShaderSource(const std::string &fileName);
	static U32 compileShader(const std::string& shaderPath, const std::string& shaderCode, U32 type);
	static U32 linkShader(const std::string& shaderProgramName, U32 vertexShader, U32 fragmentShader);

	void registerUniforms();
	S32 registerUniform(const std::string& uniformName);

public:
	Shader(const std::string &name, const std::string &vertexShaderFile, const std::string &fragmentShaderFile);
	~Shader();

	std::string getName() const;
	U32 getId() const;
	S32 findUniform(const std::string& uniformName) const;
};

#endif /* SHADER_HPP */
