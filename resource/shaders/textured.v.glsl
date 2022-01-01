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
