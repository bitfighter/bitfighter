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
      ASSERT_TRUE(LuaScriptRunner::startLua(settings->getFolderManager()->getLuaDir()));

      // Set up a levelgen object, with no script
      levelgen = new LuaLevelGenerator(serverGame);

      // Ensure environment set-up
      ASSERT_TRUE(levelgen->prepareEnvironment());

      // Grab our Lua state
      L = LuaScriptRunner::getL();
      ASSERT_TRUE(L);
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


TEST_F(LuaEnvironmentTest, sanityCheck)
{
   // Test exception throwing
   EXPECT_FALSE(levelgen->runString("a = b.b"));
}


TEST_F(LuaEnvironmentTest, sandbox)
{
   // Ensure that local setmetatable refs in sandbox are not globalized somehow
   EXPECT_FALSE(existsFunctionInEnvironment("smt"));
   EXPECT_FALSE(existsFunctionInEnvironment("gmt"));

   // Sandbox prohibits access to unsafe functions, a few listed here
   EXPECT_FALSE(existsFunctionInEnvironment("setfenv"));
   EXPECT_FALSE(existsFunctionInEnvironment("setmetatable"));

   // But it should not interfere with permitted functions
   EXPECT_TRUE(existsFunctionInEnvironment("unpack"));
   EXPECT_TRUE(existsFunctionInEnvironment("ipairs"));
   EXPECT_TRUE(existsFunctionInEnvironment("require"));
}


TEST_F(LuaEnvironmentTest, scriptIsolation)
{
   LuaLevelGenerator levelgen2(serverGame);
   levelgen2.prepareEnvironment();
   lua_State *L = levelgen->getL();

   // All scripts should have separate environment tables
   lua_getfield(L, LUA_REGISTRYINDEX, levelgen->getScriptId());
   lua_getfield(L, LUA_REGISTRYINDEX, levelgen2.getScriptId());
   EXPECT_FALSE(lua_equal(L, -1, -2));
   lua_pop(L, 2);

   // Scripts can mess with their own environment, but not others'
   EXPECT_TRUE(levelgen->runString("levelgen = nil"));
   EXPECT_TRUE(levelgen->runString("assert(levelgen == nil)"));
   EXPECT_TRUE(levelgen2.runString("assert(levelgen ~= nil)"));

   EXPECT_TRUE(levelgen->runString("BfObject= nil"));
   EXPECT_TRUE(levelgen->runString("assert(BfObject== nil)"));
   EXPECT_TRUE(levelgen2.runString("assert(BfObject~= nil)"));

   /* A true deep copy is needed before these will pass
   EXPECT_TRUE(levelgen->runString("Timer.foo = 'test'"));
   EXPECT_TRUE(levelgen->runString("assert(Timer.foo == 'test')"));
   EXPECT_TRUE(levelgen2.runString("assert(Timer.foo ~= 'test')"));
   */
}


TEST_F(LuaEnvironmentTest, immutability)
{
   EXPECT_FALSE(levelgen->runString("string.sub = nil"));
}


TEST_F(LuaEnvironmentTest, findAllObjects)
{
   EXPECT_TRUE(levelgen->runString("bf:addItem(ResourceItem.new(point.new(0,0)))"));
   EXPECT_TRUE(levelgen->runString("bf:addItem(ResourceItem.new(point.new(300,300)))"));
   EXPECT_TRUE(levelgen->runString("bf:addItem(TestItem.new(point.new(200,200)))"));


   EXPECT_TRUE(levelgen->runString("t = { }"));
   EXPECT_TRUE(levelgen->runString("bf:findAllObjects(t)"));
   EXPECT_TRUE(levelgen->runString("assert(#t == 3)"));

   EXPECT_TRUE(levelgen->runString("t = { }"));

   EXPECT_TRUE(levelgen->runString("bf:findAllObjects(t, ObjType.ResourceItem)"));
   EXPECT_TRUE(levelgen->runString("assert(#t == 2)"));
   EXPECT_TRUE(levelgen->runString("bf:findAllObjects(t, ObjType.ResourceItem)"));

   EXPECT_TRUE(levelgen->runString("t = bf:findAllObjects()"));
   EXPECT_TRUE(levelgen->runString("bf:findAllObjects(t, ObjType.ResourceItem)"));
   EXPECT_TRUE(levelgen->runString("assert(#t == 3)"));
   EXPECT_TRUE(levelgen->runString("t = bf:findAllObjects(ObjType.ResourceItem)"));
   EXPECT_TRUE(levelgen->runString("assert(#t == 2)"));
}


};
