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

#include "tnlNetObject.h"
#include "tnlTypes.h"

#include "gridDB.h"
#include "Timer.h"
#include "gameLoader.h"
#include "Rect.h"
#include "Color.h"
#include "Colors.h"
#include "shipItems.h"     // For moduleInfos

#include "dataConnection.h"      // For DataSender


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
class TeamEditor;
class UIManager;

struct IniSettings;
extern IniSettings gIniSettings;

// Modes the player could be in during the game
enum UIMode {
   PlayMode,               // Playing
   ChatMode,               // Composing chat message
   QuickChatMode,          // Showing quick-chat menu
   LoadoutMode,            // Showing loadout menu
   EngineerMode,           // Showing engineer overlay mode
};

enum VolumeType {
   SfxVolumeType,
   MusicVolumeType,
   VoiceVolumeType,
   ServerAlertVolumeType,
};

/// Base class for server and client Game subclasses.  The Game
/// base class manages all the objects in the game simulation on
/// either the server or the client, and is responsible for
/// managing the passage of time as well as rendering.

class Game : public LevelLoader
{
private:
   F32 mGridSize;     

   U32 mTimeUnconnectedToMaster;      // Time that we've been unconnected to the master
   bool mHaveTriedToConnectToMaster;

   WallSegmentManager *mWallSegmentManager;    

   // Info about modules -- access via getModuleInfo()
   Vector<ModuleInfo> mModuleInfos;
   void buildModuleInfos();

   Vector<boost::shared_ptr<AbstractTeam> > mTeams;       // List of teams

   // Functions for handling individual level parameters read in processLevelParam; some may be game-specific
   void onReadTeamParam(S32 argc, const char **argv);
   void onReadTeamChangeParam(S32 argc, const char **argv);
   void onReadSpecialsParam(S32 argc, const char **argv);
   void onReadSoccerPickupParam(S32 argc, const char **argv);
   void onReadScriptParam(S32 argc, const char **argv);
   void onReadLevelNameParam(S32 argc, const char **argv);
   void onReadLevelDescriptionParam(S32 argc, const char **argv);
   void onReadLevelCreditsParam(S32 argc, const char **argv);
   
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

   bool mGameSuspended;       // True if we're in "suspended animation" mode

public:
   static const S32 DefaultGridSize = 255;   // Size of "pages", represented by floats for intrapage locations (i.e. pixels per integer)
   static const S32 MIN_GRID_SIZE = 5;       // Ridiculous, it's true, but we step by our minimum value, so we can't make this too high
   static const S32 MAX_GRID_SIZE = 1000;    // A bit ridiculous too...  250-300 seems about right for normal use.  But we'll let folks experiment.

   static const S32 PLAYER_VISUAL_DISTANCE_HORIZONTAL = 600;    // How far player can see normally horizontally...
   static const S32 PLAYER_VISUAL_DISTANCE_VERTICAL = 450;     // ...and vertically

   static const S32 PLAYER_SCOPE_MARGIN = 150;

   static const S32 PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL = 1060;   // How far player can see with sensor activated horizontally...
   static const S32 PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL = 795;      // ...and vertically

   static const S32 PLAYER_COUNT_UNAVAILABLE = -1;

   Game(const Address &theBindAddress);      // Constructor
   virtual ~Game();                          // Destructor

   Rect getWorldExtents() { return mWorldExtents; }

   virtual U32 getPlayerCount() = 0;         // Implemented differently on client and server

   virtual bool isTestServer() { return false; }     // Overridden in ServerGame

   virtual const Color *getTeamColor(S32 teamId) const { return &Colors::white; }  // ClientGame will override
   static const Color *getBasicTeamColor(const Game *game, S32 teamId);            // Color function used in most cases, overridden by some games

   ModuleInfo *getModuleInfo(ShipModule module) { return &mModuleInfos[(U32)module]; }
   
   WallSegmentManager *getWallSegmentManager() const { return mWallSegmentManager; }      // TODO: Move back to ClientGame()

   void computeWorldObjectExtents();
   Rect computeBarrierExtents();

   Point computePlayerVisArea(Ship *ship) const;

   U32 getTimeUnconnectedToMaster() { return mTimeUnconnectedToMaster; }

   void resetLevelInfo();

   virtual void processLevelLoadLine(U32 argc, U32 id, const char **argv, GridDatabase *database, bool inEditor, const string &levelFileName);  
   bool processLevelParam(S32 argc, const char **argv);
   string toString();

   virtual bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName) = 0;

   void setGameTime(F32 time);                                          // Only used during level load process

   void addToDeleteList(GameObject *theObject, U32 delay);

   void deleteObjects(U8 typeNumber);
   void deleteObjects(TestFunc testFunc);

   F32 getGridSize() const { return mGridSize; }
   void setGridSize(F32 gridSize);

   static Point getScopeRange(bool sensorIsActive) { return sensorIsActive ? Point(PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL + PLAYER_SCOPE_MARGIN,
                                                                                   PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL  + PLAYER_SCOPE_MARGIN)
                                                                           : Point(PLAYER_VISUAL_DISTANCE_HORIZONTAL + PLAYER_SCOPE_MARGIN,
                                                                                   PLAYER_VISUAL_DISTANCE_VERTICAL  + PLAYER_SCOPE_MARGIN); }
   U32 getCurrentTime() { return mCurrentTime; }
   virtual bool isServer() = 0;              // Will be overridden by either clientGame (return false) or serverGame (return true)
   virtual void idle(U32 timeDelta) = 0;

   void checkConnectionToMaster(U32 timeDelta);
   MasterServerConnection *getConnectionToMaster();

   GameNetInterface *getNetInterface();
   virtual GridDatabase *getGameObjDatabase() { return mGameObjDatabase.get(); }    

   EditorObjectDatabase *getEditorDatabase() // TODO: Only for clientGame
   { 
      // Lazy init
      if(!mEditorDatabase.get())
         mEditorDatabase = boost::shared_ptr<EditorObjectDatabase>(new EditorObjectDatabase());
         
      return mEditorDatabase.get(); 
   }      

   void setEditorDatabase(boost::shared_ptr<GridDatabase> database) { mEditorDatabase = boost::dynamic_pointer_cast<EditorObjectDatabase>(database); }


   const Vector<SafePtr<GameObject> > &getScopeAlwaysList() { return mScopeAlwaysList; }

   void setScopeAlwaysObject(GameObject *theObject);
   GameType *getGameType() const;

   // Team functions
   S32 getTeamCount() const;
   AbstractTeam *getTeam(S32 teamIndex) const;
   void addTeam(boost::shared_ptr<AbstractTeam> team);
   void addTeam(boost::shared_ptr<AbstractTeam> team, S32 index);
   void replaceTeam(boost::shared_ptr<AbstractTeam> team, S32 index);
   virtual boost::shared_ptr<AbstractTeam> getNewTeam() = 0;
   void removeTeam(S32 teamIndex);
   void clearTeams();
   StringTableEntry getTeamName(S32 teamIndex) const;   // Return the name of the team

   void setGameType(GameType *theGameType);
   void processDeleteList(U32 timeDelta);

   IniSettings *getIniSettings() { return &gIniSettings; }

   bool isSuspended() { return mGameSuspended; }

   void resetMasterConnectTimer() { mNextMasterTryTime = 0; }

   void setReadyToConnectToMaster(bool ready) { mReadyToConnectToMaster = ready; }

   // Objects in a given level, used for status bar.  On server it's objects loaded from file, on client, it's objects dl'ed from server.
   S32 mObjectsLoaded;        
};


////////////////////////////////////////
////////////////////////////////////////

struct LevelInfo
{
public:
   StringTableEntry levelFileName;  // File level is stored in
   StringTableEntry levelName;      // Level "in-game" names
   StringTableEntry levelType;      
   S32 minRecPlayers;               // Min recommended number of players for this level
   S32 maxRecPlayers;               // Max recommended number of players for this level

   LevelInfo();      // Default constructor used on server side

   // Constructor, used on client side where we don't care about min/max players
   LevelInfo(const StringTableEntry &name, const StringTableEntry &type);

   // Constructor, used on server side, augmented with setInfo method below
   LevelInfo(const string &levelFile);

   void initialize();      // Called by constructors
};


////////////////////////////////////////
////////////////////////////////////////

class ClientRef;

class ServerGame : public Game
{
   typedef Game Parent;

private:
   enum {
      LevelSwitchTime = 5000,
      UpdateServerStatusTime = 20000,    // How often we update our status on the master server (ms)
      CheckServerStatusTime = 5000,      // If it did not send updates, recheck after ms
   };

   U32 mMaxPlayers;
   bool mTestMode;           // True if being tested from editor
   string mHostName;
   string mHostDescr;

   GridDatabase mDatabaseForBotZones;     // Database especially for BotZones to avoid gumming up the regular database with too many objects

   // Info about levels
   Vector<LevelInfo> mLevelInfos;         // Info about the level

   U32 mCurrentLevelIndex;                // Index of level currently being played
   Timer mLevelSwitchTimer;               // Track how long after game has ended before we actually switch levels
   Timer mMasterUpdateTimer;              // Periodically let the master know how we're doing
   bool mShuttingDown;
   Timer mShutdownTimer;
   GameConnection *mShutdownOriginator;   // Who started the shutdown?

   S32 mLevelLoadIndex;                   // For keeping track of where we are in the level loading process.  NOT CURRENT LEVEL IN PLAY!

   GameConnection *mSuspendor;            // Player requesting suspension if game suspended by request

   U32 mPlayerCount;
   void cleanUp();

public:
   U32 mInfoFlags;           // Not used for much at the moment, but who knows? --> propagates to master
   ServerGame(const Address &theBindAddress, const string &hostName, const string &hostDescr, U32 maxPlayers, bool testMode);    // Constructor
   virtual ~ServerGame();   // Destructor

   U32 mVoteTimer;
   S32 mVoteType;
   S32 mVoteYes;
   S32 mVoteNo;
   S32 mVoteNumber;
   StringPtr mVoteString;
   S32 mNextLevel;

   //SafePtr<GameConnection> mVoteClientConnection;
   StringTableEntry mVoteClientName;
   bool voteStart(GameConnection *client, S32 type, S32 number = 0);
   void voteClient(GameConnection *client, bool voteYes);

   enum HostingModePhases { NotHosting, LoadingLevels, DoneLoadingLevels, Hosting };      

   static const S32 NEXT_LEVEL = -1;
   static const S32 REPLAY_LEVEL = -2;
   static const S32 PREVIOUS_LEVEL = -3;
   static const S32 FIRST_LEVEL = 0;

   GridDatabase *getBotZoneDatabase() { return &mDatabaseForBotZones; }

   U32 getPlayerCount() { return mPlayerCount; }
   U32 getMaxPlayers() { return mMaxPlayers; }

   void setHostName(const char *name) { mHostName = name; }
   const char *getHostName() { return mHostName.c_str(); }

   void setHostDescr(const char *descr) { mHostDescr = descr; }
   const char *getHostDescr() { return mHostDescr.c_str(); }

   bool isFull() { return mPlayerCount == mMaxPlayers; }    // Room for more players?

   void addClient(GameConnection *theConnection);
   void removeClient(GameConnection *theConnection);

   void setShuttingDown(bool shuttingDown, U16 time, ClientRef *who, StringPtr reason);  

   void buildBasicLevelInfoList(const Vector<string> &levelList);
   void resetLevelLoadIndex();
   void loadNextLevelInfo();
   bool getLevelInfo(const string &fullFilename, LevelInfo &levelInfo);   // Populates levelInfo with data from fullFilename
   string getLastLevelLoadName();             // For updating the UI

   bool loadLevel(const string &fileName);    // Load a level

   bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName);      // Things like spawns that aren't really items

   void cycleLevel(S32 newLevelIndex = NEXT_LEVEL);

   StringTableEntry getLevelNameFromIndex(S32 indx);
   S32 getAbsoluteLevelIndex(S32 indx);      // Figures out the level index if the input is a relative index
   string getLevelFileNameFromIndex(S32 indx);

   StringTableEntry getCurrentLevelFileName();  // Return filename of level currently in play  
   StringTableEntry getCurrentLevelName();      // Return name of level currently in play
   StringTableEntry getCurrentLevelType();      // Return type of level currently in play

   bool isServer() { return true; }
   void idle(U32 timeDelta);
   void gameEnded();

   S32 getLevelNameCount();
   S32 getRobotCount();
   S32 getCurrentLevelIndex() { return mCurrentLevelIndex; }
   S32 getLevelCount() { return mLevelInfos.size(); }
   LevelInfo getLevelInfo(S32 index) { return mLevelInfos[index]; }
   void clearLevelInfos() { mLevelInfos.clear(); }
   void addLevelInfo(const LevelInfo &levelInfo) { mLevelInfos.push_back(levelInfo); }

   bool isTestServer() { return mTestMode; }

   DataSender dataSender;

   void suspendGame();
   void suspendGame(GameConnection *requestor);  // Suspend at player's request
   void unsuspendGame(bool remoteRequest);

   void suspenderLeftGame() { mSuspendor = NULL; }
   GameConnection *getSuspendor() { return mSuspendor; }

   S32 addUploadedLevelInfo(const char *filename, LevelInfo &info);

   HostingModePhases hostingModePhase;

   boost::shared_ptr<AbstractTeam> getNewTeam();
};

////////////////////////////////////////
////////////////////////////////////////

extern ServerGame *gServerGame;

extern Vector<string> gMasterAddress;

extern void endGame();

};


#endif

