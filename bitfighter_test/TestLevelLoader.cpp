//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

//#include "barrier.h"
#include "gameType.h"
#include "ServerGame.h"
#include "Level.h"
#include "GameManager.h"
#include "WallItem.h"
#include "EngineeredItem.h"


#include "TestUtils.h"
#include "LevelFilesForTesting.h"

#include "gtest/gtest.h"

namespace Zap
{

class LevelLoaderTest : public testing::Test
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

   GamePair gamePair;

   string code = getGenericHeader() + wall.toLevelCode();

   // Create a level, with our giant wall object; test to ensure the wall is created and has the right number of vertices
   Level level(code);
   
   const Vector<DatabaseObject *> *walls = level.findObjects_fast(WallItemTypeNumber);
   ASSERT_EQ(1, walls->size());
   EXPECT_EQ(TEST_POINTS, walls->get(0)->getVertCount());
}


// Test to make sure our engineered items are mounting on walls properly when the level loads
TEST(LevelLoaderTest, EngineeredItemMounting)
{
   GamePair gamePair(getLevelCodeForEngineeredItemSnapping(), 0);    // No clients, only server
   ServerGame *serverGame = GameManager::getServerGame();

   Vector<DatabaseObject *> fillItems;
   serverGame->getLevel()->findObjects(TurretTypeNumber, fillItems);
   ASSERT_EQ(1, fillItems.size());
   EXPECT_FLOAT_EQ(fillItems[0]->getPos().x, 30) << "Turret did not mount! (x-coord)";
   EXPECT_FLOAT_EQ(fillItems[0]->getPos().y, 10) << "Turret did not mount! (y-coord)";
}


// Check for turret snapping to polywalls wound different ways
TEST(LevelLoaderTest, EngineeredItemMounting2)
{
   GamePair gamePair(getLevelCodeForEngineeredItemSnapping2(), 0);    // No clients, only server
   ServerGame *serverGame = GameManager::getServerGame();

   Vector<DatabaseObject *> fillItems;
   serverGame->getLevel()->findObjects(TurretTypeNumber, fillItems);
   ASSERT_EQ(2, fillItems.size());

   for(S32 i = 0; i < fillItems.size(); i++)
   {
      Turret *turret = static_cast<Turret *>(fillItems[i]);

      Point anchor = turret->getPos();
      Point normal = turret->mAnchorNormal;
      S32 id = turret->getUserAssignedId();

      if(id == 1)
      {
         // "PolyWall -255 -76.5 -255 0 -25.5 0 -25.5 -76.5\n"    // Wall wound in default order
         // "Turret 0 -128 0 0\n"
         EXPECT_FLOAT_EQ(-128, anchor.x)  << "Turret 1 did not mount! (x-coord)";
         EXPECT_FLOAT_EQ(   0, anchor.y)  << "Turret 1 did not mount! (y-coord)";
         EXPECT_FLOAT_EQ(   0, normal.x)  << "Turret 1 bad normal (x)";
         EXPECT_FLOAT_EQ(   1, normal.y)  << "Turret 1 bad normal (y)";
      }
      else if(id == 2)
      {
         // "PolyWall -25.5 -176.5 -25.5 -100 -255 -100 -255 -176.5\n" // Wall wound in reverse order
         // "Turret!2 0 -128 -100.1 0\n"
         EXPECT_FLOAT_EQ(-128, anchor.x)  << "Turret 2 did not mount! (x-coord)";
         EXPECT_FLOAT_EQ(-100, anchor.y)  << "Turret 2 did not mount! (y-coord)";
         EXPECT_FLOAT_EQ(   0, normal.x)  << "Turret 2 bad normal (x)";
         EXPECT_FLOAT_EQ(   1, normal.y)  << "Turret 2 bad normal (y)";
      }
      else
         TNLAssert(false, "Bad id!");
   }
};
   
}     // namespace

