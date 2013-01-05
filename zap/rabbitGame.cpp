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


const Color *RabbitGameType::getTeamColor(const BfObject *object) const
{
#ifndef ZAP_DEDICATED
   // Neutral flags are orange in Rabbit
   if(object->getObjectTypeNumber() == FlagTypeNumber && object->getTeam() == TEAM_NEUTRAL)
      return &Colors::orange50;  

   // In team game, ships use team color
   if(object->getObjectTypeNumber() == PlayerShipTypeNumber && !isTeamGame())
   {
      Ship *localShip = static_cast<ClientGame *>(getGame())->getLocalShip(); // (getLocalShip can return NULL)
      
      if(object == localShip)                            // Players always appear green to themselves
         return &Colors::green;

      const Ship *ship = static_cast<const Ship *>(object);

      if(shipHasFlag(ship) || shipHasFlag(localShip))    // If a ship has the flag, it's red; if we have the flag, others are red
         return &Colors::red;

      return &Colors::green;                             // All others are green
   }
#endif

   return Parent::getTeamColor(object);
}


bool RabbitGameType::shipHasFlag(const Ship *ship) const
{
   return ship && ship->isCarryingItem(FlagTypeNumber);
}


void RabbitGameType::idle(BfObject::IdleCallPath path, U32 deltaT)
{
   Parent::idle(path, deltaT);
   
   if(path != BfObject::ServerIdleMainLoop)
      return;

   // Server only from here on
   const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);

   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *mRabbitFlag = static_cast<FlagItem *>(flags->get(i));

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


void RabbitGameType::controlObjectForClientKilled(ClientInfo *theClient, BfObject *clientObject, BfObject *killerObject)
{
   if(isGameOver())  // Avoid flooding messages on game over.
      return;

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
   if(ship->isCarryingItem(FlagTypeNumber))
      return;

   if(flag->getTeam() != ship->getTeam() && flag->getTeam() != TEAM_NEUTRAL)
      return;

   if(!ship->getClientInfo())
            return;

   if(!isGameOver())  // Avoid flooding messages when game is over
      s2cRabbitMessage(RabbitMsgGrab, ship->getClientInfo()->getName());

   flag->mTimer.reset(mFlagScoreTimer);

   flag->mountToShip(ship);

   ship->getClientInfo()->getStatistics()->mFlagPickup++;
}


bool RabbitGameType::teamHasFlag(S32 teamIndex) const
{
   return doTeamHasFlag(teamIndex);
}


void RabbitGameType::onFlagMounted(S32 teamIndex)
{
   getGame()->setTeamHasFlag(teamIndex, true);
   notifyClientsWhoHasTheFlag();
}


void RabbitGameType::itemDropped(Ship *ship, MoveItem *item)
{
   TNLAssert(isServer(), "Server only method!");

   if(item->getObjectTypeNumber() == FlagTypeNumber)
   {

      FlagItem *flag = static_cast<FlagItem *>(item);

      if(ship->getClientInfo())
      {
         flag->mTimer.reset(mFlagReturnTimer);
         if(!isGameOver())  // Avoid flooding messages on game over.
            s2cRabbitMessage(RabbitMsgDrop, ship->getClientInfo()->getName());

         Point vel = ship->getActualVel();

         flag->setActualVel(vel);
      }

      updateWhichTeamsHaveFlags();
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
   if(!isGameOver())  // Avoid flooding messages on game over.
      s2cRabbitMessage(RabbitMsgRabbitKill, rabbitShip->getClientInfo()->getName());
   updateScore(rabbitShip, RabbitKills);  
}


void RabbitGameType::onFlaggerDead(Ship *killerShip)
{
   if(!isGameOver())  // Avoid flooding messages on game over.
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
bool RabbitGameType::canBeTeamGame()       const { return true;  }
bool RabbitGameType::canBeIndividualGame() const { return true;  }


bool RabbitGameType::isSpawnWithLoadoutGame()
{
   return true;
}


};  //namespace Zap


