#include "barrier.h"
#include "gameLoader.h"
#include "gameType.h"
#include "ServerGame.h"

#include "gtest/gtest.h"

namespace Zap
{

class LevelLoaderTest: public testing::Test
{

};

TEST_F(LevelLoaderTest, longLine)
{
   U32 TEST_POINTS = 0xFFF;            //0xFFFF takes a wicked long time to run

   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(""));

   ServerGame serverGame(addr, settings, levelSource, false, false);
   GridDatabase *db = serverGame.getGameObjDatabase();

   GameType gt;
   gt.addToGame(&serverGame, serverGame.getGameObjDatabase());

   Vector<Point> geom;    
   geom.resize(TEST_POINTS);     // Preallocate for speed
   for(U32 i = 0; i < TEST_POINTS; i++)
      geom[i].set(i, i % 2);

   WallItem wall;
   wall.GeomObject::setGeom(geom);

   serverGame.unsuspendGame(false);

   EXPECT_EQ(0, serverGame.getGameObjDatabase()->findObjects_fast()->size());
   string code = serverGame.toLevelCode() + wall.toLevelCode();
   serverGame.loadLevelFromString(code, db);

   const Vector<DatabaseObject*> *objects = db->findObjects_fast();
   EXPECT_EQ(TEST_POINTS - 1, objects->size());
}

};

