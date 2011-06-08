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


#include "helperMenu.h"
#include "UIGame.h"     // For mGameUserInterface

#include "SDL/SDL_opengl.h"

namespace Zap
{

// Exit helper mode by entering play mode.  Only one mode is active at a time.
void HelperMenu::exitHelper() 
{ 
   gClientGame->mGameUserInterface->enterMode(GameUserInterface::PlayMode); 
}


// Returns true if key was handled, false if it should be further processed
bool HelperMenu::processKeyCode(KeyCode keyCode)
{
   // First, check navigation keys.  When in keyboard mode, we allow the loadout key to toggle menu on and off...
   // we can't do this in joystick mode because it is likely that the loadout key is also used to select items
   // from the loadout menu.
   if(keyCode == KEY_ESCAPE || keyCode == KEY_BACKSPACE ||
      keyCode == KEY_LEFT   || keyCode == BUTTON_DPAD_LEFT ||
      keyCode == BUTTON_BACK || (gIniSettings.inputMode == Keyboard && keyCode == getActivationKey()) )
   {
      exitHelper();      // Return to play mode, ship design unchanged
      if(gIniSettings.verboseHelpMessages)
         gClientGame->mGameUserInterface->displayMessage(Color(1.0, 0.5, 0.5), getCancelMessage());

      return true;
   }

   return false;
}


extern void renderControllerButton(F32 x, F32 y, KeyCode keyCode, bool activated, S32 offset);
extern IniSettings gIniSettings;
extern S32 getControllerButtonRenderedSize(KeyCode keyCode);

void HelperMenu::drawMenuBorderLine(S32 yPos, const Color &color)
{
   glEnableBlend;
   glBegin(GL_LINES);
      glColor(color);
      glVertex2f(UserInterface::horizMargin, yPos + 20);
      glColor(color,0);    // Fade to transparent...
      glVertex2f(400, yPos + 20);
   glEnd();
   glDisableBlend;
}

void HelperMenu::drawMenuCancelText(S32 yPos, const Color &color, S32 fontSize)
{

   S32 butSize = getControllerButtonRenderedSize(BUTTON_BACK);
   const S32 fontSizeSm = fontSize - 4;

   glColor(color);

   // RenderedSize will be -1 if the button is not defined
   if(gIniSettings.inputMode == Keyboard || butSize == -1)
      UserInterface::drawStringf( UserInterface::horizMargin, yPos, fontSizeSm, "Press [%s] to cancel", keyCodeToString(KEY_ESCAPE) );
   else
   {
      S32 xPos = UserInterface::horizMargin;
      xPos += UserInterface::drawStringAndGetWidth( xPos, yPos, fontSizeSm, "Press ");
      renderControllerButton(xPos, yPos, BUTTON_BACK, false, butSize / 2);
      xPos += butSize;
      glColor(color);
      UserInterface::drawString( xPos, yPos, fontSizeSm, " to cancel");
   }
}

};
