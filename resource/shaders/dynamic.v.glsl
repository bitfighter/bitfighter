//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// This shader gives a color per vertex

#version 120

attribute vec2 vertexPosition_modelspace; // Bitfighter gives 2D points!
attribute vec4 vertexColor;
uniform mat4 MVP;

varying vec3 fScreenPos;
varying vec4 fColor;

void main()
{
	vec4 v = vec4(vertexPosition_modelspace, 0, 1); // 1 since this is a point.
	gl_Position = vec4((MVP * v).xyz, 1); // Set the W to 1 just in-case
	
	fScreenPos = vec3(gl_Position.xyz); // In clip-space
	fColor = vertexColor; // Will be interpolated to have fancy gradients in the fragment shader
}