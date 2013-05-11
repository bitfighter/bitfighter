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

#include "tnlTypes.h"
#include "tnlAssert.h"
#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{
/**
 * @luaenum Module(1, 7)
 * The Module enum can be used to represent different module types.
 */

//   Drain gets multiplied by milliseconds
//               enum           name,     drain, cost,       type,               has2,   2cost,  menu name,             menu help text (renders in cyan)
#define MODULE_ITEM_TABLE \
   MODULE_ITEM(ModuleShield,  "Shield",      40,     0, ModulePrimaryUseActive,  false,     0, "Shield Generator",      ""                              ) \
   MODULE_ITEM(ModuleBoost,   "Turbo",       21,     0, ModulePrimaryUseActive,  true,      0, "Turbo Boost",           ""                              ) \
   MODULE_ITEM(ModuleSensor,  "Sensor",       0, 35000, ModulePrimaryUseHybrid,  false,     0, "Enhanced Sensor",       ""                              ) \
   MODULE_ITEM(ModuleRepair,  "Repair",      21,     0, ModulePrimaryUseActive,  false,     0, "Repair Module",         ""                              ) \
   MODULE_ITEM(ModuleEngineer,"Engineer",     0, 75000, ModulePrimaryUseActive,  false,     0, "Engineer",              ""                              ) \
   MODULE_ITEM(ModuleCloak,   "Cloak",       14,     0, ModulePrimaryUseActive,  false,     0, "Cloak Field Modulator", ""                              ) \
   MODULE_ITEM(ModuleArmor,   "Armor",        0,     0, ModulePrimaryUsePassive, false,     0, "Armor",                 "(makes ship harder to control)") \


// Define an enum from the first values in MODULE_ITEM_TABLE
enum ShipModule {
#define MODULE_ITEM(a, b, c, d, e, f, g, h, i) a,
   MODULE_ITEM_TABLE
#undef MODULE_ITEM
   ModuleCount, 
   ModuleNone
};



enum ModulePrimaryUseType
{
   ModulePrimaryUseActive,     // Only functional when active
   ModulePrimaryUsePassive,    // Always functional
   ModulePrimaryUseHybrid      // Always functional, with an active component
};


static const S32 ShipModuleCount = 2;                // Modules a ship can carry
static const S32 ShipWeaponCount = 3;                // Weapons a ship can carry

static const string DefaultLoadout = "Turbo, Shield, Phaser, Mine, Burst";


struct ModuleInfo
{
   // Detection ranges for sensor against cloaked players
   static const S32 SensorCloakInnerDetectionDistance = 300;   // Max detection inside this radius
   static const S32 SensorCloakOuterDetectionDistance = 500;   // No detection outside this radius

   const char *mName;
   S32 mPrimaryEnergyDrain;       // Continuous energy drain while primary component is in use
   S32 mPrimaryUseCost;           // Per use energy drain of primary component (if it has one)
   ModulePrimaryUseType mPrimaryUseType; // How the primary component of the module is activated
   bool hasSecondaryComponent;
   S32 mSecondaryUseCost;         // Per use energy drain of secondary component
   const char *mMenuName;
   const char *mMenuHelp;

   S32 getPrimaryEnergyDrain() const;
   S32 getPrimaryPerUseCost()  const;
   bool hasSecondary() const;
   S32 getSecondaryPerUseCost() const;
   ModulePrimaryUseType getPrimaryUseType() const;

   const char *getName()     const;
   const char *getMenuName() const;
   const char *getMenuHelp() const;

   static const ModuleInfo *getModuleInfo(ShipModule module);
};

extern const ModuleInfo gModuleInfo[ModuleCount];


};
#endif


