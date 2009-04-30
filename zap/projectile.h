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

class Projectile : public GameObject
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
   Projectile(WeaponType type = WeaponPhaser, Point pos = Point(), Point vel = Point(), U32 liveTime = 0, GameObject *shooter = NULL);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void handleCollision(GameObject *theObject, Point collisionPoint);

   void idle(GameObject::IdleCallPath path);
   void damageObject(DamageInfo *info);
   void explode(GameObject *hitObject, Point p);

   virtual Point getRenderVel() { return velocity; }
   virtual Point getActualVel() { return velocity; }

   void render();
   TNL_DECLARE_CLASS(Projectile);
};

// GrenadeProjectiles are the base clase used for both mines and spybugs
class GrenadeProjectile : public Item
{
private:
   typedef Item Parent;

public:
   GrenadeProjectile(Point pos = Point(), Point vel = Point(), U32 liveTime = 0, GameObject *shooter = NULL);

   enum Constants
   {
      ExplodeMask = Item::FirstFreeMask,
      FirstFreeMask = ExplodeMask << 1,
      InnerBlastRadius = 100,
      OuterBlastRadius = 250,
   };

   S32 ttl;
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
};


class HeatSeeker : public Item
{
   typedef Item Parent;
public:
   HeatSeeker(Point pos = Point(), Point vel = Point(), U32 liveTime = 0, GameObject *shooter = NULL);

   enum Constants {
      CompressedVelocityMax = 2047,
      InitialMask = BIT(0),
      ExplodedMask = BIT(1),
//      FirstFreeMask = ExplodeMask << 1,
   };

   bool collided;
              bool exploded;    // TEMP
           //    void damageObject(DamageInfo *damageInfo);
              void explode(Point p, WeaponType weaponType);

   bool alive;
   Point pos;
   Point velocity;
   Point mTarget;
   bool mHasTarget;

   WeaponType mWeaponType;

   U32 mTimeRemaining;

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void handleCollision(GameObject *theObject, Point collisionPoint);
   void explode(GameObject *hitObject, Point p);
   //void render();

   void idle(GameObject::IdleCallPath path);
   void renderItem(Point p);
   bool collide(GameObject *otherObj) { return true; };     // What does this really mean?

   SafePtr<GameObject> mShooter;


   TNL_DECLARE_CLASS(HeatSeeker);
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
};



};
#endif
