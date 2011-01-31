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
#include "gameWeapons.h"         // For WeaponType enum

#include "tnlMethodDispatch.h"
#include "tnlAssert.h"


// This is shared in both client and master.
// this read/write is usually the hardest part about struct, but this allows custom version handling.

using namespace TNL;
using namespace Zap;

namespace Zap
{
void logGameStats(VersionedGameStats *stats, S32 format)  // TODO: log game stats
   {
   GameStats *g = &stats->gameStats;
      for(S32 i = 0; i < g->teamStats.size(); i++)
      {
         TeamStats *gt = &g->teamStats[i];
         for(S32 j = 0; j < gt->playerStats.size(); j++)
         {
            PlayerStats *gp = &gt->playerStats[j];
            for(S32 k = 0; k < gp->weaponStats.size(); k++)
            {
               WeaponStats *gw = &gp->weaponStats[k];
            }
         }
      }
   }
}


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
      val->switchedTeams = (val->switchedTeamCount != 0);
      val->isRobot = s.readFlag();
      val->isAdmin = s.readFlag();
      val->isLevelChanger = s.readFlag();
      val->isAuthenticated = false; //s.readFlag();  // we may set this by comparing Nonce id.
      val->nonce.read(&s);
      //gp->gameResult = "?";
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
      //gp->switchedTeams
      s.writeFlag(val.isRobot);
      s.writeFlag(val.isAdmin);
      s.writeFlag(val.isLevelChanger);
      //gp->isAuthenticated;;  // we may set this by comparing Nonce id.
      val.nonce.write(&s);

      write(s, val.weaponStats);
	}
   void read(TNL::BitStream &s, Zap::TeamStats *val)
	{
      val->name = readString(s);
      val->score = readS32(s);
      val->color_bin = s.readInt(24); // 24 bit color
      char c[24];
      dSprintf(c, sizeof(c), "%.6X", val->color_bin);
      val->color = string(c);
      //gt->gameResult = "?";

		read(s, &val->playerStats);
	}
   void write(TNL::BitStream &s, Zap::TeamStats &val)
	{
      writeString(s, val.name);
      write(s, S32(val.score));
      s.writeInt(val.color_bin,24); // 24 bit color
      write(s, val.playerStats);
	}
   void read(TNL::BitStream &s, Zap::GameStats *val)
	{
      val->isOfficial = s.readFlag();
      val->playerCount = readU16(s);
      val->duration = readU16(s);     // game length in seconds
      val->isTeamGame = s.readFlag();
      //g->isTied = s.readFlag();
      val->gameType = readString(s);
      val->levelName = readString(s);
      read(s, &val->teamStats);
      val->teamCount = val->teamStats.size(); // is this needed?

	}
   void write(TNL::BitStream &s, Zap::GameStats &val)
	{
      s.writeFlag(val.isOfficial);
      write(s, U16(val.playerCount));
      write(s, U16(val.duration));     // game length in seconds
      s.writeFlag(val.isTeamGame);
      //s.writeFlag(g->isTied);
      writeString(s, val.gameType);
      writeString(s, val.levelName);
      write(s, val.teamStats);
	}

	
	const U8 VersionedGameStats_CurrentVersion = 0;

   /// Reads objects from a BitStream.
   void read(TNL::BitStream &s, VersionedGameStats *val)
   {
      VersionedGameStats_read_start(s);
      U8 version = readU8(s);  // Read version number.
      val->version = version;
      read(s, &val->gameStats); // This is not Vector, goes directly to read(s, Zap::GameStats *val)

         // Stops here if TNL_ENABLE_ASSERTS is on, and write/read size is not matched.
      VersionedGameStats_read_end(s);
      val->valid = true;
   }


   /// Writes objects into a BitStream. Server write and send to master.
   void write(TNL::BitStream &s, VersionedGameStats &val)
   {
      VersionedGameStats_write_start(s);
      write(s, U8(VersionedGameStats_CurrentVersion));       // send current version
      write(s, val.gameStats);

      // write(number) write 32 bit when number is S32
      // write(number) write 8 bit when number is U8
      // write(U16(number)) always write 16 bit when number is U8, S32, or any type.

      VersionedGameStats_write_end(s);
   }
}
