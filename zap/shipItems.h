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

#ifndef _SHIPITEMS_H_
#define _SHIPITEMS_H_

#include "WeaponInfo.h"    // For weapon defs for DefaultLoadout list
#include "../tnl/tnlTypes.h"

using namespace TNL;

namespace Zap
{
//   drain gets multiplied by milliseconds
//               enum           name,     drain, cost,       type,          menu name,             menu help text (renders in cyan)
#define MODULE_ITEM_TABLE \
   MODULE_ITEM(ModuleShield,  "Shield",      33,     0, ModuleUseActive,  "Shield Generator",      ""                              ) \
   MODULE_ITEM(ModuleBoost,   "Turbo",       21,     0, ModuleUseActive,  "Turbo Boost",           ""                              ) \
   MODULE_ITEM(ModuleSensor,  "Sensor",      14,     0, ModuleUseHybrid,  "Enhanced Sensor",       ""                              ) \
   MODULE_ITEM(ModuleRepair,  "Repair",      21,     0, ModuleUseActive,  "Repair Module",         ""                              ) \
   MODULE_ITEM(ModuleEngineer,"Engineer",     0, 75000, ModuleUseActive,  "Engineer",              ""                              ) \
   MODULE_ITEM(ModuleCloak,   "Cloak",       14,     0, ModuleUseActive,  "Cloak Field Modulator", ""                              ) \
   MODULE_ITEM(ModuleArmor,   "Armor",        0,     0, ModuleUsePassive, "Armor",                 "(makes ship harder to control)") \


// Define an enum from the first values in MODULE_ITEM_TABLE
enum ShipModule {
#define MODULE_ITEM(a, b, c, d, e, f, g) a,
   MODULE_ITEM_TABLE
#undef MODULE_ITEM
   ModuleCount, 
   ModuleNone
};



enum ModuleUseType
{
   ModuleUseActive,     // Only functional when active
   ModuleUsePassive,    // Always functional
   ModuleUseHybrid      // Always functional, with an active component
};


static const S32 ShipModuleCount = 2;                // Modules a ship can carry
static const S32 ShipWeaponCount = 3;                // Weapons a ship can carry
static const U8 DefaultLoadout[] = { ModuleBoost, ModuleShield, WeaponPhaser, WeaponMine, WeaponBurst };


struct ModuleInfo
{
   const char *mName;
   S32 mEnergyDrain;       // Continuous energy drain while active module is in use
   S32 mUseCost;           // Per use energy drain of module (if it has active use)
   ModuleUseType mUseType; // If the module is active, passive, both
   const char *mMenuName;
   const char *mMenuHelp;

   S32 getEnergyDrain() const;
   S32 getUseCost() const;
   const char *getName() const;
   ModuleUseType getUseType() const;
   const char *getMenuName() const;
   const char *getMenuHelp() const;
};

extern const ModuleInfo gModuleInfo[ModuleCount];

};
#endif


