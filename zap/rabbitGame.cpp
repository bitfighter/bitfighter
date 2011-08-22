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
#include "SoundSystem.h"
#include "flagItem.h"
#include "Colors.h"
#include "game.h"
#include "UIMenuItems.h"

#ifndef ZAP_DEDICATED
#include "ClientGame.h"
#endif

#include <stdio.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT_RPC(RabbitGameType, s2cRabbitMessage, (U32 msgIndex, StringTableEntry clientName), (msgIndex, clientName),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = dynamic_cast<ClientGame *>(getGame());
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) return;

   switch (msgIndex)
   {
   case RabbitMsgGrab:
      SoundSystem::playSoundEffect(SFXFlagCapture);
      clientGame->displayMessage(Color(1.0f, 0.0f, 0.0f),
                  "%s GRABBED the Carrot!",
                  clientName.getString());
      break;
   case RabbitMsgRabbitKill:
      SoundSystem::playSoundEffect(SFXShipHeal);
      clientGame->displayMessage(Color(1.0f, 0.0f, 0.0f),
                  "%s is a rabbid rabbit!",
                  clientName.getString());
      break;
   case RabbitMsgDrop:
      SoundSystem::playSoundEffect(SFXFlagDrop);
      clientGame->displayMessage(Color(0.0f, 1.0f, 0.0f),
                  "%s DROPPED the Carrot!",
                  clientName.getString());
      break;
   case RabbitMsgRabbitDead:
      SoundSystem::playSoundEffect(SFXShipExplode);
      clientGame->displayMessage(Color(1.0f, 0.0f, 0.0f),
                  "%s killed the rabbit!",
                  clientName.getString());
      break;
   case RabbitMsgReturn:
      SoundSystem::playSoundEffect(SFXFlagReturn);
      clientGame->displayMessage(Color(1.0f, 0.0f, 1.0f),
                  "The Carrot has been returned!");
      break;
   case RabbitMsgGameOverWin:
      clientGame->displayMessage(Color(1.0f, 1.0f, 0.0f),
                  "%s is the top rabbit!",
                  clientName.getString());
      break;
   case RabbitMsgGameOverTie:
      clientGame->displayMessage(Color(1.0f, 1.0f, 0.0f),
                  "No top rabbit - Carrot wins by default!");
      break;
   }
#endif
}

//-----------------------------------------------------
// RabbitGameType
//-----------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(RabbitGameType);

bool RabbitGameType::processArguments(S32 argc, const char **argv, Game *game)
{
   if (argc != 4)
      return false;

   if(!Parent::processArguments(argc, argv, game))
      return false;

   mFlagReturnTimer = atoi(argv[2]) * 1000;
   setFlagScore(atoi(argv[3]));

   return true;
}


string RabbitGameType::toString()
{
   return Parent::toString() + itos(U32(mFlagReturnTimer / 1000)) + itos(getFlagScore());
}


// Any unique items defined here must be handled in both getMenuItem() and saveMenuItem() below!
const char **RabbitGameType::getGameParameterMenuKeys()
{
    static const char *items[] = {
      "Level Name",
      "Level Descr",
      "Level Credits",
      "Levelgen Script",
      "Game Time",
      "Flag Return Time",   // <=== defined here
      "Win Score",        
      "Point Earn Rate",    // <=== defined here
      "Grid Size",
      "Min Players",
      "Max Players",
      "Allow Engr",
      "" };

      return items;
}


// Definitions for those items
boost::shared_ptr<MenuItem> RabbitGameType::getMenuItem(ClientGame *game, const char *key)
{
   if(!strcmp(key, "Flag Return Time"))
      return boost::shared_ptr<MenuItem>(new CounterMenuItem(game, "Flag Return Timer:", mFlagReturnTimer / 1000, 1, 1, 99, 
                                                             "secs", "", "Time it takes for an uncaptured flag to return home"));
   else if(!strcmp(key, "Point Earn Rate"))
      return boost::shared_ptr<MenuItem>(new CounterMenuItem(game, "Point Earn Rate:", getFlagScore(), 1, 1, 99, 
                                                             "points per minute", "", "Rate player holding the flag accrues points"));
   else return Parent::getMenuItem(game, key);
}


bool RabbitGameType::saveMenuItem(const MenuItem *menuItem, const char *key)
{
   if(!strcmp(key, "Flag Return Time"))
      mFlagReturnTimer = menuItem->getIntValue() * 1000;
   else if(!strcmp(key, "Point Earn Rate"))
      setFlagScore(menuItem->getIntValue());
   else return Parent::saveMenuItem(menuItem, key);

   return true;
}


void RabbitGameType::setFlagScore(S32 pointsPerMinute)     
{
   mFlagScoreTimer = U32(1.0f / F32(pointsPerMinute) * 60 * 1000);   // Convert to ms per point
}


S32 RabbitGameType::getFlagScore()     
{
   return S32(1.0f / F32(mFlagScoreTimer) / 60.0f / 1000.0f);        // Convert to points per minute
}


bool RabbitGameType::objectCanDamageObject(GameObject *damager, GameObject *victim)
{
   if(getGame()->getTeamCount() != 1)
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
// Client only
const Color *RabbitGameType::getShipColor(Ship *s)
{
#ifdef ZAP_DEDICATED
   return &Colors::white;
#else

   if(getGame()->getTeamCount() != 1)
      return Parent::getShipColor(s);

   GameConnection *gc = dynamic_cast<ClientGame *>(getGame())->getConnectionToServer();

   if(!gc)
      return &Colors::white;     // Something's gone wrong!

   Ship *co = dynamic_cast<Ship *>(gc->getControlObject());

   return (s == co || (!shipHasFlag(s) && !shipHasFlag(co))) ? &Colors::green : &Colors::red;
#endif
}


Color RabbitGameType::getTeamColor(S32 team)
{
   if(team != -1 || getGame()->getTeamCount() != 1)
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


void RabbitGameType::idle(GameObject::IdleCallPath path, U32 deltaT)
{
   Parent::idle(path, deltaT);
   if(path != GameObject::ServerIdleMainLoop)
      return;

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

               for(S32 i = 0; i < getClientCount(); i++)
                  getClient(i)->clientConnection->s2cDisplayMessageE( GameConnection::ColorNuclearGreen, SFXFlagReturn, returnString, Vector<StringTableEntry>() );
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

   if(ship->getOwner())
      ship->getOwner()->mStatistics.mFlagPickup++;

}


bool RabbitGameType::teamHasFlag(S32 teamId) const
{
   for(S32 i = 0; i < mFlags.size(); i++)
      if(mFlags[i] && mFlags[i]->isMounted() && mFlags[i]->getMount() && mFlags[i]->getMount()->getTeam() == (S32)teamId)
         return true;

   return false;
}


void RabbitGameType::itemDropped(Ship *ship, MoveItem *item)
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



};  //namespace Zap


