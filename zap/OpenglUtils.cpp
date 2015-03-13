//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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


void glScale(const Point &scaleFactor)
{
    glScalef(scaleFactor.x, scaleFactor.y, 1);
}


void glScale(F32 scaleFactor)
{
    glScalef(scaleFactor, scaleFactor, 1);
}


void glScale(F32 xScaleFactor, F32 yScaleFactor)
{
    glScalef(xScaleFactor, yScaleFactor, 1);
}


void glTranslate(const Point &pos)
{
   glTranslate(pos.x, pos.y);
}


void glTranslate(F32 x, F32 y)
{
   glTranslatef(x, y, 0);
}




void glRotate(F32 angle)
{
   glRotatef(angle, 0, 0, 1.0f);
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


// geomType: GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP, GL_TRIANGLES, GL_TRIANGLE_FAN, etc.
void renderPointVector(const Vector<Point> *points, U32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, points->address());
   glDrawArrays(geomType, 0, points->size());

   glDisableClientState(GL_VERTEX_ARRAY);
}


void renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType)
{
   glPushMatrix();
      glTranslate(offset);
      renderPointVector(points, geomType);
   glPopMatrix();
}


void renderLine(const Vector<Point> *points)
{
   renderPointVector(points, GL_LINE_STRIP);
}

}
