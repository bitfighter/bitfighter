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
#include "RenderUtils.h"

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


extern void drawVertLine  (S32 x,  S32 y1, S32 y2);
extern void drawHorizLine (S32 x1, S32 x2, S32 y );

void HelperMenu::drawItemMenu(S32 xPos, S32 yPos, S32 width, const char *title, const OverlayMenuItem *items, S32 count,
                              const char **legendText, const Color **legendColors, S32 legendCount)
{
   static const Color baseColor(Colors::red);

   const S32 MENU_PADDING = 5;         // Padding around outer edge of overlay

   TNLAssert(glIsEnabled(GL_BLEND), "Expect blending to be on");

   S32 displayItems = 0;

   // Count how many items we will be displaying -- some may be hidden
   for(S32 i = 0; i < count; i++)
      if(items[i].showOnMenu)
         displayItems++;

   // Frame the menu
   S32 interiorMenuHeight = MENU_FONT_SIZE + MENU_FONT_SPACING + MENU_PADDING +                       // Title
                            displayItems * (MENU_FONT_SIZE + MENU_FONT_SPACING) + 2 * MENU_PADDING +  // Menu items
                            (legendCount > 0 ? MENU_LEGEND_FONT_SIZE + 2 * MENU_FONT_SPACING : 0) +   // Legend
                            2 * MENU_PADDING +                                                        // Post-legend gap
                            MENU_LEGEND_FONT_SIZE;                                                    // Instructions at bottom

   S32 top1   = yPos - MENU_PADDING;
   S32 top2   = yPos - MENU_PADDING + MENU_FONT_SIZE + MENU_FONT_SPACING + MENU_PADDING;
   S32 bottom = yPos + interiorMenuHeight + MENU_PADDING;

   S32 interiorEdge = xPos + width + ITEM_INDENT + ITEM_HELP_PADDING + MENU_PADDING + MENU_PADDING;

   drawFilledRect(0, top1, interiorEdge, top2,   Color(.10),    0.80f);
   drawFilledRect(0, top2, interiorEdge, bottom, Colors::black, 0.80f);
   drawHollowRect(0, top1, interiorEdge, bottom, Color(.35,0,0));

   // Draw the title
   glColor(baseColor);
   drawString(xPos, yPos, MENU_FONT_SIZE, title);
   yPos += MENU_FONT_SIZE + MENU_FONT_SPACING + MENU_PADDING;


   // Determine whether to show keys or joystick buttons on menu
   GameSettings *settings = getGame()->getSettings();
   InputMode inputMode    = settings->getInputCodeManager()->getInputMode();
   bool showKeys = settings->getIniSettings()->showKeyboardKeys || inputMode == InputModeKeyboard;

   yPos += 2;    // Aesthetics -- these pixels will be compensated for below

   for(S32 i = 0; i < count; i++)
   {
      // Don't show an option that shouldn't be shown!
      if(!items[i].showOnMenu)
         continue;
      
      // Draw key controls for selecting the object to be created
      U32 joystickIndex = Joystick::SelectedPresetIndex;

      if(inputMode == InputModeJoystick)     // Only draw joystick buttons when in joystick mode
         JoystickRender::renderControllerButton(F32(xPos + (showKeys ? 5 : 25)), (F32)yPos, 
                                                joystickIndex, items[i].button, false);

      if(showKeys)
      {
         // Render key in white, or, if there is a legend, in the color of the adjacent item
         glColor(legendCount > 0 ? items[i].itemColor : &Colors::white); 
         JoystickRender::renderControllerButton((F32)xPos + 30, (F32)yPos, 
                                                joystickIndex, items[i].key, false);
      }

      glColor(items[i].itemColor);  

      S32 x = drawStringAndGetWidth(xPos + ITEM_INDENT, yPos, MENU_FONT_SIZE, items[i].name); 

      // Render help string, if one is available
      if(strcmp(items[i].help, "") != 0)
      {
         glColor(items[i].helpColor);    
         drawString(xPos + ITEM_INDENT + ITEM_HELP_PADDING + x, yPos, MENU_FONT_SIZE, items[i].help);
      }

      yPos += MENU_FONT_SIZE + MENU_FONT_SPACING;
   }

   if(legendCount > 0)
   {
      yPos += MENU_FONT_SPACING;
      S32 x = xPos + 20;
      for(S32 i = 0; i < legendCount; i++)
      {
         glColor(legendColors[i]);
         x += drawStringAndGetWidth(x, yPos, MENU_LEGEND_FONT_SIZE, legendText[i]);
      }

      yPos += MENU_LEGEND_FONT_SIZE + MENU_FONT_SPACING;
   }

   yPos += 2 * MENU_PADDING;

   yPos -= 2;    // Pixel compensation

   glColor(baseColor);

   // RenderedSize will be -1 if the button is not defined
   if(settings->getInputCodeManager()->getInputMode() == InputModeKeyboard)
      drawStringf(xPos, yPos, MENU_LEGEND_FONT_SIZE, 
                  "Press [%s] to cancel", InputCodeManager::inputCodeToString(KEY_ESCAPE));
   else
   {
      S32 butSize = JoystickRender::getControllerButtonRenderedSize(Joystick::SelectedPresetIndex, BUTTON_BACK);

      xPos += drawStringAndGetWidth(xPos, yPos, MENU_LEGEND_FONT_SIZE, "Press ") + 4;
      JoystickRender::renderControllerButton(F32(xPos + 4), F32(yPos), Joystick::SelectedPresetIndex, BUTTON_BACK, false);
      xPos += butSize;
      glColor(baseColor);
      drawString(xPos, yPos, MENU_LEGEND_FONT_SIZE, "to cancel");
   }
}


// Calculate the width of the widest item in items
S32 HelperMenu::getWidth(const OverlayMenuItem *items, S32 count)
{
   S32 width = -1;
   for(S32 i = 0; i < count; i++)
   {
      S32 w = getStringWidth(MENU_FONT_SIZE, items[i].name) + getStringWidth(MENU_FONT_SIZE, items[i].help);
      if(w > width)
         width = w;
   }

   return width;

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

   // Magic number that seems to work well... no matter that the real menu might be a different width... by
   // using this constant, menus appear at a consistent rate.
   F32 width = 400;     

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
