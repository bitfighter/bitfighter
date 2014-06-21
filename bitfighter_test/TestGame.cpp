//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"

#include "ServerGame.h"
#include "gameType.h"
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

   // We'll also test GameType::onGameOver() while we're here... that fn will return true when
   // the game has concluded, false if there is a need for overtime.
   GameType *gameType = game->getGameType();

   S32 index1 = 0;
   S32 index2 = 1;
   S32 index3 = 2;

   ASSERT_EQ(1, game->getTeamCount()) << "Expect game to start off with one team!";

   EXPECT_EQ(OnlyOnePlayerOrTeam, game->getTeamBasedGameWinner().first);  
   ASSERT_EQ(0, game->getTeam(0)->getScore());
   ASSERT_EQ(1, game->getPlayerCount(0));
   EXPECT_TRUE(gameType->onGameOver());

   gamePair.removeClient(0);

   // Add a second team -- game will handle cleanup
   game->addTeam(new Team("Team 2", Colors::green));
   ASSERT_EQ(2, game->getTeamCount());
   ASSERT_EQ(0, game->getTeam(1)->getScore());

   EXPECT_EQ(TiedByTeamsWithNoPlayers, game->getTeamBasedGameWinner().first); // Scores: 0,0, no players
   EXPECT_TRUE(gameType->onGameOver());

   // One player, on first team, score 0,0
   S32 teamIndex = index1;
   gamePair.addClient("Player 1", teamIndex);
   AbstractTeam *team1 = game->getTeam(teamIndex);
   ASSERT_EQ(0, game->getTeam(teamIndex)->getScore());
   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first);       // Scores: 0,0  -- tied, but only first team has players
   EXPECT_EQ(teamIndex, game->getTeamBasedGameWinner().second); 
   EXPECT_TRUE(gameType->onGameOver());
   gamePair.removeClient("Player 1");
   EXPECT_EQ(TiedByTeamsWithNoPlayers, game->getTeamBasedGameWinner().first);      // Scores: 0,0
   EXPECT_TRUE(gameType->onGameOver());

   // One player, on second team, score 0,0
   teamIndex = index2;
   gamePair.addClient("Player 2", teamIndex);
   AbstractTeam *team2 = game->getTeam(teamIndex);
   ASSERT_EQ(0, game->getTeam(teamIndex)->getScore());
   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first);       // Scores: 0,0  -- tied, but only second team has players
   EXPECT_EQ(teamIndex, game->getTeamBasedGameWinner().second); 
   EXPECT_TRUE(gameType->onGameOver());

   // One player each on teams 1 and 2, score 0,0
   teamIndex = index1;
   gamePair.addClient("Player 1", teamIndex);
   EXPECT_EQ(Tied, game->getTeamBasedGameWinner().first);      // Scores: 0,0  -- tied
   EXPECT_FALSE(gameType->onGameOver());

   // One player each on teams 1 and 2, score 0,1
   team1->setScore(0);
   team2->setScore(1);
   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first); 
   EXPECT_EQ(index2, game->getTeamBasedGameWinner().second);
   EXPECT_TRUE(gameType->onGameOver());

   // One player each on teams 1 and 2, score 1,0
   team1->setScore(0);
   team2->setScore(1);
   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first); 
   EXPECT_EQ(index2, game->getTeamBasedGameWinner().second);
   EXPECT_TRUE(gameType->onGameOver());

   // One player each on teams 1 and 2, score 1,1
   team1->setScore(1);
   team2->setScore(1);
   EXPECT_EQ(Tied, game->getTeamBasedGameWinner().first); 
   EXPECT_FALSE(gameType->onGameOver());

   // Add a third team with 1 player... game still tied at 1,1,0
   game->addTeam(new Team("Team 3", Colors::yellow));
   ASSERT_EQ(3, game->getTeamCount());
   gamePair.addClient("Player 3", index3);
   AbstractTeam *team3 = game->getTeam(index3);
   EXPECT_EQ(Tied, game->getTeamBasedGameWinner().first); 
   EXPECT_FALSE(gameType->onGameOver());

   // Three way tie: 1,1,1
   team3->setScore(1);
   EXPECT_EQ(Tied, game->getTeamBasedGameWinner().first); 
   EXPECT_FALSE(gameType->onGameOver());

   // Clear winner: 1,1,2
   team3->setScore(2);
   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first); 
   EXPECT_EQ(index3, game->getTeamBasedGameWinner().second);
   EXPECT_TRUE(gameType->onGameOver());

   // Clear winner: 4,1,2
   team1->setScore(4);
   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first); 
   EXPECT_EQ(index1, game->getTeamBasedGameWinner().second);
   EXPECT_TRUE(gameType->onGameOver());

   /*FileLogConsumer gMainLog;
    gMainLog.init(joindir("f:/tmp", "bitfighter.log"), "w");
   gMainLog.setMsgType(LogConsumer::LogEventConnection,    true);
*/

   // Player 2 quits, leaving team 2 without players; score still 4,1,2
   gamePair.removeClient("Player 2");
   game->countTeamPlayers();
   EXPECT_EQ(0, team2->getPlayerBotCount());

   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first); 
   EXPECT_EQ(index1, game->getTeamBasedGameWinner().second);
   EXPECT_TRUE(gameType->onGameOver());

   // Score 4,5,2
   team2->setScore(5);
   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first); 
   EXPECT_EQ(index2, game->getTeamBasedGameWinner().second);      // <-- should team 2 win??
   EXPECT_TRUE(gameType->onGameOver());

   // Score 5,5,2
   team1->setScore(5);
   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first); 
   EXPECT_EQ(index1, game->getTeamBasedGameWinner().second);      // Tied, but team 1 is declared winner
   EXPECT_TRUE(gameType->onGameOver());

   // Score 0,5,5
   team1->setScore(0);
   team2->setScore(5);
   team3->setScore(5);
   EXPECT_EQ(HasWinner, game->getTeamBasedGameWinner().first); 
   EXPECT_EQ(index3, game->getTeamBasedGameWinner().second);      // Tied, but team 3 is declared winner
   EXPECT_TRUE(gameType->onGameOver());

   // Score 5,0,5
   team1->setScore(5);
   team2->setScore(0);
   team3->setScore(5);
   EXPECT_EQ(Tied, game->getTeamBasedGameWinner().first); 
   EXPECT_FALSE(gameType->onGameOver());

   // Score 5,5,5
   team1->setScore(5);
   team2->setScore(5);
   team3->setScore(5);
   EXPECT_EQ(Tied, game->getTeamBasedGameWinner().first); 
   EXPECT_FALSE(gameType->onGameOver());
}


};
