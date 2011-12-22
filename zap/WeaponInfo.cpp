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

#include "WeaponInfo.h"


namespace Zap
{


WeaponInfo::WeaponInfo(StringTableEntry _name, U32 _fireDelay, U32 _minEnergy, U32 _drainEnergy, U32 _projVelocity, S32 _projLiveTime,
           F32 _damageAmount, F32 _damageSelfMultiplier, bool _canDamageTeammate, ProjectileType _projectileType)
{
   name = _name;
   fireDelay = _fireDelay;
   minEnergy = _minEnergy;
   drainEnergy = _drainEnergy;
   projVelocity = _projVelocity;
   projLiveTime = _projLiveTime;
   damageAmount = _damageAmount;
   damageSelfMultiplier = _damageSelfMultiplier;
   canDamageTeammate = _canDamageTeammate;
   projectileType = _projectileType;
}

const char *WeaponInfo::getWeaponName(WeaponType weaponType)
{
   switch((S32)weaponType)
   {
      case WeaponPhaser:
         return "Phaser";
      case WeaponBounce:
         return "Bouncer";
      case WeaponTriple:
         return "Triple";
      case WeaponBurst:
         return "Burst";
      case WeaponHeatSeeker:
         return "Heat Seeker";
      case WeaponMine:
         return "Mine";
      case WeaponTurret:
         return "Turret";
      case WeaponSpyBug:
         return "Spy Bug";
   }

   return "INVALID WEAPON!";
}


/////////////////////////////////////
/////////////////////////////////////


ProjectileInfo::ProjectileInfo(Color _sparkColor1, Color _sparkColor2, Color _sparkColor3,
      Color _sparkColor4, Color _projColor1, Color _projColor2, F32 _scaleFactor,
      SFXProfiles _projectileSound, SFXProfiles _impactSound )
{
   sparkColors[0] = _sparkColor1;
   sparkColors[1] = _sparkColor2;
   sparkColors[2] = _sparkColor3;
   sparkColors[3] = _sparkColor4;
   projColors[0] = _projColor1;
   projColors[1] = _projColor2;
   scaleFactor = _scaleFactor;
   projectileSound = _projectileSound;
   impactSound = _impactSound;
}


};
