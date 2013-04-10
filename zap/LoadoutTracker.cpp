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


#include "LoadoutTracker.h"

namespace Zap
{

// Constructor
LoadoutTracker::LoadoutTracker()
{
   for(S32 i = 0; i < ShipModuleCount; i++)
      mModules[i] = ModuleNone;

   for(S32 i = 0; i < ModuleCount; i++)
   {
      mModulePrimaryActive[i] = false;  
      mModuleSecondaryActive[i] = false;
   }   
   
   
   for(S32 i = 0; i < ShipWeaponCount; i++)
      mWeapons[i] = WeaponNone;

   mActiveWeapon = 0;
}


// Returns true if anything changed
bool LoadoutTracker::update(const LoadoutTracker &loadout)
{
   bool loadoutChanged = false;

   for(S32 i = 0; i < ShipModuleCount; i++)
      if(mModules[i] != loadout.mModules[i])
      {
         mModules[i] = loadout.mModules[i];
         loadoutChanged = true;
      }

   for(S32 i = 0; i < ModuleCount; i++)
   {
      mModulePrimaryActive[i] = loadout.mModulePrimaryActive[i];  
      mModuleSecondaryActive[i] = loadout.mModuleSecondaryActive[i];
   }

   for(S32 i = 0; i < ShipWeaponCount; i++)
      if(mWeapons[i] != loadout.mWeapons[i])
      {
         mWeapons[i] = loadout.mWeapons[i];
         loadoutChanged = true;
      }

   return loadoutChanged;
}


//
//bool LoadoutTracker::update(const ShipModule *modules, const WeaponType *weapons)
//{
//   bool loadoutChanged = false;
//
//   for(S32 i = 0; i < ShipModuleCount; i++)
//      if(modules[i] != mModules[i])
//      {
//         mModules[i]    = modules[i];
//         loadoutChanged = true;
//      }
//
//   for(S32 i = 0; i < ShipWeaponCount; i++)
//      if(weapons[i] != mWeapons[i])
//      {
//         mWeapons[i]    = weapons[i];
//         loadoutChanged = true;
//      }
//
//   return loadoutChanged;
//}


// Takes an array of U8s repesenting loadout... M,M,W,W,W.  See DefaultLoadout for an example
void LoadoutTracker::setLoadout(const U8 *items)
{
   for(S32 i = 0; i < ShipModuleCount; i++)
      mModules[i] = (ShipModule) items[i];

   for(S32 i = 0; i < ShipWeaponCount; i++)
      mWeapons[i] = (WeaponType) items[i + ShipModuleCount];
}


void LoadoutTracker::setModule(U32 moduleIndex, ShipModule module)
{
   mModules[moduleIndex] = module;
}


void LoadoutTracker::setWeapon(U32 weaponIndex, WeaponType weapon)
{
   mWeapons[weaponIndex] = weapon;
}


void LoadoutTracker::setActiveWeapon(U32 weaponIndex)
{
   mActiveWeapon = weaponIndex % ShipWeaponCount;
}


void LoadoutTracker::setModulePrimary(ShipModule module, bool isActive)
{
   mModulePrimaryActive[module] = isActive;
}


void LoadoutTracker::setModuleIndxPrimary(U32 moduleIndex, bool isActive)
{
   mModulePrimaryActive[mModules[moduleIndex]] = isActive;
}


void LoadoutTracker::setModuleSecondary(ShipModule module, bool isActive)
{
    mModuleSecondaryActive[module] = isActive;
}


void LoadoutTracker::setModuleIndxSecondary(U32 moduleIndex, bool isActive)
{
   mModuleSecondaryActive[mModules[moduleIndex]] = isActive;
}


void LoadoutTracker::deactivateAllModules()
{
   for(S32 i = 0; i < ModuleCount; i++)
   {
      mModulePrimaryActive[i] = false;
      mModuleSecondaryActive[i] = false;
   }
}


bool LoadoutTracker::hasModule(ShipModule mod) const
{
   for(U32 i = 0; i < ShipModuleCount; i++)
      if(mModules[i] == mod)
         return true;

   return false;
}


bool LoadoutTracker::isValid() const
{
   return mModules[0] != ModuleNone;
}


bool LoadoutTracker::isWeaponActive(U32 weaponIndex) const
{
   return weaponIndex == mActiveWeapon;
}


WeaponType LoadoutTracker::getWeapon(U32 weaponIndex) const
{
   return mWeapons[weaponIndex];
}


WeaponType LoadoutTracker::getCurrentWeapon() const
{
   return mWeapons[mActiveWeapon];
}


U32 LoadoutTracker::getCurrentWeaponIndx() const
{
   return mActiveWeapon;
}


ShipModule LoadoutTracker::getModule(U32 modIndex) const
{
   return mModules[modIndex];
}


bool LoadoutTracker::isModulePrimaryActive(ShipModule module) const
{
      return mModulePrimaryActive[module];
}


bool LoadoutTracker::isModuleSecondaryActive(ShipModule module) const
{
   return mModuleSecondaryActive[module];
}


}