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


class FlagSpawn;
class GoalZone;

class FlagItem : public MoveItem
{
private:
   Point mInitialPos;                 // Where flag was "born"
   bool mIsAtHome;

   SafePtr<GoalZone> mZone;            // GoalZone currently holding the flag, NULL if not in a zone

   const Vector<FlagSpawn> *getSpawnPoints();

protected:
   typedef MoveItem Parent;

   enum MaskBits {
      ZoneMask         = Parent::FirstFreeMask << 0,
      FirstFreeMask    = Parent::FirstFreeMask << 1
   };

public:
   FlagItem(Point pos = Point());                                    // C++ constructor
   FlagItem(Point pos, bool collidable, float radius, float mass);   // Alternate C++ constructor
   FlagItem(Point pos, Point vel, bool useDropDelay = false);        // Alternate alternate C++ constructor
   ~FlagItem();                                                      // Destructor

   FlagItem *clone() const;
   void copyAttrs(FlagItem *target);

   void initialize();      // Set inital values of things

   virtual bool processArguments(S32 argc, const char **argv, Game *game);
   virtual string toString(F32 gridSize) const;

   virtual void onAddedToGame(Game *theGame);
   virtual void renderItem(const Point &pos);

   void mountToShip(Ship *theShip);

   virtual void sendHome();

   virtual void onMountDestroyed();
   virtual bool collide(BfObject *hitObject);

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
   LUAW_DECLARE_CLASS(FlagItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
   
   S32 isInInitLoc(lua_State *L);      // Is flag in it's initial location?

   // Override some parent methods
   S32 isInCaptureZone(lua_State *L);
   S32 getCaptureZone(lua_State *L);   
};


};

#endif


