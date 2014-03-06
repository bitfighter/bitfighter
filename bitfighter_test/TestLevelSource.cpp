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


   // These tests aren't really intensive... just basic functionality.  Could be expanded to include more 
   // challenging levels

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
}


};

