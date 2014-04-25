//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"

#include "ServerGame.h"
#include "gtest/gtest.h"

namespace Zap
{

TEST(GameTest, AspectRatio)
{
   // Aspect ratio of visible area should be the same in normal and sensor modes
   EXPECT_FLOAT_EQ(Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL / Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL, 
                   Game::PLAYER_VISUAL_DISTANCE_VERTICAL   / Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_VERTICAL);
}


TEST(GameTest, Winners)
{
   GamePair gamePair;
   ServerGame *game = gamePair.server;

   // Starting scores will be 0!
   FullClientInfo player1(game, NULL, "Player 1", ClientInfo::ClassHuman);
   FullClientInfo player2(game, NULL, "Player 2", ClientInfo::ClassHuman);
   FullClientInfo player3(game, NULL, "Player 3", ClientInfo::ClassHuman);
   FullClientInfo player4(game, NULL, "Player 4", ClientInfo::ClassHuman);

   ASSERT_EQ(0, player1.getScore());      // Check for proper starting score, just in case

   game->addClient(&player1);

   // With one player, game will be tied; this is not important behavior, but is documented
   EXPECT_EQ(NULL, game->getIndividualGameWinner());    

   game->addClient(&player2);
   EXPECT_EQ(NULL, game->getIndividualGameWinner());     // Scores: 0,0

   player1.setScore(5);
   EXPECT_EQ(&player1, game->getIndividualGameWinner()); // Scores 5,0

   player2.setScore(6);
   EXPECT_EQ(&player2, game->getIndividualGameWinner()); // Scores 5,6

   player1.setScore(6);
   EXPECT_EQ(NULL, game->getIndividualGameWinner());     // Scores: 6,6

   game->addClient(&player3);
   game->addClient(&player4);
   player1.setScore(4);
   player2.setScore(5);
   player3.setScore(5);
   player4.setScore(4);
   EXPECT_EQ(NULL, game->getIndividualGameWinner());     // Scores: 4,5,5,4 <-- scenario of concern in comments

   player3.setScore(6);
   EXPECT_EQ(&player3, game->getIndividualGameWinner()); // Scores: 4,5,6,4

   player1.setScore(3);
   player2.setScore(4);
   EXPECT_EQ(NULL, game->getIndividualGameWinner());     // Scores: 4,4,4,4

   player1.setScore(6);
   EXPECT_EQ(&player1, game->getIndividualGameWinner()); // Scores: 6,4,4,4

   player1.setScore(4);
   player4.setScore(6);
   EXPECT_EQ(&player3, game->getIndividualGameWinner()); // Scores: 4,4,4,6
}


};
