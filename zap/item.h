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

#include "moveObject.h"
#include "luaObject.h"
#include "timer.h"

namespace Zap
{

class Ship;
class GoalZone;
class GameType;

////////////////////////////////////////
////////////////////////////////////////

class Item : public MoveObject, public LuaItem
{
private:
   void flagDropped();

protected:
   enum MaskBits {
      InitialMask = BIT(0),
      PositionMask = BIT(1),     // <-- Indicates position has changed and needs to be updated
      WarpPositionMask = BIT(2),
      MountMask = BIT(3),
      ZoneMask = BIT(4),
      ItemChangedMask = BIT(5),
      FirstFreeMask = BIT(6),
   };

   SafePtr<Ship> mMount;
   SafePtr<GoalZone> mZone;

   bool mIsMounted;
   bool mIsCollideable;
   bool mInitial;       // True on initial unpack, false thereafter

public:
   Item(Point p = Point(0,0), bool collideable = false, float radius = 1, float mass = 1);   // Constructor

   void idle(GameObject::IdleCallPath path);

   bool processArguments(S32 argc, const char **argv);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void setActualPos(Point p);
   void setActualVel(Point vel);

   void mountToShip(Ship *theShip);

   bool isMounted() { return mIsMounted; }
   void setZone(GoalZone *theZone);
   GoalZone *getZone() { return mZone; }
   bool isInZone() { return mZone == NULL; }
   void setCollideable(bool isCollideable) { mIsCollideable = isCollideable; }

   Ship *getMount();
   void dismount();
   void render();
   virtual void renderItem(Point pos) = 0;

   virtual void onMountDestroyed();
   virtual void onItemDropped(Ship *ship) { /* do nothing */ };

   bool collide(GameObject *otherObject);


   static const S32 NEUTRAL_TEAM = -1;
   static const S32 HOSTILE_TEAM = -2;
   static const S32 NO_TEAM = -3;

   // LuaItem interface
   S32 getLoc(lua_State *L) { return LuaObject::returnPoint(L, getActualPos()); }
   S32 getRad(lua_State *L) { return LuaObject::returnFloat(L, getRadius()); }
   S32 getVel(lua_State *L) { return LuaObject::returnPoint(L, getActualVel()); }
   virtual S32 getTeamIndx(lua_State *L) { return NEUTRAL_TEAM + 1; }     // Can be overridden for team items

   GameObject *getGameObject() { return this; }
};

////////////////////////////////////////
////////////////////////////////////////

class PickupItem : public Item
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

   bool processArguments(S32 argc, const char **argv);

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


