//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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
#include "ship.h"
#include "UIGame.h"
#include "flagItem.h"
#include "sfx.h"

#include "../glut/glutInclude.h"
#include <stdio.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(CTFGameType);

void CTFGameType::addFlag(FlagItem *theFlag)
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

void CTFGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   GameConnection *controlConnection = theShip->getControllingClient();
   ClientRef *cl = controlConnection->getClientRef();

   if(!cl)
      return;

   if(cl->teamId == theFlag->getTeam())      // Touch own flag
   {
      if(!theFlag->isAtHome())
      {
         static StringTableEntry returnString("%e0 returned the %e1 flag.");
         Vector<StringTableEntry> e;
         e.push_back(cl->name);
         e.push_back(mTeams[theFlag->getTeam()].name);
         for(S32 i = 0; i < mClientList.size(); i++)
            mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagReturn, returnString, e);

         theFlag->sendHome();

         updateScore(cl, ReturnTeamFlag);
      }
      else
      {
         // Check if this client has an enemy flag mounted
         for(S32 i = 0; i < theShip->mMountedItems.size(); i++)
         {
            Item *theItem = theShip->mMountedItems[i];
            FlagItem *mountedFlag = dynamic_cast<FlagItem *>(theItem);
            if(mountedFlag)
            {
               static StringTableEntry capString("%e0 captured the %e1 flag!");
               Vector<StringTableEntry> e;
               e.push_back(cl->name);
               e.push_back(mTeams[mountedFlag->getTeam()].name);
               for(S32 i = 0; i < mClientList.size(); i++)
                  mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

               mountedFlag->dismount();
               mountedFlag->sendHome();

               updateScore(cl, CaptureFlag);
            }
         }
      }
   }
   else
   {
      static StringTableEntry takeString("%e0 took the %e1 flag!");
      Vector<StringTableEntry> e;
      e.push_back(cl->name);
      e.push_back(mTeams[theFlag->getTeam()].name);
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
      theFlag->mountToShip(theShip);
   }
}

void CTFGameType::flagDropped(Ship *theShip, FlagItem *theFlag)
{
   static StringTableEntry dropString("%e0 dropped the %e1 flag!");
   Vector<StringTableEntry> e;
   e.push_back(theShip->mPlayerName);
   e.push_back(mTeams[theFlag->getTeam()].name);
   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
}

void CTFGameType::performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
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


void CTFGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   Parent::renderInterfaceOverlay(scoreboardVisible);
   Ship *u = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if(!u)
      return;

   for(S32 i = 0; i < mFlags.size(); i++)
   {
      if(!mFlags[i].isValid())
         continue;

      if(mFlags[i]->isMounted())
      {
         Ship *mount = mFlags[i]->getMount();
         if(mount)
            renderObjectiveArrow(mount, getTeamColor(mount->getTeam()));
      }
      else
         renderObjectiveArrow(mFlags[i], getTeamColor(mFlags[i]->getTeam()));
   }
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
         case KillSelf:
            return 0;
         case KillTeammate:
            return 0;
         case KillEnemyTurret:
            return 0;
         case KillOwnTurret:
            return 0;
		   case ReturnTeamFlag:
			   return 1;
		   case CaptureFlag:
			   return 3;
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
		   case ReturnTeamFlag:
			   return 1;
		   case CaptureFlag:
			   return 5;
         default:
            return naScore;
      }
   }
}



};

