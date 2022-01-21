//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#version 120

varying vec3 fScreenPos; // Could be useful?
varying vec4 fColor; // Has to be vec4

void main()
{
	gl_FragColor = fColor;
}