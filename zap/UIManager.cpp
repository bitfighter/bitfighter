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
#include "ScreenInfo.h"
#include "ClientGame.h"

#ifdef TNL_OS_MOBILE
#  include "SDL_opengles.h"
#else
#  include "SDL_opengl.h"
#endif


namespace Zap
{

// Constructor
UIManager::UIManager(ClientGame *clientGame) 
{ 
   mGame = clientGame; 
   mCurrentInterface = NULL;

   mLastUI = NULL;
   mLastWasLower = false;


   mMainMenuUserInterface = NULL;

   mChatInterface = NULL;
   mCreditsUserInterface = NULL;
   mDiagnosticUserInterface = NULL;
   mEditorInstructionsUserInterface = NULL;
   mEditorMenuUserInterface = NULL;
   mEditorUserInterface = NULL;
   mErrorMsgUserInterface = NULL;
   mGameMenuUserInterface = NULL;
   mGameParamUserInterface = NULL;
   mGameUserInterface = NULL;
   mHighScoresUserInterface = NULL;
   mHostMenuUserInterface = NULL;
   mInputOptionsUserInterface = NULL;
   mInstructionsUserInterface = NULL;
   mKeyDefMenuUserInterface = NULL;
   mLevelChangeOrAdminPasswordEntryUserInterface = NULL;
   mLevelMenuSelectUserInterface = NULL;
   mLevelMenuUserInterface = NULL;
   mLevelNameEntryUserInterface = NULL;
   mMessageUserInterface = NULL;
   mNameEntryUserInterface = NULL;
   mOptionsMenuUserInterface = NULL;
   mPlayerMenuUserInterface = NULL;
   mQueryServersUserInterface = NULL;
   mServerPasswordEntryUserInterface = NULL;
   mSplashUserInterface = NULL;
   mSoundOptionsMenuUserInterface = NULL;
   mSuspendedUserInterface = NULL;
   mTeamDefUserInterface = NULL;
   mTeamMenuUserInterface = NULL;
   mYesNoUserInterface = NULL;

   mMenuTransitionTimer.reset(0);      // Set to 100 for a dizzying effect; doing so will cause editor to crash, so beware!
}


// Destructor
UIManager::~UIManager()
{
   delete mMainMenuUserInterface;
   delete mChatInterface;
   delete mCreditsUserInterface;
   delete mDiagnosticUserInterface;
   delete mEditorInstructionsUserInterface;
   delete mEditorMenuUserInterface;
   delete mEditorUserInterface;
   delete mErrorMsgUserInterface;
   delete mGameMenuUserInterface;
   delete mGameParamUserInterface;
   delete mGameUserInterface;
   delete mHostMenuUserInterface;
   delete mInputOptionsUserInterface;
   delete mInstructionsUserInterface;
   delete mKeyDefMenuUserInterface;
   delete mLevelChangeOrAdminPasswordEntryUserInterface;
   delete mLevelMenuSelectUserInterface;
   delete mLevelMenuUserInterface;
   delete mLevelNameEntryUserInterface;
   delete mMessageUserInterface;
   delete mNameEntryUserInterface;
   delete mOptionsMenuUserInterface;
   delete mPlayerMenuUserInterface;
   delete mQueryServersUserInterface;
   delete mServerPasswordEntryUserInterface;
   delete mSplashUserInterface;
   delete mSoundOptionsMenuUserInterface;
   delete mSuspendedUserInterface;
   delete mTeamDefUserInterface;
   delete mTeamMenuUserInterface;
   delete mYesNoUserInterface;
}


UserInterface *UIManager::getUI(UIID menuId)
{
   switch(menuId)
   {
      case LevelChangePasswordEntryUI:
         return getLevelChangeOrAdminPasswordEntryUserInterface();
      case GlobalChatUI:
         return getChatUserInterface();
      case CreditsUI:
         return getCreditsUserInterface();
      case DiagnosticsScreenUI:
         return getDiagnosticUserInterface();
      case EditorInstructionsUI:
         return getEditorInstructionsUserInterface();
      case EditorUI:
         return getEditorUserInterface();
      case EditorMenuUI:
         return getEditorMenuUserInterface();
      case ErrorMessageUI:
         return getErrorMsgUserInterface();
      case GameMenuUI:
         return getGameMenuUserInterface();
      case GameParamsUI:
         return getGameParamUserInterface();
      case GameUI:
         return getGameUserInterface();
      case SuspendedUI:
         return getSuspendedUserInterface();
      case HighScoresUI:
         return getHighScoresUserInterface();
      case HostingUI:
         return getHostMenuUserInterface();
      case InstructionsUI:
         return getInstructionsUserInterface();
      case InputOptionsUI:
         return getInputOptionsUserInterface();
      case KeyDefUI:
         return getKeyDefMenuUserInterface();
      case LevelUI:
         return getLevelMenuUserInterface();
      case LevelNameEntryUI:
         return getLevelNameEntryUserInterface();
      case LevelTypeUI:
         return getLevelMenuUserInterface();
      case MainUI:
         return getMainMenuUserInterface();
      case NameEntryUI:
         return getNameEntryUserInterface();
      case OptionsUI:
         return getOptionsMenuUserInterface();
      case PasswordEntryUI:
         return getServerPasswordEntryUserInterface();
      case PlayerUI:
         return getPlayerMenuUserInterface();
      case SoundOptionsUI:
         return getSoundOptionsMenuUserInterface();
      case TeamUI:
         return getTeamMenuUserInterface();
      case QueryServersScreenUI:
         return getQueryServersUserInterface();
      case SplashUI:
         return getSplashUserInterface();
      case TeamDefUI:
         return getTeamDefUserInterface();
      //case TextEntryUI:
      //   return getTextEntryUserInterface();
      case YesOrNoUI:
         return getYesNoUserInterface();
      default:
         TNLAssert(false, "Unknown or unexpected UI!");
         return NULL;
   }
}


// Check whether a particular menu is already being displayed
bool UIManager::isCurrentUI(UIID uiid)
{
   if(mCurrentInterface->getMenuID() == uiid)
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


SoundOptionsMenuUserInterface *UIManager::getSoundOptionsMenuUserInterface()
{
   // Lazily initialize
   if(!mSoundOptionsMenuUserInterface)
      mSoundOptionsMenuUserInterface = new SoundOptionsMenuUserInterface(mGame);

   return mSoundOptionsMenuUserInterface;
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


InputOptionsMenuUserInterface *UIManager::getInputOptionsUserInterface()
{
   // Lazily initialize
   if(!mInputOptionsUserInterface)
      mInputOptionsUserInterface = new InputOptionsMenuUserInterface(mGame);

   return mInputOptionsUserInterface;
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
   if(mPrevUIs.size() > 0)
   {
      UserInterface *prev = mPrevUIs.last();
      mPrevUIs.pop_back();

      mLastUI = mCurrentInterface;
      mCurrentInterface = prev;
      mLastWasLower = true;
      
      mCurrentInterface->reactivate();
   }
   else
      getMainMenuUserInterface()->reactivate();      // Fallback if everything else has failed

   mMenuTransitionTimer.reset();
}


void UIManager::reactivate(UIID menuId)
{
   // Keep discarding menus until we find the one we want
   while(mPrevUIs.size() && (mPrevUIs.last()->getMenuID() != menuId) )
      mPrevUIs.pop_back();

   // Now that the next one is our target, when we reactivate, we'll be where we want to be
   reactivatePrevUI();
}


UserInterface *UIManager::getPrevUI()
{
   if(mPrevUIs.size() == 0)
      return NULL;

   return mPrevUIs.last();
}


UserInterface *UIManager::getCurrentUI()
{
   return mCurrentInterface;
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
   // This will cause stack overflows, as current can be UI is last on stack!
   //if(mPrevUIs.size() > 0)
   //   mPrevUIs.last()->render();

   for(S32 i = mPrevUIs.size() - 1; i > 0; i--)    // NOT >= 0!
      if(mPrevUIs[i] == ui)
      {
         mPrevUIs[i-1]->render();
         return;
      }
}


// Did we arrive at our current interface via the specified interface?
bool UIManager::cameFrom(UIID menuID)
{
   for(S32 i = 0; i < mPrevUIs.size(); i++)
      if(mPrevUIs[i]->getMenuID() == menuID)
         return true;

   return false;
}


void UIManager::activate(UIID menuID, bool save)         // save defaults to true
{
   activate(getUI(menuID), save);
}


void UIManager::activate(UserInterface *ui, bool save)  // save defaults to true
{
   if(save)
      saveUI(mCurrentInterface);

   // Deactivate the UI we're no longer using
   if(mCurrentInterface)             
     mCurrentInterface->onDeactivate(ui->usesEditorScreenMode());

   mLastUI = mCurrentInterface;
   mLastWasLower = false;
   mCurrentInterface = ui;
   mCurrentInterface->activate();

   mMenuTransitionTimer.reset();
}


void UIManager::saveUI(UserInterface *ui)
{
   if(ui)
      mPrevUIs.push_back(ui);
}


//extern ScreenInfo gScreenInfo;

void UIManager::renderCurrent()
{
   // The viewport has been setup by the caller so, regardless of how many clients we're running, we can just render away here.
   // Each viewport should have an aspect ratio of 800x600.

   if(mMenuTransitionTimer.getCurrent() && mLastUI)
   {
      //mLastUI->render();
      //mLastWasLower
         
      // Save viewport
      GLint viewport[4];
      glGetIntegerv(GL_VIEWPORT, viewport);    

      glViewport(viewport[0] + GLint((mLastWasLower ? 1 : -1) * viewport[2] * (1 - mMenuTransitionTimer.getFraction())), 0, viewport[2], viewport[3]);
      mLastUI->render();

      glViewport(viewport[0] - GLint((mLastWasLower ? 1 : -1) * viewport[2] * mMenuTransitionTimer.getFraction()), 0, viewport[2], viewport[3]);
      mCurrentInterface->render();

      // Restore viewport for posterity
      glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

      return;
   }

   TNLAssert(mCurrentInterface, "NULL mCurrentInterface");

   // Run the active UI renderer
   mCurrentInterface->render();
   UserInterface::renderDiagnosticKeysOverlay();    // By putting this here, it will always get rendered, regardless of active UI
   mCurrentInterface->renderMasterStatus();
}


void UIManager::idle(U32 timeDelta)
{
   mMenuTransitionTimer.update(timeDelta);
}


void UIManager::renderAndDimGameUserInterface()
{
   getGameUserInterface()->render();
   UserInterface::dimUnderlyingUI();
}


void UIManager::setHighScores(const Vector<StringTableEntry> &groupNames, const Vector<string> &names, const Vector<string> &scores)
{
   getHighScoresUserInterface()->setHighScores(groupNames, names, scores);
}


// Message relayed through master -- global chat system
void UIManager::gotGlobalChatMessage(const string &from, const string &message, bool isPrivate, bool isSystem, bool fromSelf)
{
   getChatUserInterface()->newMessage(from, message, isPrivate, isSystem, fromSelf);
}


void UIManager::gotServerListFromMaster(const Vector<IPAddress> &serverList)
{
   getQueryServersUserInterface()->gotServerListFromMaster(serverList);
}


void UIManager::setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks)
{
   getChatUserInterface()->setPlayersInGlobalChat(playerNicks);
}


void UIManager::playerJoinedGlobalChat(const StringTableEntry &playerNick)
{
   getChatUserInterface()->playerJoinedGlobalChat(playerNick);
}


void UIManager::playerLeftGlobalChat(const StringTableEntry &playerNick)
{
   getChatUserInterface()->playerLeftGlobalChat(playerNick);
}


void UIManager::gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken)
{
   getQueryServersUserInterface()->gotPingResponse(address, nonce, clientIdentityToken);
}


void UIManager::gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                                 U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired)
{
   getQueryServersUserInterface()->gotQueryResponse(address, nonce, serverName, serverDescr, playerCount, 
                                                    maxPlayers, botCount, dedicated, test, passwordRequired);
}


string UIManager::getLastSelectedServerName()
{
   return getQueryServersUserInterface()->getLastSelectedServerName();
}


void UIManager::setConnectAddressAndActivatePasswordEntryUI(const Address &serverAddress)
{
   ServerPasswordEntryUserInterface *ui = getServerPasswordEntryUserInterface();
   ui->setAddressToConnectTo(serverAddress);

   activate(ui);
}


void UIManager::enableLevelLoadDisplay()
{
   getGameUserInterface()->showLevelLoadDisplay(true, false);
}


void UIManager::serverLoadedLevel(const string &levelName)
{
   getGameUserInterface()->serverLoadedLevel(levelName);
}


void UIManager::disableLevelLoadDisplay(bool fade)
{
   getGameUserInterface()->showLevelLoadDisplay(false, fade);
}


void UIManager::renderLevelListDisplayer()
{
   getGameUserInterface()->renderLevelListDisplayer();
}


void UIManager::setMOTD(const char *motd)
{
   getMainMenuUserInterface()->setMOTD(motd); 
}


void UIManager::setNeedToUpgrade(bool needToUpgrade)
{
   getMainMenuUserInterface()->setNeedToUpgrade(needToUpgrade);
}


void UIManager::gotPassOrPermsReply(const ClientGame *game, const char *message)
{
   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(getCurrentUI()->getMenuID() == GameMenuUI)
      getGameMenuUserInterface()->mMenuSubTitle = message;
   else
      game->displayCmdChatMessage(message);     
}


void UIManager::showPlayerActionMenu(PlayerAction action)
{
   PlayerMenuUserInterface *ui = getPlayerMenuUserInterface();
   ui->action = action;

   activate(ui);
};


void UIManager::showMenuToChangeNameForPlayer(const string &playerName)
{
   TeamMenuUserInterface *ui = getTeamMenuUserInterface();
   ui->nameToChange = playerName;
   activate(ui);                  
}


};
