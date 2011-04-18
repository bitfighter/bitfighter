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

//#include "config.h"
#include "gameStats.h"
#include "../master/database.h"
#include "gameWeapons.h"         // For WeaponType enum

#include "tnlMethodDispatch.h"
#include "tnlAssert.h"

#include "Color.h"


// This is shared in both client and master.
// this read/write is usually the hardest part about struct, but this allows custom version handling.

using namespace TNL;
using namespace Zap;

namespace Zap

{

//extern ConfigDirectories gConfigDirs; // Fail to compile master.  Link errors, missing gConfigDirs.
string gSqlite = "stats";

// Sorts player stats by score, high to low
S32 QSORT_CALLBACK playerScoreSort(PlayerStats *a, PlayerStats *b)
{
   return b->points - a->points;
}


// Sorts team stats by score, high to low
S32 QSORT_CALLBACK teamScoreSort(TeamStats *a, TeamStats *b)
{
   return b->score - a->score;  
}

   // This relies on scores being sent sorted in order of descending score
   string getResult(S32 scores, S32 score1, S32 score2, S32 currScore, bool isFirst)
   {
      if(scores == 1)      // Only one player/team, winner/loser makes no sense
         return "X";
      else if(score1 == score2 && currScore == score1)     // Tie -- everyone with high score gets tie
         return "T";
      else if(isFirst)     // No tie -- first one gets the win...
         return "W";
      else                 // ...and everyone else gets the loss
         return "L";
   }


   // Fills in some of the blanks in the gameStats struct; not everything is sent
   void processStatsResults(GameStats *gameStats)
   {
      for(S32 i = 0; i < gameStats->teamStats.size(); i++)
      {
         Vector<PlayerStats> *playerStats = &gameStats->teamStats[i].playerStats;

         // Now compute winning player(s) based on score or points; but must sort first
         if(! gameStats->isTeamGame)
         {
            playerStats->sort(playerScoreSort);
            for(S32 j = 0; j < playerStats->size(); j++)
               (*playerStats)[j].gameResult = 
                  getResult(playerStats->size(), (*playerStats)[0].points, playerStats->size() == 1 ? 0 : (*playerStats)[1].points, (*playerStats)[j].points, j == 0);
         }
      }

      if(gameStats->isTeamGame)
      {
         Vector<TeamStats> *teams = &gameStats->teamStats;
         teams->sort(teamScoreSort);
         for(S32 i = 0; i < teams->size(); i++)
         {
            (*teams)[i].gameResult = 
               getResult(teams->size(), (*teams)[0].score, teams->size() == 1 ? 0 : (*teams)[1].score, (*teams)[i].score, i == 0);
            for(S32 j = 0; j < (*teams)[i].playerStats.size(); j++) // make all players in a team same gameResults
               (*teams)[i].playerStats[j].gameResult = (*teams)[i].gameResult;
         }
      }
   }


   void logGameStats(VersionedGameStats *stats) 
   {
      processStatsResults(&stats->gameStats);

      string databasePath = gSqlite + ".db";

      DatabaseWriter databaseWriter(databasePath.c_str());

      databaseWriter.insertStats(stats->gameStats);
   }


};


#ifdef TNL_ENABLE_ASSERTS
bool VersionedGameStats_testing = false;
U32 VersionedGameStats_ReadSize;
U32 VersionedGameStats_WriteSize;
#define VersionedGameStats_write_start(s) {VersionedGameStats_WriteSize = s.getBitPosition();}
#define VersionedGameStats_write_end(s) {VersionedGameStats_WriteSize = s.getBitPosition() - VersionedGameStats_WriteSize;}
#define VersionedGameStats_read_start(s) {VersionedGameStats_ReadSize = s.getBitPosition();}
#define VersionedGameStats_read_end(s) {VersionedGameStats_ReadSize = s.getBitPosition() - VersionedGameStats_ReadSize; \
   TNLAssert((!VersionedGameStats_testing) || VersionedGameStats_WriteSize == VersionedGameStats_ReadSize, "VersionedGameStats Read and write size is not equal. They must be equal size to prevent network errors.")}
#else
#define VersionedGameStats_write_start(s)
#define VersionedGameStats_write_end(s)
#define VersionedGameStats_read_start(s)
#define VersionedGameStats_read_end(s)
#endif



namespace Types
{
   U8 readU8(TNL::BitStream &s)   { U8 val; read(s, &val); return val; }
   S8 readS8(TNL::BitStream &s)   { S8 val; read(s, &val); return val; }
   U16 readU16(TNL::BitStream &s) { U16 val; read(s, &val); return val; }
   S16 readS16(TNL::BitStream &s) { S16 val; read(s, &val); return val; }
   U32 readU32(TNL::BitStream &s) { U32 val; read(s, &val); return val; }
   S32 readS32(TNL::BitStream &s) { S32 val; read(s, &val); return val; }
   // for bool, use   s.readFlag();
   string readString(TNL::BitStream &s) { char val[256]; s.readString(val); return val; }
   void writeString(TNL::BitStream &s, const string &val) { s.writeString(val.c_str()); }

   U8 VersionedGameStats_readingVersion;


   void read(TNL::BitStream &s, Zap::WeaponStats *val)
	{
      val->weaponType = WeaponType(readU8(s));
      val->shots = readU16(s);
      val->hits = readU16(s);
	}


   void write(TNL::BitStream &s, Zap::WeaponStats &val)
	{
      write(s, U8(val.weaponType));
      write(s, U16(val.shots));
      write(s, U16(val.hits));
	}


   void read(TNL::BitStream &s, Zap::PlayerStats *val)
	{
      val->name = readString(s);
      val->points = readS32(s);
      val->kills = readU16(s);
      val->deaths = readU16(s);
      val->suicides = readU16(s);
      val->switchedTeamCount = readU8(s);
      //val->switchedTeams = (val->switchedTeamCount != 0);
      val->isRobot = s.readFlag();
      val->isAdmin = s.readFlag();
      val->isLevelChanger = s.readFlag();
      val->isAuthenticated = false; //s.readFlag();  // we may set this by comparing Nonce id.
      val->nonce.read(&s);
		read(s, &val->weaponStats);
	}


   void write(TNL::BitStream &s, Zap::PlayerStats &val)
   {
      writeString(s, val.name);
      write(s, S32(val.points));
      write(s, U16(val.kills));
      write(s, U16(val.deaths));
      write(s, U16(val.suicides));
      write(s, U8(val.switchedTeamCount));
      s.writeFlag(val.isRobot);
      s.writeFlag(val.isAdmin);
      s.writeFlag(val.isLevelChanger);
      val.nonce.write(&s);

      write(s, val.weaponStats);
	}


   void read(TNL::BitStream &s, Zap::TeamStats *val)
   {
      val->name = readString(s);
      val->score = readS32(s);

      val->intColor = s.readInt(24); // 24 bit color
      val->hexColor = Color(val->intColor).toHexString();

		read(s, &val->playerStats);
	}


   void write(TNL::BitStream &s, Zap::TeamStats &val)
   {
      writeString(s, val.name);
      write(s, S32(val.score));
      s.writeInt(val.intColor, 24);    // 24 bit color
      write(s, val.playerStats);
	}


   void read(TNL::BitStream &s, Zap::GameStats *val)
	{
      val->isOfficial = s.readFlag();
      val->playerCount = readU16(s);
      val->duration = readU16(s);             // game length in seconds
      val->isTeamGame = s.readFlag();
      val->gameType = readString(s);
      val->levelName = readString(s);
      read(s, &val->teamStats);
      val->teamCount = val->teamStats.size(); // is this needed?  ==> probably not (CE)
	}


   void write(TNL::BitStream &s, Zap::GameStats &val)
	{
      s.writeFlag(val.isOfficial);
      write(s, U16(val.playerCount));
      write(s, U16(val.duration));     // game length in seconds
      s.writeFlag(val.isTeamGame);
      writeString(s, val.gameType);
      writeString(s, val.levelName);
      write(s, val.teamStats);
	}

	
   /// Reads objects from a BitStream
   void read(TNL::BitStream &s, VersionedGameStats *val)
   {
      VersionedGameStats_read_start(s);
      val->version = readU8(s);  // Read version number
      read(s, &val->gameStats);  // This is not Vector, goes directly to read(s, Zap::GameStats *val)

      // Stop here if TNL_ENABLE_ASSERTS is on, and write/read size is not matched
      VersionedGameStats_read_end(s);
      if(s.isValid())
         val->valid = true;
   }


   /// Writes objects into a BitStream. Server write and send to master.
   void write(TNL::BitStream &s, VersionedGameStats &val)
   {
      VersionedGameStats_write_start(s);
      write(s, U8(val.CURRENT_VERSION));       // Send current version
      write(s, val.gameStats);

      // To clarify what this does...
      // write(number) writes 32 bit int when number is S32
      // write(number) writes 8 bit int when number is U8
      // write(U16(number)) always writes 16 bit when number is U8, S32, or any other type


      VersionedGameStats_write_end(s);
   }
}
