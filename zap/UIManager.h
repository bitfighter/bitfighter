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

#include "tnlVector.h"
#include <memory>

using namespace std;
using namespace TNL;

namespace Zap
{

class UserInterface;
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

enum UIID {
   AdminPasswordEntryUI,
   ChatUI,
   CreditsUI,
   DiagnosticsScreenUI,
   EditorInstructionsUI,
   EditorUI,
   EditorMenuUI,
   ErrorMessageUI,
   GameMenuUI,
   GameParamsUI,
   GameUI,
   GenericUI,
   GlobalChatUI,
   SuspendedUI,
   HostingUI,
   InstructionsUI,
   KeyDefUI,
   LevelUI,
   LevelNameEntryUI,
   LevelChangePasswordEntryUI,
   LevelTypeUI,
   MainUI,
   NameEntryUI,
   OptionsUI,
   PasswordEntryUI,
   ReservedNamePasswordEntryUI,
   PlayerUI,
   TeamUI,
   QueryServersScreenUI,
   SplashUI,
   TeamDefUI,
   TextEntryUI,
   YesOrNoUI,
   GoFastAttributeEditorUI,
   TextItemAttributeEditorUI,
   InvalidUI,        // Not a valid UI
};

class UIManager
{

private:
   Game *mGame;

   MainMenuUserInterface *mMainMenuUserInterface;
   GameParamUserInterface *mGameParamUserInterface;
   YesNoUserInterface *mYesNoUserInterface;
   TeamMenuUserInterface *mTeamMenuUserInterface;
   QueryServersUserInterface *mQueryServersUserInterface;
   ServerPasswordEntryUserInterface *mServerPasswordEntryUserInterface;
   PlayerMenuUserInterface *mPlayerMenuUserInterface;
   NameEntryUserInterface *mNameEntryUserInterface;
   MessageUserInterface *mMessageUserInterface;
   LevelMenuUserInterface *mLevelMenuUserInterface;
   LevelMenuSelectUserInterface *mLevelMenuSelectUserInterface;
   AdminPasswordEntryUserInterface *mAdminPasswordEntryUserInterface;
   LevelChangePasswordEntryUserInterface *mLevelChangePasswordEntryUserInterface;
   HostMenuUserInterface *mHostMenuUserInterface;
   GameMenuUserInterface *mGameMenuUserInterface;
   ErrorMessageUserInterface *mErrorMsgUserInterface;
   InstructionsUserInterface *mInstructionsUserInterface;
   OptionsMenuUserInterface *mOptionsMenuUserInterface;
   KeyDefMenuUserInterface *mKeyDefMenuUserInterface;
   DiagnosticUserInterface *mDiagnosticUserInterface;
   CreditsUserInterface *mCreditsUserInterface;
   EditorInstructionsUserInterface *mEditorInstructionsUserInterface;
   ChatUserInterface *mChatInterface;
   SuspendedUserInterface *mSuspendedUserInterface;
   EditorMenuUserInterface *mEditorMenuUserInterface;
   SplashUserInterface *mSplashUserInterface;
   TeamDefUserInterface *mTeamDefUserInterface;
   LevelNameEntryUserInterface *mLevelNameEntryUserInterface;
   EditorUserInterface *mEditorUserInterface;

   Vector<UserInterface *> mPrevUIs;   // Previously active menus


public:
   UIManager(Game *game);     // Constructor
   ~UIManager();              // Destructor

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

   void reactivatePrevUI();
   void reactivateMenu(const UserInterface *target);
   UserInterface *getPrevUI();
   bool hasPrevUI() { return mPrevUIs.size() > 0; }
   void clearPrevUIs() { mPrevUIs.clear(); }
   void renderPrevUI();
   bool cameFrom(UIID menuID);        // Did we arrive at our current interface via the specified interface?
   void saveUI(UserInterface *ui);
};

};

#endif
