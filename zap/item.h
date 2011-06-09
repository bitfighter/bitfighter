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

#include "moveObject.h"       // Parent class
#include "EditorObject.h"     // Parent class
#include "luaObject.h"        // Parent class

#include"goalZone.h"          // For GoalZone def
#include "timer.h"

namespace Zap
{

class Ship;
class GameType;

////////////////////////////////////////
////////////////////////////////////////

class Item : public MoveObject, public LuaItem
{
   typedef MoveObject Parent;

private:
   F32 updateTimer;
   Point prevMoveVelocity;

protected:
   enum MaskBits {
      InitialMask = BIT(0),
      PositionMask = BIT(1),     // <-- Indicates position has changed and needs to be updated
      WarpPositionMask = BIT(2),
      MountMask = BIT(3),
      ZoneMask = BIT(4),
      ItemChangedMask = BIT(5),
      ExplodedMask = BIT(6),
      FirstFreeMask = BIT(7),
   };

   SafePtr<Ship> mMount;
   SafePtr<GoalZone> mZone;

   bool mIsMounted;
   bool mIsCollideable;
   bool mInitial;       // True on initial unpack, false thereafter

   U16 mItemId;         // Item ID, shared between client and server

   Timer mDroppedTimer;                   // Make flags have a tiny bit of delay before they can be picked up again
   static const U32 DROP_DELAY = 500;     // Time until we can pick the item up after it's dropped (in ms)

public:
   Item(Point p = Point(0,0), bool collideable = false, float radius = 1, float mass = 1);   // Constructor

   void idle(GameObject::IdleCallPath path);

   bool processArguments(S32 argc, const char **argv, Game *game);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void setActualPos(const Point &p);
   void setActualVel(const Point &vel);

   U16 getItemId() { return mItemId; }

   virtual void mountToShip(Ship *theShip);
   void setMountedMask() { setMaskBits(MountMask); }
   void setPositionMask() { setMaskBits(PositionMask); }

   bool isMounted() { return mIsMounted; }
   virtual bool isItemThatMakesYouVisibleWhileCloaked() { return true; }      // HuntersFlagItem overrides to false

   void setZone(GoalZone *theZone);
   GoalZone *getZone() { return mZone; }
   bool isInZone() { return mZone == NULL; }
   void setCollideable(bool isCollideable) { mIsCollideable = isCollideable; }

   Ship *getMount();
   void dismount();
   void render();
   virtual void renderItem(Point pos) = 0;

   virtual void onMountDestroyed();
   virtual void onItemDropped();

   bool collide(GameObject *otherObject);

   static const S32 TEAM_NEUTRAL = -1;
   static const S32 TEAM_HOSTILE = -2;
   static const S32 NO_TEAM = -3;      // Not exposed to lua, not used in level files, only used internally

   // LuaItem interface
   S32 getLoc(lua_State *L) { return LuaObject::returnPoint(L, getActualPos()); }
   S32 getRad(lua_State *L) { return LuaObject::returnFloat(L, getRadius()); }
   S32 getVel(lua_State *L) { return LuaObject::returnPoint(L, getActualVel()); }
   virtual S32 getTeamIndx(lua_State *L) { return TEAM_NEUTRAL + 1; }              // Can be overridden for team items
   S32 isInCaptureZone(lua_State *L) { return returnBool(L, mZone.isValid()); }    // Is flag in a team's capture zone?
   S32 isOnShip(lua_State *L) { return returnBool(L, mIsMounted); }                // Is flag being carried by a ship?
	S32 getCaptureZone(lua_State *L);
	S32 getShip(lua_State *L);
   GameObject *getGameObject() { return this; }
};

////////////////////////////////////////
////////////////////////////////////////

// Class with editor methods related to point things

class EditorPointObject : public EditorObject
{
   typedef EditorObject Parent;

public:
   EditorPointObject(GameObjectType objectType = UnknownType);       // Constructor
   EditorPointObject(const EditorPointObject &epo);                  // Copy constructor

   virtual void renderItemText(const char *text, S32 offset, F32 currentScale);
   void addToDock(Game *game, const Point &point);
};


////////////////////////////////////////
////////////////////////////////////////

class EditorItem : public Item, virtual public EditorPointObject
{
   typedef Item Parent;   
   typedef EditorObject EditorParent;

protected:
   F32 getEditorRenderScaleFactor(F32 currentScale);     // Calculates scaling factor for items in the editor

public:
   EditorItem(Point p = Point(0,0), bool collideable = false, float radius = 1, float mass = 1);   // Constructor

   // Some properties about the item that will be needed in the editor
   string toString();

   virtual void renderEditor(F32 currentScale);
   virtual F32 getEditorRadius(F32 currentScale);
};


////////////////////////////////////////
////////////////////////////////////////

class PickupItem : public EditorItem
{
private:
   typedef Item Parent;
   bool mIsVisible;
   bool mIsMomentarilyVisible;      // Used if item briefly flashes on and off, like if a ship is sitting on a repair item when it reappears
   Timer mRepopTimer;
   S32 mRepopDelay;


protected:
   enum MaskBits {
      PickupMask = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1,
   };

public:
   PickupItem(Point p = Point(), float radius = 1, S32 repopDelay = 20000);      // Constructor

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString();

   void idle(GameObject::IdleCallPath path);
   bool isVisible() { return mIsVisible; }

   U32 getRepopDelay() { return mRepopDelay; }


   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool collide(GameObject *otherObject);
   virtual bool pickup(Ship *theShip) = 0;
   virtual void onClientPickup() = 0;
};

};

#endif


