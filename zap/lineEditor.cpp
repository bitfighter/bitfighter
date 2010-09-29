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
   mFilter = allAsciiFilter;
   mPrompt = "";
   mSecret = false;
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


// Draw our cursor, assuming string is drawn at x,y  (vert spacing works differently than on the angle version
void LineEditor::drawCursor(S32 x, S32 y, U32 fontSize)
{
   if(cursorBlink)
   {
      S32 w = UserInterface::getStringWidth((F32)fontSize, mLine.c_str());

      UserInterface::drawString(x + w + S32(F32(mLine.length())/5.1), y, fontSize, "_");  // Mostly right, but still a tiny bit off
   }
}


// Draw our cursor, assuming string is drawn at x,y at specified angle 
void LineEditor::drawCursorAngle(F32 x, F32 y, F32 fontSize, F32 angle)
{
   drawCursorAngle(S32(x + 0.5), S32(y + 0.5), fontSize, angle);
}


void LineEditor::drawCursorAngle(S32 x, S32 y, F32 fontSize, F32 angle)
{
   if(cursorBlink)
   {
      S32 w = UserInterface::getStringWidth(fontSize, mLine.c_str());
      F32 cosang = cos(angle); 
      F32 sinang = sin(angle);

      F32 xpos = x + (w * cosang ); 
      F32 ypos = y + (w * sinang ); 

      UserInterface::drawAngleString_fixed(xpos, ypos, (F32)fontSize, angle, "_");
   }
}


void LineEditor::addChar(const char c) 
{ 
   if(c == 0)
      return;

   if((mFilter == digitsOnlyFilter) && (c < '0' || c > '9'))
      return;
      
   if((mFilter == numericFilter) && (c != '-' && c != '.' && (c < '0' || c > '9')))
      return;

   if((mFilter == fileNameFilter) && ! ( (c >= '0' && c <= '9') ||
                                         (c == '_')             ||
                                         (c >= 'A' && c <= 'Z') ||
                                         (c >= 'a' && c <= 'z') )  )
      return;
   
   if(length() < mMaxLen) mLine.append(string(1,c)); 
}


// keyCode will have either backspace or delete in it -- basically a convenience function
void LineEditor::handleBackspace(KeyCode keyCode)
{
   if(keyCode == KEY_BACKSPACE)
      backspacePressed();
   else       // KEY_DELETE
      deletePressed();
}


// Needed for now, may be deleteable later.  See http://forums.devx.com/archive/index.php/t-97293.html
Timer LineEditor::mBlinkTimer(100);       // <-- 100 ms is blink rate
bool  LineEditor::cursorBlink = false;

};

