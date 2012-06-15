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

enum ShipModule
{
   ModuleShield,
   ModuleBoost,
   ModuleSensor,
   ModuleRepair,
   ModuleEngineer,
   ModuleCloak,
   ModuleArmor,
   ModuleCount,
   ModuleNone,
};


enum ModulePrimaryUseType
{
   ModulePrimaryUseActive,     // Only functional when active
   ModulePrimaryUsePassive,    // Always functional
   ModulePrimaryUseHybrid      // Always functional, with an active component
};


static const S32 ShipModuleCount = 2;                // Modules a ship can carry
static const S32 ShipWeaponCount = 3;                // Weapons a ship can carry
static const U8 DefaultLoadout[] = { ModuleBoost, ModuleShield, WeaponPhaser, WeaponMine, WeaponBurst };


struct ModuleInfo
{
   const char *mName;
   S32 mPrimaryEnergyDrain;       // Continuous energy drain while primary component is in use
   S32 mPrimaryUseCost;           // Per use energy drain of primary component (if it has one)
   ModulePrimaryUseType mPrimaryUseType; // How the primary component of the module is activated
   bool hasSecondaryComponent;
   S32 mSecondaryUseCost;         // Per use energy drain of secondary component
   const char *mMenuName;
   const char *mMenuHelp;

   S32 getPrimaryEnergyDrain() const;
   S32 getPrimaryPerUseCost() const;
   bool hasSecondary() const;
   S32 getSecondaryPerUseCost() const;
   const char *getName() const;
   ModulePrimaryUseType getPrimaryUseType() const;
   const char *getMenuName() const;
   const char *getMenuHelp() const;
};

extern const ModuleInfo gModuleInfo[ModuleCount];

};
#endif


