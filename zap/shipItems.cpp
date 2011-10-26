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
#include "shipItems.h"

using namespace TNL;

namespace Zap
{

const ModuleInfo gModuleInfo[ModuleCount] =
{
   // name,     drain,  cost,          type,           has2,  2cost,2cooldown,    menu name,            menu help text (renders in cyan)
	{"Shield",   27000,     0, ModulePrimaryUseActive,  false,     0,     0,   "Shield Generator",      ""                                },
	{"Turbo",    15000,     0, ModulePrimaryUseActive,  false,     0,     0,   "Turbo Boost",           ""                                },
	{"Sensor",    8000,     0, ModulePrimaryUseHybrid,  true,  35000,   800,   "Enhanced Sensor",       ""                                },
	{"Repair",   15000,     0, ModulePrimaryUseActive,  false,     0,     0,   "Repair Module",         ""                                },
	{"Engineer",     0, 75000, ModulePrimaryUseActive,  false,     0,     0,   "Engineer",              ""                                },
	{"Cloak",     8000,     0, ModulePrimaryUseActive,  false,     0,     0,   "Cloak Field Modulator", ""                                },
	{"Armor",        0,     0, ModulePrimaryUsePassive, false,     0,     0,   "Armor",                 "(makes ship harder to control)"  }
};


S32 ModuleInfo::getPrimaryEnergyDrain() const
{
   return mPrimaryEnergyDrain;
}


S32 ModuleInfo::getPrimaryPerUseCost() const
{
   return mPrimaryUseCost;
}


bool ModuleInfo::hasSecondary() const
{
   return hasSecondaryComponent;
}


S32 ModuleInfo::getSecondaryPerUseCost() const
{
   return mSecondaryUseCost;
}


S32 ModuleInfo::getSecondaryCooldown() const
{
   return mSecondaryCooldown;
}


const char *ModuleInfo::getName() const
{
   return mName;
}


ModulePrimaryUseType ModuleInfo::getPrimaryUseType() const
{
   return mPrimaryUseType;
}


const char *ModuleInfo::getMenuName() const
{
   return mMenuName;
}


const char *ModuleInfo::getMenuHelp() const
{
   return mMenuHelp;
}


}
