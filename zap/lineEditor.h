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

#ifndef _LINE_EDITOR_H_
#define _LINE_EDITOR_H_

#include "tnlTypes.h"
#include "timer.h"
#include <string>

using namespace std;

namespace Zap
{

enum KeyCode;

//
// Class to manage all sorts of single-line editing tasks
//
class LineEditor
{
private:
   string mLine;
   U32 mMaxLen;
   static Timer mBlinkTimer;

public:
   static void updateCursorBlink(U32 timeDelta);
   static bool cursorBlink;

   LineEditor(U32 maxLength = 256, string value = "");     // Constructor

   U32 length() { return mLine.length(); }
   void addChar(char c) { if(length() < mMaxLen) mLine.push_back(c); }
   void backspacePressed() { if(length() > 0) mLine.erase(mLine.size() - 1); }     // Backspace key hit
   void deletePressed() { backspacePressed(); }                                    // Delete key hit
   void handleBackspace(KeyCode keyCode);               // Pass this either KEY_BACKSPACE or KEY_DELETE and it will do the right thing!
   void clear() { mLine.clear(); }                      // Clear the string
   char at(U32 pos);                                    // Get char at pos
   bool isEmpty() { return mLine.empty(); }             // Is string empty

   string getString() { return mLine; }                 // Return the string in string format
   void setString(const char *str) { mLine = str; }     // Set the string
   void setString(string str)      { mLine = str; }
   const char *c_str() { return mLine.c_str(); }        // Return the string in c_str format

   void drawCursor(S32 x, S32 y, S32 fontSize, F32 angle = 0);  // Draw our cursor, assuming string is drawn at x,y at specified angle

   // LineEditors are equal if their values are equal
   bool operator==(LineEditor &lineEditor) const { return mLine == lineEditor.getString(); }
};


};

#endif
