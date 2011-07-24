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

#ifndef _UI_MANAGER_H_
#define _UI_MANAGER_H_

#include <memory>

using namespace std;

namespace Zap
{

class MainMenuUserInterface;
class GameParamUserInterface;
class EditorUserInterface;
class YesNoUserInterface;
class TeamMenuUserInterface;
class TeamDefUserInterface;
class QueryServersUserInterface;
class ServerPasswordEntryUserInterface;
class PlayerMenuUserInterface;
class NameEntryUserInterface;
class MessageUserInterface;
class LevelMenuUserInterface;
class LevelMenuSelectUserInterface;
class AdminPasswordEntryUserInterface;
class LevelChangePasswordEntryUserInterface;
class HostMenuUserInterface;
class GameMenuUserInterface;
class ErrorMessageUserInterface;
class LevelNameEntryUserInterface;
class ServerPasswordEntryUserInterface;
class InstructionsUserInterface;
class OptionsMenuUserInterface;
class KeyDefMenuUserInterface;
class DiagnosticUserInterface;
class CreditsUserInterface;
class EditorInstructionsUserInterface;
class ChatUserInterface; 
class SuspendedUserInterface;
class EditorMenuUserInterface;
class SplashUserInterface;
class TeamDefUserInterface;

class Game;


class UIManager
{

private:
   Game *mGame;

   auto_ptr<MainMenuUserInterface> mMainMenuUserInterface;
   auto_ptr<GameParamUserInterface> mGameParamUserInterface;
   auto_ptr<YesNoUserInterface> mYesNoUserInterface;
   auto_ptr<TeamMenuUserInterface> mTeamMenuUserInterface;
   auto_ptr<QueryServersUserInterface> mQueryServersUserInterface;
   auto_ptr<ServerPasswordEntryUserInterface> mServerPasswordEntryUserInterface;
   auto_ptr<PlayerMenuUserInterface> mPlayerMenuUserInterface;
   auto_ptr<NameEntryUserInterface> mNameEntryUserInterface;
   auto_ptr<MessageUserInterface> mMessageUserInterface;
   auto_ptr<LevelMenuUserInterface> mLevelMenuUserInterface;
   auto_ptr<LevelMenuSelectUserInterface> mLevelMenuSelectUserInterface;
   auto_ptr<AdminPasswordEntryUserInterface> mAdminPasswordEntryUserInterface;
   auto_ptr<LevelChangePasswordEntryUserInterface> mLevelChangePasswordEntryUserInterface;
   auto_ptr<HostMenuUserInterface> mHostMenuUserInterface;
   auto_ptr<GameMenuUserInterface> mGameMenuUserInterface;
   auto_ptr<ErrorMessageUserInterface> mErrorMsgUserInterface;
   auto_ptr<LevelNameEntryUserInterface> mLevelNameEntryUserInterface;
   auto_ptr<InstructionsUserInterface> mInstructionsUserInterface;
   auto_ptr<OptionsMenuUserInterface> mOptionsMenuUserInterface;
   auto_ptr<KeyDefMenuUserInterface> mKeyDefMenuUserInterface;
   auto_ptr<DiagnosticUserInterface> mDiagnosticUserInterface;
   auto_ptr<CreditsUserInterface> mCreditsUserInterface;
   auto_ptr<EditorInstructionsUserInterface> mEditorInstructionsUserInterface;
   auto_ptr<ChatUserInterface> mChatInterface;
   auto_ptr<SuspendedUserInterface> mSuspendedUserInterface;
   auto_ptr<EditorMenuUserInterface> mEditorMenuUserInterface;
   auto_ptr<SplashUserInterface> mSplashUserInterface;
   auto_ptr<TeamDefUserInterface> mTeamDefUserInterface;

   static auto_ptr<EditorUserInterface> mEditorUserInterface;

public:
   UIManager(Game *game);     // Constructor

   /////
   // Interface getting methods
   MainMenuUserInterface *getMainMenuUserInterface();
   EditorUserInterface *getEditorUserInterface();
   GameParamUserInterface *getGameParamUserInterface();
   YesNoUserInterface *getYesNoUserInterface();
   TeamMenuUserInterface *getTeamMenuUserInterface();
   TeamDefUserInterface *getTeamDefUserInterface();
   QueryServersUserInterface *getQueryServersUserInterface();
   ServerPasswordEntryUserInterface *getServerPasswordEntryUserInterface();
   PlayerMenuUserInterface *getPlayerMenuUserInterface();
   NameEntryUserInterface *getNameEntryUserInterface();
   MessageUserInterface *getMessageUserInterface();
   LevelMenuUserInterface *getLevelMenuUserInterface();
   LevelMenuSelectUserInterface *getLevelMenuSelectUserInterface();
   AdminPasswordEntryUserInterface *getAdminPasswordEntryUserInterface();
   LevelChangePasswordEntryUserInterface *getLevelChangePasswordEntryUserInterface();
   HostMenuUserInterface *getHostMenuUserInterface();
   GameMenuUserInterface *getGameMenuUserInterface();
   ErrorMessageUserInterface *getErrorMsgUserInterface();
   LevelNameEntryUserInterface *getLevelNameEntryUserInterface();
   InstructionsUserInterface *getInstructionsUserInterface();
   OptionsMenuUserInterface *getOptionsMenuUserInterface();
   KeyDefMenuUserInterface *getKeyDefMenuUserInterface();
   DiagnosticUserInterface *getDiagnosticUserInterface();
   CreditsUserInterface *getCreditsUserInterface();
   EditorInstructionsUserInterface *getEditorInstructionsUserInterface();
   ChatUserInterface *getChatUserInterface();
   SuspendedUserInterface *getSuspendedUserInterface();
   EditorMenuUserInterface *getEditorMenuUserInterface();
   SplashUserInterface *getSplashUserInterface();
};

};

#endif
