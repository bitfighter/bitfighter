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

////////////////////////////////////////
////////////////////////////////////////

class Robot;
class AsteroidSpawn;
class Team;
class SpyBug;
class MenuUserInterface;

struct WallRec
{
   Vector<F32> verts;
   F32 width;
   bool solid;

public:
   void constructWalls(Game *theGame);
};


class GameType : public NetObject
{
   typedef NetObject Parent;

private:
   Game *mGame;
   Point getSpawnPoint(S32 team);         // Picks a spawn point for ship or robot

   Vector<SafePtr<Object> > mSpyBugs;    // List of all spybugs in the game, could be added and destroyed in-game
   bool mLevelHasLoadoutZone;
   bool mShowAllBots;
   U32 mTotalGamePlay;

   Vector<WallRec> mWalls;

   void sendChatDisplayEvent(ClientInfo *sender, bool global, const char *message, NetEvent *theEvent);      // In-game chat message

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
   bool mBotsAllowed;

   // Info about current level
   StringTableEntry mLevelName;
   StringTableEntry mLevelDescription;
   StringTableEntry mLevelCredits;

   string mScriptName;                    // Name of levelgen script, if any
   Vector<string> mScriptArgs;            // List of script params  

   S32 mMinRecPlayers;         // Recommended min players for this level
   S32 mMaxRecPlayers;         // Recommended max players for this level

   Vector<FlagSpawn> mFlagSpawnPoints;                        // List of non-team specific spawn points for flags
   Vector<boost::shared_ptr<ItemSpawn> > mItemSpawnPoints;    // List of spawn points for asteroids, circles, etc.

   Timer mScoreboardUpdateTimer;
   Timer mGameTimer;                      // Track when current game will end
   Timer mGameTimeUpdateTimer;

   Vector<SafePtr<MoveItem> > mCacheResendItem;  // speed up c2sResendItemStatus

   void idle_client(U32 deltaT);
   void idle_server(U32 deltaT);

public:
   // Potentially scoring events
   enum ScoringEvent
   {
      KillEnemy,              // all games
      KillSelf,               // all games
      KillTeammate,           // all games
      KillEnemyTurret,        // all games
      KillOwnTurret,          // all games

      KilledByAsteroid,       // all games
      KilledByTurret,         // all games

      CaptureFlag,
      CaptureZone,            // zone control -> gain zone
      UncaptureZone,          // zone control -> lose zone
      HoldFlagInZone,         // htf
      RemoveFlagFromEnemyZone,// htf
      RabbitHoldsFlag,        // rabbit, called every second
      RabbitKilled,           // rabbit
      RabbitKills,            // rabbit
      ReturnFlagsToNexus,     // nexus game
      ReturnFlagToZone,       // retrieve -> flag returned to zone
      LostFlag,               // retrieve -> enemy took flag
      ReturnTeamFlag,         // ctf -> holds enemy flag, touches own flag
      ScoreGoalEnemyTeam,     // soccer
      ScoreGoalHostileTeam,   // soccer
      ScoreGoalOwnTeam,       // soccer -> score on self
      EnemyCoreDestroyed,     // core -> enemy core is destroyed
      OwnCoreDestroyed,       // core -> own core is destroyed
      ScoringEventsCount
   };

   static const S32 MAX_TEAMS = 9;                                   // Max teams allowed -- careful changing this; used for RPC ranges
   static const S32 gFirstTeamNumber = -2;                           // First team is "Hostile to All" with index -2
   static const U32 gMaxTeamCount = MAX_TEAMS - gFirstTeamNumber;    // Number of possible teams, including Neutral and Hostile to All
   static const char *validateGameType(const char *gtype);           // Returns a valid gameType, defaulting to gDefaultGameTypeIndex if needed

   Game *getGame() const;
   bool onGhostAdd(GhostConnection *theConnection);

   static StringTableEntry getGameTypeName(GameTypes gameType);

   virtual GameTypes getGameType() const;
   const char *getGameTypeString() const;       
   virtual const char *getShortName() const;            // Will be overridden by other games
   virtual const char *getInstructionString() const;    //          -- ditto --
   virtual bool isTeamGame() const;                     // Team game if we have teams.  Otherwise it's every man for himself.
   virtual bool canBeTeamGame() const;
   virtual bool canBeIndividualGame() const;
   virtual bool teamHasFlag(S32 teamId) const;
   S32 getWinningScore() const;
   void setWinningScore(S32 score);

   void setGameTime(F32 timeInSeconds);

   U32 getTotalGameTime() const;            // In seconds
   S32 getRemainingGameTime() const;        // In seconds
   S32 getRemainingGameTimeInMs() const;    // In ms
   void extendGameTime(S32 timeInMs);

   S32 getLeadingScore() const;
   S32 getLeadingTeam() const;
   S32 getLeadingPlayerScore() const;
   S32 getLeadingPlayer() const;
   S32 getSecondLeadingPlayerScore() const;
   S32 getSecondLeadingPlayer() const;

   void catalogSpybugs();           // Build a list of spybugs in the game
   void addSpyBug(SpyBug *spybug);

   void addWall(WallRec barrier, Game *game);

   virtual bool isFlagGame();      // Does game use flags?
   virtual bool isTeamFlagGame();  // Does flag-team orientation matter?  Only false in HunterGame.
   virtual S32 getFlagCount();     // Return the number of game-significant flags

   virtual bool isCarryingItems(Ship *ship);     // Nexus game will override this

   // Some games may place restrictions on when players can fire or use modules
   virtual bool onFire(Ship *ship);
   virtual bool okToUseModules(Ship *ship);

   virtual bool isSpawnWithLoadoutGame();  // We do not spawn with our loadout, but instead need to pass through a loadout zone

   F32 getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips);

   Vector<SafePtr<FlagItem> > mFlags;    // List of flags for those games that keep lists of flags (retrieve, HTF, CTF)

   static void printRules();             // Dump game-rule info

   bool levelHasLoadoutZone();           // Does the level have a loadout zone?

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

   S32 getFlagSpawnCount() const;
   const FlagSpawn *getFlagSpawn(S32 index) const;
   const Vector<FlagSpawn> *getFlagSpawns() const;
   void addFlagSpawn(FlagSpawn flagSpawn);
   void addItemSpawn(ItemSpawn *spawn);


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
   virtual const char **getGameParameterMenuKeys();
   virtual boost::shared_ptr<MenuItem> getMenuItem(const char *key);
   virtual bool saveMenuItem(const MenuItem *menuItem, const char *key);
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

   bool areBotsAllowed();
   void setBotsAllowed(bool allowed);
   
   string getScriptLine() const;
   void setScript(const Vector<string> &args);

   string getScriptName() const;
   const Vector<string> *getScriptArgs();

   void onAddedToGame(Game *theGame);

   void onLevelLoaded();      // Server-side function run once level is loaded from file

   virtual void idle(GameObject::IdleCallPath path, U32 deltaT);

   void gameOverManGameOver();
   VersionedGameStats getGameStats();
   void getSortedPlayerScores(S32 teamIndex, Vector<ClientInfo *> &playerScores) const;
   void saveGameStats();                     // Transmit statistics to the master server

   void checkForWinningScore(S32 score);     // Check if player or team has reachede the winning score
   virtual void onGameOver();

   void serverAddClient(ClientInfo *clientInfo);         

   void serverRemoveClient(ClientInfo *clientInfo);    // Remove a client from the game

   virtual bool objectCanDamageObject(GameObject *damager, GameObject *victim);
   virtual void controlObjectForClientKilled(ClientInfo *theClient, GameObject *clientObject, GameObject *killerObject);

   virtual void spawnShip(ClientInfo *clientInfo);
   virtual void spawnRobot(Robot *robot);

   virtual void changeClientTeam(ClientInfo *client, S32 team);     // Change player to team indicated, -1 = cycle teams

#ifndef ZAP_DEDICATED
   virtual void renderInterfaceOverlay(bool scoreboardVisible);
   void renderObjectiveArrow(const GameObject *target, const Color *c, F32 alphaMod = 1.0f) const;
   void renderObjectiveArrow(const Point *p, const Color *c, F32 alphaMod = 1.0f) const;
#endif

   void addTime(U32 time);          // Extend the game by time (in ms)

   void SRV_clientRequestLoadout(ClientInfo *clientInfo, const Vector<U32> &loadout);
   void SRV_updateShipLoadout(GameObject *shipObject); // called from LoadoutZone when a Ship touches the zone
   string validateLoadout(const Vector<U32> &loadout);
   void setClientShipLoadout(ClientInfo *clientInfo, const Vector<U32> &loadout, bool silent = false);


   bool checkTeamRange(S32 team);                     // Team in range? Used for processing arguments.
   bool makeSureTeamCountIsNotZero();                 // Zero teams can cause crashiness
   virtual const Color *getShipColor(Ship *s);        // Get the color of a ship
   virtual const Color *getTeamColor(S32 team) const; // Get the color of a team, based on index
   const Color *getTeamColor(GameObject *theObject);  // Get the color of a team, based on object

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
   virtual void performProxyScopeQuery(GameObject *scopeObject, ClientInfo *clientInfo);

   virtual void onGhostAvailable(GhostConnection *theConnection);
   TNL_DECLARE_RPC(s2cSetLevelInfo, (StringTableEntry levelName, StringTableEntry levelDesc, S32 teamScoreLimit, StringTableEntry levelCreds, 
                                     S32 objectCount, F32 lx, F32 ly, F32 ux, F32 uy, bool levelHasLoadoutZone, bool engineerEnabled));
   TNL_DECLARE_RPC(s2cAddWalls, (Vector<F32> barrier, F32 width, bool solid));
   TNL_DECLARE_RPC(s2cAddTeam, (StringTableEntry teamName, F32 r, F32 g, F32 b, U32 score, bool firstTeam));
   TNL_DECLARE_RPC(s2cAddClient, (StringTableEntry clientName, bool isAuthenticated, bool isMyClient, bool isAdmin, bool isRobot, bool playAlert));
   TNL_DECLARE_RPC(s2cClientJoinedTeam, (StringTableEntry clientName, RangedU32<0, MAX_TEAMS> teamIndex));
   TNL_DECLARE_RPC(s2cClientBecameAdmin, (StringTableEntry clientName));
   TNL_DECLARE_RPC(s2cClientBecameLevelChanger, (StringTableEntry clientName));

   TNL_DECLARE_RPC(s2cSyncMessagesComplete, (U32 sequence));
   TNL_DECLARE_RPC(c2sSyncMessagesComplete, (U32 sequence));

   TNL_DECLARE_RPC(s2cSetGameOver, (bool gameOver));
   TNL_DECLARE_RPC(s2cSetTimeRemaining, (U32 timeLeftInMs));
   TNL_DECLARE_RPC(s2cChangeScoreToWin, (U32 score, StringTableEntry changer));
   

   TNL_DECLARE_RPC(s2cCanSwitchTeams, (bool allowed));

   TNL_DECLARE_RPC(s2cRenameClient, (StringTableEntry oldName,StringTableEntry newName));
   TNL_DECLARE_RPC(s2cRemoveClient, (StringTableEntry clientName));

   // Not all of these actually used?
   void updateScore(Ship *ship, ScoringEvent event, S32 data = 0);              
   void updateScore(ClientInfo *clientInfo, ScoringEvent scoringEvent, S32 data = 0); 
   void updateScore(ClientInfo *player, S32 team, ScoringEvent event, S32 data = 0);
   void updateScore(S32 team, ScoringEvent event, S32 data = 0);

   void updateLeadingTeamAndScore();   // Sets mLeadingTeamScore and mLeadingTeam
   void updateLeadingPlayerAndScore(); // Sets mLeadingTeamScore and mLeadingTeam
   void updateRatings();               // Update everyone's game-normalized ratings at the end of the game


   TNL_DECLARE_RPC(s2cSetTeamScore, (RangedU32<0, MAX_TEAMS> teamIndex, U32 score));
   TNL_DECLARE_RPC(s2cSetPlayerScore, (U16 index, S32 score));

   TNL_DECLARE_RPC(c2sRequestScoreboardUpdates, (bool updates));
   TNL_DECLARE_RPC(s2cScoreboardUpdate, (Vector<RangedU32<0, MaxPing> > pingTimes, Vector<SignedInt<24> > scores, Vector<SignedFloat<8> > ratings));

   void updateClientScoreboard(ClientInfo *clientInfo);

   TNL_DECLARE_RPC(c2sAdvanceWeapon, ());
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

   TNL_DECLARE_RPC(c2sAddTime, (U32 time));                                    // Admin is adding time to the game
   TNL_DECLARE_RPC(c2sChangeTeams, (S32 team));                                // Player wants to change teams

   TNL_DECLARE_RPC(c2sSendChatPM, (StringTableEntry toName, StringPtr message));                        // using /pm command
   TNL_DECLARE_RPC(c2sSendChat, (bool global, StringPtr message));             // In-game chat
   TNL_DECLARE_RPC(c2sSendChatSTE, (bool global, StringTableEntry ste));       // Quick-chat
   TNL_DECLARE_RPC(c2sSendCommand, (StringTableEntry cmd, Vector<StringPtr> args));

   TNL_DECLARE_RPC(s2cDisplayChatPM, (StringTableEntry clientName, StringTableEntry toName, StringPtr message));
   TNL_DECLARE_RPC(s2cDisplayChatMessage, (bool global, StringTableEntry clientName, StringPtr message));
   TNL_DECLARE_RPC(s2cDisplayChatMessageSTE, (bool global, StringTableEntry clientName, StringTableEntry message));


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

   TNL_DECLARE_RPC(c2sTriggerTeamChange, (StringTableEntry playerName, S32 teamIndex));
   TNL_DECLARE_RPC(c2sKickPlayer, (StringTableEntry playerName));

   TNL_DECLARE_CLASS(GameType);

   enum
   {
      mZoneGlowTime = 800,    // Time for visual effect, used by Nexus & GoalZone
   };

   Timer mZoneGlowTimer;
   S32 mGlowingZoneTeam;      // Which team's zones are glowing, -1 for all

   virtual void majorScoringEventOcurred(S32 team);    // Gets called when touchdown is scored...  currently only used by zone control & retrieve

   void processServerCommand(ClientInfo *clientInfo, const char *cmd, Vector<StringPtr> args);
   void addBot(Vector<StringTableEntry> args);

   map <pair<U16,U16>, Vector<Point> > cachedBotFlightPlans;  // cache of zone-to-zone flight plans, shared for all bots
};

#define GAMETYPE_RPC_S2C(className, methodName, args, argNames) \
   TNL_IMPLEMENT_NETOBJECT_RPC(className, methodName, args, argNames, NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)

#define GAMETYPE_RPC_C2S(className, methodName, args, argNames) \
   TNL_IMPLEMENT_NETOBJECT_RPC(className, methodName, args, argNames, NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhostParent, 0)

};

#endif
