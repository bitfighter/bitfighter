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
HelperMenu::HelperMenu()
{
   mAnimationTimer.setPeriod(getActivationAnimationTime());    // Transition time, in ms
}


// Destructor
HelperMenu::~HelperMenu()
{
   // Do nothing
}


void HelperMenu::initialize(ClientGame *game, HelperManager *manager)
{
   mHelperManager = manager;
   mClientGame = game;
}


const char *HelperMenu::getCancelMessage()
{
   return "";
}


InputCode HelperMenu::getActivationKey()
{
   return KEY_NONE;
}


// Exit helper mode by entering play mode
void HelperMenu::exitHelper() 
{ 
   mAnimationTimer.invert();
   mActivating = false;
   mClientGame->getUIManager()->getGameUserInterface()->exitHelper();
}


F32 HelperMenu::getFraction()
{
   if(getActivationAnimationTime() == 0)
      return 0;
   else
      return mActivating ? mAnimationTimer.getFraction() : 1 - mAnimationTimer.getFraction();
}


F32 HelperMenu::getHelperWidth() const
{
   return 400;
}


void HelperMenu::drawItemMenu(S32 xPos, S32 yPos, const char *title, const OverlayMenuItem *items, S32 count)
{
   static const Color baseColor(Colors::red);

   // Frame the menu
   S32 interiorMenuHeight = count * (MENU_FONT_SIZE + MENU_FONT_SPACING) + MENU_PADDING;

   drawMenuBorderLine(xPos, yPos,                      baseColor);
   drawMenuBorderLine(xPos, yPos + interiorMenuHeight, baseColor);

   // Draw the title
   glColor(baseColor);
   drawString(xPos, yPos, MENU_FONT_SIZE, title);
   yPos += MENU_FONT_SIZE + MENU_FONT_SPACING + MENU_PADDING;


   // Determine whether to show keys or joystick buttons on menu
   GameSettings *settings = getGame()->getSettings();
   InputMode inputMode    = settings->getInputCodeManager()->getInputMode();
   bool showKeys = settings->getIniSettings()->showKeyboardKeys || inputMode == InputModeKeyboard;

   for(S32 i = 0; i < count; i++)
   {
      // Don't show an option that shouldn't be shown!
      if(!items[i].showOnMenu)
         continue;

      // Draw key controls for selecting the object to be created
      U32 joystickIndex = Joystick::SelectedPresetIndex;

      if(inputMode == InputModeJoystick)     // Only draw joystick buttons when in joystick mode
         JoystickRender::renderControllerButton(F32(xPos + (showKeys ? 0 : 20)), (F32)yPos, 
                                                joystickIndex, items[i].button, false);

      if(showKeys)
      {
         glColor(Colors::white);             // Render key in white
         JoystickRender::renderControllerButton((F32)xPos + 20, (F32)yPos, 
                                                joystickIndex, items[i].key, false);
      }

      if(items[i].markAsSelected)
         glColor(1.0, 0.1f, 0.1f);      // Color of already selected item
      else
         glColor(0.1f, 1.0, 0.1f);      // Color of not-yet selected item


      S32 x = drawStringAndGetWidth(xPos + 50, yPos, MENU_FONT_SIZE, items[i].name); 

      // Render help string.  Highlight it if the item is not selected
      if(!items[i].markAsSelected)
         glColor(.2, .8, .8);    

      drawString(xPos + x, yPos, MENU_FONT_SIZE, items[i].help);

      yPos += MENU_FONT_SIZE + MENU_FONT_SPACING;
   }

   yPos += MENU_FONT_SIZE + MENU_FONT_SPACING + MENU_PADDING;

   drawMenuCancelText(xPos, yPos, baseColor, MENU_FONT_SIZE);
}


void HelperMenu::drawMenuBorderLine(S32 xPos, S32 yPos, const Color &color)
{
   TNLAssert(glIsEnabled(GL_BLEND), "Expect blending to be on");

   F32 vertices[] = {
         xPos,        yPos + 20,
         xPos + 400,  yPos + 20
   };
   F32 colors[] = {
         color.r, color.g, color.b, 1,
         color.r, color.g, color.b, 0,
   };
   renderColorVertexArray(vertices, colors, ARRAYSIZE(vertices) / 2, GL_LINES);
}


void HelperMenu::drawMenuCancelText(S32 xPos, S32 yPos, const Color &color, S32 fontSize)
{
   S32 butSize = JoystickRender::getControllerButtonRenderedSize(Joystick::SelectedPresetIndex, BUTTON_BACK);
   const S32 fontSizeSm = fontSize - 4;

   glColor(color);

   GameSettings *settings = getGame()->getSettings();

   // RenderedSize will be -1 if the button is not defined
   if(settings->getInputCodeManager()->getInputMode() == InputModeKeyboard || butSize == -1)
      drawStringf(xPos, yPos, fontSizeSm, 
                  "Press [%s] to cancel", InputCodeManager::inputCodeToString(KEY_ESCAPE));
   else
   {
      xPos += drawStringAndGetWidth(xPos, yPos, fontSizeSm, "Press ") + 4;
      JoystickRender::renderControllerButton(F32(xPos + 4), F32(yPos), Joystick::SelectedPresetIndex, BUTTON_BACK, false);
      xPos += butSize;
      glColor(color);
      drawString( xPos, yPos, fontSizeSm, "to cancel");
   }
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


void HelperMenu::onTextInput(char ascii)
{
   // Do nothing (overridden by ChatHelper)
}


void HelperMenu::activateHelp(UIManager *uiManager)
{
    uiManager->activate(InstructionsUI);
}


// Return true if menu is playing the closing animation
bool HelperMenu::isClosing() const
{
   return !mActivating && mAnimationTimer.getCurrent() > 0;
}


bool HelperMenu::isMovementDisabled() { return false; }
bool HelperMenu::isChatDisabled()     { return true;  }


void HelperMenu::idle(U32 deltaT) 
{
   if(mAnimationTimer.update(deltaT) && !mActivating)
      mHelperManager->doneClosingHelper();
}


S32 HelperMenu::getLeftEdgeOfMenuPos()
{
   S32 trueLeftEdge = UserInterface::horizMargin;

   if(getActivationAnimationTime() == 0)
      return trueLeftEdge;

   // else calculate where the left edge should be based on state of animation timer

   F32 width = getHelperWidth();
   return trueLeftEdge - width + (mActivating ? width - mAnimationTimer.getFraction() * width : 
                                                        mAnimationTimer.getFraction() * width);
}


void HelperMenu::onActivated()    
{
   mAnimationTimer.invert();
   mActivating = true;
}


// Duration of activation animation -- return 0 to disable
S32 HelperMenu::getActivationAnimationTime() { return 150; }


};
