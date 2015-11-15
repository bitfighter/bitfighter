//
// Copyright (c) 2014 Sergio Moura sergio@moura.us
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef __FONTSTASH_VERTEXSHADER_H
#define __FONTSTASH_VERTEXSHADER_H

const char vertexShader[] = "#version 330 core\n\
\n\
// Input vertex data, different for all executions of this shader.\n\
layout(location = 0) in vec3 vertexPosition_modelspace;\n\
layout(location = 1) in vec2 vertexUV;\n\
\n\
// Output data ; will be interpolated for each fragment.\n\
out vec2 UV;\n\
\n\
// Values that stay constant for the whole mesh.\n\
uniform mat4 MVP;\n\
\n\
void main() {\n\
    // Output position of the vertex, in clip space : MVP * position\n\
    gl_Position =  MVP * vec4(vertexPosition_modelspace,1);\n\
    \n\
    // UV of the vertex. No special space for this one.\n\
    UV = vertexUV;\n\
}";

#endif /* __FONTSTASH_VERTEXSHADER_H */
