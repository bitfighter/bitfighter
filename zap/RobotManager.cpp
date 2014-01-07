//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "RobotManager.h"

#include "ClientInfo.h"
#include "robot.h"
#include "ServerGame.h"

#include "MathUtils.h"

namespace Zap
{

// Contsructor --> Warning: game may not be fully-formed... do not access any members/functions in this constructor
RobotManager::RobotManager(ServerGame *game, GameSettingsPtr settings)
{
   mManagerActive = true;
   mAutoLevelTeams    = settings->getIniSettings()->playWithBots;
   mTargetPlayerCount = settings->getIniSettings()->minBalancedPlayers;
   mGame = game;
}


// Destructor
RobotManager::~RobotManager()
{
   // Do nothing
}


// Called when level changes or is reset
void RobotManager::onLevelChanged()
{
   mManagerActive = true;    
}


// Find the fewest players who needed to make the specified number of teams have the total number of players specified,
// assuming additional players will be added to make the teams be even.  See RobotManager tests for all sorts of examples.
// Static method
S32 RobotManager::findMinPlayers(S32 players, S32 teams)
{
   players = roundUp(players, teams);     // Make players be an even multiple of teams

   return players - (teams - 1);
}


void RobotManager::balanceTeams()
{
   if(!mAutoLevelTeams || !mManagerActive)
      return;

   // Evaluate team counts
   Vector<Vector<S32> > botCounts = mGame->getCategorizedPlayerCountsByTeam();
   S32 teamCount = mGame->getTeamCount();
   TNLAssert(botCounts.size() == teamCount, "Problem!");

   // First figure out how many "players" we have.  This is basically everyone except bots added by the autoleveler.
   // Fixed players are those we can't shuffle around; basically everyone except autoleveling bots
   S32 totalFixedPlayers = 0;
   S32 largestFixedPlayerCount = 0;

   for(S32 i = 0; i < botCounts.size(); i++)
   {
      S32 kickablePlayers = botCounts[i][ClientInfo::ClassRobotAddedByAutoleveler];
      S32 fixedPlayers = (mGame->getTeam(i)->getPlayerBotCount() - kickablePlayers);

      totalFixedPlayers += fixedPlayers;
      largestFixedPlayerCount = max(largestFixedPlayerCount, fixedPlayers);
   }

   // If teams were balanced, (and we had no autoleveling bots) how many players would the largest team have?
   S32 maxPlayersPerBalancedTeam = getMaxPlayersPerBalancedTeam(mTargetPlayerCount, teamCount);
   S32 playersNeededPerTeam = max(largestFixedPlayerCount, maxPlayersPerBalancedTeam);

   // Kick bots on any teams with more palyers than we need
   for(S32 i = 0; i < teamCount; i++)
   {
      Team *team = static_cast<Team *>(mGame->getTeam(i));

      S32 teamSize = team->getPlayerBotCount();  // All players

      // If the current team has autolevel bots and has more players than the calculated balance should have
      if(teamSize > playersNeededPerTeam)
      {
         // Find the difference
         S32 playersToKick = teamSize - playersNeededPerTeam;
         TNLAssert(playersToKick <= botCounts[i][ClientInfo::ClassRobotAddedByAutoleveler], "Just checking...");

         for(S32 j = 0; j < playersToKick; j++)
            deleteBotFromTeam(i, ClientInfo::ClassRobotAddedByAutoleveler);
      }
      else if(teamSize < playersNeededPerTeam)
      {
         Vector<const char *> noArgs;

         for(S32 j = teamSize; j < playersNeededPerTeam; j++)
            addBot(noArgs, ClientInfo::ClassRobotAddedByAutoleveler, i);
      }
   }

   mAutoLevelTeams = true;    // Reneable autoleveling, which is disabled in deleteBotFromTeam()
}


S32 RobotManager::getMaxPlayersPerBalancedTeam(S32 players, S32 teams)
{
   TNLAssert(teams > 0, "As teams -> 0, getMaxPlayersPerBalancedTeam -> infinity");

   if(players % teams == 0)
      return players / teams;

   return players / teams + 1;
}


string RobotManager::addBot(const Vector<const char *> &args, ClientInfo::ClientClass clientClass, S32 teamIndex)
{
   Robot *robot = new Robot();

   string errorMessage;
   if(!robot->processArguments(args.size(), (const char **)args.address(), mGame, errorMessage))
   {
      delete robot;
      return "!!! " + errorMessage;
   }

   // Set the team if we know it
   if(teamIndex != NO_TEAM)
      robot->setTeam(teamIndex);

   robot->addToGame(mGame, mGame->getGameObjDatabase());
   static_cast<FullClientInfo *>(robot->getClientInfo())->setClientClass(clientClass);

   return "";
}


// What is the most bots we can have on this level?
S32 RobotManager::getMaxBots(GameSettings *settings, bool isAdmin)
{
   static const S32 ABSOLUTE_MAX_BOTS = 255;    // Hard limit on number of bots on the server

   return isAdmin ? ABSOLUTE_MAX_BOTS : settings->getIniSettings()->maxBots;
}


Robot *RobotManager::getBot(S32 index)
{
   return mRobots[index];
}


// Return the total number of bots we are managing
S32 RobotManager::getBotCount() const
{
   return mRobots.size();
}


// Find bot from its id (static)
Robot *RobotManager::findBot(const char *id)
{
   for(S32 i = 0; i < mRobots.size(); i++)
      if(strcmp(mRobots[i]->getScriptId(), id) == 0)
         return mRobots[i];

   return NULL;
}


void RobotManager::addBot(Robot *robot)
{
   mRobots.push_back(robot);
}


// Remove this robot from the list of bots; does not delete it (only called from Robot desctructor)
void RobotManager::removeBot(Robot *robot)
{
   for(S32 i = 0; i < mRobots.size(); i++)
   if(mRobots[i] == robot)
   {
      mRobots.erase_fast(i);
      return;
   }
}


// Delete bot by index
void RobotManager::deleteBot(const StringTableEntry &name)
{
   for(S32 i = 0; i < mRobots.size(); i++)
   if(mRobots[i]->getClientInfo()->getName() == name)
      deleteBot(i);
}


// Delete bot by index
void RobotManager::deleteBot(S32 i)
{
   delete mRobots[i];      // Robot destructor will call removeBot, which will actually remove it from mRobots
}


void RobotManager::moreBots()
{
   mGame->countTeamPlayers();

   S32 teamCount = mGame->getTeamCount();

   // Find largest team player count
   S32 largestTeamCount = 0;

   for(S32 i = 0; i < teamCount; i++)
   {
      S32 currentCount = mGame->getTeam(i)->getPlayerBotCount();

      if(currentCount > largestTeamCount)
         largestTeamCount = currentCount;
   }

   // Determine if there are uneven teams; if so, count up the bots we'll need to add.  We'll
   // add bots until all teams are even.
   S32 neededBotCount = 0;

   for(S32 i = 0; i < teamCount; i++)
   {
      Team *team = static_cast<Team *>(mGame->getTeam(i));
      if(team->getPlayerBotCount() < largestTeamCount)
         neededBotCount += largestTeamCount - team->getPlayerBotCount();
   }

   // If teams all have the same number of players, neededBotCount will be 0 ==> add a bot to each team
   if(neededBotCount == 0)       
      for(S32 i = 0; i < teamCount; i++)
         addBot(Vector<const char *>(), ClientInfo::ClassRobotAddedByAutoleveler);

   // Otherwise, add neededBotCount bots to bring all the teams up to the same number of players as on the biggest team
   else
      for(S32 i = 0; i < neededBotCount; i++)
         addBot(Vector<const char *>(), ClientInfo::ClassRobotAddedByAutoleveler);
   mAutoLevelTeams = true;
   mManagerActive = true;
   mTargetPlayerCount = findMinPlayers(mGame->getPlayerCount() + mGame->getRobotCount(), teamCount);
}


// User activated FEWER ROBOTS menu item
// Our goal here is to reduce the number of bots by one per team, and end up with teams that have balanced player counts.
// If one team has more bots + players than the others, only it will lose a bot.
// If some teams have no bots, they suffer no losses, even if it means things end up unbalanced.
void RobotManager::fewerBots()
{
   mGame->countTeamPlayers();

   S32 teamCount = mGame->getTeamCount();

   S32 largestTeamWithBots = mGame->findLargestTeamWithBots();

   if(largestTeamWithBots == NONE)     // No teams have any bots!
      return;

   S32 targetPlayerCount = mGame->getTeam(largestTeamWithBots)->getPlayerBotCount() - 1;
   TNLAssert(targetPlayerCount >= 0, "Negative players makes no sense!");

   // Scan the teams -- any teams with a bot and more players than targetPlayerCount will lose bots
   for(S32 i = 0; i < teamCount; i++)
   {
      // Determine how many bots we can remove from this team if it has more players than the smallest team
      S32 surplus = mGame->getTeam(i)->getPlayerBotCount() - targetPlayerCount;

      for(S32 j = 0; j < surplus; j++)
         deleteBotFromTeam(i, ClientInfo::ClassRobotAddedByAutoleveler);
   }

   mAutoLevelTeams = true;
   mManagerActive = true;

   mGame->countTeamPlayers();
   mTargetPlayerCount = findMinPlayers(mGame->getPlayerCount() + mGame->getRobotCount(), teamCount);
}


// Currently only used by tests to temporarily disable bot leveling while setting up various team configurations
bool RobotManager::getAutoLevelingEnabled() const
{
   return mAutoLevelTeams;
}


// Currently only used by tests to temporarily disable bot leveling while setting up various team configurations
void RobotManager::setAutoLeveling(bool enabled)
{
   mAutoLevelTeams = enabled;
}


// Dumps teams to the console, used for debugging only
void RobotManager::printTeams(Game *game, const string &message)
{
   game->countTeamPlayers();
   S32 teams = game->getTeamCount();

   string teamDescr = "";

   for(S32 i = 0; i < teams; i++)
   {
      AbstractTeam *team = game->getTeam(i);
      teamDescr += string(team->getPlayerCount(), 'H');
      teamDescr += string(team->getBotCount(), 'B');

      if(team->getPlayerBotCount() == 0)
         teamDescr += "0";

      if(i < teams - 1)
         teamDescr += " ";
   }

   printf("%s: Teams: %s\n", message.c_str(), teamDescr.c_str());
}


// Delete bot from a given team 
// Will teamIndex == NONE gracefully, ok if team contains no bots
void RobotManager::deleteBotFromTeam(S32 teamIndex, ClientInfo::ClientClass botClass)
{
   for(S32 i = 0; i < mRobots.size(); i++)
      if(mRobots[i]->getTeam() == teamIndex && (mRobots[i]->getClientInfo()->getClientClass() == botClass || 
                                                botClass == ClientInfo::ClassAnyBot))
      {
         TNLAssert(teamIndex == mRobots[i]->getClientInfo()->getTeamIndex(), "Inconsistent team info!");
            
         deleteBot(i);

         mAutoLevelTeams = false;
         return;        // Only one!
      }
}


// Delete 'em all, and let god sort 'em out!
// Get here when player issues /kickbots command, or when they choose REMOVE ALL ROBOTS from the game menu
void RobotManager::deleteAllBots()
{
   for(S32 i = mRobots.size() - 1; i >= 0; i--)
      deleteBot(i);

   mManagerActive = false;
   mTargetPlayerCount = 0;
}


void RobotManager::clearMoves()
{
   for(S32 i = 0; i < mRobots.size(); i++)
      mRobots[i]->clearMove();
}


} 
