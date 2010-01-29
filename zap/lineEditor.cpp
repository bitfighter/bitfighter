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

#include "lineEditor.h"
#include "UI.h"

namespace Zap
{

class UserInterface;

// Constructor
LineEditor::LineEditor(U32 maxLength, string value)
{
   mMaxLen = maxLength;
   mLine = value;
}


// Update our global cursor blinkenlicht
// To implement a flashing cursor, call this routine from a UI's local idle routine,
// then draw the cursor when cursorBlink is true
void LineEditor::updateCursorBlink(U32 timeDelta)
{
   if(mBlinkTimer.update(timeDelta))
   {
      mBlinkTimer.reset();
      cursorBlink = !cursorBlink;
   }
}


char LineEditor::at(U32 pos)
{ 
   if(pos >= mLine.length())
      return 0;

   return mLine.at(pos); 
}


// Draw our cursor, assuming string is drawn at x,y
void LineEditor::drawCursor(S32 x, S32 y, S32 fontSize)     
{
   if(cursorBlink)
   {
      S32 xpos = UserInterface::getStringWidth(fontSize, mLine.c_str());
      UserInterface::drawString(x + xpos, y, fontSize, "_");
   }
}


// Needed for now, may be deleteable later.  See http://forums.devx.com/archive/index.php/t-97293.html
Timer LineEditor::mBlinkTimer(100);
bool  LineEditor::cursorBlink = false;


};