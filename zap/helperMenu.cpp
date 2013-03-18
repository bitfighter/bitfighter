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
#include "UIGame.h"              // For mGameUserInterface
#include "UIManager.h"
#include "UIInstructions.h"
#include "ClientGame.h"
#include "JoystickRender.h"
#include "RenderUtils.h"
#include "ScissorsManager.h"
#include "ScreenInfo.h"          // For getGameCanvasWidth()

#include "OpenglUtils.h"

namespace Zap
{

// Constructor
HelperMenu::HelperMenu()
{
   mAnimationTimer.setPeriod(150);    // Transition time, in ms
   mTransitionTimer.setPeriod(150);
   mTransitioning = false;
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


void HelperMenu::onActivated()    
{
   mAnimationTimer.invert();
   mActivating = true;
   mTransitioning = false;
   mTransitionTimer.clear();
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
   //if(getActivationAnimationTime() == 0)
   //   return 0;
   //else
      return mActivating ? mAnimationTimer.getFraction() : 1 - mAnimationTimer.getFraction();
}


S32 HelperMenu::calcInteriorEdge(S32 xPos, S32 width)
{
   return xPos + width + ITEM_INDENT + ITEM_HELP_PADDING + MENU_PADDING + MENU_PADDING;
}


extern void drawVertLine  (S32 x,  S32 y1, S32 y2);
extern void drawHorizLine (S32 x1, S32 x2, S32 y );

void HelperMenu::drawItemMenu(const char *title, const OverlayMenuItem *items, S32 count, const OverlayMenuItem *prevItems, S32 prevCount,
                              const char **legendText, const Color **legendColors, S32 legendCount)
{
   S32 yPos = MENU_TOP;

   static const Color baseColor(Colors::red);

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

   S32 topOfMenuItemRenderArea = yPos - MENU_PADDING + MENU_FONT_SIZE + MENU_FONT_SPACING + MENU_PADDING;
   S32 bottom = yPos + interiorMenuHeight + MENU_PADDING;

   // If we are transitioning between items of different sizes, we will gradually change the rendered size during the transition
   // Generally, the top of the menu will stay in place, while the bottom will be adjusted.  Therefore, lower items need
   // to be offset by the transitionOffset which we will calculate below.
   S32 transitionOffset = 0;

   if(mTransitionTimer.getCurrent() > 0)
      transitionOffset = (mOldBottom - bottom) * mTransitionTimer.getFraction();
   else
      mOldBottom = bottom;

   bottom += transitionOffset;

   S32 xPos = getLeftEdgeOfMenuPos();

   S32 interiorEdge = calcInteriorEdge(xPos, mWidth);

   const S32 CORNER_SIZE = 15;      
   S32 top = MENU_TOP - MENU_PADDING;      // Absolute top of menu as rendered

   Point p[] = { Point(0, top), Point(interiorEdge - CORNER_SIZE, top),     // Top
                 Point(interiorEdge, top + CORNER_SIZE),                    // Edge
                 Point(interiorEdge, bottom), Point(0, bottom) };           // Bottom

   Vector<Point> points(p, ARRAYSIZE(p));

   // Fill
   glColor(Colors::black, 0.70f);
   renderPointVector(&points, GL_POLYGON);

   // Border
   glColor(Color(.35,0,0));
   renderPointVector(&points, GL_LINE_STRIP);

   // Gray line
   glColor(Colors::gray20);
   drawHorizLine(20, interiorEdge - 20, topOfMenuItemRenderArea);

   // Draw the title
   glColor(baseColor);
   drawString(xPos, yPos, MENU_FONT_SIZE, title);
   yPos += MENU_FONT_SIZE + MENU_FONT_SPACING + MENU_PADDING;
   yPos += transitionOffset;

   bool hasLegend = legendCount > 0;

   yPos += drawMenuItems(items, count, bottom, true, hasLegend);

   if(prevItems && mTransitionTimer.getCurrent() > 0)
      drawMenuItems(prevItems, prevCount, bottom, false, hasLegend);

   ///// 
   // Legend

   if(hasLegend)
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

   //yPos -= 2;    // Pixel compensation

   /////
   // Bottom of menu

   glColor(baseColor);

   // RenderedSize will be -1 if the button is not defined
   if(getGame()->getSettings()->getInputCodeManager()->getInputMode() == InputModeKeyboard)
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


extern ScreenInfo gScreenInfo;

// Render a set of menu items.  Break this code out to make transitions easier.
S32 HelperMenu::drawMenuItems(const OverlayMenuItem *items, S32 count, S32 bottom, bool newItems, bool renderKeysWithItemColor)
{
   S32 height = (MENU_FONT_SIZE + MENU_FONT_SPACING) * count;
   S32 height7 = (MENU_FONT_SIZE + MENU_FONT_SPACING) * 7;

   S32 xPos = getLeftEdgeOfMenuPos();
   S32 yPos = MENU_TOP + MENU_FONT_SIZE + MENU_FONT_SPACING + MENU_PADDING;

   static ScissorsManager scissorsManager;

   scissorsManager.enable(mTransitionTimer.getCurrent() > 0, getGame(), 0, yPos, 
                          gScreenInfo.getGameCanvasWidth(), bottom - yPos - (4 * MENU_PADDING + MENU_LEGEND_FONT_SIZE));

   yPos += mTransitionTimer.getFraction() * height7 - (newItems ? 0 : height);

   // Determine whether to show keys or joystick buttons on menu
   GameSettings *settings = getGame()->getSettings();
   InputMode inputMode    = settings->getInputCodeManager()->getInputMode();
   bool showKeys = settings->getIniSettings()->showKeyboardKeys || inputMode == InputModeKeyboard;

   yPos += 2;    // Aesthetics

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
         glColor(renderKeysWithItemColor ? items[i].itemColor : &Colors::white); 
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

   scissorsManager.disable();

   return height;
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


S32 HelperMenu::getAnimationPeriod() const
{
   return mAnimationTimer.getPeriod();
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

   if(mTransitionTimer.update(deltaT))
      mTransitioning = false;
}


// Used for loadout-style menus
S32 HelperMenu::getLeftEdgeOfMenuPos()
{
   // Magic number that seems to work well... no matter that the real menu might be a different width... by
   // using this constant, menus appear at a consistent rate.
   F32 width = 400;     

   return UserInterface::horizMargin - width + (mActivating ? width - mAnimationTimer.getFraction() * width : 
                                                              mAnimationTimer.getFraction() * width);
}


};
