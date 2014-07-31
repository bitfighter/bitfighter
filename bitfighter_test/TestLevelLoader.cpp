//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "barrier.h"
#include "gameType.h"
#include "ServerGame.h"
#include "Level.h"
#include "GameManager.h"


#include "TestUtils.h"
#include "LevelFilesForTesting.h"

#include "gtest/gtest.h"

namespace Zap
{

class LevelLoaderTest: public testing::Test
{

};

TEST(LevelLoaderTest, longLine)
{
   U32 TEST_POINTS = 0xFFF;      //0xFFFF takes a wicked long time to run

   Vector<Point> geom;    
   geom.resize(TEST_POINTS);     // Preallocate for speed
   for(U32 i = 0; i < TEST_POINTS; i++)
      geom[i].set(i, i % 2);     // Creates a list of points: 0 0   1 1   2 0   3 1   4 0   5 1   6 0   7 1   8 0   9 1...

   WallItem wall;
   wall.GeomObject::setGeom(geom);

   string code = getGenericHeader() + wall.toLevelCode();

   // Create a level, with our giant wall object; test to ensure the wall is created and has the right number of vertices
   Level level(code);
   
   const Vector<WallItem *> walls = level.getWallList();
   ASSERT_EQ(1, walls.size());
   EXPECT_EQ(TEST_POINTS, walls[0]->getVertCount());
}


// Test to make sure our engineered items are mounting on walls properly when the level loads
TEST(LevelLoaderTest, EngineeredItemMounting)
{
   //string code = getLevelCodeForEngineeredItemSnapping();

   GamePair gamePair(getLevelCodeForEngineeredItemSnapping(), 0);    // No clients, only create a server here
   ServerGame *serverGame = GameManager::getServerGame();

   Vector<DatabaseObject *> fillItems;
   serverGame->getGameObjDatabase()->findObjects(TurretTypeNumber, fillItems);
   ASSERT_EQ(1, fillItems.size());
   EXPECT_EQ(fillItems[0]->getPos().toString(), Point(30, 10).toString()) << "Turret did not mount!";
}

};

