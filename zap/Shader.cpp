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
#include <limits>

std::string getGLShaderDebugLog(U32 object, PFNGLGETSHADERIVPROC glGet_iv, PFNGLGETSHADERINFOLOGPROC glGet_InfoLog)
{
	GLint logLength;
	std::string log;
	
	// Resize string first
	glGet_iv(static_cast<GLuint>(object), GL_INFO_LOG_LENGTH, &logLength);
	log.resize(logLength);

	if(logLength)
		glGet_InfoLog(object, logLength, NULL, &log[0]);

	log.pop_back(); // Remove null terminator (\0) that OpenGL added
	return "\n-----------GL LOG-----------\n" + log; // For looks
}

Shader::Shader(const std::string& name, const std::string& vertexShaderFile, const std::string& fragmentShaderFile)
	: mName(name), mId(0)
{
	std::string vertexShaderCode = getShaderSource(vertexShaderFile);
	std::string fragmentShaderCode = getShaderSource(fragmentShaderFile);

	U32 vertexShader = compileShader(vertexShaderFile, vertexShaderCode, GL_VERTEX_SHADER);
	U32 fragmentShader = compileShader(fragmentShaderFile, fragmentShaderCode, GL_FRAGMENT_SHADER);

	if(vertexShader != 0 && fragmentShader !=0)
		mId = linkShader(mName, static_cast<GLuint>(vertexShader), static_cast<GLuint>(fragmentShader));

	registerUniforms();
}

Shader::~Shader()
{
	glDeleteShader(static_cast<GLuint>(mId));
}

// Static
std::string Shader::getShaderSource(const std::string &fileName)
{
	std::string filename = GameSettings::getFolderManager()->findShaderFile(fileName);
	TNLAssert(filename != "", "Expected a shader filename here!");

	return filename;
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
		glDeleteProgram(shader);

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
	GLint numberOfUniforms;
	const GLsizei bufferSize = 256;
	GLchar uniformNameBuffer[bufferSize];
	glGetProgramiv(static_cast<GLuint>(mId), GL_ACTIVE_UNIFORMS, &numberOfUniforms);

	GLsizei numberOfCharsReceived;
	GLint size;
	GLuint type;
	for(int i=0; i<numberOfUniforms; i++)
	{
		glGetActiveUniform(static_cast<GLuint>(mId), i, bufferSize, &numberOfCharsReceived, &size, &type, uniformNameBuffer);
		registerUniform(uniformNameBuffer); // uniformNameBuffer is null-terminated
	}
}

S32 Shader::registerUniform(const std::string& uniformName)
{
	// Get the location ("index") of the uniform in the shader
	GLint uniformLocation = glGetUniformLocation(static_cast<GLuint>(mId), uniformName.c_str());
	TNLAssert(uniformLocation != -1, (
		"Uniform '" + uniformName + "' does not exist or is invalid in shader '" + mName + 
		"'. We should never have this issue; check the code!").c_str()
	);

	// Add the uniform to our map
	UniformPair uniformPair(uniformName, static_cast<S32>(uniformLocation));
	std::pair<UniformMap::iterator, bool> newlyAddedPair = mUniformMap.insert(uniformPair);
	TNLAssert(newlyAddedPair.second,
		("Uniform '" + uniformName + "' in shader '" + mName + "' already exists and cannot be added again!").c_str());

	return static_cast<S32>(uniformLocation);
}

std::string Shader::getName() const
{
	return mName;
}

U32 Shader::getId() const
{
	return mId;
}

// Returns -1 if not found
S32 Shader::findUniform(const std::string& uniformName) const
{
	UniformMap::const_iterator found = mUniformMap.find(uniformName);
	if(found == mUniformMap.end())
		return -1;

	return found->second;
}

#endif // BF_USE_LEGACY_GL