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


#include "UIManager.h"

#include "UI.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "UIMessage.h"
#include "UIYesNo.h"
#include "UIQueryServers.h"
#include "UIEditor.h"            // For EditorUserInterface def
#include "UIInstructions.h"
#include "UIKeyDefMenu.h"
#include "UIDiagnostics.h"
#include "UIGameParameters.h"
#include "UICredits.h"
#include "UIEditorInstructions.h"
#include "UIChat.h"
#include "UITeamDefMenu.h"
#include "UIGame.h"
#include "UIHighScores.h"


namespace Zap
{

// Constructor
UIManager::UIManager(ClientGame *clientGame) 
{ 
   mGame = clientGame; 

   mMainMenuUserInterface = NULL;
   mGameParamUserInterface = NULL;
   mYesNoUserInterface = NULL;
   mTeamMenuUserInterface = NULL;
   mQueryServersUserInterface = NULL;
   mServerPasswordEntryUserInterface = NULL;
   mGameUserInterface = NULL;
   mPlayerMenuUserInterface = NULL;
   mNameEntryUserInterface = NULL;
   mMessageUserInterface = NULL;
   mLevelMenuUserInterface = NULL;
   mLevelMenuSelectUserInterface = NULL;
   mLevelChangeOrAdminPasswordEntryUserInterface = NULL;
   mHostMenuUserInterface = NULL;
   mGameMenuUserInterface = NULL;
   mErrorMsgUserInterface = NULL;
   mInstructionsUserInterface = NULL;
   mOptionsMenuUserInterface = NULL;
   mHighScoresUserInterface = NULL;
   mKeyDefMenuUserInterface = NULL;
   mDiagnosticUserInterface = NULL;
   mCreditsUserInterface = NULL;
   mEditorInstructionsUserInterface = NULL;
   mChatInterface = NULL;
   mSuspendedUserInterface = NULL;
   mEditorMenuUserInterface = NULL;
   mSplashUserInterface = NULL;
   mLevelNameEntryUserInterface = NULL;
   mEditorUserInterface = NULL;
   mTeamDefUserInterface = NULL;
}


// Destructor
UIManager::~UIManager()
{
   delete mMainMenuUserInterface;
   delete mGameParamUserInterface;
   delete mYesNoUserInterface;
   delete mTeamMenuUserInterface;
   delete mQueryServersUserInterface;
   delete mServerPasswordEntryUserInterface;
   delete mGameUserInterface;
   delete mPlayerMenuUserInterface;
   delete mNameEntryUserInterface;
   delete mMessageUserInterface;
   delete mLevelMenuUserInterface;
   delete mLevelMenuSelectUserInterface;
   delete mLevelChangeOrAdminPasswordEntryUserInterface;
   delete mHostMenuUserInterface;
   delete mGameMenuUserInterface;
   delete mErrorMsgUserInterface;
   delete mInstructionsUserInterface;
   delete mOptionsMenuUserInterface;
   delete mKeyDefMenuUserInterface;
   delete mDiagnosticUserInterface;
   delete mCreditsUserInterface;
   delete mEditorInstructionsUserInterface;
   delete mChatInterface;
   delete mSuspendedUserInterface;
   delete mEditorMenuUserInterface;
   delete mSplashUserInterface;
   delete mLevelNameEntryUserInterface;
   delete mEditorUserInterface;
   delete mTeamDefUserInterface;
}


// Check whether a particular menu is already being displayed
bool UIManager::isOpen(UIID uiid)
{
   if(UserInterface::current->getMenuID() == uiid)
      return true;

   for(S32 i = 0; i < mPrevUIs.size(); i++)
      if(mPrevUIs[i]->getMenuID() == uiid)
         return true;

   return false;
}


GameParamUserInterface *UIManager::getGameParamUserInterface()
{
   // Lazily initialize
   if(!mGameParamUserInterface)
      mGameParamUserInterface = new GameParamUserInterface(mGame);

   return mGameParamUserInterface;
}


MainMenuUserInterface *UIManager::getMainMenuUserInterface()
{
   // Lazily initialize
   if(!mMainMenuUserInterface)
      mMainMenuUserInterface = new MainMenuUserInterface(mGame);

   return mMainMenuUserInterface;
}


EditorUserInterface *UIManager::getEditorUserInterface()
{
   // Lazily initialize
   if(!mEditorUserInterface)
      mEditorUserInterface = new EditorUserInterface(mGame);

   return mEditorUserInterface;
}


YesNoUserInterface *UIManager::getYesNoUserInterface()
{
   // Lazily initialize
   if(!mYesNoUserInterface)
      mYesNoUserInterface = new YesNoUserInterface(mGame);

   return mYesNoUserInterface;
}


TeamMenuUserInterface *UIManager::getTeamMenuUserInterface()
{
   // Lazily initialize
   if(!mTeamMenuUserInterface)
      mTeamMenuUserInterface = new TeamMenuUserInterface(mGame);

   return mTeamMenuUserInterface;
}


QueryServersUserInterface *UIManager::getQueryServersUserInterface()
{
   // Lazily initialize
   if(!mQueryServersUserInterface)
      mQueryServersUserInterface = new QueryServersUserInterface(mGame);

   return mQueryServersUserInterface;
}


ServerPasswordEntryUserInterface *UIManager::getServerPasswordEntryUserInterface()
{
   // Lazily initialize
   if(!mServerPasswordEntryUserInterface)
      mServerPasswordEntryUserInterface = new ServerPasswordEntryUserInterface(mGame);

   return mServerPasswordEntryUserInterface;
}


GameUserInterface *UIManager::getGameUserInterface()
{
   // Lazily initialize
   if(!mGameUserInterface)
      mGameUserInterface = new GameUserInterface(mGame);

   return mGameUserInterface;
}


PlayerMenuUserInterface *UIManager::getPlayerMenuUserInterface()
{
   // Lazily initialize
   if(!mPlayerMenuUserInterface)
      mPlayerMenuUserInterface = new PlayerMenuUserInterface(mGame);

   return mPlayerMenuUserInterface;
}


NameEntryUserInterface *UIManager::getNameEntryUserInterface()
{
   // Lazily initialize
   if(!mNameEntryUserInterface)
      mNameEntryUserInterface = new NameEntryUserInterface(mGame);

   return mNameEntryUserInterface;
}


MessageUserInterface *UIManager::getMessageUserInterface()
{
   // Lazily initialize
   if(!mMessageUserInterface)
      mMessageUserInterface = new MessageUserInterface(mGame);

   return mMessageUserInterface;
}


LevelMenuUserInterface *UIManager::getLevelMenuUserInterface()
{
   // Lazily initialize
   if(!mLevelMenuUserInterface)
      mLevelMenuUserInterface = new LevelMenuUserInterface(mGame);

   return mLevelMenuUserInterface;
}


LevelMenuSelectUserInterface *UIManager::getLevelMenuSelectUserInterface()
{
   // Lazily initialize
   if(!mLevelMenuSelectUserInterface)
      mLevelMenuSelectUserInterface = new LevelMenuSelectUserInterface(mGame);

   return mLevelMenuSelectUserInterface;
}


LevelChangeOrAdminPasswordEntryUserInterface *UIManager::getLevelChangeOrAdminPasswordEntryUserInterface()
{
   // Lazily initialize
   if(!mLevelChangeOrAdminPasswordEntryUserInterface)
      mLevelChangeOrAdminPasswordEntryUserInterface = new LevelChangeOrAdminPasswordEntryUserInterface(mGame);

   return mLevelChangeOrAdminPasswordEntryUserInterface;
}


HostMenuUserInterface *UIManager::getHostMenuUserInterface()
{
   // Lazily initialize
   if(!mHostMenuUserInterface)
      mHostMenuUserInterface = new HostMenuUserInterface(mGame);

   return mHostMenuUserInterface;
}


GameMenuUserInterface *UIManager::getGameMenuUserInterface()
{
   // Lazily initialize
   if(!mGameMenuUserInterface)
      mGameMenuUserInterface = new GameMenuUserInterface(mGame);

   return mGameMenuUserInterface;
}


ErrorMessageUserInterface *UIManager::getErrorMsgUserInterface()
{
   // Lazily initialize
   if(!mErrorMsgUserInterface)
      mErrorMsgUserInterface = new ErrorMessageUserInterface(mGame);

   return mErrorMsgUserInterface;
}


LevelNameEntryUserInterface *UIManager::getLevelNameEntryUserInterface()
{
   // Lazily initialize
   if(!mLevelNameEntryUserInterface)
      mLevelNameEntryUserInterface = new LevelNameEntryUserInterface(mGame);

   return mLevelNameEntryUserInterface;
}


InstructionsUserInterface *UIManager::getInstructionsUserInterface()
{
   // Lazily initialize
   if(!mInstructionsUserInterface)
      mInstructionsUserInterface = new InstructionsUserInterface(mGame);

   return mInstructionsUserInterface;
}


OptionsMenuUserInterface *UIManager::getOptionsMenuUserInterface()
{
   // Lazily initialize
   if(!mOptionsMenuUserInterface)
      mOptionsMenuUserInterface = new OptionsMenuUserInterface(mGame);

   return mOptionsMenuUserInterface;
}


HighScoresUserInterface *UIManager::getHighScoresUserInterface()
{
   // Lazily initialize
   if(!mHighScoresUserInterface)
      mHighScoresUserInterface = new HighScoresUserInterface(mGame);

   return mHighScoresUserInterface;
}


KeyDefMenuUserInterface *UIManager::getKeyDefMenuUserInterface()
{
   // Lazily initialize
   if(!mKeyDefMenuUserInterface)
      mKeyDefMenuUserInterface = new KeyDefMenuUserInterface(mGame);

   return mKeyDefMenuUserInterface;
}


DiagnosticUserInterface *UIManager::getDiagnosticUserInterface()
{
   // Lazily initialize
   if(!mDiagnosticUserInterface)
      mDiagnosticUserInterface = new DiagnosticUserInterface(mGame);

   return mDiagnosticUserInterface;
}


CreditsUserInterface *UIManager::getCreditsUserInterface()
{
   // Lazily initialize
   if(!mCreditsUserInterface)
      mCreditsUserInterface = new CreditsUserInterface(mGame);

   return mCreditsUserInterface;
}


EditorInstructionsUserInterface *UIManager::getEditorInstructionsUserInterface()
{
   // Lazily initialize
   if(!mEditorInstructionsUserInterface)
      mEditorInstructionsUserInterface = new EditorInstructionsUserInterface(mGame);

   return mEditorInstructionsUserInterface;
}


ChatUserInterface *UIManager::getChatUserInterface()
{
   // Lazily initialize
   if(!mChatInterface)
      mChatInterface = new ChatUserInterface(mGame);

   return mChatInterface;
}


SuspendedUserInterface *UIManager::getSuspendedUserInterface()
{
   // Lazily initialize
   if(!mSuspendedUserInterface)
      mSuspendedUserInterface = new SuspendedUserInterface(mGame);

   return mSuspendedUserInterface;
}


EditorMenuUserInterface *UIManager::getEditorMenuUserInterface()
{
   // Lazily initialize
   if(!mEditorMenuUserInterface)
      mEditorMenuUserInterface = new EditorMenuUserInterface(mGame);

   return mEditorMenuUserInterface;
}


SplashUserInterface *UIManager::getSplashUserInterface()
{
   // Lazily initialize
   if(!mSplashUserInterface)
      mSplashUserInterface = new SplashUserInterface(mGame);

   return mSplashUserInterface;
}


TeamDefUserInterface *UIManager::getTeamDefUserInterface()
{
   // Lazily initialize
   if(!mTeamDefUserInterface)
      mTeamDefUserInterface = new TeamDefUserInterface(mGame);

   return mTeamDefUserInterface;
}


// Reactivate previous interface, going to fallback if there is none
void UIManager::reactivatePrevUI()
{
   if(mPrevUIs.size())
   {
      UserInterface *prev = mPrevUIs.last();
      mPrevUIs.pop_back();
      prev->reactivate();
   }
   else
      getMainMenuUserInterface()->reactivate();      // Fallback if everything else has failed
}


// Like above, except we specify a target menu to go to
void UIManager::reactivateMenu(const UserInterface *target)
{
   // Keep discarding menus until we find the one we want
   while(mPrevUIs.size() && (mPrevUIs.last()->getMenuID() != target->getMenuID()) )
      mPrevUIs.pop_back();

   if(mPrevUIs.size())
      // Now that the next one is our target, when we reactivate, we'll be where we want to be
      reactivatePrevUI();
   else
      getMainMenuUserInterface()->reactivate();      // Fallback if everything else has failed
}


UserInterface *UIManager::getPrevUI()
{
   return mPrevUIs.last();
}


UserInterface *UIManager::getCurrentUI()
{
   return UserInterface::current;
}


bool UIManager::hasPrevUI()
{
   return mPrevUIs.size() > 0;
}


void UIManager::clearPrevUIs()
{
   mPrevUIs.clear();
}


// Have to pass ui to avoid stack overflow when trying to render UIs two-levels deep
void UIManager::renderPrevUI(const UserInterface *ui)
{
   for(S32 i = mPrevUIs.size() - 1; i > 0; i--)    // NOT >= 0!
      if(mPrevUIs[i] == ui)
         mPrevUIs[i-1]->render();
}


// Did we arrive at our current interface via the specified interface?
bool UIManager::cameFrom(UIID menuID)
{
   for(S32 i = 0; i < mPrevUIs.size(); i++)
      if(mPrevUIs[i]->getMenuID() == menuID)
         return true;

   return false;
}


void UIManager::saveUI(UserInterface *ui)
{
   mPrevUIs.push_back(ui);
}


void UIManager::renderCurrent()
{
   UserInterface *current = UserInterface::current;

   current->renderCurrent();
   current->renderMasterStatus();
}


};
