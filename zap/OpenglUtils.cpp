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

#include "freeglut_stroke.h"
#include "FontStrokeRoman.h"
#include "FontOrbitronLight.h"
#include "FontOrbitronMedium.h"

extern "C" {
   #include "../fontstash/fontstash.h"
}
#include "../fontstash/stb_truetype.h"

#include "Color.h"
#include "Point.h"
#include "tnlVector.h"
#include "stringUtils.h"

#include <stdio.h>

namespace Zap {

const SFG_StrokeFont *gCurrentFont = &fgStrokeRoman;  // fgStrokeOrbitronLight, fgStrokeOrbitronMed


// TODO: set up some sort of struct to hold stashes and ids for multiple fonts
static S32 stashFontId = 0;
struct sth_stash* stash = NULL;

extern string getInstalledDataDir();

void OpenglUtils::initFont()
{
   // We're switching screen modes, clean-up old stash before creating a new one
   if(stash)
      sth_delete(stash);

   stash = sth_create(512, 512);

   if (!stash)
   {
      printf("Could not create stash.\n");
      return;
   }

   // TODO create a API to access the font dir
   // TODO maybe load fonts from INI?
   string fontFile = getInstalledDataDir() + getFileSeparator() + "fonts" + getFileSeparator() +
         //"Orbitron Light.ttf";
         //"OCRA.ttf";
         "prime_regular.ttf";

   stashFontId = sth_add_font(stash, fontFile.c_str());

   if (stashFontId != 0)
      printf("Loaded font from stash %d: %s\n", stashFontId, fontFile.c_str());
   else
      printf("Can't load font! %s\n", fontFile.c_str());
}


void OpenglUtils::shutdownFont()
{
   if(stash)
      sth_delete(stash);
}


void OpenglUtils::drawTTFString(const char *string, F32 size)
{
   // Needed??
   F32 outPos;

   sth_begin_draw(stash);

   // 152.381f is what I stole from the bottom of FontStrokeRoman.h as the font size
   sth_draw_text(stash, stashFontId, size, 0.0, 0.0, string, &outPos);

   sth_end_draw(stash);
}


void OpenglUtils::drawStrokeCharacter(S32 character)
{
   /*
    * Draw a stroke character
    */
   const SFG_StrokeChar *schar;
   const SFG_StrokeStrip *strip;
   S32 i, j;

   if(!(character >= 0))
      return;
   if(!(character < gCurrentFont->Quantity))
      return;
   if(!gCurrentFont)
      return;

   schar = gCurrentFont->Characters[ character ];

   if(!schar)
      return;

   strip = schar->Strips;

   for(i = 0; i < schar->Number; i++, strip++)
   {
      // I didn't find any strip->Number larger than 34, so I chose a buffer of 64
      // This may change if we go crazy with i18n some day...
      static F32 characterVertexArray[128];
      for(j = 0; j < strip->Number; j++)
      {
        characterVertexArray[2*j]     = strip->Vertices[j].X;
        characterVertexArray[(2*j)+1] = strip->Vertices[j].Y;
      }
      renderVertexArray(characterVertexArray, strip->Number, GL_LINE_STRIP);
   }
   glTranslatef( schar->Right, 0.0, 0.0 );
}


// TODO: what to do here for TTF fonts?
int OpenglUtils::getStringLength(const unsigned char* string)
{
   U8 c;
   F32 length = 0.0;
   F32 this_line_length = 0.0;

   if(!gCurrentFont)
      return 0;
   if ( !string || ! *string )
      return 0;

   while( ( c = *string++) )
      if( c < gCurrentFont->Quantity )
      {
         if( c == '\n' ) /* EOL; reset the length of this line */
         {
            if( length < this_line_length )
               length = this_line_length;
            this_line_length = 0.0;
         }
         else  /* Not an EOL, increment the length of this line */
         {
            const SFG_StrokeChar *schar = gCurrentFont->Characters[ c ];
            if( schar )
               this_line_length += schar->Right;
         }
      }
   if( length < this_line_length )
      length = this_line_length;
   return( S32 )( length + 0.5 );
}


////////////////////////////////////////
////////////////////////////////////////


void setFont(OpenglUtils::FontId fontId)
{
   if(fontId == OpenglUtils::FontRoman)
      gCurrentFont = &fgStrokeRoman;
   else if(fontId == OpenglUtils::FontOrbitronLight)
      gCurrentFont = &fgStrokeOrbitronLight;
   else if(fontId == OpenglUtils::FontOrbitronMed)
      gCurrentFont = &fgStrokeOrbitronMed;
   else
      TNLAssert(false, "Unknown FontId!");
}


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
