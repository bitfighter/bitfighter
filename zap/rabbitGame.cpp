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

#include "rabbitGame.h"
#include "ship.h"
#include "UIGame.h"
#include "sfx.h"
#include "flagItem.h"

#include "../glut/glutInclude.h"
#include <stdio.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT_RPC(RabbitGameType, s2cRabbitMessage, (U32 msgIndex, StringTableEntry clientName), (msgIndex, clientName),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   ClientGame *clientGame = dynamic_cast<ClientGame *>(getGame());
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) return;

   switch (msgIndex)
   {
   case RabbitMsgGrab:
      SFXObject::play(SFXFlagCapture);
      clientGame->mGameUserInterface->displayMessage(Color(1.0f, 0.0f, 0.0f),
                  "%s GRABBED the Carrot!",
                  clientName.getString());
      break;
   case RabbitMsgRabbitKill:
      SFXObject::play(SFXShipHeal);
      clientGame->mGameUserInterface->displayMessage(Color(1.0f, 0.0f, 0.0f),
                  "%s is a rabbid rabbit!",
                  clientName.getString());
      break;
   case RabbitMsgDrop:
      SFXObject::play(SFXFlagDrop);
      clientGame->mGameUserInterface->displayMessage(Color(0.0f, 1.0f, 0.0f),
                  "%s DROPPED the Carrot!",
                  clientName.getString());
      break;
   case RabbitMsgRabbitDead:
      SFXObject::play(SFXShipExplode);
      clientGame->mGameUserInterface->displayMessage(Color(1.0f, 0.0f, 0.0f),
                  "%s killed the rabbit!",
                  clientName.getString());
      break;
   case RabbitMsgReturn:
      SFXObject::play(SFXFlagReturn);
      clientGame->mGameUserInterface->displayMessage(Color(1.0f, 0.0f, 1.0f),
                  "The Carrot has been returned!");
      break;
   case RabbitMsgGameOverWin:
      clientGame->mGameUserInterface->displayMessage(Color(1.0f, 1.0f, 0.0f),
                  "%s is the top rabbit!",
                  clientName.getString());
      break;
   case RabbitMsgGameOverTie:
      clientGame->mGameUserInterface->displayMessage(Color(1.0f, 1.0f, 0.0f),
                  "No top rabbit - Carrot wins by default!");
      break;
   }
}

//-----------------------------------------------------
// RabbitGameType
//-----------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(RabbitGameType);

bool RabbitGameType::processArguments(S32 argc, const char **argv)
{
   if (argc != 4)
      return false;

   if(!Parent::processArguments(argc, argv))
      return false;

   mFlagReturnTimer = U32(atof(argv[2]) * 1000);
   mFlagScoreTimer = U32(1.0f / atof(argv[3]) * 60 * 1000); //secs per point

   return true;
}


// Create some game-specific menu items for the GameParameters menu from the arguments processed above...
void RabbitGameType::addGameSpecificParameterMenuItems(Vector<MenuItem *> &menuItems)
{
   menuItems.push_back(new TimeCounterMenuItem("Game Time:", 8 * 60, 99*60, "Unlimited", "Time game will last"));
   menuItems.push_back(new CounterMenuItem("Score to Win:", 60, 5, 5, 500, "points", "", "Game ends when one player or team gets this score"));
   menuItems.push_back(new CounterMenuItem("Flag Return Timer:", 10, 1, 1, 99, "secs", "", "Time it takes for an uncaptured flag to return home"));
   menuItems.push_back(new CounterMenuItem("Point Earn Rate:", 30, 1, 1, 99, "points per minute", "", "Rate player holding the flag accrues points"));
}
 

bool RabbitGameType::objectCanDamageObject(GameObject *damager, GameObject *victim)
{
   if(mTeams.size() != 1)
	   return Parent::objectCanDamageObject(damager, victim);

   if(!damager)
      return true;

   GameConnection *damagerOwner = damager->getOwner();
   GameConnection *victimOwner = victim->getOwner();

   if( (!damagerOwner || !victimOwner) || (damagerOwner == victimOwner))      // Can damage self
      return true;

   Ship *attackShip = dynamic_cast<Ship *>(damagerOwner->getControlObject());
   Ship *victimShip = dynamic_cast<Ship *>(victimOwner->getControlObject());

   if(!attackShip || !victimShip)
      return true;

   // Hunters can only hurt rabbits -- no "friendly fire"
   return shipHasFlag(attackShip) || shipHasFlag(victimShip);
}


// Works for ships and robots!  --> or does it?  Was a template, but it wasn't working for regular ships, haven't tested with robots
Color RabbitGameType::getShipColor(Ship *s)
{
   if(mTeams.size() != 1)
	   return Parent::getShipColor(s);

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(!gc)
      return Color();
   Ship *co = dynamic_cast<Ship *>(gc->getControlObject());

   if(s == co || (!shipHasFlag(s) && !shipHasFlag(co)))
      return Color(0,1,0);
   // else
   return Color(1,0,0);
}


Color RabbitGameType::getTeamColor(S32 team)
{
   if(team != -1 || mTeams.size() != 1)
	   return Parent::getTeamColor(team);

   return Color(1, 0.5, 0);      // orange neutral team, so the neutral flag is orange.
}


bool RabbitGameType::shipHasFlag(Ship *ship)
{
   if(!ship)
      return false;

   for (S32 k = 0; k < ship->mMountedItems.size(); k++)
   {
      if(dynamic_cast<FlagItem *>(ship->mMountedItems[k].getPointer()))
         return true;
   }
   return false;
}


void RabbitGameType::idle(GameObject::IdleCallPath path)
{
   Parent::idle(path);
   if(path != GameObject::ServerIdleMainLoop)
      return;

   U32 deltaT = mCurrentMove.time;

   for(S32 flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
   {
      FlagItem *mRabbitFlag = mFlags[flagIndex];
      if(!mRabbitFlag)
		{
         TNLAssert(false, "RabbitGameType::idle NULL mFlags");
         mFlags.erase_fast(flagIndex);
		}
      else
      {
         if(mRabbitFlag->isMounted())
         {
            if(mRabbitFlag->mTimer.update(deltaT))
            {
               onFlagHeld(mRabbitFlag->getMount());
               mRabbitFlag->mTimer.reset(mFlagScoreTimer);
            }
         }
         else
         {
            if(!mRabbitFlag->isAtHome() && mRabbitFlag->mTimer.update(deltaT))
            {
               mRabbitFlag->sendHome();
               static StringTableEntry returnString("The carrot has been returned!");
               for (S32 i = 0; i < mClientList.size(); i++)
                  mClientList[i]->clientConnection->s2cDisplayMessageE( GameConnection::ColorNuclearGreen, SFXFlagReturn, returnString, Vector<StringTableEntry>() );
            }
         }
      }
   }
}

void RabbitGameType::controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject)
{
   Parent::controlObjectForClientKilled(theClient, clientObject, killerObject);

   Ship *killerShip = NULL;
   GameConnection *ko = killerObject->getOwner();
   if(ko)
      killerShip = dynamic_cast<Ship *>(ko->getControlObject());

   Ship *victimShip = dynamic_cast<Ship *>(clientObject);

   if(killerShip)
   {
      if (shipHasFlag(killerShip))
      {
         // Rabbit killed another person
         onFlaggerKill(killerShip);
      }
      else if(shipHasFlag(victimShip))
      {
         // Someone killed the rabbit!  Poor rabbit!
         onFlaggerDead(killerShip);
      }
   }
}


void RabbitGameType::shipTouchFlag(Ship *ship, FlagItem *flag)
{
   // See if the ship is already carrying a flag - can only carry one at a time
   if(ship->carryingFlag() != NO_FLAG)
      return;

   if(flag->getTeam() != ship->getTeam() && flag->getTeam() != -1)
      return;

   s2cRabbitMessage(RabbitMsgGrab, ship->getName());
   flag->mTimer.reset(mFlagScoreTimer);

   flag->mountToShip(ship);
}


bool RabbitGameType::teamHasFlag(S32 teamId)
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


void RabbitGameType::itemDropped(Ship *ship, Item *item)
{
   FlagItem *flag = dynamic_cast<FlagItem *>(item);

   if(flag)
   {
      flag->mTimer.reset(mFlagReturnTimer);
      s2cRabbitMessage(RabbitMsgDrop, ship->getName());

      Point vel = ship->getActualVel();

      //// Add a random vector to the flag
      //F32 th = TNL::Random::readF() * Float2Pi;
      //F32 f = (TNL::Random::readF() * 2 - 1) * 100;
      //Point dvel(cos(th) * f, sin(th) * f);
      //vel += dvel;

      flag->setActualVel(vel);
   }
}


void RabbitGameType::onFlagHeld(Ship *ship)
{
   updateScore(ship, RabbitHoldsFlag);    // Event: RabbitHoldsFlag
}


void RabbitGameType::addFlag(FlagItem *flag)
{
   Parent::addFlag(flag);
   if(!isGhost())
      flag->setScopeAlways();
}


// Rabbit killed another ship
void RabbitGameType::onFlaggerKill(Ship *rabbitShip)
{
   s2cRabbitMessage(RabbitMsgRabbitKill, rabbitShip->getName());
   updateScore(rabbitShip, RabbitKills);  
}


void RabbitGameType::onFlaggerDead(Ship *killerShip)
{
   s2cRabbitMessage(RabbitMsgRabbitDead, killerShip->getName());
   updateScore(killerShip, RabbitKilled); 
}


// What does a particular scoring event score?
S32 RabbitGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
{
   // In general, there are no team scores in Rabbit games... Teams is now allowed.
   //if(scoreGroup == TeamScore)
   //{
   //   return naScore;
   //}
   //else  // scoreGroup == IndividualScore
   {
      switch(scoreEvent)
      {
         case KillEnemy:
            return 0;
         case KilledByAsteroid:  // Fall through OK
         case KilledByTurret:    // Fall through OK
         case KillSelf:
            return -5;
         case KillTeammate:
            return 0;
         case KillEnemyTurret:
            return 0;
         case KillOwnTurret:
            return 0;
         case RabbitKilled:
            return 5;
         case RabbitKills:
            return 5;
         case RabbitHoldsFlag:      // Points per second
            return 1;
         default:
            return naScore;
      }
   }
}



};  //namespace Zap


