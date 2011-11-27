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
#include "tnlVector.h"
#include "Timer.h"
#include "InputCode.h"
#include <string>

using namespace std;

namespace Zap
{

//
// Class to manage all sorts of single-line editing tasks
//
class LineEditor
{
private:
   string mLine;
   string mPrompt;
   bool mMasked;

   static Timer mBlinkTimer;

   // For tab expansion 
   Vector<string> mMatchList;
   S32 mMatchIndex;
   void buildMatchList(const Vector<string> *candidates, const string &partial);

public:
   U32 mMaxLen;

   enum LineEditorFilter {
      allAsciiFilter,      // any ascii character
      digitsOnlyFilter,    // 0-9
      numericFilter,       // 0-9, -, .
      fileNameFilter,      // A-Z, a-z, 0-9, _
      noQuoteFilter        // No "s
   };

   LineEditor(U32 maxLength = 256, string value = "");   // Constructor

   U32 length() { return (U32)mLine.length(); }
   void addChar(char c);
   void backspacePressed() { if(length() > 0) mLine.erase(mLine.size() - 1); mMatchIndex = -1; }    // User hit Backspace 
   void deletePressed() { backspacePressed(); }                                                     // User hit Delete 
   void handleBackspace(InputCode inputCode);           // Pass KEY_BACKSPACE or KEY_DELETE and it will do the right thing!
   void clear() { mLine.clear(); mMatchIndex = -1;}     // Clear the string and tab-expansion matchlist
   char at(U32 pos);                                    // Get char at pos
   bool isEmpty() { return mLine.empty(); }             // Is string empty

   void setSecret(bool secret) { mMasked = secret; }

   LineEditorFilter mFilter;
   void setFilter(LineEditorFilter filter) { mFilter = filter; }

   string getString() const { return mLine; }                              // Return the string in string format
   const string *getStringPtr() const { return &mLine; }
   string getDisplayString() const { return mMasked ? string(mLine.length(), '*') : mLine; }

   void setString(const string &str) { mLine.assign(str.substr(0, mMaxLen)); }    // Set the string
   void setPrompt(const string &prompt) { mPrompt = prompt; }
   string getPrompt() { return mPrompt; }
   const char *c_str() { return mLine.c_str(); }                     // Return the string in c_str format

   void drawCursor(S32 x, S32 y, S32 fontSize);                             // Draw our cursor, assuming string is drawn at x,y
   void drawCursorAngle(F32 x, F32 y, F32 fontSize, F32 angle);             // Draw our cursor, assuming string is drawn at x,y at specified angle
   static void drawCursor(S32 x, S32 y, S32 fontSize, S32 startingWidth);   // Draw cursor starting at a given width
   static void drawCursorAngle(S32 x, S32 y, F32 fontSize, S32 startingWidth, F32 angle);

   // For tab expansion 
   void completePartial(const Vector<string> *candidates, const string &partial, size_t replacePos, const string &appender);

   S32 getMaxLen() { return mMaxLen; }

   // LineEditors are equal if their values are equal
   bool operator==(LineEditor &lineEditor) const { return mLine == lineEditor.getString(); }

};


};

#endif
