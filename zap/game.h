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

#include "gameConnection.h"
#include "gridDB.h"
#include "timer.h"
#include "gameLoader.h"
#include "point.h"
#include "UIChat.h"

#ifdef TNL_OS_WIN32
#include <windows.h>   // For screensaver... windows only feature, I'm afraid!
#endif

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

// Some forward declarations
class MasterServerConnection;
class GameNetInterface;
class GameType;
class GameObject;
class GameConnection;
class Ship;

/// Base class for server and client Game subclasses.  The Game
/// base class manages all the objects in the game simulation on
/// either the server or the client, and is responsible for
/// managing the passage of time as well as rendering.

class Game
{
protected:
   U32 mNextMasterTryTime;
   bool mReadyToConnectToMaster;

   F32 mGridSize;          // GridSize for this level (default defined below)

   struct DeleteRef
   {
      SafePtr<GameObject> theObject;
      U32 delay;

      DeleteRef(GameObject *o = NULL, U32 d = 0);
   };

   GridDatabase mDatabase;
   Vector<GameObject *> mGameObjects;
   Vector<DeleteRef> mPendingDeleteObjects;
   Vector<SafePtr<GameObject> > mScopeAlwaysList;
   U32 mCurrentTime;

   RefPtr<GameNetInterface> mNetInterface;

   SafePtr<MasterServerConnection> mConnectionToMaster;
   SafePtr<GameType> mGameType;

   bool mGameSuspended;                   // True if we're in "suspended animation" mode

public:
   static const S32 DefaultGridSize = 255;   // Size of "pages", represented by floats for intrapage locations (i.e. pixels per integer)
   static const S32 MIN_GRID_SIZE = 5;       // Ridiculous, it's true, but we step by our minimum value, so we can't make this too high
   static const S32 MAX_GRID_SIZE = 1000;    // A bit ridiculous too...  250-300 seems about right for normal use.  But we'll let folks experiment.

   enum
   {
      PlayerHorizVisDistance = 600,    // How far player can see normally horizontally...
      PlayerVertVisDistance = 450,     // ...and vertically

      PlayerScopeMargin = 150,

      PlayerSensorHorizVisDistance = 1060,   // How far player can see with sensor activated horizontally...
      PlayerSensorVertVisDistance = 795,     // ...and vertically

      MASTER_SERVER_FAILURE_RETRY = 10000,   // 10 secs
      PLAYER_COUNT_UNAVAILABLE = -1,
   };

   virtual U32 getPlayerCount() = 0;         // Implemented differently on client and server

   Game(const Address &theBindAddress);      // Constructor
   virtual ~Game() { /* Do nothing */ };     // Destructor
   
   Rect computeWorldObjectExtents();
   Rect computeBarrierExtents();

   Point computePlayerVisArea(Ship *ship);


   void addToDeleteList(GameObject *theObject, U32 delay);

   void addToGameObjectList(GameObject *theObject);
   void removeFromGameObjectList(GameObject *theObject);
   void deleteObjects(U32 typeMask);

   void setGridSize(F32 gridSize) { mGridSize = gridSize; }

   static Point getScopeRange(bool sensorIsActive) { return sensorIsActive ? Point(PlayerSensorHorizVisDistance + PlayerScopeMargin,
                                                                                   PlayerSensorVertVisDistance  + PlayerScopeMargin)
                                                                           : Point(PlayerHorizVisDistance + PlayerScopeMargin,
                                                                                   PlayerVertVisDistance  + PlayerScopeMargin); }
   F32 getGridSize() { return mGridSize; }
   U32 getCurrentTime() { return mCurrentTime; }
   virtual bool isServer() = 0;              // Will be overridden by either clientGame (return false) or serverGame (return true)
   virtual void idle(U32 timeDelta) = 0;

   void checkConnectionToMaster(U32 timeDelta);
   MasterServerConnection *getConnectionToMaster();

   GameNetInterface *getNetInterface();
   GridDatabase *getGridDatabase() { return &mDatabase; }

   const Vector<SafePtr<GameObject> > &getScopeAlwaysList() { return mScopeAlwaysList; }

   void setScopeAlwaysObject(GameObject *theObject);
   GameType *getGameType();
   U32 getTeamCount();

   void setGameType(GameType *theGameType);
   void processDeleteList(U32 timeDelta);

   bool isSuspended() { return mGameSuspended; }

   void resetMasterConnectTimer() { mNextMasterTryTime = 0; }

   void setReadyToConnectToMaster(bool ready) { mReadyToConnectToMaster = ready; }

   S32 mObjectsLoaded;        // Objects in a given level, used for status bar.  On server it's objects loaded from file, on client, it's objects dl'ed from server.
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

   
   LevelInfo() { /* Do nothing */ }    // Default constructor

   // Used on client side where we don't care about min/max players
   LevelInfo(StringTableEntry name, StringTableEntry type)     
   {
      levelName = name;  levelType = type; 
   }

   // Used on server side, augmented with setInfo method below
   LevelInfo(StringTableEntry levelFile)
   {
      levelFileName = levelFile;
   }

   // Used on server side
   void setInfo(StringTableEntry name, StringTableEntry type, S32 minPlayers, S32 maxPlayers)
   {
      levelName = name;  levelType = type;  minRecPlayers = minPlayers;  maxRecPlayers = maxPlayers; 
   }
};

////////////////////////////////////////
////////////////////////////////////////

class ClientRef;

class ServerGame : public Game, public LevelLoader
{
private:
   enum {
      LevelSwitchTime = 5000,
      UpdateServerStatusTime = 5000,      // Time to update our status on the master server (ms)
   };

   U32 mMaxPlayers;
   U32 mInfoFlags;           // Not used for much at the moment, but who knows? --> propagates to master
   bool mTestMode;           // True if being tested from editor
   string mHostName;
   string mHostDescr;

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

public:
   ServerGame(const Address &theBindAddress, U32 maxPlayers, const char *hostName, bool testMode);    // Constructor
   ~ServerGame();   // Destructor

   enum HostingModePhases { NotHosting, LoadingLevels, DoneLoadingLevels, Hosting };      

   static const S32 NEXT_LEVEL = -1;
   static const S32 REPLAY_LEVEL = -2;
   static const S32 PREVIOUS_LEVEL = -3;
   static const S32 FIRST_LEVEL = 0;

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

   void buildLevelList(Vector<StringTableEntry> levelList);
   void resetLevelLoadIndex();
   void loadNextLevel();
   string getLastLevelLoadName();         // For updating the UI

   bool loadLevel(string fileName);       // Load a level

   void cycleLevel(S32 newLevelIndex = NEXT_LEVEL);
   string getLevelFileName(string base);     // Joins base and dir, if appropriate
   StringTableEntry getLevelNameFromIndex(S32 indx);
   S32 getAbsoluteLevelIndex(S32 indx);      // Figures out the level index if the input is a relative index
   string getLevelFileNameFromIndex(S32 indx);

   StringTableEntry getCurrentLevelFileName();  // Return filename of level currently in play  
   StringTableEntry getCurrentLevelName();      // Return name of level currently in play
   StringTableEntry getCurrentLevelType();      // Return type of level currently in play

   void processLevelLoadLine(U32 argc, U32 id, const char **argv);
   bool isServer() { return true; }
   void idle(U32 timeDelta);
   void gameEnded();

   S32 getLevelNameCount();
   S32 getRobotCount();
   S32 getCurrentLevelIndex() { return mCurrentLevelIndex; }
   S32 getLevelCount() { return mLevelInfos.size(); }
   bool isTestServer() { return mTestMode; }

   void suspendGame();
   void suspendGame(GameConnection *requestor);  // Suspend at player's request
   void unsuspendGame(bool remoteRequest);

   void suspenderLeftGame() { mSuspendor = NULL; }
   GameConnection *getSuspendor() { return mSuspendor; }

   HostingModePhases hostingModePhase;
};

////////////////////////////////////////
////////////////////////////////////////

class ClientGame : public Game
{
private:
   enum {
      NumStars = 256,               // 256 stars should be enough for anybody!   -- Bill Gates
      CommanderMapZoomTime = 350,   // Transition time between regular map and commander's map; in ms, higher = slower
   };

   Point mStars[NumStars];
   Rect mWorldBounds;

   SafePtr<GameConnection> mConnectionToServer; // If this is a client game, this is the connection to the server
   bool mInCommanderMap;

   U32 mCommanderZoomDelta;

   Timer mScreenSaverTimer;
   void supressScreensaver();

public:
   ClientGame(const Address &bindAddress);

   bool hasValidControlObject();
   bool isConnectedToServer();
   GameConnection *getConnectionToServer();

   bool getInCommanderMap() { return mInCommanderMap; }
   void setInCommanderMap(bool inCommanderMap) { mInCommanderMap = inCommanderMap; }

   F32 getCommanderZoomFraction() { return mCommanderZoomDelta / F32(CommanderMapZoomTime); }
   Point worldToScreenPoint(Point p);
   void setConnectionToServer(GameConnection *connection);
   void drawStars(F32 alphaFrac, Point cameraPos, Point visibleExtent);
   void render();
   
   void renderNormal();       // Render game in normal play mode
   void renderCommander();    // Render game in commander's map mode
   void renderSuspended();    // Render suspended game

   void renderOverlayMap();   // Render the overlay map in normal play mode
   void resetZoomDelta() { mCommanderZoomDelta = CommanderMapZoomTime; } 
   void clearZoomDelta() { mCommanderZoomDelta = 0; }
   bool isServer() { return false; }
   void idle(U32 timeDelta);
   void zoomCommanderMap();

   U32 getPlayerAndRobotCount();    // Returns number of human and robot players
   U32 getPlayerCount();            // Returns number of human players

   void suspendGame()   { mGameSuspended = true; }
   void unsuspendGame() { mGameSuspended = false; }

   void prepareBarrierRenderingGeometry();      // Get walls ready for display
};

////////////////////////////////////////
////////////////////////////////////////

extern ServerGame *gServerGame;
extern ClientGame *gClientGame;
extern Address gMasterAddress;

extern void joinGame(Address remoteAddress, bool isFromMaster, bool local);
extern void endGame();

#define MASTER_PROTOCOL_VERSION 3  // Change this when releasing an incompatible cm/sm protocol (must be int)
#define CS_PROTOCOL_VERSION 30     // Change this when releasing an incompatible cs protocol (must be int)
#define BUILD_VERSION 936          // Version of the game according to SVN, will be unique every release (must be int)
#define ZAP_GAME_RELEASE "014 beta 1"     // Change this with every release -- for display purposes only, string, 
                                   // will also be used for name of installer on windows, so be careful with spaces
};


#endif


