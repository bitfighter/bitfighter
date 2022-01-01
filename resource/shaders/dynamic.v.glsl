//// Copyright 2015 Carl Hewett
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