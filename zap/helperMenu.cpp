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
#include "UIManager.h"
#include "UIInstructions.h"
#include "ClientGame.h"
#include "JoystickRender.h"

#include "OpenglUtils.h"

namespace Zap
{

// Constructor
HelperMenu::HelperMenu(ClientGame *clientGame)
{
   mClientGame = clientGame;
}

// Destructor
HelperMenu::~HelperMenu()
{
   // Do nothing
}


const char *HelperMenu::getCancelMessage()
{
   return "";
}


InputCode HelperMenu::getActivationKey()
{
   return KEY_NONE;
}


// Exit helper mode by entering play mode.  Only one mode is active at a time.
void HelperMenu::exitHelper() 
{ 
   mClientGame->enterMode(PlayMode); 
}


ClientGame *HelperMenu::getGame()
{
   return mClientGame;
}


// Returns true if key was handled, false if it should be further processed
bool HelperMenu::processInputCode(InputCode inputCode)
{
   // First, check navigation keys.  When in keyboard mode, we allow the loadout key to toggle menu on and off...
   // we can't do this in joystick mode because it is likely that the loadout key is also used to select items
   // from the loadout menu.
   if(inputCode == KEY_ESCAPE  || inputCode == KEY_BACKSPACE    ||
      inputCode == KEY_LEFT    || inputCode == BUTTON_DPAD_LEFT ||
      inputCode == BUTTON_BACK || 
      (getGame()->getSettings()->getInputCodeManager()->getInputMode() == InputModeKeyboard && inputCode == getActivationKey()) )
   {
      exitHelper();      

      if(getGame()->getSettings()->getIniSettings()->verboseHelpMessages)
         mClientGame->displayMessage(Colors::paleRed, getCancelMessage());

      return true;
   }

   return false;
}


void HelperMenu::drawMenuBorderLine(S32 yPos, const Color &color)
{
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   F32 vertices[] = {
         UserInterface::horizMargin, yPos + 20,
         400, yPos + 20
   };
   F32 colors[] = {
         color.r, color.g, color.b, 1,
         color.r, color.g, color.b, 0,
   };
   renderColorVertexArray(vertices, colors, ARRAYSIZE(vertices) / 2, GL_LINES);
}


void HelperMenu::drawMenuCancelText(S32 yPos, const Color &color, S32 fontSize)
{
   S32 butSize = JoystickRender::getControllerButtonRenderedSize(Joystick::SelectedPresetIndex, BUTTON_BACK);
   const S32 fontSizeSm = fontSize - 4;

   glColor(color);

   GameSettings *settings = getGame()->getSettings();

   // RenderedSize will be -1 if the button is not defined
   if(settings->getInputCodeManager()->getInputMode() == InputModeKeyboard || butSize == -1)
      UserInterface::drawStringf(UserInterface::horizMargin, yPos, fontSizeSm, 
                                 "Press [%s] to cancel", InputCodeManager::inputCodeToString(KEY_ESCAPE));
   else
   {
      S32 xPos = UserInterface::horizMargin;
      xPos += UserInterface::drawStringAndGetWidth(xPos, yPos, fontSizeSm, "Press ") + 4;
      JoystickRender::renderControllerButton(F32(xPos + 4), F32(yPos), Joystick::SelectedPresetIndex, BUTTON_BACK, false);
      xPos += butSize;
      glColor(color);
      UserInterface::drawString( xPos, yPos, fontSizeSm, "to cancel");
   }
}


void HelperMenu::activateHelp(UIManager *uiManager)
{
    uiManager->activate(InstructionsUI);
}


bool HelperMenu::isEngineerHelper()
{
   return false;
}


bool HelperMenu::isMovementDisabled()
{
   return false;
}


void HelperMenu::idle(U32 delta) { /* Do nothing */ }
void HelperMenu::onMenuShow() { /* Do nothing */  }


};
