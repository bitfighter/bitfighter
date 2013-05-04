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

#include "LoadoutIndicator.h"

#include "UI.h"
#include "ScreenInfo.h"
#include "ClientGame.h"
#include "FontManager.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"


using namespace Zap;

namespace Zap { namespace UI {


// Constructor
LoadoutIndicator::LoadoutIndicator()
{
   mScrollTimer.setPeriod(200);
}


void LoadoutIndicator::newLoadoutHasArrived(const LoadoutTracker &loadout)
{
   mPrevLoadout.update(mCurrLoadout);
   bool loadoutChanged = mCurrLoadout.update(loadout);

   if(loadoutChanged)
   {
      onActivated();
      resetScrollTimer();
   }
}


void LoadoutIndicator::setActiveWeapon(U32 weaponIndex)
{
   mCurrLoadout.setActiveWeapon(weaponIndex);
}


void LoadoutIndicator::setModulePrimary(ShipModule module, bool isActive)
{
   mCurrLoadout.setModulePrimary(module, isActive);
}


void LoadoutIndicator::setModuleSecondary(ShipModule module, bool isActive)
{
    mCurrLoadout.setModuleSecondary(module, isActive);
}


const LoadoutTracker *LoadoutIndicator::getLoadout() const
{
   return &mCurrLoadout;
}


static S32 getIndicatorHeight()
{
   return indicatorFontSize + 2 * indicatorPadding + 1;
}


// Returns width of indicator component
static S32 renderComponentIndicator(S32 xPos, S32 yPos, const char *name)
{
   // Draw the weapon or module name
   S32 textWidth = drawStringAndGetWidth(xPos + indicatorPadding, yPos + indicatorPadding, indicatorFontSize, name);

   S32 rectWidth  = textWidth + 2 * indicatorPadding;
   S32 rectHeight = getIndicatorHeight();

   drawHollowRect(xPos, yPos, xPos + rectWidth, yPos + rectHeight);

   return rectWidth;
}


static void doRender(const LoadoutTracker &loadout, ClientGame *game, S32 top)
{
   // If if we have no module, then this loadout has never been set, and there is nothing to render
   if(!loadout.isValid())  
      return;

   static const Color *INDICATOR_INACTIVE_COLOR = &Colors::green80;      
   static const Color *INDICATOR_ACTIVE_COLOR   = &Colors::red80;        
   static const Color *INDICATOR_PASSIVE_COLOR  = &Colors::yellow;

   U32 xPos = 10;

   FontManager::pushFontContext(LoadoutIndicatorContext);
   
   // First, the weapons
   for(U32 i = 0; i < (U32)ShipWeaponCount; i++)
   {
      glColor(loadout.isWeaponActive(i) ? INDICATOR_ACTIVE_COLOR : INDICATOR_INACTIVE_COLOR);

      S32 width = renderComponentIndicator(xPos, top, WeaponInfo::getWeaponInfo(loadout.getWeapon(i)).name.getString());

      xPos += width + indicatorPadding;
   }

   xPos += 20;    // Small horizontal gap to seperate the weapon indicators from the module indicators

   // Next, loadout modules
   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
   {
      ShipModule module = loadout.getModule(i);

      if(gModuleInfo[module].getPrimaryUseType() != ModulePrimaryUseActive)
      {
         if(gModuleInfo[module].getPrimaryUseType() == ModulePrimaryUseHybrid &&
               loadout.isModulePrimaryActive(module))
            glColor(INDICATOR_ACTIVE_COLOR);
         else
            glColor(INDICATOR_PASSIVE_COLOR);
      }
      else if(loadout.isModulePrimaryActive(module))
         glColor(INDICATOR_ACTIVE_COLOR);
      else 
         glColor(INDICATOR_INACTIVE_COLOR);

      // Always change to orange if module secondary is fired
      if(gModuleInfo[module].hasSecondary() && loadout.isModuleSecondaryActive(module))
         glColor(Colors::orange67);

      S32 width = renderComponentIndicator(xPos, top, ModuleInfo::getModuleInfo(module)->getName());

      xPos += width + indicatorPadding;
   }

   FontManager::popFontContext();
}


// Draw weapon indicators at top of the screen, runs on client
void LoadoutIndicator::render(ClientGame *game)
{
   if(!game->getSettings()->getIniSettings()->showWeaponIndicators)      // If we're not drawing them, we've got nothing to do
      return;

   if(!game->getConnectionToServer())     // Can happen when first joining a game.  This was XelloBlue's crash...
      return;

   const S32 indicatorTop    = 10;      // Top of indicator y-pos
   const S32 indicatorHeight = getIndicatorHeight();
   S32 top;

   // Old loadout
   top = Parent::prepareToRenderFromDisplay(game, indicatorTop - 1, indicatorHeight + 1);
   if(top != NO_RENDER)
   {
      doRender(mPrevLoadout, game, top);
      doneRendering();
   }

   // Current loadout
   top = Parent::prepareToRenderToDisplay(game, indicatorTop, indicatorHeight);
   doRender(mCurrLoadout, game, top);
   doneRendering();
}


} } // Nested namespace