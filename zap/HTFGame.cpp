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
#include "ship.h"
#include "flagItem.h"
#include "gameObjectRender.h"
#include "game.h"

namespace Zap
{

   // Server only
   void HTFGameType::addFlag(FlagItem *flag)
   {
      //S32 i;
      //for(i = 0; i < mFlags.size(); i++)     // What is this?!?
      //{
      //   if(mFlags[i] == NULL)
      //   {
      //      mFlags[i] = theFlag;
      //      break;
      //   }
      //}
      //if(i == mFlags.size())
      //   mFlags.push_back(theFlag);    // Parent::addFlag(flag);

      Parent::addFlag(flag);
      if(!isGhost())
         addItemOfInterest(flag);      // Server only
   }


   void HTFGameType::addZone(GoalZone *zone)
   {
      mZones.push_back(zone);
   }


   // Note -- neutral or enemy-to-all robots can't pick up the flag!!!  When we add robots, this may be important!!!
   void HTFGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
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

      static StringTableEntry stealString("%e0 stole %e2 flag from team %e1!");
      static StringTableEntry takeString("%e0 of team %e1 took %e2 flag!");
      StringTableEntry r = takeString;
      U32 teamIndex;

      if(theFlag->getZone() == NULL)
         teamIndex = theShip->getTeam();
      else
      {
         r = stealString;
         teamIndex = theFlag->getZone()->getTeam();
         if(theShip->getOwner())
            theShip->getOwner()->mStatistics.mFlagReturn++;  // used as flag steal
      }
      if(theShip->getOwner())
         theShip->getOwner()->mStatistics.mFlagPickup++;

      Vector<StringTableEntry> e;
      e.push_back(theShip->getName());
      e.push_back(getGame()->getTeamName(teamIndex));

      if(mFlags.size() == 1)
         e.push_back(theString);
      else
         e.push_back(aString);

      for(S32 i = 0; i < getClientCount(); i++)
         getClient(i)->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, r, e);
      theFlag->mountToShip(theShip);
      theFlag->setZone(NULL);
      theFlag->mTimer.clear();

      updateScore(theShip, RemoveFlagFromEnemyZone);
   }


   void HTFGameType::itemDropped(Ship *ship, Item *item)
   {
      FlagItem *flag = dynamic_cast<FlagItem *>(item);

      if(flag)
      {
         static StringTableEntry dropString("%e0 dropped %e1 flag!");

         Vector<StringTableEntry> e;
         e.push_back(ship->getName());

         if(mFlags.size() == 1)
            e.push_back(theString);
         else
            e.push_back(aString);

         for(S32 i = 0; i < getClientCount(); i++)
            getClient(i)->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
      }
   }


   void HTFGameType::shipTouchZone(Ship *s, GoalZone *z)
   {
      // See if this is an opposing team's zone
      if(s->getTeam() != z->getTeam())
         return;

      // See if this zone already has a flag in it...
      for(S32 i = 0; i < mFlags.size(); i++)
         if(mFlags[i]->getZone() == z)
            return;

      // Ok, it's an empty zone on our team... See if this ship is carrying a flag
      S32 flagIndex = s->carryingFlag();
      if(flagIndex == NO_FLAG)
         return;

      // Ok, the ship has a flag and it's on the ship...
      Item *theItem = s->mMountedItems[flagIndex];
      FlagItem *mountedFlag = dynamic_cast<FlagItem *>(theItem);
      if(mountedFlag)
      {
         static StringTableEntry capString("%e0 retrieved %e1 flag.  Team %e2 holds %e1 flag!");

         Vector<StringTableEntry> e;
         e.push_back(s->getName());
         if(mFlags.size() == 1)
            e.push_back(theString);
         else
            e.push_back(aString);

         e.push_back(getGame()->getTeamName(s->getTeam()));

         for(S32 i = 0; i < getClientCount(); i++)
            getClient(i)->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

         mountedFlag->dismount();

         S32 flagIndex;
         for(flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
            if(mFlags[flagIndex] == mountedFlag)
               break;

         mFlags[flagIndex]->setZone(z);                           // Assign zone to the flag
         mFlags[flagIndex]->mTimer.reset(ScoreTime);              // Start countdown 'til scorin' time!
         mountedFlag->setActualPos(z->getExtent().getCenter());   // Put flag smartly in center of capture zone

         updateScore(s, ReturnFlagToZone);
         if(s->getOwner())
            s->getOwner()->mStatistics.mFlagScore++;

      }
   }


   void HTFGameType::idle(GameObject::IdleCallPath path)
   {
      Parent::idle(path);

      if(path != GameObject::ServerIdleMainLoop)
         return;

      // Server only, from here on out
      for(S32 flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
      {
         if(mFlags[flagIndex]->getZone() != NULL && mFlags[flagIndex]->mTimer.update(mCurrentMove.time))     // Flag is in a zone && it's scorin' time!
         {
            S32 team = mFlags[flagIndex]->getZone()->getTeam();
            updateScore(team, HoldFlagInZone);     // Team only --> No logical way to award individual points for this event!!
            mFlags[flagIndex]->mTimer.reset();
         }
      }
   }

   // Same code as in retrieveGame, CTF
   void HTFGameType::performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
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

   void HTFGameType::renderInterfaceOverlay(bool scoreboardVisible)
   {
      Parent::renderInterfaceOverlay(scoreboardVisible);
      Ship *u = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
      if(!u)
         return;
      bool uFlag = false;
      S32 team = u->getTeam();

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i].isValid() && mFlags[i]->getMount() == u)
         {
            for(S32 j = 0; j < mZones.size(); j++)
            {
               // see if this is one of our zones and that it doesn't have a flag in it.
               if(mZones[j]->getTeam() != team)
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
                  renderObjectiveArrow( mZones[j], getTeamColor(team) );
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
            if(gz && gz->getTeam() != team)
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


StringTableEntry HTFGameType::aString("a");
StringTableEntry HTFGameType::theString("the");

TNL_IMPLEMENT_NETOBJECT(HTFGameType);


};


