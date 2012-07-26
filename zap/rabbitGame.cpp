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
#include "SoundSystem.h"
#include "flagItem.h"
#include "Colors.h"
#include "gameConnection.h"
#include "stringUtils.h"
#include "ClientInfo.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "UIGame.h"
#  include "UIMenuItems.h"
#endif

#include <stdio.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT_RPC(RabbitGameType, s2cRabbitMessage, (U32 msgIndex, StringTableEntry clientName), (msgIndex, clientName),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = static_cast<ClientGame *>(getGame());

   switch (msgIndex)
   {
      case RabbitMsgGrab:
         SoundSystem::playSoundEffect(SFXFlagCapture);
         clientGame->displayMessage(Colors::red, "%s GRABBED the Carrot!", clientName.getString());
         break;

      case RabbitMsgRabbitKill:
         SoundSystem::playSoundEffect(SFXShipHeal);
         clientGame->displayMessage(Colors::red, "%s is a rabbid rabbit!", clientName.getString());
         break;

      case RabbitMsgDrop:
         SoundSystem::playSoundEffect(SFXFlagDrop);
         clientGame->displayMessage(Colors::green, "%s DROPPED the Carrot!", clientName.getString());
         break;

      case RabbitMsgRabbitDead:
         SoundSystem::playSoundEffect(SFXShipExplode);
         clientGame->displayMessage(Colors::red, "%s killed the rabbit!", clientName.getString());
         break;

      case RabbitMsgReturn:
         SoundSystem::playSoundEffect(SFXFlagReturn);
         clientGame->displayMessage(Colors::magenta, "The Carrot has been returned!");
         break;

      case RabbitMsgGameOverWin:
         clientGame->displayMessage(Colors::yellow, "%s is the top rabbit!", clientName.getString());
         break;

      case RabbitMsgGameOverTie:
         clientGame->displayMessage(Colors::yellow, "No top rabbit - Carrot wins by default!");
         break;

      default:
         TNLAssert(false, "Invalid option");
         break;
   }
#endif
}

//-----------------------------------------------------
// RabbitGameType
//-----------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(RabbitGameType);

// Constructor
RabbitGameType::RabbitGameType()
{
   setWinningScore(100);
   mFlagReturnTimer = 30 * 1000;
   mFlagScoreTimer = 5 * 1000;
}



S32 RabbitGameType::getFlagCount()
{
   return mFlags.size();
}


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


string RabbitGameType::toString() const
{
   return Parent::toString() + " " + itos(U32(mFlagReturnTimer / 1000)) + " " + itos(getFlagScore());
}


#ifndef ZAP_DEDICATED
// Any unique items defined here must be handled in both getMenuItem() and saveMenuItem() below!
Vector<string> RabbitGameType::getGameParameterMenuKeys()
{
   Vector<string> items = Parent::getGameParameterMenuKeys();

   // Use "Win Score" as an indicator of where to insert our Rabbit specific menu items
   for(S32 i = 0; i < items.size(); i++)
      if(items[i] == "Win Score")
      {
         items.insert(i - 1, "Flag Return Time");
         items.insert(i + 2, "Point Earn Rate");

         break;
      }

   return items;
}


// Definitions for those items
boost::shared_ptr<MenuItem> RabbitGameType::getMenuItem(const string &key)
{
   if(key == "Flag Return Time")
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Flag Return Timer:", mFlagReturnTimer / 1000, 1, 1, 99, 
                                                             "secs", "", "Time it takes for an uncaptured flag to return home"));
   else if(key == "Point Earn Rate")
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Point Earn Rate:", getFlagScore(), 1, 1, 99, 
                                                             "points per minute", "", "Rate player holding the flag accrues points"));
   else 
      return Parent::getMenuItem(key);
}


bool RabbitGameType::saveMenuItem(const MenuItem *menuItem, const string &key)
{
   if(key == "Flag Return Time")
      mFlagReturnTimer = menuItem->getIntValue() * 1000;
   else if(key == "Point Earn Rate")
      setFlagScore(menuItem->getIntValue());
   else 
      return Parent::saveMenuItem(menuItem, key);

   return true;
}
#endif


void RabbitGameType::setFlagScore(S32 pointsPerMinute)     
{
   mFlagScoreTimer = U32((1.0f / F32(pointsPerMinute)) * 60 * 1000);   // Convert to ms per point
}


S32 RabbitGameType::getFlagScore() const
{
   return S32(1.0f / (F32(mFlagScoreTimer) / (60 * 1000)));            // Convert to points per minute
}


bool RabbitGameType::objectCanDamageObject(BfObject *damager, BfObject *victim)
{
   if(getGame()->getTeamCount() != 1)
      return Parent::objectCanDamageObject(damager, victim);

   if(!damager)
      return true;

   ClientInfo *damagerOwner = damager->getOwner();
   ClientInfo *victimOwner = victim->getOwner();

   if( (!damagerOwner || !victimOwner) || (damagerOwner == victimOwner))      // Can damage self
      return true;

   Ship *attackShip = damagerOwner->getShip();
   Ship *victimShip = victimOwner->getShip();

   if(!attackShip || !victimShip)
      return true;

   // Hunters can only hurt rabbits -- no "friendly fire"
   return shipHasFlag(attackShip) || shipHasFlag(victimShip);
}


// Works for ships and robots!  --> or does it?  Was a template, but it wasn't working for regular ships, haven't tested with robots
// Client only
const Color *RabbitGameType::getShipColor(Ship *ship)
{
#ifdef ZAP_DEDICATED
   return &Colors::white;
#else

   if(getGame()->getTeamCount() != 1)
      return Parent::getShipColor(ship);

   GameConnection *gc = static_cast<ClientGame *>(getGame())->getConnectionToServer();

   if(!gc)
      return &Colors::white;     // Something's gone wrong!

   BfObject *object = gc->getControlObject();
   if(!object || !isShipType(object->getObjectTypeNumber()))
      return  &Colors::white;    // Something's gone wrong!

   Ship *localShip = static_cast<Ship *>(gc->getControlObject());

   return (ship == localShip || (!shipHasFlag(ship) && !shipHasFlag(localShip))) ? &Colors::green : &Colors::red;
#endif
}


const Color *RabbitGameType::getTeamColor(S32 team) const
{
   if(team != -1 || getGame()->getTeamCount() != 1)
      return Parent::getTeamColor(team);

   return &Colors::orange50;      // orange neutral team, so the neutral flag is orange.
}


bool RabbitGameType::shipHasFlag(Ship *ship)
{
   if(!ship)
      return false;

   for (S32 k = 0; k < ship->mMountedItems.size(); k++)
   {
      if(ship->mMountedItems[k].getPointer()->getObjectTypeNumber() == FlagTypeNumber)
         return true;
   }
   return false;
}


void RabbitGameType::idle(BfObject::IdleCallPath path, U32 deltaT)
{
   Parent::idle(path, deltaT);
   if(path != BfObject::ServerIdleMainLoop)
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

               broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagReturn, returnString, Vector<StringTableEntry>());
            }
         }
      }
   }
}


void RabbitGameType::controlObjectForClientKilled(ClientInfo *theClient, BfObject *clientObject, BfObject *killerObject)
{
   Parent::controlObjectForClientKilled(theClient, clientObject, killerObject);

   Ship *killerShip = NULL;
   ClientInfo *ko = killerObject->getOwner();

   if(ko)
      killerShip = ko->getShip();

   Ship *victimShip = NULL;
   if(isShipType(clientObject->getObjectTypeNumber()))
      victimShip = static_cast<Ship *>(clientObject);

   if(killerShip)
   {
      if(shipHasFlag(killerShip))
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

   if(!ship->getClientInfo())
      return;

   s2cRabbitMessage(RabbitMsgGrab, ship->getClientInfo()->getName());
   flag->mTimer.reset(mFlagScoreTimer);

   flag->mountToShip(ship);

   ship->getClientInfo()->getStatistics()->mFlagPickup++;
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

   if(item->getObjectTypeNumber() != FlagTypeNumber)
      return;

   FlagItem *flag = static_cast<FlagItem *>(item);

   if(ship->getClientInfo())
   {
      flag->mTimer.reset(mFlagReturnTimer);
      s2cRabbitMessage(RabbitMsgDrop, ship->getClientInfo()->getName());

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
   s2cRabbitMessage(RabbitMsgRabbitKill, rabbitShip->getClientInfo()->getName());
   updateScore(rabbitShip, RabbitKills);  
}


void RabbitGameType::onFlaggerDead(Ship *killerShip)
{
   s2cRabbitMessage(RabbitMsgRabbitDead, killerShip->getClientInfo()->getName());
   updateScore(killerShip, RabbitKilled); 
}


// What does a particular scoring event score?
S32 RabbitGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
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
   else  // scoreGroup == IndividualScore
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


GameTypeId RabbitGameType::getGameTypeId() const { return RabbitGame; }

const char *RabbitGameType::getShortName()         const { return "Rab"; }
const char *RabbitGameType::getInstructionString() const { return "Grab the flag and hold it for as long as you can!"; }

bool RabbitGameType::isFlagGame()          const { return true; }
bool RabbitGameType::isTeamFlagGame()      const { return true;  }
bool RabbitGameType::canBeTeamGame()       const { return true;  }
bool RabbitGameType::canBeIndividualGame() const { return true;  }


bool RabbitGameType::isSpawnWithLoadoutGame()
{
   return true;
}


};  //namespace Zap


