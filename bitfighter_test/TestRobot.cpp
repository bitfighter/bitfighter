//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"

#include "../zap/ClientGame.h"
#include "../zap/ServerGame.h"
#include "../zap/gameType.h"
#include "../zap/luaLevelGenerator.h"

#include "gtest/gtest.h"

namespace Zap
{

using namespace std;
using namespace TNL;

TEST(RobotTest, addBot)
{
	GamePair gamePair;
	Vector<string> empty;

	EXPECT_EQ(0, gamePair.server->getRobotCount());
	EXPECT_EQ(0, gamePair.getClient(0)->getRobotCount());

	gamePair.server->addBot(empty, ClientInfo::ClassRobotAddedByAddbots);

	for(U32 i = 0; i < 10; i++)
		gamePair.idle(10);
	
	EXPECT_EQ(1, gamePair.server->getRobotCount());
	EXPECT_EQ(1, gamePair.getClient(0)->getRobotCount());
}


TEST(RobotTest, luaRobotNew)
{
	GamePair gamePair;

	EXPECT_EQ(0, gamePair.server->getRobotCount());
	EXPECT_EQ(0, gamePair.getClient(0)->getRobotCount());

	LuaLevelGenerator levelgen(gamePair.server);
	levelgen.runScript(false);

   // No robots yet!
   EXPECT_TRUE(levelgen.runString("t = bf:findAllObjects(ObjType.Robot) "
                                  "assert(#t == 0)"));
   EXPECT_EQ(0, gamePair.server->getRobotCount());

   // Add a robot!
	EXPECT_TRUE(levelgen.runString("bf:addItem(Robot.new()) "
                                  "t = bf:findAllObjects(ObjType.Robot) "
                                  "assert(#t == 1)"));
   EXPECT_EQ(1, gamePair.server->getRobotCount());

	gamePair.idle(10, 10);
	
	EXPECT_EQ(1, gamePair.server->getRobotCount());
	EXPECT_EQ(1, gamePair.getClient(0)->getRobotCount());

   EXPECT_TRUE(levelgen.runString("bots = bf:findAllObjects(ObjType.Robot) "
                                  "assert(#t == 1) "
                                  "bots[1]:removeFromGame()"));

   // Lua apparently sees the bot object as being gone immediately
   EXPECT_TRUE(levelgen.runString("t = bf:findAllObjects(ObjType.Robot) "
                                  "assert(#t == 0)"));

	gamePair.idle(20, 10);     // More than 100ms; removing bot from game has a 100ms delay
	
   EXPECT_EQ(0, gamePair.server->getRobotCount()); // RobotCount isn't decremented until bot is finally killed off
	EXPECT_EQ(0, gamePair.getClient(0)->getRobotCount());
}


/** onShipSpawned doesn't fire?

TEST(RobotTest, RemoveFromGameDuringInitialOnShipSpawn)
{
	GamePair gamePair;

	EXPECT_EQ(0, gamePair.server->getRobotCount());
	EXPECT_EQ(0, gamePair.client->getRobotCount());

	LuaLevelGenerator levelgen(gamePair.server);
	levelgen.runScript(false);

	EXPECT_TRUE(levelgen.runString("onShipSpawned = function(ship) RUN = true ; ship:removeFromGame() end"));
	EXPECT_TRUE(levelgen.runString("subscribe(Event.ShipSpawned)"));
	
	Vector<StringTableEntry> args;
	gamePair.server->getGameType()->addBot(args);

	for(U32 i = 0; i < 10; i++)
		gamePair.idle(10);

	EXPECT_TRUE(levelgen.runString("assert(RUN)"));

	EXPECT_EQ(0, gamePair.server->getRobotCount());
	EXPECT_EQ(0, gamePair.client->getRobotCount());
}
*/


};