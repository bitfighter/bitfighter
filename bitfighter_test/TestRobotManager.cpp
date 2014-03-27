//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "RobotManager.h"

#include "ClientInfo.h"
#include "gameType.h"
#include "ServerGame.h"

#include "LevelFilesForTesting.h"

#include "stringUtils.h"
#include "TestUtils.h"
#include "gtest/gtest.h"

#include <string>


namespace Zap
{

using namespace std;
using namespace TNL;

// TODO: Add some tests related to /addbot(s) and /kickbot(s) and Remove All Bots

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


static char getChar(S32 clientClass)
{
   switch(clientClass)
   {
      case (S32)ClientInfo::ClassHuman:                    return 'H';
      case (S32)ClientInfo::ClassRobotAddedByLevel:        return 'L';
      case (S32)ClientInfo::ClassRobotAddedByLevelNoTeam:  return 'l';
      case (S32)ClientInfo::ClassRobotAddedByAddbots:      return 'A';
      case (S32)ClientInfo::ClassRobotAddedByAutoleveler:  return 'B';
      case (S32)ClientInfo::ClassRobotWithUnknownSource:   return 'U';
      case (S32)ClientInfo::ClassUnknown:                  return 'X'; 
      default: 
         TNLAssert(false, "Unknown class!");
   }

   return '!';    // Or anything... this is never run!
}



// Reverses the above process, generating a string from team config for easy testing
// TODO: Use code from getCategorizedPlayerCountsByTeam() to reduce duplication
static string getTeams(const GamePair &gamePair)
{
   gamePair.server->countTeamPlayers();
   //S32 teams = gamePair.server->getTeamCount();
   Vector<Vector<S32> > teamStrings;

   teamStrings.resize(gamePair.server->getTeamCount());
   for(S32 i = 0; i < teamStrings.size(); i++)
   {
      teamStrings[i].resize((S32)ClientInfo::ClassCount);
      for(S32 j = 0; j < teamStrings[i].size(); j++)
         teamStrings[i][j] = 0;
   }

   const Vector<RefPtr<ClientInfo> > *clientInfos = gamePair.server->getClientInfos();

   for(S32 i = 0; i < clientInfos->size(); i++)
   {
      S32 team = clientInfos->get(i)->getTeamIndex();
      S32 cc   = clientInfos->get(i).getPointer()->getClientClass();

      teamStrings[team][cc]++;
   }

   string teamDescr = "";
   for(S32 i = 0; i < teamStrings.size(); i++)
   {
      S32 total = 0;
      for(S32 j = 0; j < teamStrings[i].size(); j++)
      {
         S32 count = teamStrings[i][j];
         teamDescr += string(count, getChar(j));
         total += count;
      }
      if(total == 0)
         teamDescr += "0";

      if(i < teamStrings.size() - 1)
         teamDescr += " ";
   }

   return teamDescr;
}


// Configure teams based on a configuration specified in teamConfig
// One team per word  e.g. "HHH HHB" would create two teams, one with 3 players, the other with 2 players and a bot
static void setTeams(GamePair &gamePair, const string &teamConfig)
{
   // Turn off autoleveling for the duration of this command
   bool autoLevelingWasEnabled =  gamePair.server->getAutoLevelingEnabled();
   gamePair.server->setAutoLeveling(false);

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
   gamePair.server->setAutoLeveling(autoLevelingWasEnabled);
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
   gamePair.server->addBot(botArgs, ClientInfo::ClassRobotAddedByAddbots);   EXPECT_EQ("HHHH HAB",    getTeams(gamePair));
   gamePair.server->addBot(botArgs, ClientInfo::ClassRobotAddedByAddbots);   EXPECT_EQ("HHHH HAAB",   getTeams(gamePair));
   gamePair.server->addBot(botArgs, ClientInfo::ClassRobotAddedByAddbots);   EXPECT_EQ("HHHHA HAAB",  getTeams(gamePair));
   gamePair.server->addBot(botArgs, ClientInfo::ClassRobotAddedByAddbots);   EXPECT_EQ("HHHHA HAAAB", getTeams(gamePair));
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


TEST(RobotManagerTest, levelsThatIncludeBots)
{
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   settings->getIniSettings()->playWithBots = false;

   // Test what happens when you load a level with several bots on one team, and a new player joins.  This was broken in 019.
   // Try one with a small number of players specified
   {
   settings->getIniSettings()->minBalancedPlayers = 2;
   settings->getIniSettings()->playWithBots       = true;
   GamePair gamePair(settings, getLevelCodeForEmptyLevelWithBots("0 BB"));

   gamePair.server->cycleLevel();       EXPECT_EQ("BB LL",     getTeams(gamePair));

   gamePair.addClient("Cookie Jarvis"); EXPECT_EQ("HB LL",     getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HB LL",     getTeams(gamePair));

   gamePair.addClient("Booberry");      EXPECT_EQ("HH LL",     getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HH LL",     getTeams(gamePair));

   gamePair.addClient("Frankenberry");  EXPECT_EQ("HHH LLB",   getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHH LLB",   getTeams(gamePair));

   gamePair.addClient("Count Chocula"); EXPECT_EQ("HHH HLL",   getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHH HLL",   getTeams(gamePair));

   gamePair.addClient("Toucan Sam");    EXPECT_EQ("HHHH HLLB", getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHHH HLLB", getTeams(gamePair));
   }

   // Same test, but with a larger number of players specified
   {
   settings->getIniSettings()->minBalancedPlayers = 8;
   settings->getIniSettings()->playWithBots       = true;
   GamePair gamePair(settings, getLevelCodeForEmptyLevelWithBots("0 BB"));

   gamePair.server->cycleLevel();       EXPECT_EQ("BBBB LLBB", getTeams(gamePair));

   gamePair.addClient("Cookie Jarvis"); EXPECT_EQ("HBBB LLBB", getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HBBB LLBB", getTeams(gamePair));

   gamePair.addClient("Booberry");      EXPECT_EQ("HHBB LLBB", getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHBB LLBB", getTeams(gamePair));

   gamePair.addClient("Frankenberry");  EXPECT_EQ("HHHB LLBB", getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHHB LLBB", getTeams(gamePair));

   gamePair.addClient("Count Chocula"); EXPECT_EQ("HHHB HLLB", getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHHB HLLB", getTeams(gamePair));

   gamePair.addClient("Toucan Sam");    EXPECT_EQ("HHHH HLLB", getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHHH HLLB", getTeams(gamePair));

   }

   // And again, with bot balancing disabled
   {
   settings->getIniSettings()->playWithBots = false;     // Disables autoleveling
   GamePair gamePair(settings, getLevelCodeForEmptyLevelWithBots("0 BB"));

   gamePair.server->cycleLevel();       EXPECT_EQ("0 LL",     getTeams(gamePair));

   gamePair.addClient("Cookie Jarvis"); EXPECT_EQ("H LL",     getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("H LL",     getTeams(gamePair));
   
   gamePair.addClient("Booberry");      EXPECT_EQ("HH LL",    getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HH LL",    getTeams(gamePair));
   
   gamePair.addClient("Frankenberry");  EXPECT_EQ("HHH LL",   getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHH LL",   getTeams(gamePair));
   
   gamePair.addClient("Count Chocula"); EXPECT_EQ("HHH HLL",  getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHH HLL",  getTeams(gamePair));
   
   gamePair.addClient("Toucan Sam");    EXPECT_EQ("HHHH HLL", getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HHHH HLL", getTeams(gamePair));
   }
}


TEST(RobotManagerTest, autolevelingWithLevelsThatIncludeBots)
{
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   settings->getIniSettings()->minBalancedPlayers = 12;     // A high number!
   settings->getIniSettings()->playWithBots = false;

   // And again, with bot balancing disabled
   {
   settings->getIniSettings()->playWithBots = false;     // Disables autoleveling
   GamePair gamePair(settings, getLevelCodeForEmptyLevelWithBots("0 LL L"));

   gamePair.server->cycleLevel();       EXPECT_EQ("0 LL L",      getTeams(gamePair));
                                                                
   gamePair.addClient("TonyTiger");     EXPECT_EQ("H LL L",      getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("H LL L",      getTeams(gamePair));
                                                                
   gamePair.addClient("SillyRabbit");   EXPECT_EQ("HH LL L",     getTeams(gamePair));
   gamePair.server->cycleLevel();       EXPECT_EQ("HH LL L",     getTeams(gamePair));
                                                                
   // moreBots will enable autoleveling, but the levels will be set based on current conditions... 12 will be ignored
   gamePair.server->moreBots();         EXPECT_EQ("HH LL LB",    getTeams(gamePair));      
   gamePair.server->moreBots();         EXPECT_EQ("HHB LLB LBB", getTeams(gamePair));  
   gamePair.server->fewerBots();        EXPECT_EQ("HH LL LB",    getTeams(gamePair));  

   // Robot manager will remove level-specified bots if there are no more autoleveling bots available
   gamePair.server->fewerBots();        EXPECT_EQ("HH L L",      getTeams(gamePair));  
   // But they'll be back after the next restart, as will an autolevel bot since the autoleveler is now active
   gamePair.server->cycleLevel();       EXPECT_EQ("HH LL LB",    getTeams(gamePair));
   }
}


};
