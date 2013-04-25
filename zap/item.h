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

#ifndef _ITEM_H_
#define _ITEM_H_

#include "BfObject.h"               // Parent class
#include "EditorObject.h"           // Parent class
#include "LuaScriptRunner.h"        // Parent class

#include "Timer.h"

namespace Zap
{

// A note on terminology here: an "object" is any game object, whereas an "item" is a point object that the player will interact with
// Item is now parent class of MoveItem, EngineeredItem, PickupItem

class Item : public PointObject
{
   typedef PointObject Parent;

private:
   U16 mItemId;                     // Item ID, shared between client and server

protected:
   F32 mRadius;
   Vector<Point> mOutlinePoints;    // Points representing an outline of the item, recalculated when position set

   enum MaskBits {
      InitialMask     = Parent::FirstFreeMask << 0,
      ItemChangedMask = Parent::FirstFreeMask << 1,
      ExplodedMask    = Parent::FirstFreeMask << 2,
      FirstFreeMask   = Parent::FirstFreeMask << 3
   };

   static bool mInitial;     // True on initial unpack, false thereafter

public:
   explicit Item(F32 radius = 1);   // Constructor
   virtual ~Item();                 // Destructor

   virtual bool getCollisionCircle(U32 stateIndex, Point &point, F32 &radius) const;

   virtual bool processArguments(S32 argc, const char **argv, Game *game);

   virtual U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);

   F32 getRadius();
   virtual void setRadius(F32 radius);

   virtual void setPos(const Point &pos);
   virtual void setPos(lua_State *L, S32 stackIndex);

   virtual void renderItem(const Point &pos);      // Generic renderer -- will be overridden
   virtual void render();

   U16 getItemId();
   void setItemId(U16 id);

   // Editor interface
   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   virtual F32 getEditorRadius(F32 currentScale);
   virtual string toLevelCode(F32 gridSize) const;

   virtual Rect calcExtents(); 

   // LuaItem interface

   LUAW_DECLARE_CLASS(Item);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   virtual S32 lua_getRad(lua_State *L);
   virtual S32 lua_isInCaptureZone(lua_State *L);      // Non-moving item is never in capture zone, even if it is!
   virtual S32 lua_getCaptureZone(lua_State *L);
   virtual S32 lua_getShip(lua_State *L);
};


};

#endif


