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

#ifndef _PICKUP_ITEM_H_
#define _PICKUP_ITEM_H_

#include "item.h"

namespace Zap
{


class PickupItem : public Item
{
   typedef Item Parent;

private:
   bool mIsVisible;
   bool mIsMomentarilyVisible; // Used if item briefly flashes on and off, like if a ship is sitting on a repair item when it reappears
   Timer mRepopTimer;
   S32 mRepopDelay;            // Period of mRepopTimer

protected:
   enum MaskBits {
      PickupMask    = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1
   };

public:
   PickupItem(Point p = Point(), float radius = 1, S32 repopDelay = 20000);      // Constructor

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;

   void idle(GameObject::IdleCallPath path);
   bool isVisible() { return mIsVisible; }

   U32 getRepopDelay() { return mRepopDelay; }


   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool collide(GameObject *otherObject);
   virtual bool pickup(Ship *theShip) = 0;
   virtual void onClientPickup() = 0;
};


////////////////////////////////////////
////////////////////////////////////////

class RepairItem : public PickupItem
{
protected:
   typedef PickupItem Parent;

public:
   static const S32 DEFAULT_RESPAWN_TIME = 20;    // In seconds
   static const S32 REPAIR_ITEM_RADIUS = 20;

   RepairItem(Point pos = Point());   // Constructor
   RepairItem *clone() const;

   bool pickup(Ship *theShip);
   void onClientPickup();
   void renderItem(const Point &pos);

   TNL_DECLARE_CLASS(RepairItem);

   ///// Editor methods
   const char *getEditorHelpString() { return "Repairs damage to ships. [B]"; }
   const char *getPrettyNamePlural() { return "Repair items"; }
   const char *getOnDockName() { return "Repair"; }
   const char *getOnScreenName() { return "Repair"; }

   virtual S32 getDockRadius() { return 11; }
   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua interface

   RepairItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<RepairItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, RepairItemTypeNumber); }

   S32 isVis(lua_State *L); // Is RepairItem visible? (returns boolean)
   void push(lua_State *L) {  Lunar<RepairItem>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

class EnergyItem : public PickupItem
{
private:
   typedef PickupItem Parent;

public:
   static const S32 DEFAULT_RESPAWN_TIME = 20;    // In seconds

   EnergyItem(Point p = Point());   // Constructor
   EnergyItem *clone() const;

   bool pickup(Ship *theShip);
   void onClientPickup();
   void renderItem(const Point &pos);

   TNL_DECLARE_CLASS(EnergyItem);

   ///// Editor methods
   const char *getEditorHelpString() { return "Restores energy to ships"; }
   const char *getPrettyNamePlural() { return "Energy items"; }
   const char *getOnDockName() { return "Energy"; }
   const char *getOnScreenName() { return "Energy"; }


   ///// Lua Interface

   EnergyItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<EnergyItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, EnergyItemTypeNumber); }

   S32 isVis(lua_State *L); // Is EnergyItem visible? (returns boolean)
   void push(lua_State *L) {  Lunar<EnergyItem>::push(L, this); }
};



};    // namespace

#endif