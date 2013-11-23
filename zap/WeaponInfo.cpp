//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "WeaponInfo.h"
#include "BfObject.h"
#include "projectile.h"


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


// Discover the WeaponType from any BfObject
// This method feels like a workaround for a bad object model...
WeaponType WeaponInfo::getWeaponTypeFromObject(BfObject *bfObject)
{
   U8 typeNumber = bfObject->getObjectTypeNumber();

   if(typeNumber == BulletTypeNumber)
      return static_cast<Projectile *>(bfObject)->mWeaponType;
   else if(typeNumber == BurstTypeNumber || typeNumber == MineTypeNumber || typeNumber == SpyBugTypeNumber)
      return static_cast<Burst *>(bfObject)->mWeaponType;
   else if(typeNumber == SeekerTypeNumber)
      return static_cast<Seeker *>(bfObject)->mWeaponType;

   return WeaponNone;
}


};
