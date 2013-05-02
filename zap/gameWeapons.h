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

#ifndef _GAMEWEAPONS_H_
#define _GAMEWEAPONS_H_

#include "WeaponInfo.h"
#include "SoundSystemEnums.h"
#include "Color.h"
#include "Point.h"
//#include "SoundSystem.h"

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
   static ProjectileInfo projectileInfo[ProjectileTypeCount];

   static void createWeaponProjectiles(WeaponType weapon, const Point &dir, const Point &shooterPos,
         const Point &shooterVel, S32 time, F32 shooterRadius, BfObject *shooter);
};


};

#endif


