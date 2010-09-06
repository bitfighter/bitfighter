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
#include "keyCode.h"
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
   string mPrompt;

   U32 mMaxLen;
   static Timer mBlinkTimer;

public:
   static void updateCursorBlink(U32 timeDelta);
   static bool cursorBlink;


   enum LineEditorFilter {
      allAsciiFilter,      // any ascii character
      digitsOnlyFilter,    // 0-9
      numericFilter,       // 0-9, -, .
      fileNameFilter       // A-Z, a-z, 0-9, _
   };

   LineEditor(U32 maxLength = 256, string value = "");     // Constructor

   U32 length() { return (U32)mLine.length(); }
   void addChar(char c);
   void backspacePressed() { if(length() > 0) mLine.erase(mLine.size() - 1); }     // Backspace key hit
   void deletePressed() { backspacePressed(); }                                    // Delete key hit
   void handleBackspace(KeyCode keyCode);               // Pass this either KEY_BACKSPACE or KEY_DELETE and it will do the right thing!
   void clear() { mLine.clear(); }                      // Clear the string
   char at(U32 pos);                                    // Get char at pos
   bool isEmpty() { return mLine.empty(); }             // Is string empty

   LineEditorFilter mFilter;
   void setFilter(LineEditorFilter filter) { mFilter = filter; }

   string getString() const { return mLine; }                        // Return the string in string format
   void setString(string str) { mLine = str.substr(0, mMaxLen); }    // Set the string
   void setPrompt(string prompt) { mPrompt = prompt; }
   string getPrompt() { return mPrompt; }
   const char *c_str() { return mLine.c_str(); }                     // Return the string in c_str format

   void drawCursor(S32 x, S32 y, U32 fontSize);                  // Draw our cursor, assuming string is drawn at x,y 
   void drawCursorAngle(S32 x, S32 y, F32 fontSize, F32 angle);  // Draw our cursor, assuming string is drawn at x,y at specified angle
   void drawCursorAngle(F32 x, F32 y, F32 fontSize, F32 angle);

   S32 getMaxLen() { return mMaxLen; }

   // LineEditors are equal if their values are equal
   bool operator==(LineEditor &lineEditor) const { return mLine == lineEditor.getString(); }

};

};

#endif
