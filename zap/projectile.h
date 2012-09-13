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
#include "BfObject.h"
#include "item.h"
#include "gameWeapons.h"
#include "SoundSystem.h" // for enum SFXProfiles

namespace Zap
{

class Ship;

////////////////////////////////////
////////////////////////////////////

// Basic bullet object
class Projectile : public BfObject
{
   typedef BfObject Parent;

private:
   static const S32 COMPRESSED_VELOCITY_MAX = 2047;

protected:
   enum MaskBits {
      InitialMask   = Parent::FirstFreeMask << 0,
      ExplodedMask  = Parent::FirstFreeMask << 1,
      PositionMask  = Parent::FirstFreeMask << 2,
      FirstFreeMask = Parent::FirstFreeMask << 3
   };

   Point mVelocity;

public:
   U32 mTimeRemaining;
   ProjectileType mType;
   WeaponType mWeaponType;
   bool collided;
   bool hitShip;
   bool alive;
   bool hasBounced;
   SafePtr<BfObject> mShooter;

   Projectile(WeaponType type = WeaponPhaser, Point pos = Point(), Point vel = Point(), BfObject *shooter = NULL);    // Constructor
   ~Projectile();                                                                                                     // Destructor

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void handleCollision(BfObject *theObject, Point collisionPoint);

   void idle(BfObject::IdleCallPath path);
   void damageObject(DamageInfo *info);
   void explode(BfObject *hitObject, Point p);

   virtual Point getRenderVel() const;
   virtual Point getActualVel() const;

   void render();
   void renderItem(const Point &pos);

   TNL_DECLARE_CLASS(Projectile);

   //// Lua interface
   LUAW_DECLARE_CLASS(Projectile);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 getRad(lua_State *L);      // Radius of item (returns number)
   S32 getVel(lua_State *L);
   S32 getWeapon(lua_State *L);   // Return which type of weapon this is
};


// Basic burst object, and the base clase used for both mines and spybugs
class BurstProjectile : public MoveItem
{
private:
   typedef MoveItem Parent;

public:
   BurstProjectile(Point pos = Point(), Point vel = Point(), BfObject *shooter = NULL);     // Constructor
   ~BurstProjectile();                                                                        // Destructor

   enum Constants
   {
      FirstFreeMask = MoveItem::FirstFreeMask,
   };

   static const S32 InnerBlastRadius = 100;
   static const S32 OuterBlastRadius = 250;

   SafePtr<BfObject> mShooter;
   S32 mTimeRemaining;
   bool exploded;
   bool collide(BfObject *otherObj);   // Things (like bullets) can collide with grenades

   WeaponType mWeaponType;
   void renderItem(const Point &pos);
   void idle(IdleCallPath path);
   void damageObject(DamageInfo *damageInfo);
   void doExplosion(const Point &pos);
   void explode(const Point &pos);
   StringTableEntry mSetBy;      // Who laid the mine/spy bug?

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(BurstProjectile);

   //// Lua interface
   LUAW_DECLARE_CLASS(BurstProjectile);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   virtual S32 getWeapon(lua_State *L);   // Return which type of weapon this is
};


////////////////////////////////////////
////////////////////////////////////////

class Mine : public BurstProjectile
{
   typedef BurstProjectile Parent;

public:
   enum Constants
   {
      ArmedMask = BurstProjectile::FirstFreeMask,
      SensorRadius     = 50,
   };

   static const S32 InnerBlastRadius = 100;
   static const S32 OuterBlastRadius = 250;

   Mine(Point pos = Point(), Ship *owner=NULL);    // Constructor
   ~Mine();                                        // Destructor
   Mine *clone() const;

   bool mArmed;
   SafePtr<GameConnection> mOwnerConnection;
   bool collide(BfObject *otherObj);
   void idle(IdleCallPath path);

   void damageObject(DamageInfo *damageInfo);
   void renderItem(const Point &pos);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(Mine);

   /////
   // Editor methods
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   void renderDock();

   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   string toString(F32 gridSize) const;

   ///// Lua interface
   LUAW_DECLARE_CLASS(Mine);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


////////////////////////////////////////
////////////////////////////////////////

class SpyBug : public BurstProjectile
{
   typedef BurstProjectile Parent;

public:
   SpyBug(Point pos = Point(), Ship *owner = NULL);      // Constructor
   ~SpyBug();                                            // Destructor
   SpyBug *clone() const;

   bool processArguments(S32 argc, const char **argv, Game *game);
   void onAddedToGame(Game *theGame);

   SafePtr<GameConnection> mOwnerConnection;
   bool collide(BfObject *otherObj);
   void idle(IdleCallPath path);

   void damageObject(DamageInfo *damageInfo);
   void renderItem(const Point &pos);

   bool isVisibleToPlayer(S32 playerTeam, StringTableEntry playerName, bool isTeamGame);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(SpyBug);

   /////
   // Editor methods
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   void renderDock();

   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   string toString(F32 gridSize) const;

   ///// Lua interface
   LUAW_DECLARE_CLASS(SpyBug);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


// Basic burst object, and the base clase used for both mines and spybugs
class HeatSeekerProjectile : public MoveItem
{
private:
   typedef MoveItem Parent;

   enum Constants
   {
      FirstFreeMask = MoveItem::FirstFreeMask,
   };

   static U32 SpeedIncreasePerSecond;
   static U32 TargetAcquisitionRadius;
   static F32 MaximumAngleChangePerSecond;
   static F32 TargetSearchAngle;

   SafePtr<BfObject> mAcquiredTarget;

   S32 mTimeRemaining;
   bool exploded;
   bool bounced;

   void acquireTarget();


public:
   HeatSeekerProjectile(Point pos = Point(), Point vel = Point(), BfObject *shooter = NULL);     // Constructor
   ~HeatSeekerProjectile();                                                                      // Destructor

   SafePtr<BfObject> mShooter;
   WeaponType mWeaponType;

   bool collide(BfObject *otherObj);   // Things (like bullets) can collide with grenades
   bool collided(BfObject *otherObj, U32 stateIndex);   // Things (like bullets) can collide with grenades

   void renderItem(const Point &pos);
   void idle(IdleCallPath path);
   void damageObject(DamageInfo *damageInfo);
   void doExplosion(const Point &pos);
   void handleCollision(BfObject *hitObject, Point collisionPoint);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(HeatSeekerProjectile);

   //// Lua interface
   LUAW_DECLARE_CLASS(HeatSeekerProjectile);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   virtual S32 getWeapon(lua_State *L);   // Return which type of weapon this is
};



};
#endif

