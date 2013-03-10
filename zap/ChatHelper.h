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

#ifndef _CHATHELPER_H_
#define _CHATHELPER_H_


#include "helperMenu.h"    // Parent

#include "UI.h"
#include "Timer.h"
#include "InputCode.h"

namespace Zap
{

class ChatHelper : public HelperMenu
{
   typedef HelperMenu Parent;

public:
    enum ChatType {               // Types of in-game chat messages:
      GlobalChat,                // Goes to everyone in game
      TeamChat,                  // Goes to teammates only
      CmdChat,                   // Entering a command
      NoChat                     // None of the above
   };

   static const S32 chatCmdSize;

private:
   LineEditor mLineEditor;       // Message being composed
   ChatType mCurrentChatType;    // Current in-game chat mode (global or local)

   void issueChat();                // Send chat message (either Team or Global)

   // Related to running commands
   static void serverCommandHandler(ClientGame *game, const Vector<string> &words);
   F32 getHelperWidth() const;

protected:
   void exitHelper();

public:
   explicit ChatHelper();      // Constructor
   HelperMenuType getType();

   void activate(ChatType chatType);

   bool isCmdChat();          // Returns true if we're composing a command in the chat bar, false otherwise

   void render();                
   void onActivated();  
   void activateHelp(UIManager *uiManager);

   bool processInputCode(InputCode inputCode);   
   void onTextInput(char ascii);

   const char *getChatMessage();

   bool isMovementDisabled();
   bool isChatDisabled();

   static void runCommand(ClientGame *game, const char *input);
};

};

#endif

