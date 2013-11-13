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
   GameSettingsPtr settings;  // Will be cleaned up automatically

   lua_State *L;

   LuaLevelGenerator *levelgen;


   virtual void SetUp() {
      serverGame = newServerGame();
      settings = serverGame->getSettingsPtr();

      // Set-up our environment
      LuaScriptRunner::setScriptingDir(settings->getFolderManager()->luaDir);
      EXPECT_TRUE(LuaScriptRunner::startLua());

      // Set up a levelgen object, with no script
      levelgen = new LuaLevelGenerator(serverGame);

      // Ensure environment set-up
      EXPECT_TRUE(levelgen->prepareEnvironment());

      // Grab our Lua state
      L = LuaScriptRunner::getL();
      EXPECT_TRUE(L);
   }


   virtual void TearDown()
   {
      delete levelgen;

      LuaScriptRunner::shutdown();

      delete serverGame;
   }


   bool existsFunctionInEnvironment(const string &functionName)
   {
      return LuaScriptRunner::loadFunction(L, levelgen->getScriptId(), functionName.c_str());
   }

};


TEST_F(LuaEnvironmentTest, BasicTests)
{
   // Test exception throwing
   EXPECT_FALSE(levelgen->runString("a = b.b"));
}


TEST_F(LuaEnvironmentTest, SandboxTests)
{
   // Test that our sandbox is working, try to load an evil function
   EXPECT_FALSE(existsFunctionInEnvironment("setfenv"));
}


};
