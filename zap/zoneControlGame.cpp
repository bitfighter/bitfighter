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

// Which team has the flag?
GAMETYPE_RPC_S2C(ZoneControlGameType, s2cSetFlagTeam, (S32 flagTeam), (flagTeam))
{
   mFlagTeam = flagTeam;
}

// Constructor
ZoneControlGameType::ZoneControlGameType()
{
   mFlagTeam = -1;
   mZcBadgeAchievable = true;
   mPossibleZcBadgeAchiever = NULL;
}


void ZoneControlGameType::onGhostAvailable(GhostConnection *theConnection)
{
   Parent::onGhostAvailable(theConnection);
   NetObject::setRPCDestConnection(theConnection);
   s2cSetFlagTeam(mFlagTeam);
   NetObject::setRPCDestConnection(NULL);
}


void ZoneControlGameType::addFlag(FlagItem *flag)
{
   Parent::addFlag(flag);
   mFlag = flag;
   if(!isGhost())
      addItemOfInterest(flag);      // Server only
}


// Ship picks up the flag
void ZoneControlGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   static StringTableEntry takeString("%e0 of team %e1 has the flag!");

   // A ship can only carry one flag in ZC.  If it already has one, there's nothing to do.
   if(theShip->carryingFlag() != NO_FLAG)
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

   mFlagTeam = theShip->getTeam();
   s2cSetFlagTeam(mFlagTeam);

   BfObject *theZone = theShip->isInZone(GoalZoneTypeNumber);

   if(theZone && theZone->getObjectTypeNumber() == GoalZoneTypeNumber)
      shipTouchZone(theShip, static_cast<GoalZone *>(theZone));
}


void ZoneControlGameType::itemDropped(Ship *ship, MoveItem *item)
{
   // Only continue if we're dropping a flag
   if(item->getObjectTypeNumber() != FlagTypeNumber)
      return;

   if(ship->getClientInfo())
   {
      s2cSetFlagTeam(-1);
      static StringTableEntry dropString("%e0 dropped the flag!");

      Vector<StringTableEntry> e;
      e.push_back(ship->getClientInfo()->getName());

      broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
   }
}

void ZoneControlGameType::addZone(GoalZone *z)
{
   mZones.push_back(z);
}

// Ship enters a goal zone.  What happens?
void ZoneControlGameType::shipTouchZone(Ship *s, GoalZone *z)
{
   // Zone already belongs to team, or ship has no flag.  In either case, do nothing.
   if(z->getTeam() == s->getTeam() || s->carryingFlag() == NO_FLAG)
      return;

   S32 oldTeam = z->getTeam();
   if(oldTeam >= 0)    // Zone is being captured from another team
   {
      if(mZones.size() <= 50)  // Don't display message when too many zones -- the flood of messages will get annoying!
      {
         static StringTableEntry takeString("%e0 captured a zone from team %e1!");
         Vector<StringTableEntry> e;
         e.push_back(s->getClientInfo()->getName());
         e.push_back(getGame()->getTeamName(oldTeam));

         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
      }
      updateScore(z->getTeam(), UncaptureZone);      // Inherently team-only event, no?
   }
   else                 // Zone is neutral (i.e. NOT captured from another team)
   {
      if(mZones.size() <= 50)
      {
         static StringTableEntry takeString("%e0 captured an unclaimed zone!");
         Vector<StringTableEntry> e;
         e.push_back(s->getClientInfo()->getName());

         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
      }
   }

   updateScore(s, CaptureZone);

   z->setCapturer(s->getClientInfo());                   // Assign zone to capturing player
   z->setTeam(s->getTeam());                             // Assign zone to capturing team
   s->getClientInfo()->getStatistics()->mFlagScore++;    // Record the capture


   // Check to see if team now controls all zones...
   for(S32 i = 0; i < mZones.size(); i++)
      if(mZones[i]->getTeam() != s->getTeam())     // ...no?...
         return;                                   // ...then bail

   // Team DOES control all zones.  Broadcast a message, flash zones, and create hoopla!
   static StringTableEntry tdString("Team %e0 scored a touchdown!");
   Vector<StringTableEntry> e;
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
         mPossibleZcBadgeAchiever = mZones[0]->getCapturer();

      for(S32 i = 0; i < mZones.size(); i++)
      {
         if(mZones[i]->getCapturer() != mPossibleZcBadgeAchiever)
         {
            mZcBadgeAchievable = false;
            break;
         }
      }
   }

   // Reset zones to neutral
   for(S32 i = 0; i < mZones.size(); i++)
   {
      mZones[i]->setTeam(-1);
      mZones[i]->setCapturer(NULL);
   }

   // Return the flag to spawn point
   for(S32 i = 0; i < s->mMountedItems.size(); i++)
   {
      MoveItem *theItem = s->mMountedItems[i];
      if(theItem->getObjectTypeNumber() != FlagTypeNumber)
         continue;

      FlagItem *mountedFlag = static_cast<FlagItem *>(theItem);

      mountedFlag->dismount();
      mountedFlag->sendHome();
   }

   mFlagTeam = -1;
   s2cSetFlagTeam(-1);
}


// Could probably be consolodated with HTF & others
void ZoneControlGameType::performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo)
{
   Parent::performProxyScopeQuery(scopeObject, clientInfo);

   GameConnection *connection = clientInfo->getConnection();

   S32 uTeam = scopeObject->getTeam();
   if(mFlag.isValid())
   {
      if(mFlag->isAtHome())
         connection->objectInScope(mFlag);
      else
      {
         Ship *mount = mFlag->getMount();
         if(mount && mount->getTeam() == uTeam)
         {
            connection->objectInScope(mount);
            connection->objectInScope(mFlag);
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

   if(!object || object->getObjectTypeNumber() != PlayerShipTypeNumber)
      return;

   Ship *ship = static_cast<Ship *>(object);

   bool hasFlag = mFlag.isValid() && mFlag->getMount() == ship;

   // First, render all zones that have not yet been taken by the flag holding team
   if(mFlagTeam != -1)
   {
      for(S32 i = 0; i < mZones.size(); i++)
      {
         if(mZones[i]->getTeam() != mFlagTeam)
            renderObjectiveArrow(mZones[i], mZones[i]->getColor(), hasFlag ? 1.0f : 0.4f);
         else if(mZones[i]->didRecentlyChangeTeam() && mZones[i]->getTeam() != -1 && ship->getTeam() != mFlagTeam)
         {
            // Render a blinky arrow for a recently taken zone
            Color c = mZones[i]->getColor();
            if(mZones[i]->isFlashing())
               c *= 0.7f;
            renderObjectiveArrow(mZones[i], &c);
         }
      }
   }
   if(!hasFlag)
   {
      if(mFlag.isValid())
      {
         if(!mFlag->isMounted())
            renderObjectiveArrow(mFlag, mFlag->getColor());     // Arrow to flag, if not held by player
         else
         {
            Ship *mount = mFlag->getMount();
            if(mount)
               renderObjectiveArrow(mount, mount->getColor());
         }
      }
   }
#endif
}

bool ZoneControlGameType::teamHasFlag(S32 teamId)
{
   for(S32 i = 0; i < mFlags.size(); i++)
   {
      //TNLAssert(mFlags[i], "NULL flag");
      if(mFlags[i])
         if(mFlags[i]->isMounted() && mFlags[i]->getMount() && mFlags[i]->getMount()->getTeam() == (S32)teamId)
            return true;
   }

   return false;
}


// A major scoring event has ocurred -- in this case, it's a touchdown
void ZoneControlGameType::majorScoringEventOcurred(S32 team)
{
   // Find all zones...
   fillVector.clear();
   getGame()->getGameObjDatabase()->findObjects(GoalZoneTypeNumber, fillVector);

   // ...and make sure they're not flashing...
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GoalZone *goalZone = static_cast<GoalZone *>(fillVector[i]);
      if(goalZone)
         goalZone->setFlashCount(0);
   }

   // ...then activate the glowing zone effect
   getGame()->getGameType()->mZoneGlowTimer.reset();
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

   // Let's see if anyone got the Zone Controller badge
   if(mZcBadgeAchievable &&                                       // Badge is still achievable (hasn't been forbidden by rules in shipTouchZone() )
      mPossibleZcBadgeAchiever &&
      mPossibleZcBadgeAchiever->isAuthenticated() &&              // Player must be authenticated
      getGame()->getPlayerCount() >= 4 &&                         // Game must have 4+ human players
      getGame()->getAuthenticatedPlayerCount() >= 2 &&            // Two of whom must be authenticated
      getLeadingScore() == getWinningScore() &&                   // Game must go the full score (no expired time)
      mZones.size() >= 3 &&                                       // There must be at least 3 zones
      getWinningScore() >= 3 * mZones.size() &&                   // The player must capture them all at least 3 times
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


