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

// Fill gModuleInfo with data from MODULE_ITEM_TABLE
const ModuleInfo gModuleInfo[ModuleCount] = {
#define MODULE_ITEM(a, b, c, d, e, f, g, h, i) { b, c, d, e, f, g, h, i },
   MODULE_ITEM_TABLE
#undef MODULE_ITEM
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
