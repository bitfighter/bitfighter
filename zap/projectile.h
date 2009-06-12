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
   //LuaProjectile(lua_State *L) { /* Do not use */ };            //  Lua constructor

   static const char className[];

   S32 getLoc(lua_State *L) { TNLAssert(false, "Unimplemented method!"); return 0; }     // Center of item (returns point)
   S32 getRad(lua_State *L) { TNLAssert(false, "Unimplemented method!"); return 0; }     // Radius of item (returns number)
   S32 getVel(lua_State *L) { TNLAssert(false, "Unimplemented method!"); return 0; }     // Speed of item (returns point)

   virtual S32 getClassID(lua_State *L) { return returnInt(L, BulletType); } // Object's class    

 /*  void push(lua_State *L) {  Lunar<LuaProjectile>::push(L, this); }*/

   virtual GameObject *getGameObject() { TNLAssert(false, "Unimplemented method!"); return NULL; }  // Return the underlying GameObject
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
   Projectile(lua_State *L) { /* Do not use */ };            //  Lua constructor

   //static const char className[];
   static Lunar<Projectile>::RegType methods[];

   S32 getLoc(lua_State *L);     // Center of item (returns point)
   S32 getRad(lua_State *L);     // Radius of item (returns number)
   S32 getVel(lua_State *L);     // Speed of item (returns point)
   GameObject *getGameObject();  // Return the underlying GameObject


     void push(lua_State *L) {  Lunar<LuaProjectile>::push(L, this); }
   //void push(lua_State *L) {  Lunar<Projectile>::push(L, this); }
   //S32 getClassID(lua_State *L) { return returnInt(L, BulletType); }

   //static LuaItem *getItem(lua_State *L, S32 index, U32 type, const char *functionName);
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
   bool collide(GameObject *otherObj) { return true; };   // Things (like bullets) can collide with grenades

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

   S32 getClassID(lua_State *L) { return returnInt(L, BulletType); }    // Why do we need this?
   void push(lua_State *L) {  Lunar<LuaProjectile>::push(L, this); }    // Why do we need this?
   static const char className[];
   static Lunar<GrenadeProjectile>::RegType methods[];

   // We're inheriting two copies of getLoc... one from Item and one from LuaProjectile.  
   // We need to tell the compiler which one we're using.
   using Item::getLoc;
   using Item::getRad;
   using Item::getVel;

   //S32 getLoc(lua_State *L);     // Center of item (returns point)
   //S32 getRad(lua_State *L);     // Radius of item (returns number)
   //S32 getVel(lua_State *L);     // Speed of item (returns point)
   GameObject *getGameObject();  // Return the underlying GameObject

   //static LuaItem *getItem(lua_State *L, S32 index, U32 type, const char *functionName);
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
      OuterBlastRadius = 100 ,
   };

   Mine(Point pos = Point(), Ship *owner=NULL);    // Constructor

   bool mArmed;
 //  StringTableEntry mSetBy;      // Who set the mine?
   SafePtr<GameConnection> mOwnerConnection;
   bool collide(GameObject *otherObj);
   void idle(IdleCallPath path);

   void damageObject(DamageInfo *damageInfo);
   void renderItem(Point p);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(Mine);

   // Lua interface
   void push(lua_State *L) {  Lunar<Mine>::push(L, this); }
   S32 getClassID(lua_State *L) { return returnInt(L, MineType); }
   //static const char className[];
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

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(SpyBug);

   // Lua interface
   //void push(lua_State *L) {  Lunar<SpyBug>::push(L, this); }
   //S32 getClassID(lua_State *L) { return returnInt(L, SpyBugType); }
   //static const char className[];

};



};
#endif
