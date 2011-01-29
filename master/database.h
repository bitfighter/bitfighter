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

#ifndef DATABASE_H
#define DATABASE_H

#include "../zap/gameWeapons.h"     // For WeaponType enum
#include "tnlTypes.h"
#include "tnlVector.h"
#include "tnlNonce.h"

#include <string>

// Forward declaration
namespace mysqlpp
{
	class TCPConnection;
};

using namespace TNL;
using namespace std;
using namespace Zap;

struct WeaponStats 
{
   WeaponType weaponType;
   U16 shots;
   U16 hits;
};


struct PlayerStats
{
   string name;
   bool isAuthenticated;
   Nonce nonce;
   bool isRobot;
   string gameResult;
   S32 points;
   S32 kills;
   S32 deaths;
   S32 suicides;
   bool switchedTeams;
   S32 switchedTeamCount;
   Vector<WeaponStats> weaponStats;

   bool isAdmin;
   bool isLevelChanger;
};


struct TeamStats 
{
   U32 color_bin; // The game server needs to send in number, not string
   string color;
   string name;
   S32 score;
   string gameResult;     // 'W', 'L', 'T'
   Vector<PlayerStats> playerStats;    // Info about all players on this team
};

struct ServerInformation
{
   U64 id;
   string name;
   string ip;
	ServerInformation(U64 id, const string name, const string ip) { this->id = id; this->name = name; this->ip = ip;}
};


struct GameStats
{
   string serverName;
   string serverIP;

   string gameType;
   string levelName;
   bool isOfficial;
   S32 playerCount;
   S32 duration;     // game length in seconds
   bool isTeamGame;
   S32 teamCount;
   bool isTied;
   Vector<TeamStats> teamStats;     // for team games
};


class DatabaseWriter 
{
private:
   bool mIsValid;
   const char *mServer;
   const char *mDb;
   const char *mUser;
   const char *mPassword;
   Vector<ServerInformation> cachedServers;
   U64 lastGameID;

#ifdef VERIFY_PHPBB3
   void initialize(const char *server, const char *db, const char *user, const char *password);
#else
   void initialize(const char *server, const char *db, const char *user, const char *password) {};
#endif

public:
#ifdef VERIFY_PHPBB3
   DatabaseWriter();
   DatabaseWriter(const char *server, const char *db, const char *user, const char *password);     // Constructor
   DatabaseWriter(const char *db, const char *user, const char *password);                         // Constructor
   void insertStats(const GameStats &gameStats);
#else
   DatabaseWriter() {};
   DatabaseWriter(const char *server, const char *db, const char *user, const char *password) {};     // Constructor
   DatabaseWriter(const char *db, const char *user, const char *password) {};                        // Constructor
   void insertStats(const GameStats &gameStats) {};
#endif
};


#endif


