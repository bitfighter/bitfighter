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
	Vector<const char *> args;

	EXPECT_EQ(0, gamePair.server->getRobotCount());
	EXPECT_EQ(0, gamePair.getClient(0)->getRobotCount());

	gamePair.server->addBot(args, ClientInfo::ClassRobotAddedByAddbots);

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

	EXPECT_TRUE(levelgen.runString("bf:addItem(Robot.new())"));

	for(U32 i = 0; i < 100; i++)
		gamePair.idle(10);
	
	EXPECT_EQ(1, gamePair.server->getRobotCount());
	EXPECT_EQ(1, gamePair.getClient(0)->getRobotCount());

	EXPECT_TRUE(levelgen.runString("bots = bf:findAllObjects(ObjType.Robot); bots[1]:removeFromGame()"));

	for(U32 i = 0; i < 100; i++)
		gamePair.idle(10);
	
	EXPECT_EQ(0, gamePair.server->getRobotCount());
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