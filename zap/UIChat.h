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

class ChatMessage
{
public:
   ChatMessage() { /* do nothing */ }                             // Quickie constructor

   ChatMessage(string frm, string msg, Color col, bool isPriv)    // "Real" constructor
   {
      color = col;
      message = msg;
      from = frm;
      isPrivate = isPriv;
   }

   Color color;      // Chat message colors
   string message;   // Hold chat messages
   string from;      // Hold corresponding nicks
   bool isPrivate;   // Holds public/private status of message
};

///////////////////////////////////////
//////////////////////////////////////

// For sorting our color-nick map, which we'll never do, so this is essentially a dummy
struct strCmp {
   bool operator()( string s1, string s2 ) const {
      return s1 < s2;
   }
};

// All our chat interfaces will inherit from this
class AbstractChat
{
private:
   static std::map<string, Color, strCmp> mFromColors;       // Map nicknames to colors
   static U32 mColorPtr;
   Color getNextColor();                                     // Get next available color for a new nick
   static const S32 MessagesToRetain = 200;                  // Plenty for now

   static U32 mMessageCount;

protected:
   // Message data
   static ChatMessage mMessages[MessagesToRetain];

   ChatMessage getMessage(U32 index);

   U32 getMessageCount() { return mMessageCount; }
   S32 getMessagesToRetain() { return MessagesToRetain; }

public:
   void newMessage(string from, string message, bool isPrivate);   // Handle incoming msg
   void leaveGlobalChat();

   // To be overridden:
   virtual void deliverPrivateMessage(const char *sender, const char *message) = NULL;
};


///////////////////////////////////////
//////////////////////////////////////

class ChatUserInterface: public UserInterface, public AbstractChat
{
private:
   static const S32 MessageDisplayCount = 23;
   const char *menuTitle;
   Color menuSubTitleColor;

   const char *menuFooter;

public:
   ChatUserInterface();          // Constructor

   // UI related
   void render();
   void onKeyDown(KeyCode keyCode, char ascii);
   
   void onActivate();
   void onActivateLobbyMode();

   void deliverPrivateMessage(const char *sender, const char *message);

   void onEscape();

   void idle(U32 timeDelta);

   // Mechanics related

   static const U32 GlobalChatFontSize = 16;   // Font size to display those messages

   char mChatBuffer[200];     // Outgoing chat msg
   U32 mChatCursorPos;        // Where is cursor?

   Vector<StringTableEntry> mPlayersInGlobalChat;

   void cancelChat();                                             // Get out of chat mode
   void issueChat();                                              // Send chat message
};

extern ChatUserInterface gChatInterface;

};

#endif

