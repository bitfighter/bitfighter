//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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
