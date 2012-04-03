//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

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
