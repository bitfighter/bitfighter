//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"

#include "ServerGame.h"
#include "GameTypesEnum.h"
#include "gameConnection.h"
#include "Colors.h"
#include "gtest/gtest.h"

namespace Zap
{

TEST(GameTest, AspectRatio)
{
   // Aspect ratio of visible area should be the same in normal and sensor modes
   EXPECT_FLOAT_EQ(Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL / Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL, 
                   Game::PLAYER_VISUAL_DISTANCE_VERTICAL   / Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_VERTICAL);
}


TEST(GameTest, IndividualGameWinners)
{
   GamePair gamePair;
   ServerGame *game = gamePair.server;

   ClientInfo *player1 = game->getClientInfo(0);         // gamePair comes with one client by default
   ASSERT_EQ(0, player1->getScore());                    // Check that starting score is 0, just in case

   EXPECT_EQ(OnlyOnePlayerOrTeam, game->getIndividualGameWinner().first);    

   gamePair.addClient("Player 2");
   ClientInfo *player2 = game->findClientInfo("Player 2");
   ASSERT_EQ(0, player2->getScore());                    // Check that starting score is 0, just in case

   EXPECT_EQ(Tied, game->getIndividualGameWinner().first);        // Scores: 0,0

   player1->setScore(5);
   EXPECT_EQ(HasWinner, game->getIndividualGameWinner().first);   // Scores 5,0
   EXPECT_EQ(player1,   game->getIndividualGameWinner().second); 

   player2->setScore(6);
   EXPECT_EQ(HasWinner, game->getIndividualGameWinner().first);   // Scores 5,6
   EXPECT_EQ(player2,   game->getIndividualGameWinner().second);

   player1->setScore(6);
   EXPECT_EQ(Tied, game->getIndividualGameWinner().first);        // Scores: 6,6

   gamePair.addClient("Player 3");
   ClientInfo *player3 = game->findClientInfo("Player 3");

   gamePair.addClient("Player 4");
   ClientInfo *player4 = game->findClientInfo("Player 4");

   player1->setScore(4);
   player2->setScore(5);
   player3->setScore(5);
   player4->setScore(4);
   EXPECT_EQ(Tied, game->getIndividualGameWinner().first);        // Scores: 4,5,5,4 <-- scenario of concern in comments

   player3->setScore(6);
   EXPECT_EQ(HasWinner, game->getIndividualGameWinner().first);   // Scores: 4,5,6,4
   EXPECT_EQ(player3,   game->getIndividualGameWinner().second);

   player1->setScore(4);
   player2->setScore(4);
   player3->setScore(4);
   player4->setScore(4);
   EXPECT_EQ(Tied, game->getIndividualGameWinner().first);        // Scores: 4,4,4,4

   player1->setScore(6);
   EXPECT_EQ(HasWinner, game->getIndividualGameWinner().first);   // Scores: 6,4,4,4
   EXPECT_EQ(player1,   game->getIndividualGameWinner().second);    

   player1->setScore(4);
   player4->setScore(6);
   EXPECT_EQ(HasWinner, game->getIndividualGameWinner().first);   // Scores: 4,4,4,6
   EXPECT_EQ(player4,   game->getIndividualGameWinner().second);    
}


TEST(GameTest, TeamGameWinners)
{
   GamePair gamePair;
   ServerGame *game = gamePair.server;

   ASSERT_EQ(1, game->getTeamCount()) << "Expect game to start off with one team!";
   EXPECT_EQ(OnlyOnePlayerOrTeam, game->getTeamBasedGameWinner().first);  
   ASSERT_EQ(0, game->getTeam(0)->getScore());
   ASSERT_EQ(1, game->getPlayerCount(0));

   // Add a second team -- game will handle cleanup
   game->addTeam(new Team("Team 2", Colors::green));
   ASSERT_EQ(2, game->getTeamCount()) << "Expect game to start off with one team!";
   ASSERT_EQ(0, game->getTeam(1)->getScore());

   // This following situation is actually undefined... there are no players yet, so how can we have a winner??
   //EXPECT_EQ(Tied, game->getTeamBasedGameWinner().first);  // Scores: 0,0



}


};
