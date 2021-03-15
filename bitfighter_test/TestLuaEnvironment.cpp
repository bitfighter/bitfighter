//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "../zap/ServerGame.h"
#include "../zap/gameType.h"
#include "../zap/luaLevelGenerator.h"
#include "../zap/SystemFunctions.h"
#include "../zap/robot.h"
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

   LuaLevelGenerator *levelgen = NULL;
   GamePair *gamePair = NULL;

   virtual void SetUp() {
      reset();
   }


   virtual void TearDown()
   {
      delete levelgen;

      LuaScriptRunner::shutdown();

      delete gamePair;
   }


   void reset()
   {
      if(gamePair)
         delete gamePair;

      if(levelgen)
         delete levelgen;

      gamePair = new GamePair();

      // We'll get our serverGame from a gamePair because that does a lot of other miscellaneous setup that we need
      // even if we never touch the clientGame half of the pair.
      serverGame = gamePair->server;

      //serverGame = newServerGame();
      settings = serverGame->getSettingsPtr();

      // Set-up our environment
      //EXPECT_TRUE(LuaScriptRunner::startLua(settings->getFolderManager()->luaDir));

      // Set up a levelgen object, with no script
      levelgen = new LuaLevelGenerator(serverGame);

      // Ensure environment set-up
      EXPECT_TRUE(levelgen->prepareEnvironment());

      // Grab our Lua state
      L = LuaScriptRunner::getL();
      EXPECT_TRUE(L);
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

   EXPECT_TRUE(levelgen->runString("BfObject = nil"));
   EXPECT_TRUE(levelgen->runString("assert(BfObject == nil)"));
   EXPECT_TRUE(levelgen2.runString("assert(BfObject ~= nil)"));

   /* A true deep copy is needed before these will pass
   EXPECT_TRUE(levelgen->runString("Timer.foo = 'test'"));
   EXPECT_TRUE(levelgen->runString("assert(Timer.foo == 'test')"));
   EXPECT_TRUE(levelgen2.runString("assert(Timer.foo ~= 'test')"));
   */
}


// Basic proof-of-life for robots
TEST_F(LuaEnvironmentTest, botLoading)
{
   LuaLevelGenerator levelgen(serverGame);

   Robot bot;
   bot.prepareEnvironment();
   serverGame->addBot(&bot);

   // Load a simple bot script
   bot.runString("function getName() return(\"ProofOfLifeBot\") end");

   // Make sure we can run it
   EXPECT_FALSE(bot.runCmd("getName", 0, 1));
   string name = lua_tostring(L, -1);
   EXPECT_EQ(name, "ProofOfLifeBot");

   clearStack(L);
}


// Demonstrate getting a variable from a script
TEST_F(LuaEnvironmentTest, accessGlobalScriptVar)
{
   LuaLevelGenerator levelgen(serverGame);

   Robot bot;
   bot.prepareEnvironment();

   serverGame->addBot(&bot);

   EXPECT_EQ(0, lua_gettop(L));
   bot.runString("myInt = 123; myStr = 'hello!'");

   EXPECT_EQ(123, bot.getLuaGlobalVar<S32>("myInt"));
   EXPECT_EQ("hello!", bot.getLuaGlobalVar<string>("myStr"));

   clearStack(L);
}


// Test levelgen - bot communication
TEST_F(LuaEnvironmentTest, scriptCommunication)
{
   Robot *bot = new Robot();
   bot->prepareEnvironment();
   serverGame->addBot(bot);

   /////
   // Test global chat method
   ASSERT_TRUE(bot->runString("function onMsgReceived(msg, player, isGlobal) message = msg end"));
   ASSERT_TRUE(bot->runString("bf:subscribe(Event.MsgReceived)"));

   for(U32 i = 0; i < 10; i++) serverGame->idle(10);   // Marinate

   ASSERT_TRUE(levelgen->runString("levelgen:globalMsg('Test sending global message')"));

   EXPECT_EQ("Test sending global message", bot->getLuaGlobalVar<string>("message"));
   ASSERT_EQ(0, lua_gettop(L));

   /////
   // Test sending data to bot method
   // Send a table containing multiple values
   // Note: The key to all these methods is that Lua varaibles are global by default, so message and int will be
   //       accessible with getLuaGlobalVar, which gives us an easy way to verify that the event handler was called,
   //       and with what arguments.
   ASSERT_TRUE(bot->runString("function onDataReceived(data) message = data.msg; int = data.int; end"));
   ASSERT_TRUE(bot->runString("bot:subscribe(Event.DataReceived)"));

   for(U32 i = 0; i < 10; i++) serverGame->idle(10);   // Marinate

   ASSERT_TRUE(levelgen->runString("levelgen:sendData({msg='Message in a table', int=765})"));

   EXPECT_EQ("Message in a table", bot->getLuaGlobalVar<string>("message"));
   EXPECT_EQ(765, bot->getLuaGlobalVar<S32>("int"));

   ASSERT_EQ(0, lua_gettop(L));


   // Sending mulitiple values
   ASSERT_TRUE(bot->runString("function onDataReceived(param1, param2, param3) p1 = param1; p2 = param2; p3 = param3; end"));

   for(U32 i = 0; i < 10; i++) serverGame->idle(10);   // Marinate

   ASSERT_TRUE(levelgen->runString("bf:sendData(100, 'two hundred', true)"));

   EXPECT_EQ(100, bot->getLuaGlobalVar<U32>("p1"));
   EXPECT_EQ("two hundred", bot->getLuaGlobalVar<string>("p2"));
   EXPECT_EQ(true, bot->getLuaGlobalVar<bool>("p3"));
   
   ASSERT_EQ(0, lua_gettop(L));


   // Send from bot to levelgen
   ASSERT_TRUE(levelgen->runString("function onDataReceived(a, b, c) ct = ct + 1; aaa = a; bbb=b; ccc= c; end;   ct = 0"));
   ASSERT_TRUE(levelgen->runString("levelgen:subscribe(Event.DataReceived)"));

   for(U32 i = 0; i < 10; i++) serverGame->idle(10);   // Marinate

   ASSERT_TRUE(bot->runString("bot:sendData(1, 2, 4)"));

   // Validate levelgen got the message (and take a poke at getLuaGlobalVar<xxx> while we're at it)
   EXPECT_EQ(1, levelgen->getLuaGlobalVar<S32>("aaa"));
   EXPECT_EQ(2, levelgen->getLuaGlobalVar<U8>("bbb"));
   EXPECT_EQ(4, levelgen->getLuaGlobalVar<S16>("ccc"));     
   EXPECT_EQ(1, levelgen->getLuaGlobalVar<U32>("ct"));      // Incrementing counter

   // But that the sender did not
   EXPECT_NE(1, bot->getLuaGlobalVar<S32>("p1"));
   EXPECT_NE(2, bot->getLuaGlobalVar<S32>("p2"));
   EXPECT_NE(3, bot->getLuaGlobalVar<S32>("p3"));


   // Send from bot to bot
   Robot* bot2 = new Robot();
   bot2->prepareEnvironment();
   serverGame->addBot(bot2);
   
   for(U32 i = 0; i < 10; i++) serverGame->idle(10);   // Marinate

   ASSERT_TRUE(bot2->runString("function onDataReceived(param1, param2, param3) pp1 = param1; pp2 = param2; pp3 = param3; end"));
   ASSERT_TRUE(bot2->runString("bf:subscribe(Event.DataReceived)"));    // We used bot:subscribe for other bot; both should work

   for(U32 i = 0; i < 10; i++) serverGame->idle(10);   // Marinate

   ASSERT_TRUE(bot->runString("bot:sendData(11, 12, 13)"));

   EXPECT_EQ(11, levelgen->getLuaGlobalVar<U16>("aaa"));
   EXPECT_EQ(12, levelgen->getLuaGlobalVar<S8>("bbb"));
   EXPECT_EQ(13, levelgen->getLuaGlobalVar<S32>("ccc"));

   EXPECT_EQ(11, bot2->getLuaGlobalVar<S32>("pp1"));  
   EXPECT_EQ(12, bot2->getLuaGlobalVar<S32>("pp2"));
   EXPECT_EQ(13, bot2->getLuaGlobalVar<S32>("pp3"));

   ASSERT_TRUE(bot->runString("bot:sendData(11, 12, 13)"));

   // Make sure error handling doesn't corrupt the stack with DataReceived (there's reason to suspect it might).
   // Bots will get called in the order they arrived; if bot crashes during event firing, make sure it doesn't
   // take down bot2 with it.
   ASSERT_TRUE(bot->runString("function onDataReceived(p1, p2, p3) x = p1[1] end"));   // <- script contains crashing bug
   // When the above gets run, it will trigger bot death as the code has an exception

   for(U32 i = 0; i < 10; i++) serverGame->idle(10);   // Marinate
   ASSERT_TRUE(levelgen->runString("bf:sendData(5, 6, 7)"));

   //for(U32 i = 0; i < 10; i++) serverGame->idle(10);   // Marinate to give dead bot time to delete itself

   // We want bot2 to get the data even if bot crashed handling the event
   EXPECT_EQ(5, bot2->getLuaGlobalVar<S32>("pp1"));
   EXPECT_EQ(6, bot2->getLuaGlobalVar<S32>("pp2"));
   EXPECT_EQ(7, bot2->getLuaGlobalVar<S32>("pp3"));

   // Resurrect bot with fixed script
   bot = new Robot();
   bot->prepareEnvironment();
   serverGame->addBot(bot);
   ASSERT_TRUE(bot->runString("function onDataReceived(param1, param2, param3) p1 = param1; p2 = param2; p3 = param3; end"));
   ASSERT_TRUE(bot->runString("bot:subscribe(Event.DataReceived)"));
   for(U32 i = 0; i < 10; i++) serverGame->idle(10);   // Marinate


   ///// Shared table functionality
   ASSERT_TRUE(levelgen->runString("tbl = {x=10}"));      // Create original copy of shared table in levelgen
   ASSERT_TRUE(levelgen->runString("bf:sendData(tbl, 'a', 'b')"));    

   // tbl gets copied to globals p1 and pp1 in bot and bot2 respectively.  So what happens when we change it?
   EXPECT_TRUE(bot->runString("assert(p1.x == 10)"));
   EXPECT_TRUE(bot2->runString("assert(pp1.x == 10)"));
   
   ASSERT_TRUE(levelgen->runString("tbl.x = 100"));       // Change the value in the levelgen
   EXPECT_TRUE(bot->runString("assert(p1.x == 100)"));    // New value appears, as if by magic, in the bots
   EXPECT_TRUE(bot2->runString("assert(pp1.x == 100)"));  

   // Updating the value in a bot alters the value in the levelgen and the other bot.  Weird.
   ASSERT_TRUE(bot->runString("p1.x = 200"));
   EXPECT_TRUE(levelgen->runString("assert(tbl.x == 200)"));  
   EXPECT_TRUE(bot2->runString("assert(pp1.x == 200)"));  

   // Added keys show up as well
   ASSERT_TRUE(bot2->runString("pp1.y = 30"));      
   EXPECT_TRUE(levelgen->runString("assert(tbl.y == 30)"));  
   EXPECT_TRUE(bot->runString("assert(p1.y == 30)"));
   EXPECT_EQ(3, levelgen->getLuaGlobalVar<S32>("ct"));    // Counter increments every time levelgen event handler runs


   ///// Mismatched argument counts

   // Make sure things work when we send too many values:
   ASSERT_TRUE(bot->runString("bf:sendData(6, 7, 8, 9)"));
   EXPECT_EQ(6, levelgen->getLuaGlobalVar<S32>("aaa"));
   EXPECT_EQ(7, levelgen->getLuaGlobalVar<S32>("bbb"));
   EXPECT_EQ(8, levelgen->getLuaGlobalVar<S32>("ccc"));
   EXPECT_EQ(4, levelgen->getLuaGlobalVar<S32>("ct"));    // Counter increments every time levelgen event handler runs

   EXPECT_EQ(6, bot2->getLuaGlobalVar<S32>("pp1"));
   EXPECT_EQ(7, bot2->getLuaGlobalVar<S32>("pp2"));
   EXPECT_EQ(8, bot2->getLuaGlobalVar<S32>("pp3"));

   // Or too few
   ASSERT_TRUE(levelgen->runString("levelgen:sendData(-1, 1.5)"));
   EXPECT_EQ(-1, bot->getLuaGlobalVar<S32>("p1"));
   EXPECT_FLOAT_EQ(1.5, bot->getLuaGlobalVar<F32>("p2"));
   EXPECT_EQ(0, bot->getLuaGlobalVar<F32>("p3"));         // Missing arg defaults to nil, which translates to F32 value 0 

   EXPECT_EQ(-1, bot2->getLuaGlobalVar<S32>("pp1"));
   EXPECT_FLOAT_EQ(1.5, bot2->getLuaGlobalVar<F32>("pp2"));
   EXPECT_EQ(0, bot2->getLuaGlobalVar<F32>("pp3"));

   EXPECT_EQ(4, levelgen->getLuaGlobalVar<S32>("ct"));    // Levelgen's incrementing counter shouldn't change when levelgen makes the call

   // Or none at all
   ASSERT_TRUE(bot2->runString("bf:sendData()"));
   EXPECT_EQ(0, levelgen->getLuaGlobalVar<S32>("aaa"));   // Missing args default to nil
   EXPECT_EQ(0, levelgen->getLuaGlobalVar<S32>("bbb"));
   EXPECT_FLOAT_EQ(0, levelgen->getLuaGlobalVar<F32>("ccc"));
   EXPECT_EQ(5, levelgen->getLuaGlobalVar<S32>("ct"));    // Levelgen's incrementing counter goes up because bot made the call

   EXPECT_EQ(0, bot->getLuaGlobalVar<S32>("p1"));         // Missing args default to nil
   EXPECT_EQ(0, bot->getLuaGlobalVar<S32>("p2"));
   EXPECT_FLOAT_EQ(0, bot->getLuaGlobalVar<F32>("p3"));

   ///// B0rking!!!
   // Test what happens if our event handler is removed after it has subscribed.  Doing this mostly out of curiosity, as
   // it's a total edge case that will never happen.  Mess with bot2 because after deleting and re-adding bot above, bot2
   // is earlier in the event firing order, so more likely to be a problem for later scripts like bot.
   ASSERT_TRUE(bot2->runString("onDataReceived = nil"));  // Please don't do this in a real script!!

   ASSERT_TRUE(levelgen->runString("bf:sendData('a', 'b', 'c')"));

   for(U32 i = 0; i < 10; i++) serverGame->idle(10);      // Marinate to give dead bot time to delete itself

   // We want bot to get the data even if bot2 crashed handling the event
   EXPECT_EQ("a", bot->getLuaGlobalVar<string>("p1"));
   EXPECT_EQ("b", bot->getLuaGlobalVar<string>("p2"));
   EXPECT_EQ("c", bot->getLuaGlobalVar<string>("p3"));

   delete bot;
   //delete bot2; --> Already deleted when crashed?
}


TEST_F(LuaEnvironmentTest, immutability)
{
   EXPECT_FALSE(levelgen->runString("string.sub = nil"));
}


TEST_F(LuaEnvironmentTest, findAllObjects)
{
   // Reset lua environment so we have a clean slate
   reset();

   // Make sure level hasn't been tainted by previous tests
   ASSERT_TRUE(levelgen->runString("t = bf:findAllObjects()"));
   ASSERT_TRUE(levelgen->runString("assert(#t == 1)"));        // There will be a ship here, not sure why, but there it is 
                                                               // (maybe because GamePair adds a client?)

   EXPECT_TRUE(levelgen->runString("bf:addItem(ResourceItem.new(point.new(0,0)))"));
   EXPECT_TRUE(levelgen->runString("bf:addItem(ResourceItem.new(point.new(300,300)))"));
   EXPECT_TRUE(levelgen->runString("bf:addItem(TestItem.new(point.new(200,200)))"));

   EXPECT_TRUE(levelgen->runString("t = bf:findAllObjects()"));
   EXPECT_TRUE(levelgen->runString("assert(#t == 3 + 1)"));    // + 1 for the ship

   EXPECT_TRUE(levelgen->runString("t = bf:findAllObjects(ObjType.ResourceItem)"));
   EXPECT_TRUE(levelgen->runString("assert(#t == 2)"));
}


};
