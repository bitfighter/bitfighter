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


// Base class for things that can be picked up, such as RepairItems and EnergyItems
class PickupItem : public Item
{
   typedef Item Parent;

private:
   bool mIsVisible;
   Timer mRepopTimer;

#ifndef ZAP_DEDICATED
   static EditorAttributeMenuUI *mAttributeMenuUI;      // Menu for text editing; since it's static, don't bother with smart pointer
#endif

protected:
   enum MaskBits {
      PickupMask    = Parent::FirstFreeMask << 0,
      SoundMask     = Parent::FirstFreeMask << 1,
      FirstFreeMask = Parent::FirstFreeMask << 2
   };

   S32 mRepopDelay;            // Period of mRepopTimer, in seconds


public:
   PickupItem(float radius = 1, S32 repopDelay = 20);   // Constructor
   virtual ~PickupItem();                               // Destructor

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toLevelCode(F32 gridSize) const;

   void onAddedToGame(Game *game);
   void idle(BfObject::IdleCallPath path);
   bool isVisible();
   S32 getRenderSortValue();

   U32 getRepopDelay();
   void setRepopDelay(U32 delay);

   const Vector<Point> *getOutline() const;

#ifndef ZAP_DEDICATED
   // These four methods are all that's needed to add an editable attribute to a class...
   EditorAttributeMenuUI *getAttributeMenu();
   void startEditingAttrs(EditorAttributeMenuUI *attributeMenu);    // Called when we start editing to get menus populated
   void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu);     // Called when we're done to retrieve values set by the menu

   virtual void fillAttributesVectors(Vector<string> &keys, Vector<string> &values);
#endif

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool collide(BfObject *otherObject);
   void hide();
   void show();
   virtual bool pickup(Ship *theShip);
   virtual void onClientPickup();

   bool canShowHelpBubble() const;
   //void getOutline();

	///// Lua interface
	LUAW_DECLARE_CLASS(PickupItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_isVis(lua_State *L);
   S32 lua_setVis(lua_State *L);
   S32 lua_setRegenTime(lua_State *L);
   S32 lua_getRegenTime(lua_State *L);
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

   explicit RepairItem(lua_State *L = NULL);    // Combined Lua / C++ default constructor
   virtual ~RepairItem();              // Destructor

   RepairItem *clone() const;

   bool pickup(Ship *theShip);
   void onClientPickup();
   void renderItem(const Point &pos);

   TNL_DECLARE_CLASS(RepairItem);

   ///// Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   Vector<string> *getHelpBubbleText() const;

   virtual S32 getDockRadius();
   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua interface
	LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(RepairItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


////////////////////////////////////////
////////////////////////////////////////

class EnergyItem : public PickupItem
{
private:
   typedef PickupItem Parent;

public:
   static const S32 DEFAULT_RESPAWN_TIME = 20;    // In seconds

   explicit EnergyItem(lua_State *L = NULL);    // Combined Lua / C++ default constructor
   virtual ~EnergyItem();              // Destructor

   EnergyItem *clone() const;

   bool pickup(Ship *theShip);
   void onClientPickup();
   void renderItem(const Point &pos);

   TNL_DECLARE_CLASS(EnergyItem);

   ///// Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();


   ///// Lua interface
	LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(EnergyItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};



};    // namespace

#endif
