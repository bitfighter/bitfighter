//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Bitfighter Tests

#define BF_TEST

#include "gtest/gtest.h"

#include "gameType.h"
#include "ServerGame.h"
#include "ClientGame.h"
#include "Level.h"
#include "LevelFilesForTesting.h"

#include "Colors.h"
#include "GeomUtils.h"

#include "TestUtils.h"


namespace Zap {
using namespace std;


typedef S32 Results[3];

struct TestInfo {
   string itemToTest;
   S32 objectTypeNumber;
   Results startingCondition;
   Results itemMovedToRed;
   Results itemMovedToBlue;
   Results playersMovedToRed;
   Results playersMovedToBlue;
   Results itemMovedToNeutral;
   Results itemsMovedToHostile;
};


TestInfo itemsToTestArr[] =
{     //                                                                      start      item to red |item to blue|plyrs on red|plyrs on blue |neut. items |host. items 
   {"RepairItem 0 76.5 20",                             RepairItemTypeNumber, {1, 1, 1},   {1, 1, 1},   {1, 1, 1},   {1, 1, 1},   {1, 1, 1},   {1, 1, 1},   {1, 1, 1}},
   {"TextItem 0 -127.5 0 127.5 0 57.845 \"Blue text\"", TextItemTypeNumber,   {1, 1, 0},   {1, 0, 1},   {1, 1, 0},   {1, 1, 1},   {1, 0, 0},   {1, 1, 1},   {1, 0, 0}},
   {"LineItem 0 2 Global -127.5 229.5 0 153",           LineTypeNumber,       {1, 1, 1},   {1, 1, 1},   {1, 1, 1},   {1, 1, 1},   {1, 1, 1},   {1, 1, 1},   {1, 1, 1}},   // Global -- visible on every team
   {"LineItem 0 2 -127.5 229.5 0 153 127.5 204",        LineTypeNumber,       {1, 1, 0},   {1, 0, 1},   {1, 1, 0},   {1, 1, 1},   {1, 0, 0},   {1, 1, 1},   {1, 0, 0}},   // Not global -- visible to own team only
   {"Zone 178.5 51 178.5 127.5 408 127.5 408 51",       ZoneTypeNumber,       {1, 0, 0},   {1, 0, 0},   {1, 0, 0},   {1, 0, 0},   {1, 0, 0},   {1, 0, 0},   {1, 0, 0}},
   //{"Mine 0 5 5",                                       MineTypeNumber,       {1, 1, 0},   {1, 0, 1},   {1, 1, 0},   {1, 1, 1},   {1, 0, 0},   {1, 1, 1},   {1, 0, 0}},
};


void testObjectTransmission(S32 objTypeNumber, ServerGame *serverGame, S32 severCount,
                                               ClientGame *blue,       S32 blueCount,
                                               ClientGame *red,        S32 redCount)
{
   Vector<DatabaseObject *> fillVector;

   // Repair should be everywhere
   serverGame->getLevel()->findObjects(objTypeNumber, fillVector);
   ASSERT_EQ(severCount, fillVector.size()) << "Server";
   fillVector.clear();
   blue->getLevel()->findObjects(objTypeNumber, fillVector);
   EXPECT_EQ(blueCount, fillVector.size()) << "Blue client";
   fillVector.clear();
   red->getLevel()->findObjects(objTypeNumber, fillVector);
   EXPECT_EQ(redCount, fillVector.size()) << "Red client";
}


void testScope(ServerGame *serverGame, ClientGame *blue, ClientGame *red, const TestInfo &testInfo, const Results &results)
{
   testObjectTransmission(testInfo.objectTypeNumber, 
                          serverGame, results[0], 
                          blue,       results[1], 
                          red,        results[2]);
}


Vector<TestInfo> testInfos = Vector<TestInfo>(itemsToTestArr, ARRAYSIZE(itemsToTestArr));



TEST(ObjectScopeTest, TestItemPropagation)
{
   for(S32 i = 0; i < testInfos.size(); i++)
   {
      // Create a GamePair using our text item code, with 2 clients; one will be red, the other blue.
      // The test will confirm which players get the red text item, the blue line, and the zone object.
      string levelCode = getLevelCodeForItemPropagationTests(testInfos[i].itemToTest);

      GamePair gamePair(levelCode, 2);

      // First, ensure we have two players, one red, one blue
      ServerGame *serverGame = gamePair.server;

      ASSERT_EQ(2, serverGame->getPlayerCount());
      ASSERT_EQ(Colors::blue.toHexString(), serverGame->getTeamColor(0).toHexString());
      ASSERT_EQ(Colors::red.toHexString(), serverGame->getTeamColor(1).toHexString());

      // Do the following rigamarole to break dependency on assumption client0 is blue and client1 is red
      ClientGame *client0 = gamePair.getClient(0);
      ClientGame *client1 = gamePair.getClient(1);

      ClientGame *blue = (serverGame->getTeamColor(client0->getLocalRemoteClientInfo()->getTeamIndex()).toHexString() ==
         Colors::blue.toHexString()) ? client0 : client1;

      ClientGame *red = (serverGame->getTeamColor(client0->getLocalRemoteClientInfo()->getTeamIndex()).toHexString() ==
         Colors::red.toHexString()) ? client0 : client1;

      ASSERT_EQ(Colors::blue.toHexString(), serverGame->getTeamColor(blue->getLocalRemoteClientInfo()->getTeamIndex()).toHexString());
      ASSERT_EQ(Colors::red.toHexString(), serverGame->getTeamColor(red->getLocalRemoteClientInfo()->getTeamIndex()).toHexString());

      ASSERT_FALSE(serverGame->getClientInfos()->get(0)->getConnection()->mInCommanderMap);
      ASSERT_FALSE(serverGame->getClientInfos()->get(1)->getConnection()->mInCommanderMap);

      // Now that we know which client is which, we can check to see which objects are available where
      {
         SCOPED_TRACE("Not in CommandersMap // " + testInfos[i].itemToTest);
         testScope(serverGame, blue, red, testInfos[i], testInfos[i].startingCondition);
      }

      // Turn on cmdrs map... should not affect results
      red->setUsingCommandersMap(true);
      blue->setUsingCommandersMap(true);

      gamePair.idle(10, 5);     // Let things settle

      ASSERT_TRUE(serverGame->getClientInfos()->get(0)->getConnection()->mInCommanderMap);
      ASSERT_TRUE(serverGame->getClientInfos()->get(1)->getConnection()->mInCommanderMap);
      {
         SCOPED_TRACE("In CommandersMap // " + testInfos[i].itemToTest);
         testScope(serverGame, blue, red, testInfos[i], testInfos[i].startingCondition);
      }

      Vector<DatabaseObject *> fillVector;

      // Change the textitem from red to blue
      serverGame->getLevel()->findObjects(testInfos[i].objectTypeNumber, fillVector);
      ASSERT_EQ(1, fillVector.size());
      BfObject *obj = static_cast<BfObject *>(fillVector[0]);
      obj->setTeam(0);     // Blue team

      gamePair.idle(10, 5);     // Let things settle
      {
         SCOPED_TRACE("Item Moved To Blue // " + testInfos[i].itemToTest);
         testScope(serverGame, blue, red, testInfos[i], testInfos[i].itemMovedToBlue);
      }
      obj->setTeam(1);          // Back to red

      gamePair.idle(10, 5);     // Let things settle
      {
         SCOPED_TRACE("Item Moved To Red // " + testInfos[i].itemToTest);
         testScope(serverGame, blue, red, testInfos[i], testInfos[i].itemMovedToRed);
      }

      // Now change the player's team from blue to red
      blue->changeOwnTeam(1);
      EXPECT_EQ(0, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 0 (change hasn't propagated yet)";
      gamePair.idle(10, 5);     // Let things settle
      EXPECT_EQ(1, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 1 (change should have propagated)";

      EXPECT_EQ(1, serverGame->getClientInfos()->get(0)->getTeamIndex());
      EXPECT_EQ(1, serverGame->getClientInfos()->get(1)->getTeamIndex());
      {
         SCOPED_TRACE("Blue Player Moved To Red, Items On Red // " + testInfos[i].itemToTest);
         testScope(serverGame, blue, red, testInfos[i], testInfos[i].playersMovedToRed);
      }

      // And put both players on blue
      blue->changeOwnTeam(0);
      red->changeOwnTeam(0);

      EXPECT_EQ(1, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 1 (change hasn't propagated yet)";
      EXPECT_EQ(1, red->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 1 (change hasn't propagated yet)";
      gamePair.idle(10, 5);     // Let things settle
      EXPECT_EQ(0, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 0 (change should have propagated)";
      EXPECT_EQ(0, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 0 (change should have propagated)";

      EXPECT_EQ(0, serverGame->getClientInfos()->get(0)->getTeamIndex());
      EXPECT_EQ(0, serverGame->getClientInfos()->get(1)->getTeamIndex());
      {
         SCOPED_TRACE("Both Players Moved To Blue, Items On Red // " + testInfos[i].itemToTest);
         testScope(serverGame, blue, red, testInfos[i], testInfos[i].playersMovedToBlue);
      }

      // Make items neutral and see if they propagate properly
      fillVector.clear();
      serverGame->getLevel()->findObjects(testInfos[i].objectTypeNumber, fillVector);
      ASSERT_EQ(1, fillVector.size());
      for(S32 j = 0; j < fillVector.size(); j++)
         static_cast<BfObject *>(fillVector[j])->setTeam(TEAM_NEUTRAL);
      gamePair.idle(10, 5);     // Let things settle
      {
         SCOPED_TRACE("Items On Neutral // " + testInfos[i].itemToTest);
         testScope(serverGame, blue, red, testInfos[i], testInfos[i].itemMovedToNeutral);
      }

      for(S32 j = 0; j < fillVector.size(); j++)
         static_cast<BfObject *>(fillVector[j])->setTeam(TEAM_HOSTILE);
      gamePair.idle(10, 5);     // Let things settle
      {
         SCOPED_TRACE("Items On Hostile // " + testInfos[i].itemToTest);
         testScope(serverGame, blue, red, testInfos[i], testInfos[i].itemsMovedToHostile);
      }
   }
}


}; // namespace Zap
