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

#include "Zone.h"          // Parent class
#include "BfObject.h"
#include "polygon.h"
#include "Timer.h"

namespace Zap
{

class GoalZone : public GameZone
{
   typedef GameZone Parent;

private:
   static const S32 FlashDelay = 500;
   static const S32 FlashCount = 5;

   bool mHasFlag;     // Is there a flag parked in this zone?
   S32 mScore;        // How much is this zone worth?
   ClientInfo *mCapturer;  // Who captured this zone - used in Zone Control

   S32 mFlashCount;
   Timer mFlashTimer;

protected:
   enum MaskBits {
      InitialMask   = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1
   };

public:
   GoalZone(lua_State *L = NULL);   // Combined Lua / C++ constructor
   ~GoalZone();                     // Destructor

   GoalZone *clone() const;

   bool processArguments(S32 argc, const char **argv, Game *game);
   
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void idle(BfObject::IdleCallPath path);
   void render();

   bool didRecentlyChangeTeam();

   void setTeam(S32 team);
   void onAddedToGame(Game *theGame);
   const Vector<Point> *getCollisionPoly() const;
   bool collide(BfObject *hitObject);
   
   bool isFlashing();
   void setFlashCount(S32 i);

   S32 getScore();
   //bool hasFlag();
   void setHasFlag(bool hasFlag);
   
   ClientInfo *getCapturer();
   void setCapturer(ClientInfo *clientInfo);

   TNL_DECLARE_CLASS(GoalZone);

   /////
   // Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   bool hasTeam();      
   bool canBeHostile(); 
   bool canBeNeutral(); 


   string toString(F32 gridSize) const;

   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   void renderDock();

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(GoalZone);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 hasFlag(lua_State *L);
};


};

#endif
