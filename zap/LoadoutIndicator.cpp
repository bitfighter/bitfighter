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
}



// Returns width of indicator component
static S32 renderComponentIndicator(S32 xPos, const char *name)
{
   S32 yPos = UserInterface::vertMargin;

   // Draw the weapon or module name
   S32 textWidth = drawStringAndGetWidth(xPos + indicatorPadding, yPos + indicatorPadding, indicatorFontSize, name);

   S32 rectWidth  = textWidth + 2 * indicatorPadding;
   S32 rectHeight = UserInterface::vertMargin + indicatorFontSize + 2 * indicatorPadding + 1;

   drawHollowRect(xPos, UserInterface::vertMargin, xPos + rectWidth, rectHeight);

   return rectWidth;
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

   static const Color *INDICATOR_INACTIVE_COLOR = &Colors::green80;      
   static const Color *INDICATOR_ACTIVE_COLOR   = &Colors::red80;        
   static const Color *INDICATOR_PASSIVE_COLOR  = &Colors::yellow;

   U32 xPos = UserInterface::horizMargin;

   // First, the weapons
   for(U32 i = 0; i < (U32)ShipWeaponCount; i++)
   {
      glColor(i == localShip->mActiveWeaponIndx ? INDICATOR_ACTIVE_COLOR : INDICATOR_INACTIVE_COLOR);

      S32 width = renderComponentIndicator(xPos, GameWeapon::weaponInfo[localShip->getWeapon(i)].name.getString());

      xPos += width + indicatorPadding;
   }

   xPos += 20;    // Small horizontal gap to seperate the weapon indicators from the module indicators

   // Next, loadout modules
   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
   {
      if(gModuleInfo[localShip->getModule(i)].getPrimaryUseType() != ModulePrimaryUseActive)
      {
         if(gModuleInfo[localShip->getModule(i)].getPrimaryUseType() == ModulePrimaryUseHybrid &&
               localShip->isModulePrimaryActive(localShip->getModule(i)))
            glColor(INDICATOR_ACTIVE_COLOR);
         else
            glColor(INDICATOR_PASSIVE_COLOR);
      }
      else if(localShip->isModulePrimaryActive(localShip->getModule(i)))
         glColor(INDICATOR_ACTIVE_COLOR);
      else 
         glColor(INDICATOR_INACTIVE_COLOR);

      // Always change to orange if module secondary is fired
      if(gModuleInfo[localShip->getModule(i)].hasSecondary() &&
            localShip->isModuleSecondaryActive(localShip->getModule(i)))
         glColor(Colors::orange67);

      S32 width = renderComponentIndicator(xPos, game->getModuleInfo(localShip->getModule(i))->getName());

      xPos += width + indicatorPadding;
   }
}

}