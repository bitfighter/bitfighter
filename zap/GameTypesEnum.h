//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAMETYPESENUM_H_
#define _GAMETYPESENUM_H_

namespace Zap
{     
/**
 * @luaenum GameType(1,3)
 * The GameType enum represets the different types of game.
 */                                           //      LuaEnum not used?     
//                 Enum              ClassName             LuaEnum     GameType Name     TeamGame  Game Instructions                
#define GAME_TYPE_TABLE \
   GAME_TYPE_ITEM( BitmatchGame,    "GameType",            "Bitmatch", "Bitmatch",         false, "Simple combat game -- zap as many players as you can.  Also has a team variant where you can work together to rack up points." )                                                \
   GAME_TYPE_ITEM( NexusGame,       "NexusGameType",       "Nexus",    "Nexus",            false, "Blast other players and grab their flags.  Bring them to the Nexus when it is open to score.  Points grow geometricaly, so one big score is worth more than two smaller ones." ) \
   GAME_TYPE_ITEM( RabbitGame,      "RabbitGameType",      "Rabbit",   "Rabbit",           false, "Grab the flag and hold on to it as long as you can.  Get the flag by zapping the player who has it and getting it before other players do." )                                   \
   GAME_TYPE_ITEM( CTFGame,         "CTFGameType",         "CTF",      "Capture the Flag", true,  "Capture the enemy flag and touch it to your own flag to score.  You can also recapture a stolen flag by flying over it." )                                                      \
   GAME_TYPE_ITEM( CoreGame,        "CoreGameType",        "Core",     "Core",             true,  "Win by destroying all the enemy cores before they destroy yours." )                                                                                                                   \
   GAME_TYPE_ITEM( HTFGame,         "HTFGameType",         "HTF",      "Hold the Flag",    true,  "Capture enemy flags and return them to your goals; the longer you hold them there, the more points you'll rack up." )                                                           \
   GAME_TYPE_ITEM( RetrieveGame,    "RetrieveGameType",    "Retrieve", "Retrieve",         true,  "Capture enemy flags and return them to your capture zone.  When all your zones are full, your points are locked in." )                                                          \
   GAME_TYPE_ITEM( SoccerGame,      "SoccerGameType",      "Soccer",   "Soccer",           true,  "Push or shoot the ball into an enemy goal zone." )                                                                                                                              \
   GAME_TYPE_ITEM( ZoneControlGame, "ZoneControlGameType", "ZC",       "Zone Control",     true,  "Control zones by flying through them with the flag.  Capture all the zones to lock in your points." )                                                                           \
// NOTE: Above list is sorted by individual games first, followed by team games, sorted alphabetically along the way

   // Define an enum from the first column of GAME_TYPE_TABLE
   enum GameTypeId {
#  define GAME_TYPE_ITEM(enumValue, b, c, d, e, f) enumValue,
       GAME_TYPE_TABLE
#  undef GAME_TYPE_ITEM
       NoGameType,
       GameTypesCount
   };


/**
 * @luaenum ScoringEvent(1,2)
 * The ScoringEvent enum represents different actions that change the score.
 */
#define SCORING_EVENT_TABLE \
   SCORING_EVENT_ITEM(KillEnemy,               "KillEnemy",               "Applies to all game types")                            \
   SCORING_EVENT_ITEM(KillSelf,                "KillSelf",                "Applies to all game types")                            \
   SCORING_EVENT_ITEM(KillTeammate,            "KillTeammate",            "Applies to all game types")                            \
   SCORING_EVENT_ITEM(KillEnemyTurret,         "KillEnemyTurret",         "Applies to all game types")                            \
   SCORING_EVENT_ITEM(KillOwnTurret,           "KillOwnTurret",           "Applies to all game types")                            \
                                                                                                                                  \
   SCORING_EVENT_ITEM(KilledByAsteroid,        "KilledByAsteroid",        "Applies to all game types")                            \
   SCORING_EVENT_ITEM(KilledByTurret,          "KilledByTurret",          "Applies to all game types")                            \
                                                                                                                                  \
   SCORING_EVENT_ITEM(CaptureFlag,             "CaptureFlag",             "Applies to CTF game")                                  \
   SCORING_EVENT_ITEM(CaptureZone,             "CaptureZone",             "Gain zone -> Applies to Zone Control game")            \
   SCORING_EVENT_ITEM(UncaptureZone,           "UncaptureZone",           "Lose zone -> Applies to Zone Control game")            \
   SCORING_EVENT_ITEM(HoldFlagInZone,          "HoldFlagInZone",          "Applies to HTF game")                                  \
   SCORING_EVENT_ITEM(RemoveFlagFromEnemyZone, "RemoveFlagFromEnemyZone", "Applies to HTF game")                                  \
   SCORING_EVENT_ITEM(RabbitHoldsFlag,         "RabbitHoldsFlag",         "Applies to Rabbit game, called every second")          \
   SCORING_EVENT_ITEM(RabbitKilled,            "RabbitKilled",            "Applies to Rabbit game")                               \
   SCORING_EVENT_ITEM(RabbitKills,             "RabbitKills",             "Applies to Rabbit game")                               \
   SCORING_EVENT_ITEM(ReturnFlagsToNexus,      "ReturnFlagsToNexus",      "Applies to Nexus game")                                \
   SCORING_EVENT_ITEM(ReturnFlagToZone,        "ReturnFlagToZone",        "Flag returned to zone -> Applies to Retrieve game")    \
   SCORING_EVENT_ITEM(LostFlag,                "LostFlag",                "Enemy took flag -> Applies to Retrieve game")          \
   SCORING_EVENT_ITEM(ReturnTeamFlag,          "ReturnTeamFlag",          "Holds enemy flag, touches own flag -> Applies to CTF") \
   SCORING_EVENT_ITEM(ScoreGoalEnemyTeam,      "ScoreGoalEnemyTeam",      "Applies to Soccer game")                               \
   SCORING_EVENT_ITEM(ScoreGoalHostileTeam,    "ScoreGoalHostileTeam",    "Applies to Soccer game")                               \
   SCORING_EVENT_ITEM(ScoreGoalOwnTeam,        "ScoreGoalOwnTeam",        "Score on self -> Applies to Soccer game")              \
   SCORING_EVENT_ITEM(EnemyCoreDestroyed,      "EnemyCoreDestroyed",      "Enemy core is destroyed -> Applies to Core game")      \
   SCORING_EVENT_ITEM(OwnCoreDestroyed,        "OwnCoreDestroyed",        "Own core is destroyed -> Applies to Core game")        \


   // Define an enum of scoring events from the values in SCORING_EVENT_TABLE
   enum ScoringEvent {
      #define SCORING_EVENT_ITEM(value, b, c) value,
         SCORING_EVENT_TABLE
      #undef SCORING_EVENT_ITEM
         ScoringEventsCount
   };

};


#endif
