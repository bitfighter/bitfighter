//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LOADOUT_TRACKER_H_
#define _LOADOUT_TRACKER_H_

#include "tnlTypes.h"
#include "tnlVector.h"

#include "shipItems.h"
#include "WeaponInfo.h"

#include "LuaScriptRunner.h"

#include <string>

using namespace TNL;
using namespace std;

namespace Zap 
{

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
   virtual ~LoadoutTracker();

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

   void setModuleIndexPrimary(U32 moduleIndex, bool isActive);
   void setModuleIndexSecondary(U32 moduleIndex, bool isActive);

   void deactivateAllModules();

   void setActiveWeapon(U32 weaponIndex);

   bool isValid() const;
   bool isWeaponActive(U32 weaponIndex) const;

   WeaponType getWeapon(U32 weaponIndex) const;
   WeaponType getActiveWeapon() const;
   U32 getActiveWeaponIndex() const;

   void resetLoadout();    // Reset this loadout to its factory settings



   ShipModule getModule(U32 modIndex) const;

   bool hasModule(ShipModule mod) const;
   bool hasWeapon(WeaponType weapon) const;

   bool isModulePrimaryActive(ShipModule module) const;
   bool isModuleSecondaryActive(ShipModule module) const;

   Vector<U8> toU8Vector() const;
   string toString(bool compact) const;
};



}
#endif
