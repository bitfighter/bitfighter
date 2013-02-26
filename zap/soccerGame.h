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
#include "LuaWrapper.h"

namespace Zap
{

class Ship;
class SoccerBallItem;

class SoccerGameType : public GameType
{
   typedef GameType Parent;

private:
   SafePtr<SoccerBallItem> mBall;

public:
   SoccerGameType();

   void scoreGoal(Ship *ship, StringTableEntry lastPlayerTouchName, S32 lastPlayerTouchTeam, S32 goalTeamIndex, S32 score);
   void updateSoccerScore(Ship *ship, S32 scoringTeam, ScoringEvent scoringEvent, S32 score);   // Helper function to make sure the two-arg version of updateScore doesn't get a null ship

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

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   virtual const char *getInstructionString() const;

   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;

   TNL_DECLARE_RPC(s2cSoccerScoreMessage, (U32 msgIndex, StringTableEntry clientName, RangedU32<0, GameType::gMaxTeamCount> teamIndex));
   TNL_DECLARE_CLASS(SoccerGameType);
};


////////////////////////////////////////
////////////////////////////////////////

class SoccerBallItem : public MoveItem
{
   typedef MoveItem Parent;      // TODO: Should be PointObject???

private:
   Point mInitialPos;
   Timer mSendHomeTimer;
   S32 mLastPlayerTouchTeam;
   F32 mDragFactor;
   StringTableEntry mLastPlayerTouchName;
   SafePtr<Ship> mLastPlayerTouch;
   bool mLuaBall;

public:
   explicit SoccerBallItem(lua_State *L = NULL);      // Combined Lua / C++ default constructor
   virtual ~SoccerBallItem();                // Destructor

   SoccerBallItem *clone() const;

   static const S32 SOCCER_BALL_RADIUS = 30;

   void renderItem(const Point &pos);
   void resetPlayerTouch();
   void sendHome();
   void damageObject(DamageInfo *theInfo);
   void idle(BfObject::IdleCallPath path);
   bool processArguments(S32 argc, const char **argv, Game *game);
   string toLevelCode(F32 gridSize) const;

   void onAddedToGame(Game *game);

   bool collide(BfObject *hitObject);

   TNL_DECLARE_CLASS(SoccerBallItem);

   ///// Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   void renderDock();
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);

   const Color *getColor() const;


   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   ///// Lua Interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(SoccerBallItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

};

};


#endif


