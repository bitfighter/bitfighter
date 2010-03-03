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

#ifndef _PROJECTILE_H_
#define _PROJECTILE_H_

#include "gameType.h"
#include "gameObject.h"
#include "item.h"
#include "gameWeapons.h"

namespace Zap
{

class Ship;

class LuaProjectile : public LuaItem
{
public:
   LuaProjectile() { /* Do not use */ };            //  C++ constructor

   LuaProjectile(lua_State *L) { /* Do not use */ };            //  Lua constructor

   static const char className[];
   static Lunar<LuaProjectile>::RegType methods[];
   static const char *getClassName() { return "LuaProjectile"; }

   virtual S32 getWeapon(lua_State *L) { TNLAssert(false, "Unimplemented method!"); return 0; };    // Return info about the weapon/projectile

   //============================
   // LuaItem interface

   virtual S32 getClassID(lua_State *L) { return returnInt(L, BulletType); } // Object's class   

   S32 getLoc(lua_State *L) { TNLAssert(false, "Unimplemented method!"); return 0; }        // Center of item (returns point)
   S32 getRad(lua_State *L) { TNLAssert(false, "Unimplemented method!"); return 0; }        // Radius of item (returns number)
   S32 getVel(lua_State *L) { TNLAssert(false, "Unimplemented method!"); return 0; }        // Speed of item (returns point)
   S32 getTeamIndx(lua_State *L) { TNLAssert(false, "Unimplemented method!"); return 0; }   // Team of shooter

   virtual GameObject *getGameObject() { TNLAssert(false, "Unimplemented method!"); return NULL; }  // Return the underlying GameObject
   
   void push(lua_State *L) {  TNLAssert(false, "Unimplemented method!"); }
};


class Projectile : public GameObject, public LuaProjectile
{
public:
   enum {
      CompressedVelocityMax = 2047,
      InitialMask = BIT(0),
      ExplodedMask = BIT(1),
   };

   Point pos;
   Point velocity;
   U32 mTimeRemaining;
   ProjectileType mType;
   WeaponType mWeaponType;
   bool collided;
   bool hitShip;
   bool alive;
   SafePtr<GameObject> mShooter;

   // Constructor
   Projectile(WeaponType type = WeaponPhaser, Point pos = Point(), Point vel = Point(), GameObject *shooter = NULL);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void handleCollision(GameObject *theObject, Point collisionPoint);

   void idle(GameObject::IdleCallPath path);
   void damageObject(DamageInfo *info);
   void explode(GameObject *hitObject, Point p);

   virtual Point getRenderVel() { return velocity; }
   virtual Point getActualVel() { return velocity; }
   virtual Point getActualPos() { return pos; }

   void render() { renderItem(getActualPos()); }     // TODO: Get rid of this! (currently won't render without it)
   void renderItem(Point p);

   TNL_DECLARE_CLASS(Projectile);

   // Lua interface
   S32 getLoc(lua_State *L);     // Center of item (returns point)
   S32 getRad(lua_State *L);     // Radius of item (returns number)
   S32 getVel(lua_State *L);     // Speed of item (returns point)
   S32 getTeamIndx(lua_State *L);   // Return team of shooter     

   GameObject *getGameObject();  // Return the underlying GameObject
   S32 getWeapon(lua_State *L) { return returnInt(L, mWeaponType ); }       // Return which type of weapon this is

   void push(lua_State *L) {  Lunar<LuaProjectile>::push(L, this); }
};


// GrenadeProjectiles are the base clase used for both mines and spybugs
class GrenadeProjectile : public Item, public LuaProjectile
{
private:
   typedef Item Parent;

public:
   GrenadeProjectile(Point pos = Point(), Point vel = Point(), GameObject *shooter = NULL);

   enum Constants
   {
      ExplodeMask = Item::FirstFreeMask,
      FirstFreeMask = ExplodeMask << 1,
      InnerBlastRadius = 100,
      OuterBlastRadius = 250,
   };

   S32 mTimeRemaining;
   bool exploded;
   bool collide(GameObject *otherObj) { return true; }   // Things (like bullets) can collide with grenades

   WeaponType mWeaponType;
   void renderItem(Point p);
   void idle(IdleCallPath path);
   void damageObject(DamageInfo *damageInfo);
   void explode(Point p, WeaponType weaponType);
   StringTableEntry mSetBy;      // Who laid the mine/spy bug?

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(GrenadeProjectile);

   // Lua interface
   GrenadeProjectile(lua_State *L) { /* Do not use */ };            //  Lua constructor

   // Need to provide these methods to deal with the complex web of inheritence... these come from both
   // Item and LuaProjectile, and by providing these, everything works.
   S32 getLoc(lua_State *L) { return Parent::getLoc(L); }     // Center of item (returns point)
   S32 getRad(lua_State *L) { return Parent::getRad(L); }     // Radius of item (returns number)
   S32 getVel(lua_State *L) { return Parent::getVel(L); }     // Speed of item (returns point)
   S32 getTeamIndx(lua_State *L) { return returnInt(L, mTeam + 1); }   // Team of shooter

   GameObject *getGameObject() { return this; }               // Return the underlying GameObject
   S32 getWeapon(lua_State *L) { return returnInt(L, WeaponBurst ); }       // Return which type of weapon this is

   void push(lua_State *L) {  Lunar<LuaProjectile>::push(L, this); }
};


class Mine : public GrenadeProjectile
{
   typedef GrenadeProjectile Parent;

public:
   enum Constants
   {
      ArmedMask = GrenadeProjectile::FirstFreeMask,

      SensorRadius     = 50,
      InnerBlastRadius = 75,
      OuterBlastRadius = 100,
   };

   Mine(Point pos = Point(), Ship *owner=NULL);    // Constructor

   bool mArmed;
   SafePtr<GameConnection> mOwnerConnection;
   bool collide(GameObject *otherObj);
   void idle(IdleCallPath path);

   void damageObject(DamageInfo *damageInfo);
   void renderItem(Point p);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(Mine);

   // Lua interface
   Mine(lua_State *L) { /* Do not use */ };            //  Lua constructor

   static const char className[];
   static Lunar<Mine>::RegType methods[];

   virtual S32 getClassID(lua_State *L) { return returnInt(L, MineType); } // Object's class   

   S32 getLoc(lua_State *L) { return Parent::getLoc(L); }     // Center of item (returns point)
   S32 getRad(lua_State *L) { return Parent::getRad(L); }     // Radius of item (returns number)
   S32 getVel(lua_State *L) { return Parent::getVel(L); }     // Speed of item (returns point)

   GameObject *getGameObject() { return this; }               // Return the underlying GameObject
   S32 getWeapon(lua_State *L) { return returnInt(L, WeaponMine ); }       // Return which type of weapon this is

   void push(lua_State *L) { Lunar<LuaProjectile>::push(L, this); }
};


class SpyBug : public GrenadeProjectile
{
   typedef GrenadeProjectile Parent;

public:
   SpyBug(Point pos = Point(), Ship *owner = NULL);      // Constructor
   bool processArguments(S32 argc, const char **argv);
   void onAddedToGame(Game *theGame);


   SafePtr<GameConnection> mOwnerConnection;
   bool collide(GameObject *otherObj);
   void idle(IdleCallPath path);

   void damageObject(DamageInfo *damageInfo);
   void renderItem(Point p);

   bool isVisibleToPlayer(S32 playerTeam, StringTableEntry playerName, bool isTeamGame);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(SpyBug);

   // Lua interface
   SpyBug(lua_State *L) { /* Do not use */ };            //  Lua constructor

   static const char className[];
   static Lunar<SpyBug>::RegType methods[];

   virtual S32 getClassID(lua_State *L) { return returnInt(L, SpyBugType); } // Object's class   

   S32 getLoc(lua_State *L) { return Parent::getLoc(L); }     // Center of item (returns point)
   S32 getRad(lua_State *L) { return Parent::getRad(L); }     // Radius of item (returns number)
   S32 getVel(lua_State *L) { return Parent::getVel(L); }     // Speed of item (returns point)

   GameObject *getGameObject() { return this; }               // Return the underlying GameObject
   S32 getWeapon(lua_State *L) { return returnInt(L, WeaponSpyBug ); }       // Return which type of weapon this is

   void push(lua_State *L) { Lunar<LuaProjectile>::push(L, this); }
};



};
#endif

