//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAME_H_
#define _GAME_H_

#include "gameLoader.h"          // Parent class

#include "GameTypesEnum.h"
#include "DismountModesEnum.h"

#include "GameSettings.h"
#include "SoundEffect.h"         // For SFXHandle

#include "teamInfo.h"            // For ClassManager
#include "BfObject.h"            // For TypeNumber def
#include "md5wrapper.h"

#include "Timer.h"
#include "Rect.h"

#include "tnlNetObject.h"
#include "tnlTypes.h"
#include "tnlThread.h"
#include "tnlNonce.h"

#include "boost/smart_ptr/shared_ptr.hpp"

#include <string>

namespace Master
{
   class DatabaseAccessThread;
}


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

const U32 MAX_GAME_NAME_LEN = 32;      // Any longer, and it won't fit on-screen
const U32 MAX_GAME_DESCR_LEN = 60;     // Any longer, and it won't fit on-screen; also limits max length of credits string

////////////////////////////////////////
////////////////////////////////////////

// Some forward declarations
class AnonymousMasterServerConnection;
class MasterServerConnection;
class FlagItem;
class GameNetInterface;
class GameType;
class BfObject;
class GameConnection;
class Ship;
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

   explicit NameToAddressThread(const char *address_string);  // Constructor
   virtual ~NameToAddressThread();                            // Destructor

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
class Teleporter;
class MoveItem;
class AbstractSpawn;

struct WallRec;


class Game
{
private:
   F32 mLegacyGridSize;
   U32 mLevelFormat;      // Version of level file loaded.  Used for legacy analyses
   bool mHasLevelFormat;

   static const U32 CurrentLevelFormat;

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
   Master::DatabaseAccessThread *mSecondaryThread;

protected:
   U32 mNextMasterTryTime;

   bool mReadyToConnectToMaster;

   Vector<Robot *> mRobots;               // Grand master list of all robots in the current game
   Rect mWorldExtents;                    // Extents of everything
   string mLevelFileHash;                 // MD5 hash of level file

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

   U32 mLevelDatabaseId;

   RefPtr<GameNetInterface> mNetInterface;

   SafePtr<MasterServerConnection> mConnectionToMaster;

   // Not really a queue, but good enough for now!
   SafePtr<AnonymousMasterServerConnection> mAnonymousMasterServerConnection;

   SafePtr<GameType> mGameType;

   bool mGameSuspended;       // True if we're in "suspended animation" mode

   GameSettingsPtr mSettings;

   S32 findClientIndex(const StringTableEntry &name);

   // On the Client, this list will track info about every player in the game.  Note that the local client will also be represented here,
   // but the info in these records will only be managed by the server.  E.g. if the local client's name changes, the client's record
   // should not be updated directly, but rather by notifying the server, and having the server notify us.
   Vector<RefPtr<ClientInfo> > mClientInfos;

   TeamManager mTeamManager;

   virtual AbstractTeam *getNewTeam() = 0;

public:
   static const S32 MAX_TEAMS = 9;           // Max teams allowed -- careful changing this; used for RPC ranges

   static const S32 PLAYER_VISUAL_DISTANCE_HORIZONTAL = 600;    // How far player can see normally horizontally...
   static const S32 PLAYER_VISUAL_DISTANCE_VERTICAL = 450;      // ...and vertically

   static const S32 PLAYER_SCOPE_MARGIN = 150;

   static const S32 PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL = 800;    // How far player can see with sensor equipped horizontally...
   static const S32 PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_VERTICAL = 600;      // ...and vertically

   static md5wrapper md5;

   Game(const Address &theBindAddress, GameSettingsPtr settings);   // Constructor
   virtual ~Game();                                                 // Destructor

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

   bool isSuspended() const;

   virtual string getCurrentLevelFileName() const;
   virtual U32 getMaxPlayers() const;
   virtual bool isDedicated() const;
   virtual bool isTestServer() const = 0;

   virtual void gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken);
   virtual void gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                                U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired);

   virtual void displayMessage(const Color &msgColor, const char *format, ...) const;
   virtual ClientInfo *getLocalRemoteClientInfo() const;
   virtual void quitEngineerHelper();


   ClientInfo *findClientInfo(const StringTableEntry &name);      // Find client by name
   Ship *findShip(const StringTableEntry &clientName);            // Find ship by name
   
   const Rect *getWorldExtents() const;

   virtual const Color *getTeamColor(S32 teamId) const;

   void computeWorldObjectExtents();
   Rect computeBarrierExtents();

   Point computePlayerVisArea(Ship *ship) const;
   virtual Point worldToScreenPoint(const Point *p, S32 canvasWidth, S32 canvasHeight) const;
   virtual F32 getCommanderZoomFraction() const;
   virtual void renderBasicInterfaceOverlay() const;
   virtual void emitTextEffect(const string &text, const Color &color, const Point &pos) const;


   U32 getTimeUnconnectedToMaster();
   virtual void onConnectedToMaster();

   void resetLevelInfo();

   // Manage bot lists
   Robot *getBot(S32 index);
   virtual S32 getBotCount() const;
   Robot *findBot(const char *id);                // Find bot with specified script id
   virtual void addBot(Robot *robot);
   void removeBot(Robot *robot);
   void deleteBot(const StringTableEntry &name);  // Delete by name 
   void deleteBot(S32 i);                         // Delete by index
   void deleteBotFromTeam(S32 teamIndex);         // Delete by teamIndex
   void deleteAllBots();                          // Delete 'em all, let God sort 'em out!

   void loadLevelFromString(const string &contents, GridDatabase *database, const string& filename = "");
   bool loadLevelFromFile(const string &filename, GridDatabase *database);
   void parseLevelLine(const char *line, GridDatabase *database, const string &levelFileName);

   void processLevelLoadLine(U32 argc, S32 id, const char **argv, GridDatabase *database, const string &levelFileName);  
   bool processLevelParam(S32 argc, const char **argv);
   string toLevelCode() const;

   virtual bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database, S32 id) = 0;

   virtual void addPolyWall(BfObject *polyWall, GridDatabase *database);     
   virtual void addWallItem(BfObject *wallItem, GridDatabase *database);     

   void addWall(const WallRec &barrier);

   virtual void deleteLevelGen(LuaLevelGenerator *levelgen) = 0; 


   virtual Ship *getLocalPlayerShip() const = 0;

   void setGameTime(F32 timeInMinutes);      // Used by test and lua

   void addToDeleteList(BfObject *theObject, U32 delay);

   void deleteObjects(U8 typeNumber);
   void deleteObjects(TestFunc testFunc);

   F32 getLegacyGridSize() const;

   U32 getCurrentTime();
   virtual bool isServer() const = 0;        // Implemented by ClientGame (returns false) and ServerGame (returns true)

   void checkConnectionToMaster(U32 timeDelta);
   MasterServerConnection *getConnectionToMaster();
   void setConnectionToMaster(MasterServerConnection *m);

   void runAnonymousMasterRequest(MasterConnectionCallback callback);
   void processAnonymousMasterConnection();

   GameNetInterface *getNetInterface();
   virtual GridDatabase *getGameObjDatabase();

   const Vector<SafePtr<BfObject> > &getScopeAlwaysList();

   void setScopeAlwaysObject(BfObject *theObject);
   GameType *getGameType() const;

   // MD5 utilties
   string getSaltedHash(const string &stringToBeHashed) const;


   // Team functions
   S32 getTeamCount() const;
   
   AbstractTeam *getTeam(S32 teamIndex) const;

   bool getTeamHasFlag(S32 teamIndex) const;

   S32 getTeamIndex(const StringTableEntry &playerName);
   S32 getTeamIndexFromTeamName(const char *teamName) const;

   void countTeamPlayers() const;      // Makes sure that the mTeams[] structure has the proper player counts

   void addTeam(AbstractTeam *team);
   void addTeam(AbstractTeam *team, S32 index);
   void replaceTeam(AbstractTeam *team, S32 index);
   void removeTeam(S32 teamIndex);
   void clearTeams();

   void setTeamHasFlag(S32 teamIndex, bool hasFlag);
   void clearTeamHasFlagList();

   F32 getShipAccelModificationFactor(const Ship *ship) const;
   void teleporterDestroyed(Teleporter *teleporter);

   StringTableEntry getTeamName(S32 teamIndex) const;   // Return the name of the team

   void setGameType(GameType *theGameType);
   void processDeleteList(U32 timeDelta);

   GameSettings   *getSettings() const;
   GameSettingsPtr getSettingsPtr() const;


   void setReadyToConnectToMaster(bool ready);

   // Objects in a given level, used for status bar.  On server it's objects loaded from file, on client, it's objects dl'ed from server.
   S32 mObjectsLoaded;  

   Point getScopeRange(bool sensorEquipped);

   string makeUnique(const char *name);

   virtual void setLevelDatabaseId(U32 id);
   U32 getLevelDatabaseId() const;

   virtual string getPlayerName() const;

   // A couple of statics to keep gServerGame out of some classes
   static const GridDatabase *getServerGameObjectDatabase();

   // Passthroughs to GameType
   void onFlagMounted(S32 teamIndex);
   void itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode);
   const Color *getObjTeamColor(const BfObject *obj) const;
   bool objectCanDamageObject(BfObject *damager, BfObject *victim) const;
   void releaseFlag(const Point &pos, const Point &vel = Point(0,0), const S32 count = 1) const;
   S32 getRenderTime() const;
   Vector<AbstractSpawn *> getSpawnPoints(TypeNumber typeNumber, S32 teamIndex);
   void addFlag(FlagItem *flag);
   void shipTouchFlag(Ship *ship, FlagItem *flag);
   void shipTouchZone(Ship *ship, GoalZone *zone);
   bool isTeamGame() const;
   Timer &getGlowZoneTimer();
   S32 getGlowingZoneTeam();
   string getScriptName() const;
   bool levelHasLoadoutZone();
   void updateShipLoadout(BfObject *shipObject);

   void sendChat(const StringTableEntry &senderName, ClientInfo *senderClientInfo, const StringPtr &message, bool global, S32 teamIndex);
   void sendPrivateChat(const StringTableEntry &senderName, const StringTableEntry &receiverName, const StringPtr &message);
   void sendAnnouncementFromController(const StringPtr &message);

   S32 getRemainingGameTime() const;        // In seconds

   void updateClientChangedName(ClientInfo *clientInfo, StringTableEntry newName);
   bool objectCanDamageObject(BfObject *damager, BfObject *victim);

   virtual SFXHandle playSoundEffect(U32 profileIndex, F32 gain = 1.0f) const = 0;
   virtual SFXHandle playSoundEffect(U32 profileIndex, const Point &position) const = 0;
   virtual SFXHandle playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain = 1.0f) const = 0;
   virtual void queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p) const = 0;

   // Misc junk
   static void seedRandomNumberGenerator(const string &name);

   virtual void addInlineHelpItem(HelpItem item) const;
   virtual void removeInlineHelpItem(HelpItem item, bool markAsSeen) const;
   virtual F32 getObjectiveArrowHighlightAlpha() const;

    Master::DatabaseAccessThread *getSecondaryThread();
};


};


#endif

