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
#include "dataConnection.h"      // For DataSender
#include "SharedConstants.h"     // For badges enum

#include "GameTypesEnum.h"
#include "GameSettings.h"

#include "teamInfo.h"            // For ClassManager

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




// Uncomment the following line to show server data in the game window.  This will allow you to see how lag affects various
// game items.  Note that in most cases this will only work when you are hosting the game yourself.  DO NOT ship code with
// this option enabled, as it will likely cause crashes in some situations.

#if !defined(TNL_DEBUG) && defined(SHOW_SERVER_SITUATION) && !defined(ZAP_DEDICATED)
//#define SHOW_SERVER_SITUATION
#endif



using namespace std;

namespace Zap
{

const U32 MAX_GAME_NAME_LEN = 32;     // Any longer, and it won't fit on-screen
const U32 MAX_GAME_DESCR_LEN = 60;    // Any longer, and it won't fit on-screen; also limits max length of credits string

////////////////////////////////////////
////////////////////////////////////////

// Some forward declarations
class AnonymousMasterServerConnection;
class MasterServerConnection;
class GameNetInterface;
class GameType;
class BfObject;
class GameConnection;
class Ship;
class GameUserInterface;
struct UserInterfaceData;
class WallSegmentManager;
class Robot;

class AbstractTeam;
class Team;
class EditorTeam;
class UIManager;

struct IniSettings;

typedef void (MasterServerConnection::*MasterConnectionCallback)();

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

   NameToAddressThread(const char *address_string);  // Constructor
   virtual ~NameToAddressThread();                   // Destructor

   U32 run();
};


/// Base class for server and client Game subclasses.  The Game
/// base class manages all the objects in the game simulation on
/// either the server or the client, and is responsible for
/// managing the passage of time as well as rendering.

// Some forward declarations
class ClientRef;
class ClientInfo;
class PolyWall;
class WallItem;
class LuaLevelGenerator;

class Game : public LevelLoader
{
private:
   F32 mGridSize;  

   U32 mTimeUnconnectedToMaster;          // Time that we've been disconnected to the master
   bool mHaveTriedToConnectToMaster;

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
   U32 mNextMasterTryTime;

   bool mReadyToConnectToMaster;

   Vector<Robot *> mRobots;               // Grand master list of all robots in the current game
   Rect mWorldExtents;                    // Extents of everything
   string mLevelFileHash;                 // MD5 hash of level file

   Timer mTimeToSuspend;                  // Countdown to suspend to allow our fade animation to play out

   virtual void idle(U32 timeDelta);      // Only called from ServerGame::idle() and ClientGame::idle()

   virtual void cleanUp();
   
   struct DeleteRef
   {
      SafePtr<BfObject> theObject;
      U32 delay;

      DeleteRef(BfObject *o = NULL, U32 d = 0);
   };

   boost::shared_ptr<GridDatabase> mGameObjDatabase;                // Database for all normal objects

   Vector<DeleteRef> mPendingDeleteObjects;
   Vector<SafePtr<BfObject> > mScopeAlwaysList;
   U32 mCurrentTime;

   RefPtr<GameNetInterface> mNetInterface;

   SafePtr<MasterServerConnection> mConnectionToMaster;

   // Not really a queue, but good enough for now!
   SafePtr<AnonymousMasterServerConnection> mAnonymousMasterServerConnection;

   SafePtr<GameType> mGameType;

   bool mGameSuspended;       // True if we're in "suspended animation" mode

   GameSettings *mSettings;

   S32 findClientIndex(const StringTableEntry &name);

   // On the Client, this list will track info about every player in the game.  Note that the local client will also be represented here,
   // but the info in these records will only be managed by the server.  E.g. if the local client's name changes, the client's record
   // should not be updated directly, but rather by notifying the server, and having the server notify us.
   Vector<RefPtr<ClientInfo> > mClientInfos;

   TeamManager mTeamManager;

   virtual AbstractTeam *getNewTeam() = 0;

public:
   idleLinkedList idlingObjects;

   static const S32 DefaultGridSize = 255;   // Size of "pages", represented by floats for intrapage locations (i.e. pixels per integer)
   static const S32 MIN_GRID_SIZE = 5;       // Ridiculous, it's true, but we step by our minimum value, so we can't make this too high
   static const S32 MAX_GRID_SIZE = 1000;    // A bit ridiculous too...  250-300 seems about right for normal use.  But we'll let folks experiment.

   static const S32 PLAYER_VISUAL_DISTANCE_HORIZONTAL = 600;    // How far player can see normally horizontally...
   static const S32 PLAYER_VISUAL_DISTANCE_VERTICAL = 450;      // ...and vertically

   static const S32 PLAYER_SCOPE_MARGIN = 150;

   static const S32 PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL = 800;    // How far player can see with sensor equipped horizontally...
   static const S32 PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_VERTICAL = 600;      // ...and vertically

   Game(const Address &theBindAddress, GameSettings *settings);   // Constructor
   virtual ~Game();                                               // Destructor

   void setActiveTeamManager(TeamManager *teamManager);

   S32 getClientCount() const;                                    // Total number of players, human and robot
   S32 getPlayerCount() const;                                    // Returns number of human players
   S32 getAuthenticatedPlayerCount() const;                       // Number of authenticated human players
   S32 getRobotCount() const;                                     // Returns number of bots

   ClientInfo *getClientInfo(S32 index) const;
   const Vector<RefPtr<ClientInfo> > *getClientInfos();

   void addToClientList(ClientInfo *clientInfo);                  
   void removeFromClientList(const StringTableEntry &name);       // Client side
   void removeFromClientList(ClientInfo *clientInfo);             // Server side
   void clearClientList();

   void setAddTarget();
   void clearAddTarget();
   static Game *getAddTarget();

   ClientInfo *findClientInfo(const StringTableEntry &name);      // Find client by name
   
   Rect getWorldExtents();

   virtual bool isTestServer();                                   // Overridden in ServerGame

   virtual const Color *getTeamColor(S32 teamId) const;

   static const ModuleInfo *getModuleInfo(ShipModule module);
   
   void computeWorldObjectExtents();
   Rect computeBarrierExtents();

   Point computePlayerVisArea(Ship *ship) const;

   U32 getTimeUnconnectedToMaster();
   void onConnectedToMaster();

   void resetLevelInfo();

   // Manage bot lists
   Robot *getBot(S32 index);
   S32 getBotCount();
   Robot *findBot(const char *id);                // Find bot with specified script id
   void addBot(Robot *robot);
   void removeBot(Robot *robot);
   void deleteBot(const StringTableEntry &name);  // Delete by name 
   void deleteBot(S32 i);                         // Delete by index
   void deleteAllBots();                          // Delete 'em all, let God sort 'em out!


   virtual void processLevelLoadLine(U32 argc, S32 id, const char **argv, GridDatabase *database, const string &levelFileName);  
   bool processLevelParam(S32 argc, const char **argv);
   string toString();

   virtual bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database, S32 id) = 0;
   virtual void addPolyWall(PolyWall *polyWall, GridDatabase *database) = 0;     
   virtual void addWallItem(WallItem *wallItem, GridDatabase *database) = 0;     

   virtual void deleteLevelGen(LuaLevelGenerator *levelgen) = 0; 


   void setGameTime(F32 time);                                          // Only used during level load process

   void addToDeleteList(BfObject *theObject, U32 delay);

   void deleteObjects(U8 typeNumber);
   void deleteObjects(TestFunc testFunc);

   F32 getGridSize() const;
   void setGridSize(F32 gridSize);

   U32 getCurrentTime();
   virtual bool isServer() = 0;              // Will be overridden by either clientGame (return false) or serverGame (return true)

   void checkConnectionToMaster(U32 timeDelta);
   MasterServerConnection *getConnectionToMaster();

   void runAnonymousMasterRequest(MasterConnectionCallback callback);
   void processAnonymousMasterConnection();

   GameNetInterface *getNetInterface();
   virtual GridDatabase *getGameObjDatabase();

   const Vector<SafePtr<BfObject> > &getScopeAlwaysList();

   void setScopeAlwaysObject(BfObject *theObject);
   GameType *getGameType() const;

   // Team functions
   S32 getTeamCount() const;
   
   AbstractTeam *getTeam(S32 teamIndex) const;

   Vector<Team *> *getSortedTeamList_score() const;
   bool getTeamHasFlag(S32 teamIndex) const;

   S32 getTeamIndex(const StringTableEntry &playerName);
   S32 getTeamIndexFromTeamName(const char *teamName) const;

   void countTeamPlayers();      // Makes sure that the mTeams[] structure has the proper player counts

   void addTeam(AbstractTeam *team);
   void addTeam(AbstractTeam *team, S32 index);
   void replaceTeam(AbstractTeam *team, S32 index);
   void removeTeam(S32 teamIndex);
   void clearTeams();

   void setTeamHasFlag(S32 teamIndex, bool hasFlag);
   void clearTeamHasFlagList();


   StringTableEntry getTeamName(S32 teamIndex) const;   // Return the name of the team

   void setGameType(GameType *theGameType);
   void processDeleteList(U32 timeDelta);

   GameSettings *getSettings();

   bool isSuspended();
   bool isOrIsAboutToBeSuspended();

   void resetMasterConnectTimer();

   void setReadyToConnectToMaster(bool ready);

   // Objects in a given level, used for status bar.  On server it's objects loaded from file, on client, it's objects dl'ed from server.
   S32 mObjectsLoaded;  

   Point getScopeRange(bool sensorEquipped);

   string makeUnique(const char *name);
};


////////////////////////////////////////
////////////////////////////////////////

struct LevelInfo
{   
private:
   void initialize();      // Called by constructors

public:
   StringTableEntry mLevelFileName;  // File level is stored in
   StringTableEntry mLevelName;      // Level "in-game" names

   GameTypeId mLevelType;      
   S32 minRecPlayers;               // Min recommended number of players for this level
   S32 maxRecPlayers;               // Max recommended number of players for this level

   LevelInfo();      // Default constructor used on server side

   // Constructor, used on client side where we don't care about min/max players
   LevelInfo(const StringTableEntry &name, GameTypeId type);

   // Constructor, used on server side, augmented with setInfo method below
   LevelInfo(const string &levelFile);

   const char *getLevelTypeName();
};


};


#endif

