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

#ifndef _GOALZONE_H_
#define _GOALZONE_H_

#include "gameObject.h"
#include "polygon.h"
#include "Timer.h"

namespace Zap
{

class GoalZone : public EditorPolygon
{
   typedef GameObject Parent;

private:
   static const S32 FlashDelay = 500;
   static const S32 FlashCount = 5;

   bool mHasFlag;     // Is there a flag parked in this zone?
   S32 mScore;        // How much is this zone worth?

   S32 mFlashCount;
   Timer mFlashTimer;

protected:
   enum MaskBits {
      InitialMask   = Parent::FirstFreeMask << 0,
      TeamMask      = Parent::FirstFreeMask << 1,
      FirstFreeMask = Parent::FirstFreeMask << 2
   };

public:
   GoalZone();        // Constructor
   GoalZone *clone() const;

   bool processArguments(S32 argc, const char **argv, Game *game);
   
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void idle(GameObject::IdleCallPath path);
   void render();

   bool didRecentlyChangeTeam() { return mFlashCount != 0; }
   S32 getRenderSortValue();

   void setTeam(S32 team);
   void onAddedToGame(Game *theGame);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool collide(GameObject *hitObject);
   
   bool isFlashing() { return mFlashCount & 1; }
   void setFlashCount(S32 i) { mFlashCount = i; }

   S32 getScore() { return mScore; }
   //bool hasFlag() { return mHasFlag; }
   void setHasFlag(bool hasFlag) { mHasFlag = hasFlag; }
   
   TNL_DECLARE_CLASS(GoalZone);

   /////
   // Editor methods
   const char *getEditorHelpString() { return "Target area used in a variety of games."; }
   const char *getPrettyNamePlural() { return "Goal zones"; }
   const char *getOnDockName() { return "Goal"; }
   const char *getOnScreenName() { return "Goal"; }
   string toString(F32 gridSize) const;

   void renderEditor(F32 currentScale);
   void renderDock();

   /////
   // Lua Interface
   GoalZone(lua_State *L) { /* Do nothing */ };   //  Lua constructor

   static const char className[];                 // Class name as it appears to Lua scripts
   static Lunar<GoalZone>::RegType methods[];

   GameObject *getGameObject() { return this; }   // Return the underlying GameObject
   S32 hasFlag(lua_State *L) { return returnBool(L, mHasFlag); }
   S32 getClassID(lua_State *L) { return returnInt(L, GoalZoneTypeNumber); }

//private:
  void push(lua_State *L) {  Lunar<GoalZone>::push(L, this); }
};


};

#endif
