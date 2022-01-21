//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// This file is heavily based off http://www.opengl-tutorial.org/

#version 120

// Interpolated values from the vertex shader
varying vec2 UV;

// Values that stay constant for the whole mesh
uniform sampler2D textureSampler;
uniform vec4 color;
uniform int isAlphaTexture;

void main()
{
	// A note on conditionals in shaders: https://stackoverflow.com/a/37837060
	gl_FragColor = (isAlphaTexture == 1) ?
			vec4(color.r, color.g, color.b, texture2D(textureSampler, UV).a * color.a)
		:
			texture2D(textureSampler, UV) * color;
}
