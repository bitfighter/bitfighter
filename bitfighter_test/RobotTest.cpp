#include "TestUtils.h"
#include "../zap/ClientGame.h"
#include "../zap/ServerGame.h"
#include "../zap/gameType.h"
#include "../zap/luaLevelGenerator.h"
#include "gtest/gtest.h"
#include "lua.h"

namespace Zap
{

using namespace std;
using namespace TNL;

TEST(RobotTest, addBot)
{
	GamePair gamePair;
	Vector<StringTableEntry> args;

	EXPECT_EQ(gamePair.server->getRobotCount(), 0);
	EXPECT_EQ(gamePair.client->getRobotCount(), 0);

	gamePair.server->getGameType()->addBot(args);

	for(U32 i = 0; i < 10; i++)
		gamePair.idle(10);
	
	EXPECT_EQ(gamePair.server->getRobotCount(), 1);
	EXPECT_EQ(gamePair.client->getRobotCount(), 1);
}


TEST(RobotTest, luaRobotNew)
{
	GamePair gamePair;

	EXPECT_EQ(gamePair.server->getRobotCount(), 0);
	EXPECT_EQ(gamePair.client->getRobotCount(), 0);

	LuaLevelGenerator levelgen(gamePair.server);
	levelgen.runScript(false);
	lua_State *L = levelgen.getL();

	EXPECT_TRUE(levelgen.runString("bf:addItem(Robot.new())"));

	for(U32 i = 0; i < 10; i++)
		gamePair.idle(10);
	
	EXPECT_EQ(gamePair.server->getRobotCount(), 1);
	EXPECT_EQ(gamePair.client->getRobotCount(), 1);
}


};