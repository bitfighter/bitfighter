//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "WeaponInfo.h"
#ifndef BF_MASTER
#include "BfObject.h"
#include "projectile.h"
#endif


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

#ifndef BF_MASTER
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


BfObject *WeaponInfo::getWeaponShooterFromObject(BfObject *bfObject)
{
   if(bfObject == NULL)
      return NULL;

   U8 typeNumber = bfObject->getObjectTypeNumber();

   if(typeNumber == BulletTypeNumber)
      return static_cast<Projectile *>(bfObject)->getShooter();
   else if(typeNumber == BurstTypeNumber || typeNumber == MineTypeNumber || typeNumber == SpyBugTypeNumber)
      return static_cast<Burst *>(bfObject)->getShooter();
   else if(typeNumber == SeekerTypeNumber)
      return static_cast<Seeker *>(bfObject)->getShooter();

   return NULL;
}
#endif

};
