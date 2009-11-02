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

#include "goalZone.h"
#include "gameType.h"
#include "ship.h"
#include "flagItem.h"
#include "gameObjectRender.h"

namespace Zap
{

class RetrieveGameType : public GameType
{
private:
   typedef GameType Parent;
   Vector<GoalZone *> mZones;

public:
   void addFlag(FlagItem *theFlag)
   {
      S32 i;
      for(i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i] == NULL)
         {
            mFlags[i] = theFlag;
            break;
         }
      }
      if(i == mFlags.size())
         mFlags.push_back(theFlag);

      if(!isGhost())
         addItemOfInterest(theFlag);
   }


   void addZone(GoalZone *zone)
   {
      mZones.push_back(zone);
   }


   bool isFlagGame() { return true; }


   // Note -- neutral or enemy-to-all robots can't pick up the flag!!!  
   void shipTouchFlag(Ship *theShip, FlagItem *theFlag)
   {
      // See if the ship is already carrying a flag - can only carry one at a time
      if(theShip->carryingFlag() != NO_FLAG)
         return;

      // Can only pick up flags on your team or neutral
      if(theFlag->getTeam() != -1 && theShip->getTeam() != theFlag->getTeam())
         return;

      S32 flagIndex;

      for(flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
         if(mFlags[flagIndex] == theFlag)
            break;

      // See if this flag is already in a flag zone owned by the ship's team
      if(theFlag->getZone() != NULL && theFlag->getZone()->getTeam() == theShip->getTeam())
         return;

      static StringTableEntry stealString("%e0 stole a flag from team %e1!");
      static StringTableEntry takeString("%e0 of team %e1 took a flag!");
      static StringTableEntry oneFlagTakeString("%e0 of team %e1 took the flag!");

      StringTableEntry r = takeString;

      if(mFlags.size() == 1)
         r = oneFlagTakeString;

      S32 team;
      if(theFlag->getZone() == NULL)      // Picked up flag just sitting around
         team = theShip->getTeam();
      else                                // Grabbed flag from enemy zone
      {
         r = stealString;
         team = theFlag->getZone()->getTeam();
         updateScore(team, LostFlag);
      }

      Vector<StringTableEntry> e;
      e.push_back(theShip->getName());
      e.push_back(mTeams[team].name);

      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, r, e);

      theFlag->mountToShip(theShip);
      updateScore(theShip, RemoveFlagFromEnemyZone);
      theFlag->setZone(NULL);
   }


   void flagDropped(Ship *theShip, FlagItem *theFlag)
   {
      static StringTableEntry dropString("%e0 dropped a flag!");
      Vector<StringTableEntry> e;
      e.push_back(theShip->getName());
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
   }


   // The ship has entered a drop zone, either friend or foe
   void shipTouchZone(Ship *s, GoalZone *z)
   {
      // See if this is an opposing team's zone.  If so, do nothing.
      if(s->getTeam() != z->getTeam())
         return;

      // See if this zone already has a flag in it.  If so, do nothing.
      for(S32 i = 0; i < mFlags.size(); i++)
         if(mFlags[i]->getZone() == z)
            return;

      // Ok, it's an empty zone on our team: See if this ship is carrying a flag...
      S32 flagIndex = s->carryingFlag();

      if(flagIndex == NO_FLAG)
         return;

      // Ok, the ship has a flag and it's on the ship and we're in an empty zone
      Item *theItem = s->mMountedItems[flagIndex];
      FlagItem *mountedFlag = dynamic_cast<FlagItem *>(theItem);
      if(mountedFlag)
      {
         static StringTableEntry capString("%e0 retrieved a flag!");
         static StringTableEntry oneFlagCapString("%e0 retrieved the flag!");

         Vector<StringTableEntry> e;
         e.push_back(s->getName());
         for(S32 i = 0; i < mClientList.size(); i++)
            mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen,
               SFXFlagCapture, (mFlags.size() == 1) ? oneFlagCapString : capString, e);

         // Drop the flag into the zone
         mountedFlag->dismount();

         S32 flagIndex;
         for(flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
            if(mFlags[flagIndex] == mountedFlag)
               break;

         mFlags[flagIndex]->setZone(z);
         mountedFlag->setActualPos(z->getExtent().getCenter());

         // Score the flag...
         updateScore(s, ReturnFlagToZone);

         // See if all the flags are owned by one team...
         for(S32 i = 0; i < mFlags.size(); i++)
         {
            bool ourFlag = (mFlags[i]->getTeam() == s->getTeam()) || (mFlags[i]->getTeam() == -1);    // Team flag || neutral flag
            if(ourFlag && (!mFlags[i]->getZone() || mFlags[i]->getZone()->getTeam() != s->getTeam()))
               return;     // ...if not, we're done
         }

         // One team has all the flags
         if(mFlags.size() != 1)
         {
            static StringTableEntry capAllString("Team %e0 retrieved all the flags!");
            e[0] = mTeams[s->getTeam()].name;

            for(S32 i = 0; i < mClientList.size(); i++)
               mClientList[i]->clientConnection->s2cTouchdownScored(SFXFlagCapture, s->getTeam(), capAllString, e);
         }

         // Return all the flags to their starting locations
         for(S32 i = 0; i < mFlags.size(); i++)
         {
            mFlags[i]->setZone(NULL);
            mFlags[i]->sendHome();
         }
      }
   }


   // A major scoring event has ocurred -- in this case, it's all flags being collected by one team
   void majorScoringEventOcurred(S32 team)
   {
      gClientGame->getGameType()->mZoneGlowTimer.reset();
      mGlowingZoneTeam = team;
   }

   // Same code as in HTF, CTF
   void performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
   {
      Parent::performProxyScopeQuery(scopeObject, connection);
      S32 uTeam = scopeObject->getTeam();

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i]->isAtHome() || mFlags[i]->getZone())
            connection->objectInScope(mFlags[i]);
         else
         {
            Ship *mount = mFlags[i]->getMount();
            if(mount && mount->getTeam() == uTeam)
            {
               connection->objectInScope(mount);
               connection->objectInScope(mFlags[i]);
            }
         }
      }
   }


   void renderInterfaceOverlay(bool scoreboardVisible)
   {
      Parent::renderInterfaceOverlay(scoreboardVisible);
      Ship *u = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
      if(!u)
         return;
      bool uFlag = false;

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i].isValid() && mFlags[i]->getMount() == u)
         {
            for(S32 j = 0; j < mZones.size(); j++)
            {
               // see if this is one of our zones and that it doesn't have a flag in it.
               if(mZones[j]->getTeam() != u->getTeam())
                  continue;
               S32 k;
               for(k = 0; k < mFlags.size(); k++)
               {
                  if(!mFlags[k].isValid())
                     continue;
                  if(mFlags[k]->getZone() == mZones[j])
                     break;
               }
               if(k == mFlags.size())
                  renderObjectiveArrow(mZones[j], getTeamColor(u->getTeam()));
            }
            uFlag = true;
            break;
         }
      }

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(!mFlags[i].isValid())
            continue;

         if(!mFlags[i]->isMounted() && !uFlag)
         {
            GoalZone *gz = mFlags[i]->getZone();
            if(gz && gz->getTeam() != u->getTeam())
               renderObjectiveArrow(mFlags[i], getTeamColor(gz->getTeam()));
            else if(!gz)
               renderObjectiveArrow(mFlags[i], getTeamColor(-1));
         }
         else
         {
            Ship *mount = mFlags[i]->getMount();
            if(mount && mount != u)
               renderObjectiveArrow(mount, getTeamColor(mount->getTeam()));
         }
      }
   }


   // What does a particular scoring event score?
   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
   {
      if(scoreGroup == TeamScore)
      {
         switch(scoreEvent)
         {
            case KillEnemy:
               return 0;
            case KillSelf:
               return 0;
            case KillTeammate:
               return 0;
            case KillEnemyTurret:
               return 0;
            case KillOwnTurret:
               return 0;
            case KilledByAsteroid:
               return 0;
		      case ReturnFlagToZone:
               return 1;
            case RemoveFlagFromEnemyZone:
               return 0;
         case LostFlag:    // Not really an individual scoring event!
               return -1;
            default:
               return naScore;
         }
      }
      else  // scoreGroup == IndividualScore
      {
         switch(scoreEvent)
         {
            case KillEnemy:
               return 1;
            case KillSelf:
               return -1;
            case KillTeammate:
               return 0;
            case KillEnemyTurret:
               return 1;
            case KillOwnTurret:
               return -1;
            case KilledByAsteroid:
               return 0;
            case ReturnFlagToZone:
               return 2;
            case RemoveFlagFromEnemyZone:
               return 1;
            // case LostFlag:    // Not really an individual scoring event!
            //    return 0;
            default:
               return naScore;
         }
      }
   }

   GameTypes getGameType() { return RetrieveGame; }
   const char *getGameTypeString() { return "Retrieve"; }
   const char *getInstructionString() { return "Find all the flags, and bring them to your capture zones!"; }
   bool isTeamGame() { return true; }
   bool canBeTeamGame() { return true; }
   bool canBeIndividualGame() { return false; }


   TNL_DECLARE_CLASS(RetrieveGameType);
};

TNL_IMPLEMENT_NETOBJECT(RetrieveGameType);


};

