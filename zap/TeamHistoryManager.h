//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TEAM_MANAGER_H_
#define _TEAM_MANAGER_H_

#include "Intervals.h"
#include "tnlVector.h"
#include "Timer.h"

#include <string>

using namespace std;

namespace Zap
{


class TeamHistoryManager
{
   typedef pair<string, S32> TeamPair;

private:
   // Support for team locking
   Vector<string> mNames;
   Vector<Timer> mTimers;

   Vector<Vector<string> > mTeamAssignmentNames;
   Vector<Vector<S32> >    mTeamAssignmentTeams;

   void deletePlayer(S32 nameIndex);

public:
   static const S32 LockedTeamsForgetClientTime = ONE_MINUTE;
   static const S32 LockedTeamsNoAdminsGracePeriod = FIFTEEN_SECONDS;

   void idle(U32 timeDelta);

   void onPlayerJoined(const string &name);
   void onPlayerQuit(const string &name);
   void onTeamsUnlocked();

   void addPlayer(const string &name, S32 teamCount, S32 teamIndex);

   S32 getTeam(const string &name, S32 teams) const;
};

}


#endif

