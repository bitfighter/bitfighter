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

// Interpolated values from the vertex shader
varying vec2 UV;

// Values that stay constant for the whole mesh
uniform sampler2D textureSampler; // In this case, black with alpha channel
uniform vec4 color;

void main()
{
	// Remove any color the texture had, if any. Only keep alpha channel.
	vec4 textureValue = vec4(0.0, 0.0, 0.0, texture2D(textureSampler, UV).a);

	// * color.a to support semi-transparent text
	gl_FragColor = (textureValue + vec4(color.rgb, 0)) * vec4(1.0, 1.0, 1.0, color.a);
}
