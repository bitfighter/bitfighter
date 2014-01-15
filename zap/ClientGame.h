//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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

   static const S32 RATING_NOT_KNOWN = S32_MIN;

private:
   SafePtr<GameConnection> mConnectionToServer; // If this is a client game, this is the connection to the server

   UIManager *mUIManager;

   string mRemoteLevelDownloadFilename;
   bool mShowAllObjectOutlines;     // For debugging purposes

   Vector<string> mMuteList;        // List of players we aren't listening to anymore because they've annoyed us!
   Vector<string> mVoiceMuteList;   // List of players we mute because they are abusing voice chat

   string mEnteredServerPermsPassword;
   string mEnteredServerAccessPassword;

   PersonalRating mPlayerLevelRating;
   PersonalRating mPlayerLevelRatingOrig;
   S16 mTotalLevelRating;
   S16 mTotalLevelRatingOrig;

   string mPreviousLevelName;    // For /prevlevel command

   bool needsRating() const;

   static PersonalRating getNextRating(PersonalRating currentRating);

   // ClientGame has two ClientInfos for the local player; mClientInfo is a FullClientInfo, which contains a rich array of information
   // about the local player.  When a player connects to a server, all players (including the connecting player) are sent a much briefer
   // set of info about the player, which is stored in a RemoteClientInfo.  So when a player connects, they get several RemoteClientInfos,
   // one of which describes themselves.  It is occasionally useful to have a handle on that, which is what mLocalRemoteClientInfo provides.
   RefPtr<ClientInfo> mClientInfo;              // ClientInfo object for local client
   SafePtr<ClientInfo> mLocalRemoteClientInfo;  // ClientInfo pointer to the remote ClientInfo received from the server

   S32 findClientIndex(const StringTableEntry &name);
   S32 getLevelThreshold(S32 val) const;        // Have we earned a levelup message?

   AbstractTeam *getNewTeam();

public:
   ClientGame(const Address &bindAddress, GameSettingsPtr settings, UIManager *uiManager);
   virtual ~ClientGame();

   void joinLocalGame(GameNetInterface *remoteInterface);
   void joinRemoteGame(Address remoteAddress, bool isFromMaster);

   ServerGame *getServerGame() const;

   void closeConnectionToGameServer();

   //UserInterfaceData *mUserInterfaceData;

   bool isConnectedToServer() const;
   bool isConnectedToMaster() const;

   bool isTestServer() const;

   GameConnection *getConnectionToServer() const;
   
   void setConnectionToServer(GameConnection *connection);

   ClientInfo *getClientInfo() const;

   ClientInfo *getLocalRemoteClientInfo() const;

   string getPlayerName() const;
   string getPlayerPassword() const;
   void userEnteredLoginCredentials(const string &name, const string &password, bool savePassword);

   void correctPlayerName(const string &name);                                      // When server corrects capitalization of name or similar

   void displayShipDesignChangedMessage(const LoadoutTracker &loadout, const char *msgToShowIfLoadoutsAreTheSame);
   
   void startLoadingLevel(bool engineerEnabled);
   void doneLoadingLevel();

   void gotTotalLevelRating(S16 rating);
   void gotPlayerLevelRating(S32 rating);

   bool canRateLevel() const;
   void levelIsNotReallyInTheDatabase();

   void setLevelDatabaseId(U32 id);

   string getCurrentLevelFileName() const;
   void setPreviousLevelName(const string &name);
   void showPreviousLevelName() const;


   UIManager *getUIManager() const;

   void toggleShowAllObjectOutlines();
   bool showAllObjectOutlines() const;

   PersonalRating toggleLevelRating();
   S16 getTotalLevelRating() const;
   PersonalRating getPersonalLevelRating() const;
   void updateOriginalRating();
   void restoreOriginalRating();
   void resetRatings();



   bool isLevelInDatabase() const;

   // A place to store input from the joysticks while we are composing our moves
   F32 mJoystickInputs[JoystickAxesDirectionCount];

   void resetCommandersMap();
   F32 getCommanderZoomFraction() const;

   void onGameUIActivated();     // Gets run when the GameUI is first activated (from main menu)
   void onGameStarting();

   void setEnteringGameOverScoreboardPhase();   // Post-game scoreboard is about to be displayed
   void onGameReallyAndTrulyOver();             // Post-game scoreboard has already been displayed

   Point worldToScreenPoint(const Point *p, S32 canvasWidth, S32 canvasHeight) const;

   bool isServer() const;
   void idle(U32 timeDelta);
   void setUsingCommandersMap(bool usingCommandersMap);

   // HelpItem related
   void addInlineHelpItem(HelpItem item) const;
   void addInlineHelpItem(U8 objectType, S32 objectTeam, S32 playerTeam) const;
   void removeInlineHelpItem(HelpItem item, bool markAsSeen) const;
   F32 getObjectiveArrowHighlightAlpha() const;

   void gotServerListFromMaster(const Vector<IPAddress> &serverList);

   // Got some shizzle
   void gotGlobalChatMessage(const char *playerNick, const char *message, bool isPrivate);
   void gotChatMessage(const StringTableEntry &clientName, const StringPtr &message, bool global);
   void gotChatPM(const StringTableEntry &fromName, const StringTableEntry &toName, const StringPtr &message);
   void gotAnnouncement(const string &announcement);
   void gotVoiceChat(const StringTableEntry &from, const ByteBufferPtr &voiceBuffer);

   void gameTypeIsAboutToBeDeleted();
   void activatePlayerMenuUi();
   void renderBasicInterfaceOverlay() const;


   void onPlayerJoined(ClientInfo *clientInfo, bool isLocalClient, bool playAlert, bool showMessage);
   void onPlayerQuit(const StringTableEntry &name);

   void setSpawnDelayed(bool spawnDelayed);
   bool isSpawnDelayed() const;
   void undelaySpawn();

   // Chat related
   void sendChat(bool isGlobal, const StringPtr &message);
   void sendChatSTE(bool global, const StringTableEntry &message) const;
   void sendCommand(const StringTableEntry &cmd, const Vector<StringPtr> &args);
   void setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks);
   void playerJoinedGlobalChat(const StringTableEntry &playerNick);
   void playerLeftGlobalChat(const StringTableEntry &playerNick);


   // Team related
   void changePlayerTeam(const StringTableEntry &playerName, S32 teamIndex) const;
   void changeOwnTeam(S32 teamIndex) const;
   void switchTeams();     // User selected Switch Teams meunu item


   // Some FxManager passthroughs
   void emitBlast(const Point &pos, U32 size);
   void emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2);
   void emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation);
   void emitTextEffect(const string &text, const Color &color, const Point &pos) const;
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

   // Passthroughs to mUi
   void quitEngineerHelper();
   void newLoadoutHasArrived(const LoadoutTracker &loadout);
   void setActiveWeapon(U32 weaponIndex);
   bool isShowingDebugShipCoords();


   void requestSpawnDelayed(bool incursPenalty) const;
   U32 getReturnToGameDelay() const;
   bool inReturnToGameCountdown() const;


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

   void setHighScores(const Vector<StringTableEntry> &groupNames, const Vector<string> &names, const Vector<string> &scores) const;

   string getRemoteLevelDownloadFilename() const;
   void setRemoteLevelDownloadFilename(const string &filename);

   void submitServerAccessPassword(const Address &connectAddress, const char *password);
   bool submitServerPermissionsPassword(const char *password);


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
   void displayMessage(const Color &msgColor, const char *format, ...) const;
   void displayCmdChatMessage(const char *format, ...) const;

   void onConnectedToMaster();
   void onConnectionTerminated(const Address &serverAddress, NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected);
   void onConnectionToMasterTerminated(NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected);

   void runCommand(const char *command);

   string getRequestedServerName();

   void setServerPassword(const string &password);
   string getEnteredServerAccessPassword();

   void displayErrorMessage(const char *format, ...) const;
   void displaySuccessMessage(const char *format, ...) const;

   void suspendGame();
   void unsuspendGame();
   void setGameSuspended_FromServerMessage(bool suspend);

   S32 getBotCount() const;
   GridDatabase *getBotZoneDatabase() const;


   // For loading levels in editor
   bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database, S32 id);

   void addPolyWall(BfObject *polyWall, GridDatabase *database);     // Add polyWall item to game
   void addWallItem(BfObject *wallItem, GridDatabase *database);     // Add wallItem item to game

   Ship *getLocalPlayerShip() const;

   // Settings related
   InputMode getInputMode();

   void setShowingInGameHelp(bool showing);
   void resetInGameHelpMessages();

};


}


#endif

