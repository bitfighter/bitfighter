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
#include "UINameEntry.h"
#include "UIGame.h"
#include "UIQueryServers.h"
#include "UICredits.h"
#include "UIChat.h"
#include "UIEditor.h"
#include "UIInstructions.h"
#include "UIDiagnostics.h"
#include "UIKeyDefMenu.h"
#include "UIErrorMessage.h"
#include "gameObjectRender.h"    // For renderBitfighterLogo, glColor
#include "game.h"
#include "gameType.h"
#include "input.h"
//#include "keyCode.h"
#include "IniFile.h"
#include "config.h"
#include "Colors.h"
#include "ScreenInfo.h"
#include "Joystick.h"
#include "JoystickRender.h"

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"

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


extern void actualizeScreenMode(bool);
extern void exitGame();

////////////////////////////////////
////////////////////////////////////

// Max number of menu items we show on screen before we go into scrolling mode
#define MAX_MENU_SIZE S32((gScreenInfo.getGameCanvasHeight() - 150) / (getTextSize() + getGap()))   

// Constructor
MenuUserInterface::MenuUserInterface(ClientGame *game) : UserInterface(game)
{
   setMenuID(GenericUI);
   mMenuTitle = "Menu:";
   mMenuSubTitle = "";

   selectedIndex = 0;
   itemSelectedWithMouse = false;
   currOffset = 0;
   mRenderInstructions = true;
}

extern bool gDisableShipKeyboardInput;

// Gets run when menu is activated.  This is also called by almost all other menus/subclasses.
void MenuUserInterface::onActivate()
{
   gDisableShipKeyboardInput = true;       // Keep keystrokes from getting to game
   selectedIndex = 0;
   currOffset = 0;
}


void MenuUserInterface::onReactivate()
{
   gDisableShipKeyboardInput = true;       // Keep keystrokes from getting to game
}


void MenuUserInterface::idle(U32 timeDelta)
{
   // Controls rate of scrolling long menus with mouse
   mScrollTimer.update(timeDelta);

   // Call mouse handler so users can scroll scrolling menus just by holding mouse in position
   // (i.e. we don't want to limit scrolling action only to times when user moves mouse)
   if(itemSelectedWithMouse)
      processMouse();

   LineEditor::updateCursorBlink(timeDelta);
}


// Return index offset to account for scrolling menus
S32 MenuUserInterface::getOffset()
{
   S32 offset = 0;
   S32 count = menuItems.size();

   if(count > MAX_MENU_SIZE)     // Do some sort of scrolling
   {
      offset = currOffset;

      // itemSelectedWithMouse basically lets users highlight the top and bottom items in a scrolling list,
      // which can't be done when using the keyboard
      if(selectedIndex - currOffset < (itemSelectedWithMouse ? 0 : 1))
         offset = selectedIndex - (itemSelectedWithMouse ? 0 : 1);
      else if( selectedIndex - currOffset > (MAX_MENU_SIZE - (itemSelectedWithMouse ? 1 : 2)) )
         offset = selectedIndex - (MAX_MENU_SIZE - (itemSelectedWithMouse ? 1 : 2));

      if(offset < 0)
         offset = 0;
      else if(offset + MAX_MENU_SIZE >= menuItems.size())
         offset = menuItems.size() - MAX_MENU_SIZE;
   }
   currOffset = offset;

   return offset;
}


// Get vert pos of first menu item
S32 MenuUserInterface::getYStart()
{
   S32 vertOff = (getMenuID() == MainUI) ? 40 : 0;    // Make room for the logo on the main menu

   if(getMenuID() == GameParamsUI)  // If we're on the GameParams menu, start at a constant position
      return 70;
   else                             // Otherwise, attpempt to center the menu vertically
      return (gScreenInfo.getGameCanvasHeight() - min(menuItems.size(), MAX_MENU_SIZE) * (getTextSize() + getGap())) / 2 + vertOff;
}


extern IniSettings gIniSettings;

static void renderMenuInstructions()
{
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   S32 y = canvasHeight - UserInterface::vertMargin - 20;
   const S32 size = 18;

    glColor3f(1, 1, 1);     // white

   if(gIniSettings.inputMode == InputModeKeyboard)
     UserInterface::drawCenteredString(y, size, "UP, DOWN to choose | ENTER to select | ESC exits menu");
   else
   {
     S32 totalWidth = JoystickRender::getControllerButtonRenderedSize(BUTTON_DPAD_UP) +
           JoystickRender::getControllerButtonRenderedSize(BUTTON_DPAD_DOWN) +
           JoystickRender::getControllerButtonRenderedSize(BUTTON_START) +
           JoystickRender::getControllerButtonRenderedSize(BUTTON_BACK) +
           UserInterface::getStringWidth(size, "to choose |  to select |  exits menu");

     S32 x = canvasWidth / 2 - UserInterface::horizMargin - totalWidth/2;

     JoystickRender::renderControllerButton((F32)x, (F32)y, BUTTON_DPAD_UP, false);
     x += JoystickRender::getControllerButtonRenderedSize(BUTTON_DPAD_UP) + UserInterface::getStringWidth(size, " ");

     JoystickRender::renderControllerButton((F32)x, (F32)y, BUTTON_DPAD_DOWN, false);
     x += JoystickRender::getControllerButtonRenderedSize(BUTTON_DPAD_DOWN) + UserInterface::getStringWidth(size, " ");

     glColor3f(1,1,1);
     static const char *msg1 = "to choose | ";
     UserInterface::drawString(x, y, size, msg1);
     x += UserInterface::getStringWidth(size, msg1);

     JoystickRender::renderControllerButton((F32)x, F32(y + 4), BUTTON_START, false);
     x += JoystickRender::getControllerButtonRenderedSize(BUTTON_START);

     glColor3f(1,1,1);
     static const char *msg2 = "to select | ";
     UserInterface::drawString(x, y, size, msg2);
     x += UserInterface::getStringWidth(size, msg2);

     JoystickRender::renderControllerButton(F32(x + 4), F32(y + 4), BUTTON_BACK, false);
     x += JoystickRender::getControllerButtonRenderedSize(BUTTON_BACK) + 4;

     glColor3f(1,1,1);
     UserInterface::drawString(x, y, size, "exits menu");
   }
}


static const S32 ARROW_WIDTH = 100;
static const S32 ARROW_HEIGHT = 20;
static const S32 ARROW_MARGIN = 5;

static void renderArrowAbove(S32 pos, S32 height)
{
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   for(S32 i = 1; i >= 0; i--)
   {
      // First create a black poly to blot out what's behind, then the arrow itself
      glColor(i ? Colors::black : Colors::blue);
      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2i( (canvasWidth - ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7);
         glVertex2i( (canvasWidth + ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7);
         glVertex2i(canvasWidth / 2, pos - (height + ARROW_MARGIN ) - 7);
      glEnd();
   }
}


static void renderArrowBelow(S32 pos, S32 height)
{
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   for(S32 i = 1; i >= 0; i--)
   {
      // First create a black poly to blot out what's behind, then the arrow itself
      glColor(i ? Colors::black : Colors::blue);
      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2i( (canvasWidth - ARROW_WIDTH) / 2, pos + ARROW_MARGIN - 7);
         glVertex2i( (canvasWidth + ARROW_WIDTH) / 2, pos + ARROW_MARGIN - 7);
         glVertex2i(canvasWidth / 2, pos + (height + ARROW_MARGIN) - 7);
      glEnd();
   }
}


// Basic menu rendering
void MenuUserInterface::render()
{
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   // Draw the game screen, then dim it out so you can still see it under our overlay
   if(getGame()->getConnectionToServer())
   {
      if(getUIManager()->getPrevUI() != this)  // TODO: fix problem with prevUI being the same as this causing errors
         getUIManager()->renderPrevUI();

      glColor4f(0, 0, 0, 0.6f);

      glEnableBlend;
         glBegin(GL_POLYGON);
            glVertex2i(0, 0);
            glVertex2i(canvasWidth, 0);
            glVertex2i(canvasWidth, canvasHeight);
            glVertex2i(0, canvasHeight);
         glEnd();
      glDisableBlend;
   }

   glColor(Colors::white);    
   drawCenteredString(vertMargin, 30, mMenuTitle.c_str());

   glColor(mMenuSubTitleColor);
   drawCenteredString(vertMargin + 35, 18, mMenuSubTitle.c_str());

   if(mRenderInstructions)
      renderMenuInstructions();

   S32 count = menuItems.size();

   if(count > MAX_MENU_SIZE)     // Need some sort of scrolling?
      count = MAX_MENU_SIZE;

   S32 yStart = getYStart();
   S32 offset = getOffset();

   S32 adjfact = 0;              // Just because it looks good  (was 2, which looks crappy on gameParams menu.  0 seems to look ok everywhere.
   S32 shrinkfact = 1;

   for(S32 i = 0; i < count; i++)
   {
      S32 y = yStart + i * (getTextSize() + getGap());

      if(selectedIndex == i + offset)  // Highlight selected item
         for(S32 j = 1; j >= 0; j--)
         {
            glColor(j ? Color(0,0,0.4) : Colors::blue);   // Fill, then outline
            glBegin(j ? GL_POLYGON : GL_LINES);
               glVertex2i(0,           y - getGap() / 2 + adjfact + shrinkfact);
               glVertex2i(canvasWidth, y - getGap() / 2 + adjfact + shrinkfact);
               glVertex2i(canvasWidth, y + getTextSize() + getGap() / 2 + adjfact - shrinkfact);
               glVertex2i(0,           y + getTextSize() + getGap() / 2 + adjfact - shrinkfact);
            glEnd();
         }

      menuItems[i+offset]->render(y, getTextSize(), selectedIndex == i+offset);
   }

   // Render an indicator that there are scrollable items above and/or below
   if(menuItems.size() > MAX_MENU_SIZE)
   {
      glColor(Colors::blue);

      if(offset > 0)                                  // There are items above
         renderArrowAbove(yStart, ARROW_HEIGHT);

      if(offset < menuItems.size() - MAX_MENU_SIZE)    // There are items below
         renderArrowBelow(yStart + (getTextSize() + getGap()) * MAX_MENU_SIZE, ARROW_HEIGHT);
   }

   // Render a help string at the bottom of the menu
   const S32 helpFontSize = 15;
   glColor(Colors::green);
   S32 ypos = canvasHeight - vertMargin - 50;

   // Render a special instruction line (should this be a method of CounterMenuItemType?
   UserInterface::drawCenteredString(ypos, helpFontSize, menuItems[selectedIndex]->getSpecialEditingInstructions() );

   ypos -= helpFontSize + 5;
   drawCenteredString(ypos, helpFontSize, menuItems[selectedIndex]->getHelp());

   renderExtras();  // Draw something unique on a menu
}


// Handle mouse input, figure out which menu item we're over, and highlight it
void MenuUserInterface::onMouseMoved()
{
   // Really only matters when starting to host game... don't want to be able to change menu items while the levels are loading.
   // This is purely an aesthetic issue, a minor irritant.
   if(gServerGame && gServerGame->hostingModePhase == ServerGame::LoadingLevels)
      return;

   itemSelectedWithMouse = true;
   SDL_ShowCursor(SDL_ENABLE);  // Show cursor when user moves mouse

   selectedIndex = U32( floor(( gScreenInfo.getMousePos()->y - getYStart() + 10 ) / (getTextSize() + getGap())) ) + currOffset;

   processMouse();
}


void MenuUserInterface::processMouse()
{
   if(menuItems.size() > MAX_MENU_SIZE)   // We have a scrolling situation here...
   {
      if(selectedIndex < currOffset)      // Scroll up
      {
         if(!mScrollTimer.getCurrent() && currOffset > 0)
         {
            currOffset--;
            mScrollTimer.reset(100);
         }
         selectedIndex = currOffset;
      }
      else if(selectedIndex > currOffset + MAX_MENU_SIZE - 1)   // Scroll down
      {
         if(!mScrollTimer.getCurrent() && selectedIndex > currOffset + MAX_MENU_SIZE - 2)
         {
            currOffset++;
            mScrollTimer.reset(MOUSE_SCROLL_INTERVAL);
         }
         selectedIndex = currOffset + MAX_MENU_SIZE - 1;
      }
      else
         mScrollTimer.clear();
   }

   if(selectedIndex < 0)                              // Scrolled off top of list
   {
      selectedIndex = 0;
      currOffset = 0;
   }
   else if(selectedIndex >= menuItems.size())         // Scrolled off bottom of list
   {
      selectedIndex = menuItems.size() - 1;
      currOffset = max(menuItems.size() - MAX_MENU_SIZE, 0);
   }
}


// All key handling now under one roof!
void MenuUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_UNKNOWN)
      return;

   // Check for in autorepeat mode
   mRepeatMode = mKeyDown;
   mKeyDown = true;

   // Handle special case of keystrokes during hosting preparation phases
   if(gServerGame && (gServerGame->hostingModePhase == ServerGame::LoadingLevels || 
                      gServerGame->hostingModePhase == ServerGame::DoneLoadingLevels))
   {
      if(keyCode == KEY_ESCAPE)     // can only get here when hosting
      {
         gServerGame->hostingModePhase = ServerGame::NotHosting;
         getUIManager()->getHostMenuUserInterface()->clearLevelLoadDisplay();
         endGame();
      }
      return;
   }

   MainMenuUserInterface *ui = getUIManager()->getMainMenuUserInterface();

   if(!ui->mFirstTime)
      ui->showAnimation = false;    // Stop animations if a key is pressed

   menuItems[selectedIndex]->handleKey(keyCode, ascii) || processMenuSpecificKeys(keyCode, ascii) || processKeys(keyCode, ascii);

   // Finally, since the user has indicated they want to use keyboard/controller input, hide the cursor
   if(keyCode != MOUSE_LEFT && keyCode != MOUSE_MIDDLE && keyCode != MOUSE_RIGHT && keyCode != KEY_ESCAPE)
      SDL_ShowCursor(SDL_DISABLE);
}


void MenuUserInterface::onKeyUp(KeyCode keyCode)
{
   mKeyDown = false;
   mRepeatMode = false;
}


// Generic handler looks for keystrokes and translates them into menu actions
bool MenuUserInterface::processMenuSpecificKeys(KeyCode keyCode, char ascii)
{
   // First check for some shortcut keys

   for(S32 i = 0; i < menuItems.size(); i++)
   {
      if(keyCode == menuItems[i]->key1 || keyCode == menuItems[i]->key2)
      {
         selectedIndex = i;

         menuItems[i]->activatedWithShortcutKey();
         return true;
      }
   }

   return false;
}


// Process the keys that work on all menus
bool MenuUserInterface::processKeys(KeyCode keyCode, char ascii)
{
   keyCode = convertJoystickToKeyboard(keyCode);

   if(keyCode == KEY_LEFT || keyCode == KEY_RIGHT || keyCode == MOUSE_LEFT || keyCode == MOUSE_RIGHT)
   {
      menuItems[selectedIndex]->handleKey(keyCode, ascii);
      playBoop();
   }

   else if(keyCode == KEY_SPACE || keyCode == KEY_RIGHT || keyCode == KEY_ENTER || keyCode == MOUSE_LEFT)
   {
      playBoop();
      if(keyCode != MOUSE_LEFT)
         itemSelectedWithMouse = false;

      else // it was MOUSE_LEFT after all
      {
         // Make sure we're actually pointing at a menu item before we process it
         S32 yStart = getYStart();
         const Point *mousePos = gScreenInfo.getMousePos();

         if(mousePos->y < yStart || mousePos->y > yStart + (menuItems.size() + 1) * (getTextSize() + getGap()))
            return true;
      }

      menuItems[selectedIndex]->handleKey(keyCode, ascii);

      if(menuItems[selectedIndex]->enterAdvancesItem())
         advanceItem();
   }

   else if(keyCode == KEY_ESCAPE)
   {
      playBoop();
      onEscape();
   }
   else if(keyCode == KEY_UP || (keyCode == KEY_TAB && getKeyState(KEY_SHIFT)))   // Prev item
   {
      selectedIndex--;
      itemSelectedWithMouse = false;

      if(selectedIndex < 0)                        // Scrolling off the top
      {
         if((menuItems.size() > MAX_MENU_SIZE) && mRepeatMode)        // Allow wrapping on long menus only when not in repeat mode
         {
            selectedIndex = 0;               // No wrap --> (first item)
            return true;                     // (leave before playBoop)
         }
         else                                      // Always wrap on shorter menus
            selectedIndex = menuItems.size() - 1;  // Wrap --> (select last item)
      }
      playBoop();
   }

   else if(keyCode == KEY_DOWN || keyCode == KEY_TAB)    // Next item
      advanceItem();

   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
   {
      getUIManager()->getChatUserInterface()->activate();
      playBoop();
   }
   else if(keyCode == keyDIAG)            // Turn on diagnostic overlay
   {
      getUIManager()->getDiagnosticUserInterface()->activate();
      playBoop();
   }

   return true;      // Probably wrong, but doesn't really matter at this point
}


void MenuUserInterface::advanceItem()
{
   selectedIndex++;
   itemSelectedWithMouse = false;

   if(selectedIndex >= menuItems.size())     // Scrolling off the bottom
   {
      if((menuItems.size() > MAX_MENU_SIZE) && mRepeatMode)     // Allow wrapping on long menus only when not in repeat mode
      {
         selectedIndex = menuItems.size() - 1;                  // No wrap --> (last item)
         return;                                                // (leave before playBoop)
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


//////////
// MenuUserInterface callbacks
//////////

static void joinSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getQueryServersUserInterface()->activate();
}

static void hostSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getHostMenuUserInterface()->activate();
}

static void helpSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getInstructionsUserInterface()->activate();
}

static void optionsSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getOptionsMenuUserInterface()->activate();
}

static void editorSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getEditorUserInterface()->setLevelFileName("");      // Reset this so we get the level entry screen
   game->getUIManager()->getEditorUserInterface()->activate();
}

static void creditsSelectedCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getCreditsUserInterface()->activate();
}

static void quitSelectedCallback(ClientGame *game, U32 unused)
{
   exitGame();
}

//////////

// Constructor
MainMenuUserInterface::MainMenuUserInterface(ClientGame *game) : Parent(game)
{
   showAnimation = true;
   mFirstTime = true;
   setMenuID(MainUI);
   mMenuTitle = "";
   mMOTD[0] = 0;
   mMenuSubTitle = "";
   mRenderInstructions = false;

   mNeedToUpgrade = false;           // Assume we're up-to-date until we hear from the master
   mShowedUpgradeAlert = false;      // So we don't show the upgrade message more than once

   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(game, 0, "JOIN LAN/INTERNET GAME", joinSelectedCallback,    "", KEY_J)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(game, 0, "HOST GAME",              hostSelectedCallback,    "", KEY_H)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(game, 0, "INSTRUCTIONS",           helpSelectedCallback,    "", KEY_I, keyHELP)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(game, 0, "OPTIONS",                optionsSelectedCallback, "", KEY_O)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(game, 0, "LEVEL EDITOR",           editorSelectedCallback,  "", KEY_L, KEY_E)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(game, 0, "CREDITS",                creditsSelectedCallback, "", KEY_C)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(game, 0, "QUIT",                   quitSelectedCallback,    "", KEY_Q)));
}


void MainMenuUserInterface::onActivate()
{
   // Time for a clean start.  No matter how we got here, there's no going back.
   // Needed mainly because the editor makes things confusing.  Now that that's been reworked,
   // it's probably not needed at all.
   getUIManager()->clearPrevUIs();

   mFadeInTimer.reset(FadeInTime);
   mColorTimer.reset(ColorTime);
   mColorTimer2.reset(ColorTime2);
   mTransDir = true;

   mFirstTime = false;

   if(showAnimation)
      getUIManager()->getSplashUserInterface()->activate();   // Show splash screen the first time through
}


// Set the MOTD we recieved from the master
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
   Parent::render();

   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   if(mMOTD != "")
   {
      U32 width = getStringWidth(20, mMOTD);
      glColor(Colors::white);
      U32 totalWidth = width + canvasWidth;
      U32 pixelsPerSec = 100;
      U32 delta = getGame()->getCurrentTime() - motdArriveTime;
      delta = U32(delta * pixelsPerSec * 0.001) % totalWidth;

      drawString(canvasWidth - delta, MOTD_POS, 20, mMOTD);
   }

   // Fade in the menu here if we are showing it the first time...  this will tie in
   // nicely with the splash screen, and make the transition less jarring and sudden
   if(showAnimation)
   {
      glEnableBlend;
         glBegin(GL_POLYGON);
            glColor4f(0, 0, 0, (F32) mFadeInTimer.getCurrent() / (F32) FadeInTime);
            glVertex2i(0, 0);
            glVertex2i(canvasWidth, 0);
            glVertex2i(canvasWidth, canvasHeight);
            glVertex2i(0, canvasHeight);
         glEnd();
      glDisableBlend;
   }

   // Render logo at top, never faded
   renderStaticBitfighterLogo();
}


void MainMenuUserInterface::idle(U32 timeDelta)
{
   mFadeInTimer.update(timeDelta);
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
   glColor3f(1,1,1);
   S32 size = 20;
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

   ui->activate();

   mShowedUpgradeAlert = true;            // Only show this alert once per session -- we don't need to beat them over the head with it!
}


// Take action based on menu selection
void MainMenuUserInterface::processSelection(U32 index)
{
   if(!mFirstTime)
      showAnimation = false;
}


void MainMenuUserInterface::onEscape()
{
   exitGame();    // Quit!
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


static string getVolMsg(F32 volume)
{
   S32 vol = U32((volume + 0.05) * 10.0);

   string msg = itos(vol);

   if(vol == 0)
      msg += " [MUTE]";

   return msg;
}


//////////
// Callbacks for Options menu
static void setSFXVolumeCallback(ClientGame *game, U32 vol)
{
   game->getIniSettings()->sfxVolLevel = F32(vol) / 10;
}

static void setMusicVolumeCallback(ClientGame *game, U32 vol)
{
   game->getIniSettings()->musicVolLevel = F32(vol) / 10;
}

static void setVoiceVolumeCallback(ClientGame *game, U32 vol)
{
   game->getIniSettings()->voiceChatVolLevel = F32(vol) / 10;
}


static void setControlsCallback(ClientGame *game, U32 val)
{
   game->getIniSettings()->controlsRelative = (val == 1);
}


static void setFullscreenCallback(ClientGame *game, U32 mode)
{
   game->getIniSettings()->oldDisplayMode = game->getIniSettings()->displayMode;     // Save existing setting

   game->getIniSettings()->displayMode = (DisplayMode)mode;
   actualizeScreenMode(false);
}


static void defineKeysCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getKeyDefMenuUserInterface()->activate();
}

static void setControllerCallback(ClientGame *game, U32 jsType)
{
   game->getIniSettings()->joystickType = jsType;
}


static void addStickOptions(Vector<string> *opts)
{
   opts->clear();
   opts->push_back("KEYBOARD");
   
   for(S32 i = 0; i < Joystick::DetectedJoystickNameList.size(); i++)
      opts->push_back(string("JOYSTICK ") + itos(i+1));
}


static S32 INPUT_MODE_MENU_ITEM_INDEX = 0;

static void setInputModeCallback(ClientGame *game, U32 val)
{
   S32 sticks = Joystick::DetectedJoystickNameList.size();

   if(sticks != Joystick::DetectedJoystickNameList.size())
   {
      ToggleMenuItem *menuItem = dynamic_cast<ToggleMenuItem *>(game->getUIManager()->getOptionsMenuUserInterface()->menuItems[INPUT_MODE_MENU_ITEM_INDEX].get());

      if(menuItem)
         addStickOptions(&menuItem->mOptions);

      if(val > (U32)Joystick::DetectedJoystickNameList.size())
      {
         val = 0;
         menuItem->setValueIndex(0);
      }

      // Special case handler for common situation
      if(sticks == 0 && Joystick::DetectedJoystickNameList.size() == 1)      // User just plugged a stick in
         menuItem->setValueIndex(1);
   }

   game->getIniSettings()->inputMode = (val == 0) ? InputModeKeyboard : InputModeJoystick;
   if(val >= 1) 
      Joystick::UseJoystickNumber = val - 1;

   Joystick::initJoystick();      // Will allow people to plug in joystick while in this menu...
}


static void setVoiceEchoCallback(ClientGame *game, U32 val)
{
   game->getIniSettings()->echoVoice = (val == 1);
}

//////////

MenuItem *getWindowModeMenuItem(ClientGame *game)
{
   Vector<string> opts;   
   opts.push_back("WINDOWED");
   opts.push_back("FULLSCREEN STRETCHED");
   opts.push_back("FULLSCREEN");

   return new ToggleMenuItem(game, "DISPLAY MODE:", opts, (U32)game->getIniSettings()->displayMode, true, 
                             setFullscreenCallback, "Set the game mode to windowed or fullscreen", KEY_G);
}


void OptionsMenuUserInterface::setupMenus()
{
   menuItems.clear();
   
   Vector<string> opts;
   opts.push_back("ABSOLUTE");
   opts.push_back("RELATIVE");
   menuItems.push_back(boost::shared_ptr<MenuItem>(new ToggleMenuItem(getGame(), "CONTROLS:", opts, gIniSettings.controlsRelative ? 1 : 0, true, 
                                                   setControlsCallback, "Set controls to absolute or relative mode",    KEY_C)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(getWindowModeMenuItem(getGame())));

   Joystick::initJoystick();   // Refresh joystick list

   addStickOptions(&opts);

   menuItems.push_back(boost::shared_ptr<MenuItem>(new ToggleMenuItem(getGame(),
                                                                      "PRIMARY INPUT:", 
                                                                      opts, 
                                                                      (U32)gIniSettings.inputMode,
                                                                      true, 
                                                                      setInputModeCallback, 
                                                                      "Specify whether you want to play with your keyboard or joystick", 
                                                                      KEY_P, KEY_I)));

   INPUT_MODE_MENU_ITEM_INDEX = menuItems.size() - 1;

   opts.clear();
   for(S32 i = 0; i < ControllerTypeCount; i++)
      opts.push_back(Joystick::joystickTypeToPrettyString(i));

   // Simple bounds check -- could be GenericController, UnknownController, or NoController
   U32 selectedOption = gIniSettings.joystickType < ControllerTypeCount ? gIniSettings.joystickType : 0;

   menuItems.push_back(boost::shared_ptr<MenuItem>(new ToggleMenuItem(getGame(), "JOYSTICK:", opts, selectedOption, true, 
                                                                      setControllerCallback, "Choose which joystick to use in joystick mode", KEY_J)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), menuItems.size(), "DEFINE KEYS / BUTTONS", defineKeysCallback, 
                                                                "Remap keyboard or joystick controls", KEY_D, KEY_K)));

   opts.clear();
   for(S32 i = 0; i <= 10; i++)
      opts.push_back(getVolMsg( F32(i) / 10 ));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new ToggleMenuItem(getGame(), "SFX VOLUME:",        opts, U32((gIniSettings.sfxVolLevel + 0.05) * 10.0), false, 
                       setSFXVolumeCallback,   "Set sound effects volume", KEY_S)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new ToggleMenuItem(getGame(), "MUSIC VOLUME:",      opts, U32((gIniSettings.musicVolLevel + 0.05) * 10.0), false,
                       setMusicVolumeCallback, "Set music volume", KEY_M)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new ToggleMenuItem(getGame(), "VOICE CHAT VOLUME:", opts, U32((gIniSettings.voiceChatVolLevel + 0.05) * 10.0), false, 
                       setVoiceVolumeCallback, "Set voice chat volume",    KEY_V)));

   // No music yet, so keep this out to keep menus from getting too long.  Uncomment when we have music.
   //menuItems.push_back(new MenuItem("MUSIC VOLUME:", getVolMsg(gIniSettings.musicVolLevel), 6, KEY_M, KEY_UNKNOWN));

   opts.clear();
   opts.push_back("DISABLED");
   opts.push_back("ENABLED");
   menuItems.push_back(boost::shared_ptr<MenuItem>(new ToggleMenuItem(getGame(), "VOICE ECHO:", opts, gIniSettings.echoVoice ? 1 : 0, true, 
                                                   setVoiceEchoCallback, "Toggle whether you hear your voice on voice chat",  KEY_E)));
}


static bool isFullScreen(DisplayMode displayMode)
{
   return displayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED || displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;
}


void OptionsMenuUserInterface::toggleDisplayMode()
{
   DisplayMode oldMode = gIniSettings.oldDisplayMode;

   gIniSettings.oldDisplayMode = gIniSettings.displayMode;     // Save current setting
   // When we're in the editor, and we toggle views, we'll skip one of the fullscreen modes, as they essentially do the same thing in that UI
   if(UserInterface::current->getMenuID() == EditorUI)
   {
      if(isFullScreen(gIniSettings.displayMode))
         gIniSettings.displayMode = DISPLAY_MODE_WINDOWED;

      // If we know what the previous fullscreen mode was, use that
      else if(isFullScreen(oldMode))
         gIniSettings.displayMode = oldMode;

      // Otherwise, pick some sort of full-screen mode...
      else
         gIniSettings.displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;
   }
   else  // Not in the editor, just advance to the next mode
   {
      DisplayMode mode = DisplayMode((U32)gIniSettings.displayMode + 1);
      gIniSettings.displayMode = (mode == DISPLAY_MODE_UNKNOWN) ? (DisplayMode) 0 : mode;    // Bounds check 
   }

   actualizeScreenMode(false);
}


// Save options to INI file, and return to our regularly scheduled program
void OptionsMenuUserInterface::onEscape()
{
   saveSettingsToINI(&gINI);
   getUIManager()->reactivatePrevUI();      //mGameUserInterface
}


////////////////////////////////////////
////////////////////////////////////////

extern string gPlayerPassword;
extern ClientInfo gClientInfo;


// Constructor
NameEntryUserInterface::NameEntryUserInterface(ClientGame *game) : MenuUserInterface(game)
{
   setMenuID(OptionsUI);
   mMenuTitle = "ENTER YOUR NICKNAME:";
   mReason = NetConnection::ReasonNone;
}


void NameEntryUserInterface::setReactivationReason(NetConnection::TerminationReason r) 
{ 
   mReason = r; 
   mMenuTitle = ""; 
}


void NameEntryUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenu();
   getGame()->setReadyToConnectToMaster(false);
}


extern void seedRandomNumberGenerator(string name);

// User is ready to move on... deal with it
static void nameAndPasswordAcceptCallback(ClientGame *clientGame, U32 unused)
{
   UIManager *uiManager = clientGame->getUIManager();
   NameEntryUserInterface *ui = uiManager->getNameEntryUserInterface();

   if(uiManager->hasPrevUI())
      uiManager->reactivatePrevUI();
   else
      uiManager->getMainMenuUserInterface()->activate();

   clientGame->resetMasterConnectTimer();

   clientGame->getIniSettings()->lastName     = gClientInfo.name = ui->menuItems[1]->getValueForWritingToLevelFile();
   clientGame->getIniSettings()->lastPassword = gPlayerPassword  = ui->menuItems[2]->getValueForWritingToLevelFile();

   saveSettingsToINI(&gINI);             // Get that baby into the INI file

   clientGame->setReadyToConnectToMaster(true);
   seedRandomNumberGenerator(gClientInfo.name);
   //gClientInfo.id.getRandom();          // Generate a player ID - messes up with the rename and Authentication
   if(clientGame->getConnectionToServer())                 // Rename while in game server, if connected
      clientGame->getConnectionToServer()->c2sRenameClient(gClientInfo.name);
}


void NameEntryUserInterface::setupMenu()
{
   menuItems.clear();

   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "OK", nameAndPasswordAcceptCallback, "")));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new EditableMenuItem(getGame(), "NICKNAME:", gClientInfo.name, "ChumpChange", "", MAX_PLAYER_NAME_LENGTH)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new EditableMenuItem(getGame(), "PASSWORD:", gPlayerPassword, "", "", MAX_PLAYER_PASSWORD_LENGTH)));
   
   menuItems[1]->setFilter(LineEditor::noQuoteFilter);      // quotes are incompatible with PHPBB3 logins
   menuItems[2]->setSecret(true);
}


void NameEntryUserInterface::renderExtras()
{
   const S32 size = 15;
   const S32 gap = 5;
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   const S32 rows = 3;
   S32 row = 0;

   glColor3f(0,1,0);

   drawCenteredString(canvasHeight - vertMargin - 30 - (rows - row) * size - (rows - row) * gap, size, 
            "You can skip this screen by editing the [Settings] section of Bitfighter.ini");
   row++;

   drawCenteredString(canvasHeight - vertMargin - 30 - (rows - row) * size - (rows - row) * gap, size, 
            "A password is only needed if you are using a reserved name.  You can reserve your");
   row++;

   drawCenteredString(canvasHeight - vertMargin - 30 - (rows - row) * size - (rows - row) * gap, size, 
            "nickname by registering for the bitfighter.org forums.  Registration is free.");


   if(mReason == NetConnection::ReasonBadLogin || mReason == NetConnection::ReasonInvalidUsername)
   {
      const char *message[] = { "If you have reserved this name by registering for",
                                "the forums, enter your forum password below. Otherwise,",
                                "this user name may be reserved. Please choose another."
                              };

      renderMessageBox("Invalid Name or Password", "", message, 3, -190);
   }
}

// Save options to INI file, and return to our regularly scheduled program
void NameEntryUserInterface::onEscape()
{
   exitGame();
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


extern string gHostName, gHostDescr;
extern string gLevelChangePassword, gAdminPassword, gServerPassword;
extern void initHostGame(Address bindAddress, Vector<string> &levelList, bool testMode);

static void startHostingCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getHostMenuUserInterface()->saveSettings();

   Vector<string> levelList = LevelListLoader::buildLevelList();
   initHostGame(Address(IPProtocol, Address::Any, 28000), levelList, false);
}

void HostMenuUserInterface::setupMenus()
{
   menuItems.clear();

   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "START HOSTING", startHostingCallback, "", KEY_H)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new EditableMenuItem(getGame(), "SERVER NAME:", gHostName, "<Bitfighter Host>", "", QueryServersUserInterface::MaxServerNameLen,  KEY_N)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new EditableMenuItem(getGame(), "DESCRIPTION:", gHostDescr, "<Empty>",                    
                                                                        "", QueryServersUserInterface::MaxServerDescrLen, KEY_D)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new EditableMenuItem(getGame(), "LEVEL CHANGE PASSWORD:", gLevelChangePassword, "<Anyone can change levels>", 
                                                                        "", MAX_PASSWORD_LENGTH, KEY_L)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new EditableMenuItem(getGame(), "ADMIN PASSWORD:",        gAdminPassword,       "<No remote admin access>",   
                                                                        "", MAX_PASSWORD_LENGTH, KEY_A)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new EditableMenuItem(getGame(), "CONNECTION PASSWORD:",   gServerPassword,      "<Anyone can connect>",       
                                                   "", MAX_PASSWORD_LENGTH, KEY_C)));

   menuItems.push_back(boost::shared_ptr<MenuItem>(new YesNoMenuItem(getGame(), "ALLOW MAP DOWNLOADS:", gIniSettings.allowGetMap, NULL, "", KEY_M)));

   //menuItems.push_back(boost::shared_ptr<MenuItem>(new CounterMenuItem("MAXIMUM PLAYERS:",   gIniSettings.maxplayers, 1, 2, MAX_PLAYERS, "", "", "", KEY_P)));
   //menuItems.push_back(boost::shared_ptr<MenuItem>(new EditableMenuItem("PORT:",                  "28000",              "Use default of 28000", 
   //                                         "", 10, KEY_P)));
}


// Save options to INI file, and return to our regularly scheduled program
// This only gets called when escape not already handled by preprocessKeys(), i.e. when we're not editing
void HostMenuUserInterface::onEscape()
{
   saveSettings();
   getUIManager()->reactivatePrevUI();     
}


// Save parameters in INI file
void HostMenuUserInterface::saveSettings()
{
   gHostName            = gIniSettings.hostname            = menuItems[OPT_NAME]->getValue();
   gHostDescr           = gIniSettings.hostdescr           = menuItems[OPT_DESCR]->getValue();
   gLevelChangePassword = gIniSettings.levelChangePassword = menuItems[OPT_LVL_PASS]->getValue();
   gAdminPassword       = gIniSettings.adminPassword       = menuItems[OPT_ADMIN_PASS]->getValue();    
   gServerPassword      = gIniSettings.serverPassword      = menuItems[OPT_PASS]->getValue();
   gIniSettings.allowGetMap                                = menuItems[OPT_GETMAP]->getValue() == "yes";
   //gIniSettings.maxplayers                                 = menuItems[OPT_MAX_PLAYERS]->getIntValue();

   saveSettingsToINI(&gINI);
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
      glEnableBlend;
      for(S32 i = 0; i < mLevelLoadDisplayNames.size(); i++)
      {
         glColor4f(1,1,1, (1.4f - ((F32) (mLevelLoadDisplayNames.size() - i) / 10.f)) * 
                                        (levelLoadDisplayDisplay ? 1 : levelLoadDisplayFadeTimer.getFraction()) );
         drawStringf(100, gScreenInfo.getGameCanvasHeight() - vertMargin - (mLevelLoadDisplayNames.size() - i) * 20, 
                     15, "%s", mLevelLoadDisplayNames[i].c_str());
      }
      glDisableBlend;
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
   endGame();
}


static void addTwoMinsCallback(ClientGame *game, U32 unused)
{
   if(game->getGameType())
      game->getGameType()->addTime(2 * 60 * 1000);

   game->getUIManager()->reactivatePrevUI();     // And back to our regularly scheduled programming!
}


static void chooseNewLevelCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getLevelMenuUserInterface()->activate();
}


static void restartGameCallback(ClientGame *game, U32 unused)
{
   game->getConnectionToServer()->c2sRequestLevelChange(-2, false);
   game->getUIManager()->reactivatePrevUI();     // And back to our regularly scheduled programming! 
}


static void levelChangePWCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getLevelChangePasswordEntryUserInterface()->activate();
}


static void adminPWCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getAdminPasswordEntryUserInterface()->activate();
}


static void kickPlayerCallback(ClientGame *game, U32 unused)
{
   PlayerMenuUserInterface *ui = game->getUIManager()->getPlayerMenuUserInterface();

   ui->action = PlayerMenuUserInterface::Kick;
   ui->activate();
}


void GameMenuUserInterface::buildMenu()
{
   menuItems.clear();

   lastInputMode = gIniSettings.inputMode;      // Save here so we can see if we need to display alert msg if input mode changes

   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "OPTIONS",      optionsSelectedCallback, "", KEY_O)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "INSTRUCTIONS", helpSelectedCallback,    "", KEY_I, keyHELP)));
   GameType *theGameType = getGame()->getGameType();

   // Add any game-specific menu items
   if(theGameType)
   {
      mGameType = theGameType;
      theGameType->addClientGameMenuOptions(getGame(), menuItems);
   }

   GameConnection *gc = (getGame())->getConnectionToServer();
   if(gc)
   {
      if(gc->isLevelChanger())
      {
         menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "PLAY DIFFERENT LEVEL", chooseNewLevelCallback, "", KEY_L, KEY_P)));
         menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "ADD TIME (2 MINS)",    addTwoMinsCallback,     "", KEY_T, KEY_2)));
         menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "RESTART LEVEL",        restartGameCallback,    "", KEY_R)));
      }
      else
         menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "ENTER LEVEL CHANGE PASSWORD", levelChangePWCallback, "", KEY_L, KEY_P)));

      if(gc->isAdmin())
      {
         GameType *gameType = getGame()->getGameType();

         // Add any game-specific menu items
         if(gameType)
         {
            mGameType = gameType;
            gameType->addAdminGameMenuOptions(menuItems);
         }

         menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "KICK A PLAYER", kickPlayerCallback, "", KEY_K)));
      }
      else
         menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "ENTER ADMIN PASSWORD", adminPWCallback, "", KEY_A, KEY_E)));
   }

   if(getUIManager()->cameFrom(EditorUI))    // Came from editor
      menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "RETURN TO EDITOR", endGameCallback, "", KEY_Q, KEY_R)));
   else
      menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "QUIT GAME",        endGameCallback, "", KEY_Q)));
}


void GameMenuUserInterface::onEscape()
{
   getUIManager()->reactivatePrevUI();      //mGameUserInterface

   // Show alert about input mode changing, if needed
   bool inputModesChanged = lastInputMode == gIniSettings.inputMode;
   getUIManager()->getGameUserInterface()->resetInputModeChangeAlertDisplayTimer(inputModesChanged ? 0 : 2800);

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
static const U32 UPLOAD_LEVELS_MENUID = 1000;


static void selectLevelTypeCallback(ClientGame *game, U32 level)
{  
   LevelMenuSelectUserInterface *ui = game->getUIManager()->getLevelMenuSelectUserInterface();

   // First entry will be "All Levels", subsequent entries will be level types populated from mLevelInfos
   if(level == 0)
      ui->category = ALL_LEVELS;
   else if(level == UPLOAD_LEVELS_MENUID)
      ui->category = UPLOAD_LEVELS;

   else
   {
      // Replace the following with a getLevelCount() function on game??
      GameConnection *gc = game->getConnectionToServer();
      if(!gc || gc->mLevelInfos.size() < (S32(level) - 1))
         return;

      ui->category = gc->mLevelInfos[level - 1].levelType.getString();
   }

  ui->activate();
}


void LevelMenuUserInterface::onActivate()
{
   Parent::onActivate();
   mMenuTitle = "CHOOSE LEVEL TYPE:";

   // replace with getLevelCount() method on game?
   GameConnection *gc = getGame()->getConnectionToServer();
   if(!gc || !gc->mLevelInfos.size())
      return;

   menuItems.clear();

   char c[] = "A";   // Shortcut key
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, ALL_LEVELS, selectLevelTypeCallback, "", stringToKeyCode(c))));

   // Cycle through all levels, looking for unique type strings
   for(S32 i = 0; i < gc->mLevelInfos.size(); i++)
   {
      S32 j;
      for(j = 0; j < menuItems.size(); j++)
         if(gc->mLevelInfos[i].levelName == "" || !stricmp(gc->mLevelInfos[i].levelType.getString(), menuItems[j]->getPrompt().c_str()) )     
         {
            break;                  // Skip over levels with blank names or duplicate entries
         }
      if(j == menuItems.size())     // Must be a new type
      {
         strncpy(c, gc->mLevelInfos[i].levelType.getString(), 1);
         menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), i + 1, gc->mLevelInfos[i].levelType.getString(), 
                                                                      selectLevelTypeCallback, "", stringToKeyCode(c))));
      }
   }

   menuItems.sort(menuItemValueSort);

   if((gc->mSendableFlags & 1) && !gc->isLocalConnection())   // local connection is useless, already have all maps..
      menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), UPLOAD_LEVELS_MENUID, UPLOAD_LEVELS, 
                                                                   selectLevelTypeCallback, "", stringToKeyCode(c))));
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
extern ConfigDirectories gConfigDirs;

void LevelMenuSelectUserInterface::processSelection(U32 index)     
{
   Parent::onActivate();
   GameConnection *gc = getGame()->getConnectionToServer();

   if((index & UPLOAD_LEVELS_BIT) && (index & (~UPLOAD_LEVELS_BIT)) < U32(mLevels.size()))
   {
      string filename = strictjoindir(gConfigDirs.levelDir, mLevels[index & (~UPLOAD_LEVELS_BIT)]);

      if(!gc->s2rUploadFile(filename.c_str(),1))
         getGame()->displayErrorMessage("Can't upload level: unable to read file");
   }
   else
   {
      // The selection index is the level to load
      gc->c2sRequestLevelChange(index, false);
   }

   getUIManager()->reactivateMenu(getUIManager()->getGameUserInterface());    // Jump back to the game menu
}


void LevelMenuSelectUserInterface::onActivate()
{
   Parent::onActivate();
   mMenuTitle = "CHOOSE LEVEL: [" + category + "]";

   // Replace with a getLevelCount() method on Game?
   GameConnection *gc = getGame()->getConnectionToServer();
   if(!gc || !gc->mLevelInfos.size())
      return;

   menuItems.clear();

   mLevels.clear();

   char c[2];
   c[1] = 0; // null terminate

   if(!strcmp(category.c_str(), UPLOAD_LEVELS))
   {
      mLevels = LevelListLoader::buildLevelList();
      for(S32 i=0; i < mLevels.size(); i++)
      {
         c[0] = mLevels[i].c_str()[0];
         menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), i | UPLOAD_LEVELS_BIT, mLevels[i].c_str(), 
                                                                      processLevelSelectionCallback, "", stringToKeyCode(c))));
      }
   }
 
   for(S32 i = 0; i < gc->mLevelInfos.size(); i++)
   {
      if(gc->mLevelInfos[i].levelName == "")   // Skip levels with blank names --> but all should have names now!
         continue;
      if(!strcmp( gc->mLevelInfos[i].levelType.getString(), category.c_str() ) || 
         !strcmp(category.c_str(), ALL_LEVELS) )
      {
         c[0] = gc->mLevelInfos[i].levelName.getString()[0];
         menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), i, gc->mLevelInfos[i].levelName.getString(), 
                                                                      processLevelSelectionCallback, "", stringToKeyCode(c))));
      }
   }

   menuItems.sort(menuItemValueSort);
   currOffset = 0;

   if(itemSelectedWithMouse)
      onMouseMoved();
   else
      selectedIndex = 0;
}

#undef ALL_LEVELS


// Override parent, and make keys simply go to first level with that letter, rather than selecting it automatically
bool LevelMenuSelectUserInterface::processMenuSpecificKeys(KeyCode keyCode, char ascii)
{
   // First check for some shortcut keys
   for(S32 i = 0; i < menuItems.size(); i++)
   {
      // Lets us advance to next level with same starting letter  
      S32 indx = selectedIndex + i + 1;
      if(indx >= menuItems.size())
         indx -= menuItems.size();

      if(keyCode == menuItems[indx]->key1 || keyCode == menuItems[indx]->key2)
      {
         selectedIndex = indx;
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
   // Find selected player, and put that value into index; compensates for sorting
   for(S32 i = 0; i < menuItems.size(); i++)
      if(menuItems[i]->getIndex() == (S32)index)
      {
         index = i;
         break;
      }

   GameConnection *gc = getGame()->getConnectionToServer();

   if(action == ChangeTeam)
   {
      TeamMenuUserInterface *ui = getUIManager()->getTeamMenuUserInterface();

      ui->activate();     // Show menu to let player select a new team
      ui->nameToChange = menuItems[index]->getPrompt();
   }
   else if(gc)    // action == Kick
   {
      StringTableEntry e(menuItems[index]->getPrompt().c_str());
      gc->c2sAdminPlayerAction(e, action, -1);
   }

   if(action != ChangeTeam)                              // Unless we need to move on to the change team screen...
      getUIManager()->reactivateMenu(getUIManager()->getGameUserInterface());     // ...it's back to the game!
}


// By putting the menu building code in render, menus can be dynamically updated
void PlayerMenuUserInterface::render()
{
   menuItems.clear();
   GameType *gameType = getGame()->getGameType();
   if(!gameType)
      return;

   if(!getGame()->getConnectionToServer())
      return;

   char c[] = "A";      // Dummy shortcut key
   for(S32 i = 0; i < gameType->getClientCount(); i++)
   {
      RefPtr<ClientRef> client = gameType->getClient(i);

      strncpy(c, client->name.getString(), 1);        // Grab first char of name for a shortcut key

      // Will be used to show admin/player/robot prefix on menu
      PlayerType pt = client->isRobot ? PlayerTypeRobot : (client->isAdmin ? PlayerTypeAdmin : PlayerTypePlayer);    

      PlayerMenuItem *newItem = new PlayerMenuItem(getGame(), i, client->name.getString(), playerSelectedCallback, stringToKeyCode(c), pt);
      menuItems.push_back(boost::shared_ptr<MenuItem>(newItem));
      menuItems.last()->setUnselectedColor(gameType->getTeamColor(client->getTeam()));
   }

   menuItems.sort(menuItemValueSort);

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
}


static void processTeamSelectionCallback(ClientGame *game, U32 index)        
{
   game->getUIManager()->getTeamMenuUserInterface()->processSelection(index);
}


void TeamMenuUserInterface::processSelection(U32 index)        
{
   // Make sure user isn't just changing to the team they're already on...

   GameType *gt = getGame()->getGameType();
   GameConnection *gc = getGame()->getConnectionToServer();
   if(!gc || !gt)
      return;

   if(index != (U32)gt->getTeam(nameToChange.c_str()))
   {
      if(getPrevMenuID() == PlayerUI)     // Initiated by an admin (PlayerUI is the kick/change team player-pick admin menu)
      {
         StringTableEntry e(nameToChange.c_str());
         gc->c2sAdminPlayerAction(e, PlayerMenuUserInterface::ChangeTeam, index);   // Index will be the team index
      }
      else                                // Came from player changing own team
      {
         gt->c2sChangeTeams(index);
      }
   }

   getUIManager()->reactivateMenu(getUIManager()->getGameUserInterface());    // Back to the game!
}


// By reconstructing our menu at render time, changes to teams caused by others will be reflected immediately
void TeamMenuUserInterface::render()
{
   menuItems.clear();

   GameType *gameType = getGame()->getGameType();
   if(!gameType)
      return;

   gameType->countTeamPlayers();    // Make sure numPlayers is correctly populated

   char c[] = "A";      // Dummy shortcut key, will change below
   for(S32 i = 0; i < getGame()->getTeamCount(); i++)
   {
      AbstractTeam *team = getGame()->getTeam(i);
      strncpy(c, team->getName().getString(), 1);     // Grab first char of name for a shortcut key

      bool isCurrent = (i == gameType->getTeam(nameToChange.c_str()));
      
      menuItems.push_back(boost::shared_ptr<MenuItem>(new TeamMenuItem(getGame(), i, team, processTeamSelectionCallback, stringToKeyCode(c), isCurrent)));
   }

   string name = "";
   if(getGame()->getConnectionToServer() && getGame()->getConnectionToServer()->getControlObject())
   {
      Ship *ship = dynamic_cast<Ship *>(getGame()->getConnectionToServer()->getControlObject());
      if(ship)
         name = ship->getName().getString();
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

#undef MAX_MENU_SIZE

};

