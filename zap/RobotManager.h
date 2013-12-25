//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _ROBOT_MANAGER_H_
#define _ROBOT_MANAGER_H_

#include "GameSettings.h"

#include "tnlTypes.h"

using namespace std;
using namespace TNL;

namespace Zap
{
  
class ServerGame;
class Robot;

class RobotManager
{
private:
   Vector<Robot *> mRobots;      // Grand master list of all robots in the current game

   bool mManagerActive;          // True when the manager is active
   bool mAutoLevelTeams;         // When true, bots will be added/removed to make sure all teams are even
   
   S32 mTargetPlayerCount;       // Target number of bots and players; actual count may be higher when mAutoLevelTeams is true
   S32 mTeams;                   // Team count in the current game
   ServerGame *mGame;

public:
   RobotManager(ServerGame *game, GameSettingsPtr settings);     // Contsructor
   virtual ~RobotManager();                                      // Destructor

   void onLevelChanged(S32 teams);     // Called when level changes or is reset

   void balanceTeams();

   string addBot(const Vector<const char *> &args);

   static S32 getMaxBots(GameSettings *settings, bool isAdmin);

   // These are public only for access by tests
   static S32 findMinPlayers(S32 players, S32 teams);   
   static S32 getMaxPlayersPerBalancedTeam(S32 players, S32 teams);


   Robot *getBot(S32 index);
   S32 getBotCount() const;
   Robot *findBot(const char *id);
   void addBot(Robot *robot);
   void removeBot(Robot *robot);
   void deleteBot(const StringTableEntry &name);
   void deleteBot(S32 i);
   void moreBots();
   void fewerBots();
   void deleteBotFromTeam(S32 teamIndex);

   void deleteAllBots();

   void clearMoves();
};

}


#endif

