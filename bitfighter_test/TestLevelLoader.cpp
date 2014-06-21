//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "barrier.h"
#include "gameLoader.h"
#include "gameType.h"
#include "ServerGame.h"
#include "Level.h"

#include "LevelFilesForTesting.h"

#include "gtest/gtest.h"

namespace Zap
{

class LevelLoaderTest: public testing::Test
{

};

TEST_F(LevelLoaderTest, longLine)
{
   U32 TEST_POINTS = 0xFFF;      //0xFFFF takes a wicked long time to run

   Vector<Point> geom;    
   geom.resize(TEST_POINTS);     // Preallocate for speed
   for(U32 i = 0; i < TEST_POINTS; i++)
      geom[i].set(i, i % 2);     // Creates a list of points: 0 0   1 1   2 0   3 1   4 0   5 1   6 0   7 1   8 0   9 1...

   WallItem wall;
   wall.GeomObject::setGeom(geom);

   string code = getGenericHeader() + wall.toLevelCode();

   Level level;
   EXPECT_EQ(0, level.findObjects_fast()->size());
   level.loadLevelFromString(code);

   
   const Vector<DatabaseObject*> *objects = level.findObjects_fast();
   EXPECT_EQ(TEST_POINTS - 1, objects->size());
}

};

