//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef WEAPONINFO_H_
#define WEAPONINFO_H_

#include "tnlNetStringTable.h"

using namespace TNL;

namespace Zap
{

// Forward declarations
class BfObject;


/**
 * @luaenum Weapon(2, 1)
 * The Weapon enum can be used to represent a weapon in some functions.
 * It can also be used with the `WeaponInfo` table to provide data about a weapon's statistics:
 * @code
 * -- Get information like this:
 * print(WeaponInfo[Weapon.Phaser].damage)
 *
 * -- This will print all info for all weapons
 * for i = Weapon.Phaser, Weapon.SpyBug do
 * 
 *    print()
 * 
 *    for k, v in pairs(WeaponInfo[i]) do
 *       print(k .. ": " .. tostring(v))
 *    end
 * end
 * @endcode
 * The elements of this table are tables with the following keys:
 *   - `projectileVelocity`
 *   - `fireDelay`
 *   - `classId`
 *   - `damageSelf`
 *   - `name`
 *   - `canDamageTeammate`
 *   - `projectileLifeTime`
 *   - `energyDrain`
 *   - `minEnergy`
 *   - `damage`
 */
   //                                       Enum in Lua:   Fire  Min    Enrgy  Proj Proj. Dam-  Self-dam. Can dam.   Projectile
   //             Enum          Name        Weapon.Phaser  Delay Enrgy  Drain  Vel. Life  age    Factor   Teammate      Type
#define WEAPON_ITEM_TABLE \
   WEAPON_ITEM(WeaponPhaser,  "Phaser",      "Phaser",     100,   500,   500,  600, 1000, 0.19f,  0,       false, ProjectilePhaser ) \
   WEAPON_ITEM(WeaponBounce,  "Bouncer",     "Bouncer",    100,  1600,  1600,  540, 1500, 0.15f,  0.4f,    false, ProjectileBounce ) \
   WEAPON_ITEM(WeaponTriple,  "Triple",      "Triple",     200,  3500,  3500,  550,  850, 0.17f,  0,       false, ProjectileTriple ) \
   WEAPON_ITEM(WeaponBurst,   "Burst",       "Burst",      700, 10000, 10000,  500, 1000, 0.50f,  1.0f,    false, NotAProjectile   ) \
   WEAPON_ITEM(WeaponSeeker,  "Seeker",      "Seeker",     400, 10000, 10000,  600, 8000, 0.21f,  1.0f,    false, NotAProjectile   ) \
   WEAPON_ITEM(WeaponMine,    "Mine",        "Mine",       900, 55000, 55000,  500,   -1, 0.50f,  1.0f,    true,  NotAProjectile   ) \
   WEAPON_ITEM(WeaponTurret,  "Turret",      "Turret",     150,     0,     0,  800,  800, 0.11f,  1.0f,    true,  ProjectileTurret ) \
   WEAPON_ITEM(WeaponSpyBug,  "Spy Bug",     "SpyBug",     800, 50000, 50000,  800,   -1, 0,      1.0f,    true,  NotAProjectile   ) \


// Define an enum from the first values in WEAPON_ITEM_TABLE
enum WeaponType {
#define WEAPON_ITEM(value, b, c, d, e, f, g, h, i, j, k, l) value,
   WEAPON_ITEM_TABLE
#undef WEAPON_ITEM
   WeaponCount, 
   WeaponNone
};


// Note that not all WeaponTypes are Projectile weapons, so don't have ProjectileTypes
enum ProjectileType
{
   ProjectilePhaser,
   ProjectileBounce,
   ProjectileTriple,
   ProjectileTurret,
   ProjectileTypeCount,
   NotAProjectile  // Need this so we can fit a non-ProjectileType (like mine) into a constructor intended for proper projectiles
};


struct WeaponInfo
{
   static const char *getWeaponName(WeaponType weaponType);

   StringTableEntry name;           // Display name of the weapon.
   U32 fireDelay;                   // Delay between shots.
   S32 minEnergy;                   // Minimum energy to fire.  (Use S32 to avoid compiler warnings when comparing with other S32s)
   U32 drainEnergy;                 // Amount of energy to drain per shot.
   U32 projVelocity;                // How fast shot travels (dist/second)
   S32 projLiveTime;                // How long shot lives (millisecs)
   F32 damageAmount;                // Damage shot does
   F32 damageSelfMultiplier;        // Adjust damage if you shoot yourself
   bool canDamageTeammate;
   ProjectileType projectileType;   // If this is a projectile item, which sort is it?  If not, use NotAProjectile

   static WeaponInfo getWeaponInfo(WeaponType weaponType);

#ifndef BF_MASTER
   static WeaponType getWeaponTypeFromObject(BfObject *bfObject);
   static BfObject *getWeaponShooterFromObject(BfObject *bfObject);
#endif
};


};

#endif /* WEAPONINFO_H_ */
