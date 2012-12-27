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

#include "CTFGame.h"
#include "flagItem.h"
#include "ClientInfo.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "UIGame.h"
#endif

#include <stdio.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(CTFGameType);


void CTFGameType::addFlag(FlagItem *flag)
{
   Parent::addFlag(flag);

   if(!isGhost())
      addItemOfInterest(flag);      // Server only
}


void CTFGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   ClientInfo *clientInfo = theShip->getClientInfo();

   if(!clientInfo)
      return;

   if(theShip->getTeam() == theFlag->getTeam())      // Touch own flag
   {
      if(!theFlag->isAtHome())
      {
         static StringTableEntry returnString("%e0 returned the %e1 flag.");

         Vector<StringTableEntry> e;
         e.push_back(clientInfo->getName());
         e.push_back(getGame()->getTeamName(theFlag->getTeam()));
         
         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagReturn, returnString, e);

         theFlag->sendHome();

         updateScore(theShip, ReturnTeamFlag);

         clientInfo->getStatistics()->mFlagReturn++;
      }
      else     // Flag is at home
      {
         // Check if this client has an enemy flag mounted
         for(S32 i = 0; i < theShip->getMountedItemCount(); i++)
         {
            MountableItem *mountedItem = theShip->getMountedItem(i);

            if(mountedItem && mountedItem->getObjectTypeNumber() == FlagTypeNumber)
            {
               FlagItem *mountedFlag = static_cast<FlagItem *>(mountedItem);
               static StringTableEntry capString("%e0 captured the %e1 flag!");

               Vector<StringTableEntry> e;
               e.push_back(theShip->getClientInfo()->getName());
               e.push_back(getGame()->getTeamName(mountedFlag->getTeam()));

               broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

               mountedFlag->dismount();
               mountedFlag->sendHome();

               updateScore(theShip, CaptureFlag);

               clientInfo->getStatistics()->mFlagScore++;
            }
         }
      }
   }
   else     // Touched enemy flag
   {
      // Make sure we don't already have a flag mounted... this will never happen in an ordinary game
      // But you can only carry one flag in CTF
      if(!theShip->isCarryingItem(FlagTypeNumber))
      {
         // Alert the clients
         static StringTableEntry takeString("%e0 took the %e1 flag!");
         Vector<StringTableEntry> e;
         e.push_back(clientInfo->getName());
         e.push_back(getGame()->getTeamName(theFlag->getTeam()));

         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);

         theFlag->mountToShip(theShip);

         clientInfo->getStatistics()->mFlagPickup++;
      }
   }
}


class FlagItem;

void CTFGameType::itemDropped(Ship *ship, MoveItem *item)
{
   if(item->getObjectTypeNumber() == FlagTypeNumber)
   {
      FlagItem *flag = static_cast<FlagItem *>(item);
      static StringTableEntry dropString("%e0 dropped the %e1 flag!");

      Vector<StringTableEntry> e;
      e.push_back(ship->getClientInfo()->getName());
      e.push_back(getGame()->getTeamName(flag->getTeam()));

      broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
   }
}


// Identical to HTF, Retrieve
void CTFGameType::performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo)
{
   GameConnection *connection = clientInfo->getConnection();

   Parent::performProxyScopeQuery(scopeObject, clientInfo);
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


void CTFGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
#ifndef ZAP_DEDICATED
   // Render basic overlay
   Parent::renderInterfaceOverlay(scoreboardVisible);

   // Add some objective arrows...
   // ...but rendering objective arrows makes no sense if there is no ship at the moment
   BfObject *object = static_cast<ClientGame *>(getGame())->getConnectionToServer()->getControlObject();

   if(!object || object->getObjectTypeNumber() != PlayerShipTypeNumber)
      return;

   for(S32 i = 0; i < mFlags.size(); i++)
   {
      if(!mFlags[i].isValid())
         continue;

      if(mFlags[i]->isMounted())
      {
         Ship *mount = mFlags[i]->getMount();
         if(mount)
            renderObjectiveArrow(mount);
      }
      else
         renderObjectiveArrow(mFlags[i]);
   }
#endif
}


bool CTFGameType::teamHasFlag(S32 teamIndex) const
{
   return doTeamHasFlag(teamIndex);
}


void CTFGameType::onFlagMounted(S32 teamIndex)
{
   getGame()->setTeamHasFlag(teamIndex, true);
   notifyClientsWhoHasTheFlag();
}


void CTFGameType::onFlagDismounted()
{
   updateWhichTeamsHaveFlags();
}


// What does a particular scoring event score?
S32 CTFGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
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
         case ReturnTeamFlag:
            return 0;
         case CaptureFlag:
            return 1;
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
         case ReturnTeamFlag:
            return 1;
         case CaptureFlag:
            return 5;
         default:
            return naScore;
      }
   }
}


GameTypeId CTFGameType::getGameTypeId() const { return CTFGame; }

const char *CTFGameType::getShortName()         const { return "CTF"; }
const char *CTFGameType::getInstructionString() const { return "Take the opposing team's flag and touch it to your flag!"; }


bool CTFGameType::isFlagGame()          const { return true; }
bool CTFGameType::isTeamGame()          const { return true; }
bool CTFGameType::canBeTeamGame()       const { return true; }
bool CTFGameType::canBeIndividualGame() const { return false; }


};


