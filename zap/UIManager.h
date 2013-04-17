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

#include "Timer.h"
#include "tnlVector.h"
#include <memory>

using namespace std;
using namespace TNL;

namespace Zap
{

class ChatUserInterface; 
class CreditsUserInterface;
class DiagnosticUserInterface;
class EditorInstructionsUserInterface;
class EditorMenuUserInterface;
class EditorUserInterface;
class ErrorMessageUserInterface;
class GameMenuUserInterface;
class GameParamUserInterface;
class GameUserInterface;
class HighScoresUserInterface;
class HostMenuUserInterface;
class InputOptionsMenuUserInterface;
class InstructionsUserInterface;
class KeyDefMenuUserInterface;
class LevelChangeOrAdminPasswordEntryUserInterface;
class LevelMenuSelectUserInterface;
class LevelMenuUserInterface;
class LevelNameEntryUserInterface;
class MainMenuUserInterface;
class MessageUserInterface;
class NameEntryUserInterface;
class OptionsMenuUserInterface;
class PlayerMenuUserInterface;
class SoundOptionsMenuUserInterface;
class QueryServersUserInterface;
class ServerPasswordEntryUserInterface;
class ServerPasswordEntryUserInterface;
class SplashUserInterface;
class SuspendedUserInterface;
class TeamDefUserInterface;
class TeamDefUserInterface;
class TeamMenuUserInterface;
class UserInterface;
class YesNoUserInterface;

class Game;
class ClientGame;

enum UIID {
   CreditsUI,
   DiagnosticsScreenUI,
   EditorInstructionsUI,
   EditorUI,
   EditorMenuUI,        // <=== Don't confuse this with EditorUI!!!
   ErrorMessageUI,
   GameMenuUI,
   GameParamsUI,
   GameUI,
   GenericUI,
   GlobalChatUI,
   SuspendedUI,
   HighScoresUI,
   HostingUI,
   InputOptionsUI,
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
   PlayerUI,
   TeamUI,
   QueryServersScreenUI,
   SoundOptionsUI,
   SplashUI,
   TeamDefUI,
   TextEntryUI,
   YesOrNoUI,
   InvalidUI        // Not a valid UI
};

class UIManager
{

private:
   ClientGame *mGame;
   UserInterface                                *mCurrentInterface;

   ChatUserInterface                            *mChatInterface;
   CreditsUserInterface                         *mCreditsUserInterface;
   DiagnosticUserInterface                      *mDiagnosticUserInterface;
   EditorInstructionsUserInterface              *mEditorInstructionsUserInterface;
   EditorMenuUserInterface                      *mEditorMenuUserInterface;
   EditorUserInterface                          *mEditorUserInterface;
   ErrorMessageUserInterface                    *mErrorMsgUserInterface;
   GameMenuUserInterface                        *mGameMenuUserInterface;
   GameParamUserInterface                       *mGameParamUserInterface;
   GameUserInterface                            *mGameUserInterface;
   HighScoresUserInterface                      *mHighScoresUserInterface;
   HostMenuUserInterface                        *mHostMenuUserInterface;
   InputOptionsMenuUserInterface                *mInputOptionsUserInterface;
   InstructionsUserInterface                    *mInstructionsUserInterface;
   KeyDefMenuUserInterface                      *mKeyDefMenuUserInterface;
   LevelChangeOrAdminPasswordEntryUserInterface *mLevelChangeOrAdminPasswordEntryUserInterface;
   LevelMenuSelectUserInterface                 *mLevelMenuSelectUserInterface;
   LevelMenuUserInterface                       *mLevelMenuUserInterface;
   LevelNameEntryUserInterface                  *mLevelNameEntryUserInterface;
   MainMenuUserInterface                        *mMainMenuUserInterface;
   MessageUserInterface                         *mMessageUserInterface;
   NameEntryUserInterface                       *mNameEntryUserInterface;
   OptionsMenuUserInterface                     *mOptionsMenuUserInterface;
   PlayerMenuUserInterface                      *mPlayerMenuUserInterface;
   SoundOptionsMenuUserInterface                *mSoundOptionsMenuUserInterface;
   QueryServersUserInterface                    *mQueryServersUserInterface;
   ServerPasswordEntryUserInterface             *mServerPasswordEntryUserInterface;
   SplashUserInterface                          *mSplashUserInterface;
   SuspendedUserInterface                       *mSuspendedUserInterface;
   TeamDefUserInterface                         *mTeamDefUserInterface;
   TeamMenuUserInterface                        *mTeamMenuUserInterface;
   YesNoUserInterface                           *mYesNoUserInterface;

   Vector<UserInterface *> mPrevUIs;   // Previously active menus

   UserInterface *mLastUI;             // Menu most immediately active before mCurrentUI
   bool mLastWasLower;                 // True if mLastUI was lower in the hierarchy than mCurrentUI

   Timer mMenuTransitionTimer;

public:
   explicit UIManager(ClientGame *clientGame);  // Constructor
   virtual ~UIManager();                        // Destructor

   bool isCurrentUI(UIID uiid);
   UserInterface *getUI(UIID menuId);


   /////
   // Interface getting methods
   MainMenuUserInterface                        *getMainMenuUserInterface();

   ChatUserInterface                            *getChatUserInterface();
   CreditsUserInterface                         *getCreditsUserInterface();
   DiagnosticUserInterface                      *getDiagnosticUserInterface();
   EditorInstructionsUserInterface              *getEditorInstructionsUserInterface();
   EditorMenuUserInterface                      *getEditorMenuUserInterface();
   EditorUserInterface                          *getEditorUserInterface();
   ErrorMessageUserInterface                    *getErrorMsgUserInterface();
   GameMenuUserInterface                        *getGameMenuUserInterface();
   GameParamUserInterface                       *getGameParamUserInterface();
   GameUserInterface                            *getGameUserInterface();
   HighScoresUserInterface                      *getHighScoresUserInterface();
   HostMenuUserInterface                        *getHostMenuUserInterface();
   InputOptionsMenuUserInterface                *getInputOptionsUserInterface();
   InstructionsUserInterface                    *getInstructionsUserInterface();
   KeyDefMenuUserInterface                      *getKeyDefMenuUserInterface();
   LevelChangeOrAdminPasswordEntryUserInterface *getLevelChangeOrAdminPasswordEntryUserInterface();
   LevelMenuSelectUserInterface                 *getLevelMenuSelectUserInterface();
   LevelMenuUserInterface                       *getLevelMenuUserInterface();
   LevelNameEntryUserInterface                  *getLevelNameEntryUserInterface();
   MessageUserInterface                         *getMessageUserInterface();
   NameEntryUserInterface                       *getNameEntryUserInterface();
   OptionsMenuUserInterface                     *getOptionsMenuUserInterface();
   PlayerMenuUserInterface                      *getPlayerMenuUserInterface();
   QueryServersUserInterface                    *getQueryServersUserInterface();
   ServerPasswordEntryUserInterface             *getServerPasswordEntryUserInterface();
   SoundOptionsMenuUserInterface                *getSoundOptionsMenuUserInterface();
   SplashUserInterface                          *getSplashUserInterface();
   SuspendedUserInterface                       *getSuspendedUserInterface();
   TeamDefUserInterface                         *getTeamDefUserInterface();
   TeamMenuUserInterface                        *getTeamMenuUserInterface();
   YesNoUserInterface                           *getYesNoUserInterface();

   void reactivatePrevUI();
   void reactivate(UIID menuId);
   bool hasPrevUI();
   void clearPrevUIs();
   void renderPrevUI(const UserInterface *ui);
   bool cameFrom(UIID menuID);        // Did we arrive at our current interface via the specified interface?
   void saveUI(UserInterface *ui);

   void renderCurrent();
   UserInterface *getCurrentUI();
   UserInterface *getPrevUI();
   void activate(UIID menuID, bool save = true);
   void activate(UserInterface *, bool save = true);

   void idle(U32 timeDelta);
};

};

#endif

