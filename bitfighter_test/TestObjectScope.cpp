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
   fillVector.clear();
}


void checkObjects(ServerGame *serverGame, ClientGame *blue, ClientGame *red)
{
   {
      SCOPED_TRACE("RepairItem");
      testObjectTransmission(RepairItemTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
   {
      SCOPED_TRACE("Blue Line");
      testObjectTransmission(LineTypeNumber, serverGame, 1, blue, 1, red, 0);
   }
   {
      SCOPED_TRACE("Red Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 0, red, 1);
   }
   {
      SCOPED_TRACE("Zone");
      testObjectTransmission(ZoneTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
}


void checkObjectsWhenBothPlayersAreOnRed(ServerGame *serverGame, ClientGame *blue, ClientGame *red)
{
   SCOPED_TRACE("BothPlayersAreOnRed");
   {
      SCOPED_TRACE("RepairItem");
      testObjectTransmission(RepairItemTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
   {
      SCOPED_TRACE("Blue Line");
      testObjectTransmission(LineTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
   {
      SCOPED_TRACE("Red Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
   {
      SCOPED_TRACE("Zone");
      testObjectTransmission(ZoneTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
}


void checkObjectsWhenBothPlayersAreOnBlue(ServerGame *serverGame, ClientGame *blue, ClientGame *red)
{
   SCOPED_TRACE("BothPlayersAreOnBlue");
   {
      SCOPED_TRACE("Blue Line");
      testObjectTransmission(LineTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
   {
      SCOPED_TRACE("Red Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
}


void checkObjectsWhenNeutral(ServerGame *serverGame, ClientGame *blue, ClientGame *red)
{
   SCOPED_TRACE("Objects are Neutral");
   {
      SCOPED_TRACE("Blue Line");
      testObjectTransmission(LineTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
   {
      SCOPED_TRACE("Red Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
}

void checkObjectsWhenHostile(ServerGame *serverGame, ClientGame *blue, ClientGame *red)
{
   SCOPED_TRACE("Objects are Hostile");
   {
      SCOPED_TRACE("Blue Line");
      testObjectTransmission(LineTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
   {
      SCOPED_TRACE("Red Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
}


TEST(ObjectScopeTest, TestItemPropagation)
{
   // Create a GamePair using our text item code, with 2 clients; one will be red, the other blue.
   // The test will confirm which players get the red text item, the blue line, and the zone object.
   string levelCode = getLevelCodeForItemPropagationTests();

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
      SCOPED_TRACE("Not in CommandersMap");
      checkObjects(serverGame, blue, red);
   }

   // Turn on cmdrs map... should not affect results
   red->setUsingCommandersMap(true);
   blue->setUsingCommandersMap(true);

   gamePair.idle(10, 5);     // Let things settle

   ASSERT_TRUE(serverGame->getClientInfos()->get(0)->getConnection()->mInCommanderMap);
   ASSERT_TRUE(serverGame->getClientInfos()->get(1)->getConnection()->mInCommanderMap);

   {
      SCOPED_TRACE("In CommandersMap");
      checkObjects(serverGame, blue, red);
   }

   Vector<DatabaseObject *> fillVector;

   // Change the textitem from red to blue
   serverGame->getLevel()->findObjects(TextItemTypeNumber, fillVector);
   ASSERT_EQ(1, fillVector.size());
   BfObject *obj = static_cast<BfObject *>(fillVector[0]);
   obj->setTeam(0);     // Blue team

   gamePair.idle(10, 5);     // Let things settle
   {
      SCOPED_TRACE("Blue Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 1, red, 0);
   }
   obj->setTeam(1);          // Back to red

   gamePair.idle(10, 5);     // Let things settle
   {
      SCOPED_TRACE("Red Text (after revert)");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 0, red, 1);
   }

   // Now change the player's team from blue to red
   blue->changeOwnTeam(1);
   EXPECT_EQ(0, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 0 (change hasn't propagated yet)";
   gamePair.idle(10, 5);     // Let things settle
   EXPECT_EQ(1, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 1 (change should have propagated)";

   EXPECT_EQ(1, serverGame->getClientInfos()->get(0)->getTeamIndex());
   EXPECT_EQ(1, serverGame->getClientInfos()->get(1)->getTeamIndex());

   checkObjectsWhenBothPlayersAreOnRed(serverGame, blue, red);


   // And put both players on blue
   blue->changeOwnTeam(0);
   red->changeOwnTeam(0);

   EXPECT_EQ(1, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 1 (change hasn't propagated yet)";
   EXPECT_EQ(1, red->getLocalRemoteClientInfo()->getTeamIndex())  << "Expect this client to be on team 1 (change hasn't propagated yet)";
   gamePair.idle(10, 5);     // Let things settle
   EXPECT_EQ(0, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 0 (change should have propagated)";
   EXPECT_EQ(0, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 0 (change should have propagated)";

   EXPECT_EQ(0, serverGame->getClientInfos()->get(0)->getTeamIndex());
   EXPECT_EQ(0, serverGame->getClientInfos()->get(1)->getTeamIndex());
   checkObjectsWhenBothPlayersAreOnBlue(serverGame, blue, red);

   // Make items neutral and see if they propagate properly
   fillVector.clear();
   serverGame->getLevel()->findObjects(TextItemTypeNumber, fillVector);
   serverGame->getLevel()->findObjects(LineTypeNumber, fillVector);
   ASSERT_EQ(2, fillVector.size());
   for(S32 i = 0; i < fillVector.size(); i++)
      static_cast<BfObject *>(fillVector[i])->setTeam(TEAM_NEUTRAL);
   gamePair.idle(10, 5);     // Let things settle
   {
      SCOPED_TRACE("Neutral!!");
      checkObjectsWhenNeutral(serverGame, blue, red);
   }

   for(S32 i = 0; i < fillVector.size(); i++)
      static_cast<BfObject *>(fillVector[i])->setTeam(TEAM_HOSTILE);
   gamePair.idle(10, 5);     // Let things settle
   {
      SCOPED_TRACE("Hostile!!");
      checkObjectsWhenHostile(serverGame, blue, red);
   }
}


}; // namespace Zap
