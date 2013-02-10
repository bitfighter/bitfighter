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

#ifndef _FLAGITEM_H_
#define _FLAGITEM_H_

#include "ship.h"
#include "LuaWrapper.h"

namespace Zap
{
////////////////////////////////////////
////////////////////////////////////////


// Forward declarations
class AbstractSpawn;
class GoalZone;

class FlagItem : public MountableItem
{
   typedef MountableItem Parent;

private:
   Point mInitialPos;                  // Where flag was "born"
   bool mIsAtHome;

   SafePtr<GoalZone> mZone;            // GoalZone currently holding the flag, NULL if not in a zone

   void removeOccupiedSpawnPoints(Vector<AbstractSpawn *> &spawnPoints);


protected:
   enum MaskBits {
      ZoneMask         = Parent::FirstFreeMask << 0,
      FirstFreeMask    = Parent::FirstFreeMask << 1
   };

public:
   FlagItem(lua_State *L = NULL);                                              // Combined Lua / C++ default constructor
   FlagItem(const Point &pos, bool collidable, float radius, float mass);      // Alternate C++ constructor
   FlagItem(const Point &pos, const Point &vel, bool useDropDelay = false);    // Alternate alternate C++ constructor
   virtual ~FlagItem();                                                        // Destructor

   FlagItem *clone() const;
   void copyAttrs(FlagItem *target);

   void initialize();      // Set inital values of things

   virtual bool processArguments(S32 argc, const char **argv, Game *game);
   virtual string toLevelCode(F32 gridSize) const;

   virtual void onAddedToGame(Game *theGame);
   virtual void renderItem(const Point &pos);
   virtual void renderItemAlpha(const Point &pos, F32 alpha);

   void mountToShip(Ship *theShip);

   virtual void sendHome();

   virtual bool collide(BfObject *hitObject);
   void dismount(DismountMode dismountMode);

   TestFunc collideTypes();

   bool isAtHome();

   Timer mTimer;                       // Used for games like HTF where time a flag is held is important

   virtual U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);
   virtual void idle(BfObject::IdleCallPath path);

   // For tracking GoalZones where the flag might be at the moment
   void setZone(GoalZone *goalZone);
   GoalZone *getZone();
   bool isInZone();

   // Methods that really only apply to NexusFlagItems; having them here lets us get rid of a bunch of dynamic_casts
   virtual void changeFlagCount(U32 change);
   virtual U32 getFlagCount();


   TNL_DECLARE_CLASS(FlagItem);

   ///// Editor stuff

   void renderDock();
   F32 getEditorRadius(F32 currentScale);

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();


   ///// Lua Interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(FlagItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
   
   S32 lua_isInInitLoc(lua_State *L);      // Is flag in it's initial location?

   // Override some parent methods
   S32 lua_isInCaptureZone(lua_State *L);
   S32 lua_getCaptureZone(lua_State *L);   
};


};

#endif


