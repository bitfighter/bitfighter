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

//#include "tnlTypes.h"

// Constants that need to be the same on both the game and the master server
// This file is shared between both code bases.

#define MAX_CHAT_MSG_LENGTH 2048
#define MAX_PLAYER_NAME_LENGTH 32      // Max length of a player name
#define MAX_PLAYER_PASSWORD_LENGTH 32

static const unsigned int MOTD_LEN = 256;
static const unsigned int MAX_PLAYERS = 128;    // Absolute maximum players ever allowed in a game

enum AuthenticationStatus {
   AuthenticationStatusAuthenticatedName,
   AuthenticationStatusUnauthenticatedName,
   AuthenticationStatusTryAgainLater,
   AuthenticationStatusFailed,
   AuthenticationStatusCount
};

#endif

