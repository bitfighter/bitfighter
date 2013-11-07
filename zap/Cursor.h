//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef CURSORS_H
#define CURSORS_H

#include "tnlTypes.h"
#include "Point.h"


struct SDL_Cursor;

namespace Zap 
{

struct Cursor 
{
private:
   void reverseBits();           // Reverse order of bits in an array of bytes
   SDL_Cursor *toSDL();          // Convert to SDL format

public:     
   // These fields have to be public to initialize them inline in Cursors.cpp
   U32 width, height;
   U32 hotX, hotY;
   U8 bits[128];   
   U8 maskBits[128];

   static void init();                    // Initialize all cursors
   static SDL_Cursor *getSpray();
   static SDL_Cursor *getVerticalResize();
   static SDL_Cursor *getDefault();
   static SDL_Cursor *getTransparent();

   static void enableCursor();
   static void disableCursor();
};

}

#endif
