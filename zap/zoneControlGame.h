//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "gameType.h"

namespace Zap
{
class Ship;

class ZoneControlGameType : public GameType
{
private:
   typedef GameType Parent;

   Vector<GoalZone*> mZones;
   SafePtr<FlagItem> mFlag;
   S32 mFlagTeam;

public:
   ZoneControlGameType()
   {
      mFlagTeam = -1;
   }

   bool isFlagGame() { return true; }

   void onGhostAvailable(GhostConnection *theConnection)
   {
      Parent::onGhostAvailable(theConnection);
      NetObject::setRPCDestConnection(theConnection);
      s2cSetFlagTeam(mFlagTeam);
      NetObject::setRPCDestConnection(NULL);
   }

   void shipTouchFlag(Ship *ship, FlagItem *flag);
   void itemDropped(Ship *ship, Item *item);
   void addZone(GoalZone *zone);

   void addFlag(FlagItem *flag)     // Server only
   {
      Parent::addFlag(flag);
      mFlag = flag;
      if(!isGhost())
         addItemOfInterest(flag);      // Server only
   }

   void shipTouchZone(Ship *ship, GoalZone *zone);

   GameTypes getGameType() { return ZoneControlGame; }
   const char *getGameTypeString() const { return "Zone Control"; }
   const char *getShortName() const { return "ZC"; }
   const char *getInstructionString() { return "Capture all the zones by carrying the flag into them! "; }
   bool isTeamGame() { return true; }
   bool canBeTeamGame() { return true; }
   bool canBeIndividualGame() { return false; }


   void renderInterfaceOverlay(bool scoreboardVisible);
   bool teamHasFlag(S32 teamId);

   void performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection);
   void majorScoringEventOcurred(S32 team);    // Gets run when a touchdown is scored

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   TNL_DECLARE_RPC(s2cSetFlagTeam, (S32 newFlagTeam));
   TNL_DECLARE_CLASS(ZoneControlGameType);
};

};