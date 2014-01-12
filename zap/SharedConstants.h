//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#ifndef _SHARED_CONSTANTS_H_
#define _SHARED_CONSTANTS_H_

#include "tnlTypes.h"      // for BIT macro

using namespace TNL;

// Constants that need to be the same on both the game and the master server
// This file is shared between both code bases.

// Use caution when changing -- master may need to work with multiple versions of clients!!!
// In some cases, it may be better to create a new constant, rather than changing an old one.
// The old one may be deleted after a few versions go by.

#define MAX_CHAT_MSG_LENGTH 2048
#define MAX_PLAYER_NAME_LENGTH 32      // Max length of a player name
#define MAX_PLAYER_PASSWORD_LENGTH 32


enum PersonalRating     // These need to be able to fit into S16 for totalRating
{
   RatingGood = 1,
   RatingNeutral = 0,
   RatingBad = -1,
   Unrated = S16_MIN,                     // -32768       
   UnknownRating = S16_MIN + 1,           // -32767
   NotReallyInTheDatabase = S16_MIN + 2,  // -32766
   MinumumLegitimateRating = S16_MIN + 3  // -32765
};


static const U32 MOTD_LEN = 256;
static const U32 MAX_PLAYERS = 127;    // Absolute maximum players ever allowed in a game  --> should not be more than U8_MAX

enum AuthenticationStatus {
   AuthenticationStatusAuthenticatedName,
   AuthenticationStatusUnauthenticatedName,
   AuthenticationStatusTryAgainLater,
   AuthenticationStatusFailed,
   AuthenticationStatusCount
};


// !!! Important !!!
// When adding a new badge here, we need to add it to Game Jolt, and also update the achievements table in the database
enum MeritBadges {
   NO_BADGES = 0,             // <--- REALLY?????
   DEVELOPER_BADGE = 0,
   FIRST_VICTORY = 1,
   BADGE_TWENTY_FIVE_FLAGS = 2,
   BADGE_BBB_GOLD = 3,
   BADGE_BBB_SILVER = 4,
   BADGE_BBB_BRONZE = 5,
   BADGE_BBB_PARTICIPATION = 6,
   BADGE_LEVEL_DESIGN_WINNER = 7,
   BADGE_ZONE_CONTROLLER = 8,
   BADGE_RAGING_RABID_RABBIT = 9,
   BADGE_HAT_TRICK = 10,
   BADGE_LAST_SECOND_WIN = 11,
   //...
   BADGE_COUNT = 32              // Changing this value will require updating master server and protocol version
};

enum ServerInfoFlags {
   TestModeFlag  = BIT(0),       // If server is testing a level from the editor
   DebugModeFlag = BIT(1),       // If player is using a debug build (i.e. is probably a dev testing something)
};

enum ClientInfoFlags {
   ClientDebugModeFlag = BIT(0)  // If player is using a debug build (i.e. is probably a dev testing something)
};

enum MasterConnectionType {
   MasterConnectionTypeClient = 0,     // Connection from client
   MasterConnectionTypeServer = 1,     // Connection from game server
   MasterConnectionTypeAnonymous = 2,  // Anonymous connection
   //...                               // Room for one more!
   MasterConnectionTypeCount = 4,      // Changing this will require updating the master protocol
   MasterConnectionTypeNone,
};

#endif

