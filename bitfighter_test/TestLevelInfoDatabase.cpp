//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelInfoDatabase.h"
#include "LevelSource.h"

//#include "TestUtils.h"
#include <sstream>

#include "gtest/gtest.h"


namespace LevelInfoDatabase
{

// Our real database will never be this messy!
static const string csvContents =
   "d58e3582afa99040e27b92b13c8f2280, \"Level, One\", NexusGameType,1,2,1000, \n"
   "6b34fe24ac2ff8103f6fce1f0da2ef57, \"\"Level Two\"\", CTFGameType,3,999,4567,script.lua\n"
   "3e12ec4d994fefe424c88687d738a874, Level Three  ,  ZoneControlGameType, 5, 33 ,   888,  \"script2\"    \n"
   "062c148454b0db6e5a29547c0220a83e, Bad Game Type  ,  NoneGameType , 8, 10, 123, ""\n";


TEST(LevelInfoDatabaseTest, reading)
{
   LevelInfoDb::LevelInfoDatabase db;

   LevelInfoDb::readCsvFromStream(stringstream(csvContents), db);

   EXPECT_EQ("Level, One",    db.levelName ["d58e3582afa99040e27b92b13c8f2280"]);
   EXPECT_EQ(NexusGame,       db.gameTypeId["d58e3582afa99040e27b92b13c8f2280"]);
   EXPECT_EQ(1,               db.minPlayers["d58e3582afa99040e27b92b13c8f2280"]);
   EXPECT_EQ(2,               db.maxPlayers["d58e3582afa99040e27b92b13c8f2280"]);
   EXPECT_EQ(1000,            db.levelSize ["d58e3582afa99040e27b92b13c8f2280"]);
   EXPECT_EQ("",              db.scriptName["d58e3582afa99040e27b92b13c8f2280"]);

   EXPECT_EQ("\"Level Two\"", db.levelName ["6b34fe24ac2ff8103f6fce1f0da2ef57"]);
   EXPECT_EQ(CTFGame,         db.gameTypeId["6b34fe24ac2ff8103f6fce1f0da2ef57"]);
   EXPECT_EQ(3,               db.minPlayers["6b34fe24ac2ff8103f6fce1f0da2ef57"]);
   EXPECT_EQ(999,             db.maxPlayers["6b34fe24ac2ff8103f6fce1f0da2ef57"]);
   EXPECT_EQ(4567,            db.levelSize ["6b34fe24ac2ff8103f6fce1f0da2ef57"]);
   EXPECT_EQ("script.lua",    db.scriptName["6b34fe24ac2ff8103f6fce1f0da2ef57"]);

   EXPECT_EQ("Level Three",   db.levelName ["3e12ec4d994fefe424c88687d738a874"]);
   EXPECT_EQ(ZoneControlGame, db.gameTypeId["3e12ec4d994fefe424c88687d738a874"]);
   EXPECT_EQ(5,               db.minPlayers["3e12ec4d994fefe424c88687d738a874"]);
   EXPECT_EQ(33,              db.maxPlayers["3e12ec4d994fefe424c88687d738a874"]);
   EXPECT_EQ(888,             db.levelSize ["3e12ec4d994fefe424c88687d738a874"]);
   EXPECT_EQ("script2",       db.scriptName["3e12ec4d994fefe424c88687d738a874"]);

   // The following entry shouldn't exist, but checking it in this manner will make it exist... and so will return ""
   // We normally won't need to worry about this.
   EXPECT_EQ("",              db.levelName ["062c148454b0db6e5a29547c0220a83e"]);    
   EXPECT_EQ(0,               db.gameTypeId["062c148454b0db6e5a29547c0220a83e"]);    
}   


TEST(LevelInfoDatabaseTest, writing)
{
   {
      LevelInfo levelInfo("New level", NexusGame, 6, 9, "");

      stringstream out;
      levelInfo.writeToStream(out, "3e12ec4d994fefe424c88687d738a874");

      EXPECT_EQ("3e12ec4d994fefe424c88687d738a874,\"New level\",Nexus,6,9,\n", out.str());
   }
   {
      LevelInfo levelInfo("Quote \" in name", RabbitGame, 0, 99, "script.lua");

      stringstream out;
      levelInfo.writeToStream(out, "b18ea44550b68d0d012bd9017c4a864a");

      EXPECT_EQ("b18ea44550b68d0d012bd9017c4a864a,\"Quote \" in name\",Rabbit,0,99,script.lua\n", out.str());
   }

}

};
