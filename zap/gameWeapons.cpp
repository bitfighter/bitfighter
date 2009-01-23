//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#include "gameWeapons.h"
#include "projectile.h"
#include "ship.h"

namespace Zap
{

// do not add a weapon with a fire delay > Ship::MaxFireDelay
// or update the constant.

ShipWeaponInfo gWeapons[] =    //                 Fire   Min    Drain Proj  Proj  Dam-   Can damage  Can damage Projectile
{                              //    Name         Delay  Energy Energy Vel. Life  age       self      teammate   Type
   ShipWeaponInfo(StringTableEntry("Phaser"),      100,   500,   500,  600, 1000, 0.21f,   false,      false,   ProjectilePhaser ),
   ShipWeaponInfo(StringTableEntry("Bouncer"),     100,  1800,  1800,  540, 1500, 0.15f,   true,       false,   ProjectileBounce ),
   ShipWeaponInfo(StringTableEntry("Triple"),      200,  2100,  2100,  550,  850, 0.14f,   true,       false,   ProjectileTriple ),
   ShipWeaponInfo(StringTableEntry("Burst"),       700,  5000,  5000,  500, 1000, 0.50f,   true,       false,   NotAProjectile ),
   ShipWeaponInfo(StringTableEntry("Heat Seeker"), 700,  5000,  5000,  100, 6000, 0.12f,   true,       false,   NotAProjectile ),
   ShipWeaponInfo(StringTableEntry("Mine"),        900, 55000, 55000,  500, 1000, 0.50f,   true,       true,    NotAProjectile ),
   ShipWeaponInfo(StringTableEntry("Turret"),        0,     0,     0,  800,  800, 0.11f,   true,       true,    ProjectileTurret ),
   ShipWeaponInfo(StringTableEntry("Spy Bug"),     800, 50000, 50000,  800,  800, 0,       true,       true,    NotAProjectile ),      // Damage in this case is getting pushed around by the explosion
};

ProjectileInfo gProjInfo[ProjectileTypeCount] = {
   //               SparkColor1    SparkColor2   SparkColor3    SparkColor4     ProjectileColor1  ProjectileColor2 Scale  Fire sound          Impact soune
   ProjectileInfo( Color(1,0,1), Color(1,1,1), Color(0,0,1),   Color(1,0,0),   Color(1, 0, 0.5), Color(0.5, 0, 1), 1.0f, SFXPhaserProjectile, SFXPhaserImpact ), // Phaser
   ProjectileInfo( Color(1,1,0), Color(1,0,0), Color(1,0.5,0), Color(1,1,1),   Color(1, 1, 0),   Color(1, 0, 0),   1.3f, SFXBounceProjectile, SFXBounceImpact ), // Bounce
   ProjectileInfo( Color(0,0,1), Color(0,1,0), Color(0,0.5,1), Color(0,1,0.5), Color(0, 0.5, 1), Color(0, 1, 0.5), 0.7f, SFXTripleProjectile, SFXTripleImpact ), // Triple
   ProjectileInfo( Color(0,1,1), Color(1,1,0), Color(0,1,0.5), Color(0.5,1,0), Color(0.5, 1, 0), Color(0, 1, 0.5), 0.6f, SFXTurretProjectile, SFXTurretImpact ), // Turret
};


// Here we actually intantiate the various projectiles when fired
void createWeaponProjectiles(WeaponType weapon, Point &dir, Point &shooterPos, Point &shooterVel, F32 shooterRadius, GameObject *shooter)
{
   GameObject *proj = NULL;
   ShipWeaponInfo *wi = gWeapons + weapon;
   Point projVel = dir * F32(wi->projVelocity) + dir * shooterVel.dot(dir);
   Point firePos = shooterPos + dir * shooterRadius;

   switch(weapon)
   {
      case WeaponTriple:      // Add three bullets!
         {
            Point velPerp(projVel.y, -projVel.x);
            velPerp.normalize(40.0f);
            (new Projectile(weapon, firePos, projVel + velPerp, wi->projLiveTime, shooter))->addToGame(shooter->getGame());
            (new Projectile(weapon, firePos, projVel - velPerp, wi->projLiveTime, shooter))->addToGame(shooter->getGame());
         }
      case WeaponPhaser:
      case WeaponBounce:
      case WeaponTurretBlaster:
         (new Projectile(weapon, firePos, projVel, wi->projLiveTime, shooter))->addToGame(shooter->getGame());
         break;
      case WeaponBurst:
         (new GrenadeProjectile(firePos, projVel, wi->projLiveTime, shooter))->addToGame(shooter->getGame());
         break;
      case WeaponMine:
         (new Mine(firePos, dynamic_cast<Ship *>(shooter)))->addToGame(shooter->getGame());
         break;
      case WeaponHeatSeeker:
         (new HeatSeeker(firePos, projVel, wi->projLiveTime, shooter))->addToGame(shooter->getGame());
         break;
      case WeaponSpyBug:
         (new SpyBug(firePos, dynamic_cast<Ship *>(shooter)))->addToGame(shooter->getGame());
         break;

   }
}

};

