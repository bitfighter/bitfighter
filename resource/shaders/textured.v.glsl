//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// This file is heavily based off http://www.opengl-tutorial.org/, see SpecialThanks.txt

#version 120

// Input vertex data, different for all executions
attribute vec2 vertexPosition_modelspace; // Bitfighter gives 2D points!
attribute vec2 vertexUV;

// Values that stay constant for the whole mesh
uniform mat4 MVP;

// Output data
varying vec2 UV; // Proxy, sends UV coord to fragment shader

void main()
{
	// Output position of the vertex
	vec4 v = vec4(vertexPosition_modelspace, 0, 1); // 1 since this is a point.
	gl_Position = vec4((MVP * v).xyz, 1); // Set the W to 1 just in-case
	
	// UV of the vertex
	UV = vertexUV;
}
