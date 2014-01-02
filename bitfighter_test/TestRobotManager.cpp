//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "RobotManager.h"

#include "ClientInfo.h"
#include "gameType.h"
#include "ServerGame.h"

#include "stringUtils.h"
#include "TestUtils.h"
#include "gtest/gtest.h"

#include <string>


namespace Zap
{

using namespace std;
using namespace TNL;

// Test this scenario!!
// Watusimoto: i found a bug, not sure if it's related to your RobotManager - if you start a level with 'Robot' s in the level file, people are sorted to either team without respecting the bot count
// so say I have 5 aegisbots in a file all on team blue and 5 humans are playing, if you restart the level, the humans get places 2 on blue 2 on red
// *4 humans
// only happens if players are already connected to the game server and the level is started

TEST(RobotManagerTest, findMinPlayers)
{ 
   //        v--add this many       players--v  v--teams
   EXPECT_EQ(1, RobotManager::findMinPlayers(1, 1));
   EXPECT_EQ(1, RobotManager::findMinPlayers(1, 4));
   EXPECT_EQ(1, RobotManager::findMinPlayers(2, 4));
   EXPECT_EQ(1, RobotManager::findMinPlayers(3, 4));
   EXPECT_EQ(2, RobotManager::findMinPlayers(2, 1));
   EXPECT_EQ(1, RobotManager::findMinPlayers(2, 2));  // Add 1 player in a 2 team game to get 2 players when bots fill in
   EXPECT_EQ(5, RobotManager::findMinPlayers(5, 1));
   EXPECT_EQ(5, RobotManager::findMinPlayers(5, 2));
   EXPECT_EQ(5, RobotManager::findMinPlayers(6, 2));
   EXPECT_EQ(6, RobotManager::findMinPlayers(6, 5));
   EXPECT_EQ(6, RobotManager::findMinPlayers(9, 5)); // Add 6 players in a 5 team game to get at least 9 players when bots fill in
}


TEST(RobotManagerTest, getMaxPlayersPerBalancedTeam)
{
   //                                             players--v  v--teams
   EXPECT_EQ(1, RobotManager::getMaxPlayersPerBalancedTeam( 1, 1));
   EXPECT_EQ(1, RobotManager::getMaxPlayersPerBalancedTeam( 1, 2));
   EXPECT_EQ(2, RobotManager::getMaxPlayersPerBalancedTeam(10, 5));
   EXPECT_EQ(3, RobotManager::getMaxPlayersPerBalancedTeam(11, 5));
}


// Reverses the above process, generating a string from team config for easy testing
static string getTeams(const GamePair &gamePair)
{
   gamePair.server->countTeamPlayers();
   S32 teams = gamePair.server->getTeamCount();

   string teamDescr = "";

   for(S32 i = 0; i < teams; i++)
   {
      AbstractTeam *team = gamePair.server->getTeam(i);
      teamDescr += string(team->getPlayerCount(), 'H');
      teamDescr += string(team->getBotCount(), 'B');

      if(team->getPlayerBotCount() == 0)
         teamDescr += "0";

      if(i < teams - 1)
         teamDescr += " ";
   }

   return teamDescr;
}


// Configure teams based on a configuration specified in teamConfig
// One team per word  e.g. "HHH HHB" would create two teams, one with 3 players, the other with 2 players and a bot
static void setTeams(GamePair &gamePair, const string &teamConfig)
{
   Vector<string> words = parseString(teamConfig);

   S32 teams = words.size();

   // Add teams -- a fresh gamePair will have 0 players and 1 team
   for(S32 i = 1; i < teams; i++)
      gamePair.server->addTeam(new Team());

   for(S32 i = 0; i < words.size(); i++)           // Iterate over teams
      for(S32 j = 0; j < words[i].size(); j++)     // Iterate over chars
      {
         if(words[i][j] == 'H')
            gamePair.addClient("Human " + itos(i) + " " + itos(j), i);
         else if(words[i][j] == 'B')
            gamePair.addBotClient("Bot " + itos(i) + " " + itos(j), i);
         else
            TNLAssert(false, "Invalid char!");
         //printf("%s == > %s\n", teamConfig.c_str(), getTeams(gamePair).c_str());
      }

   gamePair.server->countTeamPlayers();
}


TEST(RobotManagerTest, moreLessBots)
{
   // Disable auto-leveling so we can create teams without bot manager interfering
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   settings->getIniSettings()->playWithBots = false;

   Vector<const char *> botArgs;

   // By putting each of tese tests in a block, we can not worry too much about how to add/remove bots and players
   // for each test.  Instead, we can blow the entire gamePair away and just start clean each time.  The point of
   // the tests is to look at more/less bots, not adding/removing players in bulk.
   {  // =-=-=-= One team =-=-=-=
   GamePair gamePair(settings); 

   // Make sure game starts off with no players
   ASSERT_EQ(0, gamePair.server->getPlayerCount());

   setTeams(gamePair, "HHB");        EXPECT_EQ("HHB",    getTeams(gamePair));
   gamePair.server->moreBots();      EXPECT_EQ("HHBB",   getTeams(gamePair));
   gamePair.server->moreBots();      EXPECT_EQ("HHBBB",  getTeams(gamePair));
   gamePair.server->moreBots();      EXPECT_EQ("HHBBBB", getTeams(gamePair));
   gamePair.server->fewerBots();     EXPECT_EQ("HHBBB",  getTeams(gamePair));
   gamePair.server->fewerBots();     EXPECT_EQ("HHBB",   getTeams(gamePair));
   gamePair.server->fewerBots();     EXPECT_EQ("HHB",    getTeams(gamePair));
   gamePair.server->fewerBots();     EXPECT_EQ("HH",     getTeams(gamePair));
   gamePair.server->fewerBots();     EXPECT_EQ("HH",     getTeams(gamePair));
   gamePair.server->moreBots();      EXPECT_EQ("HHB",    getTeams(gamePair));    
   }

   {  // =-=-=-= Two teams =-=-=-=
   GamePair gamePair(settings);   

   setTeams(gamePair, "HHH BBB");    EXPECT_EQ("HHH BBB", getTeams(gamePair));

   gamePair.server->fewerBots();     EXPECT_EQ("HHH BB",      getTeams(gamePair));
   gamePair.server->fewerBots();     EXPECT_EQ("HHH B",       getTeams(gamePair));
   gamePair.server->fewerBots();     EXPECT_EQ("HHH 0",       getTeams(gamePair));
   gamePair.server->moreBots();      EXPECT_EQ("HHH BBB",     getTeams(gamePair));    
   gamePair.server->moreBots();      EXPECT_EQ("HHHB BBBB",   getTeams(gamePair));    
   gamePair.server->moreBots();      EXPECT_EQ("HHHBB BBBBB", getTeams(gamePair));  

   // New player joins other team; autoleveling should be enabled -- since target game size is 5v5, bot will be removed
   gamePair.addClient("newclient", 1);    EXPECT_EQ("HHHBB HBBBB",      getTeams(gamePair));

   // With autoleveling on, changing teams should trigger balancing
   ClientInfo *clientInfo = gamePair.server->findClientInfo("newclient");
   gamePair.server->getGameType()->changeClientTeam(clientInfo, 0);  EXPECT_EQ("HHHHB BBBBB", getTeams(gamePair));
   gamePair.server->getGameType()->changeClientTeam(clientInfo, 1);  EXPECT_EQ("HHHBB HBBBB", getTeams(gamePair));

   // Remove the player again, and try again with the other team
   gamePair.removeClient("newclient");    EXPECT_EQ("HHHBB BBBBB",      getTeams(gamePair));
   gamePair.addClient("newclient", 0);    EXPECT_EQ("HHHHB BBBBB",      getTeams(gamePair));

   // /kickbot -- disables autoleveling
   gamePair.server->kickSingleBotFromLargestTeamWithBots();  EXPECT_EQ("HHHH BBBBB", getTeams(gamePair));
   gamePair.server->kickSingleBotFromLargestTeamWithBots();  EXPECT_EQ("HHHH BBBB",  getTeams(gamePair));
   gamePair.server->kickSingleBotFromLargestTeamWithBots();  EXPECT_EQ("HHHH BBB",   getTeams(gamePair));
   gamePair.server->kickSingleBotFromLargestTeamWithBots();  EXPECT_EQ("HHHH BB",    getTeams(gamePair));
   gamePair.server->kickSingleBotFromLargestTeamWithBots();  EXPECT_EQ("HHHH B",     getTeams(gamePair));

   // With autoleveling off, changing teams should not trigger balancing
   clientInfo = gamePair.server->findClientInfo("newclient");
   gamePair.server->getGameType()->changeClientTeam(clientInfo, 1);  EXPECT_EQ("HHH HB", getTeams(gamePair));
   gamePair.server->getGameType()->changeClientTeam(clientInfo, 0);  EXPECT_EQ("HHHH B", getTeams(gamePair));

   // With autoleveling off, no bots will be added when new client joins
   gamePair.addClient("newclient2", 0);    EXPECT_EQ("HHHHH B",      getTeams(gamePair));
   gamePair.removeClient("newclient2");    EXPECT_EQ("HHHH B",       getTeams(gamePair));
   gamePair.addClient("newclient2", 1);    EXPECT_EQ("HHHH HB",      getTeams(gamePair));

   // /addbot
   gamePair.server->addBot(botArgs);   EXPECT_EQ("HHHH HBB",    getTeams(gamePair));
   gamePair.server->addBot(botArgs);   EXPECT_EQ("HHHH HBBB",   getTeams(gamePair));
   gamePair.server->addBot(botArgs);   EXPECT_EQ("HHHHB HBBB",  getTeams(gamePair));
   gamePair.server->addBot(botArgs);   EXPECT_EQ("HHHHB HBBBB", getTeams(gamePair));
   }


   {  // =-=-=-= Three teams =-=-=-=
   GamePair gamePair(settings);                

   setTeams(gamePair, "HHH HH H");   EXPECT_EQ("HHH HH H",       getTeams(gamePair));
   gamePair.server->fewerBots();     EXPECT_EQ("HHH HH H",       getTeams(gamePair));
   gamePair.server->fewerBots();     EXPECT_EQ("HHH HH H",       getTeams(gamePair));
   gamePair.server->moreBots();      EXPECT_EQ("HHH HHB HBB",    getTeams(gamePair));    
   gamePair.server->moreBots();      EXPECT_EQ("HHHB HHBB HBBB", getTeams(gamePair));    

   }

   {  // =-=-=-= Six teams =-=-=-=
   GamePair gamePair(settings);       

   setTeams(gamePair, "BBBBB HH H B B B"); EXPECT_EQ("BBBBB HH H B B B",                             getTeams(gamePair));
   gamePair.server->moreBots();            EXPECT_EQ("BBBBB HHBBB HBBBB BBBBB BBBBB BBBBB",          getTeams(gamePair));    
   gamePair.server->moreBots();            EXPECT_EQ("BBBBBB HHBBBB HBBBBB BBBBBB BBBBBB BBBBBB",    getTeams(gamePair)); 
   gamePair.server->fewerBots();           EXPECT_EQ("BBBBB HHBBB HBBBB BBBBB BBBBB BBBBB",          getTeams(gamePair));
   gamePair.server->fewerBots();           EXPECT_EQ("BBBB HHBB HBBB BBBB BBBB BBBB",                getTeams(gamePair));
   gamePair.server->fewerBots();           EXPECT_EQ("BBB HHB HBB BBB BBB BBB",                      getTeams(gamePair));
   gamePair.server->fewerBots();           EXPECT_EQ("BB HH HB BB BB BB",                            getTeams(gamePair));
   gamePair.server->fewerBots();           EXPECT_EQ("B HH H B B B",                                 getTeams(gamePair));
   gamePair.server->fewerBots();           EXPECT_EQ("0 HH H 0 0 0",                                 getTeams(gamePair));
   gamePair.server->fewerBots();           EXPECT_EQ("0 HH H 0 0 0",                                 getTeams(gamePair));
   gamePair.server->moreBots();            EXPECT_EQ("BB HH HB BB BB BB",                            getTeams(gamePair));    
   gamePair.server->moreBots();            EXPECT_EQ("BBB HHB HBB BBB BBB BBB",                      getTeams(gamePair));    
   }

   {
   GamePair gamePair(settings); 

   setTeams(gamePair, "BBB HHH BBB H B H B");
   EXPECT_EQ("BBB HHH BBB H B H B", getTeams(gamePair));
   }
}


};
