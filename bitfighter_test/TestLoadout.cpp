//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelFilesForTesting.h"
#include "TestUtils.h"
#include "ClientGame.h"
#include "ServerGame.h"

#include "gtest/gtest.h"

namespace Zap
{


TEST(LoadoutTest, LoadoutChangeModeTest) 
{
   Vector<S32> loadoutZoneCount({ 0,0,0,0 });
   string level = getLevelWithVariableNumberOfLoadoutZones(loadoutZoneCount);

   GamePair pair(level);
   pair.idle(100, 100);    // Perhaps unneeded... but unharmful

   ClientGame *client = pair.getClient(0);
   ServerGame *server = pair.server;

   EXPECT_TRUE(client->levelHasLoadoutZone());
   EXPECT_TRUE(server->levelHasLoadoutZone());
}

   
};
