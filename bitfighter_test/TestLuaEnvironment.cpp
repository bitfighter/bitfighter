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

class LuaEnvironmentTest : public testing::Test {
protected:
   ServerGame *serverGame;
   GameSettingsPtr settings;

   virtual void SetUp() {
      serverGame = newServerGame();

      settings = serverGame->getSettingsPtr();

      LuaScriptRunner::setScriptingDir(settings->getFolderManager()->luaDir);
      EXPECT_TRUE(LuaScriptRunner::startLua());
   }


   virtual void TearDown()
   {
      LuaScriptRunner::shutdown();

      delete serverGame;
      serverGame = NULL;
   }
};


TEST_F(LuaEnvironmentTest, BasicTests)
{
	LuaLevelGenerator levelgen(serverGame);
	levelgen.runScript(false);

	EXPECT_FALSE(levelgen.runString("a = b.b"));

}


};
