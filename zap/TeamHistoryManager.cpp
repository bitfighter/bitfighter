//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TeamHistoryManager.h"

#include "TeamConstants.h"


using namespace std;

namespace Zap
{

void TeamHistoryManager::idle(U32 timeDelta)
{
   for(S32 i = 0; i < mTimers.size(); i++)
      if(mTimers[i].update(timeDelta))
         deletePlayer(i);
}


// Remove player from all tracking structures
void TeamHistoryManager::deletePlayer(S32 nameIndex)
{
   for(S32 i = 0; i < mTeamAssignmentNames.size(); i++)
   {
      S32 index = mTeamAssignmentNames[i].getIndex(mNames[nameIndex]);
      if(index != -1)
      {
         mTeamAssignmentNames[i].erase_fast(index);
         mTeamAssignmentTeams[i].erase_fast(index);
      }
   }

   mTimers.erase_fast(nameIndex);
   mNames.erase_fast(nameIndex);
}


void TeamHistoryManager::addPlayer(const string &name, S32 teamCount, S32 teamIndex)
{
   TNLAssert(teamCount > 0, "Surely there's at least one team here!");

   // No need to track which team players are on in a 1-team game... so we won't!
   if(teamCount <= 1)
      return;

   // See if we already know this player
   if(mNames.getIndex(name) == -1)
   {
      mNames.push_back(name);
      mTimers.push_back(Timer(LockedTeamsForgetClientTime));
      mTimers.last().clear();    // Don't start timer until player quits
   }

   // Make sure we have enough slots available for this teamCount
   // Use teamCount - 2 because Vectors are 0-based, and there are no 0-team configurations;
   //    furthermore, with 1-team configuration, we know everyone is on team 1, so don't bother with it.
   // +1 because slots are a count of how many spots we need, which is inherently 1-indexed.
   S32 slotsRequired = teamCount - 2 + 1;
   if(mTeamAssignmentNames.size() < slotsRequired)
   {
      mTeamAssignmentNames.resize(slotsRequired);
      mTeamAssignmentTeams.resize(slotsRequired);
   }

   // Is player already known to this configuration?
   if(mTeamAssignmentNames[teamCount - 2].getIndex(name) != -1)
      return;

   mTeamAssignmentNames[teamCount - 2].push_back(name);
   mTeamAssignmentTeams[teamCount - 2].push_back(teamIndex);
}


// Clear player's timer
void TeamHistoryManager::onPlayerJoined(const string &name)
{
   // Reset their timer
   S32 index = mNames.getIndex(name);
   if(index == -1)
      return;

   mTimers[index].clear();
}


// Start player's timer
void TeamHistoryManager::onPlayerQuit(const string &name)
{
   S32 index = mNames.getIndex(name);

   if(index == -1)
      return;

   mTimers[index].reset();
}


// When teams are unlocked, we can purge our history
void TeamHistoryManager::onTeamsUnlocked()
{
   mNames.clear();
   mTimers.clear();

   for(S32 i = 0; i < mTeamAssignmentNames.size(); i++)
   {
      mTeamAssignmentNames[i].clear();
      mTeamAssignmentTeams[i].clear();
   }

   mTeamAssignmentNames.clear();
   mTeamAssignmentTeams.clear();
}


// Returns team a player was previously assigned to; returns NO_TEAM if player was not on a team
S32 TeamHistoryManager::getTeam(const string &name, S32 teams) const
{
   TNLAssert(teams > 0, "Surely there's at least one team here!");

   if(teams == 1)
      return 1;

   S32 teamIndex = teams - 2;

   // Do we know anything about games with this many teams?
   if(mTeamAssignmentNames.size() <= teamIndex)
      return NO_TEAM;

   S32 index = mTeamAssignmentNames[teamIndex].getIndex(name);

   // Player is unknown for this team configuration
   if(index == -1)
      return NO_TEAM;

   return mTeamAssignmentTeams[teamIndex][index];
}


}


