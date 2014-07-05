//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "lineEditor.h"
#include "Colors.h"
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


U32 LineEditor::length() const
{
   return (U32) mLine.length();
}


void LineEditor::backspacePressed()
{
   if(mCursorOffset == 0)
      return;

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


// Has to return a string because if we return c_str, the string it's based on will be deleted when we exit this function
// and our pointer will become invalid.  Sorry!
string LineEditor::getDisplayString() const
{
   U32 offsetCharacters;
   static const U32 chunkSize = 10;
   if(mCursorOffset < mDisplayedCharacters)
   {
      offsetCharacters = 0;
   }
   else
   {
      offsetCharacters = (mCursorOffset - mDisplayedCharacters) / chunkSize + 1;
      offsetCharacters *= chunkSize;
   }

   return mMasked ? string(mLine.length() - offsetCharacters, MASK_CHAR) : 
                    mLine.substr(offsetCharacters, MIN(mDisplayedCharacters, mLine.length() - offsetCharacters));
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
void LineEditor::completePartial(const Vector<string> *candidates, const string &partial, std::size_t replacePos, const string &appender, bool wrapQuotes)
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
      if(wrapQuotes && matchedString.find_first_of(" ") != string::npos)
         matchedString = "\"" + matchedString +"\"";

      setString(mLine.substr(0, replacePos).append(appender + matchedString));    // Add match to the command
   }
}


// Draw our cursor, assuming string is drawn at x,y  (vert spacing works differently than on the angle version
void LineEditor::drawCursor(S32 x, S32 y, S32 fontSize) const
{
   static const U32 chunkSize = 10;
   S32 offset;
   
   if(mMasked)
   {
      offset = getStringWidth(fontSize, string(mCursorOffset, MASK_CHAR).c_str());
   }
   else
   {
      // FIXME: I need to be documented!
      U32 offsetCharacters;
      if(mCursorOffset < mDisplayedCharacters)
      {
         offsetCharacters = 0;
      }
      else
      {
         offsetCharacters = (mCursorOffset - mDisplayedCharacters) / chunkSize + 1;
         offsetCharacters *= chunkSize;
      }
      
      offset = getStringWidth(fontSize, mLine.substr(offsetCharacters, mCursorOffset - offsetCharacters).c_str());
   }

   drawCursor(x, y, fontSize, offset);
}


// Draw our cursor, assuming string is drawn at x,y and cursor should be offset to the right
void LineEditor::drawCursor(S32 x, S32 y, S32 fontSize, S32 offset) const
{
   if(Platform::getRealMilliseconds() / 500 % 2)
      return;

   // Cursor width.  We just draw as a bar because all we support is insert mode
   static const S32 width = 2;

   // This would be used for overwrite mode, if we supported it
//   S32 width = MAX(2, getStringWidth(fontSize, mLine.substr(mCursorOffset, 1).c_str()));

   drawFilledRect(x + offset, y, x + offset + width, y + fontSize + 3, Colors::white, 0.3f);
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

      // %s are banned because of their use in this function: ChatMessageDisplayer::substitueVars(), and it could cause
      // confusion if a player referred to another player and got a variable substitiution instead.  We could/should probably
      // either remove or improve that capability, then we can remove the following line.
      if(c == '%') return false; 

      if(c == ' ' && mLine.c_str()[0] == 0) return false; // Don't let name start with a space.
      break;
   case fileNameFilter:
      // \/:*?"<>|
      if(string("\\/:*?\"<>|").find(c) != string::npos) return false;
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
   if(inputCode == KEY_U && InputCodeManager::checkModifier(KEY_CTRL))
   {
      setString(mLine.substr(mCursorOffset, mLine.length() - mCursorOffset));
      mCursorOffset = 0;
      return true;
   }

   if(inputCode == KEY_W && InputCodeManager::checkModifier(KEY_CTRL))
   {
      size_t spacePos = mLine.rfind(" ", mCursorOffset - 2);
      if(spacePos == string::npos)
      {
         spacePos = 0;
      }
      else
      {
         spacePos += 1;
      }

      string left = mLine.substr(0, spacePos);
      string right = mLine.substr(mCursorOffset, mLine.length() - mCursorOffset);
      setString(left + right);
      mCursorOffset = left.length();
      return true;
   }

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

