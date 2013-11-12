//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "../zap/ServerGame.h"
#include "../zap/gameType.h"
#include "../zap/luaLevelGenerator.h"
#include "../zap/SystemFunctions.h"
#include "gtest/gtest.h"

namespace Zap
{

using namespace std;
using namespace TNL;


TEST(LuaEnvironmentTest, EnvironmentSetup)
{
   ServerGame *serverGame = newServerGame();
   GameSettingsPtr settings = serverGame->getSettingsPtr();

   LuaScriptRunner::setScriptingDir(settings->getFolderManager()->luaDir);
	EXPECT_TRUE(LuaScriptRunner::startLua());


//	LuaLevelGenerator levelgen(serverGame);
//	levelgen.runScript(false);
//
//	EXPECT_TRUE(levelgen.runString("bf:addItem(Robot.new())"));

}


};
