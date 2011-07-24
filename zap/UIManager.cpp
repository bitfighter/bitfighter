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

#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "UIMessage.h"
#include "UIYesNo.h"
#include "UIQueryServers.h"
//#include "UIErrorMessage.h"
#include "UIEditor.h"            // For EditorUserInterface def, needed by EditorGame stuff
#include "UIInstructions.h"
#include "UIKeyDefMenu.h"
#include "UIDiagnostics.h"
#include "UIGameParameters.h"
#include "UICredits.h"
#include "UIEditorInstructions.h"
#include "UIChat.h"
#include "UITeamDefMenu.h"


namespace Zap
{

// Constructor
UIManager::UIManager(Game *game) 
{ 
   mGame = game; 
}


GameParamUserInterface *UIManager::getGameParamUserInterface()
{
   // Lazily initialize
   if(!mGameParamUserInterface.get())
      mGameParamUserInterface = auto_ptr<GameParamUserInterface>(new GameParamUserInterface(mGame));

   return mGameParamUserInterface.get();
}


MainMenuUserInterface *UIManager::getMainMenuUserInterface()
{
   // Lazily initialize
   if(!mMainMenuUserInterface.get())
      mMainMenuUserInterface = auto_ptr<MainMenuUserInterface>(new MainMenuUserInterface(mGame));

   return mMainMenuUserInterface.get();
}


auto_ptr<EditorUserInterface> UIManager::mEditorUserInterface;

EditorUserInterface *UIManager::getEditorUserInterface()
{
   // Lazily initialize
   if(!mEditorUserInterface.get())
      mEditorUserInterface = auto_ptr<EditorUserInterface>(new EditorUserInterface(new EditorGame()));

   return mEditorUserInterface.get();
}


YesNoUserInterface *UIManager::getYesNoUserInterface()
{
   // Lazily initialize
   if(!mYesNoUserInterface.get())
      mYesNoUserInterface = auto_ptr<YesNoUserInterface>(new YesNoUserInterface(mGame));

   return mYesNoUserInterface.get();
}


TeamMenuUserInterface *UIManager::getTeamMenuUserInterface()
{
   // Lazily initialize
   if(!mTeamMenuUserInterface.get())
      mTeamMenuUserInterface = auto_ptr<TeamMenuUserInterface>(new TeamMenuUserInterface(mGame));

   return mTeamMenuUserInterface.get();
}


QueryServersUserInterface *UIManager::getQueryServersUserInterface()
{
   // Lazily initialize
   if(!mQueryServersUserInterface.get())
      mQueryServersUserInterface = auto_ptr<QueryServersUserInterface>(new QueryServersUserInterface(mGame));

   return mQueryServersUserInterface.get();
}


ServerPasswordEntryUserInterface *UIManager::getServerPasswordEntryUserInterface()
{
   // Lazily initialize
   if(!mServerPasswordEntryUserInterface.get())
      mServerPasswordEntryUserInterface = auto_ptr<ServerPasswordEntryUserInterface>(new ServerPasswordEntryUserInterface(mGame));

   return mServerPasswordEntryUserInterface.get();
}


PlayerMenuUserInterface *UIManager::getPlayerMenuUserInterface()
{
   // Lazily initialize
   if(!mPlayerMenuUserInterface.get())
      mPlayerMenuUserInterface = auto_ptr<PlayerMenuUserInterface>(new PlayerMenuUserInterface(mGame));

   return mPlayerMenuUserInterface.get();
}


NameEntryUserInterface *UIManager::getNameEntryUserInterface()
{
   // Lazily initialize
   if(!mNameEntryUserInterface.get())
      mNameEntryUserInterface = auto_ptr<NameEntryUserInterface>(new NameEntryUserInterface(mGame));

   return mNameEntryUserInterface.get();
}


MessageUserInterface *UIManager::getMessageUserInterface()
{
   // Lazily initialize
   if(!mMessageUserInterface.get())
      mMessageUserInterface = auto_ptr<MessageUserInterface>(new MessageUserInterface(mGame));

   return mMessageUserInterface.get();
}


LevelMenuUserInterface *UIManager::getLevelMenuUserInterface()
{
   // Lazily initialize
   if(!mLevelMenuUserInterface.get())
      mLevelMenuUserInterface = auto_ptr<LevelMenuUserInterface>(new LevelMenuUserInterface(mGame));

   return mLevelMenuUserInterface.get();
}


LevelMenuSelectUserInterface *UIManager::getLevelMenuSelectUserInterface()
{
   // Lazily initialize
   if(!mLevelMenuSelectUserInterface.get())
      mLevelMenuSelectUserInterface = auto_ptr<LevelMenuSelectUserInterface>(new LevelMenuSelectUserInterface(mGame));

   return mLevelMenuSelectUserInterface.get();
}


AdminPasswordEntryUserInterface *UIManager::getAdminPasswordEntryUserInterface()
{
   // Lazily initialize
   if(!mAdminPasswordEntryUserInterface.get())
      mAdminPasswordEntryUserInterface = auto_ptr<AdminPasswordEntryUserInterface>(new AdminPasswordEntryUserInterface(mGame));

   return mAdminPasswordEntryUserInterface.get();
}


LevelChangePasswordEntryUserInterface *UIManager::getLevelChangePasswordEntryUserInterface()
{
   // Lazily initialize
   if(!mLevelChangePasswordEntryUserInterface.get())
      mLevelChangePasswordEntryUserInterface = auto_ptr<LevelChangePasswordEntryUserInterface>(new LevelChangePasswordEntryUserInterface(mGame));

   return mLevelChangePasswordEntryUserInterface.get();
}


HostMenuUserInterface *UIManager::getHostMenuUserInterface()
{
   // Lazily initialize
   if(!mHostMenuUserInterface.get())
      mHostMenuUserInterface = auto_ptr<HostMenuUserInterface>(new HostMenuUserInterface(mGame));

   return mHostMenuUserInterface.get();
}


GameMenuUserInterface *UIManager::getGameMenuUserInterface()
{
   // Lazily initialize
   if(!mGameMenuUserInterface.get())
      mGameMenuUserInterface = auto_ptr<GameMenuUserInterface>(new GameMenuUserInterface(mGame));

   return mGameMenuUserInterface.get();
}


ErrorMessageUserInterface *UIManager::getErrorMsgUserInterface()
{
   // Lazily initialize
   if(!mErrorMsgUserInterface.get())
      mErrorMsgUserInterface = auto_ptr<ErrorMessageUserInterface>(new ErrorMessageUserInterface(mGame));

   return mErrorMsgUserInterface.get();
}


LevelNameEntryUserInterface *UIManager::getLevelNameEntryUserInterface()
{
   // Lazily initialize
   if(!mLevelNameEntryUserInterface.get())
      mLevelNameEntryUserInterface = auto_ptr<LevelNameEntryUserInterface>(new LevelNameEntryUserInterface(mGame));

   return mLevelNameEntryUserInterface.get();
}


InstructionsUserInterface *UIManager::getInstructionsUserInterface()
{
   // Lazily initialize
   if(!mInstructionsUserInterface.get())
      mInstructionsUserInterface = auto_ptr<InstructionsUserInterface>(new InstructionsUserInterface(mGame));

   return mInstructionsUserInterface.get();
}


OptionsMenuUserInterface *UIManager::getOptionsMenuUserInterface()
{
   // Lazily initialize
   if(!mOptionsMenuUserInterface.get())
      mOptionsMenuUserInterface = auto_ptr<OptionsMenuUserInterface>(new OptionsMenuUserInterface(mGame));

   return mOptionsMenuUserInterface.get();
}


KeyDefMenuUserInterface *UIManager::getKeyDefMenuUserInterface()
{
   // Lazily initialize
   if(!mKeyDefMenuUserInterface.get())
      mKeyDefMenuUserInterface = auto_ptr<KeyDefMenuUserInterface>(new KeyDefMenuUserInterface(mGame));

   return mKeyDefMenuUserInterface.get();
}


DiagnosticUserInterface *UIManager::getDiagnosticUserInterface()
{
   // Lazily initialize
   if(!mDiagnosticUserInterface.get())
      mDiagnosticUserInterface = auto_ptr<DiagnosticUserInterface>(new DiagnosticUserInterface(mGame));

   return mDiagnosticUserInterface.get();
}


CreditsUserInterface *UIManager::getCreditsUserInterface()
{
   // Lazily initialize
   if(!mCreditsUserInterface.get())
      mCreditsUserInterface = auto_ptr<CreditsUserInterface>(new CreditsUserInterface(mGame));

   return mCreditsUserInterface.get();
}


EditorInstructionsUserInterface *UIManager::getEditorInstructionsUserInterface()
{
   // Lazily initialize
   if(!mEditorInstructionsUserInterface.get())
      mEditorInstructionsUserInterface = auto_ptr<EditorInstructionsUserInterface>(new EditorInstructionsUserInterface(mGame));

   return mEditorInstructionsUserInterface.get();
}


ChatUserInterface *UIManager::getChatUserInterface()
{
   // Lazily initialize
   if(!mChatInterface.get())
      mChatInterface = auto_ptr<ChatUserInterface>(new ChatUserInterface(mGame));

   return mChatInterface.get();
}


SuspendedUserInterface *UIManager::getSuspendedUserInterface()
{
   // Lazily initialize
   if(!mSuspendedUserInterface.get())
      mSuspendedUserInterface = auto_ptr<SuspendedUserInterface>(new SuspendedUserInterface(mGame));

   return mSuspendedUserInterface.get();
}


EditorMenuUserInterface *UIManager::getEditorMenuUserInterface()
{
   // Lazily initialize
   if(!mEditorMenuUserInterface.get())
      mEditorMenuUserInterface = auto_ptr<EditorMenuUserInterface>(new EditorMenuUserInterface(mGame));

   return mEditorMenuUserInterface.get();
}


SplashUserInterface *UIManager::getSplashUserInterface()
{
   // Lazily initialize
   if(!mSplashUserInterface.get())
      mSplashUserInterface = auto_ptr<SplashUserInterface>(new SplashUserInterface(mGame));

   return mSplashUserInterface.get();
}


TeamDefUserInterface *UIManager::getTeamDefUserInterface()
{
   // Lazily initialize
   if(!mTeamDefUserInterface.get())
      mTeamDefUserInterface = auto_ptr<TeamDefUserInterface>(new TeamDefUserInterface(mGame));

   return mTeamDefUserInterface.get();
}



};