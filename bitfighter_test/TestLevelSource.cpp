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

   pair<Vector<string>, Vector<LevelInfo> > levels = getLevels();

   for(S32 i = 0; i < levels.first.size(); i++)
   {
      LevelInfo levelInfo;
      LevelSource::getLevelInfoFromCodeChunk(levels.first[i], levelInfo);

      EXPECT_STREQ("Test Level", levels.second[i].mLevelName.getString());
      EXPECT_EQ(BitmatchGame, levels.second[i].mLevelType);
      EXPECT_EQ(0, levels.second[i].minRecPlayers);
      EXPECT_EQ(0, levels.second[i].maxRecPlayers);
      EXPECT_EQ("", levels.second[i].mScriptFileName);

   }
}


};

