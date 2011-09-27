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

namespace Zap
{

const ModuleInfo gModuleInfo[ModuleCount] =
{
   // name,     drain,  cost,          type,        2cost, 2cooldown, menu name,        help name
	{"Shield",   27000,     0, ModulePrimaryUseActive,      0,  1000, "Shield Generator",      ""  },
	{"Turbo",    15000,     0, ModulePrimaryUseActive,      0,  1000, "Turbo Boost",           ""  },
	{"Sensor",    8000,     0, ModulePrimaryUseHybrid,  35000,   800, "Enhanced Sensor",       ""  },
	{"Repair",   15000,     0, ModulePrimaryUseActive,      0,  1000, "Repair Module",         ""  },
	{"Engineer",     0, 75000, ModulePrimaryUseActive,      0,  1000, "Engineer",              ""  },
	{"Cloak",     8000,     0, ModulePrimaryUseActive,      0,  1000, "Cloak Field Modulator", ""  },
	{"Armor",        0,     0, ModulePrimaryUsePassive,     0,  1000, "ModuleArmor",           ""  }
};

}
