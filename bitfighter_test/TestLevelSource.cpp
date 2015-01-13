//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
   
#include "LevelSource.h"
#include "LevelFilesForTesting.h"

#include "stringUtils.h"

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
      SCOPED_TRACE("i = " + itos(i));
      LevelInfo levelInfo;
      LevelSource::getLevelInfoFromCodeChunk(levels.first[i], levelInfo);

      EXPECT_EQ(levelInfo.mLevelName,      levels.second[i].mLevelName);
      EXPECT_EQ(levelInfo.mLevelType,      levels.second[i].mLevelType);
      EXPECT_EQ(levelInfo.minRecPlayers,   levels.second[i].minRecPlayers);
      EXPECT_EQ(levelInfo.maxRecPlayers,   levels.second[i].maxRecPlayers);
      EXPECT_EQ(levelInfo.mScriptFileName, levels.second[i].mScriptFileName);
   }
}


};

