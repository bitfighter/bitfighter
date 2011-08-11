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
   EngineeredObjectCount
};


enum ModuleUseType
{
   ModuleUseActive,     // Only functional when active
   ModuleUsePassive,    // Always functional
   ModuleUseHybrid      // Always functional, with an active component
};

class ModuleInfo
{
private:
   ShipModule mModuleType;
   S32 mEnergyDrain;       // Continuous energy drain while in use
   S32 mUseCost;           // Per use energy drain
   const char *mName;
   const char *mMenuName;
   const char *mMenuHelp;
   ModuleUseType mUseType; // How module is activated

public:
   ModuleInfo(ShipModule moduleType)       // Constructor
   {
      switch(moduleType)
      {
         case ModuleShield:
            mName = "Shield";
            mMenuName = "Shield Generator";
            mMenuHelp = "";
            mEnergyDrain = 27000;
            mUseCost = 0;
            mUseType = ModuleUseActive;
            break;

         case ModuleBoost:
            mName = "Turbo";
            mMenuName = "Turbo Boost";
            mMenuHelp = "";
            mEnergyDrain = 15000;
            mUseCost = 0;
            mUseType = ModuleUseActive;
            break;

         case ModuleSensor:
            mName = "Sensor";
            mMenuName = "Enhanced Sensor";
            mMenuHelp = "(makes Spy Bug Placer available)";
            mEnergyDrain = 8000;
            mUseCost = 0;
            mUseType = ModuleUseHybrid;
            break;

         case ModuleRepair:
            mName = "Repair";
            mMenuName = "Repair Module";
            mMenuHelp = "";
            mEnergyDrain = 15000;
            mUseCost = 0;
            mUseType = ModuleUseActive;
            break;

         case ModuleEngineer:
            mName = "Engineer";
            mMenuName = "Engineer";
            mMenuHelp = "";
            mEnergyDrain = 0;
            mUseCost = 75000;
            mUseType = ModuleUseActive;
            break;

         case ModuleCloak:
            mName = "Cloak";
            mMenuName = "Cloak Field Modulator";
            mMenuHelp = "";
            mEnergyDrain = 8000;
            mUseCost = 0;
            mUseType = ModuleUseActive;
            break;

         case ModuleArmor:
            mName = "Armor";
            mMenuName = "Armor";
            mMenuHelp = "";
            mEnergyDrain = 0;
            mUseCost = 0;
            mUseType = ModuleUsePassive;
            break;

         default:
            TNLAssert(false, "Something's gone wrong again!");
            break;
      }
   };

   ShipModule getModuleType() { return mModuleType; }
   S32 getEnergyDrain() { return mEnergyDrain; }
   S32 getPerUseCost() { return mUseCost; }
   const char *getName() { return mName; }
   ModuleUseType getUseType() { return mUseType; }
   const char *getMenuName() { return mMenuName; }
   const char *getMenuHelp() { return mMenuHelp; }
};


};
#endif


