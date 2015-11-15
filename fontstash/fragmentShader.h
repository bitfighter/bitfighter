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

#ifndef __FONTSTASH_FRAGMENTSHADER_H
#define __FONTSTASH_FRAGMENTSHADER_H

const char fragmentShader[] = "#version 330 core\n\
\n\
// Interpolated values from the vertex shaders\n\
in vec2 UV;\n\
\n\
// Ouput data\n\
out vec4 color;\n\
\n\
// Values that stay constant for the whole mesh.\n\
uniform sampler2D myTextureSampler;\n\
\n\
void main() {\n\
    // Output color = color of the texture at the specified UV\n\
    color = texture(myTextureSampler, UV).rrrr;\n\
}";

#endif /* __FONTSTASH_FRAGMENTSHADER_H */
