//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CHATHELPER_H_
#define _CHATHELPER_H_


#include "helperMenu.h"    // Parent
#include "lineEditor.h"

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

protected:
   void exitHelper();

public:
   explicit ChatHelper();      // Constructor
   virtual ~ChatHelper();

   HelperMenuType getType();

   void activate(ChatType chatType);

   bool isCmdChat() const;    // Returns true if we're composing a command in the chat bar, false otherwise

   void render() const;                
   void onActivated();  
   void activateHelp(UIManager *uiManager);

   bool processInputCode(InputCode inputCode);   
   void onTextInput(char ascii);

   const char *getChatMessage() const;

   bool isMovementDisabled() const;
   bool isChatDisabled() const;

   static void runCommand(ClientGame *game, const char *input);
};

};

#endif

