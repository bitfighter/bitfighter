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

#include "SoundSystem.h"

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
   mSettings = clientGame->getSettings();

   mCurrentInterface = NULL;

   mLastUI = NULL;
   mLastWasLower = false;

   mMenuTransitionTimer.reset(0);      // Set to 100 for a dizzying effect; doing so will cause editor to crash, so beware!
}


// Destructor
UIManager::~UIManager()
{
   // Do nothing
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
      getUI<MainMenuUserInterface>()->reactivate();      // Fallback if everything else has failed

   mMenuTransitionTimer.reset();
}


void UIManager::reactivate(const UserInterface *ui)
{
   // Keep discarding menus until we find the one we want
   while(mPrevUIs.size() && (mPrevUIs.last() != ui))
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


// Game connection is terminated -- reactivate the appropriate UI
void UIManager::onConnectionTerminated()
{
   if(cameFrom<EditorUserInterface>())
     reactivate(getUI<EditorUserInterface>());
   else
     reactivate(getUI<MainMenuUserInterface>());
}


void UIManager::renderCurrent()
{
   // The viewport has been setup by the caller so, regardless of how many clients we're running, we can just render away here.
   // Each viewport should have an aspect ratio of 800x600.

   if(mMenuTransitionTimer.getCurrent() && mLastUI)
   {


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
   processAudio(timeDelta);
}


// Select music based on where we are
MusicLocation UIManager::selectMusic()
{
   UserInterface *currentUI = getCurrentUI();

   // In game (or one of its submenus)...
   if(currentUI == getUI<GameUserInterface>() || cameFrom<GameUserInterface>())
      return MusicLocationGame;

   // In editor...
   if(currentUI == getUI<EditorUserInterface>() || getUI<EditorUserInterface>())
      return MusicLocationEditor;

   // In credits...
   if(currentUI == getUI<CreditsUserInterface>() || cameFrom<CreditsUserInterface>())
      return MusicLocationCredits;

   // Otherwise, we must be in the menus...
   return MusicLocationMenus;
}


void UIManager::processAudio(U32 timeDelta)
{
   SoundSystem::processAudio(timeDelta, 
                             mSettings->getIniSettings()->sfxVolLevel,
                             mSettings->getIniSettings()->getMusicVolLevel(),
                             mSettings->getIniSettings()->voiceChatVolLevel,
                             selectMusic());  
}


void UIManager::renderAndDimGameUserInterface()
{
   getUI<GameUserInterface>()->render();
   UserInterface::dimUnderlyingUI();
}


void UIManager::setHighScores(const Vector<StringTableEntry> &groupNames, const Vector<string> &names, const Vector<string> &scores)
{
   getUI<HighScoresUserInterface>()->setHighScores(groupNames, names, scores);
}


// Message relayed through master -- global chat system
void UIManager::gotGlobalChatMessage(const string &from, const string &message, bool isPrivate, bool isSystem, bool fromSelf)
{
   getUI<ChatUserInterface>()->newMessage(from, message, isPrivate, isSystem, fromSelf);
}


void UIManager::gotServerListFromMaster(const Vector<IPAddress> &serverList)
{
   getUI<QueryServersUserInterface>()->gotServerListFromMaster(serverList);
}


void UIManager::setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks)
{
   getUI<ChatUserInterface>()->setPlayersInGlobalChat(playerNicks);
}


void UIManager::playerJoinedGlobalChat(const StringTableEntry &playerNick)
{
   getUI<ChatUserInterface>()->playerJoinedGlobalChat(playerNick);
}


void UIManager::playerLeftGlobalChat(const StringTableEntry &playerNick)
{
   getUI<ChatUserInterface>()->playerLeftGlobalChat(playerNick);
}


void UIManager::gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken)
{
   getUI<QueryServersUserInterface>()->gotPingResponse(address, nonce, clientIdentityToken);
}


void UIManager::gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                                 U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired)
{
   getUI<QueryServersUserInterface>()->gotQueryResponse(address, nonce, serverName, serverDescr, playerCount, 
                                                        maxPlayers, botCount, dedicated, test, passwordRequired);
}


string UIManager::getLastSelectedServerName()
{
   return getUI<QueryServersUserInterface>()->getLastSelectedServerName();
}


void UIManager::setConnectAddressAndActivatePasswordEntryUI(const Address &serverAddress)
{
   ServerPasswordEntryUserInterface *ui = getUI<ServerPasswordEntryUserInterface>();
   ui->setAddressToConnectTo(serverAddress);

   activate(ui);
}


void UIManager::enableLevelLoadDisplay()
{
   getUI<GameUserInterface>()->showLevelLoadDisplay(true, false);
}


void UIManager::serverLoadedLevel(const string &levelName)
{
   getUI<GameUserInterface>()->serverLoadedLevel(levelName);
}


void UIManager::disableLevelLoadDisplay(bool fade)
{
   getUI<GameUserInterface>()->showLevelLoadDisplay(false, fade);
}


void UIManager::renderLevelListDisplayer()
{
   getUI<GameUserInterface>()->renderLevelListDisplayer();
}


void UIManager::setMOTD(const char *motd)
{
   getUI<MainMenuUserInterface>()->setMOTD(motd); 
}


void UIManager::setNeedToUpgrade(bool needToUpgrade)
{
   getUI<MainMenuUserInterface>()->setNeedToUpgrade(needToUpgrade);
}


void UIManager::gotPasswordOrPermissionsReply(const ClientGame *game, const char *message)
{
   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(getCurrentUI() == getUI<GameMenuUserInterface>())
      getUI<GameMenuUserInterface>()->mMenuSubTitle = message;
   else
      game->displayCmdChatMessage(message);     
}


void UIManager::showPlayerActionMenu(PlayerAction action)
{
   PlayerMenuUserInterface *ui = getUI<PlayerMenuUserInterface>();
   ui->action = action;

   activate(ui);
};


void UIManager::showMenuToChangeNameForPlayer(const string &playerName)
{
   TeamMenuUserInterface *ui = getUI<TeamMenuUserInterface>();
   ui->nameToChange = playerName;
   activate(ui);                  
}


void UIManager::displayMessageBox(const StringTableEntry &title, const StringTableEntry &instr, 
                                  const Vector<StringTableEntry> &messages)
{
   ErrorMessageUserInterface *ui = getUI<ErrorMessageUserInterface>();

   ui->reset();
   ui->setTitle(title.getString());
   ui->setInstr(instr.getString());

   for(S32 i = 0; i < messages.size(); i++)
      ui->setMessage(i + 1, messages[i].getString());      // UIErrorMsgInterface ==> first line = 1

   activate(ui);
}


void UIManager::displayMessageBox(const char *title, const char *instr, const Vector<string> &messages)
{
   ErrorMessageUserInterface *ui = getUI<ErrorMessageUserInterface>();

   ui->reset();
   ui->setTitle(title);
   ui->setInstr(instr);

   for(S32 i = 0; i < messages.size(); i++)
      ui->setMessage(i + 1, messages[i]);      // UIErrorMsgInterface ==> first line = 1

   activate(ui);
}


void UIManager::startLoadingLevel(F32 lx, F32 ly, F32 ux, F32 uy, bool engineerEnabled)
{
   clearSparks();
   getUI<EditorUserInterface>()->clearRobotLines();
   getUI<GameUserInterface>()->startLoadingLevel(lx, ly, ux, uy, engineerEnabled);
}


void UIManager::readRobotLine(const string &robotLine)
{
   getUI<EditorUserInterface>()->addRobotLine(robotLine);
}


void UIManager::doneLoadingLevel()
{
   getUI<GameUserInterface>()->doneLoadingLevel();
}


void UIManager::clearSparks()
{
   getUI<GameUserInterface>()->clearSparks();
}


void UIManager::emitBlast(const Point &pos, U32 size)
{
   getUI<GameUserInterface>()->emitBlast(pos, size);
}


void UIManager::emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2)
{
   getUI<GameUserInterface>()->emitBurst(pos, scale, color1, color2);
}


void UIManager::emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation)
{
   getUI<GameUserInterface>()->emitDebrisChunk(points, color, pos, vel, ttl, angle, rotation);
}


void UIManager::emitTextEffect(const string &text, const Color &color, const Point &pos)
{
   getUI<GameUserInterface>()->emitTextEffect(text, color, pos);
}


void UIManager::emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, UI::SparkType sparkType)
{
   getUI<GameUserInterface>()->emitSpark(pos, vel, color, ttl, sparkType);
}


void UIManager::emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors)
{
   getUI<GameUserInterface>()->emitExplosion(pos, size, colorArray, numColors);
}


void UIManager::emitTeleportInEffect(const Point &pos, U32 type)
{
   getUI<GameUserInterface>()->emitTeleportInEffect(pos, type);
}


SFXHandle UIManager::playSoundEffect(U32 profileIndex, F32 gain)
{
   return getUI<GameUserInterface>()->playSoundEffect(profileIndex, gain);
}


SFXHandle UIManager::playSoundEffect(U32 profileIndex, const Point &position)
{
   return getUI<GameUserInterface>()->playSoundEffect(profileIndex, position);
}


SFXHandle UIManager::playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain)
{
   return getUI<GameUserInterface>()->playSoundEffect(profileIndex, position, velocity, gain);
}


void UIManager::playNextTrack()
{
   getUI<GameUserInterface>()->playNextTrack();
}


void UIManager::playPrevTrack()
{
   getUI<GameUserInterface>()->playPrevTrack();
}


void UIManager::queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p)
{
   getUI<GameUserInterface>()->queueVoiceChatBuffer(effect, p);
}


};
