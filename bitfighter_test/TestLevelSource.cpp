//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
   
#include "LevelSource.h"
#include "LevelFilesForTesting.h"

#include "tnlNetInterface.h"
#include "gtest/gtest.h"

namespace Zap
{


TEST(TestLevelSource, tests)
{
   Address addr;
   NetInterface net(addr);   // We never use this, but it will initialize TNL to get past an assert

   LevelInfo levelInfo;    // Empty levelInfo
   
   LevelSource::getLevelInfoFromCodeChunk(getLevelCode1(), levelInfo);

   EXPECT_STREQ("Test Level", levelInfo.mLevelName.getString());
   EXPECT_EQ(BitmatchGame, levelInfo.mLevelType);
   EXPECT_EQ(0, levelInfo.minRecPlayers);
   EXPECT_EQ(0, levelInfo.maxRecPlayers);
   EXPECT_EQ("", levelInfo.mScriptFileName);


   LevelSource::getLevelInfoFromCodeChunk(getLevelCodeForTestingEngineer1(), levelInfo);

   EXPECT_STREQ("Engineer Test Bed One", levelInfo.mLevelName.getString());
   EXPECT_EQ(BitmatchGame, levelInfo.mLevelType);
   EXPECT_EQ(0, levelInfo.minRecPlayers);
   EXPECT_EQ(0, levelInfo.maxRecPlayers);
   EXPECT_EQ("", levelInfo.mScriptFileName);


   S64 t1, t2;
   S32 runs = 10000;
   t1 = Platform::getRealMilliseconds();
   for(S32 i = 0; i < runs; i++)
   {
      LevelSource::getLevelInfoFromCodeChunk(getLevelCode1(), levelInfo);
      LevelSource::getLevelInfoFromCodeChunk(getLevelCodeForTestingEngineer1(), levelInfo);
   }
   t2 = Platform::getRealMilliseconds();
   printf("New: %d\n", t2 - t1);

   t1 = Platform::getRealMilliseconds();
   for(S32 i = 0; i < runs; i++)
   {
      LevelSource::getLevelInfoFromCodeChunk_orig(getLevelCode1(), levelInfo);
      LevelSource::getLevelInfoFromCodeChunk_orig(getLevelCodeForTestingEngineer1(), levelInfo);
   }
   t2 = Platform::getRealMilliseconds();
   printf("Orig: %d\n", t2 - t1);



   t1 = Platform::getRealMilliseconds();
   for(S32 i = 0; i < runs; i++)
   {
      LevelSource::getLevelInfoFromCodeChunk_RegExp(getLevelCode1(), levelInfo);
      LevelSource::getLevelInfoFromCodeChunk_RegExp(getLevelCodeForTestingEngineer1(), levelInfo);
   }
   t2 = Platform::getRealMilliseconds();
   printf("regexp: %d\n", t2 - t1);





}


};

