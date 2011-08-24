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

private:
   Vector<GoalZone *> mGoals;
   SafePtr<SoccerBallItem> mBall;
   bool mSoccerPickupAllowed;


public:
   SoccerGameType();
   bool processSpecialsParam(const char *param);
   string getSpecialsLine();

   void scoreGoal(Ship *ship, StringTableEntry lastPlayerTouchName, S32 lastPlayerTouchTeam, S32 goalTeamIndex, S32 score);
   void updateSoccerScore(Ship *ship, S32 scoringTeam, ScoringEvent scoringEvent, S32 score);   // Helper function to make sure the two-arg version of updateScore doesn't get a null ship

   bool isSoccerPickupAllowed() { return mSoccerPickupAllowed; }
   void setSoccerPickupAllowed(bool allowed) { mSoccerPickupAllowed = allowed; }

   void addZone(GoalZone *theZone);
   void itemDropped(Ship *ship, MoveItem *item);
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
   const char *getGameTypeString() const { return "Soccer"; }
   const char *getShortName() const { return "S"; }
   virtual const char *getInstructionString() { return "Push the ball into the opposing team's goal!"; }
   //bool isTeamGame() { return getGame()->getTeamCount() > 1; } 
   bool canBeTeamGame() { return true; }
   bool canBeIndividualGame() { return true; }

   void shipTouchZone(Ship *s, GoalZone *z);

   bool onFire(Ship *ship);             
   // No modules while carrying the ball!
   bool okToUseModules(Ship *ship) { return Parent::okToUseModules(ship) && ! ship->isCarryingItem(SoccerBallItemTypeNumber); }

   TNL_DECLARE_RPC(s2cSoccerScoreMessage, (U32 msgIndex, StringTableEntry clientName, RangedU32<0, GameType::gMaxTeamCount> teamIndex));
   TNL_DECLARE_CLASS(SoccerGameType);
};


////////////////////////////////////////
////////////////////////////////////////

class SoccerBallItem : public MoveItem
{
   typedef MoveItem Parent;      // TODO: Should be EditorItem???

private:
   Point initialPos;
   Timer mSendHomeTimer;
   SafePtr<Ship> mLastPlayerTouch;
   SafePtr<Ship> mLastPlayerMounted;
   S32 mLastPlayerTouchTeam;
   StringTableEntry mLastPlayerTouchName;
   F32 mDragFactor;

public:
   bool mAllowPickup;
   U32 mPickupTime;
   SoccerBallItem(Point pos = Point());   // C++ constructor
   SoccerBallItem *clone() const;

   static const S32 SOCCER_BALL_RADIUS = 30;

   void renderItem(const Point &pos);
   void sendHome();
   void damageObject(DamageInfo *theInfo);
   void idle(GameObject::IdleCallPath path);
   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;

   void onAddedToGame(Game *theGame);
   void onItemDropped();

   bool collide(GameObject *hitObject);

   TNL_DECLARE_CLASS(SoccerBallItem);

   ///// Editor methods
   const char *getEditorHelpString() { return "Soccer ball, can only be used in Soccer games."; }
   const char *getPrettyNamePlural() { return "SoccerBalls"; }
   const char *getOnDockName() { return "Ball"; }
   const char *getOnScreenName() { return "SoccerBall"; }
   bool hasTeam() { return false; }
   bool canBeHostile() { return false; }
   bool canBeNeutral() { return false; }

   void renderDock();
   void renderEditor(F32 currentScale);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   ///// Lua Interface

   SoccerBallItem(lua_State *L) { /* Do nothing */ };   //  Lua constructor

   static const char className[];
   static Lunar<SoccerBallItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, SoccerBallItemTypeNumber); }
   void push(lua_State *L) {  Lunar<SoccerBallItem>::push(L, this); }   

};

};


#endif


