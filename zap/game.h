//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#include "../tnl/tnlNetObject.h"

#include "gameConnection.h"
#include "gridDB.h"
#include "timer.h"
#include "gameLoader.h"
#include "point.h"

#include <windows.h>   // For screensaver
#include <string>

///
/// Zap - a 2D space game demonstrating the full capabilities of the
/// Torque Network Library.
///
/// The Zap example game is a 2D vector-graphics game that utilizes
/// some of the more advanced features of the TNL.  Zap also demonstrates
/// the use of client-side prediction, and interpolation to present
/// a consistent simulation to clients over a connection with perceptible
/// latency.
///
/// Zap can run in 3 modes - as a client, a client and server, or a dedicated
/// server.  The dedicated server option is available only as a launch
/// parameter from the command line.
///
/// If it is run as a client, Zap uses the GLUT library to perform
/// cross-platform window intialization, event processing and OpenGL setup.
///
/// Zap implements a simple game framework.  The GameObject class is
/// the root class for all of the various objects in the Zap world, including
/// Ship, Barrier and Projectile instances.  The Game class, which is instanced
/// once for the client and once for the server, manages the current
/// list of GameObject instances.
///
/// Zap clients can connect to servers directly that are on the same LAN
/// or for which the IP address is known.  Zap is also capable of talking
/// to the TNL master server and using its arranged connection functionality
/// to talk to servers.
///
/// The simplified user interface for Zap is managed entirely through
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

public:
   enum
   {
      DefaultGridSize = 255,           // Size of "pages", represented by floats for intrapage locations (i.e. pixels per integer)
      minGridSize = 5,                 // Ridiculous, it's true, but we step by our minimum value, so we can't make this too high
      maxGridSize = 1000,              // A bit ridiculous too...  250-300 seems about right for normal use.  But we'll let folks experiment.

      PlayerHorizVisDistance = 600,    // How far player can see normally horizontally...
      PlayerVertVisDistance = 450,     // ...and vertically

      PlayerScopeMargin = 150,

      PlayerSensorHorizVisDistance = 1060,   // How far player can see with sensor activated horizontally...
      PlayerSensorVertVisDistance = 795,     // ...and vertically

      MasterServerConnectAttemptDelay = 60000,
   };


   Game(const Address &theBindAddress);
   virtual ~Game() {}

   Rect computeWorldObjectExtents();
   Point computePlayerVisArea(Ship *ship);

   void addToDeleteList(GameObject *theObject, U32 delay);

   void addToGameObjectList(GameObject *theObject);
   void removeFromGameObjectList(GameObject *theObject);
   void deleteObjects(U32 typeMask);

   void setGridSize(F32 gridSize) { mGridSize = gridSize; }

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

   S32 mObjectsLoaded;        // Objects in a given level, used for status bar.  On server it's objects loaded from file, on client, it's objects dl'ed from server.
};

class ServerGame : public Game, public LevelLoader
{
private:
   enum {
      LevelSwitchTime = 5000,
      UpdateServerStatusTime = 5000,      // Time to update our status on the master server (ms)
   };

   U32 mPlayerCount;
   U32 mMaxPlayers;
   U32 mInfoFlags;           // Not used for much at the moment, but who knows? --> propigates to master
   const char *mHostName;
   const char *mHostDescr;

   // Info about levels
   Vector<StringTableEntry> mLevelList;
   Vector<StringTableEntry> mLevelNames;
   Vector<StringTableEntry> mLevelTypes;
   Vector<S32> mMinRecPlayers;            // Recommended min number of players for this level
   Vector<S32> mMaxRecPlayers;            // Recommended max number of players for this level

   U32 mCurrentLevelIndex;
   Timer mLevelSwitchTimer;               // Track how long after game has ended before we actually switch levels
   Timer mMasterUpdateTimer;              // Periodically let the master know how we're doing
   S32 mLevelLoadIndex;                   // For keeping track of where we are in the level loading process

public:
   U32 getPlayerCount() { return mPlayerCount; }
   U32 getMaxPlayers() { return mMaxPlayers; }
   const char *getHostName() { return mHostName; }
   const char *getHostDescr() { return mHostDescr; }

   bool isFull() { return mPlayerCount == mMaxPlayers; }    // Room for more players?

   void addClient(GameConnection *theConnection);
   void removeClient(GameConnection *theConnection);
   ServerGame(const Address &theBindAddress, U32 maxPlayers, const char *hostName);

   void setLevelList(Vector<StringTableEntry> levelList);
   void resetLevelLoadIndex();
   void loadNextLevel();
   string ServerGame::getCurrentLevelLoadName();      // For updating the UI
   bool loadLevel(string fileName);

   void cycleLevel(S32 newLevelIndex = -1);
   string getLevelFileName(string base);     // Handles prepending subfolder, if needed
   StringTableEntry getLevelNameFromIndex(S32 indx);
   string getLevelFileNameFromIndex(S32 indx);
   StringTableEntry getCurrentLevelName();   // Return name of level currently in play
   StringTableEntry getCurrentLevelType();   // Return type of level currently in play

   void processLevelLoadLine(int argc, const char **argv);
   bool isServer() { return true; }
   void idle(U32 timeDelta);
   void gameEnded();

   S32 getLevelNameCount();
};

class Ship;

class ClientGame : public Game
{
private:
   enum {
      NumStars = 256,      // 256 stars should be enough for anybody!   -- Bill Gates
      CommanderMapZoomTime = 350,
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

   bool getInCommanderMap() { return mInCommanderMap; }

   F32 getCommanderZoomFraction() { return mCommanderZoomDelta / F32(CommanderMapZoomTime); }
   Point worldToScreenPoint(Point p);
   void setConnectionToServer(GameConnection *connection);
   void drawStars(F32 alphaFrac, Point cameraPos, Point visibleExtent);
   GameConnection *getConnectionToServer();
   void render();
   void renderNormal();       // Render game in normal play mode
   void renderCommander();    // Render game in commander's map mode
   void renderOverlayMap();   // Render the overlay map in normal play mode
   void resetZoomDelta() { mCommanderZoomDelta = CommanderMapZoomTime; }      // Used by teleporter
   bool isServer() { return false; }
   void idle(U32 timeDelta);
   void zoomCommanderMap();
};

extern ServerGame *gServerGame;
extern ClientGame *gClientGame;
extern Address gMasterAddress;

extern void initHostGame(Address bindAddress);
extern void joinGame(Address remoteAddress, bool isFromMaster, bool local);
extern void endGame();

// already bumbed master, release.  cs also ok
#define MASTER_PROTOCOL_VERSION 2  // Change this when releasing an incompatible cm protocol (must be int)
#define CS_PROTOCOL_VERSION 16     // Change this when releasing an incompatible cs protocol (must be int)
#define BUILD_VERSION 280          // Version of the game according to SVN, will be unique every release (must be int)
#define ZAP_GAME_RELEASE "011 preveiew" //"Bitfighter Release Candidate 010"   // Change this with every release -- for display purposes only, string

};


#endif
