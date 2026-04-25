//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gameType.h"
#include "ServerGame.h"
#include "TestUtils.h"
#include "gtest/gtest.h"

#include <memory>

namespace Zap
{

class TestGameType : public GameType
{
public:
   TestGameType() : GameType() {}
   virtual bool isTeamGame() const { return false; } // Individual game
};

TEST(GameTypeTest, OnGameOverTieDetection)
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(""));

   ServerGame serverGame(addr, settings, levelSource, false, false);
   std::unique_ptr<TestGameType> gt(new TestGameType());
   gt->addToGame(&serverGame, serverGame.getGameObjDatabase());

   // Create 4 players - as robots
   // RefPtr will handle cleanup of these once serverGame is destroyed
   FullClientInfo *p1 = new FullClientInfo(&serverGame, NULL, "Player1", ClientInfo::ClassRobotAddedByAddbots);
   FullClientInfo *p2 = new FullClientInfo(&serverGame, NULL, "Player2", ClientInfo::ClassRobotAddedByAddbots);
   FullClientInfo *p3 = new FullClientInfo(&serverGame, NULL, "Player3", ClientInfo::ClassRobotAddedByAddbots);
   FullClientInfo *p4 = new FullClientInfo(&serverGame, NULL, "Player4", ClientInfo::ClassRobotAddedByAddbots);

   serverGame.addToClientList(p1);
   serverGame.addToClientList(p2);
   serverGame.addToClientList(p3);
   serverGame.addToClientList(p4);

   // Scenario 1: Tie in the middle [4, 5, 5, 4]
   p1->setScore(4);
   p2->setScore(5);
   p3->setScore(5);
   p4->setScore(4);

   gt->onGameOver();
   EXPECT_TRUE(gt->wasTied());

   // Scenario 2: Unique winner [6, 5, 5, 4]
   p1->setScore(6);
   p2->setScore(5);
   p3->setScore(5);
   p4->setScore(4);

   gt->onGameOver();
   EXPECT_FALSE(gt->wasTied());

   // Scenario 3: Unique winner at the end [4, 5, 5, 6]
   p1->setScore(4);
   p2->setScore(5);
   p3->setScore(5);
   p4->setScore(6);

   gt->onGameOver();
   EXPECT_FALSE(gt->wasTied());

   // Scenario 4: Multiple-way tie [5, 5, 5, 5]
   p1->setScore(5);
   p2->setScore(5);
   p3->setScore(5);
   p4->setScore(5);

   gt->onGameOver();
   EXPECT_TRUE(gt->wasTied());
}

};
