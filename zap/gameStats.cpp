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

#include "gameStats.h"
//#include "config.h"
#include "../master/database.h"
#include "gameWeapons.h"         // For WeaponType enum

#include "tnlMethodDispatch.h"
#include "tnlAssert.h"

#include "Color.h"


// This is shared in both client and master.
// this read/write is usually the hardest part about struct, but this allows custom version handling.

using namespace TNL;
using namespace Zap;



U32 calculateChecksum(BitStream &s, U32 length, U32 bitStart = 0)
{
   U64 sum = length;
   U32 prevPos = s.getBitPosition();
   s.setBitPosition(bitStart);
   while(s.getBitPosition() < bitStart + length - 64)
   {
      U64 n;
      s.read(&n);
      sum += n;
   }
   sum += s.readInt64(((length - 1) & 63) + 1);
   s.setBitPosition(prevPos);
   return U32(sum >> 32) + U32(sum);
}


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


   void read(TNL::BitStream &s, Zap::WeaponStats *val, U8 version)
   {
      val->weaponType = WeaponType(readU8(s));
      val->shots = readU16(s);
      val->hits = readU16(s);
      if(version >= 1)
         val->hitBy = readU16(s);
      else
         val->hitBy = 0;
   }


   void write(TNL::BitStream &s, Zap::WeaponStats &val, U8 version)
   {
      write(s, U8(val.weaponType));
      write(s, U16(val.shots));
      write(s, U16(val.hits));
      if(version >= 1)
         write(s, U16(val.hitBy));
   }

   void read(TNL::BitStream &s, Zap::ModuleStats *val, U8 version)
   {
      val->weaponType = ShipModule(readU8(s));
      val->shots = readU16(s);
   }


   void write(TNL::BitStream &s, Zap::ModuleStats &val, U8 version)
   {
      write(s, U8(val.ShipModule));
      write(s, U16(val.));
   }


   void read(TNL::BitStream &s, Zap::PlayerStats *val, U8 version)
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
      read(s, &val->weaponStats, version);
      if(version >= 1)
      {
         val->isAuthenticated = s.readFlag();
         val->isHosting = s.readFlag();
         val->fratricides = readU16(s);
         read(s, &val->moduleStats, version);
      }
      else
      {
         val->isAuthenticated = false;
         val->isHosting = false;
         val->fratricides = 0;
      }
   }


   void write(TNL::BitStream &s, Zap::PlayerStats &val, U8 version)
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
      write(s, val.weaponStats, version);
      if(version >= 1)
      {
         s.writeFlag(val.isAuthenticated);
         s.writeFlag(val.isHosting);
         write(s, U16(val.fratricides));
         write(s, val.moduleStats, version);
      }
   }


   void read(TNL::BitStream &s, Zap::TeamStats *val, U8 version)
   {
      val->name = readString(s);
      val->score = readS32(s);

      val->intColor = s.readInt(24); // 24 bit color
      val->hexColor = Color(val->intColor).toHexString();

      read(s, &val->playerStats, version);
   }


   void write(TNL::BitStream &s, Zap::TeamStats &val, U8 version)
   {
      writeString(s, val.name);
      write(s, S32(val.score));
      s.writeInt(val.intColor, 24);    // 24 bit color
      write(s, val.playerStats, version);
   }


   void read(TNL::BitStream &s, Zap::GameStats *val, U8 version)
   {
      val->isOfficial = s.readFlag();
      val->playerCount = readU16(s);
      val->duration = readU16(s);             // game length in seconds
      val->isTeamGame = s.readFlag();
      val->gameType = readString(s);
      val->levelName = readString(s);
      read(s, &val->teamStats, version);
   }


   void write(TNL::BitStream &s, Zap::GameStats &val, U8 version)
   {
      s.writeFlag(val.isOfficial);
      write(s, U16(val.playerCount));
      write(s, U16(val.duration));     // game length in seconds
      s.writeFlag(val.isTeamGame);
      writeString(s, val.gameType);
      writeString(s, val.levelName);
      write(s, val.teamStats, version);
   }

   
   /// Reads objects from a BitStream
   void read(TNL::BitStream &s, VersionedGameStats *val)
   {
      U32 bitStart = s.getBitPosition();
      val->version = readU8(s);  // Read version number
      val->valid = false;

      if(val->version > VersionedGameStats::CURRENT_VERSION)  // this might happen with outdated master
         return;

      read(s, &val->gameStats, val->version);

      if(!s.isValid() || val->gameStats.teamStats.size() == 0)  // team size should never be zero
         return;

      if(val->version >= 1)  // protect against incomplete or damaged data
      {
         U32 actualChecksum = calculateChecksum(s, bitStart, s.getBitPosition() - bitStart);
         val->valid = (actualChecksum == readU32(s));
      }
      else
         val->valid = true;
   }


   /// Writes objects into a BitStream. Server write and send to master.
   void write(TNL::BitStream &s, VersionedGameStats &val)
   {
      U32 bitStart = s.getBitPosition();
      write(s, U8(val.CURRENT_VERSION));       // Send current version
      write(s, val.gameStats, val.CURRENT_VERSION);

      // To clarify what this does...
      // write(number) writes 32 bit int when number is S32
      // write(number) writes 8 bit int when number is U8
      // write(U16(number)) always writes 16 bit when number is U8, S32, or any other type

      if(val.CURRENT_VERSION >= 1)
      {
         U32 checksum = calculateChecksum(s, bitStart, s.getBitPosition() - bitStart);
         s.write(U32(checksum));
      }

   }
}
