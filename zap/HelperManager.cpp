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

#include "HelperManager.h"

#include "ClientGame.h"

using namespace TNL;

namespace Zap
{


// Constructor
HelperManager::HelperManager()
{
   mOffDeckHelper = NULL;
   mGame = NULL;
}


void HelperManager::initialize(ClientGame *game)
{
   mGame = game;

   mChatHelper.initialize(game, this);
   mQuickChatHelper.initialize(game, this);
   mLoadoutHelper.initialize(game, this);
   mEngineerHelper.initialize(game, this);
   mTeamShuffleHelper.initialize(game, this);
}


// Gets run when a player joins the game
void HelperManager::onPlayerJoined()
{
   if(mHelperStack.contains(&mTeamShuffleHelper))
      mTeamShuffleHelper.onPlayerJoined();
}


void HelperManager::onPlayerQuit()
{
   if(mHelperStack.contains(&mTeamShuffleHelper))
      mTeamShuffleHelper.onPlayerQuit();
}


void HelperManager::onGameOver()
{
   if(mHelperStack.contains(&mTeamShuffleHelper))   // Exit Shuffle helper to keep things from getting too crashy
      exitHelper(&mTeamShuffleHelper); 
}


void HelperManager::onTextInput(char ascii)
{
   if(mHelperStack.size() > 0)
      mHelperStack.last()->onTextInput(ascii);
}


void HelperManager::reset()
{
   mHelperStack.clear();               // Make sure we're not in chat or loadout-select mode or somesuch
   mOffDeckHelper = NULL;
}


void HelperManager::idle(U32 timeDelta)
{
   for(S32 i = 0; i < mHelperStack.size(); i++)
      mHelperStack[i]->idle(timeDelta);

   if(mOffDeckHelper)
      mOffDeckHelper->idle(timeDelta);
}


void HelperManager::render() const
{
   // Higher indexed helpers render on top
   for(S32 i = 0; i < mHelperStack.size(); i++)
      mHelperStack[i]->render();

   if(mOffDeckHelper)
      mOffDeckHelper->render();
}


bool HelperManager::isHelperActive(HelperMenu::HelperMenuType helperType) const
{
   return mHelperStack.size() > 0 && mHelperStack.last()->getType() == helperType;
}


void HelperManager::activateHelp(UIManager *uiManager)
{
   mHelperStack.last()->activateHelp(uiManager);
}


// Entering chat mode is allowed if 1) there are no helpers open; or 2) the top-most helper allows it
bool HelperManager::isChatAllowed() const
{
   return mHelperStack.size() == 0 || !mHelperStack.last()->isChatDisabled();
}


bool HelperManager::isMovementDisabled() const
{
   return mHelperStack.size() > 0 && mHelperStack.last()->isMovementDisabled();
}


bool HelperManager::isHelperActive() const
{
   return mHelperStack.size() > 0;
}


// Set whether engineer is allowed on this level
void HelperManager::pregameSetup(bool isEnabled)
{
   mLoadoutHelper.pregameSetup(isEnabled);
}


void HelperManager::setSelectedEngineeredObject(U32 objectType)
{
   mEngineerHelper.setSelectedEngineeredObject(objectType);
}


// Returns true if key was handled, false if it should be further processed
bool HelperManager::processInputCode(InputCode inputCode)
{
   if(isHelperActive())
      return mHelperStack.last()->processInputCode(inputCode);

   return false;
}


// TODO: What happens when you are in engineer and are chatting and get killed?


// Used when ship dies while engineering
void HelperManager::quitEngineerHelper()
{
   if(mHelperStack.size() > 0 && mHelperStack.last() == &mEngineerHelper)
      mHelperStack.pop_back();
}


// Enter QuickChat, Loadout, or Engineer mode
void HelperManager::activateHelper(HelperMenu::HelperMenuType helperType, bool activatedWithChatCmd)
{
   // If we're activating this item with a chat cmd, we need to make it the 2nd to last item so that when we
   // can close the chat overlay by popping the last item on the stack, as usual
   S32 index = activatedWithChatCmd ? mHelperStack.size() - 1 : mHelperStack.size();
   TNLAssert(index >= 0, "Bad index!");

   switch(helperType)
   {
      case HelperMenu::ChatHelperType:
         mHelperStack.insert(index, &mChatHelper);
         break;
      case HelperMenu::QuickChatHelperType:
         mHelperStack.insert(index, &mQuickChatHelper);
         break;
      case HelperMenu::LoadoutHelperType:
         mHelperStack.insert(index, &mLoadoutHelper);
         break;
      case HelperMenu::EngineerHelperType:
         mHelperStack.insert(index, &mEngineerHelper);
         break;
      case HelperMenu::ShuffleTeamsHelperType:
         mHelperStack.insert(index, &mTeamShuffleHelper);
         break;
      default:
         TNLAssert(false, "Unknown helperType!");
         return;
   }

   mHelperStack[index]->onActivated();
}


void HelperManager::activateHelper(ChatHelper::ChatType chatType)
{
   activateHelper(HelperMenu::ChatHelperType, false);
   mChatHelper.activate(chatType);
}


// Exit the top-most helper
void HelperManager::exitHelper()
{
   if(mHelperStack.size() > 0)
      doExitHelper(mHelperStack.size() - 1);
}


void HelperManager::doneClosingHelper()
{
   mOffDeckHelper = NULL;
}


// We will darken certain areas of the screen when the helper is active.  This computes how much.  
F32 HelperManager::getDimFactor() const
{
   static const F32 DIM    = 0.2;    
   static const F32 BRIGHT = 1.0;

   // fromDim and toDim are true if chat text should be dimmed, false if not.  Their state is based on the
   // characteristics of the active helper and the state of its activation/deactivation animation.
   bool fromDim = false, toDim = false;

   if(mOffDeckHelper && mOffDeckHelper->isChatDisabled())
      fromDim = true;

   if(mHelperStack.size() > 0 && mHelperStack.last()->isChatDisabled())
      toDim = true;

   if(fromDim)
   {
      if(toDim)         // Transitioning from a dim interfact to another one... should just stay dim
         return DIM;    
      else
         return mOffDeckHelper->getFraction() * (1 - DIM) + DIM;
   }
   else
   {
      if(toDim)
         return mHelperStack.last()->getFraction() * (1 - DIM) + DIM;
      else              // Transitioning from a bright interfact to another one... should just stay bright
         return BRIGHT;
   }
}


// Exit the specified helper
void HelperManager::exitHelper(HelperMenu *helper)
{
   S32 index = mHelperStack.getIndex(helper);

   if(index >= 0)
      doExitHelper(index);
}


void HelperManager::doExitHelper(S32 index)
{
   //mHelperStack[index]->deactivate();
   mOffDeckHelper = mHelperStack[index];
   mHelperStack.erase(index);
   mGame->unsuspendGame();

   if(mHelperStack.size() == 0)
      mGame->undelaySpawn();

   // If animation is disabled for this helper, immediately call doneClosingHelper()
   if(mOffDeckHelper->getAnimationTime() == 0)
      doneClosingHelper();
}


// Render potential location to deploy engineered item -- does nothing if we're not engineering
void HelperManager::renderEngineeredItemDeploymentMarker(const Ship *ship)
{
   if(mHelperStack.getIndex(&mEngineerHelper) != -1)
      mEngineerHelper.renderDeploymentMarker(ship);
}


// Return message being composed in in-game chat
const char *HelperManager::getChatMessage() const
{
   return mChatHelper.getChatMessage();
}


}
