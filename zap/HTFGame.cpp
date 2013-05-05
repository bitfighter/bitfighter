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

#include "HTFGame.h"
#include "goalZone.h"
#include "gameObjectRender.h"

#include "Game.h"


namespace Zap
{

// Constructor
HTFGameType::HTFGameType()
{
   // Do nothing
}


bool HTFGameType::isFlagGame() const { return true; }


// Server only
void HTFGameType::addFlag(FlagItem *flag)
{
   Parent::addFlag(flag);

   if(!isGhost())
      addItemOfInterest(flag);      // Server only
}


// Note -- neutral or enemy-to-all robots can't pick up the flag!!!  When we add robots, this may be important!!!
void HTFGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   // See if the ship is already carrying a flag - can only carry one at a time
   if(theShip->isCarryingItem(FlagTypeNumber))
      return;

   // Can only pick up flags on your team or neutral
   if(theFlag->getTeam() != -1 && theShip->getTeam() != theFlag->getTeam())
      return;

   // See if this flag is already in a flag zone owned by the ship's team
   if(theFlag->getZone() != NULL && theFlag->getZone()->getTeam() == theShip->getTeam())
      return;

   ClientInfo *clientInfo = theShip->getClientInfo();
   if(!clientInfo)
      return;

   static StringTableEntry stealString("%e0 stole %e2 flag from team %e1!");
   static StringTableEntry takeString("%e0 of team %e1 took %e2 flag!");

   StringTableEntry *r;
   U32 teamIndex;

   if(theFlag->getZone() == NULL)
   {
      r = &takeString;
      teamIndex = theShip->getTeam();
   }
   else
   {
      r = &stealString;
      teamIndex = theFlag->getZone()->getTeam();

      clientInfo->getStatistics()->mFlagReturn++;  // used as flag steal
   }

   clientInfo->getStatistics()->mFlagPickup++;

   Vector<StringTableEntry> e;
   e.push_back(clientInfo->getName());
   e.push_back(getGame()->getTeamName(teamIndex));

   if(getGame()->getGameObjDatabase()->getObjectCount(FlagTypeNumber) == 1)
      e.push_back(theString);
   else
      e.push_back(aString);

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, *r, e);

   theFlag->mountToShip(theShip);
   theFlag->setZone(NULL);
   theFlag->mTimer.clear();

   updateScore(theShip, RemoveFlagFromEnemyZone);
}


void HTFGameType::itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode)
{
   Parent::itemDropped(ship, item, dismountMode);

   if(item->getObjectTypeNumber() == FlagTypeNumber)
   {
      if(dismountMode != DISMOUNT_SILENT)
      {
         if(ship->getClientInfo())
         {
            static StringTableEntry dropString("%e0 dropped %e1 flag!");

            Vector<StringTableEntry> e;
            e.push_back(ship->getClientInfo()->getName());

            if(getGame()->getGameObjDatabase()->getObjectCount(FlagTypeNumber) == 1)
               e.push_back(theString);
            else
               e.push_back(aString);

            broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
         }
      }
   }
}


void HTFGameType::shipTouchZone(Ship *ship, GoalZone *zone)
{
   // Is this our zone?
   if(ship->getTeam() != zone->getTeam())
      return;

   // Does it already have a flag in it?
   const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);
   for(S32 i = 0; i < flags->size(); i++)
      if(static_cast<FlagItem *>(flags->get(i))->getZone() == zone)
         return;

   // Is the ship carrying a flag?
   S32 flagIndex = ship->getFlagIndex();
   if(flagIndex == NO_FLAG)
      return;

   // Ok, the ship has a flag and it's on the ship...
   FlagItem *mountedFlag = static_cast<FlagItem *>(ship->getMountedItem(flagIndex));

   static StringTableEntry capString("%e0 retrieved %e1 flag.  Team %e2 holds %e1 flag!");

   Vector<StringTableEntry> e;
   e.push_back(ship->getClientInfo()->getName());

   if(flags->size() == 1)
      e.push_back(theString);
   else
      e.push_back(aString);

   e.push_back(getGame()->getTeamName(ship->getTeam()));

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

   mountedFlag->dismount(DISMOUNT_SILENT);

   mountedFlag->setZone(zone);                                 // Assign zone to the flag
   mountedFlag->mTimer.reset(ScoreTime);                       // Start countdown 'til scorin' time!  // TODO: Should this timer be on the zone instead?
   mountedFlag->setActualPos(zone->getExtent().getCenter());   // Put flag smartly in center of capture zone

   updateScore(ship, ReturnFlagToZone);
   ship->getClientInfo()->getStatistics()->mFlagScore++;
}


void HTFGameType::idle(BfObject::IdleCallPath path, U32 deltaT)
{
   Parent::idle(path, deltaT);

   if(path != BfObject::ServerIdleMainLoop)
      return;

   // Server only, from here on out
   const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);

   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *flag = static_cast<FlagItem *>(flags->get(i));
      if(flag->getZone() != NULL && flag->mTimer.update(deltaT))     // Flag is in a zone && it's scorin' time!
      {
         S32 team = flag->getZone()->getTeam();
         updateScore(team, HoldFlagInZone);     // Team only --> No logical way to award individual points for this event!!
         flag->mTimer.reset();      // TODO: Move this timer to the zone -- makes no sense to have it on the flag!
      }
   }
}

// Same code as in retrieveGame, CTF
void HTFGameType::performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo)
{
   Parent::performProxyScopeQuery(scopeObject, clientInfo);

   GameConnection *connection = clientInfo->getConnection();

   S32 uTeam = scopeObject->getTeam();


   const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);

   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *flag = static_cast<FlagItem *>(flags->get(i));
      if(flag->isAtHome() || flag->getZone())      // Flag is at home or in a zone
         connection->objectInScope(flag);
      else
      {
         Ship *mount = flag->getMount();
         if(mount && mount->getTeam() == uTeam)
         {
            connection->objectInScope(mount);
            connection->objectInScope(flag);
         }
      }
   }
}


void HTFGameType::renderInterfaceOverlay(bool scoreboardVisible, S32 canvasWidth, S32 canvasHeight) const
{
#ifndef ZAP_DEDICATED

   Parent::renderInterfaceOverlay(scoreboardVisible, canvasWidth, canvasHeight);

   Ship *ship = getGame()->getLocalPlayerShip();

   if(!ship)
      return;

   bool uFlag = false;
   S32 team = ship->getTeam();

   const Vector<DatabaseObject *> *goalZones = getGame()->getGameObjDatabase()->findObjects_fast(GoalZoneTypeNumber);
   const Vector<DatabaseObject *> *flags     = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);

   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *flag = static_cast<FlagItem *>(flags->get(i));
      
      if(flag->getMount() != ship)
         continue;

      // Flag is mounted on our ship (generally, this will only get run once, as ships won't carry more than one flag)
      for(S32 j = 0; j < goalZones->size(); j++)
      {
         GoalZone *goalZone = static_cast<GoalZone *>(goalZones->get(j));

         // Find zones on our team that have no flags
         if(goalZone->getTeam() != team)
            continue;

         bool found = false;
         for(S32 k = 0; k < flags->size(); k++)
         {
            FlagItem *kflag = static_cast<FlagItem *>(flags->get(k));

            if(kflag->getZone() == goalZone)
            {
               found = true;
               break;
            }
         }
         if(!found)
            renderObjectiveArrow(goalZone, canvasWidth, canvasHeight);
      }
      uFlag = true;
      break;
   }

   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *flag = static_cast<FlagItem *>(flags->get(i));

      if(!flag->isMounted() && !uFlag)
      {
         GoalZone *goalZone = flag->getZone();

         if(goalZone && goalZone->getTeam() != team)
            renderObjectiveArrow(flag, goalZone->getColor(), canvasWidth, canvasHeight);
         else if(!goalZone)
            renderObjectiveArrow(flag, getTeamColor(TEAM_NEUTRAL), canvasWidth, canvasHeight);
      }
      else
      {
         Ship *mount = flag->getMount();
         if(mount && mount != ship)
            renderObjectiveArrow(mount, canvasWidth, canvasHeight);
      }
   }
#endif
}

// What does a particular scoring event score?
S32 HTFGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
{
   if(scoreGroup == TeamScore)
   {
      switch(scoreEvent)
      {
         case KillEnemy:
            return 0;
         case KilledByAsteroid:  // Fall through OK
         case KilledByTurret:    // Fall through OK
         case KillSelf:
            return 0;
         case KillTeammate:
            return 0;
         case KillEnemyTurret:
            return 0;
         case KillOwnTurret:
            return 0;
         case ReturnFlagToZone:
            return 0;
         case HoldFlagInZone:    // Per ScoreTime ms
         return 1;
         case RemoveFlagFromEnemyZone:
            return 0;
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
         case KilledByAsteroid:  // Fall through OK
         case KilledByTurret:    // Fall through OK
         case KillSelf:
            return -1;
         case KillTeammate:
            return 0;
         case KillEnemyTurret:
            return 1;
         case KillOwnTurret:
            return -1;
         case ReturnFlagToZone:
            return 2;
         case HoldFlagInZone:    // There's not a good way to award these points
            return naScore;      // and unless we really want them, let's not bother
         case RemoveFlagFromEnemyZone:
            return 1;
         default:
            return naScore;
      }
   }
}


GameTypeId HTFGameType::getGameTypeId() const { return HTFGame; }
const char *HTFGameType::getShortName() const { return "HTF"; }

static const char *instructions[] = { "Hold the flags at",  "your capture zones!" };
const char **HTFGameType::getInstructionString() const { return instructions; }

bool HTFGameType::isTeamGame()          const { return true;  }
bool HTFGameType::canBeTeamGame()       const { return true;  }
bool HTFGameType::canBeIndividualGame() const { return false; }


StringTableEntry HTFGameType::aString("a");
StringTableEntry HTFGameType::theString("the");

TNL_IMPLEMENT_NETOBJECT(HTFGameType);


};


