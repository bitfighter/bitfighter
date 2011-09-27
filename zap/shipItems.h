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

class ModuleInfo
{
private:
   ShipModule mModuleType;
   S32 mPrimaryEnergyDrain;       // Continuous energy drain while primary component is in use
   S32 mPrimaryUseCost;           // Per use energy drain of primary component (if it has one)
   S32 mSecondaryUseCost;         // Per use energy drain of secondary component
   S32 mSecondaryCooldown;        // Cooldown between allowed secondary component uses, in milliseconds
   const char *mName;
   const char *mMenuName;
   const char *mMenuHelp;
   ModulePrimaryUseType mPrimaryUseType; // How the primary component of the module is activated

public:
   ModuleInfo(ShipModule moduleType)       // Constructor
   {
      switch(moduleType)
      {
         case ModuleShield:
            mName = "Shield";
            mMenuName = "Shield Generator";
            mMenuHelp = "";
            mPrimaryEnergyDrain = 27000;
            mPrimaryUseCost = 0;
            mPrimaryUseType = ModulePrimaryUseActive;
            mSecondaryUseCost = 0;
            mSecondaryCooldown = 1000;
            break;

         case ModuleBoost:
            mName = "Turbo";
            mMenuName = "Turbo Boost";
            mMenuHelp = "";
            mPrimaryEnergyDrain = 15000;
            mPrimaryUseCost = 0;
            mPrimaryUseType = ModulePrimaryUseActive;
            mSecondaryUseCost = 0;
            mSecondaryCooldown = 1000;
            break;

         case ModuleSensor:
            mName = "Sensor";
            mMenuName = "Enhanced Sensor";
            mMenuHelp = "(makes Spy Bug Placer available)";
            mPrimaryEnergyDrain = 8000;
            mPrimaryUseCost = 0;
            mPrimaryUseType = ModulePrimaryUseHybrid;
            mSecondaryUseCost = 35000;
            mSecondaryCooldown = 800;
            break;

         case ModuleRepair:
            mName = "Repair";
            mMenuName = "Repair Module";
            mMenuHelp = "";
            mPrimaryEnergyDrain = 15000;
            mPrimaryUseCost = 0;
            mPrimaryUseType = ModulePrimaryUseActive;
            mSecondaryUseCost = 0;
            mSecondaryCooldown = 1000;
            break;

         case ModuleEngineer:
            mName = "Engineer";
            mMenuName = "Engineer";
            mMenuHelp = "";
            mPrimaryEnergyDrain = 0;
            mPrimaryUseCost = 75000;
            mPrimaryUseType = ModulePrimaryUseActive;
            mSecondaryUseCost = 0;
            mSecondaryCooldown = 1000;
            break;

         case ModuleCloak:
            mName = "Cloak";
            mMenuName = "Cloak Field Modulator";
            mMenuHelp = "";
            mPrimaryEnergyDrain = 8000;
            mPrimaryUseCost = 0;
            mPrimaryUseType = ModulePrimaryUseActive;
            mSecondaryUseCost = 0;
            mSecondaryCooldown = 1000;
            break;

         case ModuleArmor:
            mName = "Armor";
            mMenuName = "Armor";
            mMenuHelp = "";
            mPrimaryEnergyDrain = 0;
            mPrimaryUseCost = 0;
            mPrimaryUseType = ModulePrimaryUsePassive;
            mSecondaryUseCost = 0;
            mSecondaryCooldown = 1000;
            break;

         default:
            TNLAssert(false, "Something's gone wrong again!");
            break;
      }
   };

   ShipModule getModuleType() { return mModuleType; }
   S32 getPrimaryEnergyDrain() { return mPrimaryEnergyDrain; }
   S32 getPrimaryPerUseCost() { return mPrimaryUseCost; }
   S32 getSecondaryPerUseCost() { return mSecondaryUseCost; }
   S32 getSecondaryCooldown() { return mSecondaryCooldown; }
   const char *getName() { return mName; }
   ModulePrimaryUseType getPrimaryUseType() { return mPrimaryUseType; }
   const char *getMenuName() { return mMenuName; }
   const char *getMenuHelp() { return mMenuHelp; }
};


};
#endif


