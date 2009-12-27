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
   switch (msgIndex)
   {
   case RabbitMsgGrab:
      SFXObject::play(SFXFlagCapture);
      gGameUserInterface.displayMessage(Color(1.0f, 0.0f, 0.0f),
                  "%s GRABBED the Carrot!",
                  clientName.getString());
      break;
   case RabbitMsgRabbitKill:
      SFXObject::play(SFXShipHeal);
      gGameUserInterface.displayMessage(Color(1.0f, 0.0f, 0.0f),
                  "%s is a rabbid rabbit!",
                  clientName.getString());
      break;
   case RabbitMsgDrop:
      SFXObject::play(SFXFlagDrop);
      gGameUserInterface.displayMessage(Color(0.0f, 1.0f, 0.0f),
                  "%s DROPPED the Carrot!",
                  clientName.getString());
      break;
   case RabbitMsgRabbitDead:
      SFXObject::play(SFXShipExplode);
      gGameUserInterface.displayMessage(Color(1.0f, 0.0f, 0.0f),
                  "%s killed the rabbit!",
                  clientName.getString());
      break;
   case RabbitMsgReturn:
      SFXObject::play(SFXFlagReturn);
      gGameUserInterface.displayMessage(Color(1.0f, 0.0f, 1.0f),
                  "The Carrot has been returned!");
      break;
   case RabbitMsgGameOverWin:
      gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.0f),
                  "%s is the top rabbit!",
                  clientName.getString());
      break;
   case RabbitMsgGameOverTie:
      gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.0f),
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

   mFlagReturnTimer = Timer(atoi(argv[2]) * 1000);
   mFlagScoreTimer = Timer((U32)(1.0f / atoi(argv[3]) * 60 * 1000)); //secs per point

   return true;
}

// Describe the arguments processed above...
Vector<GameType::ParameterDescription> RabbitGameType::describeArguments()
{
   Vector<GameType::ParameterDescription> descr;

   GameType::ParameterDescription item;
   item.name = "Game Time:";
   item.help = "Time game will last";
   item.value = 8;
   item.units = "mins";
   item.minval = 1;
   item.maxval = 99;
   descr.push_back(item);

   item.name = "Score to Win:";
   item.help = "Game ends when one team gets this score";
   item.value = 100;
   item.units = "points";
   item.minval = 5;
   item.maxval = 500;
   descr.push_back(item);

   item.name = "Flag Return Timer:";
   item.help = "What is this??";
   item.value = 10;
   item.units = "secs";
   item.minval = 1;
   item.maxval = 99;
   descr.push_back(item);

   item.name = "Point Earn Rate:";
   item.help = "Time you must hold the flag to earn a point";
   item.value = 30;
   item.units = "points per minute";
   item.minval = 1;
   item.maxval = 99;
   descr.push_back(item);

   return descr;
}

bool RabbitGameType::objectCanDamageObject(GameObject *damager, GameObject *victim)
{
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

   // Hunters can only hurt rabbits and only rabbits can hurt hunters -- no "friendly fire"
   return shipHasFlag(attackShip) != shipHasFlag(victimShip);
}


// Works for ships and robots!  --> or does it?  Was a template, but it wasn't working for regular ships, haven't tested with robots
Color RabbitGameType::getShipColor(Ship *s)
{
   GameConnection *gc = gClientGame->getConnectionToServer();
   if(!gc)
      return Color();
   Ship *co = dynamic_cast<Ship *>(gc->getControlObject());

   if(s == co || shipHasFlag(s) == shipHasFlag(co))
      return Color(0,1,0);
   // else
   return Color(1,0,0);
}

Color RabbitGameType::getTeamColor(S32 team)
{
   return Color(1, 0.5, 0);      // Orange
}

bool RabbitGameType::shipHasFlag(Ship *ship)
{
   if (!ship)
      return false;

   for (S32 k = 0; k < ship->mMountedItems.size(); k++)
   {
      if (dynamic_cast<FlagItem *>(ship->mMountedItems[k].getPointer()))
         return true;
   }
   return false;
}


void RabbitGameType::idle(GameObject::IdleCallPath path)
{
   Parent::idle(path);
   if(path != GameObject::ServerIdleMainLoop || !mRabbitFlag)
      return;

   U32 deltaT = mCurrentMove.time;

   if (mRabbitFlag->isMounted())
   {
      if (mFlagScoreTimer.update(deltaT))
      {
         onFlagHeld(mRabbitFlag->getMount());
         mFlagScoreTimer.reset();
      }
   }
   else
   {
      if (!mRabbitFlag->isAtHome() && mFlagReturnTimer.update(deltaT))
      {
         mFlagReturnTimer.reset();
         mRabbitFlag->sendHome();
         static StringTableEntry returnString("The carrot has been returned!");
         for (S32 i = 0; i < mClientList.size(); i++)
            mClientList[i]->clientConnection->s2cDisplayMessageE( GameConnection::ColorNuclearGreen, SFXFlagReturn, returnString, Vector<StringTableEntry>() );
      }
   }
   Parent::idle(path);
}

void RabbitGameType::controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject)
{
   Parent::controlObjectForClientKilled(theClient, clientObject, killerObject);

   Ship *killerShip = NULL;
   GameConnection *ko = killerObject->getOwner();
   if(ko)
      killerShip = dynamic_cast<Ship *>(ko->getControlObject());

   Ship *victimShip = dynamic_cast<Ship *>(clientObject);

   if (killerShip)
   {
      if (shipHasFlag(killerShip))
      {
         // Rabbit killed another person
         onFlaggerKill(killerShip);
      }
      else if (shipHasFlag(victimShip))
      {
         // Someone killed the rabbit!  Poor rabbit!
         onFlaggerDead(killerShip);
      }
   }
}


void RabbitGameType::shipTouchFlag(Ship *ship, FlagItem *flag)
{
   s2cRabbitMessage(RabbitMsgGrab, ship->getName());

   flag->mountToShip(ship);
}


void RabbitGameType::flagDropped(Ship *theShip, FlagItem *theFlag)
{
   mFlagScoreTimer.reset();
   mFlagReturnTimer.reset();
   s2cRabbitMessage(RabbitMsgDrop, theShip->getName());

   Point vel = theShip->getActualVel();

   //// Add a random vector to the flag
   //F32 th = TNL::Random::readF() * Float2Pi;
   //F32 f = (TNL::Random::readF() * 2 - 1) * 100;
   //Point dvel(cos(th) * f, sin(th) * f);
   //vel += dvel;

   theFlag->setActualVel(vel);
}


void RabbitGameType::onFlagHeld(Ship *ship)
{
   updateScore(ship, RabbitHoldsFlag);    // Event: RabbitHoldsFlag
}


void RabbitGameType::addFlag(FlagItem *theFlag)
{
   mRabbitFlag = theFlag;
   theFlag->setScopeAlways();
}

// Rabbit killed another ship
void RabbitGameType::onFlaggerKill(Ship *rabbitShip)
{
   s2cRabbitMessage(RabbitMsgRabbitKill, rabbitShip->getName());
   updateScore(rabbitShip, RabbitKills);      // Event: RabbitKills
}

void RabbitGameType::onFlaggerDead(Ship *killerShip)
{
   s2cRabbitMessage(RabbitMsgRabbitDead, killerShip->getName());
   updateScore(killerShip, RabbitKilled);     // Event: RabbitKilled
}


// What does a particular scoring event score?
S32 RabbitGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
{
   // In general, there are no team scores in Rabbit games...
   if(scoreGroup == TeamScore)
   {
      return naScore;
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



};  //namespace Zap

