//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gameStats.h"
#include "tnlBitStream.h"
#include "gtest/gtest.h"

using namespace TNL;
using namespace Zap;

namespace Zap
{

TEST(GameStatsTest, CompressedS32)
{
   PacketStream stream;
   S32 values[] = {0, 1, -1, 127, 128, -128, -129, 256, -256, 123456, -123456, S32_MAX, S32_MIN};

   for(size_t i = 0; i < sizeof(values)/sizeof(values[0]); ++i)
   {
      stream.reset();
      Types::writeCompressedS32(stream, values[i]);
      stream.setBitPosition(0);
      S32 result = Types::readCompressedS32(stream);
      EXPECT_EQ(values[i], result) << "Failed for value: " << values[i];
   }
}

TEST(GameStatsTest, CompressedU32)
{
   PacketStream stream;
   U32 values[] = {0, 1, 127, 128, 255, 256, 123456, U32_MAX};

   for(size_t i = 0; i < sizeof(values)/sizeof(values[0]); ++i)
   {
      stream.reset();
      Types::writeCompressedU32(stream, values[i]);
      stream.setBitPosition(0);
      U32 result = Types::readCompressedU32(stream);
      EXPECT_EQ(values[i], result) << "Failed for value: " << values[i];
   }
}

TEST(GameStatsTest, LoadoutStats)
{
   PacketStream stream;
   LoadoutStats src, dest;
   src.loadoutHash = 0xDEADBEEF;

   Types::write(stream, src, 1);
   stream.setBitPosition(0);
   Types::read(stream, &dest, 1);

   EXPECT_EQ(src.loadoutHash, dest.loadoutHash);
}

TEST(GameStatsTest, WeaponStatsV0)
{
   PacketStream stream;
   WeaponStats src, dest;
   src.weaponType = WeaponPhaser;
   src.shots = 100;
   src.hits = 50;
   src.hitBy = 10; // Not sent in V0

   Types::write(stream, src, 0);
   stream.setBitPosition(0);
   Types::read(stream, &dest, 0);

   EXPECT_EQ(src.weaponType, dest.weaponType);
   EXPECT_EQ(src.shots, dest.shots);
   EXPECT_EQ(src.hits, dest.hits);
   EXPECT_EQ(0u, dest.hitBy);
}

TEST(GameStatsTest, WeaponStatsV1)
{
   PacketStream stream;
   WeaponStats src, dest;
   src.weaponType = WeaponBurst;
   src.shots = 1000;
   src.hits = 500;
   src.hitBy = 200;

   Types::write(stream, src, 1);
   stream.setBitPosition(0);
   Types::read(stream, &dest, 1);

   EXPECT_EQ(src.weaponType, dest.weaponType);
   EXPECT_EQ(src.shots, dest.shots);
   EXPECT_EQ(src.hits, dest.hits);
   EXPECT_EQ(src.hitBy, dest.hitBy);
}

TEST(GameStatsTest, ModuleStats)
{
   PacketStream stream;
   ModuleStats src, dest;
   src.shipModule = ModuleShield;
   src.seconds = 3600;

   Types::write(stream, src);
   stream.setBitPosition(0);
   Types::read(stream, &dest);

   EXPECT_EQ(src.shipModule, dest.shipModule);
   EXPECT_EQ(src.seconds, dest.seconds);
}

TEST(GameStatsTest, PlayerStatsV3)
{
   PacketStream stream;
   PlayerStats src, dest;
   src.name = "TestPlayer";
   src.isAuthenticated = true;
   // src.nonce initialization if needed
   src.isRobot = false;
   src.points = 1234;
   src.kills = 10;
   src.deaths = 5;
   src.suicides = 1;
   src.switchedTeamCount = 2;
   src.fratricides = 0;
   src.flagPickup = 3;
   src.flagDrop = 1;
   src.flagReturn = 1;
   src.flagScore = 1;
   src.crashedIntoAsteroid = 2;
   src.changedLoadout = 5;
   src.teleport = 4;
   src.playTime = 600;

   WeaponStats ws;
   ws.weaponType = WeaponPhaser;
   ws.shots = 100;
   ws.hits = 50;
   ws.hitBy = 5;
   src.weaponStats.push_back(ws);

   ModuleStats ms;
   ms.shipModule = ModuleBoost;
   ms.seconds = 120;
   src.moduleStats.push_back(ms);

   LoadoutStats ls;
   ls.loadoutHash = 12345;
   src.loadoutStats.push_back(ls);

   src.turretKills = 1;
   src.ffKills = 2;
   src.astKills = 3;
   src.turretsEngr = 4;
   src.ffEngr = 5;
   src.telEngr = 6;
   src.distTraveled = 7890;

   Types::write(stream, src, 3);
   stream.setBitPosition(0);
   Types::read(stream, &dest, 3);

   EXPECT_EQ(src.name, dest.name);
   EXPECT_EQ(src.points, dest.points);
   EXPECT_EQ(src.kills, dest.kills);
   EXPECT_EQ(src.deaths, dest.deaths);
   EXPECT_EQ(src.isAuthenticated, dest.isAuthenticated);
   EXPECT_EQ(src.weaponStats.size(), dest.weaponStats.size());
   EXPECT_EQ(src.weaponStats[0].weaponType, dest.weaponStats[0].weaponType);
   EXPECT_EQ(src.moduleStats.size(), dest.moduleStats.size());
   EXPECT_EQ(src.loadoutStats.size(), dest.loadoutStats.size());
   EXPECT_EQ(src.turretKills, dest.turretKills);
   EXPECT_EQ(src.distTraveled, dest.distTraveled);
}

TEST(GameStatsTest, TeamStats)
{
   PacketStream stream;
   TeamStats src, dest;
   src.name = "Blue Team";
   src.score = 100;
   src.intColor = 0x0000FF;

   PlayerStats ps;
   ps.name = "Player1";
   src.playerStats.push_back(ps);

   Types::write(stream, src, 3);
   stream.setBitPosition(0);
   Types::read(stream, &dest, 3);

   EXPECT_EQ(src.name, dest.name);
   EXPECT_EQ(src.score, dest.score);
   EXPECT_EQ(src.intColor, dest.intColor);
   EXPECT_EQ(src.playerStats.size(), dest.playerStats.size());
   EXPECT_EQ(src.playerStats[0].name, dest.playerStats[0].name);
}

TEST(GameStatsTest, GameStats)
{
   PacketStream stream;
   GameStats src, dest;
   src.isOfficial = true;
   src.duration = 1200;
   src.isTesting = false;
   src.build_version = 12345;
   src.isTeamGame = true;
   src.gameType = "CTF";
   src.levelName = "TestLevel";

   TeamStats ts;
   ts.name = "Red Team";
   src.teamStats.push_back(ts);

   Types::write(stream, src, 3);
   stream.setBitPosition(0);
   Types::read(stream, &dest, 3);

   EXPECT_EQ(src.isOfficial, dest.isOfficial);
   EXPECT_EQ(src.duration, dest.duration);
   EXPECT_EQ(src.gameType, dest.gameType);
   EXPECT_EQ(src.levelName, dest.levelName);
   EXPECT_EQ(src.teamStats.size(), dest.teamStats.size());
   EXPECT_EQ(src.playerCount, dest.playerCount);
}

TEST(GameStatsTest, VersionedGameStats)
{
   PacketStream stream;
   VersionedGameStats src, dest;
   src.gameStats.levelName = "BigGame";

   // Need at least one team as per read implementation
   TeamStats ts;
   ts.name = "Solitary Team";
   src.gameStats.teamStats.push_back(ts);

   Types::write(stream, src);
   stream.setBitPosition(0);
   Types::read(stream, &dest);

   EXPECT_TRUE(dest.valid);
   EXPECT_EQ(src.gameStats.levelName, dest.gameStats.levelName);
}

} // namespace Zap
