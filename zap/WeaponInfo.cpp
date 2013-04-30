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

// Get the name of a weapon given its enum value -- only used for writing to the database
const char *WeaponInfo::getWeaponName(WeaponType weaponType)
{
   static const char *weaponName[] = {
#  define WEAPON_ITEM(a, name, c, d, e, f, g, h, i, j, k, l) name,
      WEAPON_ITEM_TABLE
#  undef WEAPON_ITEM
   };

   return weaponName[(S32)weaponType];
}


// Define a list of WeaponInfos
WeaponInfo weaponInfo[] = {
#  define WEAPON_ITEM(a, name, c, delay, minEn, drainEn, projVel, projTTL, damage, selfDamage, canDamTMs, projType) \
   { StringTableEntry(name), delay, minEn, drainEn, projVel, projTTL, damage, selfDamage, canDamTMs, projType },
      WEAPON_ITEM_TABLE
#  undef WEAPON_ITEM
};


WeaponInfo WeaponInfo::getWeaponInfo(WeaponType weaponType)
{
   return weaponInfo[weaponType];
}


};
