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

#ifndef WEAPONINFO_H_
#define WEAPONINFO_H_

#include "tnlNetStringTable.h"
#include "Color.h"
#include "SoundSystem.h"

using namespace TNL;

namespace Zap
{
/**
  * @luaenum Weapon(2, 1)
  * The Weapon enum can be used to represent a weapon in some functions.
  */
   //                                       Enum in Lua:   Fire  Min    Enrgy  Proj Proj. Dam-  Self-dam. Can dam.   Projectile
   //             Enum          Name        Weapon.Phaser  Delay Enrgy  Drain  Vel. Life  age    Factor   Teammate      Type
#define WEAPON_ITEM_TABLE \
   WEAPON_ITEM(WeaponPhaser,  "Phaser",      "Phaser",     100,   500,   500,  600, 1000, 0.19f,  0,       false, ProjectilePhaser ) \
   WEAPON_ITEM(WeaponBounce,  "Bouncer",     "Bouncer",    100,  1600,  1600,  540, 1500, 0.15f,  0.4f,    false, ProjectileBounce ) \
   WEAPON_ITEM(WeaponTriple,  "Triple",      "Triple",     200,  1700,  1700,  550,  850, 0.17f,  0,       false, ProjectileTriple ) \
   WEAPON_ITEM(WeaponBurst,   "Burst",       "Burst",      700, 10000, 10000,  500, 1000, 0.50f,  1.0f,    false, NotAProjectile   ) \
   WEAPON_ITEM(WeaponSeeker,  "Seeker",      "Seeker",     700, 20000, 20000,  600, 8000, 0.34f,  1.0f,    false, NotAProjectile   ) \
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
};


/////////////////////////////////////
/////////////////////////////////////

static const U32 NumSparkColors = 4;

struct ProjectileInfo
{
   ProjectileInfo(Color _sparkColor1, Color _sparkColor2, Color _sparkColor3, Color _sparkColor4, Color _projColor1,
         Color _projColor2, F32 _scaleFactor, SFXProfiles _projectileSound, SFXProfiles _impactSound );

   Color       sparkColors[NumSparkColors];
   Color       projColors[2];
   F32         scaleFactor;
   SFXProfiles projectileSound;
   SFXProfiles impactSound;
};


};

#endif /* WEAPONINFO_H_ */
