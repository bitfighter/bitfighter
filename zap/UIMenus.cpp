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

#include "UIMenus.h"

#include "UIGame.h"
#include "UIEditor.h"         // Can get rid of this with a passthrough in MenuManager
#include "UIErrorMessage.h"   // Can get rid of this with a passthrough in MenuManager

#include "Colors.h"
#include "gameObjectRender.h"    // For renderBitfighterLogo, glColor
#include "ClientGame.h"
#include "ServerGame.h"
#include "gameType.h"            // Can get rid of this with some simple passthroughs
#include "IniFile.h"
#include "ScreenInfo.h"
#include "JoystickRender.h"
#include "Cursor.h"
#include "VideoSystem.h"
#include "FontManager.h"
#include "RenderUtils.h"

#include "OpenglUtils.h"

#include <algorithm>
#include <string>
#include <math.h>

namespace Zap
{

// Sorts alphanumerically by menuItem's prompt  ==> used for getting levels in the right order and such
S32 QSORT_CALLBACK menuItemValueSort(boost::shared_ptr<MenuItem> *a, boost::shared_ptr<MenuItem> *b)
{
   return stricmp((*a)->getPrompt().c_str(), (*b)->getPrompt().c_str());
}


extern void shutdownBitfighter();

////////////////////////////////////
////////////////////////////////////

// Constructor
MenuUserInterface::MenuUserInterface(ClientGame *game) : UserInterface(game)
{
   initialize();
}


MenuUserInterface::MenuUserInterface(ClientGame *game, const string &title) : UserInterface(game)
{
   initialize();

   mMenuTitle = title;
}


void MenuUserInterface::initialize()
{
   setMenuID(GenericUI);
   mMenuTitle = "Menu:";
   mMenuSubTitle = "";

   selectedIndex = 0;
   itemSelectedWithMouse = false;
   mFirstVisibleItem = 0;
   mRenderInstructions = true;
   mRenderSpecialInstructions = true;
   mIgnoreNextMouseEvent = false;

   // Max number of menu items we show on screen before we go into scrolling mode -- won't work with mixed size menus
   mMaxMenuSize = S32((gScreenInfo.getGameCanvasHeight() - 150) / (getTextSize(MENU_ITEM_SIZE_NORMAL) + getGap(MENU_ITEM_SIZE_NORMAL)));
}


// Gets run when menu is activated.  This is also called by almost all other menus/subclasses.
void MenuUserInterface::onActivate()
{
   mDisableShipKeyboardInput = true;       // Keep keystrokes from getting to game
   selectedIndex = 0;
   mFirstVisibleItem = 0;
}


void MenuUserInterface::onReactivate()
{
   mDisableShipKeyboardInput = true;       // Keep keystrokes from getting to game
}


void MenuUserInterface::clearMenuItems()
{
   mMenuItems.clear();
}


void MenuUserInterface::sortMenuItems()
{
   mMenuItems.sort(menuItemValueSort);
}


void MenuUserInterface::addMenuItem(MenuItem *menuItem)
{
   menuItem->setMenu(this);
   mMenuItems.push_back(boost::shared_ptr<MenuItem>(menuItem));
}


// For those times when you really need to add a pre-packaged menu item... normally, you won't need to do this
void MenuUserInterface::addWrappedMenuItem(boost::shared_ptr<MenuItem> menuItem)
{
   menuItem->setMenu(this);
   mMenuItems.push_back(menuItem);
}


S32 MenuUserInterface::getMenuItemCount()
{
   return mMenuItems.size();
}


MenuItem *MenuUserInterface::getLastMenuItem()
{
   return mMenuItems.last().get();
}


MenuItem *MenuUserInterface::getMenuItem(S32 index)
{
   return mMenuItems[index].get();
}


void MenuUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   // Controls rate of scrolling long menus with mouse
   mScrollTimer.update(timeDelta);

   // Call mouse handler so users can scroll scrolling menus just by holding mouse in position
   // (i.e. we don't want to limit scrolling action only to times when user moves mouse)
   if(itemSelectedWithMouse)
      processMouse();
}


// Return index offset to account for scrolling menus; basically caluclates index of topmost visible item
S32 MenuUserInterface::getOffset()
{
   S32 offset = 0;

   if(isScrollingMenu())     // Do some sort of scrolling
   {
      // itemSelectedWithMouse basically lets users highlight the top and bottom items in a scrolling list,
      // which can't be done when using the keyboard
      if(selectedIndex - mFirstVisibleItem < (itemSelectedWithMouse ? 0 : 1))
         offset = selectedIndex - (itemSelectedWithMouse ? 0 : 1);
      else if( selectedIndex - mFirstVisibleItem > (mMaxMenuSize - (itemSelectedWithMouse ? 1 : 2)) )
         offset = selectedIndex - (mMaxMenuSize - (itemSelectedWithMouse ? 1 : 2));
      else offset = mFirstVisibleItem;
   }

   mFirstVisibleItem = checkMenuIndexBounds(offset);

   return mFirstVisibleItem;
}


bool MenuUserInterface::isScrollingMenu()
{
   return mMenuItems.size() > mMaxMenuSize;
}


S32 MenuUserInterface::checkMenuIndexBounds(S32 index)
{
   if(index < 0)
      return 0;
   
   if(index > getMaxFirstItemIndex())
      return getMaxFirstItemIndex();

   return index;
}


// Get vert pos of first menu item
S32 MenuUserInterface::getYStart()
{
   S32 vertOff = (getMenuID() == MainUI) ? 40 : 0;    // Make room for the logo on the main menu

   if(getMenuID() == GameParamsUI)  // If we're on the GameParams menu, start at a constant position
      return 70;
   else                             // Otherwise, attpempt to center the menu vertically
      return (gScreenInfo.getGameCanvasHeight() - min(mMenuItems.size(), mMaxMenuSize) * 
             (getTextSize(MENU_ITEM_SIZE_NORMAL) + getGap(MENU_ITEM_SIZE_NORMAL))) / 2 + vertOff;
}


static void renderMenuInstructions(GameSettings *settings)
{
   S32 canvasWidth  = gScreenInfo.getGameCanvasWidth();
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   S32 y = canvasHeight - UserInterface::vertMargin - 20;
   const S32 size = 18;

   glColor(Colors::white);

   U32 joystickIndex = Joystick::SelectedPresetIndex;

   if(settings->getInputCodeManager()->getInputMode() == InputModeKeyboard)
      drawCenteredString(y, size, "UP, DOWN to choose | ENTER to select | ESC exits menu");
   else
   {
      S32 upWidth    = JoystickRender::getControllerButtonRenderedSize(joystickIndex, BUTTON_DPAD_UP);
      S32 downWidth  = JoystickRender::getControllerButtonRenderedSize(joystickIndex, BUTTON_DPAD_DOWN);
      S32 startWidth = JoystickRender::getControllerButtonRenderedSize(joystickIndex, BUTTON_START);
      S32 backWidth  = JoystickRender::getControllerButtonRenderedSize(joystickIndex, BUTTON_BACK);

      S32 totalWidth = upWidth + downWidth + startWidth + backWidth +
                       getStringWidth(size, "to choose |  to select |  exits menu");

      S32 x = canvasWidth / 2 - UserInterface::horizMargin - totalWidth / 2;

      JoystickRender::renderControllerButton((F32)x, (F32)y, joystickIndex, BUTTON_DPAD_UP, false);
      x += upWidth + getStringWidth(size, " ");

      JoystickRender::renderControllerButton((F32)x, (F32)y, joystickIndex, BUTTON_DPAD_DOWN, false);
      x += downWidth + getStringWidth(size, " ");

      glColor(Colors::white);
      static const char *msg1 = "to choose | ";

      x += drawStringAndGetWidth(x, y, size, msg1);

      JoystickRender::renderControllerButton((F32)x, F32(y + 4), joystickIndex, BUTTON_START, false);
      x += startWidth;

      glColor(Colors::white);
      static const char *msg2 = "to select | ";
      x += drawStringAndGetWidth(x, y, size, msg2);

      JoystickRender::renderControllerButton(F32(x + 4), F32(y + 4), joystickIndex, BUTTON_BACK, false);
      x += backWidth;

      glColor(Colors::white);
      drawString(x, y, size, "exits menu");
   }
}


static void renderArrow(S32 pos, bool pointingUp)
{
   static const S32 ARROW_WIDTH = 100;
   static const S32 ARROW_HEIGHT = 20;
   static const S32 ARROW_MARGIN = 5;

   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   F32 y = 0;
   if(pointingUp)    // Up arrow
      y = pos - (ARROW_HEIGHT + ARROW_MARGIN) - 7;
   else              // Down arrow
      y = pos + (ARROW_HEIGHT + ARROW_MARGIN) - 7;

   F32 vertices[] = {
         (canvasWidth - ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7,
         (canvasWidth + ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7,
         canvasWidth / 2, y
   };
   for(S32 i = 1; i >= 0; i--)
   {
      // First create a black poly to blot out what's behind, then the arrow itself
      glColor(i ? Colors::black : Colors::blue);
      renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, i ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
   }
}


static void renderArrowAbove(S32 pos)
{
   renderArrow(pos, true);
}


static void renderArrowBelow(S32 pos)
{
   renderArrow(pos, false);
}


// Basic menu rendering
void MenuUserInterface::render()
{
   FontManager::pushFontContext(MenuContext);

   S32 canvasWidth  = gScreenInfo.getGameCanvasWidth();
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   // Draw the game screen, then dim it out so you can still see it under our overlay
   if(getGame()->getConnectionToServer())
   {
      if(getUIManager()->getGameUserInterface())
         getUIManager()->getGameUserInterface()->render();

      dimUnderlyingUI();
   }

   // Title 
   glColor(Colors::white);    
   drawCenteredString(vertMargin, 30, mMenuTitle.c_str());
   
   // Subtitle
   glColor(mMenuSubTitleColor);
   drawCenteredString(vertMargin + 35, 18, mMenuSubTitle.c_str());

   // Instructions
   if(mRenderInstructions)
      renderMenuInstructions(getGame()->getSettings());

   S32 count = mMenuItems.size();

   if(isScrollingMenu())     // Need some sort of scrolling?
      count = mMaxMenuSize;

   S32 yStart = getYStart();
   S32 offset = getOffset();

   S32 shrinkfact = 1;

   S32 y = yStart;

   for(S32 i = 0; i < count; i++)
   {
      MenuItemSize size = getMenuItem(i)->getSize();
      S32 textsize = getTextSize(size);
      S32 gap = getGap(size);
      
      // Highlight selected item
      if(selectedIndex == i + offset)  
         drawMenuItemHighlight(0,           y - gap / 2 + shrinkfact, 
                               canvasWidth, y + textsize + gap / 2 - shrinkfact);

      S32 indx = i + offset;
      mMenuItems[indx]->render(y, textsize, selectedIndex == indx);

      y += textsize + gap;
   }

   // Render an indicator that there are scrollable items above and/or below
   if(isScrollingMenu())
   {
      if(offset > 0)                     // There are items above
         renderArrowAbove(yStart);

      if(offset < getMaxFirstItemIndex())     // There are items below
         renderArrowBelow(yStart + (getTextSize(MENU_ITEM_SIZE_NORMAL) + getGap(MENU_ITEM_SIZE_NORMAL)) * mMaxMenuSize + 6);
   }

   // Render a help string at the bottom of the menu
   if(U32(selectedIndex) < U32(mMenuItems.size()))
   {
      const S32 helpFontSize = 15;
      S32 ypos = canvasHeight - vertMargin - 50;

      // Render a special instruction line
      if(mRenderSpecialInstructions)
      {
         glColor(Colors::menuHelpColor);  
         drawCenteredString(ypos, helpFontSize, mMenuItems[selectedIndex]->getSpecialEditingInstructions());
      }

      ypos -= helpFontSize + 5;
      glColor(Colors::yellow);
      drawCenteredString(ypos, helpFontSize, mMenuItems[selectedIndex]->getHelp().c_str());
   }

   renderExtras();  // Draw something unique on a menu

   FontManager::popFontContext();
}


// Calculates maximum index that the first item can have -- on non scrolling menus, this will be 0
S32 MenuUserInterface::getMaxFirstItemIndex()
{
   return max(mMenuItems.size() - mMaxMenuSize, 0);
}


// Fill responses with values from each menu item in turn
void MenuUserInterface::getMenuResponses(Vector<string> &responses)
{
   for(S32 i = 0; i < mMenuItems.size(); i++)
      responses.push_back(mMenuItems[i]->getValue());
}


// Handle mouse input, figure out which menu item we're over, and highlight it
void MenuUserInterface::onMouseMoved()
{
   if(mIgnoreNextMouseEvent)     // Suppresses spurious mouse events from the likes of SDL_WarpMouse
   {
      mIgnoreNextMouseEvent = false;
      return;
   }

   Parent::onMouseMoved();

   // Really only matters when starting to host game... don't want to be able to change menu items while the levels are loading.
   // This is purely an aesthetic issue, a minor irritant.
   if(gServerGame && gServerGame->hostingModePhase == ServerGame::LoadingLevels)
      return;

   itemSelectedWithMouse = true;
   Cursor::enableCursor();  // Show cursor when user moves mouse

   selectedIndex = getSelectedMenuItem();

   processMouse();
}


S32 MenuUserInterface::getSelectedMenuItem()
{
   S32 mouseY = (S32)gScreenInfo.getMousePos()->y;   

   S32 cumHeight = getYStart();

   // Mouse is above the top of the menu
   if(mouseY <= cumHeight)    // That's cumulative height, you pervert!
      return mFirstVisibleItem;

   // Mouse is on the menu
   for(S32 i = 0; i < getMenuItemCount() - 1; i++)
   {
      MenuItemSize size = getMenuItem(i)->getSize();
      S32 height = getGap(size) / 2 + getTextSize(size);

      cumHeight += height;

      if(mouseY < cumHeight)
         return i + mFirstVisibleItem;     

      cumHeight += getGap(size) / 2;
   }

   // Mouse is below bottom of menu
   return getMenuItemCount() - 1 + mFirstVisibleItem;
}


void MenuUserInterface::processMouse()
{
   if(isScrollingMenu())   // We have a scrolling situation here...
   {
      if(selectedIndex <= mFirstVisibleItem)                          // Scroll up
      {
         if(!mScrollTimer.getCurrent() && mFirstVisibleItem > 0)
         {
            mFirstVisibleItem--;
            mScrollTimer.reset(100);
         }
         selectedIndex = mFirstVisibleItem;
      }
      else if(selectedIndex > mFirstVisibleItem + mMaxMenuSize - 1)  // Scroll down
      {
         if(!mScrollTimer.getCurrent() && selectedIndex > mFirstVisibleItem + mMaxMenuSize - 2)
         {
            mFirstVisibleItem++;
            mScrollTimer.reset(MOUSE_SCROLL_INTERVAL);
         }
         selectedIndex = mFirstVisibleItem + mMaxMenuSize - 1;
      }
      else
         mScrollTimer.clear();
   }

   if(selectedIndex < 0)                          // Scrolled off top of list
   {
      selectedIndex = 0;
      mFirstVisibleItem = 0;
   }
   else if(selectedIndex >= mMenuItems.size())    // Scrolled off bottom of list
   {
      selectedIndex = mMenuItems.size() - 1;
      mFirstVisibleItem = getMaxFirstItemIndex();
   }
}


bool MenuUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;

   // Capture mouse wheel on scrolling menus and use it to scroll.  Otherwise, let it be processed by individual menu items.
   // This will usually work because scrolling menus do not (at this time) contain menu items that themselves use the wheel.
   if(isScrollingMenu())
   {
      if(inputCode == MOUSE_WHEEL_DOWN)
      {
         mFirstVisibleItem = checkMenuIndexBounds(mFirstVisibleItem + 1);

         onMouseMoved();
         return true;
      }
      else if(inputCode == MOUSE_WHEEL_UP)
      {
         mFirstVisibleItem = checkMenuIndexBounds(mFirstVisibleItem - 1);

         onMouseMoved();
         return true;
      }
   }

   if(inputCode == KEY_UNKNOWN)
      return true;

   // Check for in autorepeat mode
   mRepeatMode = mKeyDown;
   mKeyDown = true;

   // Handle special case of keystrokes during hosting preparation phases
   if(gServerGame && (gServerGame->hostingModePhase == ServerGame::LoadingLevels || 
                      gServerGame->hostingModePhase == ServerGame::DoneLoadingLevels))
   {
      if(inputCode == KEY_ESCAPE)     // Can only get here when hosting
      {
         gServerGame->hostingModePhase = ServerGame::NotHosting;
         getUIManager()->getHostMenuUserInterface()->clearLevelLoadDisplay();
         getGame()->closeConnectionToGameServer();

         delete gServerGame;
         gServerGame = NULL;
      }

      // All other keystrokes will be ignored
      return true;
   }

   // Process each key handler in turn until one works
   bool keyHandled = processMenuSpecificKeys(inputCode);

   if(!keyHandled)
      keyHandled = processKeys(inputCode);


   // Finally, since the user has indicated they want to use keyboard/controller input, hide the pointer
   if(!InputCodeManager::isMouseAction(inputCode) && inputCode != KEY_ESCAPE)
      Cursor::disableCursor();

   return keyHandled;
}


void MenuUserInterface::onTextInput(char ascii)
{

   if(U32(selectedIndex) < U32(mMenuItems.size()))
      mMenuItems[selectedIndex]->handleTextInput(ascii);
}


void MenuUserInterface::onKeyUp(InputCode inputCode)
{
   mKeyDown = false;
   mRepeatMode = false;
}


// Generic handler looks for keystrokes and translates them into menu actions
bool MenuUserInterface::processMenuSpecificKeys(InputCode inputCode)
{
   // Don't process shortcut keys if the current menuitem has text input
   if(U32(selectedIndex) < U32(mMenuItems.size()) && mMenuItems[selectedIndex]->hasTextInput())
      return false;

   // Check for some shortcut keys
   for(S32 i = 0; i < mMenuItems.size(); i++)
   {
      if(inputCode == mMenuItems[i]->key1 || inputCode == mMenuItems[i]->key2)
      {
         selectedIndex = i;

         mMenuItems[i]->activatedWithShortcutKey();
         itemSelectedWithMouse = false;
         return true;
      }
   }

   return false;
}


S32 MenuUserInterface::getTotalMenuItemHeight()
{
   S32 height = 0;
   for(S32 i = 0; i < mMenuItems.size(); i++)
   {
      MenuItemSize size = mMenuItems[i]->getSize();
      height += getTextSize(size) + getGap(size);
   }

   return height;
}


// Process the keys that work on all menus
bool MenuUserInterface::processKeys(InputCode inputCode)
{
   inputCode = InputCodeManager::convertJoystickToKeyboard(inputCode);
   
   if(Parent::onKeyDown(inputCode))
   { 
      // Do nothing 
   }

   else if(U32(selectedIndex) >= U32(mMenuItems.size()))  // Probably empty menu... Can only go back.
   {
      onEscape();
   }
   else if(mMenuItems[selectedIndex]->handleKey(inputCode))
   {
   }
   else if(inputCode == KEY_ENTER || (inputCode == KEY_SPACE && !mMenuItems[selectedIndex]->hasTextInput()))
   {
      playBoop();
      if(inputCode != MOUSE_LEFT)
         itemSelectedWithMouse = false;

      else // it was MOUSE_LEFT after all
      {
         // Make sure we're actually pointing at a menu item before we process it
         S32 yStart = getYStart();
         const Point *mousePos = gScreenInfo.getMousePos();

         getSelectedMenuItem();

         if(mousePos->y < getYStart() || yStart + getTotalMenuItemHeight())
            return true;
      }

      mMenuItems[selectedIndex]->handleKey(inputCode);

      if(mMenuItems[selectedIndex]->enterAdvancesItem())
         advanceItem();
   }

   else if(inputCode == KEY_ESCAPE)
   {
      playBoop();
      onEscape();
   }
   else if(inputCode == KEY_UP || (inputCode == KEY_TAB && InputCodeManager::checkModifier(KEY_SHIFT)))   // Prev item
   {
      selectedIndex--;
      itemSelectedWithMouse = false;

      if(selectedIndex < 0)                        // Scrolling off the top
      {
         if(isScrollingMenu() && mRepeatMode)      // Allow wrapping on long menus only when not in repeat mode
         {
            selectedIndex = 0;               // No wrap --> (first item)
            return true;                     // (leave before playBoop)
         }
         else                                         // Always wrap on shorter menus
            selectedIndex = mMenuItems.size() - 1;    // Wrap --> (select last item)
      }
      playBoop();
   }

   else if(inputCode == KEY_DOWN || inputCode == KEY_TAB)    // Next item
      advanceItem();

   // If nothing was handled, return false
   else
      return false;

   // If we made it here, then something was handled
   return true;
}


S32 MenuUserInterface::getTextSize(MenuItemSize size)
{
   return size == MENU_ITEM_SIZE_NORMAL ? 23 : 15;
}


S32 MenuUserInterface::getGap(MenuItemSize size)
{
   return 18;
}


void MenuUserInterface::renderExtras()
{
   /* Do nothing */
}


void MenuUserInterface::advanceItem()
{
   selectedIndex++;
   itemSelectedWithMouse = false;

   if(selectedIndex >= mMenuItems.size())     // Scrolling off the bottom
   {
      if(isScrollingMenu() && mRepeatMode)    // Allow wrapping on long menus only when not in repeat mode
      {
         selectedIndex = getMenuItemCount() - 1;                 // No wrap --> (last item)
         return;                                                 // (leave before playBoop)
      }
      else                     // Always wrap on shorter menus
         selectedIndex = 0;    // Wrap --> (first item)
   }
   playBoop();
}


void MenuUserInterface::onEscape()
{
   // Do nothing
}


////////////////////////////////////////
////////////////////////////////////////

bool MenuUserInterfaceWithIntroductoryAnimation::mFirstTime = true;


// Constructor
MenuUserInterfaceWithIntroductoryAnimation::MenuUserInterfaceWithIntroductoryAnimation(ClientGame *game) : Parent(game)
{
   mShowingAnimation = false;
}


void MenuUserInterfaceWithIntroductoryAnimation::onActivate()
{
   if(mFirstTime)
   {
      mFadeInTimer.reset(FadeInTime);
      getUIManager()->activate(SplashUI);   // Show splash screen the first time through
      mShowingAnimation = true;
      mFirstTime = false;
   }
}


void MenuUserInterfaceWithIntroductoryAnimation::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);
   mFadeInTimer.update(timeDelta);

   mShowingAnimation = false;
}


void MenuUserInterfaceWithIntroductoryAnimation::render()
{
   Parent::render();

   // Fade in the menu here if we are showing it the first time...  this will tie in
   // nicely with the splash screen, and make the transition less jarring and sudden
   if(mFadeInTimer.getCurrent())
      dimUnderlyingUI(mFadeInTimer.getFraction());

   // Render logo at top, never faded
   renderStaticBitfighterLogo();
}


bool MenuUserInterfaceWithIntroductoryAnimation::onKeyDown(InputCode inputCode)
{
   if(mShowingAnimation)
   {
      mShowingAnimation = false;    // Stop animations if a key is pressed
      return true;                  // Swallow the keystroke
   }

   return Parent::onKeyDown(inputCode);
}


// Take action based on menu selection
void MenuUserInterfaceWithIntroductoryAnimation::processSelection(U32 index)
{
   mShowingAnimation = false;
}


////////////////////////////////////////
////////////////////////////////////////

//////////
// MainMenuUserInterface callbacks
//////////

static void joinSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(QueryServersScreenUI);
}

static void hostSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(HostingUI);
}

static void helpSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(InstructionsUI);
}

static void optionsSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(OptionsUI);
}

static void highScoresSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(HighScoresUI);
}

static void editorSelectedCallback(ClientGame *game, U32 unused)
{
   game->setLevelDatabaseId(0);
   game->getUIManager()->getEditorUserInterface()->setLevelFileName("");      // Reset this so we get the level entry screen
   game->getUIManager()->activate(EditorUI);
}

static void creditsSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(CreditsUI);
}

static void quitSelectedCallback(ClientGame *game, U32 unused)
{
   shutdownBitfighter();
}

//////////

// Constructor
MainMenuUserInterface::MainMenuUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(MainUI);
   mMenuTitle = "";
   mMOTD[0] = 0;
   mMenuSubTitle = "";
   mRenderInstructions = false;

   mNeedToUpgrade = false;           // Assume we're up-to-date until we hear from the master
   mShowedUpgradeAlert = false;      // So we don't show the upgrade message more than once

   InputCode keyHelp = getInputCode(game->getSettings(), InputCodeManager::BINDING_HELP);

   addMenuItem(new MenuItem("JOIN LAN/INTERNET GAME", joinSelectedCallback,       "", KEY_J));
   addMenuItem(new MenuItem("HOST GAME",              hostSelectedCallback,       "", KEY_H));
   addMenuItem(new MenuItem("INSTRUCTIONS",           helpSelectedCallback,       "", KEY_I, keyHelp));
   addMenuItem(new MenuItem("OPTIONS",                optionsSelectedCallback,    "", KEY_O));
   addMenuItem(new MenuItem("HIGH SCORES",            highScoresSelectedCallback, "", KEY_S));
   addMenuItem(new MenuItem("LEVEL EDITOR",           editorSelectedCallback,     "", KEY_L, KEY_E));
   addMenuItem(new MenuItem("CREDITS",                creditsSelectedCallback,    "", KEY_C));
   addMenuItem(new MenuItem("QUIT",                   quitSelectedCallback,       "", KEY_Q));
}


void MainMenuUserInterface::onActivate()
{
   Parent::onActivate();

   // Time for a clean start.  No matter how we got here, there's no going back.
   // Needed mainly because the editor makes things confusing.  Now that that's been reworked,
   // it's probably not needed at all.
   //getUIManager()->clearPrevUIs();

   mColorTimer.reset(ColorTime);
   mColorTimer2.reset(ColorTime2);
   mTransDir = true;
}


// Set the MOTD we received from the master
void MainMenuUserInterface::setMOTD(const char *motd)
{
   strncpy(mMOTD, motd, MOTD_LEN);     

   motdArriveTime = getGame()->getCurrentTime();    // Used for scrolling the message
}


// Set needToUpgrade flag that tells us the client is out-of-date
void MainMenuUserInterface::setNeedToUpgrade(bool needToUpgrade)
{
   mNeedToUpgrade = needToUpgrade;

   if(mNeedToUpgrade && !mShowedUpgradeAlert)
      showUpgradeAlert();
}

static const S32 MOTD_POS = 540;

void MainMenuUserInterface::render()
{
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   if(strcmp(mMOTD, ""))
   {
      U32 width = getStringWidth(20, mMOTD);
      glColor(Colors::white);
      U32 totalWidth = width + canvasWidth;
      U32 pixelsPerSec = 100;
      U32 delta = getGame()->getCurrentTime() - motdArriveTime;
      delta = U32(delta * pixelsPerSec * 0.001) % totalWidth;

      drawString(canvasWidth - delta, MOTD_POS, 20, mMOTD);
   }

   // Parent renderer might dim what we've drawn so far, so run it last so it can have access to everything
   Parent::render();
}


void MainMenuUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   if(mColorTimer.update(timeDelta))
   {
      mColorTimer.reset(ColorTime);
      mTransDir = !mTransDir;
   }

   if(mColorTimer2.update(timeDelta))
   {
      mColorTimer2.reset(ColorTime2);
      mTransDir2 = !mTransDir2;
   }
}


bool MainMenuUserInterface::getNeedToUpgrade()
{
   return mNeedToUpgrade;
}


void MainMenuUserInterface::renderExtras()
{
   glColor(Colors::white);
   const S32 size = 20;
   drawCenteredString(gScreenInfo.getGameCanvasHeight() - vertMargin - size, size, "join us @ www.bitfighter.org");
}


void MainMenuUserInterface::showUpgradeAlert()
{
   ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

   ui->reset();
   ui->setTitle("OUTDATED VERSION");
   ui->setMessage(1, "You are running an older version of Bitfighter.");
   ui->setMessage(2, "You will only be able to play with players who still");
   ui->setMessage(3, "have the same outdated version.");
   ui->setMessage(4, "");
   ui->setMessage(5, "To get the latest, visit bitfighter.org");

   getUIManager()->activate(ui);

   mShowedUpgradeAlert = true;            // Only show this alert once per session -- we don't need to beat them over the head with it!
}


void MainMenuUserInterface::onEscape()
{
   shutdownBitfighter();    // Quit!
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
OptionsMenuUserInterface::OptionsMenuUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(OptionsUI);
   mMenuTitle = "OPTIONS MENU:";
}


void OptionsMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
}


//////////
// Callbacks for Options menu

static void inputCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(InputOptionsUI);
}


static void soundOptionsSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(SoundOptionsUI);
}


static void setFullscreenCallback(ClientGame *game, U32 mode)
{
   GameSettings *settings = game->getSettings();

   // Save existing setting
   settings->getIniSettings()->oldDisplayMode = game->getSettings()->getIniSettings()->displayMode;     

   settings->getIniSettings()->displayMode = (DisplayMode)mode;
   VideoSystem::actualizeScreenMode(game->getSettings(), false, game->getUIManager()->getCurrentUI()->usesEditorScreenMode());
}


//////////

// Used below and by UIEditor
MenuItem *getWindowModeMenuItem(U32 displayMode)
{
   Vector<string> opts;   
   opts.push_back("WINDOWED");
   opts.push_back("FULLSCREEN STRETCHED");
   opts.push_back("FULLSCREEN");

   return new ToggleMenuItem("DISPLAY MODE:", opts, displayMode, true, 
                             setFullscreenCallback, "Set the game mode to windowed or fullscreen", KEY_G);
}


void OptionsMenuUserInterface::setupMenus()
{
   clearMenuItems();
   Vector<string> opts;

   GameSettings *settings = getGame()->getSettings();

   addMenuItem(new MenuItem(getMenuItemCount(), "INPUT", inputCallback, 
                        "Joystick settings, Remap keys", KEY_I));

   addMenuItem(new MenuItem(getMenuItemCount(), "SOUNDS & MUSIC", soundOptionsSelectedCallback, 
                        "Change sound and music related options", KEY_S));

   addMenuItem(new YesNoMenuItem("AUTOLOGIN:", !settings->shouldShowNameEntryScreenOnStartup(), 
                                 "If selected, you will automatically log in "
                                 "on start, bypassing the first screen", KEY_A));

#ifndef TNL_OS_MOBILE
   addMenuItem(getWindowModeMenuItem((U32)settings->getIniSettings()->displayMode));
#endif

#ifdef INCLUDE_CONN_SPEED_ITEM
   opts.clear();
   opts.push_back("VERY LOW");
   opts.push_back("LOW");
   opts.push_back("MEDIUM");  // there is 5 options, -2 (very low) to 2 (very high)
   opts.push_back("HIGH");
   opts.push_back("VERY HIGH");

   addMenuItem(new ToggleMenuItem("CONNECTION SPEED:", opts, settings->getIniSettings()->connectionSpeed + 2, true, 
                                  setConnectionSpeedCallback, "Speed of your connection, if your ping goes too high, try slower speed.",  KEY_E));
#endif
}


static bool isFullScreen(DisplayMode displayMode)
{
   return displayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED || displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;
}


void OptionsMenuUserInterface::toggleDisplayMode()
{
   GameSettings *settings = getGame()->getSettings();

   DisplayMode oldMode = settings->getIniSettings()->oldDisplayMode;

   settings->getIniSettings()->oldDisplayMode = settings->getIniSettings()->displayMode;     // Save current setting

   // When we're in the editor, and we toggle views, we'll skip one of the fullscreen modes, as they essentially do the same thing in that UI
   bool editorScreenMode = getGame()->getUIManager()->getCurrentUI()->usesEditorScreenMode();
   if(editorScreenMode)
   {
      if(isFullScreen(settings->getIniSettings()->displayMode))
         settings->getIniSettings()->displayMode = DISPLAY_MODE_WINDOWED;

      // If we know what the previous fullscreen mode was, use that
      else if(isFullScreen(oldMode))
         settings->getIniSettings()->displayMode = oldMode;

      // Otherwise, pick some sort of full-screen mode...
      else
         settings->getIniSettings()->displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;
   }
   else  // Not in the editor, just advance to the next mode
   {
      DisplayMode mode = DisplayMode((U32)settings->getIniSettings()->displayMode + 1);
      settings->getIniSettings()->displayMode = (mode == DISPLAY_MODE_UNKNOWN) ? (DisplayMode) 0 : mode;    // Bounds check 
   }

   VideoSystem::actualizeScreenMode(settings, false, editorScreenMode);
}


// Save options to INI file, and return to our regularly scheduled program
void OptionsMenuUserInterface::onEscape()
{
   bool autologin = getMenuItem(2)->getIntValue();

   getGame()->getSettings()->setAutologin(autologin);

   saveSettingsToINI(&gINI, getGame()->getSettings());
   getUIManager()->reactivatePrevUI();      //mGameUserInterface
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
InputOptionsMenuUserInterface::InputOptionsMenuUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(InputOptionsUI);
   mMenuTitle = "INPUT OPTIONS:";
}


void InputOptionsMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
}


//////////
// Callbacks for InputOptions menu
static void setControlsCallback(ClientGame *game, U32 val)
{
   game->getSettings()->getIniSettings()->controlsRelative = (val == 1);
}


static void defineKeysCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(KeyDefUI);
}


static void setControllerCallback(ClientGame *game, U32 joystickIndex)
{
   game->getSettings()->getIniSettings()->joystickType = Joystick::JoystickPresetList[joystickIndex].identifier;
   Joystick::setSelectedPresetIndex(joystickIndex);
}


static void addStickOptions(Vector<string> *opts)
{
   opts->clear();
   opts->push_back("KEYBOARD");
   
   for(S32 i = 0; i < GameSettings::DetectedJoystickNameList.size(); i++)
      opts->push_back(string("JOYSTICK ") + itos(i + 1));
}


static S32 INPUT_MODE_MENU_ITEM_INDEX = 0;

// Must be static; keeps track of the number of sticks the user had last time the setInputModeCallback was run.
// That lets the function know if it needs to rebuild the menu because of new stick values available.
static S32 sticks = -1;    

static void setInputModeCallback(ClientGame *game, U32 inputModeIndex)
{
   GameSettings *settings = game->getSettings();

   // Refills GameSettings::DetectedJoystickNameList to allow people to plug in joystick while in this menu...
   Joystick::initJoystick(settings);

   // If there is a different number of sticks than previously detected
   if(sticks != GameSettings::DetectedJoystickNameList.size())
   {
      ToggleMenuItem *menuItem = dynamic_cast<ToggleMenuItem *>(game->getUIManager()->getInputOptionsUserInterface()->
                                                                getMenuItem(INPUT_MODE_MENU_ITEM_INDEX));

      // Rebuild this menu with the new number of sticks
      if(menuItem)
         addStickOptions(&menuItem->mOptions);

      // Loop back to the first index if we hit the end of the list
      if(inputModeIndex > (U32)GameSettings::DetectedJoystickNameList.size())
      {
         inputModeIndex = 0;
         menuItem->setValueIndex(0);
      }

      // Special case handler for common situation
      if(sticks == 0 && GameSettings::DetectedJoystickNameList.size() == 1)      // User just plugged a stick in
         menuItem->setValueIndex(1);

      // Save the current number of sticks
      sticks = GameSettings::DetectedJoystickNameList.size();
   }

   if(inputModeIndex == 0)
      settings->getInputCodeManager()->setInputMode(InputModeKeyboard);
   else
      settings->getInputCodeManager()->setInputMode(InputModeJoystick);


   if(inputModeIndex >= 1)
      GameSettings::UseJoystickNumber = inputModeIndex - 1;

   Joystick::enableJoystick(settings, true);
}


//////////

void InputOptionsMenuUserInterface::setupMenus()
{
   clearMenuItems();
   
   Vector<string> opts;

   GameSettings *settings = getGame()->getSettings();

   Joystick::initJoystick(settings);            // Refresh joystick list
   Joystick::enableJoystick(settings, true);    // Refresh joystick list

   addStickOptions(&opts);

   U32 inputMode = (U32)settings->getInputCodeManager()->getInputMode();   // 0 = keyboard, 1 = joystick
   if(inputMode == InputModeJoystick)
      inputMode += GameSettings::UseJoystickNumber;

   addMenuItem(new ToggleMenuItem("PRIMARY INPUT:", 
                                  opts, 
                                  (U32)inputMode,
                                  true, 
                                  setInputModeCallback, 
                                  "Specify whether you want to play with your keyboard or joystick", 
                                  KEY_P, KEY_I));

   INPUT_MODE_MENU_ITEM_INDEX = getMenuItemCount() - 1;

   opts.clear();
   // Add the joystick names to opts
   Joystick::getAllJoystickPrettyNames(opts);

   U32 selectedOption = Joystick::SelectedPresetIndex;

   addMenuItem(new ToggleMenuItem("JOYSTICK:", opts, selectedOption, true, 
                                  setControllerCallback, "Choose which joystick to use in joystick mode", KEY_J));

   addMenuItem(new MenuItem(getMenuItemCount(), "DEFINE KEYS / BUTTONS", defineKeysCallback, 
                            "Remap keyboard or joystick controls", KEY_D, KEY_K));

   opts.clear();
   opts.push_back("ABSOLUTE");
   opts.push_back("RELATIVE");

   bool relative = settings->getIniSettings()->controlsRelative;

   addMenuItem(new ToggleMenuItem("CONTROLS:", opts, relative ? 1 : 0, true, 
                                  setControlsCallback, "Set controls to absolute (normal) or relative (like a tank) mode", KEY_C));
}


// Save options to INI file, and return to our regularly scheduled program
void InputOptionsMenuUserInterface::onEscape()
{
   saveSettingsToINI(&gINI, getGame()->getSettings());
   getUIManager()->reactivatePrevUI();      
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SoundOptionsMenuUserInterface::SoundOptionsMenuUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(SoundOptionsUI);
   mMenuTitle = "SOUND OPTIONS:";
}


void SoundOptionsMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
}


static string getVolMsg(F32 volume)
{
   S32 vol = U32((volume + 0.05) * 10.0);

   string msg = itos(vol);

   if(vol == 0)
      msg += " [MUTE]";

   return msg;
}


//////////
// Callbacks for SoundOptions menu
static void setSFXVolumeCallback(ClientGame *game, U32 vol)
{
   game->getSettings()->getIniSettings()->sfxVolLevel = F32(vol) / 10;
}

static void setMusicVolumeCallback(ClientGame *game, U32 vol)
{
   game->getSettings()->getIniSettings()->setMusicVolLevel(F32(vol) / 10);
}

static void setVoiceVolumeCallback(ClientGame *game, U32 vol)
{
   F32 oldVol = game->getSettings()->getIniSettings()->voiceChatVolLevel;
   game->getSettings()->getIniSettings()->voiceChatVolLevel = F32(vol) / 10;
   if((oldVol == 0) != (vol == 0) && game->getConnectionToServer())
      game->getConnectionToServer()->s2rVoiceChatEnable(vol != 0);
}


static void setVoiceEchoCallback(ClientGame *game, U32 val)
{
   game->getSettings()->getIniSettings()->echoVoice = (val == 1);
}


void SoundOptionsMenuUserInterface::setupMenus()
{
   clearMenuItems();
   Vector<string> opts;

   GameSettings *settings = getGame()->getSettings();

   for(S32 i = 0; i <= 10; i++)
      opts.push_back(getVolMsg( F32(i) / 10 ));

   addMenuItem(new ToggleMenuItem("SFX VOLUME:",        opts, U32((settings->getIniSettings()->sfxVolLevel + 0.05) * 10.0), false, 
                                  setSFXVolumeCallback,   "Set sound effects volume", KEY_S));

   if(settings->getSpecified(NO_MUSIC))
         addMenuItem(new MessageMenuItem("MUSIC MUTED FROM COMMAND LINE", Colors::red));
   else
      addMenuItem(new ToggleMenuItem("MUSIC VOLUME:",      opts, U32((settings->getIniSettings()->getMusicVolLevel() + 0.05) * 10.0), false,
                                     setMusicVolumeCallback, "Set music volume", KEY_M));

   addMenuItem(new ToggleMenuItem("VOICE CHAT VOLUME:", opts, U32((settings->getIniSettings()->voiceChatVolLevel + 0.05) * 10.0), false, 
                                  setVoiceVolumeCallback, "Set voice chat volume", KEY_V));
   opts.clear();
   opts.push_back("DISABLED");
   opts.push_back("ENABLED");
   addMenuItem(new ToggleMenuItem("VOICE ECHO:", opts, settings->getIniSettings()->echoVoice ? 1 : 0, true, 
                                  setVoiceEchoCallback, "Toggle whether you hear your voice on voice chat",  KEY_E));
}


// Save options to INI file, and return to our regularly scheduled program
void SoundOptionsMenuUserInterface::onEscape()
{
   saveSettingsToINI(&gINI, getGame()->getSettings());
   getUIManager()->reactivatePrevUI();      //mGameUserInterface
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
NameEntryUserInterface::NameEntryUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(NameEntryUI);
   mMenuTitle = "";
   mReason = NetConnection::ReasonNone;
   mRenderInstructions = false;

}


void NameEntryUserInterface::setReactivationReason(NetConnection::TerminationReason reason) 
{ 
   mReason = reason; 
   mMenuTitle = ""; 
}


void NameEntryUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenu();
   getGame()->setReadyToConnectToMaster(false);
}


// User has entered name and password, and has clicked OK
static void nameAndPasswordAcceptCallback(ClientGame *clientGame, U32 unused)
{
   UIManager *uiManager = clientGame->getUIManager();
   NameEntryUserInterface *ui = uiManager->getNameEntryUserInterface();

   if(uiManager->hasPrevUI())
      uiManager->reactivatePrevUI();
   else
      uiManager->activate(MainUI);

   string enteredName     = ui->getMenuItem(1)->getValueForWritingToLevelFile();
   string enteredPassword = ui->getMenuItem(2)->getValueForWritingToLevelFile();

   bool savePassword      = ui->getMenuItem(3)->getIntValue() != 0;

   clientGame->userEnteredLoginCredentials(enteredName, enteredPassword, savePassword);
}


void NameEntryUserInterface::setupMenu()
{
   clearMenuItems();
   mRenderSpecialInstructions = false;

   addMenuItem(new MenuItem("OK", nameAndPasswordAcceptCallback, ""));
   addMenuItem(new TextEntryMenuItem("NICKNAME:", getGame()->getSettings()->getIniSettings()->lastName, 
                                    getGame()->getSettings()->getDefaultName(), "", MAX_PLAYER_NAME_LENGTH));
   addMenuItem(new TextEntryMenuItem("PASSWORD:", getGame()->getSettings()->getPlayerPassword(), "", "", MAX_PLAYER_PASSWORD_LENGTH));

   // If we have already saved a PW, this defaults to yes; to no otherwise
   MenuItem *menuItem = new YesNoMenuItem("SAVE PASSWORD:", getGame()->getSettings()->getPlayerPassword() != "", "");
   menuItem->setSize(MENU_ITEM_SIZE_SMALL);
   addMenuItem(menuItem);
   
   getMenuItem(1)->setFilter(LineEditor::nickNameFilter);  // Quotes are incompatible with PHPBB3 logins, %s are used for var substitution
   getMenuItem(2)->setSecret(true);
}


void NameEntryUserInterface::renderExtras()
{
   const S32 size = 15;
   const S32 gap = 5;
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   const S32 rows = 3;
   S32 row = 0;

   S32 instrGap = mRenderInstructions ? 30 : 0;

   glColor(Colors::menuHelpColor);

   //drawCenteredString(canvasHeight - vertMargin - instrGap - (rows - row) * size - (rows - row) * gap, size, 
   //         "You can skip this screen by editing the [Settings] section of Bitfighter.ini");
   row++;

   drawCenteredString(canvasHeight - vertMargin - instrGap - (rows - row) * size - (rows - row) * gap, size, 
            "A password is only needed if you are using a reserved name.  You can reserve your");
   row++;

   drawCenteredString(canvasHeight - vertMargin - instrGap - (rows - row) * size - (rows - row) * gap, size, 
            "nickname by registering for the bitfighter.org forums.  Registration is free.");


   if(mReason == NetConnection::ReasonBadLogin || mReason == NetConnection::ReasonInvalidUsername)
   {
      string message[] = { "If you have reserved this name by registering for",
                           "the forums, enter your forum password below. Otherwise,",
                           "this user name may be reserved. Please choose another."
                         };

      renderMessageBox("Invalid Name or Password", "", message, 3, -190);
   }

   // Render logo at top, never faded
   renderStaticBitfighterLogo();

}

// Save options to INI file, and return to our regularly scheduled program
void NameEntryUserInterface::onEscape()
{
   shutdownBitfighter();
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
HostMenuUserInterface::HostMenuUserInterface(ClientGame *game) : MenuUserInterface(game)
{
   setMenuID(HostingUI);
   mMenuTitle ="HOST A GAME:";

   levelLoadDisplayFadeTimer.setPeriod(1000);
   levelLoadDisplayDisplay = true;
   mEditingIndex = -1;     // Not editing at the start
}


void HostMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();

   mLevelLoadDisplayTotal = 0;
   clearLevelLoadDisplay();
}


extern void initHostGame(GameSettings *settings, const Vector<string> &levelList, bool testMode, bool dedicatedServer);

static void startHostingCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getHostMenuUserInterface()->saveSettings();

   GameSettings *settings = game->getSettings();

   initHostGame(settings, settings->getLevelList(), false, false);
}


void HostMenuUserInterface::setupMenus()
{
   clearMenuItems();

   GameSettings *settings = getGame()->getSettings();

   addMenuItem(new MenuItem("START HOSTING", startHostingCallback, "", KEY_H));

   addMenuItem(new TextEntryMenuItem("SERVER NAME:", settings->getHostName(), 
                                     "<Bitfighter Host>", "", MaxServerNameLen,  KEY_N));

   addMenuItem(new TextEntryMenuItem("DESCRIPTION:", settings->getHostDescr(),                    
                                     "<Empty>", "", MaxServerDescrLen, KEY_D));

   addMenuItem(new TextEntryMenuItem("LEVEL CHANGE PASSWORD:", settings->getLevelChangePassword(), 
                                     "<Anyone can change levels>", "", MAX_PASSWORD_LENGTH, KEY_L));

   addMenuItem(new TextEntryMenuItem("ADMIN PASSWORD:", settings->getAdminPassword(),       
                                     "<No remote admin access>", "", MAX_PASSWORD_LENGTH, KEY_A));

   addMenuItem(new TextEntryMenuItem("CONNECTION PASSWORD:", settings->getServerPassword(), "<Anyone can connect>", "",      
                                     MAX_PASSWORD_LENGTH, KEY_C));

   addMenuItem(new YesNoMenuItem("ALLOW MAP DOWNLOADS:", settings->getIniSettings()->allowGetMap, "", KEY_M));

   //addMenuItem(new CounterMenuItem("MAXIMUM PLAYERS:",   settings->getIniSettings()->maxplayers, 1, 2, MAX_PLAYERS, "", "", "", KEY_P));
   //addMenuItem(new TextEntryMenuItem("PORT:",             itos(DEFAULT_GAME_PORT),  "Use default of " + itos(DEFAULT_GAME_PORT), 
   //                                         "", 10, KEY_P));
}


// Save options to INI file, and return to our regularly scheduled program
// This only gets called when escape not already handled by preprocessKeys(), i.e. when we're not editing
void HostMenuUserInterface::onEscape()
{
   saveSettings();
   getUIManager()->reactivatePrevUI();     
}


// Save parameters and get them into the INI file
void HostMenuUserInterface::saveSettings()
{
   GameSettings *settings = getGame()->getSettings();

   settings->setHostName (getMenuItem(OPT_NAME)->getValue(), true);
   settings->setHostDescr(getMenuItem(OPT_DESCR)->getValue(), true);

   settings->setAdminPassword(getMenuItem(OPT_ADMIN_PASS)->getValue(), true);
   settings->setLevelChangePassword(getMenuItem(OPT_LVL_PASS)->getValue(), true);
   settings->setServerPassword(getMenuItem(OPT_PASS)->getValue(), true);

   settings->getIniSettings()->allowGetMap = (getMenuItem(OPT_GETMAP)->getIntValue() != 0);
   //settings->getIniSettings()->maxplayers = getMenuItem(OPT_MAX_PLAYERS)->getIntValue();

   saveSettingsToINI(&gINI, getGame()->getSettings());
}


void HostMenuUserInterface::render()
{
   Parent::render();

   // If we're in LoadingLevels mode, show the progress panel...
   renderProgressListItems();
   if(gServerGame && (gServerGame->hostingModePhase == ServerGame::LoadingLevels || 
                      gServerGame->hostingModePhase == ServerGame::DoneLoadingLevels))
   {
      // There will be exactly one new entry every time we get here!
      addProgressListItem("Loaded level " + gServerGame->getLastLevelLoadName() + "...");
   }
}


// Add bit of text to progress item, and manage the list
void HostMenuUserInterface::addProgressListItem(string item)
{
   mLevelLoadDisplayNames.push_back(item);

   mLevelLoadDisplayTotal++;

   // Keep the list from growing too long:
   if(mLevelLoadDisplayNames.size() > 15)
      mLevelLoadDisplayNames.erase(0);
}


void HostMenuUserInterface::clearLevelLoadDisplay()
{
   mLevelLoadDisplayNames.clear();
   mLevelLoadDisplayTotal = 0;
}


void HostMenuUserInterface::renderProgressListItems()
{
   if(levelLoadDisplayDisplay || levelLoadDisplayFadeTimer.getCurrent() > 0)
   {
      TNLAssert(glIsEnabled(GL_BLEND), "Blending should be enabled here!");

      for(S32 i = 0; i < mLevelLoadDisplayNames.size(); i++)
      {
         glColor(Colors::white, (1.4f - ((F32) (mLevelLoadDisplayNames.size() - i) / 10.f)) * 
                                        (levelLoadDisplayDisplay ? 1 : levelLoadDisplayFadeTimer.getFraction()) );
         drawStringf(100, gScreenInfo.getGameCanvasHeight() - vertMargin - (mLevelLoadDisplayNames.size() - i) * 20, 
                     15, "%s", mLevelLoadDisplayNames[i].c_str());
      }
   }
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
GameMenuUserInterface::GameMenuUserInterface(ClientGame *game) : MenuUserInterface(game)
{
   setMenuID(GameMenuUI);
   mMenuTitle = "GAME MENU:";
}


void GameMenuUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   GameConnection *gc = getGame()->getConnectionToServer();

   if(gc && gc->waitingForPermissionsReply() && gc->gotPermissionsReply())      // We're waiting for a reply, and it has arrived
   {
      gc->setWaitingForPermissionsReply(false);
      buildMenu();                                                   // Update menu to reflect newly available options
   }
}


void GameMenuUserInterface::onActivate()
{
   Parent::onActivate();
   buildMenu();
   mMenuSubTitle = "";
   mMenuSubTitleColor = Colors::cyan;
}


void GameMenuUserInterface::onReactivate()
{
   mMenuSubTitle = "";
}



static void endGameCallback(ClientGame *game, U32 unused)
{
   game->closeConnectionToGameServer();

   if(gServerGame)
   {
      delete gServerGame;
      gServerGame = NULL;
   }
}


static void addTwoMinsCallback(ClientGame *game, U32 unused)
{
   if(game->getGameType())
      game->getGameType()->addTime(2 * 60 * 1000);

   game->getUIManager()->reactivatePrevUI();     // And back to our regularly scheduled programming!
}


static void chooseNewLevelCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(LevelTypeUI);
}


static void restartGameCallback(ClientGame *game, U32 unused)
{
   game->getConnectionToServer()->c2sRequestLevelChange(-2, false);
   game->getUIManager()->reactivatePrevUI();     // And back to our regularly scheduled programming! 
}


static void levelChangeOrAdminPWCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(LevelChangePasswordEntryUI);
}


static void kickPlayerCallback(ClientGame *game, U32 unused)
{
   PlayerMenuUserInterface *ui = game->getUIManager()->getPlayerMenuUserInterface();

   ui->action = PlayerMenuUserInterface::Kick;

   game->getUIManager()->activate(ui);
}


void GameMenuUserInterface::buildMenu()
{
   clearMenuItems();
   GameSettings *settings = getGame()->getSettings();
   
   // Save input mode so we can see if we need to display alert if it changes
   lastInputMode = settings->getInputCodeManager()->getInputMode();  


   addMenuItem(new MenuItem("OPTIONS",      optionsSelectedCallback, "", KEY_O));
   addMenuItem(new MenuItem("INSTRUCTIONS", helpSelectedCallback,    "", KEY_I, getInputCode(settings, InputCodeManager::BINDING_HELP)));

   GameType *gameType = getGame()->getGameType();

   // Add any game-specific menu items
   if(gameType)
   {
      mGameType = gameType;
      gameType->addClientGameMenuOptions(getGame(), this);
   }

   GameConnection *gc = (getGame())->getConnectionToServer();

   if(gc)
   {
      if(gc->getClientInfo()->isLevelChanger())
      {
         addMenuItem(new MenuItem("PLAY DIFFERENT LEVEL", chooseNewLevelCallback, "", KEY_L, KEY_P));
         addMenuItem(new MenuItem("ADD TIME (2 MINS)",    addTwoMinsCallback,     "", KEY_T, KEY_2));
         addMenuItem(new MenuItem("RESTART LEVEL",        restartGameCallback,    "", KEY_R));
      }

      if(gc->getClientInfo()->isAdmin())
      {
         // Add any game-specific menu items
         if(gameType)
         {
            mGameType = gameType;
            gameType->addAdminGameMenuOptions(this);
         }

         addMenuItem(new MenuItem("KICK A PLAYER", kickPlayerCallback, "", KEY_K));
      }

      // Owner already has max permissions, so don't show option to enter a password
      if(!gc->getClientInfo()->isOwner())
         addMenuItem(new MenuItem("ENTER PASSWORD", levelChangeOrAdminPWCallback, "", KEY_A, KEY_E));
   }

   if(getUIManager()->cameFrom(EditorUI))    // Came from editor
      addMenuItem(new MenuItem("RETURN TO EDITOR", endGameCallback, "", KEY_Q, KEY_R));
   else
      addMenuItem(new MenuItem("QUIT GAME",        endGameCallback, "", KEY_Q));
}


void GameMenuUserInterface::onEscape()
{
   getUIManager()->reactivatePrevUI();      //mGameUserInterface

   // Show alert about input mode changing, if needed
   bool inputModesChanged = (lastInputMode != getGame()->getSettings()->getInputCodeManager()->getInputMode());
   getUIManager()->getGameUserInterface()->resetInputModeChangeAlertDisplayTimer(inputModesChanged ? 2800 : 0);
}

////////////////////////////////////////
////////////////////////////////////////

// Constructor
LevelMenuUserInterface::LevelMenuUserInterface(ClientGame *game) : MenuUserInterface(game)
{
   setMenuID(LevelTypeUI);
}


static const char *UPLOAD_LEVELS = "UPLOAD LEVELS";
static const char *ALL_LEVELS = "All Levels";
static const U32 ALL_LEVELS_MENUID = 0x80000001;
static const U32 UPLOAD_LEVELS_MENUID = 0x80000002;


static void selectLevelTypeCallback(ClientGame *game, U32 level)
{
   LevelMenuSelectUserInterface *ui = game->getUIManager()->getLevelMenuSelectUserInterface();

   // First entry will be "All Levels", subsequent entries will be level types populated from mLevelInfos
   if(level == ALL_LEVELS_MENUID)
      ui->category = ALL_LEVELS;
   else if(level == UPLOAD_LEVELS_MENUID)
      ui->category = UPLOAD_LEVELS;

   else
   {
      // Replace the following with a getLevelCount() function on game??
      GameConnection *gc = game->getConnectionToServer();
      if(!gc || U32(gc->mLevelInfos.size()) < level)
         return;

      ui->category = gc->mLevelInfos[level - 1].getLevelTypeName();
   }

  game->getUIManager()->activate(ui);
}


void LevelMenuUserInterface::onActivate()
{
   Parent::onActivate();
   mMenuTitle = "CHOOSE LEVEL TYPE:";

   // replace with getLevelCount() method on game?
   GameConnection *gc = getGame()->getConnectionToServer();
   if(!gc || !gc->mLevelInfos.size())
      return;

   clearMenuItems();

   char c[] = "A";   // Shortcut key
   addMenuItem(new MenuItem(ALL_LEVELS_MENUID, ALL_LEVELS, selectLevelTypeCallback, "", InputCodeManager::stringToInputCode(c)));

   // Cycle through all levels, looking for unique type strings
   for(S32 i = 0; i < gc->mLevelInfos.size(); i++)
   {
      bool found = false;

      for(S32 j = 0; j < getMenuItemCount(); j++)
         if(strcmp(gc->mLevelInfos[i].getLevelTypeName(), "") == 0 || 
            strcmp(gc->mLevelInfos[i].getLevelTypeName(), getMenuItem(j)->getPrompt().c_str()) == 0)     
         {
            found = true;
            break;            // Skip over levels with blank names or duplicate entries
         }

      if(!found)              // Not found above, must be a new type
      {
         const char *gameTypeName = gc->mLevelInfos[i].getLevelTypeName();
         c[0] = gameTypeName[0];
         c[1] = '\0';
//         logprintf("LEVEL - %s", name.c_str());
         addMenuItem(new MenuItem(i + 1, gameTypeName, selectLevelTypeCallback, "", InputCodeManager::stringToInputCode(c)));
      }
   }

   sortMenuItems();

   if((gc->mSendableFlags & GameConnection::ServerFlagAllowUpload) && !gc->isLocalConnection())   // local connection is useless, already have all maps..
      addMenuItem(new MenuItem(UPLOAD_LEVELS_MENUID, UPLOAD_LEVELS, selectLevelTypeCallback, "", InputCodeManager::stringToInputCode(c)));
}


void LevelMenuUserInterface::onEscape()
{
   getUIManager()->reactivatePrevUI();    // to mGameUserInterface
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
LevelMenuSelectUserInterface::LevelMenuSelectUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(LevelUI);
}


static void processLevelSelectionCallback(ClientGame *game, U32 index)             
{
   game->getUIManager()->getLevelMenuSelectUserInterface()->processSelection(index);
}


const U32 UPLOAD_LEVELS_BIT = 0x80000000;

void LevelMenuSelectUserInterface::processSelection(U32 index)     
{
   Parent::onActivate();
   GameConnection *gc = getGame()->getConnectionToServer();

   if((index & UPLOAD_LEVELS_BIT) && (index & (~UPLOAD_LEVELS_BIT)) < U32(mLevels.size()))
   {
      FolderManager *folderManager = getGame()->getSettings()->getFolderManager();
      string filename = strictjoindir(folderManager->levelDir, mLevels[index & (~UPLOAD_LEVELS_BIT)]);

      if(!gc->s2rUploadFile(filename.c_str(), GameConnection::LevelFileTransmissionInProgress))
         getGame()->displayErrorMessage("!!! Can't upload level: unable to read file");
   }
   else
      gc->c2sRequestLevelChange(index, false);     // The selection index is the level to load

   getUIManager()->reactivate(GameUI);             // Back to the game menu
}


void LevelMenuSelectUserInterface::onActivate()
{
   Parent::onActivate();
   mMenuTitle = "CHOOSE LEVEL: [" + category + "]";

   // Replace with a getLevelCount() method on Game?
   ClientGame *game = getGame();
   GameConnection *gc = game->getConnectionToServer();

   if(!gc || !gc->mLevelInfos.size())
      return;

   clearMenuItems();

   mLevels.clear();

   char c[2];
   c[1] = 0;   // null termination

   if(!strcmp(category.c_str(), UPLOAD_LEVELS))
   {
      // Get all the playable levels in levelDir
      mLevels = getGame()->getSettings()->getLevelList();     

      for(S32 i = 0; i < mLevels.size(); i++)
      {
         c[0] = mLevels[i].c_str()[0];
         addMenuItem(new MenuItem(i | UPLOAD_LEVELS_BIT, mLevels[i].c_str(), processLevelSelectionCallback, "", InputCodeManager::stringToInputCode(c)));
      }
   }
 
   for(S32 i = 0; i < gc->mLevelInfos.size(); i++)
   {
      if(gc->mLevelInfos[i].mLevelName == "")   // Skip levels with blank names --> but all should have names now!
         continue;

      if(strcmp(gc->mLevelInfos[i].getLevelTypeName(), category.c_str()) == 0 || category == ALL_LEVELS)
      {
         const char *levelName = gc->mLevelInfos[i].mLevelName.getString();
         c[0] = levelName[0];
         addMenuItem(new MenuItem(i, levelName, processLevelSelectionCallback, "", InputCodeManager::stringToInputCode(c)));
      }
   }

   sortMenuItems();
   mFirstVisibleItem = 0;

   if(itemSelectedWithMouse)
      onMouseMoved();
   else
      selectedIndex = 0;
}


// Override parent, and make keys simply go to first level with that letter, rather than selecting it automatically
bool LevelMenuSelectUserInterface::processMenuSpecificKeys(InputCode inputCode)
{
   // First check for some shortcut keys
   for(S32 i = 0; i < getMenuItemCount(); i++)
   {
      // Lets us advance to next level with same starting letter  
      S32 indx = selectedIndex + i + 1;
      if(indx >= getMenuItemCount())
         indx -= getMenuItemCount();

      if(inputCode == getMenuItem(indx)->key1 || inputCode == getMenuItem(indx)->key2)
      {
         selectedIndex = indx;
         itemSelectedWithMouse = false;

         // Move the mouse to the new selection to make things "feel better"
         MenuItemSize size;
         S32 y = getYStart();

         for(S32 j = getOffset(); j < selectedIndex; j++)
         {
            size = getMenuItem(j)->getSize();
            y += getTextSize(size) + getGap(size);
         }

         y += getTextSize(size) / 2;

         // WarpMouse fires a mouse event, which will cause the cursor to become visible, which we don't want.  Therefore,
         // we must resort to the kind of gimicky/hacky method of setting a flag, telling us that we should ignore the
         // next mouse event that comes our way.  It might be better to handle this at the Event level, by creating a custom
         // method called WarpMouse that adds the suppression.  At this point, however, the only place we care about this
         // is here so...  well... this works.
#if SDL_VERSION_ATLEAST(2,0,0)
         SDL_WarpMouseInWindow(gScreenInfo.sdlWindow, (S32)gScreenInfo.getMousePos()->x, y);
#else
         SDL_WarpMouse(gScreenInfo.getMousePos()->x, y);
#endif
         Cursor::disableCursor();
         mIgnoreNextMouseEvent = true;
         playBoop();

         return true;
      }
   }

   return false;
}


void LevelMenuSelectUserInterface::onEscape()
{
   getUIManager()->reactivatePrevUI();    // to LevelMenuUserInterface
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
PlayerMenuUserInterface::PlayerMenuUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(PlayerUI);
}


static void playerSelectedCallback(ClientGame *game, U32 index) 
{
   game->getUIManager()->getPlayerMenuUserInterface()->playerSelected(index);
}


void PlayerMenuUserInterface::playerSelected(U32 index)
{
   // When we created the menu, names were not sorted, and item indices were assigned in "natural order".  Then
   // the menu items were sorted by name, and now the indices are now jumbled.  This bit here tries to get the
   // new, actual list index of an item given its original index.
   for(S32 i = 0; i < getMenuItemCount(); i++)
      if(getMenuItem(i)->getIndex() == (S32)index)
      {
         index = i;
         break;
      }

   GameType *gt = getGame()->getGameType();

   if(action == ChangeTeam)
   {
      TeamMenuUserInterface *ui = getUIManager()->getTeamMenuUserInterface();
      ui->nameToChange = getMenuItem(index)->getPrompt();

      getUIManager()->activate(TeamUI);     // Show menu to let player select a new team
   }

   else if(gt)    // action == Kick
      gt->c2sKickPlayer(getMenuItem(index)->getPrompt());


   if(action != ChangeTeam)                   // Unless we need to move on to the change team screen...
      getUIManager()->reactivate(GameUI);     // ...it's back to the game!
}


// By putting the menu building code in render, menus can be dynamically updated
void PlayerMenuUserInterface::render()
{
   clearMenuItems();

   GameConnection *conn = getGame()->getConnectionToServer();
   if(!conn)
      return;

   char c[] = "A";      // Dummy shortcut key
   for(S32 i = 0; i < getGame()->getClientCount(); i++)
   {
      ClientInfo *clientInfo = ((Game *)getGame())->getClientInfo(i);      // Lame!

      strncpy(c, clientInfo->getName().getString(), 1);        // Grab first char of name for a shortcut key

      // Will be used to show admin/player/robot prefix on menu
      PlayerType pt = clientInfo->isRobot() ? PlayerTypeRobot : (clientInfo->isAdmin() ? PlayerTypeAdmin : PlayerTypePlayer);    

      PlayerMenuItem *newItem = new PlayerMenuItem(i, clientInfo->getName().getString(), playerSelectedCallback, 
                                                   InputCodeManager::stringToInputCode(c), pt);
      newItem->setUnselectedColor(*getGame()->getTeamColor(clientInfo->getTeamIndex()));

      addMenuItem(newItem);
   }

   sortMenuItems();

   if(action == Kick)
      mMenuTitle = "CHOOSE PLAYER TO KICK:";
   else if(action == ChangeTeam)
      mMenuTitle = "CHOOSE WHOSE TEAM TO CHANGE:";
   Parent::render();
}


void PlayerMenuUserInterface::onEscape()
{
   getUIManager()->reactivatePrevUI();   //mGameUserInterface
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
TeamMenuUserInterface::TeamMenuUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(TeamUI);
   mMenuSubTitle = "[Human Players | Bots | Score]";
}


static void processTeamSelectionCallback(ClientGame *game, U32 index)        
{
   game->getUIManager()->getTeamMenuUserInterface()->processSelection(index);
}


void TeamMenuUserInterface::processSelection(U32 index)        
{
   GameType *gt = getGame()->getGameType();

   if(!gt)
      return;

   // Make sure user isn't just changing to the team they're already on...
   if(index != (U32)getGame()->getTeamIndex(nameToChange.c_str()))
   {
      if(getPrevMenuID() == PlayerUI)        // Initiated by an admin (PlayerUI is the kick/change team player-pick admin menu)
      {
         StringTableEntry e(nameToChange.c_str());
         gt->c2sTriggerTeamChange(e, index); // Index will be the team index
      }
      else                                   // Came from player changing own team
         gt->c2sChangeTeams(index); 
   }

   getUIManager()->reactivate(GameUI);       // Back to the game!
}


// By reconstructing our menu at render time, changes to teams caused by others will be reflected immediately
void TeamMenuUserInterface::render()
{
   clearMenuItems();

   getGame()->countTeamPlayers();                     // Make sure numPlayers is correctly populated

   char c[] = "A";                                    // Dummy shortcut key, will change below
   for(S32 i = 0; i < getGame()->getTeamCount(); i++)
   {
      AbstractTeam *team = getGame()->getTeam(i);
      strncpy(c, team->getName().getString(), 1);     // Grab first char of name for a shortcut key

      bool isCurrent = (i == getGame()->getTeamIndex(nameToChange.c_str()));
      
      addMenuItem(new TeamMenuItem(i, team, processTeamSelectionCallback, InputCodeManager::stringToInputCode(c), isCurrent));
   }

   string name = "";
   if(getGame()->getConnectionToServer() && getGame()->getConnectionToServer()->getControlObject())
   {
      Ship *ship = dynamic_cast<Ship *>(getGame()->getConnectionToServer()->getControlObject());
      if(ship && ship->getClientInfo())
         name = ship->getClientInfo()->getName().getString();
   }

   if(name != nameToChange)    // i.e. names differ, this isn't the local player
   {
      name = nameToChange;
      name += " ";
   }
   else
      name = "";

   // Finally, set menu title
   mMenuTitle = (string("TEAM TO SWITCH ") + name + "TO:").c_str();       // No space before the TO!

   Parent::render();
}


void TeamMenuUserInterface::onEscape()
{
   getUIManager()->reactivatePrevUI();
}


};

