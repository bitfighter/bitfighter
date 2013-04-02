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
#include "ScreenInfo.h"
#include "ClientGame.h"
#include "OpenglUtils.h"
#include "UI.h"
#include "shipItems.h"     // For ShipModuleCount, ShipWeaponCount


using namespace Zap;

namespace UI
{


// Constructor
LoadoutIndicator::LoadoutIndicator()
{
   // Do nothing
   mScrollTimer.setPeriod(200);
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


static void doRender(ShipModule *modules, WeaponType *weapons, Ship *ship, ClientGame *game, S32 top)
{
   if(modules[0] == ModuleNone)
      return;

   static const Color *INDICATOR_INACTIVE_COLOR = &Colors::green80;      
   static const Color *INDICATOR_ACTIVE_COLOR   = &Colors::red80;        
   static const Color *INDICATOR_PASSIVE_COLOR  = &Colors::yellow;

   U32 xPos = UserInterface::horizMargin;
   
   // First, the weapons
   for(U32 i = 0; i < (U32)ShipWeaponCount; i++)
   {
      glColor(i == ship->mActiveWeaponIndx ? INDICATOR_ACTIVE_COLOR : INDICATOR_INACTIVE_COLOR);

      S32 width = renderComponentIndicator(xPos, top, GameWeapon::weaponInfo[weapons[i]].name.getString());

      xPos += width + indicatorPadding;
   }

   xPos += 20;    // Small horizontal gap to seperate the weapon indicators from the module indicators

   // Next, loadout modules
   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
   {
      if(gModuleInfo[modules[i]].getPrimaryUseType() != ModulePrimaryUseActive)
      {
         if(gModuleInfo[modules[i]].getPrimaryUseType() == ModulePrimaryUseHybrid &&
               ship->isModulePrimaryActive(modules[i]))
            glColor(INDICATOR_ACTIVE_COLOR);
         else
            glColor(INDICATOR_PASSIVE_COLOR);
      }
      else if(ship->isModulePrimaryActive(modules[i]))
         glColor(INDICATOR_ACTIVE_COLOR);
      else 
         glColor(INDICATOR_INACTIVE_COLOR);

      // Always change to orange if module secondary is fired
      if(gModuleInfo[modules[i]].hasSecondary() &&
            ship->isModuleSecondaryActive(modules[i]))
         glColor(Colors::orange67);

      S32 width = renderComponentIndicator(xPos, top, game->getModuleInfo(modules[i])->getName());

      xPos += width + indicatorPadding;
   }
}


// Draw weapon indicators at top of the screen, runs on client
void LoadoutIndicator::render(ClientGame *game)
{
   if(!game->getSettings()->getIniSettings()->showWeaponIndicators)      // If we're not drawing them, we've got nothing to do
      return;

   if(!game->getConnectionToServer())     // Can happen when first joining a game.  This was XelloBlue's crash...
      return;

   Ship *localShip = dynamic_cast<Ship *>(game->getConnectionToServer()->getControlObject());
   if(!localShip)
      return;

   const S32 indicatorTop    = UserInterface::vertMargin;      // Top of indicator y-pos
   const S32 indicatorHeight = getIndicatorHeight();
   S32 top;

   // Old loadout
   top = Parent::prepareToRenderFromDisplay(game, indicatorTop - 1, indicatorHeight + 1);
   if(top != NO_RENDER)
   {
      doRender(localShip->getOldModules(), localShip->getOldWeapons(), localShip, game, top);
      doneRendering();
   }

   // Current loadout
   top = Parent::prepareToRenderToDisplay(game, indicatorTop, indicatorHeight);
   doRender(localShip->getModules(), localShip->getWeapons(), localShip, game, top);
   doneRendering();

}


}