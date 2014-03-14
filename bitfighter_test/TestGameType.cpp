//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gameType.h"
#include "gtest/gtest.h"

namespace Zap
{

TEST(GameTypeTests, Lookups)
{
   // The easy cases
   EXPECT_EQ(BitmatchGame,    GameType::getGameTypeIdFromName("GameType"));
   EXPECT_EQ(NexusGame,       GameType::getGameTypeIdFromName("NexusGameType"));
   EXPECT_EQ(RabbitGame,      GameType::getGameTypeIdFromName("RabbitGameType"));
   EXPECT_EQ(CTFGame,         GameType::getGameTypeIdFromName("CTFGameType"));
   EXPECT_EQ(CoreGame,        GameType::getGameTypeIdFromName("CoreGameType"));
   EXPECT_EQ(HTFGame,         GameType::getGameTypeIdFromName("HTFGameType"));
   EXPECT_EQ(RetrieveGame,    GameType::getGameTypeIdFromName("RetrieveGameType"));
   EXPECT_EQ(SoccerGame,      GameType::getGameTypeIdFromName("SoccerGameType"));
   EXPECT_EQ(ZoneControlGame, GameType::getGameTypeIdFromName("ZoneControlGameType"));

   // Bad input
   EXPECT_EQ(NoGameType, GameType::getGameTypeIdFromName("FunkyGameType"));
}

};
