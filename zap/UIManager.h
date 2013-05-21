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

#include "SoundEffect.h"

#include "SoundSystemEnums.h"
#include "SparkTypesEnum.h"
#include "PlayerActionEnum.h"

#include "HelpItemManager.h"     // For HelpItem def
#include "LoadoutTracker.h"

#include "Color.h"
#include "Point.h"
#include "Timer.h"

#include "tnlVector.h"
#include "tnlNetStringTable.h"
#include "tnlNetConnection.h"
#include "tnlUDP.h"
#include "tnlNonce.h"

#include <memory>

using namespace std;
using namespace TNL;

namespace Zap
{

class Game;
class ClientGame;
class GameSettings;
class UserInterface;


class UIManager
{

private:
   ClientGame *mGame;
   GameSettings *mSettings;
   bool mUserHasSeenTimeoutMessage;

   UserInterface *mCurrentInterface;

   Vector<UserInterface *> mPrevUIs;   // Previously active menus

   UserInterface *mLastUI;             // Menu most immediately active before mCurrentUI
   bool mLastWasLower;                 // True if mLastUI was lower in the hierarchy than mCurrentUI

   Timer mMenuTransitionTimer;

   // Sounds
   MusicLocation selectMusic();
   void processAudio(U32 timeDelta);


public:
   explicit UIManager(ClientGame *clientGame);  // Constructor
   virtual ~UIManager();                        // Destructor


   template <typename T>
   T *getUI()
   {
      static T ui(mGame);        // Uses lazy initialization

      return &ui;
   }


   template <typename T>
   bool isCurrentUI()
   {
      return mCurrentInterface == getUI<T>();
   }


   // Did we arrive at our current interface via the specified interface?
   template <typename T>
   bool cameFrom()
   {
      for(S32 i = 0; i < mPrevUIs.size(); i++)
         if(mPrevUIs[i] == getUI<T>())
            return true;

      return false;
   }

 
   void reactivatePrevUI();
   void reactivate(const UserInterface *ui);

   bool hasPrevUI();
   void clearPrevUIs();
   void renderPrevUI(const UserInterface *ui);


   void saveUI(UserInterface *ui);

   void renderCurrent();
   UserInterface *getCurrentUI();
   UserInterface *getPrevUI();

   template <typename T>
   void activate(bool save = true)
   {
      activate(getUI<T>(), save);
   }

   void activate(UserInterface *ui, bool save = true);

   void idle(U32 timeDelta);

   void renderAndDimGameUserInterface();

   // Connecting and disconnecting
   void onConnectedToMaster();
   void onConnectionToMasterTerminated(NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected);
   void onConnectionTerminated(const Address &serverAddress, NetConnection::TerminationReason reason, const char *reasonStr);
   void onConnectionToServerRejected(const char *reason);

   void onPlayerJoined(const char *playerName, bool isLocalClient, bool playAlert, bool showMessage);
   void onPlayerQuit(const char *name);
   void displayMessage(const Color &msgColor, const char *format, ...);

   void onGameOver();

   // Sounds and music
   SFXHandle playSoundEffect(U32 profileIndex, const Point &position) const;
   SFXHandle playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain) const;
   SFXHandle playSoundEffect(U32 profileIndex, F32 gain) const;
   void setMovementParams(SFXHandle& effect, const Point &position, const Point &velocity) const;
   void stopSoundEffect(SFXHandle &effect) const;
   void setListenerParams(const Point &position, const Point &velocity) const;
   void playNextTrack() const;
   void playPrevTrack() const;
   void queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p) const;


   // Some passthroughs

   // HighScores UI:
   void setHighScores(const Vector<StringTableEntry> &groupNames, const Vector<string> &names, const Vector<string> &scores);

   // ChatUI:
   void gotGlobalChatMessage(const string &from, const string &message, bool isPrivate, bool isSystem, bool fromSelf);
   void setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks);
   void playerJoinedGlobalChat(const StringTableEntry &playerNick);
   void playerLeftGlobalChat  (const StringTableEntry &playerNick);

   // QueryServersUI:
   void gotServerListFromMaster(const Vector<IPAddress> &serverList);
   void gotPingResponse (const Address &address, const Nonce &nonce, U32 clientIdentityToken);
   void gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                         U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired);
   string getLastSelectedServerName();

   // ServerPasswordUI
   void setConnectAddressAndActivatePasswordEntryUI(const Address &serverAddress);

   // GameUI
   void renderLevelListDisplayer();
   void enableLevelLoadDisplay();
   void serverLoadedLevel(const string &levelName);
   void disableLevelLoadDisplay(bool fade);
   void gotPasswordOrPermissionsReply(const ClientGame *game, const char *message);

   // MainMenuUi
   void setMOTD(const char *motd);
   void setNeedToUpgrade(bool needToUpgrade);

   // PlayerUI
   void showPlayerActionMenu(PlayerAction action);

   // TeamMenuUI
   void showMenuToChangeTeamForPlayer(const string &playerName);

   // ErrorMessageUI
   void displayMessageBox(const StringTableEntry &title, const StringTableEntry &instr, const Vector<StringTableEntry> &messages);
   void displayMessageBox(const char *title, const char *instr, const Vector<string> &messages);

   // GameUI
   void startLoadingLevel(F32 lx, F32 ly, F32 ux, F32 uy, bool engineerEnabled);
   void doneLoadingLevel();
   void clearSparks();
   void emitBlast(const Point &pos, U32 size);
   void emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2);
   void emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation);
   void emitTextEffect(const string &text, const Color &color, const Point &pos);
   void emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, UI::SparkType sparkType);
   void emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors);
   void emitTeleportInEffect(const Point &pos, U32 type);
   void addInlineHelpItem(HelpItem item);
   void onChatMessageReceived(const Color &msgColor, const char *format, ...);
   void gotAnnouncement(const string &announcement);
   bool isInScoreboardMode();
   void newLoadoutHasArrived(const LoadoutTracker &loadout);
   void setActiveWeapon(U32 weaponIndex);
   bool isShowingDebugShipCoords();

   // EditorUI
   void readRobotLine(const string &robotLine);

};

};

#endif

