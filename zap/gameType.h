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

#ifndef _GAMETYPE_H_
#define _GAMETYPE_H_

#include "Timer.h"
#include "flagItem.h"
#include "teamInfo.h"
#include "gameStats.h"           // For VersionedGameStats
#include "statistics.h"
#include "Spawn.h"

#include "gameConnection.h"      // For MessageColors enum
#include "GameTypesEnum.h"

#include <string>
#include <boost/shared_ptr.hpp>

#include <map>


namespace Zap
{

// Some forward declarations
class GoalZone;
class CoreItem;
class MenuItem;
class MoveItem;
class ClientGame;
class Robot;
class AsteroidSpawn;
class Team;
class SpyBug;
class MenuUserInterface;
class Zone;

struct WallRec;


////////////////////////////////////////
////////////////////////////////////////

class GameTimer
{
private:
   Timer mTimer;
   bool mIsUnlimited;
   bool mGameOver;
   S32 mRenderingOffset;

public:
   void reset(U32 timeInMs);
   void sync(U32 deltaT);
   bool update(U32 deltaT);
   void extend(S32 deltaT);

   bool isUnlimited() const;
   void setIsUnlimited(bool isUnlimited);
   U32 getCurrent() const; 
   U32 getTotalGameTime() const;
   S32 getRenderingOffset() const;
   void setRenderingOffset(S32 offset);
   void setGameIsOver();

   string toString_minutes() const;      // Creates string representation of timer for level saving purposes
};


////////////////////////////////////////
////////////////////////////////////////

/**
 * @luaenum ScoringEvent(1,1)
 * The ScoringEvent enum represents different actions that change the score.
 */
#define SCORING_EVENT_TABLE \
   SCORING_EVENT_ITEM(KillEnemy,               "KillEnemy")               /* all games                                 */ \
   SCORING_EVENT_ITEM(KillSelf,                "KillSelf")                /* all games                                 */ \
   SCORING_EVENT_ITEM(KillTeammate,            "KillTeammate")            /* all games                                 */ \
   SCORING_EVENT_ITEM(KillEnemyTurret,         "KillEnemyTurret")         /* all games                                 */ \
   SCORING_EVENT_ITEM(KillOwnTurret,           "KillOwnTurret")           /* all games                                 */ \
                                                                                                                          \
   SCORING_EVENT_ITEM(KilledByAsteroid,        "KilledByAsteroid")        /* all games                                 */ \
   SCORING_EVENT_ITEM(KilledByTurret,          "KilledByTurret")          /* all games                                 */ \
                                                                                                                          \
   SCORING_EVENT_ITEM(CaptureFlag,             "CaptureFlag")             /*                                           */ \
   SCORING_EVENT_ITEM(CaptureZone,             "CaptureZone")             /* zone control -> gain zone                 */ \
   SCORING_EVENT_ITEM(UncaptureZone,           "UncaptureZone")           /* zone control -> lose zone                 */ \
   SCORING_EVENT_ITEM(HoldFlagInZone,          "HoldFlagInZone")          /* htf                                       */ \
   SCORING_EVENT_ITEM(RemoveFlagFromEnemyZone, "RemoveFlagFromEnemyZone") /* htf                                       */ \
   SCORING_EVENT_ITEM(RabbitHoldsFlag,         "RabbitHoldsFlag")         /* rabbit, called every second               */ \
   SCORING_EVENT_ITEM(RabbitKilled,            "RabbitKilled")            /* rabbit                                    */ \
   SCORING_EVENT_ITEM(RabbitKills,             "RabbitKills")             /* rabbit                                    */ \
   SCORING_EVENT_ITEM(ReturnFlagsToNexus,      "ReturnFlagsToNexus")      /* nexus game                                */ \
   SCORING_EVENT_ITEM(ReturnFlagToZone,        "ReturnFlagToZone")        /* retrieve -> flag returned to zone         */ \
   SCORING_EVENT_ITEM(LostFlag,                "LostFlag")                /* retrieve -> enemy took flag               */ \
   SCORING_EVENT_ITEM(ReturnTeamFlag,          "ReturnTeamFlag")          /* ctf -> holds enemy flag, touches own flag */ \
   SCORING_EVENT_ITEM(ScoreGoalEnemyTeam,      "ScoreGoalEnemyTeam")      /* soccer                                    */ \
   SCORING_EVENT_ITEM(ScoreGoalHostileTeam,    "ScoreGoalHostileTeam")    /* soccer                                    */ \
   SCORING_EVENT_ITEM(ScoreGoalOwnTeam,        "ScoreGoalOwnTeam")        /* soccer -> score on self                   */ \
   SCORING_EVENT_ITEM(EnemyCoreDestroyed,      "EnemyCoreDestroyed")      /* core -> enemy core is destroyed           */ \
   SCORING_EVENT_ITEM(OwnCoreDestroyed,        "OwnCoreDestroyed")        /* core -> own core is destroyed             */ \


class GameType : public NetObject
{
   typedef NetObject Parent;

   static const S32 TeamNotSpecified = -99999;

private:
   Game *mGame;

   Vector<SafePtr<SpyBug> > mSpyBugs;    // List of all spybugs in the game, could be added and destroyed in-game

   Point getSpawnPoint(S32 team);        // Pick a spawn point for ship or robot


   bool mLevelHasLoadoutZone;
   bool mLevelHasPredeployedFlags;
   bool mLevelHasFlagSpawns;

   bool mShowAllBots;
   U32 mTotalGamePlay;

   Vector<WallRec> mWalls;

   // In-game chat message:
   void sendChat(const StringTableEntry &senderName, ClientInfo *senderClientInfo, const StringPtr &message, bool global);

   S32 mWinningScore;               // Game over when team (or player in individual games) gets this score
   S32 mLeadingTeam;                // Team with highest score
   S32 mLeadingTeamScore;           // Score of mLeadingTeam
   S32 mLeadingPlayer;              // Player index of mClientInfos with highest score
   S32 mLeadingPlayerScore;         // Score of mLeadingPlayer
   S32 mSecondLeadingPlayer;        // Player index of mClientInfos with highest score
   S32 mSecondLeadingPlayerScore;   // Score of mLeadingPlayer
   S32 mDigitsNeededToDisplayScore; // Digits needed to display scores

   bool mCanSwitchTeams;            // Player can switch teams when this is true, not when it is false
   bool mBetweenLevels;             // We'll need to prohibit certain things (like team changes) when game is in an "intermediate" state
   bool mGameOver;                  // Set to true when an end condition is met

   bool mEngineerEnabled;
   bool mEngineerUnrestrictedEnabled;
   bool mBotsAllowed;
   bool mBotBalancingDisabled;

   // Info about current level
   StringTableEntry mLevelName;
   StringTableEntry mLevelDescription;
   StringTableEntry mLevelCredits;

   string mScriptName;                 // Name of levelgen script, if any
   Vector<string> mScriptArgs;         // List of script params  

   S32 mMinRecPlayers;         // Recommended min players for this level
   S32 mMaxRecPlayers;         // Recommended max players for this level

   Vector<SafePtr<MoveItem> > mCacheResendItem;  // Speed up c2sResendItemStatus

   void idle_client(U32 deltaT);
   void idle_server(U32 deltaT);

protected:
   Timer mScoreboardUpdateTimer;

   GameTimer mGameTimer;               // Track when current game will end
   Timer mGameTimeUpdateTimer;         // Timer for when to send clients a game clock update
   Timer mBotBalanceAnalysisTimer;     // Analyze if we need to add/remove bots to balance team
                       
   virtual void syncTimeRemaining(U32 timeLeft);
   virtual void setTimeRemaining(U32 timeLeft, bool isUnlimited);                         // Runs on server
   virtual void setTimeRemaining(U32 timeLeft, bool isUnlimited, S32 renderingOffset);    // Runs on client

   void notifyClientsWhoHasTheFlag();           // Notify the clients when flag status changes... only called by some game types (server only)
   bool doTeamHasFlag(S32 teamIndex) const;     // Do the actual work of figuring out if the specified team has the flag  (server only)
   void updateWhichTeamsHaveFlags();         


public:
   // Define an enum of scoring events from the values in SCORING_EVENT_TABLE
   enum ScoringEvent {
      #define SCORING_EVENT_ITEM(value, b) value,
         SCORING_EVENT_TABLE
      #undef SCORING_EVENT_ITEM
         ScoringEventsCount
   };

   static const S32 MAX_GAME_TIME = S32_MAX;

   static const S32 MAX_TEAMS = 9;                                   // Max teams allowed -- careful changing this; used for RPC ranges
   static const S32 gFirstTeamNumber = -2;                           // First team is "Hostile to All" with index -2
   static const U32 gMaxTeamCount = MAX_TEAMS - gFirstTeamNumber;    // Number of possible teams, including Neutral and Hostile to All
   static const char *validateGameType(const char *gtype);           // Returns a valid gameType, defaulting to base class if needed

   Game *getGame() const;
   bool onGhostAdd(GhostConnection *theConnection);
   void onGhostRemove();

   void broadcastTimeSyncSignal();                     // Send remaining time to all clients
   void broadcastNewRemainingTime();                   // Send remaining time to all clients after time has been updated

   void addZone(Zone *zone);

   const char *getGameTypeName() const;   

   virtual GameTypeId getGameTypeId() const;
   virtual const char *getShortName() const;          // Will be overridden by other games
   virtual const char *getInstructionString() const;  //          -- ditto --
   virtual bool isTeamGame() const;                   // Team game if we have teams.  Otherwise it's every man for himself.
   virtual bool canBeTeamGame() const;
   virtual bool canBeIndividualGame() const;
   virtual bool teamHasFlag(S32 teamIndex) const;

   virtual void onFlagMounted(S32 teamIndex);         // A flag was picked up by a ship on the specified team
   virtual void onFlagDismounted();                   // A flag was dropped by a ship

   S32 getWinningScore() const;
   void setWinningScore(S32 score);

   Vector<AbstractSpawn *> getSpawnPoints(TypeNumber typeNumber, S32 teamIndex = TeamNotSpecified);


   // Info about the level itself
   bool hasFlagSpawns() const;      
   bool hasPredeployedFlags() const;

   void setGameTime(F32 timeInSeconds);

   U32 getTotalGameTime() const;            // In seconds
   S32 getRemainingGameTime() const;        // In seconds
   S32 getRemainingGameTimeInMs() const;    // In ms
   S32 getRenderingOffset() const;
   bool isTimeUnlimited() const;
   void extendGameTime(S32 timeInMs);

   S32 getLeadingScore() const;
   S32 getLeadingTeam() const;
   S32 getLeadingPlayerScore() const;
   S32 getLeadingPlayer() const;
   S32 getSecondLeadingPlayerScore() const;
   S32 getSecondLeadingPlayer() const;

   void catalogSpybugs();           // Build a list of spybugs in the game
   void addSpyBug(SpyBug *spybug);

   void addWall(const WallRec &barrier, Game *game);

   virtual bool isFlagGame() const;      // Does game use flags?
   virtual bool isTeamFlagGame() const;  // Does flag-team orientation matter?  Only false in HunterGame.
   virtual S32 getFlagCount();     // Return the number of game-significant flags

   virtual bool isCarryingItems(Ship *ship);     // Nexus game will override this

   // Some games may place restrictions on when players can fire or use modules
   virtual bool onFire(Ship *ship);

   virtual bool isSpawnWithLoadoutGame();  // We do not spawn with our loadout, but instead need to pass through a loadout zone

   F32 getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips);

   Vector<SafePtr<FlagItem> > mFlags;    // List of flags for those games that keep lists of flags (retrieve, HTF, CTF)

   static void printRules();             // Dump game-rule info

   bool levelHasLoadoutZone();           // Does the level have a loadout zone?

   bool advanceGameClock(U32 deltaT);

   // Game-specific location for the bottom of the scoreboard on the lower-right corner
   // (because games like nexus have more stuff down there we need to look out for)
   virtual U32 getLowerRightCornerScoreboardOffsetFromBottom() const;

   enum
   {
      RespawnDelay = 1500,
      SwitchTeamsDelay = 60000,   // Time between team switches (ms) -->  60000 = 1 minute
      naScore = -99999,           // Score representing a nonsesical event
      NO_FLAG = -1,               // Constant used for ship not having a flag
   };


   const Vector<WallRec> *getBarrierList();

   Rect mViewBoundsWhileLoading;    // Show these view bounds while loading the map
   S32 mObjectsExpected;            // Count of objects we expect to get with this level (for display purposes only)

   struct ItemOfInterest
   {
      SafePtr<MoveItem> theItem;
      U32 teamVisMask;        // Bitmask, where 1 = object is visible to team in that position, 0 if not
   };

   Vector<ItemOfInterest> mItemsOfInterest;

   void addItemOfInterest(MoveItem *theItem);

   S32 getDigitsNeededToDisplayScore() const;

   void broadcastMessage(GameConnection::MessageColors color, SFXProfiles sfx, const StringTableEntry &formatString);

   void broadcastMessage(GameConnection::MessageColors color, SFXProfiles sfx, 
                         const StringTableEntry &formatString, const Vector<StringTableEntry> &e);

   bool isGameOver() const;

   static const char *getGameTypeName(GameTypeId gameType);       // Return string like "Capture The Flag"
   static const char *getGameTypeClassName(GameTypeId gameType);  // Return string like "CTFGameType"

   static Vector<string> getGameTypeNames();

   bool mHaveSoccer;                // Does level have soccer balls? used to determine weather or not to send s2cSoccerCollide

   bool mBotZoneCreationFailed;

   enum
   {
      MaxPing = 999,
      DefaultGameTime = 10 * 60 * 1000,
      DefaultWinningScore = 8,
   };

   // Some games have extra game parameters.  We need to create a structure to communicate those parameters to the editor so
   // it can make an intelligent decision about how to handle them.  Note that, for now, all such parameters are assumed to be S32.
   struct ParameterDescription
   {
      const char *name;
      const char *units;
      const char *help;
      S32 value;     // Default value for this parameter
      S32 minval;    // Min value for this param
      S32 maxval;    // Max value for this param
   };

   enum ScoringGroup
   {
      IndividualScore,
      TeamScore,
   };

   virtual S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);
   static string getScoringEventDescr(ScoringEvent event);

   // Static vectors used for constructing update RPCs
   static Vector<RangedU32<0, MaxPing> > mPingTimes;
   static Vector<SignedInt<24> > mScores;
   static Vector<SignedFloat<8> > mRatings;

   GameType(S32 winningScore = DefaultWinningScore);    // Constructor
   virtual ~GameType();                                 // Destructor

   virtual void addToGame(Game *game, GridDatabase *database);

   virtual bool processArguments(S32 argc, const char **argv, Game *game);
   virtual string toString() const;

#ifndef ZAP_DEDICATED
   virtual Vector<string> getGameParameterMenuKeys();
   virtual boost::shared_ptr<MenuItem> getMenuItem(const string &key);
   virtual bool saveMenuItem(const MenuItem *menuItem, const string &key);
#endif

   virtual bool processSpecialsParam(const char *param);
   virtual string getSpecialsLine();


   const StringTableEntry *getLevelName() const;
   void setLevelName(const StringTableEntry &levelName);

   const StringTableEntry *getLevelDescription() const;
   void setLevelDescription(const StringTableEntry &levelDescription);

   const StringTableEntry *getLevelCredits() const;
   void setLevelCredits(const StringTableEntry &levelCredits);

   S32 getMinRecPlayers();
   void setMinRecPlayers(S32 minPlayers);

   S32 getMaxRecPlayers();
   void setMaxRecPlayers(S32 maxPlayers);

   bool isEngineerEnabled();
   void setEngineerEnabled(bool enabled);
   bool isEngineerUnrestrictedEnabled();
   void setEngineerUnrestrictedEnabled(bool enabled);

   bool areBotsAllowed();
   void setBotsAllowed(bool allowed);
   
   string getScriptLine() const;
   void setScript(const Vector<string> &args);

   string getScriptName() const;
   const Vector<string> *getScriptArgs();

   void onAddedToGame(Game *theGame);

   void onLevelLoaded();      // Server-side function run once level is loaded from file

   virtual void idle(BfObject::IdleCallPath path, U32 deltaT);

   void gameOverManGameOver();
   VersionedGameStats getGameStats();
   void getSortedPlayerScores(S32 teamIndex, Vector<ClientInfo *> &playerScores) const;
   void saveGameStats();                     // Transmit statistics to the master server

   void achievementAchieved(U8 achievement, const StringTableEntry &playerName);

   virtual void onGameOver();

   void serverAddClient(ClientInfo *clientInfo);         

   void serverRemoveClient(ClientInfo *clientInfo);   // Remove a client from the game

   virtual bool objectCanDamageObject(BfObject *damager, BfObject *victim);
   virtual void controlObjectForClientKilled(ClientInfo *theClient, BfObject *clientObject, BfObject *killerObject);

   virtual bool spawnShip(ClientInfo *clientInfo);
   virtual void spawnRobot(Robot *robot);

   virtual void changeClientTeam(ClientInfo *client, S32 team);     // Change player to team indicated, -1 = cycle teams

#ifndef ZAP_DEDICATED
   virtual void renderInterfaceOverlay(bool scoreboardVisible);
   void renderObjectiveArrow(const BfObject *target, const Color *c, F32 alphaMod = 1.0f) const;
   void renderObjectiveArrow(const Point &p, const Color *c, F32 alphaMod = 1.0f) const;
#endif

   void addTime(U32 time);          // Extend the game by time (in ms)

   void SRV_clientRequestLoadout(ClientInfo *clientInfo, const Vector<U8> &loadout);
   void SRV_updateShipLoadout(BfObject *shipObject); // called from LoadoutZone when a Ship touches the zone
   string validateLoadout(const Vector<U8> &loadout);
   void setClientShipLoadout(ClientInfo *clientInfo, const Vector<U8> &loadout, bool silent = false);


   bool checkTeamRange(S32 team);                     // Team in range? Used for processing arguments.
   bool makeSureTeamCountIsNotZero();                 // Zero teams can cause crashiness
   virtual const Color *getShipColor(Ship *s);        // Get the color of a ship
   virtual const Color *getTeamColor(S32 team) const; // Get the color of a team, based on index
   const Color *getTeamColor(BfObject *theObject);  // Get the color of a team, based on object

   //S32 getTeam(const StringTableEntry &playerName);   // Given a player's name, return their team

   virtual bool isDatabasable();                      // Makes no sense to insert a GameType in our spatial database!

   // gameType flag methods for CTF, Rabbit, Football
   virtual void addFlag(FlagItem *flag);
   virtual void itemDropped(Ship *ship, MoveItem *item);
   virtual void shipTouchFlag(Ship *ship, FlagItem *flag);

   virtual void addZone(GoalZone *zone);
   virtual void shipTouchZone(Ship *ship, GoalZone *zone);

   void queryItemsOfInterest();
   void performScopeQuery(GhostConnection *connection);
   virtual void performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo);

   virtual void onGhostAvailable(GhostConnection *theConnection);
   TNL_DECLARE_RPC(s2cSetLevelInfo, (StringTableEntry levelName, StringTableEntry levelDesc, S32 teamScoreLimit, StringTableEntry levelCreds, 
                                     S32 objectCount, F32 lx, F32 ly, F32 ux, F32 uy, bool levelHasLoadoutZone, bool engineerEnabled, bool engineerAbuseEnabled));
   TNL_DECLARE_RPC(s2cAddWalls, (Vector<F32> barrier, F32 width, bool solid));
   TNL_DECLARE_RPC(s2cAddTeam, (StringTableEntry teamName, F32 r, F32 g, F32 b, U32 score, bool firstTeam));
   TNL_DECLARE_RPC(s2cAddClient, (StringTableEntry clientName, bool isAuthenticated, Int<BADGE_COUNT> badges, 
                                  bool isMyClient, bool isAdmin, bool isLevelChanger, bool isRobot, bool isSpawnDelayed, bool isBusy, bool playAlert, bool showMessage));
   TNL_DECLARE_RPC(s2cClientJoinedTeam, (StringTableEntry clientName, RangedU32<0, MAX_TEAMS> teamIndex, bool showMessage));

   TNL_DECLARE_RPC(s2cClientBecameAdmin,        (StringTableEntry clientName));
   TNL_DECLARE_RPC(s2cClientBecameLevelChanger, (StringTableEntry clientName, bool isLevelChanger));

   TNL_DECLARE_RPC(s2cSyncMessagesComplete, (U32 sequence));
   TNL_DECLARE_RPC(c2sSyncMessagesComplete, (U32 sequence));

   TNL_DECLARE_RPC(s2cSetGameOver, (bool gameOver));
   TNL_DECLARE_RPC(s2cSyncTimeRemaining, (U32 timeLeftInMs));
   TNL_DECLARE_RPC(s2cSetNewTimeRemaining, (U32 timeLeftInMs, bool isUnlimited, S32 renderingOffset));
   TNL_DECLARE_RPC(s2cChangeScoreToWin, (U32 score, StringTableEntry changer));

   TNL_DECLARE_RPC(s2cSendFlagPossessionStatus, (U16 packedBits));

   TNL_DECLARE_RPC(s2cCanSwitchTeams, (bool allowed));

   TNL_DECLARE_RPC(s2cRenameClient, (StringTableEntry oldName,StringTableEntry newName));
   void updateClientChangedName(ClientInfo *clientInfo, StringTableEntry newName);

   TNL_DECLARE_RPC(s2cRemoveClient, (StringTableEntry clientName));

   TNL_DECLARE_RPC(s2cAchievementMessage, (U32 achievement, StringTableEntry clientName));

   // Not all of these actually used?
   void updateScore(Ship *ship, ScoringEvent event, S32 data = 0);              
   void updateScore(ClientInfo *clientInfo, ScoringEvent scoringEvent, S32 data = 0); 
   void updateScore(S32 team, ScoringEvent event, S32 data = 0);
   virtual void updateScore(ClientInfo *player, S32 team, ScoringEvent event, S32 data = 0); // Core uses their own updateScore

   void updateLeadingTeamAndScore();   // Sets mLeadingTeamScore and mLeadingTeam
   void updateLeadingPlayerAndScore(); // Sets mLeadingTeamScore and mLeadingTeam
   void updateRatings();               // Update everyone's game-normalized ratings at the end of the game


   TNL_DECLARE_RPC(s2cSetTeamScore, (RangedU32<0, MAX_TEAMS> teamIndex, U32 score));
   TNL_DECLARE_RPC(s2cSetPlayerScore, (U16 index, S32 score));

   TNL_DECLARE_RPC(c2sRequestScoreboardUpdates, (bool updates));
   TNL_DECLARE_RPC(s2cScoreboardUpdate, (Vector<RangedU32<0, MaxPing> > pingTimes, Vector<SignedInt<24> > scores, Vector<SignedFloat<8> > ratings));

   void updateClientScoreboard(ClientInfo *clientInfo);

   TNL_DECLARE_RPC(c2sChooseNextWeapon, ());
   TNL_DECLARE_RPC(c2sChoosePrevWeapon, ());
   TNL_DECLARE_RPC(c2sSelectWeapon, (RangedU32<0, ShipWeaponCount> index));
   TNL_DECLARE_RPC(c2sDropItem, ());

   // These are used when the client sees something happen and wants a confirmation from the server
   TNL_DECLARE_RPC(c2sResendItemStatus, (U16 itemId));

#ifndef ZAP_DEDICATED
   // Handle additional game-specific menu options for the client and the admin
   virtual void addClientGameMenuOptions(ClientGame *game, MenuUserInterface *menu);
   //virtual void processClientGameMenuOption(U32 index);                        // Param used only to hold team, at the moment

   virtual void addAdminGameMenuOptions(MenuUserInterface *menu);
#endif

   void sendChatFromRobot(bool global, const StringPtr &message, ClientInfo *botClientInfo);
   void sendChatFromController(const StringPtr &message);

   TNL_DECLARE_RPC(c2sAddTime, (U32 time));                                    // Admin is adding time to the game
   TNL_DECLARE_RPC(c2sChangeTeams, (S32 team));                                // Player wants to change teams
   void processClientRequestForChangingGameTime(S32 time, bool isUnlimited, bool changeTimeIfAlreadyUnlimited, S32 voteType);

   TNL_DECLARE_RPC(c2sSendChatPM, (StringTableEntry toName, StringPtr message));                        // using /pm command
   TNL_DECLARE_RPC(c2sSendChat, (bool global, StringPtr message));             // In-game chat
   TNL_DECLARE_RPC(c2sSendChatSTE, (bool global, StringTableEntry ste));       // Quick-chat
   TNL_DECLARE_RPC(c2sSendCommand, (StringTableEntry cmd, Vector<StringPtr> args));

   TNL_DECLARE_RPC(s2cDisplayChatPM, (StringTableEntry clientName, StringTableEntry toName, StringPtr message));
   TNL_DECLARE_RPC(s2cDisplayChatMessage, (bool global, StringTableEntry clientName, StringPtr message));

   // killerName will be ignored if killer is supplied
   TNL_DECLARE_RPC(s2cKillMessage, (StringTableEntry victim, StringTableEntry killer, StringTableEntry killerName));

   TNL_DECLARE_RPC(c2sVoiceChat, (bool echo, ByteBufferPtr compressedVoice));
   TNL_DECLARE_RPC(s2cVoiceChat, (StringTableEntry client, ByteBufferPtr compressedVoice));

   TNL_DECLARE_RPC(c2sSetTime, (U32 time));
   TNL_DECLARE_RPC(c2sSetWinningScore, (U32 score));
   TNL_DECLARE_RPC(c2sResetScore, ());
   TNL_DECLARE_RPC(c2sAddBot, (Vector<StringTableEntry> args));
   TNL_DECLARE_RPC(c2sAddBots, (U32 count, Vector<StringTableEntry> args));
   TNL_DECLARE_RPC(c2sKickBot, ());
   TNL_DECLARE_RPC(c2sKickBots, ());
   TNL_DECLARE_RPC(c2sShowBots, ());
   TNL_DECLARE_RPC(c2sSetMaxBots, (S32 count));
   TNL_DECLARE_RPC(c2sBanPlayer, (StringTableEntry playerName, U32 duration));
   TNL_DECLARE_RPC(c2sBanIp, (StringTableEntry ipAddressString, U32 duration));
   TNL_DECLARE_RPC(c2sRenamePlayer, (StringTableEntry playerName, StringTableEntry newName));
   TNL_DECLARE_RPC(c2sGlobalMutePlayer, (StringTableEntry playerName));
   TNL_DECLARE_RPC(c2sClearScriptCache, ());
   TNL_DECLARE_RPC(c2sTriggerTeamChange, (StringTableEntry playerName, S32 teamIndex));
   TNL_DECLARE_RPC(c2sKickPlayer, (StringTableEntry playerName));

   TNL_DECLARE_RPC(s2cSetIsSpawnDelayed, (StringTableEntry name, bool idle));
   TNL_DECLARE_RPC(s2cSetPlayerEngineeringTeleporter, (StringTableEntry name, bool isEngineeringTeleporter));

   TNL_DECLARE_CLASS(GameType);

   enum
   {
      mZoneGlowTime = 800,    // Time for visual effect, used by Nexus & GoalZone
   };

   Timer mZoneGlowTimer;
   S32 mGlowingZoneTeam;      // Which team's zones are glowing, -1 for all

   virtual void majorScoringEventOcurred(S32 team);    // Gets called when touchdown is scored...  currently only used by zone control & retrieve

   void processServerCommand(ClientInfo *clientInfo, const char *cmd, Vector<StringPtr> args);
   void addBotFromClient(Vector<StringTableEntry> args);

   string addBot(Vector<StringTableEntry> args);
   void balanceTeams();

   map <pair<U16,U16>, Vector<Point> > cachedBotFlightPlans;  // cache of zone-to-zone flight plans, shared for all bots

   GameTimer *getTimer();
};

#define GAMETYPE_RPC_S2C(className, methodName, args, argNames) \
   TNL_IMPLEMENT_NETOBJECT_RPC(className, methodName, args, argNames, NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)

#define GAMETYPE_RPC_C2S(className, methodName, args, argNames) \
   TNL_IMPLEMENT_NETOBJECT_RPC(className, methodName, args, argNames, NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhostParent, 0)

};

#endif
