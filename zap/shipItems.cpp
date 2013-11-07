//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
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


const ModuleInfo *ModuleInfo::getModuleInfo(ShipModule module)
{
   TNLAssert(U32(module) < U32(ModuleCount), "Module out of range!");
   return &gModuleInfo[(U32) module];
}


}
