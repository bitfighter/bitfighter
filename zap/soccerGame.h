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

#ifndef _SOCCERGAME_H_
#define _SOCCERGAME_H_

#include "gameType.h"
#include "item.h"

namespace Zap
{

class Ship;
class SoccerBallItem;

class SoccerGameType : public GameType
{
   typedef GameType Parent;
   Vector<GoalZone *> mGoals;
   SafePtr<SoccerBallItem> mBall;

public:

   void scoreGoal(Ship *ship, S32 goalTeamIndex);
   void addZone(GoalZone *theZone);
   void setBall(SoccerBallItem *theBall);
   void renderInterfaceOverlay(bool scoreboardVisible);

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   // Messages
   enum {
      SoccerMsgScoreGoal,
      SoccerMsgScoreOwnGoal,
      SoccerMsgGameOverTeamWin,
      SoccerMsgGameOverTie,
   };

   GameTypes getGameType() { return SoccerGame; }
   const char *getGameTypeString() { return "Soccer"; }
   virtual const char *getInstructionString() { return "Push the ball into the opposing team's goal."; }
   bool isTeamGame() { return true; }
   bool canBeTeamGame() { return true; }
   bool canBeIndividualGame() { return false; }


   TNL_DECLARE_RPC(s2cSoccerScoreMessage, (U32 msgIndex, StringTableEntry clientName, RangedU32<0, GameType::gMaxTeamCount> teamIndex));
   TNL_DECLARE_CLASS(SoccerGameType);
};

class SoccerBallItem : public Item, LuaObject
{
private:
   typedef Item Parent;
   Point initialPos;
   Timer mSendHomeTimer;
   Ship *mLastPlayerTouch;

public:
   SoccerBallItem(Point pos = Point());   // C++ constructor

   static const S32 radius = 30;          // Radius of soccer ball

   void renderItem(Point pos);
   void sendHome();
   void damageObject(DamageInfo *theInfo);
   void idle(GameObject::IdleCallPath path);
   bool processArguments(S32 argc, const char **argv);
   Vector<GameType::ParameterDescription> describeArguments();

   void onAddedToGame(Game *theGame);

   bool collide(GameObject *hitObject);

   TNL_DECLARE_CLASS(SoccerBallItem);

   ///// Lua Interface

   SoccerBallItem(lua_State *L);             //  Lua constructor

   static const char className[];
   static Lunar<SoccerBallItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, SoccerBallItemType); }

};

};


#endif

