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

namespace Zap
{

class Ship;
class FlagItem;

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

   RabbitGameType();  // Constructor

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString() const;

#ifndef ZAP_DEDICATED
   Vector<string> getGameParameterMenuKeys();
   boost::shared_ptr<MenuItem> getMenuItem(const string &key);
   bool saveMenuItem(const MenuItem *menuItem, const string &key);
#endif

   void idle(BfObject::IdleCallPath path, U32 deltaT);

   void addFlag(FlagItem *flag);
   void itemDropped(Ship *ship, MoveItem *item);
   void shipTouchFlag(Ship *ship, FlagItem *flag);

   S32 getFlagCount();

   bool objectCanDamageObject(BfObject *damager, BfObject *victim);
   void controlObjectForClientKilled(ClientInfo *theClient, BfObject *clientObject, BfObject *killerObject);
   bool shipHasFlag(Ship *ship);

   bool teamHasFlag(S32 teamIndex) const;
   void onFlagMounted(S32 teamIndex);
   void onFlagDismounted();

   const Color *getShipColor(Ship *s);
   const Color *getTeamColor(S32 team, U8 objTypeNumber = UnknownTypeNumber) const;

   void onFlagHeld(Ship *ship);
   void onFlaggerDead(Ship *killerShip);
   void onFlaggerKill(Ship *rabbitShip);
   void onFlagReturned();

   void setFlagScore(S32 pointsPerMinute);
   S32 getFlagScore() const;

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   const char *getInstructionString() const;

   bool isFlagGame() const;
   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;


   bool isSpawnWithLoadoutGame();

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   TNL_DECLARE_RPC(s2cRabbitMessage, (U32 msgIndex, StringTableEntry clientName));
   TNL_DECLARE_CLASS(RabbitGameType);
};

};


#endif


