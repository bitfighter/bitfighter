//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// This file is heavily based off http://www.opengl-tutorial.org/, see SpecialThanks.txt

#version 120

// Interpolated values from the vertex shader
varying vec2 UV;

// Values that stay constant for the whole mesh
uniform sampler2D textureSampler;

void main()
{
	gl_FragColor = texture2D(textureSampler, UV);
}
