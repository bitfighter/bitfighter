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
#include "../master/database.h"

#include "tnlMethodDispatch.h"

// This is shared in both client and master.
// this read/write is usually the hardest part about this struct, but this allows custom version handling.

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


////////////////////////////////////////
////////////////////////////////////////

PlayerStats::PlayerStats()    // Constructor
{      
   isAuthenticated = false;
   isRobot         = false;
   isAdmin         = false;
   isLevelChanger  = false;
   isHosting       = false;

   gameResult = 0;

   points              = 0;
   kills               = 0;
   deaths              = 0;
   suicides            = 0;
   switchedTeamCount   = 0;
   flagPickup          = 0;
   flagDrop            = 0;
   flagReturn          = 0;
   flagScore           = 0;
   crashedIntoAsteroid = 0;
   changedLoadout      = 0;
   teleport            = 0;
   playTime            = 0;
   fratricides         = 0;

   // V3 stats
   turretKills         = 0;
   ffKills             = 0;
   astKills            = 0;
   turretsEngr         = 0;
   ffEngr              = 0;
   telEngr             = 0;
   distTraveled        = 0;
}


////////////////////////////////////////
////////////////////////////////////////

TeamStats::TeamStats()      // Constructor
{
   intColor    = 0;
   score       = 0;
   gameResult  = 0;
}


////////////////////////////////////////
////////////////////////////////////////

GameStats::GameStats()      // Constructor
{
   isOfficial = false;
   isTesting  = false;
   isTeamGame = false;

   cs_protocol_version = 0;
   build_version       = 0;
   playerCount         = 0;
   duration            = 0;
}


////////////////////////////////////////
////////////////////////////////////////

// This relies on scores being sent sorted in order of descending score
char getResult(S32 scores, S32 score1, S32 score2, S32 currScore, bool isFirst)
{
   if(scores == 1)      // Only one player/team, winner/loser makes no sense
      return 'X';
   else if(score1 == score2 && currScore == score1)     // Tie -- everyone with high score gets tie
      return 'T';
   else if(isFirst)     // No tie -- first one gets the win...
      return 'W';
   else                 // ...and everyone else gets the loss
      return 'L';
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
               getResult(playerStats->size(), (*playerStats)[0].points, playerStats->size() == 1 ? 
                                                            0 : (*playerStats)[1].points, (*playerStats)[j].points, j == 0);
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

}

////////////////////////////////////////
////////////////////////////////////////

namespace Types
{

U8  readU8(TNL::BitStream &s)  { U8  val; read(s, &val); return val; }
S8  readS8(TNL::BitStream &s)  { S8  val; read(s, &val); return val; }
U16 readU16(TNL::BitStream &s) { U16 val; read(s, &val); return val; }
S16 readS16(TNL::BitStream &s) { S16 val; read(s, &val); return val; }
U32 readU32(TNL::BitStream &s) { U32 val; read(s, &val); return val; }
S32 readS32(TNL::BitStream &s) { S32 val; read(s, &val); return val; }
// for bool, use   s.readFlag();


string readString(TNL::BitStream &s) { char val[256]; s.readString(val); return val; }
void writeString(TNL::BitStream &s, const string &val) { s.writeString(val.c_str()); }


// Use of this compression helps when number are likely to be a low number
// zero takes only 1 bit, -128 to 128 takes 10 bit, or else it will take 34 bits
void writeCompressedS32(TNL::BitStream &s, S32 value)
{
   if(s.writeFlag(value != 0))
   {
      if(s.writeFlag(value <= 128 && value >= -128))
         s.writeInt(U32(value > 0 ? value - 1 : value + 256), 8);
      else
         s.writeInt((U32)value, 32);
   }
}


S32 readCompressedS32(TNL::BitStream &s)
{
   if(s.readFlag())
   {
      if(s.readFlag())
      {
         S32 out = S32(s.readInt(8));
         return out <= 127 ? out + 1 : out - 256;
      }
      else
         return S32(s.readInt(32));
   }
   else
      return 0;
}


// Unsigned version, 1 - 256 will take 10 bits
void writeCompressedU32(TNL::BitStream &s, U32 value)
{
   if(s.writeFlag(value != 0))
   {
      if(s.writeFlag(value == (value << 24 >> 24)))
         s.writeInt(value, 8);
      else
         s.writeInt(value, 32);
   }
}


U32 readCompressedU32(TNL::BitStream &s)
{
   if(s.readFlag())
   {
      if(s.readFlag())
         return s.readInt(8);
      else
         return s.readInt(32);
   }
   else
      return 0;
}


void read(TNL::BitStream &s, Zap::LoadoutStats *val, U8 version)
{
   if(version >= 2)
      val->loadoutHash = readCompressedU32(s);
}


void write(TNL::BitStream &s, Zap::LoadoutStats &val, U8 version)
{
   if(version >= 2)
      writeCompressedU32(s, val.loadoutHash);
}


void read(TNL::BitStream &s, Zap::WeaponStats *val, U8 version)
{
   val->weaponType = WeaponType(readU8(s));

   if(version == 0)
   {
      val->shots = readU16(s);
      val->hits  = readU16(s);
      val->hitBy = 0;
   }
   else
   {
      val->shots = readCompressedU32(s);
      val->hits  = readCompressedU32(s);
      val->hitBy = readCompressedU32(s);
   }
}


void write(TNL::BitStream &s, Zap::WeaponStats &val, U8 version)
{
   write(s, U8(val.weaponType));

   if(version == 0)
   {
      write(s, U16(val.shots));
      write(s, U16(val.hits));
   }
   else
   {
      writeCompressedU32(s, val.shots);
      writeCompressedU32(s, val.hits);
      writeCompressedU32(s, val.hitBy);
   }
}


// Read/write is called by write(TNL::BitStream &s, TNL::Vector<T> &val, A arg1); arg1 is passed to us as a 3rd param
// which we don't care about.
void read(TNL::BitStream &s, Zap::ModuleStats *val, U8 dummy)
{
   val->shipModule = ShipModule(readU8(s));
   val->seconds = readCompressedU32(s);
}


void write(TNL::BitStream &s, Zap::ModuleStats &val, U8 dummy)
{
   write(s, U8(val.shipModule));
   writeCompressedU32(s, val.seconds);
}


void readVersion0Stats(TNL::BitStream &s, Zap::PlayerStats *val)
{
   val->points = readS32(s);
   val->kills = readU16(s);
   val->deaths = readU16(s);
   val->suicides = readU16(s);
   val->switchedTeamCount = readU8(s);
   val->isRobot = s.readFlag();
   val->isAdmin = s.readFlag();
   val->isLevelChanger = s.readFlag();
   val->nonce.read(&s);
}


void writeVersion0Stats(TNL::BitStream &s, Zap::PlayerStats &val)
{
   write(s, S32(val.points));
   write(s, U16(val.kills));
   write(s, U16(val.deaths));
   write(s, U16(val.suicides));
   write(s, U8(val.switchedTeamCount));
   s.writeFlag(val.isRobot);
   s.writeFlag(val.isAdmin);
   s.writeFlag(val.isLevelChanger);
   val.nonce.write(&s);
}


void readVersion1Stats(TNL::BitStream &s, Zap::PlayerStats *val)
{
   val->points = readCompressedS32(s);
   val->kills = readCompressedU32(s);
   val->deaths = readCompressedU32(s);
   val->suicides = readCompressedU32(s);
   val->switchedTeamCount = readCompressedU32(s);

   val->isRobot = s.readFlag();
   val->isAdmin = s.readFlag();
   val->isLevelChanger = s.readFlag();
   val->isHosting = s.readFlag();
   val->isAuthenticated = s.readFlag();

   if(val->isAuthenticated)
      val->nonce.read(&s); // Only needed if server claims a player is authenticated

   val->fratricides = readCompressedU32(s);
   val->flagPickup = readCompressedU32(s);
   val->flagDrop = readCompressedU32(s);
   val->flagReturn = readCompressedU32(s);
   val->flagScore = readCompressedU32(s);
   val->crashedIntoAsteroid = readCompressedU32(s);
   val->changedLoadout = readCompressedU32(s);
   val->teleport = readCompressedU32(s);
   val->playTime = readCompressedU32(s);

   read(s, &val->moduleStats, U8(0));     // This 0 ends up in the dummy param described higher up
}


void writeVersion1Stats(TNL::BitStream &s, Zap::PlayerStats &val)
{
   writeCompressedS32(s, val.points);
   writeCompressedU32(s, val.kills);
   writeCompressedU32(s, val.deaths);
   writeCompressedU32(s, val.suicides);
   writeCompressedU32(s, val.switchedTeamCount);

   s.writeFlag(val.isRobot);
   s.writeFlag(val.isAdmin);
   s.writeFlag(val.isLevelChanger);
   s.writeFlag(val.isHosting);
   s.writeFlag(val.isAuthenticated);

   if(val.isAuthenticated)
      val.nonce.write(&s);    // Only needed if server claims a player is authenticated

   writeCompressedU32(s, val.fratricides);
   writeCompressedU32(s, val.flagPickup);
   writeCompressedU32(s, val.flagDrop);
   writeCompressedU32(s, val.flagReturn);
   writeCompressedU32(s, val.flagScore);
   writeCompressedU32(s, val.crashedIntoAsteroid);
   writeCompressedU32(s, val.changedLoadout);
   writeCompressedU32(s, val.teleport);
   writeCompressedU32(s, val.playTime);

   write(s, val.moduleStats, U8(0));    // This 0 ends up in the dummy param described higher up
}


void readVersion3Stats(TNL::BitStream &s, Zap::PlayerStats *val)
{
   val->turretKills  = readCompressedU32(s);
   val->ffKills      = readCompressedU32(s);
   val->astKills     = readCompressedU32(s);
   val->turretsEngr  = readCompressedU32(s);
   val->ffEngr       = readCompressedU32(s);
   val->telEngr      = readCompressedU32(s);
   val->distTraveled = readCompressedU32(s);
}


void writeVersion3Stats(TNL::BitStream &s, const Zap::PlayerStats &val)
{
   writeCompressedU32(s, val.turretKills);
   writeCompressedU32(s, val.ffKills);
   writeCompressedU32(s, val.astKills);
   writeCompressedU32(s, val.turretsEngr);
   writeCompressedU32(s, val.ffEngr);
   writeCompressedU32(s, val.telEngr);
   writeCompressedU32(s, val.distTraveled);
}


void read(TNL::BitStream &s, Zap::PlayerStats *val, U8 version)
{
   val->name = readString(s);

   if(version == 0)
      readVersion0Stats(s, val);

   if(version >= 1)
      readVersion1Stats(s, val);

   read(s, &val->loadoutStats, version);
   read(s, &val->weaponStats, version);

   if(version >= 3)
      readVersion3Stats(s, val);
}


void write(TNL::BitStream &s, Zap::PlayerStats &val, U8 version)
{
   writeString(s, val.name);

   if(version == 0)
      writeVersion0Stats(s, val);
      
   if(version >= 1)
      writeVersion1Stats(s, val);

   write(s, val.loadoutStats, version);
   write(s, val.weaponStats, version);

   if(version >= 3)
      writeVersion3Stats(s, val);
}


void read(TNL::BitStream &s, Zap::TeamStats *val, U8 version)
{
   val->name  = readString(s);
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

   if(version == 0)
   {
      readU16(s);                   // playerCount - calculated below
      val->duration = readU16(s);   // game length in seconds
      val->isTesting = false;
      val->build_version = 0;
   }
   else
   {
      val->duration = readCompressedU32(s);
      val->isTesting = s.readFlag();
      val->build_version = readS32(s);
   }

   val->isTeamGame = s.readFlag();
   val->gameType = readString(s);
   val->levelName = readString(s);
   read(s, &val->teamStats, version);

   val->playerCount = 0;

   for(S32 i = 0; i < val->teamStats.size(); i++)
      val->playerCount += val->teamStats[i].playerStats.size();  // count number of players
}


void write(TNL::BitStream &s, Zap::GameStats &val, U8 version)
{
   s.writeFlag(val.isOfficial);

   if(version == 0)
   {
      write(s, U16(val.playerCount));
      write(s, U16(val.duration));     // game length in seconds
   }
   else
   {
      writeCompressedU32(s, val.duration);
      s.writeFlag(val.isTesting);
      write(s, S32(val.build_version));
   }

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


/// Writes objects into a BitStream.  Server write and send to master.
void write(TNL::BitStream &s, VersionedGameStats &val)
{
   U32 bitStart = s.getBitPosition();

#ifdef TNL_ENABLE_ASSERTS
   for(S32 i = -200; i < 200; i++)
   {  writeCompressedS32(s, i);
      s.setBitPosition(bitStart);
      TNLAssert(readCompressedS32(s) == i, "Problem with read / write CompressedS32");
      s.setBitPosition(bitStart);
   }
   for(U32 i = 0; i < 400; i++)
   {  writeCompressedU32(s, i);
      s.setBitPosition(bitStart);
      TNLAssert(readCompressedU32(s) == i, "Problem with read / write CompressedU32");
      s.setBitPosition(bitStart);
   }
#endif

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
