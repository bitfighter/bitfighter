//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelFilesForTesting.h"
#include "TestUtils.h"
#include "ClientGame.h"
#include "ServerGame.h"

#include "UIMenuItems.h"

#include "gtest/gtest.h"

namespace Zap
{


void doTest(const Vector<S32> &loadoutZoneCount, S32 neutralLoadoutZoneCount, S32 hostileLoadoutZoneCount, const Vector<S32> &results)
{
   ASSERT_EQ(loadoutZoneCount.size(), results.size()) << "Malformed test!";      // Sanity check

   string level = getLevelWithVariableNumberOfLoadoutZones(loadoutZoneCount, neutralLoadoutZoneCount, hostileLoadoutZoneCount);

   GamePair pair(level);

   ClientGame *client = pair.getClient(0);
   ServerGame *server = pair.server;

   for(S32 i = 0; i < results.size(); i++)
      if(results[i])
      {
         EXPECT_TRUE(client->levelHasLoadoutZoneForTeam(i));
         EXPECT_TRUE(server->levelHasLoadoutZoneForTeam(i));
      }
      else
      {
         EXPECT_FALSE(client->levelHasLoadoutZoneForTeam(i));
         EXPECT_FALSE(server->levelHasLoadoutZoneForTeam(i));
      }
}


// Verify that our LoadoutZone team tracking is working
TEST(LoadoutTest, TestLevelHasLoadoutZoneForTeam) 
{
   // Base case
   {
      Vector<S32> teamLoadoutZoneCount({ 3,2,0 });
      S32 neutralLoadoutZoneCount(0);
      S32 hostileLoadoutZoneCount(0);
      Vector<S32> results({ true, true, false });

      doTest(teamLoadoutZoneCount, neutralLoadoutZoneCount, hostileLoadoutZoneCount, results);
   }

   // Make sure neutral zones work
   {
      Vector<S32> teamLoadoutZoneCount({ 0,1,0 });
      S32 neutralLoadoutZoneCount(1);
      S32 hostileLoadoutZoneCount(0);
      Vector<S32> results({ true, true, true });     

      doTest(teamLoadoutZoneCount, neutralLoadoutZoneCount, hostileLoadoutZoneCount, results);
   }

   // And hostile ones don't
   {
      Vector<S32> teamLoadoutZoneCount({ 0,9,0,1 });
      S32 neutralLoadoutZoneCount(0);
      S32 hostileLoadoutZoneCount(2);
      Vector<S32> results({ false, true, false, true });

      doTest(teamLoadoutZoneCount, neutralLoadoutZoneCount, hostileLoadoutZoneCount, results);
   }

   // And neutral + hostile work as expected
   {
      Vector<S32> teamLoadoutZoneCount({ 3,2,0,0,0,1 });
      S32 neutralLoadoutZoneCount(1);
      S32 hostileLoadoutZoneCount(1);
      Vector<S32> results({ true, true, true, true, true, true });
         
      doTest(teamLoadoutZoneCount, neutralLoadoutZoneCount, hostileLoadoutZoneCount, results);
   }
}


//TODO: Move this to a new test file!!!  Doesn't belong here!!
TEST(LoadoutTest, TestTenthsCounter) 
{
   F32 val = 4.3f;
   FloatCounterMenuItem menuItem("Test", val, 0.1, 2.1, 40.5, 1, "what", "ever", "man");

   EXPECT_EQ("4.3", menuItem.getValue());    // 4.3
   EXPECT_FLOAT_EQ(val, stof(menuItem.getValue()));
}
   
}
