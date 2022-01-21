//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// This shader uses one color for all vertices

#version 120

attribute vec2 vertexPosition_modelspace; // Bitfighter gives 2D points!
uniform mat4 MVP;
uniform float pointSize;

void main()
{
	vec4 v = vec4(vertexPosition_modelspace, 0, 1); // 1 since this is a point.
	gl_Position = vec4((MVP * v).xyz, 1); // Set the W to 1 just in-case
	gl_PointSize = pointSize;
}
