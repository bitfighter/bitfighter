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

#include "gameWeapons.h"
#include "projectile.h"
#include "ship.h"
#include "game.h"

#include "Colors.h"

namespace Zap
{


ProjectileInfo GameWeapon::projectileInfo[ProjectileTypeCount] =
{
   //               SparkColor1     SparkColor2     SparkColor3       SparkColor4     ProjectileColor1  ProjectileColor2  Scale  Fire sound          Impact sound
   ProjectileInfo( Colors::magenta, Colors::white,  Colors::blue,     Colors::red,    Color(1, 0, 0.5), Color(0.5, 0, 1), 1.0f, SFXPhaserProjectile, SFXPhaserImpact ), // Phaser
   ProjectileInfo( Colors::yellow,  Colors::red,    Colors::orange50, Colors::white,  Colors::yellow,   Colors::red,      1.3f, SFXBounceProjectile, SFXBounceImpact ), // Bounce
   ProjectileInfo( Colors::blue,    Colors::green,  Color(0,0.5,1),   Color(0,1,0.5), Color(0, 0.5, 1), Color(0, 1, 0.5), 0.7f, SFXTripleProjectile, SFXTripleImpact ), // Triple
   ProjectileInfo( Colors::cyan,    Colors::yellow, Color(0,1,0.5),   Color(0.5,1,0), Color(0.5, 1, 0), Color(0, 1, 0.5), 0.6f, SFXTurretProjectile, SFXTurretImpact ), // Turret
};


// Define a list of WeaponInfos
WeaponInfo GameWeapon::weaponInfo[] = {
#  define WEAPON_ITEM(a, name, c, delay, minEn, drainEn, projVel, projTTL, damage, selfDamage, canDamTMs, projType) \
   { StringTableEntry(name), delay, minEn, drainEn, projVel, projTTL, damage, selfDamage, canDamTMs, projType },
      WEAPON_ITEM_TABLE
#  undef WEAPON_ITEM
};

// Here we actually intantiate the various projectiles when fired
void GameWeapon::createWeaponProjectiles(WeaponType weapon, const Point &dir, const Point &shooterPos, const Point &shooterVel, S32 time, F32 shooterRadius, BfObject *shooter)
{
   //BfObject *proj = NULL;
   WeaponInfo *wi = weaponInfo + weapon;
   Point projVel = dir * F32(wi->projVelocity) + dir * shooterVel.dot(dir);
   Point firePos = shooterPos + dir * shooterRadius;

   // Advance pos by the distance the projectile would have traveled in time... fixes skipped shot effect on stuttering CPU
   firePos += projVel * F32(time) / 1000.0;

   Game *game = shooter->getGame();

   switch(weapon)
   {
      case WeaponTriple:      // Add three bullets!
         {
            Point velPerp(projVel.y, -projVel.x);
            velPerp.normalize(40.0f); // <== spread factor
            (new Projectile(weapon, firePos, projVel,           shooter))->addToGame(game, game->getGameObjDatabase());
            (new Projectile(weapon, firePos, projVel + velPerp, shooter))->addToGame(game, game->getGameObjDatabase());
            (new Projectile(weapon, firePos, projVel - velPerp, shooter))->addToGame(game, game->getGameObjDatabase());
         }
         break;
      case WeaponPhaser:
      case WeaponBounce:
      case WeaponTurret:
         (new Projectile(weapon, firePos, projVel, shooter))->addToGame(game, game->getGameObjDatabase());
         break;
      case WeaponBurst:                                         // 0.9 to fix firing through barriers
         (new BurstProjectile(shooterPos + dir * shooterRadius * 0.9f, projVel, shooter))->addToGame(game, game->getGameObjDatabase());
         break;
      case WeaponMine:
         (new Mine(firePos, static_cast<Ship *>(shooter)))->addToGame(game, game->getGameObjDatabase());
         break;
      case WeaponSpyBug:
         (new SpyBug(firePos, static_cast<Ship *>(shooter)))->addToGame(game, game->getGameObjDatabase());
         break;
      case WeaponHeatSeeker:
         (new HeatSeekerProjectile(shooterPos + dir * shooterRadius * 0.9f, projVel, shooter))->addToGame(game, game->getGameObjDatabase());
         break;
      default:
         break;
   }
}

};


