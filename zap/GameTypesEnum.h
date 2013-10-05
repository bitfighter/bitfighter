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

#ifndef _GAMETYPESENUM_H_
#define _GAMETYPESENUM_H_

namespace Zap
{     
/**
 * @luaenum GameType(1,2)
 * The GameType enum represets the different types of game.
 */
//                 Enum              ClassName             LuaEnum     GameType Name     TeamGame  Game Instructions                
#define GAME_TYPE_TABLE \
   GAME_TYPE_ITEM( BitmatchGame,    "GameType",            "Bitmatch", "Bitmatch",         false, "Simple combat game -- kill as many players as you can.  Also has a team variant where you can work together to rack up kills." )                                                \
   GAME_TYPE_ITEM( NexusGame,       "NexusGameType",       "Nexus",    "Nexus",            false, "Kill other players and grab their flags.  Bring them to the Nexus when it is open to score.  Points grow geometricaly, so one big score is worth more than two smaller ones." ) \
   GAME_TYPE_ITEM( RabbitGame,      "RabbitGameType",      "Rabbit",   "Rabbit",           false, "Grab the flag and hold on to it as long as you can.  Get the flag by killing the player who has it can getting it before other players do." )                                   \
   GAME_TYPE_ITEM( CTFGame,         "CTFGameType",         "CTF",      "Capture the Flag", true,  "Capture the enemy flag and touch it to your own flag to score.  You can also recapture a stolen flag by flying over it." )                                                      \
   GAME_TYPE_ITEM( CoreGame,        "CoreGameType",        "Core",     "Core",             true,  "Win by killing all the enemy cores before they kill yours." )                                                                                                                   \
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


   // Define an enum of scoring events from the values in SCORING_EVENT_TABLE
   enum ScoringEvent {
      #define SCORING_EVENT_ITEM(value, b) value,
         SCORING_EVENT_TABLE
      #undef SCORING_EVENT_ITEM
         ScoringEventsCount
   };

};


#endif
