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

#ifndef _GAME_H_
#define _GAME_H_

#include "gridDB.h"
#include "Timer.h"
#include "gameLoader.h"
#include "Rect.h"
#include "Colors.h"
#include "shipItems.h"           // For moduleInfos
#include "EditorObject.h"        // For NO_TEAM
#include "dataConnection.h"      // For DataSender
#include "SharedConstants.h"     // For badges enum
#include "statistics.h"          // For Statistics def

#include "GameTypesEnum.h"

#include "GameSettings.h"

#include "tnlNetObject.h"
#include "tnlTypes.h"
#include "tnlThread.h"

#include "boost/smart_ptr/shared_ptr.hpp"

#include <string>

///
/// Bitfighter - a 2D space game demonstrating the full capabilities of the
/// Torque Network Library.
///
/// The Bitfighter example game is a 2D vector-graphics game that utilizes
/// some of the more advanced features of the TNL.  Bitfighter also demonstrates
/// the use of client-side prediction, and interpolation to present
/// a consistent simulation to clients over a connection with perceptible
/// latency.
///
/// Bitfighter can run in 3 modes - as a client, a client and server, or a dedicated
/// server.  The dedicated server option is available only as a launch
/// parameter from the command line.
///
/// If it is run as a client, Bitfighter uses the GLUT library to perform
/// cross-platform window intialization, event processing and OpenGL setup.
///
/// Bitfighter implements a simple game framework.  The GameObject class is
/// the root class for all of the various objects in the Bitfighter world, including
/// Ship, Barrier and Projectile instances.  The Game class, which is instanced
/// once for the client and once for the server, manages the current
/// list of GameObject instances.
///
/// Bitfighter clients can connect to servers directly that are on the same LAN
/// or for which the IP address is known.  Bitfighter is also capable of talking
/// to the TNL master server and using its arranged connection functionality
/// to talk to servers.
///
/// The simplified user interface for Bitfighter is managed entirely through
/// subclasses of the UserInterface class.  Each UserInterface subclass
/// represents one "screen" in the UI.  The GameUserInterface is the most complicated,
/// being responsible for the user interface while the client is actually
/// playing a game.  The only other somewhat complicated UI is the
/// QueryServersUserInterface class, which implements a full server browser
/// for choosing from a list of LAN and master server queried servers.
///

using namespace std;

namespace Zap
{

const U32 MAX_GAME_NAME_LEN = 32;     // Any longer, and it won't fit on-screen
const U32 MAX_GAME_DESCR_LEN = 60;    // Any longer, and it won't fit on-screen; also limits max length of credits string


////////////////////////////////////////
////////////////////////////////////////

class GameConnection;
class SoundEffect;
class VoiceDecoder;
class LuaPlayerInfo;
class Ship;

// This object only concerns itself with things that one client tracks about another.  We use it for other purposes, of course, 
// as a convenient strucure for holding certain settings about the local client, or about remote clients when we are running on the server.
// But the general scope of what we track should be limited; other items should be stored directly on the GameConnection object itself.
class ClientInfo : public Object  // using "Object" will be needed for GameObject's "SafePtr"
{
private:
   LuaPlayerInfo *mPlayerInfo;   // Lua access to this class
   Statistics mStatistics;       // Statistics tracker
   SafePtr<Ship> mShip;          // SafePtr will return NULL if ship object is deleted
   Vector<U32> mLoadout;
   bool mNeedToCheckAuthenticationWithMaster;

protected:
   StringTableEntry mName;
   S32 mScore;
   S32 mTotalScore;
   Nonce mId;
   S32 mTeamIndex;
   S32 mPing;
   bool mIsLevelChanger;
   bool mIsAdmin;
   bool mIsRobot;
   bool mIsAuthenticated;
   Int<BADGE_COUNT> mBadges;

public:
   ClientInfo();           // Constructor
   virtual ~ClientInfo();  // Destructor

   virtual GameConnection *getConnection() = 0;
   virtual void setConnection(GameConnection *conn) = 0;

   const StringTableEntry getName();
   void setName(const StringTableEntry &name);

   S32 getScore();
   void setScore(S32 score);
   void addScore(S32 score);

   const Vector<U32> &getLoadout();
   Timer respawnTimer;

   void resetLoadout();
   Vector<U32> mOldLoadout;      // Server: to respawn with old loadout  Client: to check if using same loadout configuration
   void sRequestLoadout(Vector<U32> &loadout);

   void setNeedToCheckAuthenticationWithMaster(bool needToCheck);
   bool getNeedToCheckAuthenticationWithMaster();

   bool isSpawnDelayed();

   Ship *getShip();
   void setShip(Ship *ship);

   virtual void setRating(F32 rating) = 0;
   virtual F32 getRating() = 0;
   F32 getCalculatedRating();
   void endOfGameScoringHandler();     // Resets stats and the like


   S32 getPing();
   void setPing(S32 ping);

   S32 getTeamIndex();
   void setTeamIndex(S32 teamIndex);

   virtual void setAuthenticated(bool isAuthenticated, Int<BADGE_COUNT> badges);
   bool isAuthenticated();
   Int<BADGE_COUNT> getBadges();

   bool isLevelChanger();
   void setIsLevelChanger(bool isLevelChanger);

   bool isAdmin();
   void setIsAdmin(bool isAdmin);

   bool isRobot();

   Statistics *getStatistics();      // Return pointer to statistics tracker 

   LuaPlayerInfo *getPlayerInfo();

   bool sEngineerDeployObject(U32 type);      // Player using engineer module, robots use this, bypassing the net interface. True if successful.


   Nonce *getId();

   virtual SoundEffect *getVoiceSFX() = 0;
   virtual VoiceDecoder *getVoiceDecoder() = 0;
};

////////////////////////////////////////
////////////////////////////////////////

class GameConnection;

class FullClientInfo : public ClientInfo
{
   typedef ClientInfo Parent;

private:
   GameConnection *mClientConnection;
   
public:
   FullClientInfo(GameConnection *clientConnection, bool isRobot);     // Constructor
   virtual ~FullClientInfo();                                          // Destructor


   // WARNING!! mClientConnection can be NULL on client!!! (though should never be NULL on server)
   GameConnection *getConnection();
   void setConnection(GameConnection *conn);

   void setAuthenticated(bool isAuthenticated, Int<BADGE_COUNT> badges);

   void setRating(F32 rating);
   F32 getRating();

   SoundEffect *getVoiceSFX();
   VoiceDecoder *getVoiceDecoder();
};


////////////////////////////////////////
////////////////////////////////////////
// RemoteClientInfo is used on the client side to track information about other players; these other players do not have a connection
// to us -- all the information we know about them is located on this RemoteClientInfo object.
#ifndef ZAP_DEDICATED
class RemoteClientInfo : public ClientInfo
{
   typedef ClientInfo Parent;

private:
   F32 mRating;      // Ratings are provided by the server and stored here

   // For voice chat
   RefPtr<SoundEffect> mVoiceSFX;
   VoiceDecoder *mDecoder;

public:
   RemoteClientInfo(const StringTableEntry &name, bool isAuthenticated, Int<BADGE_COUNT> badges, bool isRobot, bool isAdmin);  // Constructor
   virtual ~RemoteClientInfo();                                                                       // Destructor

   GameConnection *getConnection();
   void setConnection(GameConnection *conn);

   F32 getRating();
   void setRating(F32 rating);

   void initialize();

   // Voice chat stuff -- these will be invalid on the server side
   SoundEffect *getVoiceSFX();
   VoiceDecoder *getVoiceDecoder();
};
#endif


////////////////////////////////////////
////////////////////////////////////////

// Some forward declarations
class MasterServerConnection;
class GameNetInterface;
class GameType;
class BfObject;
class GameObject;
class GameConnection;
class Ship;
class GameUserInterface;
struct UserInterfaceData;
class WallSegmentManager;

class AbstractTeam;
class Team;
class EditorTeam;
class UIManager;

struct IniSettings;

// Modes the player could be in during the game
enum UIMode {
   PlayMode,               // Playing
   ChatMode,               // Composing chat message
   QuickChatMode,          // Showing quick-chat menu
   LoadoutMode,            // Showing loadout menu
   EngineerMode,           // Showing engineer overlay mode
   TeamShuffleMode         // Player shuffling teams
};

enum VolumeType {
   SfxVolumeType,
   MusicVolumeType,
   VoiceVolumeType,
   ServerAlertVolumeType,
};


// DNS resolve ("bitfighter.org:25955") will freeze the game unless this is done as a seperate thread
class NameToAddressThread : public TNL::Thread
{
private:
   string mAddress_string;
public:
   Address mAddress;
   bool mDone;
   bool mUsed;

   NameToAddressThread(const char *address_string);  // Constructor
   virtual ~NameToAddressThread();                   // Destructor

   U32 run();
};


/// Base class for server and client Game subclasses.  The Game
/// base class manages all the objects in the game simulation on
/// either the server or the client, and is responsible for
/// managing the passage of time as well as rendering.

class ClientRef;
class TeamManager;

class Game : public LevelLoader
{
private:
   F32 mGridSize;  

   U32 mTimeUnconnectedToMaster;                         // Time that we've been disconnected to the master
   bool mHaveTriedToConnectToMaster;

   WallSegmentManager *mWallSegmentManager;    
   TeamManager *mActiveTeamManager;

   // Functions for handling individual level parameters read in processLevelParam; some may be game-specific
   virtual void onReadTeamParam(S32 argc, const char **argv);
   void onReadTeamChangeParam(S32 argc, const char **argv);
   void onReadSpecialsParam(S32 argc, const char **argv);
   void onReadScriptParam(S32 argc, const char **argv);
   void onReadLevelNameParam(S32 argc, const char **argv);
   void onReadLevelDescriptionParam(S32 argc, const char **argv);
   void onReadLevelCreditsParam(S32 argc, const char **argv);

   S32 mPlayerCount;     // Humans only, please!
   S32 mRobotCount;

   NameToAddressThread *mNameToAddressThread;

protected:
   boost::shared_ptr<EditorObjectDatabase> mEditorDatabase;    // TODO: Move to clientGame

   virtual void cleanUp();
   U32 mNextMasterTryTime;
   bool mReadyToConnectToMaster;

   Rect mWorldExtents;     // Extents of everything

   string mLevelFileHash;  // MD5 hash of level file

   struct DeleteRef
   {
      SafePtr<GameObject> theObject;
      U32 delay;

      DeleteRef(GameObject *o = NULL, U32 d = 0);
   };

   boost::shared_ptr<GridDatabase> mGameObjDatabase;                // Database for all normal objects

   Vector<DeleteRef> mPendingDeleteObjects;
   Vector<SafePtr<GameObject> > mScopeAlwaysList;
   U32 mCurrentTime;

   RefPtr<GameNetInterface> mNetInterface;

   SafePtr<MasterServerConnection> mConnectionToMaster;
   SafePtr<GameType> mGameType;

   bool mGameSuspended;                      // True if we're in "suspended animation" mode

   GameSettings *mSettings;

   S32 findClientIndex(const StringTableEntry &name);

   // On the Client, this list will track info about every player in the game.  Note that the local client will also be represented here,
   // but the info in these records will only be managed by the server.  E.g. if the local client's name changes, the client's record
   // should not be updated directly, but rather by notifying the server, and having the server notify us.
   Vector<ClientInfo *> mClientInfos;

   TeamManager *mTeamManager;

   virtual AbstractTeam *getNewTeam() = 0;

public:
   static const S32 DefaultGridSize = 255;   // Size of "pages", represented by floats for intrapage locations (i.e. pixels per integer)
   static const S32 MIN_GRID_SIZE = 5;       // Ridiculous, it's true, but we step by our minimum value, so we can't make this too high
   static const S32 MAX_GRID_SIZE = 1000;    // A bit ridiculous too...  250-300 seems about right for normal use.  But we'll let folks experiment.

   static const S32 PLAYER_VISUAL_DISTANCE_HORIZONTAL = 600;    // How far player can see normally horizontally...
   static const S32 PLAYER_VISUAL_DISTANCE_VERTICAL = 450;      // ...and vertically

   static const S32 PLAYER_SCOPE_MARGIN = 150;

   static const S32 PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL = 1060;   // How far player can see with sensor activated horizontally...
   static const S32 PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL = 795;      // ...and vertically


   Game(const Address &theBindAddress, GameSettings *settings);         // Constructor
   virtual ~Game();                                                     // Destructor

   void setActiveTeamManager(TeamManager *teamManager);

   S32 getClientCount() const;                                          // Total number of players, human and robot
   S32 getPlayerCount() const;                                          // Returns number of human players
   S32 getRobotCount() const;                                           // Returns number of bots

   ClientInfo *getClientInfo(S32 index);
   const Vector<ClientInfo *> *getClientInfos();

   void addToClientList(ClientInfo *clientInfo);               
   void removeFromClientList(const StringTableEntry &name);             // Client side
   void removeFromClientList(ClientInfo *clientInfo);                   // Server side
   void clearClientList();

   ClientInfo *findClientInfo(const StringTableEntry &name);            // Find client by name
   
   Rect getWorldExtents();

   virtual bool isTestServer();                                         // Overridden in ServerGame

   virtual const Color *getTeamColor(S32 teamId) const;                 // ClientGame will override

   static const ModuleInfo *getModuleInfo(ShipModule module);
   
   WallSegmentManager *getWallSegmentManager() const;                   // TODO: Move back to ClientGame()

   void computeWorldObjectExtents();
   Rect computeBarrierExtents();

   Point computePlayerVisArea(Ship *ship) const;

   U32 getTimeUnconnectedToMaster();

   void resetLevelInfo();

   virtual void processLevelLoadLine(U32 argc, U32 id, const char **argv, GridDatabase *database, bool inEditor, const string &levelFileName);  
   bool processLevelParam(S32 argc, const char **argv);
   string toString();

   virtual bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database) = 0;

   void setGameTime(F32 time);                                          // Only used during level load process

   void addToDeleteList(GameObject *theObject, U32 delay);

   void deleteObjects(U8 typeNumber);
   void deleteObjects(TestFunc testFunc);

   F32 getGridSize() const;
   void setGridSize(F32 gridSize);

   U32 getCurrentTime();
   virtual bool isServer() = 0;              // Will be overridden by either clientGame (return false) or serverGame (return true)
   virtual void idle(U32 timeDelta) = 0;

   void checkConnectionToMaster(U32 timeDelta);
   MasterServerConnection *getConnectionToMaster();

   GameNetInterface *getNetInterface();
   virtual GridDatabase *getGameObjDatabase();

   EditorObjectDatabase *getEditorDatabase(); // TODO: Only for clientGame

   void setEditorDatabase(boost::shared_ptr<GridDatabase> database);

   bool runLevelGenScript(const FolderManager *folderManager, const string &scriptName, const Vector<string> &scriptArgs, 
                          GridDatabase *targetDatabase);

   const Vector<SafePtr<GameObject> > &getScopeAlwaysList();

   void setScopeAlwaysObject(GameObject *theObject);
   GameType *getGameType() const;

   // Team functions
   S32 getTeamCount() const;
   AbstractTeam *getTeam(S32 teamIndex) const;

   S32 getTeamIndex(const StringTableEntry &playerName);

   void countTeamPlayers();      // Makes sure that the mTeams[] structure has the proper player counts

   void addTeam(AbstractTeam *team);
   void addTeam(AbstractTeam *team, S32 index);
   void replaceTeam(AbstractTeam *team, S32 index);
   void removeTeam(S32 teamIndex);
   void clearTeams();

   StringTableEntry getTeamName(S32 teamIndex) const;   // Return the name of the team

   void setGameType(GameType *theGameType);
   void processDeleteList(U32 timeDelta);

   GameSettings *getSettings();

   bool isSuspended();

   void resetMasterConnectTimer();

   void setReadyToConnectToMaster(bool ready);

   // Objects in a given level, used for status bar.  On server it's objects loaded from file, on client, it's objects dl'ed from server.
   S32 mObjectsLoaded;  

   Point getScopeRange(bool sensorIsActive);
};


////////////////////////////////////////
////////////////////////////////////////

struct LevelInfo
{
public:
   StringTableEntry levelFileName;  // File level is stored in
   StringTableEntry levelName;      // Level "in-game" names
   GameTypes levelType;      
   S32 minRecPlayers;               // Min recommended number of players for this level
   S32 maxRecPlayers;               // Max recommended number of players for this level

   LevelInfo();      // Default constructor used on server side

   // Constructor, used on client side where we don't care about min/max players
   LevelInfo(const StringTableEntry &name, GameTypes type);

   // Constructor, used on server side, augmented with setInfo method below
   LevelInfo(const string &levelFile);

   void initialize();      // Called by constructors

   string getLevelTypeName();
};


////////////////////////////////////////
////////////////////////////////////////

class ServerGame : public Game
{
   typedef Game Parent;

private:
   enum {
      LevelSwitchTime = 5000,
      UpdateServerStatusTime = 20000,    // How often we update our status on the master server (ms)
      CheckServerStatusTime = 5000,      // If it did not send updates, recheck after ms
   };

   bool mTestMode;           // True if being tested from editor

   GridDatabase mDatabaseForBotZones;     // Database especially for BotZones to avoid gumming up the regular database with too many objects

   // Info about levels
   Vector<LevelInfo> mLevelInfos;         // Info about the level

   U32 mCurrentLevelIndex;                // Index of level currently being played
   Timer mLevelSwitchTimer;               // Track how long after game has ended before we actually switch levels
   Timer mMasterUpdateTimer;              // Periodically let the master know how we're doing
   bool mShuttingDown;
   Timer mShutdownTimer;
   GameConnection *mShutdownOriginator;   // Who started the shutdown?

   bool mDedicated;

   S32 mLevelLoadIndex;                   // For keeping track of where we are in the level loading process.  NOT CURRENT LEVEL IN PLAY!

   GameConnection *mSuspendor;            // Player requesting suspension if game suspended by request

   // For simulating CPU stutter
   Timer mStutterTimer;                   
   Timer mStutterSleepTimer;
   U32 mAccumulatedSleepTime;

   void setCurrentLevelIndex(S32 nextLevel, S32 playerCount);  // Helper for cycleLevel()
   void resetAllClientTeams();                                 // Resets all player team assignments

   bool onlyClientIs(GameConnection *client);

   void cleanUp();

   AbstractTeam *getNewTeam();

public:
   ServerGame(const Address &address, GameSettings *settings, bool testMode, bool dedicated);    // Constructor
   virtual ~ServerGame();   // Destructor

   U32 mInfoFlags;           // Not used for much at the moment, but who knows? --> propagates to master
   U32 mVoteTimer;
   S32 mVoteType;
   S32 mVoteYes;
   S32 mVoteNo;
   S32 mVoteNumber;
   StringPtr mVoteString;
   S32 mNextLevel;

   //SafePtr<GameConnection> mVoteClientConnection;
   StringTableEntry mVoteClientName;
   bool voteStart(ClientInfo *clientInfo, S32 type, S32 number = 0);
   void voteClient(ClientInfo *clientInfo, bool voteYes);

   enum HostingModePhases {
      NotHosting,
      LoadingLevels,
      DoneLoadingLevels,
      Hosting
   };

   bool startHosting();

   static const S32 NEXT_LEVEL = -1;
   static const S32 REPLAY_LEVEL = -2;
   static const S32 PREVIOUS_LEVEL = -3;
   static const S32 FIRST_LEVEL = 0;

   GridDatabase *getBotZoneDatabase();

   U32 getMaxPlayers();

   bool isDedicated();
   void setDedicated(bool dedicated);

   bool isFull();      // More room at the inn?

   void addClient(ClientInfo *clientInfo);
   void removeClient(ClientInfo *clientInfo);
   

   void setShuttingDown(bool shuttingDown, U16 time, GameConnection *who, StringPtr reason);  

   void buildBasicLevelInfoList(const Vector<string> &levelList);
   void resetLevelLoadIndex();
   void loadNextLevelInfo();
   bool getLevelInfo(const string &fullFilename, LevelInfo &levelInfo); // Populates levelInfo with data from fullFilename
   string getLastLevelLoadName();             // For updating the UI

   bool loadLevel(const string &fileName);        // Load a level

   bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database);   // Things like spawns that aren't really items

   void cycleLevel(S32 newLevelIndex = NEXT_LEVEL);

   StringTableEntry getLevelNameFromIndex(S32 indx);
   S32 getAbsoluteLevelIndex(S32 indx);      // Figures out the level index if the input is a relative index
   string getLevelFileNameFromIndex(S32 indx);

   StringTableEntry getCurrentLevelFileName();  // Return filename of level currently in play  
   StringTableEntry getCurrentLevelName();      // Return name of level currently in play
   GameTypes getCurrentLevelType();             // Return type of level currently in play
   StringTableEntry getCurrentLevelTypeName();  // Return name of type of level currently in play

   bool isServer();
   void idle(U32 timeDelta);
   bool isReadyToShutdown(U32 timeDelta);
   void gameEnded();

   S32 getLevelNameCount();
   S32 getCurrentLevelIndex();
   S32 getLevelCount();
   LevelInfo getLevelInfo(S32 index);
   void clearLevelInfos();
   void addLevelInfo(const LevelInfo &levelInfo);

   bool isTestServer();

   DataSender dataSender;

   void suspendGame();
   void suspendGame(GameConnection *requestor);  // Suspend at player's request
   void unsuspendGame(bool remoteRequest);

   void suspenderLeftGame();
   GameConnection *getSuspendor();

   S32 addUploadedLevelInfo(const char *filename, LevelInfo &info);

   HostingModePhases hostingModePhase;
};

////////////////////////////////////////
////////////////////////////////////////

extern ServerGame *gServerGame;

};


#endif

