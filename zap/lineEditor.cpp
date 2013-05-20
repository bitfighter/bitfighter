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

#include "MathUtils.h"
#include "RenderUtils.h"

#include <math.h>

namespace Zap
{

// Constructor
LineEditor::LineEditor(U32 maxLength, string value, U32 displayedCharacters)
{
   mMaxLen     = maxLength;
   mLine       = value;
   mFilter     = allAsciiFilter;
   mPrompt     = "";
   mMasked     = false;
   mMatchIndex = -1;
   mCursorOffset = mLine.length();
   mDisplayedCharacters = displayedCharacters;
}

LineEditor::~LineEditor()
{
   // Do nothing
}

U32 LineEditor::length()
{
   return (U32) mLine.length();
}


void LineEditor::backspacePressed()
{
   if(mCursorOffset == 0)
   {
      return;
   }

   mLine = mLine.substr(0, mCursorOffset - 1) + mLine.substr(mCursorOffset, mLine.length() - mCursorOffset);
   mCursorOffset -= 1;
   mMatchIndex = -1;
}


void LineEditor::deletePressed()
{
   if(mCursorOffset >= mLine.length())
   {
      return;
   }

   mLine = mLine.substr(0, mCursorOffset) + mLine.substr(mCursorOffset + 1, mLine.length() - mCursorOffset);
   mMatchIndex = -1;
}


void LineEditor::clear()
{
   mLine.clear();
   mCursorOffset = 0;
   mMatchIndex = -1;
}


bool LineEditor::isEmpty() const
{
   return mLine.empty();
}


void LineEditor::setSecret(bool secret)
{
   mMasked = secret;
}


void LineEditor::setFilter(LineEditorFilter filter)
{
   mFilter = filter;
}


string LineEditor::getString() const
{
   return mLine;
}


const string *LineEditor::getStringPtr() const
{
   return &mLine;
}


string LineEditor::getDisplayString() const
{
   U32 offsetCharacters = mCursorOffset / mDisplayedCharacters * mDisplayedCharacters;
   return mMasked ? string(mLine.length() - offsetCharacters, MASK_CHAR) : mLine.substr(offsetCharacters, MIN(mDisplayedCharacters, mLine.length() - offsetCharacters));
}

string LineEditor::getStringBeforeCursor() const
{
   return mMasked ? string(mCursorOffset, MASK_CHAR) : mLine.substr(0, mCursorOffset);
}

S32 LineEditor::getCursorOffset() const
{
   return mCursorOffset;
}


void LineEditor::setString(const string &str)
{
   mLine.assign(str.substr(0, mMaxLen));
   mCursorOffset = mLine.length();
}


void LineEditor::setPrompt(const string &prompt)
{
   mPrompt = prompt;
}


string LineEditor::getPrompt() const
{
   return mPrompt;
}


const char *LineEditor::c_str() const
{
   return mLine.c_str();
}


char LineEditor::at(U32 pos) const
{
   if(pos >= mLine.length())
      return 0;

   return mLine.at(pos);
} 


// Given a list of potential match candidates, and a partially typed string, find all candidates that are potential matches
void LineEditor::buildMatchList(const Vector<string> *candidates, const string &partial)
{
   // Search for matching candidates
   if(mMatchIndex == -1)    // -1 --> Need to build a new match list (gets set to -1 when we change mLineEditor by typing)
   {
      mMatchList.clear();

      string::size_type len = partial.size();

      for(S32 i = 0; i < candidates->size(); i++)
      {
         // If partial is empty, then everything matches -- we want all candidates in our list
         if(partial == "" || stricmp(partial.c_str(), (*candidates)[i].substr(0, len).c_str()) == 0)
            mMatchList.push_back((*candidates)[i]);
      }
   }
}


// Find best match from list of candidates given a partially typed entry partial
// Note that candidates could be NULL; size_t is equivalent to a U32 in VC++
void LineEditor::completePartial(const Vector<string> *candidates, const string &partial, std::size_t replacePos, const string &appender)
{
   // Now we have our candidates list... let's compare to what the player has already typed to generate completion string
   if(candidates && candidates->size() > 0)
   {
      buildMatchList(candidates, partial);   // Filter candidates by what we've typed so far

      if(mMatchList.size() == 0)             // Found no matches... no expansion possible
         return;

      mMatchIndex++;                         // Advance to next potential match

      if(mMatchIndex >= mMatchList.size())   // Handle wrap-around
         mMatchIndex = 0;

      // If match contains a space, wrap it in quotes
      string matchedString = mMatchList[mMatchIndex];
      if(matchedString.find_first_of(" ") != string::npos)
         matchedString = "\"" + matchedString +"\"";

      setString(mLine.substr(0, replacePos).append(appender + matchedString));    // Add match to the command
   }
}


// Draw our cursor, assuming string is drawn at x,y  (vert spacing works differently than on the angle version
void LineEditor::drawCursor(S32 x, S32 y, S32 fontSize)
{
   S32 offset;
   
   if(mMasked)
   {
      offset = getStringWidth(fontSize, string(mCursorOffset, MASK_CHAR).c_str());
   }
   else
   {
      U32 offsetCharacters = mCursorOffset / mDisplayedCharacters * mDisplayedCharacters;
      offset = getStringWidth(fontSize, mLine.substr(offsetCharacters, mCursorOffset - offsetCharacters).c_str());
   }

   drawCursorAngle(x, y + fontSize, fontSize, offset, 0);
}


// Draw our cursor, assuming string is drawn at x,y with starting width
void LineEditor::drawCursor(S32 x, S32 y, S32 fontSize, S32 startingWidth)
{
   drawCursorAngle(x, y + fontSize, fontSize, startingWidth, 0);
}


// Draw our cursor, assuming string is drawn at x,y at specified angle 
void LineEditor::drawCursorAngle(F32 x, F32 y, F32 fontSize, F32 angle)
{
   S32 width = S32(getStringWidth(fontSize, mLine.c_str()));
   drawCursorAngle(S32(x), S32(y), fontSize, width, angle);
}


void LineEditor::drawCursorAngle(S32 x, S32 y, S32 fontSize, S32 width, F32 angle)
{
   drawCursorAngle(x, y, F32(fontSize), width, angle);
}


// static
void LineEditor::drawCursorAngle(S32 x, S32 y, F32 fontSize, S32 width, F32 angle)
{
   if((Platform::getRealMilliseconds() / 100) % 2)
   {
      F32 xpos = x + ((F32) width  * cos(angle));
      F32 ypos = y + ((F32) width  * sin(angle));

      drawAngleString(xpos, ypos, (F32)fontSize, angle, "_");
   }
}


bool LineEditor::addChar(const char c) 
{ 
   if(c == 0)
      return false;

   switch(mFilter)
   {
   case digitsOnlyFilter: if(c < '0' || c > '9') return false;
      break;
   case numericFilter: if(c != '-' && c != '.' && (c < '0' || c > '9')) return false;
      break;
   case nickNameFilter:
      if(c == '"') return false;
      // if(c == '%') return false; // Did not see any problem with allowing % in usernames; to avoid this problem, use logprintf("%s", nicknamestring), not logprintf(nicknamestring)
      if(c == ' ' && mLine.c_str()[0] == 0) return false; // Don't let name start with a space.
      break;
   case fileNameFilter:
      if((c < '0' || c > '9') && (c != '_') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z')) return false;
      break;
   case allAsciiFilter:
   default:
      break;
   }

   if(length() < mMaxLen)
   {
      mLine = mLine.substr(0, mCursorOffset) + c + mLine.substr(mCursorOffset, mLine.length() - mCursorOffset);
      mCursorOffset += 1;
   }
   mMatchIndex = -1;
   return true;
}


bool LineEditor::handleKey(InputCode inputCode)
{
   switch(inputCode)
   {
      case KEY_BACKSPACE:
         backspacePressed();
         break;
      case KEY_DELETE:
         deletePressed();
         break;
      case KEY_HOME:
         mCursorOffset = 0;
         break;
      case KEY_END:
         mCursorOffset = mLine.length();
         break;
      case KEY_LEFT:
         mCursorOffset = max(0, (int) mCursorOffset - 1);
         break;
      case KEY_RIGHT:
         mCursorOffset = min((int) mLine.length(), (int) mCursorOffset + 1);
         break;
      default:
         return false;
   }
   return true;
}


S32 LineEditor::getMaxLen()
{
   return mMaxLen;
}


bool LineEditor::operator==(LineEditor &lineEditor) const
{
   return mLine == lineEditor.getString();
}


};

