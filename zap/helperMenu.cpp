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


extern void drawVertLine  (S32 x,  S32 y1, S32 y2);
extern void drawHorizLine (S32 x1, S32 x2, S32 y );

void HelperMenu::drawItemMenu(const char *title, const OverlayMenuItem *items, S32 count, const OverlayMenuItem *prevItems, S32 prevCount,
                              const char **legendText, const Color **legendColors, S32 legendCount)
{
   glPushMatrix();

   glTranslate(getLeftEdgeOfMenuPos(), 0, 0);

   static const Color baseColor(Colors::red);

   TNLAssert(glIsEnabled(GL_BLEND), "Expect blending to be on");

   S32 displayItems = 0;

   // Count how many items we will be displaying -- some may be hidden
   for(S32 i = 0; i < count; i++)
      if(items[i].showOnMenu)
         displayItems++;

   bool hasLegend = legendCount > 0;

   const S32 grayLineBuffer = 10;

   // Height of menu parts
   const S32 topPadding        = MENU_PADDING;
   const S32 titleHeight       = TITLE_FONT_SIZE + grayLineBuffer ;
   const S32 itemsHeight       = displayItems * (MENU_FONT_SIZE + MENU_FONT_SPACING) + MENU_PADDING + grayLineBuffer;
   const S32 legendHeight      = (hasLegend ? MENU_LEGEND_FONT_SIZE + MENU_FONT_SPACING : 0); 
   const S32 instructionHeight = MENU_LEGEND_FONT_SIZE;
   const S32 bottomPadding     = MENU_PADDING;

   // Total height of the menu
   const S32 totalHeight = topPadding + titleHeight + itemsHeight + legendHeight + instructionHeight + bottomPadding;     

   S32 yPos = MENU_TOP + topPadding;
   S32 menuBottom = MENU_TOP + totalHeight;

   // If we are transitioning between items of different sizes, we will gradually change the rendered size during the transition
   // Generally, the top of the menu will stay in place, while the bottom will be adjusted.  Therefore, lower items need
   // to be offset by the transitionOffset which we will calculate below.
   S32 transitionOffset = 0;

   if(mTransitionTimer.getCurrent() > 0)
      transitionOffset = (mOldBottom - menuBottom) * mTransitionTimer.getFraction();
   else
   {
      mOldBottom = menuBottom;
      mOldCount = displayItems;
   }

   menuBottom += transitionOffset;

   FontManager::pushFontContext(FontManager::OverlayMenuContext);

   // First do a dry run with the menu items to get their indent level (draw == false).  Later
   // we'll draw them for real.
   S32 itemIndent = drawMenuItems(false, items, count, 0, menuBottom, true, hasLegend);

   S32 interiorEdge = mItemWidth + itemIndent + MENU_PADDING;

   S32 grayLineLeft   = 20;
   S32 grayLineRight  = interiorEdge - 20;
   S32 grayLineCenter = (grayLineLeft + grayLineRight) / 2;

   renderMenuFrame(interiorEdge, totalHeight + transitionOffset);

   // Draw the title (above gray line)
   glColor(baseColor);
   drawCenteredString(grayLineCenter, yPos, TITLE_FONT_SIZE, title);
   yPos += titleHeight;

   // Gray line
   glColor(Colors::gray20);

   drawHorizLine(grayLineLeft, grayLineRight, yPos + 2);

   yPos += grayLineBuffer;

   // Draw menu items (below gray line)
   drawMenuItems(true, items, count, yPos, menuBottom, true, hasLegend);

   // If we're in transition, we need to call drawMenuItems again with the old items
   if(prevItems && mTransitionTimer.getCurrent() > 0)
      drawMenuItems(true, prevItems, prevCount, yPos + 2, menuBottom, false, hasLegend);

   // itemsHeight includes grayLineBuffer, transitionOffset accounts for potentially changing menu height during transition
   yPos += itemsHeight + transitionOffset;     

   if(hasLegend)
      renderLegend(grayLineCenter, yPos - legendHeight - 3, legendText, legendColors, legendCount);

   yPos += legendHeight;

   renderPressEscapeToCancel(grayLineCenter, yPos, baseColor, getGame()->getSettings()->getInputCodeManager()->getInputMode());

   FontManager::popFontContext();

   glPopMatrix();
}


extern ScreenInfo gScreenInfo;

// Render a set of menu items.  Break this code out to make transitions easier (when we'll be rendering two sets of items).
S32 HelperMenu::drawMenuItems(bool draw, const OverlayMenuItem *items, S32 count, S32 yPos, S32 bottom, bool newItems, bool renderKeysWithItemColor)
{
   S32 displayItems = 0;
   // Count how many items we will be displaying -- some may be hidden
   for(S32 i = 0; i < count; i++)
      if(items[i].showOnMenu)
         displayItems++;

   S32 height = (MENU_FONT_SIZE + MENU_FONT_SPACING) * displayItems;
   S32 oldHeight = (MENU_FONT_SIZE + MENU_FONT_SPACING) * mOldCount;

      // Determine whether to show keys or joystick buttons on menu
   GameSettings *settings   = getGame()->getSettings();
   InputMode inputMode      = settings->getInputCodeManager()->getInputMode();
   bool showKeys            = settings->getIniSettings()->showKeyboardKeys || inputMode == InputModeKeyboard;
   bool showJoystickButtons = inputMode == InputModeJoystick;

   // For testing, cycle through all valid combinations of keyboard/joystick button display to make sure all render properly
   //S32 mode = Platform::getRealMilliseconds() / 1000 % 3;
   //if(mode == 0) { showKeys = false; showJoystickButtons = true;  }
   //if(mode == 1) { showKeys = true;  showJoystickButtons = false; }
   //if(mode == 2) { showKeys = true;  showJoystickButtons = true;  }

   S32 keyIndent  = showKeys ? (showJoystickButtons ? 28 : 20) : 0;
   S32 jsIndent   = showJoystickButtons ? 20 : 0;
   S32 itemIndent = jsIndent + keyIndent + 19;

   // If draw is false, this was just a dry run to get itemIndent
   if(!draw)
      return itemIndent;

   static ScissorsManager scissorsManager;

   scissorsManager.enable(mTransitionTimer.getCurrent() > 0, getGame(), 0, yPos, 
                          gScreenInfo.getGameCanvasWidth(), bottom - yPos - (4 * MENU_PADDING + MENU_LEGEND_FONT_SIZE));

   yPos += mTransitionTimer.getFraction() * oldHeight - (newItems ? 0 : height);


   yPos += 2;    // Aesthetics

   for(S32 i = 0; i < count; i++)
   {
      // Don't show an option that shouldn't be shown!
      if(!items[i].showOnMenu)
         continue;
      
      // Draw key controls for selecting the object to be created
      U32 joystickIndex = Joystick::SelectedPresetIndex;

      if(showJoystickButtons)     // Only draw joystick buttons when in joystick mode
         JoystickRender::renderControllerButton(F32(jsIndent), (F32)yPos, 
                                                joystickIndex, items[i].button, false);

      if(showKeys)
      {
         // Render key in white, or, if there is a legend, in the color of the adjacent item
         glColor(renderKeysWithItemColor ? items[i].itemColor : &Colors::white); 
         JoystickRender::renderControllerButton((F32)keyIndent + jsIndent, (F32)yPos, 
                                                joystickIndex, items[i].key, false);
      }

      glColor(items[i].itemColor);  

      S32 x = drawStringAndGetWidth(itemIndent, yPos, MENU_FONT_SIZE, items[i].name); 

      // Render help string, if one is available
      if(strcmp(items[i].help, "") != 0)
      {
         glColor(items[i].helpColor);    
         drawString(itemIndent + x, yPos, MENU_FONT_SIZE, items[i].help);
      }

      yPos += MENU_FONT_SIZE + MENU_FONT_SPACING;
   }

   scissorsManager.disable();

   return itemIndent;
}


void HelperMenu::renderPressEscapeToCancel(S32 xPos, S32 yPos, const Color &baseColor, InputMode inputMode)
{
   glColor(baseColor);

   // RenderedSize will be -1 if the button is not defined
   if(inputMode == InputModeKeyboard)
      drawStringfc(xPos, yPos, MENU_LEGEND_FONT_SIZE, 
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


void HelperMenu::renderLegend(S32 x, S32 y, const char **legendText, const Color **legendColors, S32 legendCount)
{
   S32 width = 0;
   y += MENU_FONT_SPACING;

   const S32 SPACE_BETWEEN_LEGEND_ITEMS = 7;

   // First, get the total width so we can center poperly
   for(S32 i = 0; i < legendCount; i++)
      width += getStringWidth(MENU_LEGEND_FONT_SIZE, legendText[i]) + SPACE_BETWEEN_LEGEND_ITEMS;

   x -= width / 2;

   for(S32 i = 0; i < legendCount; i++)
   {
      glColor(legendColors[i]);
      x += drawStringAndGetWidth(x, y, MENU_LEGEND_FONT_SIZE, legendText[i]) + SPACE_BETWEEN_LEGEND_ITEMS;
   }
}


void HelperMenu::renderMenuFrame(S32 interiorEdge, S32 height)
{
   const S32 CORNER_SIZE = 15;      
   S32 bottom = MENU_TOP + height;

   Point p[] = { Point(0, MENU_TOP), Point(interiorEdge - CORNER_SIZE, MENU_TOP),   // Top
                 Point(interiorEdge, MENU_TOP + CORNER_SIZE),                       // Edge
                 Point(interiorEdge, bottom), Point(0, bottom) };                   // Bottom

   Vector<Point> points(p, ARRAYSIZE(p));

   // Fill
   glColor(Colors::black, 0.70f);
   renderPointVector(&points, GL_POLYGON);

   // Border
   glColor(Color(.35,0,0));
   renderPointVector(&points, GL_LINE_STRIP);
}


// Calculate the width of the widest item in items
S32 HelperMenu::getMaxItemWidth(const OverlayMenuItem *items, S32 count)
{
   S32 width = -1;
   for(S32 i = 0; i < count; i++)
   {
      S32 w = getStringWidth(FontManager::OverlayMenuContext, MENU_FONT_SIZE, items[i].name) + getStringWidth(MENU_FONT_SIZE, items[i].help);
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

   mTransitionTimer.update(deltaT);
}


// Used for loadout-style menus
S32 HelperMenu::getLeftEdgeOfMenuPos()
{
   // Magic number that seems to work well... no matter that the real menu might be a different width... by
   // using this constant, menus appear at a consistent rate.
   F32 width = 400;     

   return (mActivating ? -mAnimationTimer.getFraction() * width : 
                         (mAnimationTimer.getFraction() - 1) * width);
}


};
