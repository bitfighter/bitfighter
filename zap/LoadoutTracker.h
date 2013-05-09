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

#ifndef _LOADOUT_TRACKER_H_
#define _LOADOUT_TRACKER_H_

#include "shipItems.h"
#include "WeaponInfo.h"

#include "tnlTypes.h"
#include "tnlVector.h"
#include <string>

using namespace TNL;
using namespace std;

namespace Zap 
{

class LuaLoadout;

class LoadoutTracker
{
private:
   ShipModule mModules[ShipModuleCount];     // Modules player's ship is carrying
   WeaponType mWeapons[ShipWeaponCount];     // Weapons player's ship is carrying

   U32 mActiveWeapon;      // Index of active weapon

   bool mModulePrimaryActive[ModuleCount];         // Is the primary component of the module active at this moment?
   bool mModuleSecondaryActive[ModuleCount];       // Is the secondary component of the module active?

public:
   LoadoutTracker();                            // Constructor
   LoadoutTracker(const string &loadoutStr);  
   LoadoutTracker(const Vector<U8> &loadout);

   bool operator == (const LoadoutTracker &other) const;
   bool operator != (const LoadoutTracker &other) const;

   bool update(const LoadoutTracker &tracker);

   // Set loadout in bulk
   void setLoadout(const Vector<U8> &items);   // Pass an array of U8s repesenting loadout... M,M,W,W,W
   void setLoadout(const string &loadoutStr);

   // Or set loadout a la carte
   void setModule(U32 moduleIndex, ShipModule module);
   void setWeapon(U32 weaponIndex, WeaponType weapon);

   void setModulePrimary(ShipModule module, bool isActive);
   void setModuleSecondary(ShipModule module, bool isActive);

   void setModuleIndxPrimary(U32 moduleIndex, bool isActive);
   void setModuleIndxSecondary(U32 moduleIndex, bool isActive);

   void deactivateAllModules();

   void setActiveWeapon(U32 weaponIndex);

   bool isValid() const;
   bool isWeaponActive(U32 weaponIndex) const;

   WeaponType getWeapon(U32 weaponIndex) const;
   WeaponType getActiveWeapon() const;
   U32 getActiveWeaponIndx() const;

   void resetLoadout();    // Reset this loadout to its factory settings



   ShipModule getModule(U32 modIndex) const;

   bool hasModule(ShipModule mod) const;
   bool hasWeapon(WeaponType weapon) const;

   bool isModulePrimaryActive(ShipModule module) const;
   bool isModuleSecondaryActive(ShipModule module) const;

   Vector<U8> toU8Vector() const;
   string toString() const;
};



}
#endif