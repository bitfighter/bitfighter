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

#ifndef _GAMESTATS_H_
#define _GAMESTATS_H_

#include "gameWeapons.h"
#include "shipItems.h"

#include "tnlTypes.h"
#include "tnlVector.h"
#include "tnlBitStream.h"
#include "tnlNetStringTable.h"
#include "tnlNonce.h"

using namespace std;
using namespace TNL;

namespace Zap {

struct WeaponStats 
{
   WeaponType weaponType;
   U32 shots;
   U32 hits;
   U32 hitBy;
};

struct ModuleStats 
{
   ShipModule shipModule;
   U32 seconds;
};

struct LoadoutStats
{
   U32 loadoutHash;
};

// Embedded within TeamStats below
struct PlayerStats
{
   string name;
   bool isAuthenticated;
   Nonce nonce;         // used for authentication, will only send if isAuthenticated = true
   bool isRobot;
   char gameResult;     // 'W', 'L', 'T'  // not sent, calculated in master
   S32 points;
   U32 kills;
   U32 turretKills;     // Turrets killed xx
   U32 ffKills;         // FFs killed xx
   U32 astKills;        // Asteroids killed xx
   U32 turretsEngr;     // Turrets constructed with engineer  xx
   U32 ffEngr;          // Forcefields engineered  xx
   U32 telEngr;         // Teleporters engineered  xx
   U32 deaths;
   U32 suicides;
   U32 switchedTeamCount;
   Vector<WeaponStats> weaponStats;
   Vector<ModuleStats> moduleStats;
   Vector<LoadoutStats> loadoutStats;

   U32 flagPickup;
   U32 flagDrop;
   U32 flagReturn;
   U32 flagScore;
   U32 crashedIntoAsteroid;
   U32 changedLoadout;
   U32 teleport;
   U32 distTraveled;   //xx
	U32 playTime;

   bool isAdmin;
   bool isLevelChanger;
   bool isHosting;

   U32 fratricides;  // count of killing your team
};


// Embedded within GameStats below
struct TeamStats 
{
   U32 intColor;    // To send as number, not string
   string hexColor; // not sent, calculated on receiving based on intColor
   string name;
   S32 score;
   char gameResult;     // 'W', 'L', 'T'  // not sent, calculated in master
   Vector<PlayerStats> playerStats;    // Info about all players on this team
};


// Embedded with VersionedGameStats below
struct GameStats
{
   string serverName;       // not sent, master fills this in
   string serverIP;         // not sent, master fills this in
   S32 cs_protocol_version; // not sent, master fills this in
   S32 build_version;

   string gameType;
   string levelName;
   bool isOfficial;
   bool isTesting;
   U32 playerCount;  // not sent, this is calculated while receiving
   U32 duration;     // game length in seconds
   bool isTeamGame;
   Vector<TeamStats> teamStats;     // for team games
};


// This contains the whole kilbasa
struct VersionedGameStats
{
   static const U8 CURRENT_VERSION = 2;

   U8 version;
   bool valid;
   GameStats gameStats;
};


extern char getResult(S32 scores, S32 score1, S32 score2, S32 currScore, bool isFirst);
//extern S32 QSORT_CALLBACK playerScoreSort(PlayerStats *a, PlayerStats *b);
extern S32 QSORT_CALLBACK teamScoreSort(TeamStats *a, TeamStats *b);
extern void processStatsResults(GameStats *gameStats);
extern void logGameStats(VersionedGameStats *stats);

};    // end namespace Zap



namespace Types
{
   extern void read (TNL::BitStream &s, Zap::LoadoutStats *val, U8 version);
   extern void write(TNL::BitStream &s, Zap::LoadoutStats &val, U8 version);
   extern void read (TNL::BitStream &s, Zap::WeaponStats  *val, U8 version);
   extern void write(TNL::BitStream &s, Zap::WeaponStats  &val, U8 version);
   extern void read (TNL::BitStream &s, Zap::ModuleStats  *val, U8 version);
   extern void write(TNL::BitStream &s, Zap::ModuleStats  &val, U8 version);
   extern void read (TNL::BitStream &s, Zap::PlayerStats  *val, U8 version);
   extern void write(TNL::BitStream &s, Zap::PlayerStats  &val, U8 version);
   extern void read (TNL::BitStream &s, Zap::TeamStats    *val, U8 version);
   extern void write(TNL::BitStream &s, Zap::TeamStats    &val, U8 version);
   extern void read (TNL::BitStream &s, Zap::GameStats    *val, U8 version);
   extern void write(TNL::BitStream &s, Zap::GameStats    &val, U8 version);
   extern void read (TNL::BitStream &s, Zap::VersionedGameStats *val);
   extern void write(TNL::BitStream &s, Zap::VersionedGameStats &val);
};




#endif

