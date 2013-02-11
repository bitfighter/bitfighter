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

#include "zoneControlGame.h"
#include "goalZone.h"
#include "flagItem.h"
#include "gameObjectRender.h"
#include "ClientInfo.h"
#include "gameConnection.h"
#include "masterConnection.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#endif


namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(ZoneControlGameType);

// Constructor
ZoneControlGameType::ZoneControlGameType()
{
   mZcBadgeAchievable = true;
   mPossibleZcBadgeAchiever = NULL;
}


void ZoneControlGameType::addFlag(FlagItem *flag)
{
   Parent::addFlag(flag);
   if(!isGhost())
      addItemOfInterest(flag);      // Server only
}


// Ship picks up the flag
void ZoneControlGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   static StringTableEntry takeString("%e0 of team %e1 has the flag!");

   // A ship can only carry one flag in ZC.  If it already has one, there's nothing to do.
   if(theShip->isCarryingItem(FlagTypeNumber))
      return;

   // Can only pick up flags on your team or neutral
   if(theFlag->getTeam() != -1 && theShip->getTeam() != theFlag->getTeam())
      return;

   if(!theShip->getClientInfo())
      return;

   Vector<StringTableEntry> e;
   e.push_back(theShip->getClientInfo()->getName());
   e.push_back(getGame()->getTeamName(theShip->getTeam()));

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);

   theFlag->mountToShip(theShip);

   theShip->getClientInfo()->getStatistics()->mFlagPickup++;

   BfObject *theZone = theShip->isInZone(GoalZoneTypeNumber);

   if(theZone && theZone->getObjectTypeNumber() == GoalZoneTypeNumber)
      shipTouchZone(theShip, static_cast<GoalZone *>(theZone));
}


void ZoneControlGameType::itemDropped(Ship *ship, MoveItem *item, MountableItem::DismountMode dismountMode)
{
   Parent::itemDropped(ship, item, dismountMode);

   if(item->getObjectTypeNumber() == FlagTypeNumber)
   {
      if(dismountMode != MountableItem::DISMOUNT_SILENT)
      {
         if(ship->getClientInfo())
         {
            static StringTableEntry dropString("%e0 dropped the flag!");

            Vector<StringTableEntry> e;
            e.push_back(ship->getClientInfo()->getName());

            broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
         }
      }
   }
}


// Ship enters a goal zone.  What happens?
void ZoneControlGameType::shipTouchZone(Ship *s, GoalZone *z)
{
   // Zone already belongs to team, or ship has no flag.  In either case, do nothing.
   if(z->getTeam() == s->getTeam() || !s->isCarryingItem(FlagTypeNumber))
      return;

   static Vector<StringTableEntry> e;
   e.clear();

   static const S32 MAX_ZONES_TO_NOTIFY = 50;   // Don't display messages when too many zones -- the flood of messages will get annoying!
   const S32 oldTeam = z->getTeam();

   const Vector<DatabaseObject *> *zones = getGame()->getGameObjDatabase()->findObjects_fast(GoalZoneTypeNumber);
   S32 zoneCount = zones->size();

   if(oldTeam >= 0)                             // Zone is being captured from another team
   {
      if(zoneCount <= MAX_ZONES_TO_NOTIFY)  
      {
         static StringTableEntry takeString("%e0 captured a zone from team %e1!");
         
         e.push_back(s->getClientInfo()->getName());
         e.push_back(getGame()->getTeamName(oldTeam));

         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
      }

      updateScore(z->getTeam(), UncaptureZone);      // Inherently team-only event, no?
   }
   else                 // Zone is neutral (i.e. NOT captured from another team)
   {
      if(zoneCount <= MAX_ZONES_TO_NOTIFY)
      {
         static StringTableEntry takeString("%e0 captured an unclaimed zone!");

         e.push_back(s->getClientInfo()->getName());

         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
      }
   }

   updateScore(s, CaptureZone);

   z->setCapturer(s->getClientInfo());                   // Assign zone to capturing player
   z->setTeam(s->getTeam());                             // Assign zone to capturing team
   s->getClientInfo()->getStatistics()->mFlagScore++;    // Record the capture

   // Does team control all zones? ...
   for(S32 i = 0; i < zoneCount; i++)
      if(static_cast<GoalZone *>(zones->get(i))->getTeam() != s->getTeam())   // ...no?...
         return;                                                              // ...then bail

   // Team DOES control all zones.  Broadcast a message, flash zones, and create hoopla!
   static StringTableEntry tdString("Team %e0 scored a touchdown!");
   e.clear();
   e.push_back(getGame()->getTeamName(s->getTeam()));

   
   for(S32 i = 0; i < getGame()->getClientCount(); i++)
   {
      GameConnection *gc = getGame()->getClientInfo(i)->getConnection();
      if(gc)
      {
         if(isGameOver())  // Avoid flooding messages on game over. (empty formatString)
            gc->s2cTouchdownScored(SFXNone, s->getTeam(), StringTableEntry(), e);
         else
            gc->s2cTouchdownScored(SFXFlagSnatch, s->getTeam(), tdString, e);
      }
   }

   // Test for zone controller badge
   // We do the following:
   // - Set the first zone capturer in the touchdown to be the possible badge achiever
   // - Test to make sure the same person captures all the zones in the touch down
   // - If the above test ever fails, set a boolean to forbid the badge from being earned
   if(mZcBadgeAchievable)
   {
      if(mPossibleZcBadgeAchiever == NULL)
         mPossibleZcBadgeAchiever = static_cast<GoalZone *>(zones->first())->getCapturer();

      for(S32 i = 0; i < zoneCount; i++)
      {
         if(static_cast<GoalZone *>(zones->get(i))->getCapturer() != mPossibleZcBadgeAchiever)
         {
            mZcBadgeAchievable = false;
            break;
         }
      }
   }

   // Reset zones to neutral
   for(S32 i = 0; i < zoneCount; i++)
   {
      GoalZone *zone = static_cast<GoalZone *>(zones->get(i));

      zone->setTeam(-1);
      zone->setCapturer(NULL);
   }

   // Return the flag to spawn point
   for(S32 i = 0; i < s->getMountedItemCount(); i++)
   {
      MountableItem *item = s->getMountedItem(i);
      if(item->getObjectTypeNumber() != FlagTypeNumber)
         continue;

      FlagItem *mountedFlag = static_cast<FlagItem *>(item);

      mountedFlag->dismount(MountableItem::DISMOUNT_SILENT);
      mountedFlag->sendHome();
   }
}


// Could probably be consolodated with HTF & others
void ZoneControlGameType::performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo)
{
   Parent::performProxyScopeQuery(scopeObject, clientInfo);

   GameConnection *connection = clientInfo->getConnection();

   S32 uTeam = scopeObject->getTeam();

   const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);
   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *flag = static_cast<FlagItem *>(flags->get(i));

      if(flag->isAtHome())
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


// Do some extra rendering required by this game, runs on client
void ZoneControlGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
#ifndef ZAP_DEDICATED

   Parent::renderInterfaceOverlay(scoreboardVisible);

   BfObject *object = static_cast<ClientGame *>(getGame())->getConnectionToServer()->getControlObject();

   if(!object)
      return;

   TNLAssert(object->getObjectTypeNumber() == PlayerShipTypeNumber, "Control object is not ship?!?");    // Added 9/23/2012, can probably delete

   Ship *ship = static_cast<Ship *>(object);

   bool localClientHasFlag = (ship->getFlagCount() != 0);

   const Vector<DatabaseObject *> *zones = getGame()->getGameObjDatabase()->findObjects_fast(GoalZoneTypeNumber);
   const S32 zoneCount = zones->size();

   if(localClientHasFlag)
   {
      // Show all the GoalZones to go to
      for(S32 i = 0; i < zoneCount; i++)
      {
         GoalZone *zone = static_cast<GoalZone *>(zones->get(i));

         TNLAssert(zone, "There was a !zone check here before, not sure what it was for!");

         if(zone->getTeam() != ship->getTeam())
            renderObjectiveArrow(zone);
      }
   }
   else
   {
      const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);

      // Show all flags that can be picked up or is on the ship
      for(S32 i = 0; i < flags->size(); i++)
      {
         FlagItem *flag = static_cast<FlagItem *>(flags->get(i));
         if(flag->getTeam() == TEAM_NEUTRAL || flag->getTeam() == ship->getTeam() || flag->isMounted())
         {
            if(!flag->isMounted())
               renderObjectiveArrow(flag);
            else
               if(flag->getMount())
                  renderObjectiveArrow(flag->getMount());
         }
      }

      S32 whichTeamHasFlag = -1;
      for(S32 i = getGame()->getTeamCount() - 1; i >= 0; i--)
         if(getGame()->getTeamHasFlag(i))
         {
            if(whichTeamHasFlag != -1)
            {
               whichTeamHasFlag = -9; // multiple teams have flags
               break;
            }
            whichTeamHasFlag = i;
         }

      // Show all zones the ship holding flag can go to
      if(whichTeamHasFlag != -1)
         for(S32 i = 0; i < zoneCount; i++)
         {
            GoalZone *zone = static_cast<GoalZone *>(zones->get(i));

            TNLAssert(zone, "There was a !zone check here before, not sure what it was for!");

            if(zone->getTeam() != whichTeamHasFlag)
               renderObjectiveArrow(zone, zone->getColor(), 0.4f);

            //      Zone recently changed hands   &&        Zone is not neutral      &&  Zone is not local player's team 
            else if(zone->didRecentlyChangeTeam() && zone->getTeam() != TEAM_NEUTRAL && zone->getTeam() != ship->getTeam())
            {
               // Render a blinky arrow for a recently captured zone
               Color c = *zone->getColor();

               if(zone->isFlashing())
                  c *= 0.7f;
               renderObjectiveArrow(zone, &c);
            }
         }
   }
#endif
}


bool ZoneControlGameType::teamHasFlag(S32 teamIndex) const
{
   return doTeamHasFlag(teamIndex);
}


void ZoneControlGameType::onFlagMounted(S32 teamIndex)
{
   getGame()->setTeamHasFlag(teamIndex, true);
   notifyClientsWhoHasTheFlag();
}


// A major scoring event has ocurred -- in this case, it's a touchdown
void ZoneControlGameType::majorScoringEventOcurred(S32 team)
{
   // Find all zones...
   fillVector.clear();
   const Vector<DatabaseObject *> *goalZones = getGame()->getGameObjDatabase()->findObjects_fast(GoalZoneTypeNumber);

   // ...and make sure they're not flashing...
   for(S32 i = 0; i < goalZones->size(); i++)
   {
      GoalZone *goalZone = static_cast<GoalZone *>(goalZones->get(i));
      if(goalZone)
         goalZone->setFlashCount(0);
   }

   // ...then activate the glowing zone effect
   mZoneGlowTimer.reset();
}


// What does a particular scoring event score?
S32 ZoneControlGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
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
         case CaptureZone:
            return 1;
         case UncaptureZone:
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
         case CaptureZone:
            return 1;
         case UncaptureZone:    // This pretty much has to stay at 0, as the player doing the "uncapturing" will
            return 0;         // also be credited for a CaptureZone event
         default:
            return naScore;
      }
   }
}


void ZoneControlGameType::onGameOver()
{
   Parent::onGameOver();

   const S32 zoneCount = getGame()->getGameObjDatabase()->getObjectCount(GoalZoneTypeNumber);

   // Let's see if anyone got the Zone Controller badge
   if(mZcBadgeAchievable &&                                       // Badge is still achievable (hasn't been forbidden by rules in shipTouchZone() )
      mPossibleZcBadgeAchiever &&
      mPossibleZcBadgeAchiever->isAuthenticated() &&              // Player must be authenticated
      getGame()->getPlayerCount() >= 4 &&                         // Game must have 4+ human players
      getGame()->getAuthenticatedPlayerCount() >= 2 &&            // Two of whom must be authenticated
      getLeadingScore() == getWinningScore() &&                   // Game must go the full score (no expired time)
      zoneCount >= 3 &&                                           // There must be at least 3 zones
      getWinningScore() >= 3 * zoneCount &&                       // The player must capture them all at least 3 times
      !mPossibleZcBadgeAchiever->hasBadge(BADGE_ZONE_CONTROLLER)) // Player doesn't already have the badge
   {
      achievementAchieved(BADGE_ZONE_CONTROLLER, mPossibleZcBadgeAchiever->getName());
   }
}


GameTypeId ZoneControlGameType::getGameTypeId() const { return ZoneControlGame; }

const char *ZoneControlGameType::getShortName()         const { return "ZC"; }
const char *ZoneControlGameType::getInstructionString() const { return "Capture all the zones by carrying the flag into them! "; }

bool ZoneControlGameType::isFlagGame()          const { return true;  }
bool ZoneControlGameType::isTeamGame()          const { return true;  }
bool ZoneControlGameType::canBeTeamGame()       const { return true;  }
bool ZoneControlGameType::canBeIndividualGame() const { return false; }

};


