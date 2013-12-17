//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SERVER_GAME_H_
#define _SERVER_GAME_H_

#include "game.h"                // Parent class

#include "BotNavMeshZone.h"
#include "dataConnection.h"
#include "LevelSource.h"         // For LevelSourcePtr def
#include "LevelSpecifierEnum.h"

#include "Intervals.h"

using namespace std;

namespace Zap
{

class LuaLevelGenerator;
class LuaGameInfo;
class Robot;
class PolyWall;
class WallItem;
class ItemSpawn;
struct LevelInfo;

static const string UploadPrefix = "upload_";
static const string DownloadPrefix = "download_";

class ServerGame : public Game
{
   typedef Game Parent;

private:
   enum {
      UpdateServerStatusTime = TWENTY_SECONDS,    // How often we update our status on the master server (ms)
      CheckServerStatusTime = FIVE_SECONDS,       // If it did not send updates, recheck after ms
      BotControlTickInterval = 33,                // Interval for how often should we let bots fire the onTick event (ms)
   };

   bool mTestMode;                        // True if being tested from editor

   GridDatabase mDatabaseForBotZones;     // Database especially for BotZones to avoid gumming up the regular database with too many objects

   LevelSourcePtr mLevelSource;

   U32 mCurrentLevelIndex;                // Index of level currently being played
   Timer mLevelSwitchTimer;               // Track how long after game has ended before we actually switch levels
   Timer mMasterUpdateTimer;              // Periodically let the master know how we're doing
   bool mShuttingDown;
   Timer mShutdownTimer;
   SafePtr<GameConnection> mShutdownOriginator;   // Who started the shutdown?

   bool mDedicated;
   bool mAutoAddBots;
   S32 mMinPlayerCount;             // Target number of players when we are auto-adding bots          

   S32 mLevelLoadIndex;             // For keeping track of where we are in the level loading process.  NOT CURRENT LEVEL IN PLAY!

   SafePtr<GameConnection> mSuspendor;            // Player requesting suspension if game suspended by request
   Timer mTimeToSuspend;
public:
   static const U32 PreSuspendSettlingPeriod = TWO_SECONDS;
private:

   // For simulating CPU stutter
   Timer mStutterTimer;                   
   Timer mStutterSleepTimer;
   U32 mAccumulatedSleepTime;

   Vector<LuaLevelGenerator *> mLevelGens;
   Vector<LuaLevelGenerator *> mLevelGenDeleteList;


   Vector<string> mSentHashes;            // Hashes of levels already sent to master

   void updateStatusOnMaster();           // Give master a status report for this server
   void processVoting(U32 timeDelta);     // Manage any ongoing votes
   void processSimulatedStutter(U32 timeDelta);

   string getLevelFileNameFromIndex(S32 indx);


   void resetAllClientTeams();                        // Resets all player team assignments

   bool onlyClientIs(GameConnection *client);

   void cleanUp();
   bool loadLevel();                                  // Load the level pointed to by mCurrentLevelIndex
   void runLevelGenScript(const string &scriptName);  // Run any levelgens specified by the level or in the INI

   AbstractTeam *getNewTeam();

   RefPtr<NetEvent> mSendLevelInfoDelayNetInfo;
   Timer mSendLevelInfoDelayCount;

   void clearBotMoves();

   Timer botControlTickTimer;

   LuaGameInfo *mGameInfo;

   GridDatabase *mBotZoneDatabase;
   Vector<BotNavMeshZone *> mAllZones;

public:
   ServerGame(const Address &address, GameSettingsPtr settings, LevelSourcePtr levelSource, bool testMode, bool dedicated);    // Constructor
   virtual ~ServerGame();   // Destructor

   U32 mInfoFlags;           // Not used for much at the moment, but who knows? --> propagates to master

   enum VoteType {
      VoteLevelChange,
      VoteAddTime,
      VoteSetTime,
      VoteSetScore,
      VoteChangeTeam,
      VoteResetScore,
   };

   // These are public so this can be accessed by tests
   static const U32 MaxTimeDelta = TWO_SECONDS;     
   static const U32 LevelSwitchTime = FIVE_SECONDS;

   U32 mVoteTimer;
   VoteType mVoteType;
   S32 mVoteYes;
   S32 mVoteNo;
   S32 mVoteNumber;
   S32 mNextLevel;

   StringTableEntry mVoteClientName;
   bool voteStart(ClientInfo *clientInfo, VoteType type, S32 number = 0);
   void voteClient(ClientInfo *clientInfo, bool voteYes);

   bool startHosting();

   U32 getMaxPlayers() const;

   bool isTestServer() const;
   bool isDedicated() const;
   void setDedicated(bool dedicated);

   bool isFull();      // More room at the inn?

   void addClient(ClientInfo *clientInfo);
   void removeClient(ClientInfo *clientInfo);

   void setShuttingDown(bool shuttingDown, U16 time, GameConnection *who, StringPtr reason);  

   void resetLevelLoadIndex();
   string loadNextLevelInfo();
   bool populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo);

   void deleteLevelGen(LuaLevelGenerator *levelgen);     // Add misbehaved levelgen to the kill list

   bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database, S32 id);

   void addPolyWall(BfObject *polyWall, GridDatabase *database);
   void addWallItem(BfObject *wallItem, GridDatabase *database);

   void cycleLevel(S32 newLevelIndex = NEXT_LEVEL);
   void sendLevelStatsToMaster();

   void onConnectedToMaster();

   /////
   // Bot related
   void startAllBots();                            // Loop through all our bots and run thier main() functions
   
   void addBot(Robot *robot);
   S32 getBotCount() const;

   bool getAutoAddBots() const;
   void setAutoAddBots(bool addBots);

   S32 getMinPlayerCount() const;
   void setMinPlayerCount(S32 count);
   /////


   StringTableEntry getLevelNameFromIndex(S32 indx);
   S32 getAbsoluteLevelIndex(S32 indx);            // Figures out the level index if the input is a relative index

   string getCurrentLevelFileName() const;         // Return filename of level currently in play  
   StringTableEntry getCurrentLevelName() const;   // Return name of level currently in play
   GameTypeId getCurrentLevelType();               // Return type of level currently in play
   StringTableEntry getCurrentLevelTypeName();     // Return name of type of level currently in play

   bool isServer() const;
   void idle(U32 timeDelta);
   bool isReadyToShutdown(U32 timeDelta);
   void gameEnded();

   S32 getCurrentLevelIndex();
   S32 getLevelCount();
   LevelInfo getLevelInfo(S32 index);
   void clearLevelInfos();
   void sendLevelListToLevelChangers(const string &message = "");

   DataSender dataSender;

   bool isOrIsAboutToBeSuspended();
   bool clientCanSuspend(ClientInfo *info);
   void suspendGame();
   void suspendGame(GameConnection *gc);
   void unsuspendGame(bool remoteRequest);

   void suspenderLeftGame();
   GameConnection *getSuspendor();
   void suspendIfNoActivePlayers(bool delaySuspend = false);
   void unsuspendIfActivePlayers();

   Ship *getLocalPlayerShip() const;

   S32 addLevel(const LevelInfo &info);
   void removeLevel(S32 index);

   // SFX Related -- these will just generate an error, as they should never be called
   SFXHandle playSoundEffect(U32 profileIndex, F32 gain = 1.0f) const;
   SFXHandle playSoundEffect(U32 profileIndex, const Point &position) const;
   SFXHandle playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain = 1.0f) const;
   void queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p) const;

   LuaGameInfo *getGameInfo();

   /////
   // BotNavMeshZone management
   GridDatabase *getBotZoneDatabase() const;
   const Vector<BotNavMeshZone *> *getBotZones() const;
   U16 findZoneContaining(const Point &p) const;

   friend class ObjectTest;

};


}


#endif

