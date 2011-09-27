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

#include "../tnl/tnlAssert.h"

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


// Things you can build with Engineer 
enum EngineerBuildObjects
{
   EngineeredTurret,
   EngineeredForceField,
   EngineeredItemCount
};


enum ModulePrimaryUseType
{
   ModulePrimaryUseActive,     // Only functional when active
   ModulePrimaryUsePassive,    // Always functional
   ModulePrimaryUseHybrid      // Always functional, with an active component
};

struct ModuleInfo
{
   const char *mName;
   TNL::S32 mPrimaryEnergyDrain;       // Continuous energy drain while primary component is in use
   TNL::S32 mPrimaryUseCost;           // Per use energy drain of primary component (if it has one)
   ModulePrimaryUseType mPrimaryUseType; // How the primary component of the module is activated
   TNL::S32 mSecondaryUseCost;         // Per use energy drain of secondary component
   TNL::S32 mSecondaryCooldown;        // Cooldown between allowed secondary component uses, in milliseconds
   const char *mMenuName;
   const char *mMenuHelp;

   TNL::S32 getPrimaryEnergyDrain() const { return mPrimaryEnergyDrain; }
   TNL::S32 getPrimaryPerUseCost() const { return mPrimaryUseCost; }
   TNL::S32 getSecondaryPerUseCost() const { return mSecondaryUseCost; }
   TNL::S32 getSecondaryCooldown() const { return mSecondaryCooldown; }
   const char *getName() const { return mName; }
   ModulePrimaryUseType getPrimaryUseType() const { return mPrimaryUseType; }
   const char *getMenuName() const { return mMenuName; }
   const char *getMenuHelp() const { return mMenuHelp; }
};

extern const ModuleInfo gModuleInfo[ModuleCount];

};
#endif


