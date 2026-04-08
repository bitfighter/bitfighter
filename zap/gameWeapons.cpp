//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gameWeapons.h"
#include "projectile.h"
#include "game.h"
#include "XtankShape.h"    // For XtankWeapon, xtankWeaponInfos

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


// This is for client graphics/display
// It must align with ProjectileStyle enum
ProjectileInfo GameWeapon::projectileInfo[ProjectileStyleCount] =
{
   //               SparkColor1     SparkColor2     SparkColor3       SparkColor4     ProjectileColor1  ProjectileColor2  Scale  Fire sound          Impact sound
   ProjectileInfo( Colors::magenta, Colors::white,  Colors::blue,     Colors::red,    Color(1, 0, 0.5), Color(0.5, 0, 1), 1.0f, SFXPhaserProjectile, SFXPhaserImpact ), // Phaser
   ProjectileInfo( Colors::yellow,  Colors::red,    Colors::orange50, Colors::white,  Colors::yellow,   Colors::red,      1.3f, SFXBounceProjectile, SFXBounceImpact ), // Bounce
   ProjectileInfo( Colors::blue,    Colors::green,  Color(0,0.5,1),   Color(0,1,0.5), Color(0, 0.5, 1), Color(0, 1, 0.5), 0.7f, SFXTripleProjectile, SFXTripleImpact ), // Triple
   ProjectileInfo( Colors::cyan,    Colors::yellow, Color(0,1,0.5),   Color(0.5,1,0), Color(0.5, 1, 0), Color(0, 1, 0.5), 0.6f, SFXTurretProjectile, SFXTurretImpact ), // Turret
   ProjectileInfo( Colors::blue,    Colors::magenta,Colors::red,      Colors::cyan,   Colors::blue,     Colors::cyan,     3.0f, SFXRailgunProjectile,SFXRailgunImpact ), // Railgun
   // Xtank weapon styles.  projColors[0] is the bullet fill colour; projColors[1] is the
   // outline / highlight colour.  Spark colours match the bullet to give consistent sparks.
   //                SparkColor1        SparkColor2         SparkColor3         SparkColor4           ProjColor1              ProjColor2            Scale  Fire sound           Impact sound
   ProjectileInfo( Colors::blue,      Colors::cyan,       Colors::white,      Colors::blue,         Colors::blue,           Colors::cyan,         1.0f, SFXTurretProjectile,  SFXTurretImpact  ), // XtankBlue   (MachineGun, Tracer)
   ProjectileInfo( Colors::orange50,  Colors::yellow,     Colors::red,        Colors::orange50,     Colors::orange50,       Colors::yellow,       1.0f, SFXPhaserProjectile,  SFXPhaserImpact  ), // XtankOrange (Grenade)
   ProjectileInfo( Colors::yellow,    Colors::white,      Colors::orange50,   Colors::yellow,       Colors::yellow,         Colors::white,        1.0f, SFXBounceProjectile,  SFXBounceImpact  ), // XtankYellow (Rocket, Bomb)
   ProjectileInfo( Colors::green,     Colors::yellow,     Colors::green,      Colors::cyan,         Colors::green,          Color(0,0.8f,0.2f),   1.0f, SFXTripleProjectile,  SFXTripleImpact  ), // XtankGreen  (Acid, Fire)
   ProjectileInfo( Colors::magenta,   Color(0.5f,0,1),    Colors::blue,       Colors::magenta,      Colors::magenta,        Color(0.5f,0,1),      1.0f, SFXPhaserProjectile,  SFXPhaserImpact  ), // XtankViolet (Missile)
   ProjectileInfo( Colors::white,     Colors::cyan,       Colors::white,      Colors::cyan,         Colors::white,          Colors::cyan,         1.0f, SFXRailgunProjectile, SFXRailgunImpact ), // XtankLaser  (Laser beam)
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
      case WeaponRailgun:
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


// Create a projectile for an xtank weapon using xtank-derived speed and
// lifetime, so it behaves like the original xtank game.  barrelTip is the
// world-space muzzle position (shooterRadius is 0 since we already have the
// exact muzzle position).  The xtank-specific ProjectileStyle is applied after
// construction so the projectile is rendered with an xtank look.
void GameWeapon::createXtankProjectile(XtankWeapon::Type weapon, const Point &dir,
      const Point &barrelTip, const Point &shooterVel, S32 time, BfObject *shooter)
{
   if((S32)weapon < 0 || (S32)weapon >= XtankWeapon::Count)
      return;

   const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)weapon];
   WeaponType bfWeapon = wi.bfWeapon;
   Game *game = shooter->getGame();

   // Compute projectile velocity using the xtank-derived speed, not the BF
   // weapon default speed.  We still add the shooter's velocity component
   // along the fire direction (relative shooting).
   Point projVel = dir * F32(wi.projVelocity) + dir * shooterVel.dot(dir);

   // Advance the spawn position for any latency the caller passes in.
   Point firePos = barrelTip + projVel * F32(time) / 1000.0f;

   switch(bfWeapon)
   {
      case WeaponTriple:   // Fire (spreads into 3 pellets)
         {
            const F32 SPREAD_FACTOR = 40.0f;
            Point velPerp(projVel.y, -projVel.x);
            velPerp.normalize(SPREAD_FACTOR);
            for(S32 k = -1; k <= 1; k++)
            {
               Projectile *p = new Projectile(bfWeapon, firePos, projVel + velPerp * F32(k), shooter);
               p->mTimeRemaining = wi.projLiveTime;
               p->mStyle         = wi.style;
               p->addToGame(game, game->getGameObjDatabase());
            }
         }
         break;

      case WeaponPhaser:
      case WeaponBounce:
      case WeaponTurret:
      case WeaponRailgun:
         {
            Projectile *p = new Projectile(bfWeapon, firePos, projVel, shooter);
            p->mTimeRemaining = wi.projLiveTime;
            p->mStyle         = wi.style;
            p->addToGame(game, game->getGameObjDatabase());
         }
         break;

      case WeaponBurst:   // Grenade / Rocket / Bomb (area explosion)
         {
            Burst *b = new Burst(firePos, projVel, shooter);
            b->addToGame(game, game->getGameObjDatabase());
         }
         break;

      case WeaponSeeker:  // Missile (guided)
         {
            Seeker *s = new Seeker(firePos, projVel, dir.ATAN2(), shooter);
            s->addToGame(game, game->getGameObjDatabase());
         }
         break;

      default:
         break;
   }
}

};


