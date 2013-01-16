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

#ifndef _TELEPORTER_H_
#define _TELEPORTER_H_

#include "SimpleLine.h"    // For SimpleLine def
#include "gameConnection.h"
#include "BfObject.h"
#include "projectile.h"    // For LuaItem
#include "Point.h"
#include "Colors.h"
#include "Engineerable.h"

#include "tnlNetObject.h"

namespace Zap
{

class Teleporter;

// Manage destinations for a teleporter
struct DestManager 
{
   private:
      Vector<Point> mDests;
      Teleporter *mOwner;

   public:
      void setOwner(Teleporter *owner);

      S32 getDestCount() const;
      Point getDest(S32 index) const;
      S32 getRandomDest() const;

      void addDest(const Point &dest);
      void setDest(S32 index, const Point &dest);
      void delDest(S32 index);

      void resize(S32 count);
      void read(S32 index, BitStream *stream);     // Read a single dest
      void read(BitStream *stream);                // Read a whole list of dests

      void clear();
      /*const*/ Vector<Point> *getDestList() /*const*/;
};


////////////////////////////////////////
////////////////////////////////////////

class Teleporter : public SimpleLine, public Engineerable
{
   typedef SimpleLine Parent;

public:
   enum {
      InitMask      = Parent::FirstFreeMask << 0,
      TeleportMask  = Parent::FirstFreeMask << 1,
      HealthMask    = Parent::FirstFreeMask << 2,
      DestroyedMask = Parent::FirstFreeMask << 3,
      FirstFreeMask = Parent::FirstFreeMask << 4,

      TeleporterCooldown = 1500,             // Time teleporter remains idle after it has been used
      TeleporterExpandTime = 1350,
      TeleportInExpandTime = 750,
      TeleportInRadius = 120,

      TeleporterExplosionTime = 1000,
   };

   static const S32 TELEPORTER_RADIUS = 75;  // Overall size of the teleporter -- this is in fact a radius!

private:
   S32 mLastDest;    // Destination of last ship through

   bool mHasExploded;
   F32 mStartingHealth;

   Timer mExplosionTimer;
   Timer mTeleportCooldown;

   bool mFinalExplosionTriggered;

   DestManager mDestManager;

   SafePtr<Ship> mEngineeringShip;

   void initialize(const Point &pos, const Point &dest, Ship *engineeringShip);
   void doSetGeom(lua_State *L);    // Helper
   void computeExtent();

public:
   Teleporter(lua_State *L = NULL);                                           // Combined default C++/Lua constructor
   Teleporter(const Point &pos, const Point &dest, Ship *engineeringShip);    // Constructor used by engineer
   virtual ~Teleporter();                                                     // Destructor

   Teleporter *clone() const;

   U32 mTime;

   U32 mTeleporterCooldown;

   // Destination management
   S32 getDestCount();
   Point getDest(S32 index);
   void addDest(const Point &dest);
   void delDest(S32 index);


   void newObjectFromDock(F32 gridSize);

   static bool checkDeploymentPosition(const Point &position, GridDatabase *gb, Ship *ship);

   virtual bool processArguments(S32 argc, const char **argv, Game *game);
   string toLevelCode(F32 gridSize) const;
   Rect calcExtents();

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void damageObject(DamageInfo *theInfo);
   void onDestroyed();
   bool collide(BfObject *otherObject);

   const Vector<Point> *getCollisionPoly() const;
   bool getCollisionCircle(U32 state, Point &center, F32 &radius) const;

   void idle(BfObject::IdleCallPath path);
   void render();

#ifndef ZAP_DEDICATED
   void doExplosion();
#endif

   void onAddedToGame(Game *theGame);

   TNL_DECLARE_CLASS(Teleporter);

   TNL_DECLARE_RPC(s2cAddDestination, (Point));   
   TNL_DECLARE_RPC(s2cClearDestinations, ());   


   ///// Editor Methods
   Color getEditorRenderColor();

   void renderEditorItem();

   void onAttrsChanging();
   void onGeomChanging();
   void onGeomChanged();

   void onConstructed();

   bool hasAnyDests();
   void setEndpoint(const Point &point);

   // Some properties about the item that will be needed in the editor
   const char *getOnScreenName();
   const char *getOnDockName();
   const char *getPrettyNamePlural();
   const char *getEditorHelpString();

   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();


   ///// Lua Interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(Teleporter);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 addDest(lua_State *L);
   S32 delDest(lua_State *L);
   S32 clearDests(lua_State *L);
   S32 getDest(lua_State *L);
   S32 getDestCount(lua_State *L);

   // Overrides
   S32 setGeom(lua_State *L);
   S32 getGeom(lua_State *L);
};


};

#endif
