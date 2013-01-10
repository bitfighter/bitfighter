//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo relased for Torque Network Library by GarageGames.com
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


#ifndef _SHARED_CONSTANTS_H_
#define _SHARED_CONSTANTS_H_

#include "tnlTypes.h"      // for BIT macro


// Constants that need to be the same on both the game and the master server
// This file is shared between both code bases.

// Use caution when changing -- master may need to work with multiple versions of clients!!!
// In some cases, it may be better to create a new constant, rather than changing an old one.
// The old one may be deleted after a few versions go by.

#define MAX_CHAT_MSG_LENGTH 2048
#define MAX_PLAYER_NAME_LENGTH 32      // Max length of a player name
#define MAX_PLAYER_PASSWORD_LENGTH 32

static const TNL::U32 MOTD_LEN = 256;
static const TNL::U32 MAX_PLAYERS = 127;    // Absolute maximum players ever allowed in a game  --> should not be more than U8_MAX

enum AuthenticationStatus {
   AuthenticationStatusAuthenticatedName,
   AuthenticationStatusUnauthenticatedName,
   AuthenticationStatusTryAgainLater,
   AuthenticationStatusFailed,
   AuthenticationStatusCount
};


enum MeritBadges {
   NO_BADGES = 0,
   DEVELOPER_BADGE = 0,
   FIRST_VICTORY = 1,
   BADGE_TWENTY_FIVE_FLAGS = 2,
   BADGE_BBB_GOLD = 3,
   BADGE_BBB_SILVER = 4,
   BADGE_BBB_BRONZE = 5,
   BADGE_BBB_PARTICIPATION = 6,
   BADGE_LEVEL_DESIGN_WINNER = 7,
   BADGE_ZONE_CONTROLLER = 8,
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

