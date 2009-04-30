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

#ifndef _UICHAT_H_
#define _UICHAT_H_

#include "tnlNetBase.h"

#include "game.h"
#include "UI.h"
#include "point.h"

#include <map>

namespace Zap
{

class ChatUserInterface : public UserInterface
{

public:
   const char *menuTitle;
   Color menuSubTitleColor;

   const char *menuFooter;

   ChatUserInterface();          // Constructor

   // UI related
   void render();
   void onKeyDown(KeyCode keyCode, char ascii);
   void onActivate();
   void onEscape();

   void idle(U32 timeDelta);

   // Mechanics related
   enum {
      MessageDisplayCount = 18,  // How many chat messages do we display?
   };

   static const U32 GlobalChatFontSize = 20;   // Font size to display those messages

   char mChatBuffer[200];     // Outgoing chat msg
   U32 mChatCursorPos;        // Where is cursor?

   Color mDisplayMessageColor[MessageDisplayCount];               // Chat message colors
   char mMessages[MessageDisplayCount][MAX_CHAT_MSG_LENGTH];      // Hold chat messages
   char mNicks[MessageDisplayCount][MAX_SHORT_TEXT_LEN];          // Hold corresponding nicks
   bool mIsPrivate[MessageDisplayCount];                          // Holds public/private status of message
   void newMessage(const char *nick, bool isPrivate, const char *message, ...);   // Handle incoming msg
   void cancelChat();                                             // Get out of chat mode
   void issueChat();                                              // Send chat message

   S32 mColorPtr;             // Keep track of which color will be used for next unknown nick

   // For sorting our color-nick map, which we'll never do, so this is essentially a dummy
   struct strCmp {
      bool operator()( const char* s1, const char* s2 ) const {
         return strcmp( s1, s2 ) < 0;
      }
   };

   std::map<const char*, Color, strCmp> mNickColors;  // Map nicknames to colors
   Color getNextColor();                              // Get next available color for a new nick
   S32 mNumColors;
};

extern ChatUserInterface gChatInterface;

};

#endif
