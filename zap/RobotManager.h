//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _ROBOT_MANAGER_H_
#define _ROBOT_MANAGER_H_

#include "GameSettings.h"
#include "ClientInfo.h"       // For ClientClass enum
#include "TeamConstants.h"    // For NO_TEAM def

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
   ServerGame *mGame;

public:
   RobotManager(ServerGame *game, GameSettingsPtr settings);     // Contsructor
   virtual ~RobotManager();                                      // Destructor

   void onLevelChanged();        // Called when level changes or is reset

   void balanceTeams();

   string addBot(const Vector<string> &args, ClientInfo::ClientClass clientClass, S32 teamIndex = NO_TEAM);

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
   void deleteBotsFromTeam(S32 botsToKick, S32 teamIndex);
   bool deleteBotFromTeam(S32 teamIndex, ClientInfo::ClientClass botClass);

   // Currently only used by tests to temporarily disable bot leveling while setting up various team configurations
   bool getAutoLevelingEnabled() const;
   void setAutoLeveling(bool enabled);


   static void printTeams(Game *game, const string &message);  // For debugging only

   void deleteAllBots();

   void clearMoves();
};

}


#endif

