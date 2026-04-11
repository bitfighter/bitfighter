//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAMEWEAPONS_H_
#define _GAMEWEAPONS_H_

#include "WeaponInfo.h"
#include "XtankShape.h"    // for XtankWeapon::Type, XtankWeaponInfo
#include "SoundSystemEnums.h"
#include "Color.h"
#include "Point.h"

#include "tnlTypes.h"

using namespace TNL;

namespace Zap
{

class BfObject;


static const U32 NumSparkColors = 4;

struct ProjectileInfo
{
   ProjectileInfo(Color _sparkColor1, Color _sparkColor2, Color _sparkColor3, Color _sparkColor4, Color _projColor1,
         Color _projColor2, F32 _scaleFactor, SFXProfiles _projectileSound, SFXProfiles _impactSound );
   virtual ~ProjectileInfo();

   Color       sparkColors[NumSparkColors];
   Color       projColors[2];
   F32         scaleFactor;
   SFXProfiles projectileSound;
   SFXProfiles impactSound;
};


////////////////////////////////////////
////////////////////////////////////////

struct GameWeapon
{
   static ProjectileInfo projectileInfo[ProjectileStyleCount];

   static void createWeaponProjectiles(WeaponType weapon, const Point &dir, const Point &shooterPos,
         const Point &shooterVel, S32 time, F32 shooterRadius, BfObject *shooter);

   // Create an xtank projectile fired from barrelTip in direction dir.
   // The projectile type and behavior are determined by the xtank weapon's
   // mapped bfWeapon.  shooterRadius should be 0 when barrelTip is already
   // the world-space muzzle position.
   static void createXtankProjectile(XtankWeapon weapon, const Point &dir,
         const Point &barrelTip, const Point &shooterVel, S32 time, BfObject *shooter);
};


};

#endif


