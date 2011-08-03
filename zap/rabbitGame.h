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

#ifndef _RABBITGAME_H_
#define _RABBITGAME_H_

#include "gameType.h"
#include "item.h"
#include "gameWeapons.h"
#include "shipItems.h"

namespace Zap
{
class Ship;

class RabbitGameType : public GameType
{
   typedef GameType Parent;

   U32 mFlagReturnTimer;
   U32 mFlagScoreTimer;

public:

   enum
   {
      RabbitMsgGrab,
      RabbitMsgRabbitKill,
      RabbitMsgRabbitDead,
      RabbitMsgDrop,
      RabbitMsgReturn,
      RabbitMsgGameOverWin,
      RabbitMsgGameOverTie
   };

   RabbitGameType()
   {
      setWinningScore(100);
      mFlagReturnTimer = 30 * 1000;
      mFlagScoreTimer = 5 * 1000;
   }

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString();

   const char **getGameParameterMenuKeys();
   boost::shared_ptr<MenuItem> getMenuItem(Game *game, const char *key);
   bool saveMenuItem(const MenuItem *menuItem, const char *key);

   void idle(GameObject::IdleCallPath path, U32 deltaT);

   void addFlag(FlagItem *flag);
   void itemDropped(Ship *ship, Item *item);
   void shipTouchFlag(Ship *ship, FlagItem *flag);

   bool isFlagGame() { return true; }
   S32 getFlagCount() { return mFlags.size(); }

   bool objectCanDamageObject(GameObject *damager, GameObject *victim);
   void controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject);
   bool shipHasFlag(Ship *ship);
   bool teamHasFlag(S32 team) const;
   const Color *getShipColor(Ship *s);

   Color getTeamColor(S32 team);

   void onFlagHeld(Ship *ship);
   void onFlaggerDead(Ship *killerShip);
   void onFlaggerKill(Ship *rabbitShip);
   void onFlagReturned();

   void setFlagScore(S32 pointsPerMinute);
   S32 getFlagScore() ;

   GameTypes getGameType() { return RabbitGame; }
   const char *getGameTypeString() const { return "Rabbit"; }
   const char *getShortName() const { return "Rab"; }
   const char *getInstructionString() { return "Grab the flag and hold it for as long as you can!"; }
   //bool isTeamGame() { return getGame()->getTeamCount() != 1; }
   bool canBeTeamGame() { return true; }
   bool canBeIndividualGame() { return true; }

   bool isSpawnWithLoadoutGame() { return true; }

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   TNL_DECLARE_RPC(s2cRabbitMessage, (U32 msgIndex, StringTableEntry clientName));
   TNL_DECLARE_CLASS(RabbitGameType);
};

};


#endif


