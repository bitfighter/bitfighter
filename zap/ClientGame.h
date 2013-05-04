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

#ifndef _CLIENTGAME_H_
#define _CLIENTGAME_H_

#ifdef ZAP_DEDICATED
#  error "ClientGame.h shouldn't be included in dedicated build"
#endif

#include "game.h"
#include "HelpItemManager.h"     // For HelpItems enum
#include "SoundEffect.h"         // For SFXHandle def
#include "Engineerable.h"        // For EngineerResponseEvent enum
#include "ClientInfo.h"          // For ClientRole enum
#include "SparkTypesEnum.h"
#include "gameConnection.h"


#ifdef TNL_OS_WIN32
#  include <windows.h>           // For screensaver... windows only feature, I'm afraid!
#endif


using namespace std;

namespace Zap
{


class ClientGame : public Game
{
   typedef Game Parent;

private:
   SafePtr<GameConnection> mConnectionToServer; // If this is a client game, this is the connection to the server

   bool mInCommanderMap;
   Timer mCommanderZoomDelta;

   Timer mScreenSaverTimer;
   void supressScreensaver();

   UIManager *mUIManager;

   string mRemoteLevelDownloadFilename;

   Vector<string> mMuteList;        // List of players we aren't listening to anymore because they've annoyed us!
   Vector<string> mVoiceMuteList;   // List of players we mute because they are abusing voice chat

   bool mGameIsRunning;             // True if a suspended game is being played without us, false if it's full stop for everyone

   bool mSeenTimeOutMessage;

   GameUserInterface *mUi;

   SFXHandle mModuleSound[ModuleCount];

   // ClientGame has two ClientInfos for the local player; mClientInfo is a FullClientInfo, which contains a rich array of information
   // about the local player.  When a player connects to a server, all players (including the connecting player) are sent a much briefer
   // set of info about the player, which is stored in a RemoteClientInfo.  So when a player connects, they get several RemoteClientInfos,
   // one of which describes themselves.  It is occasionally useful to have a handle on that, which is what mLocalRemoteClientInfo provides.
   RefPtr<ClientInfo> mClientInfo;              // ClientInfo object for local client
   SafePtr<ClientInfo> mLocalRemoteClientInfo;  // ClientInfo pointer to the remote ClientInfo received from the server

   S32 findClientIndex(const StringTableEntry &name);

   AbstractTeam *getNewTeam();

   Vector<string> mRobots;                      // A list of robots read in from a level file when loading from the editor

public:
   ClientGame(const Address &bindAddress, GameSettings *settings);
   virtual ~ClientGame();

   void joinLocalGame(GameNetInterface *remoteInterface);
   void joinRemoteGame(Address remoteAddress, bool isFromMaster);

   void closeConnectionToGameServer();

   UserInterfaceData *mUserInterfaceData;

   bool isConnectedToServer() const;

   GameConnection *getConnectionToServer() const;
   void setConnectionToServer(GameConnection *connection);

   ClientInfo *getClientInfo() const;

   ClientInfo *getLocalRemoteClientInfo() const;
   Ship *getLocalShip() const;

   string getPlayerName() const;
   string getPlayerPassword() const;
   void userEnteredLoginCredentials(const string &name, const string &password, bool savePassword);

   void correctPlayerName(const string &name);                                      // When server corrects capitalization of name or similar

   void displayShipDesignChangedMessage(const LoadoutTracker &loadout, const char *msgToShowIfLoadoutsAreTheSame);
   
   void startLoadingLevel(F32 lx, F32 ly, F32 ux, F32 uy, bool engineerEnabled);
   void doneLoadingLevel();

   void switchTeams();     // User selected Switch Teams meunu item

   UIManager *getUIManager() const;
   GameUserInterface *getUi() const;

   // A place to store input from the joysticks while we are composing our moves
   F32 mJoystickInputs[JoystickAxesDirectionCount];

   bool getInCommanderMap();
   void setInCommanderMap(bool inCommanderMap);
   F32 getCommanderZoomFraction() const;

   void setGameOver();     // Post-game scoreboard is about to be displayed
   void onGameOver();      // Post-game scoreboard has already been displayed

   Point worldToScreenPoint(const Point *p, S32 canvasWidth, S32 canvasHeight) const;

   void render();          // Delegates to renderNormal, renderCommander, or renderSuspended, as appropriate

   bool isServer();
   void idle(U32 timeDelta);
   void toggleCommanderMap();

   Ship *findShip(const StringTableEntry &clientName);

   void addHelpItem(HelpItem item);

   void gotServerListFromMaster(const Vector<IPAddress> &serverList);

   // Got some shizzle
   void gotGlobalChatMessage(const char *playerNick, const char *message, bool isPrivate);
   void gotChatMessage(const StringTableEntry &clientName, const StringPtr &message, bool global);
   void gotChatPM(const StringTableEntry &fromName, const StringTableEntry &toName, const StringPtr &message);
   void gotAnnouncement(const string &announcement);
   void gotVoiceChat(const StringTableEntry &from, const ByteBufferPtr &voiceBuffer);

   void gameTypeIsAboutToBeDeleted();
   void activatePlayerMenuUi();
   void renderBasicInterfaceOverlay(bool scoreboardVisible);


   void onPlayerJoined(ClientInfo *clientInfo, bool isLocalClient, bool playAlert, bool showMessage);
   void onPlayerQuit(const StringTableEntry &name);

   void setSpawnDelayed(bool spawnDelayed);
   bool isSpawnDelayed() const;
   void undelaySpawn();
   F32 getUIFadeFactor() const;


   // Chat related
   void sendChat(bool isGlobal, const StringPtr &message);
   void sendCommand(const StringTableEntry &cmd, const Vector<StringPtr> &args);
   void setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks);
   void playerJoinedGlobalChat(const StringTableEntry &playerNick);
   void playerLeftGlobalChat(const StringTableEntry &playerNick);

   // Some FxManager passthroughs
   void clearSparks();
   void emitBlast(const Point &pos, U32 size);
   void emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2);
   void emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation);
   void emitTextEffect(const string &text, const Color &color, const Point &pos);
   void emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, UI::SparkType sparkType);
   void emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors);
   void emitTeleportInEffect(const Point &pos, U32 type);
   void emitShipExplosion(const Point &pos);

   // Sound some related passthroughs
   SFXHandle playSoundEffect(U32 profileIndex, F32 gain = 1.0f) const;
   SFXHandle playSoundEffect(U32 profileIndex, const Point &position) const;
   SFXHandle playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain = 1.0f) const;
   void playNextTrack() const;
   void playPrevTrack() const;
   void queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p) const;

   void updateModuleSounds(const Point &pos, const Point &vel, const LoadoutTracker &loadout);

   // Passthroughs to mUi
   void quitEngineerHelper();
   void newLoadoutHasArrived(const LoadoutTracker &loadout);
   void setActiveWeapon(U32 weaponIndex);
   bool isShowingDebugShipCoords();


   void requestSpawnDelayed();
   U32 getReturnToGameDelay() const;

   void deleteLevelGen(LuaLevelGenerator *levelgen); 

   // Check for permissions
   bool hasOwner(const char *failureMessage);
   bool hasAdmin(const char *failureMessage);
   bool hasLevelChange(const char *failureMessage);

   void gotEngineerResponseEvent(EngineerResponseEvent event);
   void setBusyChatting(bool busy);       // Tell the server we are (or are not) busy chatting

   void addToMuteList(const string &name);
   void removeFromMuteList(const string &name);
   bool isOnMuteList(const string &name);

   void addToVoiceMuteList(const string &name);
   void removeFromVoiceMuteList(const string &name);
   bool isOnVoiceMuteList(const string &name);

   void connectionToServerRejected(const char *reason);
   void setMOTD(const char *motd);
   void setNeedToUpgrade(bool needToUpgrade);

   void setHighScores(Vector<StringTableEntry> groupNames, Vector<string> names, Vector<string> scores);

   string getRemoteLevelDownloadFilename() const;
   void setRemoteLevelDownloadFilename(const string &filename);

   void changePassword(GameConnection::ParamType type, const Vector<string> &words, bool required);
   void changeServerParam(GameConnection::ParamType type, const Vector<string> &words);
   bool checkName(const string &name);    // Make sure name is valid, and correct case of name if otherwise correct

   // Alert users when they get a reply to their request for elevated permissions
   void gotPermissionsReply(ClientInfo::ClientRole role);
   void gotWrongPassword();

   void gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken);
   void gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                         U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired);

   void shutdownInitiated(U16 time, const StringTableEntry &name, const StringPtr &reason, bool originator);
   void cancelShutdown();

   void displayMessageBox(const StringTableEntry &title, const StringTableEntry &instr, const Vector<StringTableEntry> &message) const;
   void displayMessage(const Color &msgColor, const char *format, ...);

   void onConnectedToMaster();
   void onConnectionTerminated(const Address &serverAddress, NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected);
   void onConnectionToMasterTerminated(NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected);

   void runCommand(const char *command);

   string getRequestedServerName();
   string getServerPassword();
   string getHashedServerPassword();

   void displayErrorMessage(const char *format, ...);
   void displaySuccessMessage(const char *format, ...);

   void suspendGame(bool gameIsRunning);
   void unsuspendGame();

   // For loading levels in editor
   bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database, S32 id);
   void addPolyWall(PolyWall *polyWall, GridDatabase *database);                          // Add polyWall item to game
   void addWallItem(WallItem *wallItem, GridDatabase *database);                          // Add wallItem item to game

   void setSelectedEngineeredObject(U32 objectType);

   const Vector<string> *getLevelRobotLines() const;
};

////////////////////////////////////////
////////////////////////////////////////


extern ClientGame *gClientGame;

};


#endif

