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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _HELPER_MANAGER_H_
#define _HELPER_MANAGER_H_

#include "ChatHelper.h"
#include "quickChatHelper.h"
#include "loadoutHelper.h"
#include "engineerHelper.h"
#include "TeamShuffleHelper.h"


namespace Zap
{

class HelperManager
{
private:
   // Various helper objects
   Vector<HelperMenu *> mHelperStack;        // Current helper
   HelperMenu *mOffDeckHelper;               // Kind of the opposite of on deck
   ClientGame *mGame;

   // Singleton helper classes
   ChatHelper        mChatHelper;
   QuickChatHelper   mQuickChatHelper;
   LoadoutHelper     mLoadoutHelper;
   EngineerHelper    mEngineerHelper;
   TeamShuffleHelper mTeamShuffleHelper;

   void doExitHelper(S32 index);

public:
   HelperManager();     // Constructor

   void initialize(ClientGame *game);

   void onPlayerJoined();
   void onPlayerQuit();
   void onGameOver();
   void onTextInput(char ascii);

   void reset();

   void idle(U32 timeDelta);

   void render() const;
   void renderEngineeredItemDeploymentMarker(const Ship *ship);

   bool isChatAllowed() const;
   bool isMovementDisabled() const;
   bool isHelperActive() const;
   bool isHelperActive(HelperMenu::HelperMenuType helperType) const;

   bool processInputCode(InputCode inputCode);

   void activateHelp(UIManager *uiManager);              // Show the appropriate help screen, depending on which helper is active

   void quitEngineerHelper();
   void pregameSetup(bool isEnabled);                    // Set whether engineer is allowed on this level
   void setSelectedEngineeredObject(U32 objectType);

   void activateHelper(HelperMenu::HelperMenuType helperType, bool activatedWithChatCmd);
   void activateHelper(ChatHelper::ChatType chatType);   // Activate chat helper with specific chat subtype
   void exitHelper();
   void exitHelper(HelperMenu *helper);
   void doneClosingHelper();

   F32 getDimFactor() const;
   const char *getChatMessage() const;
};


}


#endif
