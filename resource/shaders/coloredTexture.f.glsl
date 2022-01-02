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

void main()
{
	// Remove any color the texture had, if any. Only keep alpha channel.
	vec4 textureValue = vec4(0.0, 0.0, 0.0, texture2D(textureSampler, UV).a);

	// * color.a to support semi-transparent text
	gl_FragColor = (textureValue + vec4(color.rgb, 0)) * vec4(1.0, 1.0, 1.0, color.a);
}
