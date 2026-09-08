//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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


// Helper to build a mini level string with a FogOfWar line
static string makeFogOfWarLevel(const string &fogOfWarLine)
{
   return
      "LevelFormat 2\n"
      "GameType 10 8\n"
      "LevelName Test\n"
      "LevelDescription \"\"\n"
      "LevelCredits \"\"\n"
      "Team Blue 0 0 1\n"
      "Specials\n"
      "MinPlayers\n"
      "MaxPlayers\n"
      + fogOfWarLine + "\n"
      "Spawn 0 0 0\n";
}


// Helper: loads a level string and runs a lambda with the resulting GameType.
// The ServerGame stays alive for the duration of the lambda call.
template<typename Func>
static void withLoadedLevel(const string &levelCode, Func fn)
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(""));
   ServerGame serverGame(addr, settings, levelSource, false, false);
   serverGame.unsuspendGame(false);
   serverGame.loadLevelFromString(levelCode, serverGame.getGameObjDatabase());
   fn(serverGame.getGameType());
}


TEST_F(LevelLoaderTest, fogOfWarParsing_Yes)
{
   withLoadedLevel(makeFogOfWarLevel("FogOfWar Yes"), [](GameType *gt) {
      ASSERT_NE(nullptr, gt);
      EXPECT_EQ(FogOfWar::YES, gt->getFogOfWarMode());
      EXPECT_TRUE(gt->isFogOfWarEnabled());
   });
}


TEST_F(LevelLoaderTest, fogOfWarParsing_No)
{
   withLoadedLevel(makeFogOfWarLevel("FogOfWar No"), [](GameType *gt) {
      ASSERT_NE(nullptr, gt);
      EXPECT_EQ(FogOfWar::NO, gt->getFogOfWarMode());
      EXPECT_FALSE(gt->isFogOfWarEnabled());
   });
}


TEST_F(LevelLoaderTest, fogOfWarParsing_Default)
{
   withLoadedLevel(makeFogOfWarLevel("FogOfWar Default"), [](GameType *gt) {
      ASSERT_NE(nullptr, gt);
      EXPECT_EQ(FogOfWar::DEFAULT, gt->getFogOfWarMode());
   });
}


TEST_F(LevelLoaderTest, fogOfWarParsing_DefaultImplied)
{
   string level =
      "LevelFormat 2\n"
      "GameType 10 8\n"
      "LevelName Test\n"
      "LevelDescription \"\"\n"
      "LevelCredits \"\"\n"
      "Team Blue 0 0 1\n"
      "Specials\n"
      "MinPlayers\n"
      "MaxPlayers\n"
      "Spawn 0 0 0\n";

   withLoadedLevel(level, [](GameType *gt) {
      ASSERT_NE(nullptr, gt);
      EXPECT_EQ(FogOfWar::DEFAULT, gt->getFogOfWarMode());
   });
}


TEST_F(LevelLoaderTest, fogOfWarParsing_CaseInsensitive)
{
   withLoadedLevel(makeFogOfWarLevel("FogOfWar yes"), [](GameType *gt) {
      ASSERT_NE(nullptr, gt);
      EXPECT_EQ(FogOfWar::YES, gt->getFogOfWarMode());
   });

   withLoadedLevel(makeFogOfWarLevel("FogOfWar YES"), [](GameType *gt) {
      ASSERT_NE(nullptr, gt);
      EXPECT_EQ(FogOfWar::YES, gt->getFogOfWarMode());
   });

   withLoadedLevel(makeFogOfWarLevel("FogOfWar nO"), [](GameType *gt) {
      ASSERT_NE(nullptr, gt);
      EXPECT_EQ(FogOfWar::NO, gt->getFogOfWarMode());
   });
}


// Helper: round-trip a level string through toLevelCode and re-parse
static void roundTripFogOfWar(const string &levelCode, FogOfWar expected, const string &expectedLine)
{
   string serialized;
   {
      Address addr;
      GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
      LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(""));
      ServerGame outerGame(addr, settings, levelSource, false, false);
      outerGame.unsuspendGame(false);
      outerGame.loadLevelFromString(levelCode, outerGame.getGameObjDatabase());

      GameType *gt = outerGame.getGameType();
      ASSERT_NE(nullptr, gt);
      EXPECT_EQ(expected, gt->getFogOfWarMode());

      serialized = gt->getGame()->toLevelCode();
      EXPECT_NE(string::npos, serialized.find(expectedLine));
   }  // outerGame destroyed here

   // Now re-parse in a fresh ServerGame
   Address addr2;
   GameSettingsPtr settings2 = GameSettingsPtr(new GameSettings());
   LevelSourcePtr levelSource2 = LevelSourcePtr(new StringLevelSource(""));
   ServerGame innerGame(addr2, settings2, levelSource2, false, false);
   innerGame.unsuspendGame(false);
   innerGame.loadLevelFromString(serialized + "Spawn 0 0 0\n", innerGame.getGameObjDatabase());

   GameType *gt2 = innerGame.getGameType();
   ASSERT_NE(nullptr, gt2);
   EXPECT_EQ(expected, gt2->getFogOfWarMode());
}


TEST_F(LevelLoaderTest, fogOfWarRoundTrip)
{
   roundTripFogOfWar(makeFogOfWarLevel("FogOfWar Yes"), FogOfWar::YES, "FogOfWar Yes");
}


TEST_F(LevelLoaderTest, fogOfWarRoundTrip_Default)
{
   roundTripFogOfWar(makeFogOfWarLevel("FogOfWar Default"), FogOfWar::DEFAULT, "FogOfWar Default");
}


TEST_F(LevelLoaderTest, fogOfWarRoundTrip_No)
{
   roundTripFogOfWar(makeFogOfWarLevel("FogOfWar No"), FogOfWar::NO, "FogOfWar No");
}

};

