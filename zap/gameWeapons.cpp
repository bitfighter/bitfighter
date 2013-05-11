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
#include "game.h"

#include "Colors.h"

   
namespace Zap
{

ProjectileInfo::ProjectileInfo(Color _sparkColor1, Color _sparkColor2, Color _sparkColor3,
      Color _sparkColor4, Color _projColor1, Color _projColor2, F32 _scaleFactor,
      SFXProfiles _projectileSound, SFXProfiles _impactSound )
{
   sparkColors[0]  = _sparkColor1;
   sparkColors[1]  = _sparkColor2;
   sparkColors[2]  = _sparkColor3;
   sparkColors[3]  = _sparkColor4;
   projColors[0]   = _projColor1;
   projColors[1]   = _projColor2;
   scaleFactor     = _scaleFactor;
   projectileSound = _projectileSound;
   impactSound     = _impactSound;
}

// Destructor
ProjectileInfo::~ProjectileInfo()
{
   // Do nothing
}


ProjectileInfo GameWeapon::projectileInfo[ProjectileTypeCount] =
{
   //               SparkColor1     SparkColor2     SparkColor3       SparkColor4     ProjectileColor1  ProjectileColor2  Scale  Fire sound          Impact sound
   ProjectileInfo( Colors::magenta, Colors::white,  Colors::blue,     Colors::red,    Color(1, 0, 0.5), Color(0.5, 0, 1), 1.0f, SFXPhaserProjectile, SFXPhaserImpact ), // Phaser
   ProjectileInfo( Colors::yellow,  Colors::red,    Colors::orange50, Colors::white,  Colors::yellow,   Colors::red,      1.3f, SFXBounceProjectile, SFXBounceImpact ), // Bounce
   ProjectileInfo( Colors::blue,    Colors::green,  Color(0,0.5,1),   Color(0,1,0.5), Color(0, 0.5, 1), Color(0, 1, 0.5), 0.7f, SFXTripleProjectile, SFXTripleImpact ), // Triple
   ProjectileInfo( Colors::cyan,    Colors::yellow, Color(0,1,0.5),   Color(0.5,1,0), Color(0.5, 1, 0), Color(0, 1, 0.5), 0.6f, SFXTurretProjectile, SFXTurretImpact ), // Turret
};


// Here we actually intantiate the various projectiles when fired
void GameWeapon::createWeaponProjectiles(WeaponType weapon, const Point &dir, const Point &shooterPos, const Point &shooterVel, S32 time, F32 shooterRadius, BfObject *shooter)
{
   Point projVel = dir * F32(WeaponInfo::getWeaponInfo(weapon).projVelocity) + dir * shooterVel.dot(dir);
   Point firePos = shooterPos + dir * shooterRadius;

   // Advance pos by the distance the projectile would have traveled in time... fixes skipped shot effect on stuttering CPU
   firePos += projVel * F32(time) / 1000.0;

   Game *game = shooter->getGame();

   switch(weapon)
   {
      case WeaponTriple:      // Add three bullets!
         {
            const F32 SPREAD_FACTOR = 40.0f;    // Larger = broader spread
            Point velPerp(projVel.y, -projVel.x);
            velPerp.normalize(SPREAD_FACTOR); 
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
         (new Burst(shooterPos + dir * shooterRadius * 0.9f, projVel, shooter))->addToGame(game, game->getGameObjDatabase());
         break;
      case WeaponMine:
         (new Mine(firePos, shooter))->addToGame(game, game->getGameObjDatabase());
         break;
      case WeaponSpyBug:
         (new SpyBug(firePos, shooter))->addToGame(game, game->getGameObjDatabase());
         break;
      case WeaponSeeker:
         (new Seeker(shooterPos + dir * shooterRadius * 0.9f, projVel, dir.ATAN2(), shooter))->addToGame(game, game->getGameObjDatabase());
         break;
      default:
         break;
   }
}

};


