/*
 * OpenglUtils.cpp
 *
 *  Created on: Jun 8, 2011
 *
 *  Most of this was taken directly from freeglut sources
 *
 *  freeglut license:
 *
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Thu Dec 16 1999
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "OpenglUtils.h"

#include "Color.h"
#include "Point.h"
#include "tnlVector.h"
#include "stringUtils.h"

#include "FontManager.h"


#include <stdio.h>

namespace Zap {


void glColor(const Color &c, float alpha)
{
    glColor4f(c.r, c.g, c.b, alpha);
}

void glColor(const Color *c, float alpha)
{
    glColor4f(c->r, c->g, c->b, alpha);
}

void glColor(F32 c, float alpha)
{
   glColor4f(c, c, c, alpha);
}

void glScale(F32 scaleFactor)
{
    glScalef(scaleFactor, scaleFactor, 1);
}


void glTranslate(const Point &pos)
{
   glTranslatef(pos.x, pos.y, 0);
}


void setDefaultBlendFunction()
{
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void renderVertexArray(const S8 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_BYTE, 0, verts);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}


void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_SHORT, 0, verts);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}


void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, verts);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}


void renderColorVertexArray(const F32 vertices[], const F32 colors[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, vertices);
   glColorPointer(4, GL_FLOAT, 0, colors);
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
}


void renderPointVector(const Vector<Point> *points, U32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, sizeof(Point), points->address());
   glDrawArrays(geomType, 0, points->size());

   glDisableClientState(GL_VERTEX_ARRAY);
}

// Use slower method here because we need to visit each point to add offset
void renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType)
{
   // I chose 32768 as a buffer because that was enough for the editor to handle at least
   // 10 ctf3-sized levels with points in the editor.  I added the assert just in case,
   // it may need to be increased for some crazy levels out there
   static F32 pointVectorVertexArray[32768];
   TNLAssert(points->size() <= 32768, "static array for this render function is too small");

   for(S32 i = 0; i < points->size(); i++)
   {
      pointVectorVertexArray[2*i]     = points->get(i).x + offset.x;
      pointVectorVertexArray[(2*i)+1] = points->get(i).y + offset.y;
   }

   glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(2, GL_FLOAT, 0, pointVectorVertexArray);
      glDrawArrays(geomType, 0, points->size());
   glDisableClientState(GL_VERTEX_ARRAY);
}


void renderLine(const Vector<Point> *points)
{
   renderPointVector(points, GL_LINE_STRIP);
}

}
