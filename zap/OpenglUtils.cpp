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

#include "SDL/SDL_opengl.h"
#include "freeglut_stroke.h"
#include "freeglut_stroke_roman.h"

using namespace TNL;

namespace Zap {

void OpenglUtils::drawCharacter(S32 character)
{
   /*
    * Draw a stroke character
    */
   const SFG_StrokeChar *schar;
   const SFG_StrokeStrip *strip;
   S32 i, j;
   const SFG_StrokeFont* font;

   // This is GLUT_STROKE_ROMAN
   font = &fgStrokeRoman;

   if(!(character >= 0))
      return;
   if(!(character < font->Quantity))
      return;
   if(!font)
      return;

   schar = font->Characters[ character ];

   if(!schar)
      return;

   strip = schar->Strips;

   for( i = 0; i < schar->Number; i++, strip++ )
   {
      glBegin( GL_LINE_STRIP );
      for( j = 0; j < strip->Number; j++ )
         glVertex2f( strip->Vertices[ j ].X, strip->Vertices[ j ].Y );
      glEnd( );
      //glBegin( GL_POINTS );  // points only cause problems with smoothing and semi-transparent
      //for( j = 0; j < strip->Number; j++ )
      //   glVertex2f( strip->Vertices[ j ].X, strip->Vertices[ j ].Y );
      //glEnd( );
   }
   glTranslatef( schar->Right, 0.0, 0.0 );
}

int OpenglUtils::getStringLength(const unsigned char* string )
{
   U8 c;
   F32 length = 0.0;
   F32 this_line_length = 0.0;
   const SFG_StrokeFont* font;

   font = &fgStrokeRoman;
   if(!font)
      return 0;
   if ( !string || ! *string )
      return 0;

   while( ( c = *string++) )
      if( c < font->Quantity )
      {
         if( c == '\n' ) /* EOL; reset the length of this line */
         {
            if( length < this_line_length )
               length = this_line_length;
            this_line_length = 0.0;
         }
         else  /* Not an EOL, increment the length of this line */
         {
            const SFG_StrokeChar *schar = font->Characters[ c ];
            if( schar )
               this_line_length += schar->Right;
         }
      }
   if( length < this_line_length )
      length = this_line_length;
   return( S32 )( length + 0.5 );
}

}
