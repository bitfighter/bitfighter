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
#include "gameObjectRender.h"    // For renderBitfighterLogo
#include "game.h"
#include "gameType.h"
#include "input.h"
#include "keyCode.h"
#include "IniFile.h"
#include "config.h"

#include "../glut/glutInclude.h"
#include <algorithm>
#include <string>

namespace Zap
{

const S32 gMaxMenuSize = 8;      // Max number of menu items we show on screen before we go into scrolling mode

MenuUserInterface::MenuUserInterface()    // Constructor
{
   setMenuID(GenericUI);
   dSprintf(menuTitle, sizeof(menuTitle), "Menu:");
   menuSubTitle = "";

   menuFooterContainsInstructions = true;
   selectedIndex = 0;
   itemSelectedWithMouse = false;
   currOffset = 0;
}

extern bool gDisableShipKeyboardInput;
extern Point gMousePos;

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
}


// Return index offset to account for scrolling menus
S32 MenuUserInterface::getOffset()
{
   S32 offset = 0;
   S32 count = menuItems.size();

   if(count > gMaxMenuSize)     // Do some sort of scrolling
   {
      offset = currOffset;

      // itemSelectedWithMouse basically lets users highlight the top and bottom items in a scrolling list,
      // which can't be done when using the keyboard
      if(selectedIndex - currOffset < (itemSelectedWithMouse ? 0 : 1))
         offset = selectedIndex - (itemSelectedWithMouse ? 0 : 1);
      else if( selectedIndex - currOffset > (gMaxMenuSize - (itemSelectedWithMouse ? 1 : 2)) )
         offset = selectedIndex - (gMaxMenuSize - (itemSelectedWithMouse ? 1 : 2));

      if(offset < 0)
         offset = 0;
      else if(offset + gMaxMenuSize >= menuItems.size())
         offset = menuItems.size() - gMaxMenuSize;
   }
   currOffset = offset;

   return offset;
}


// Get vert pos of first menu item
S32 MenuUserInterface::getYStart()
{
   S32 vertOff = (getMenuID() == MainUI) ? 40 : 0;    // Make room for the logo on the main menu
   return (canvasHeight - min(menuItems.size(), gMaxMenuSize) * UserInterface::MenuItemHeight) / 2 + vertOff;
}


extern IniSettings gIniSettings;
extern void renderControllerButton(F32 x, F32 y, KeyCode keyCode, bool activated, S32 offset);
extern S32 getControllerButtonRenderedSize(KeyCode keyCode);

void MenuUserInterface::renderMenuInstructions(S32 variant)
{
   S32 y = canvasHeight - vertMargin - 20;
   const S32 size = 18;

   if(gIniSettings.inputMode == Keyboard)
   {
	  const char *menuFooter = "UP, DOWN to choose | ENTER to select | ESC exits menu";
	  drawCenteredString(y, size, menuFooter);
   }
   else
   {
	  S32 totalWidth = getControllerButtonRenderedSize(BUTTON_DPAD_UP) + getControllerButtonRenderedSize(BUTTON_DPAD_DOWN) +
					   getControllerButtonRenderedSize(BUTTON_START) +   getControllerButtonRenderedSize(BUTTON_BACK) +
					   getStringWidth(size, "to choose |  to select |  exits menu");

	  S32 x = canvasWidth/2 - horizMargin - totalWidth/2;

	  renderControllerButton(x, y, BUTTON_DPAD_UP, false);
	  x += getControllerButtonRenderedSize(BUTTON_DPAD_UP) + getStringWidth(size, " ");

	  renderControllerButton(x, y, BUTTON_DPAD_DOWN, false);
	  x += getControllerButtonRenderedSize(BUTTON_DPAD_DOWN) + getStringWidth(size, " ");

	  static const char *msg1 = "to choose | ";
	  drawString(x, y, size, msg1);
	  x += getStringWidth(size, msg1);

	  renderControllerButton(x, y + 4, BUTTON_START, false);
	  x += getControllerButtonRenderedSize(BUTTON_START);

	  static const char *msg2 = "to select | ";
	  drawString(x, y, size, msg2);
	  x += getStringWidth(size, msg2);

	  renderControllerButton(x, y + 4, BUTTON_BACK, false);
	  x += getControllerButtonRenderedSize(BUTTON_BACK);

	  drawString(x, y, size, "exits menu");
   }
}


void MenuUserInterface::render()
{
   // Draw the game screen, then dim it out so you can still see it under our overlay
   if(gClientGame->getConnectionToServer())
   {
      gGameUserInterface.render();
      glColor4f(0, 0, 0, 0.6);

      glEnable(GL_BLEND);
         glBegin(GL_POLYGON);
            glVertex2f(0, 0);
            glVertex2f(canvasWidth, 0);
            glVertex2f(canvasWidth, canvasHeight);
            glVertex2f(0, canvasHeight);
         glEnd();
      glDisable(GL_BLEND);
   }

   glColor3f(1, 1, 1);     // white
   drawCenteredString(vertMargin, 30, menuTitle);

   glColor(menuSubTitleColor);
   drawCenteredString(vertMargin + 35, 18, menuSubTitle);

   glColor3f(1, 1, 1);     // white
   if(menuFooterContainsInstructions) 
      renderMenuInstructions(1);
   else
      drawCenteredString(canvasHeight - vertMargin - 20, 18, menuFooter);

   if(selectedIndex >= menuItems.size())
      selectedIndex = 0;

   S32 count = menuItems.size();

   if(count > gMaxMenuSize)     // Need some sort of scrolling
      count = gMaxMenuSize;

   S32 yStart = getYStart();
   S32 offset = getOffset();

   for(S32 i = 0; i < count; i++)
   {
      S32 y = yStart + i * UserInterface::MenuItemHeight;

      if(selectedIndex == i + offset)  // Highlight selected item
      {
         glColor3f(0, 0, 0.4);         // Fill
         glBegin(GL_POLYGON);
            glVertex2f(0, y - 2);
            glVertex2f(canvasWidth, y - 2);
            glVertex2f(canvasWidth, y + 25 + 5);
            glVertex2f(0, y + 25 + 5);
         glEnd();

         glColor3f(0, 0, 1);           // Outline
         glBegin(GL_LINES);
            glVertex2f(0, y - 2);
            glVertex2f(canvasWidth, y - 2);
            glVertex2f(canvasWidth, y + 25 + 5);
            glVertex2f(0, y + 25 + 5);
         glEnd();
      }
      glColor(menuItems[i+offset].color);

      string temp = menuItems[i+offset].mText;

      // Add a player type prefix if requested
      if(menuItems[i+offset].mType == PlayerTypePlayer)
         temp = "[Player] " + temp;
      else if(menuItems[i+offset].mType == PlayerTypeAdmin)
         temp = "[Admin] " + temp;
      else if(menuItems[i+offset].mType == PlayerTypeRobot)
         temp = "[Robot] " + temp;

      if(menuItems[i+offset].mPlayers != -1)    // Must be a team list
      {
         char str[24];
         dSprintf(str, 24, " [%d / %d]", menuItems[i+offset].mPlayers, menuItems[i+offset].mScore);
         if(menuItems[i+offset].mCurrTeam)      // Draw indicator on current team
            temp = "-> " + temp;
         temp = temp + str;
      }

      drawCenteredString(y, 25, temp.c_str());
   }

   // Render an indicator that there are scrollable items above and/or below
   if(menuItems.size() > gMaxMenuSize)
   {
      glColor3f(0, 0, 1);

      if(offset > 0)                                  // There are items above
         renderArrowAbove(yStart);

      if(offset < menuItems.size() - gMaxMenuSize)    // There are items below
         renderArrowBelow(yStart + UserInterface::MenuItemHeight * gMaxMenuSize);
   }

   renderExtras();  // Draw something unique on a menu.  Not currently used anywhere...
}

#define ARROW_WIDTH 100
#define ARROW_HEIGHT 20
#define ARROW_MARGIN 5

void MenuUserInterface::renderArrowAbove(S32 pos)
{
   // First create a black poly to blot out what's behind
   glColor3f(0, 0, 0);
   glBegin(GL_POLYGON);
      glVertex2f( (canvasWidth - ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7);
      glVertex2f( (canvasWidth + ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7);
      glVertex2f(canvasWidth / 2, pos - (ARROW_HEIGHT + ARROW_MARGIN ) - 7);
   glEnd();

   // Then render the arrow itself
   glColor3f(0, 0, 1);
   glBegin(GL_LINE_LOOP);
      glVertex2f( (canvasWidth - ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7);
      glVertex2f( (canvasWidth + ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7);
      glVertex2f(canvasWidth / 2, pos - (ARROW_HEIGHT + ARROW_MARGIN ) - 7);
   glEnd();
}

void MenuUserInterface::renderArrowBelow(S32 pos)
{
   glColor3f(0, 0, 1);
   glBegin(GL_LINE_LOOP);
      glVertex2f( (canvasWidth - ARROW_WIDTH) / 2, pos + ARROW_MARGIN - 7);
      glVertex2f( (canvasWidth + ARROW_WIDTH) / 2, pos + ARROW_MARGIN - 7);
      glVertex2f(canvasWidth / 2, pos + (ARROW_HEIGHT + ARROW_MARGIN) - 7);
   glEnd();
}

#undef ARROW_WIDTH
#undef ARROW_HEIGHT
#undef ARROW_MARGIN


// Handle mouse input, figure out which menu item we're over, and highlight it
void MenuUserInterface::onMouseMoved(S32 x, S32 y)
{
   itemSelectedWithMouse = true;
   glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);            // Show cursor when user moves mouse

   Point mousePos = gEditorUserInterface.convertWindowToCanvasCoord(gMousePos);
   selectedIndex = floor(( mousePos.y - getYStart() + 10 ) / UserInterface::MenuItemHeight) + currOffset;

   processMouse();
}


void MenuUserInterface::processMouse()
{
   if(menuItems.size() > gMaxMenuSize)    // We have a scrolling situation here...
   {
      S32 yStart = getYStart();

      if(selectedIndex < currOffset)      // Scroll up
      {
         if(!mScrollTimer.getCurrent() && currOffset > 0)
         {
            currOffset--;
            mScrollTimer.reset(100);
         }
         selectedIndex = currOffset;
      }
      else if(selectedIndex > currOffset + gMaxMenuSize - 1)   // Scroll down
      {
         if(!mScrollTimer.getCurrent() && selectedIndex > currOffset + gMaxMenuSize - 2)
         {
            currOffset++;
            mScrollTimer.reset(MouseScrollInterval);
         }
         selectedIndex = currOffset + gMaxMenuSize - 1;
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
      currOffset = max(menuItems.size() - gMaxMenuSize, 0);
   }
}

enum HostingModePhases { NotHosting, LoadingLevels, DoneLoadingLevels, Hosting };
extern HostingModePhases gHostingModePhase;


// All key handling now under one roof!
void MenuUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_UNKNOWN)
      return;

   // Check for in autorepeat mode
   mRepeatMode = mKeyDown;
   mKeyDown = true;

   // Handle special case of keystrokes during hosting preparation phases
   if(gHostingModePhase == LoadingLevels || gHostingModePhase == DoneLoadingLevels) 
   {
      if(keyCode == KEY_ESCAPE)
      {
         gHostingModePhase = NotHosting;
         gMainMenuUserInterface.clearLevelLoadDisplay();
         endGame();
      }
      return;
   }

   gMainMenuUserInterface.firstTime = false;    // Stop animations if a key is pressed
   processMenuSpecificKeys(keyCode);            // This will, in turn, call the more generic keyboard handler if needed
}


void MenuUserInterface::onKeyUp(KeyCode keyCode)
{
   mKeyDown = false;
   mRepeatMode = false;
}


// Generic handler looks for keystrokes and translates them into menu actions
void MenuUserInterface::processMenuSpecificKeys(KeyCode keyCode)
{
   // First check for some shortcut keys

   for(S32 i = 0; i < menuItems.size(); i++)
   {
      if(keyCode == menuItems[i].key1 || keyCode == menuItems[i].key2)
      {
         selectedIndex = i;
         UserInterface::playBoop();

         if(getKeyState(KEY_SHIFT))
            processShiftSelection(menuItems[i].mIndex);
         else
            processSelection(menuItems[i].mIndex);

         return;
      }
   }

   // Still here?  Try the standard keys
   processStandardKeys(keyCode);
}

// Process the keys that work on all menus
void MenuUserInterface::processStandardKeys(KeyCode keyCode)
{
   if(keyCode == KEY_SPACE || keyCode == KEY_RIGHT || keyCode == KEY_ENTER || /*keyCode == BUTTON_DPAD_RIGHT ||*/ keyCode == BUTTON_START || keyCode == MOUSE_LEFT)
   {
      UserInterface::playBoop();
      if(keyCode != MOUSE_LEFT)
         itemSelectedWithMouse = false;
      else // it was MOUSE_LEFT after all
      {
         // Make sure we're actually pointing at a menu item before we process it
         S32 yStart = getYStart();
         Point mousePos = gEditorUserInterface.convertWindowToCanvasCoord(gMousePos);

         if(mousePos.y < yStart || mousePos.y > yStart + (menuItems.size() + 1) * UserInterface::MenuItemHeight)
            return;
      }
      processSelection(menuItems[selectedIndex].mIndex);
   }
   else if(keyCode == KEY_LEFT || /*keyCode == BUTTON_DPAD_LEFT ||*/ keyCode == MOUSE_RIGHT)
   {
      UserInterface::playBoop();
      processShiftSelection(menuItems[selectedIndex].mIndex);
   }
   else if(keyCode == KEY_ESCAPE || keyCode == BUTTON_BACK)
   {
      UserInterface::playBoop();
      onEscape();
   }
   else if(keyCode == KEY_UP || keyCode == BUTTON_DPAD_UP)        // Prev item
   {
      selectedIndex--;
      itemSelectedWithMouse = false;

      if(selectedIndex < 0)                        // Scrolling off the top
      {
         if((menuItems.size() > gMaxMenuSize) && mRepeatMode)        // Allow wrapping on long menus only when not in repeat mode
         {
            selectedIndex = 0;               // No wrap --> (first item)
            return;                          // (leave before playBoop)
         }
         else                                      // Always wrap on shorter menus
            selectedIndex = menuItems.size() - 1;  // Wrap --> (select last item)
      }
      UserInterface::playBoop();
   }

   else if(keyCode == KEY_DOWN || keyCode == BUTTON_DPAD_DOWN)    // Next item
   {
      selectedIndex++;
      itemSelectedWithMouse = false;

      if(selectedIndex >= menuItems.size())     // Scrolling off the bottom
      {
         if((menuItems.size() > gMaxMenuSize) && mRepeatMode)     // Allow wrapping on long menus only when not in repeat mode
         {
            selectedIndex = menuItems.size() - 1;                 // No wrap --> (last item)
            return;                                               // (leave before playBoop)
         }
         else                     // Always wrap on shorter menus
            selectedIndex = 0;    // Wrap --> (first item)
      }
      UserInterface::playBoop();
   }
   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
   {
      gChatInterface.activate();
      UserInterface::playBoop();
   }

//   else if(keyCode == keyDIAG)            // Turn on diagnostic overlay
//   {
//      gDiagnosticInterface.activate();
//      UserInterface::playBoop();
//   }

   // Finally, since the user has indicated they want to use keyboard/controller input, hide the cursor
   if(keyCode != MOUSE_LEFT && keyCode != MOUSE_MIDDLE && keyCode != MOUSE_RIGHT && keyCode != KEY_ESCAPE)
      glutSetCursor(GLUT_CURSOR_NONE);
}


void MenuUserInterface::onEscape()
{
   // Do nothing
}

//////////
MainMenuUserInterface gMainMenuUserInterface;

// Constructor
MainMenuUserInterface::MainMenuUserInterface()
{
   firstTime = true;
   setMenuID(MainUI);
   dSprintf(menuTitle, sizeof(menuTitle), "");
   motd[0] = 0;
   menuSubTitle = "";
   //menuSubTitleColor = Color(1,1,1);
   menuFooter = "join us @ www.bitfighter.org";
   menuFooterContainsInstructions = false;
   mNeedToUpgrade = false;                         // Assume we're up-to-date until we hear from the master
   mShowedUpgradeAlert = false;                    // So we don't show the upgrade message more than once
   levelLoadDisplayFadeTimer.setPeriod(1000);
   gMainMenuUserInterface.levelLoadDisplayDisplay = true;

   //                                  Text              ID      Keys
   menuItems.push_back(MenuItem("JOIN LAN/INTERNET GAME", 0, KEY_J, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem("HOST GAME",              1, KEY_H, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem("INSTRUCTIONS",           2, KEY_I, keyHELP     ));
   menuItems.push_back(MenuItem("OPTIONS",                3, KEY_O, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem("LEVEL EDITOR",           4, KEY_L, KEY_E       ));
   menuItems.push_back(MenuItem("CREDITS",                5, KEY_C, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem("QUIT",                   6, KEY_Q, KEY_UNKNOWN ));
}

void MainMenuUserInterface::onActivate()
{
   // Time for a clean start.  No matter how we got here, there's no going back.
   // Needed mainly because the editor makes things confusing.  Now that that's been reworked,
   // it's probably not needed at all.
   prevUIs.clear();
   mFadeInTimer.reset(FadeInTime);
   mColorTimer.reset(ColorTime);
   mColorTimer2.reset(ColorTime2);
   mTransDir = true;
   mLevelLoadDisplayTotal = 0;
   clearLevelLoadDisplay();

   if(firstTime)
      gSplashUserInterface.activate();          // Show splash screen the first time throug
}

// Set the MOTD we recieved from the master
void MainMenuUserInterface::setMOTD(const char *motdString)
{
   strcpy(motd, motdString);
   if(gClientGame)
      motdArriveTime = gClientGame->getCurrentTime();    // Used for scrolling the message
}

// Set needToUpgrade flag that tells us the client is out-of-date
void MainMenuUserInterface::setNeedToUpgrade(bool needToUpgrade)
{
   mNeedToUpgrade = needToUpgrade;

   if(mNeedToUpgrade && !mShowedUpgradeAlert)
      showUpgradeAlert();
}


void MainMenuUserInterface::render()
{
   // If we're in LoadingLevels mode, show the progress panel...
   renderProgressListItems();
   if(gHostingModePhase == LoadingLevels || gHostingModePhase == DoneLoadingLevels)
   {
      // There will be exactly one new entry every time we get here!
      addProgressListItem("Loaded level " + gServerGame->getCurrentLevelLoadName() + "...");
      return;
   }

   // else...

  Parent::render();

   if(motd[0])
   {
      U32 width = getStringWidth(20, motd);
      glColor3f(1,1,1);
      U32 totalWidth = width + canvasWidth;
      U32 pixelsPerSec = 100;
      U32 delta = gClientGame->getCurrentTime() - motdArriveTime;
      delta = U32(delta * pixelsPerSec * 0.001) % totalWidth;

      drawString(canvasWidth - delta, 540, 20, motd);
   }

   // Fade in the menu here if we are showing it the first time...  this will tie in
   // nicely with the splash screen, and make the transition less jarring and sudden
   if(firstTime)
   {
      glEnable(GL_BLEND);
         glBegin(GL_POLYGON);
            glColor4f(0, 0, 0, (F32) mFadeInTimer.getCurrent() / (F32) FadeInTime);
            glVertex2f(0, 0);
            glVertex2f(canvasWidth, 0);
            glVertex2f(canvasWidth, canvasHeight);
            glVertex2f(0, canvasHeight);
         glEnd();
      glDisable(GL_BLEND);
   }

   // Render logo at top, never faded
   renderStaticBitfighterLogo();
}


// Add bit of text to progress item, and manage the list
void MainMenuUserInterface::addProgressListItem(string item)
{
   mLevelLoadDisplayNames.push_back(item);

   mLevelLoadDisplayTotal++;

   // Keep the list from growing too long:
   if(mLevelLoadDisplayNames.size() > 15)
      mLevelLoadDisplayNames.erase(0);
}


void MainMenuUserInterface::clearLevelLoadDisplay()
{
   mLevelLoadDisplayNames.clear();
   mLevelLoadDisplayTotal = 0;
}


void MainMenuUserInterface::renderProgressListItems()
{
   if(levelLoadDisplayDisplay || levelLoadDisplayFadeTimer.getCurrent() > 0)
   {
      glEnable(GL_BLEND);
      for(S32 i = 0; i < mLevelLoadDisplayNames.size(); i++)
      {
         glColor4f(1,1,1, (1.4 - ((F32) (mLevelLoadDisplayNames.size() - i) / 10.0)) * (levelLoadDisplayDisplay ? 1 : levelLoadDisplayFadeTimer.getFraction()) );
         drawStringf(100, canvasHeight - vertMargin - (mLevelLoadDisplayNames.size() - i) * 20, 15, "%s", mLevelLoadDisplayNames[i].c_str());
      }
      glDisable(GL_BLEND);
   }
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


void MainMenuUserInterface::showUpgradeAlert()
{
   gErrorMsgUserInterface.reset();
   gErrorMsgUserInterface.setTitle("OUTDATED VERSION");
   gErrorMsgUserInterface.setMessage(1, "You are running an older version of Bitfighter.");
   gErrorMsgUserInterface.setMessage(2, "You will only be able to play with players who still");
   gErrorMsgUserInterface.setMessage(3, "have the same outdated version.");
   gErrorMsgUserInterface.setMessage(4, "");
   gErrorMsgUserInterface.setMessage(5, "To get the latest, visit bitfighter.org");

   gErrorMsgUserInterface.activate();
   mShowedUpgradeAlert = true;            // Only show this alert once per session -- we don't need to beat them over the head with it!
}


extern void exitGame();

// Take action based on menu selection
void MainMenuUserInterface::processSelection(U32 index)
{
   switch(index)
   {
      case 0:
         gQueryServersUserInterface.activate();
         break;
      case 1:
         initHostGame(Address(IPProtocol, Address::Any, 28000), false);
         break;
      case 2:
         gInstructionsUserInterface.activate();
         break;
      case 3:
         gOptionsMenuUserInterface.activate();
         break;
      case 4:
         gEditorUserInterface.setLevelFileName("");      // Reset this so we get the level entry screen
         gEditorUserInterface.activate();
         break;
      case 5:
         gCreditsUserInterface.activate();
         break;
      case 6:
         exitGame();
         break;
   }
   firstTime = false;
}


void MainMenuUserInterface::onEscape()
{
   exitGame();    // Quit!
}

//////////
OptionsMenuUserInterface gOptionsMenuUserInterface;

// Constructor
OptionsMenuUserInterface::OptionsMenuUserInterface()
{
   setMenuID(OptionsUI);
   dSprintf(menuTitle, sizeof(menuTitle), "OPTIONS MENU:");
}

extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;

void OptionsMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
   gDisableShipKeyboardInput = true;       // Keep keystrokes from getting to game
}


// Convert string to lower case
string lcase(string strToConvert)
{
   for(U32 i = 0; i < strToConvert.length(); i++)
      strToConvert[i] = tolower(strToConvert[i]);
   return strToConvert;
}

// Convert string to upper case
string ucase(string strToConvert)
{
   for(U32 i = 0; i < strToConvert.length(); i++)
      strToConvert[i] = toupper(strToConvert[i]);
   return strToConvert;
}


extern CIniFile gINI;

// Note that this is called from main() after INI file is loaded
void OptionsMenuUserInterface::setJoystick(ControllerTypeType jsType)
{
   // Set joystick type if we found anything other than None or Unknown
   // Otherwise, it makes more sense to remember what the user had last specified

   if (jsType != NoController && jsType != UnknownController && jsType != GenericController)
      gIniSettings.joystickType = jsType;
   // else do nothing and leave the value we read from the INI file alone

   // Set primary input to joystick if any controllers were found, even a generic one
   if(jsType == NoController || jsType == UnknownController)
      gIniSettings.inputMode = Keyboard;
   else
      gIniSettings.inputMode = Joystick;
}

void OptionsMenuUserInterface::setupMenus()
{
   menuItems.clear();
   if(gIniSettings.controlsRelative)
      menuItems.push_back(MenuItem("CONTROLS: RELATIVE", 0, KEY_C, KEY_UNKNOWN));
   else
      menuItems.push_back(MenuItem("CONTROLS: ABSOLUTE", 0, KEY_C, KEY_UNKNOWN));

   if(gIniSettings.fullscreen)
      menuItems.push_back(MenuItem("GAME MODE: FULLSCREEN", 1, KEY_G, KEY_UNKNOWN));
   else
      menuItems.push_back(MenuItem("GAME MODE: WINDOWED", 1, KEY_G, KEY_UNKNOWN));

   if(gIniSettings.inputMode == Keyboard)
      menuItems.push_back(MenuItem("PRIMARY INPUT: KEYBOARD", 2, KEY_P, KEY_I));
   else
      menuItems.push_back(MenuItem("PRIMARY INPUT: JOYSTICK", 2, KEY_P, KEY_I));

   // Why won't this work??  MenuItem wants a const char, and this doesn't seem to provide it...  If this worked, we'd get rid
   // of the following block, and reduce duplication of code...
   // It won't work because by the time we need the string, it's gone out of scope, and man C++ really sucks with strings.
   // I should rewrite this crap in Perl ;-)

   //char str[255];
   //dSprintf(str, sizeof(str), "JOYSTICK: %s", ucase(joystickTypeToPrettyString(gIniSettings.joystickType)).c_str() );
   //menuItems.push_back(MenuItem(str, 3, KEY_J, KEY_UNKNOWN));

   switch(gIniSettings.joystickType)
   {
      case LogitechWingman:
         menuItems.push_back(MenuItem("JOYSTICK: LOGITECH WINGMAN DUAL-ANALOG", 3, KEY_J, KEY_UNKNOWN));
         break;
      case LogitechDualAction:
         menuItems.push_back(MenuItem("JOYSTICK: LOGITECH DUAL ACTION", 3, KEY_J, KEY_UNKNOWN));
         break;
      case SaitekDualAnalogP880:
         menuItems.push_back(MenuItem("JOYSTICK: SAITEK P-880 DUAL-ANALOG", 3, KEY_J, KEY_UNKNOWN));
         break;
      case SaitekDualAnalogRumblePad:
         menuItems.push_back(MenuItem("JOYSTICK: SAITEK P-480 DUAL-ANALOG", 3, KEY_J, KEY_UNKNOWN));
         break;
      case PS2DualShock:
         menuItems.push_back(MenuItem("JOYSTICK: PS2 DUALSHOCK USB", 3, KEY_J, KEY_UNKNOWN));
         break;
      case PS2DualShockConversionCable:
         menuItems.push_back(MenuItem("JOYSTICK: PS2 DUALSHOCK CONVERSION CABLE", 3, KEY_J, KEY_UNKNOWN));
         break;
      case XBoxController:
         menuItems.push_back(MenuItem("JOYSTICK: XBOX CONTROLLER USB", 3, KEY_J, KEY_UNKNOWN));
         break;
      case XBoxControllerOnXBox:
         menuItems.push_back(MenuItem("JOYSTICK: XBOX CONTROLLER", 3, KEY_J, KEY_UNKNOWN));
         break;
      default:
         menuItems.push_back(MenuItem("JOYSTICK: UNKNOWN", 3, KEY_J, KEY_UNKNOWN));
   }


   menuItems.push_back(MenuItem("DEFINE KEYS / BUTTONS", 4, KEY_D, KEY_K));

   // Yes, this is entirely lame, but I'm sick of trying to figure out
   // why a more logical construct isn't working, and hell, this works
   // and is fast and probably no one else is ever going to see it anyway.
   // Except you, of course ;-)
   // Alright, I know why the more logical construct won't work, but the way
   // around it is with a global variable or somesuch, which will be even kludgier
   // than this.
   switch ((U32) ((gIniSettings.sfxVolLevel + 0.05) * 10.0))
   {
      case 0:
         menuItems.push_back(MenuItem("SFX VOLUME: 0 [MUTE]", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 1:
         menuItems.push_back(MenuItem("SFX VOLUME: 1", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 2:
         menuItems.push_back(MenuItem("SFX VOLUME: 2", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 3:
         menuItems.push_back(MenuItem("SFX VOLUME: 3", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 4:
         menuItems.push_back(MenuItem("SFX VOLUME: 4", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 5:
         menuItems.push_back(MenuItem("SFX VOLUME: 5", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 6:
         menuItems.push_back(MenuItem("SFX VOLUME: 6", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 7:
         menuItems.push_back(MenuItem("SFX VOLUME: 7", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 8:
         menuItems.push_back(MenuItem("SFX VOLUME: 8", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 9:
         menuItems.push_back(MenuItem("SFX VOLUME: 9", 5, KEY_S, KEY_UNKNOWN));
         break;
      case 10:
         menuItems.push_back(MenuItem("SFX VOLUME: 10", 5, KEY_S, KEY_UNKNOWN));
         break;
   }

   // No music yet, so keep this out to keep menus from getting too long.  Uncomment when we have music.
   //switch ((U32) ((gIniSettings.musicVolLevel + 0.05) * 10.0))    // See comment above explaining lameness
   //{
   //   case 0:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 0 [MUTE]", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 1:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 1", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 2:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 2", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 3:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 3", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 4:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 4", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 5:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 5", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 6:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 6", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 7:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 7", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 8:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 8", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 9:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 9", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //   case 10:
   //      menuItems.push_back(MenuItem("MUSIC VOLUME: 10", 6, KEY_M, KEY_UNKNOWN));
   //      break;
   //}

   switch ((U32) ((gIniSettings.voiceChatVolLevel + 0.05) * 10.0))    // See comment above explaining lameness
   {
      case 0:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 0 [MUTE]", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 1:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 1", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 2:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 2", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 3:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 3", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 4:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 4", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 5:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 5", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 6:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 6", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 7:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 7", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 8:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 8", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 9:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 9", 7, KEY_V, KEY_UNKNOWN));
         break;
      case 10:
         menuItems.push_back(MenuItem("VOICE CHAT VOLUME: 10", 7, KEY_V, KEY_UNKNOWN));
         break;
   }


   if(gIniSettings.echoVoice)
      menuItems.push_back(MenuItem("VOICE ECHO: ENABLED", 8, KEY_E, KEY_UNKNOWN));
   else
      menuItems.push_back(MenuItem("VOICE ECHO: DISABLED", 8, KEY_E, KEY_UNKNOWN));
}


// Actually put us in windowed or full screen mode.  Pass true the first time this is used, false subsequently.
void OptionsMenuUserInterface::actualizeScreenMode(bool first)
{
   if(gIniSettings.fullscreen)      // Entering fullscreen mode
   {
      if(!first)
      {
         gIniSettings.winXPos = glutGet(GLUT_WINDOW_X);
         gIniSettings.winYPos = glutGet(GLUT_WINDOW_Y);

         gINI.SetValueI("Settings", "WindowXPos", gIniSettings.winXPos, true);
         gINI.SetValueI("Settings", "WindowYPos", gIniSettings.winYPos, true);
      }

      glutFullScreen();
   }
   else           // Leaving fullscreen, entering windowed mode
   {
      glutReshapeWindow((int) (gScreenWidth * gIniSettings.winSizeFact), (int) (gScreenHeight * gIniSettings.winSizeFact) );
      glutPositionWindow(gIniSettings.winXPos, gIniSettings.winYPos);
   }
}

void OptionsMenuUserInterface::toggleFullscreen()
{
   gIniSettings.fullscreen = !gIniSettings.fullscreen;
   actualizeScreenMode();
}

void OptionsMenuUserInterface::processSelection(U32 index)        // Handler for unshifted menu shortcut key
{
   switch(index)
   {
   case 0:
      gIniSettings.controlsRelative = !gIniSettings.controlsRelative;
      break;
   case 1:
      toggleFullscreen();
      break;
   case 2:     // Toggle input mode
      gIniSettings.inputMode = (gIniSettings.inputMode == Keyboard ? Joystick : Keyboard);
      break;
   case 3:
         gIniSettings.joystickType++;
         if(gIniSettings.joystickType >= ControllerTypeCount)
            gIniSettings.joystickType = 0;
      break;
   case 4:
      gKeyDefMenuUserInterface.activate();
      break;
   case 5:
      gIniSettings.sfxVolLevel += 0.1;
      if(gIniSettings.sfxVolLevel > 1.0)
         gIniSettings.sfxVolLevel = 1.0;
      break;
   case 6:
      gIniSettings.musicVolLevel += 0.1;
      if(gIniSettings.musicVolLevel > 1.0)
         gIniSettings.musicVolLevel = 1.0;
      break;
   case 7:
      gIniSettings.voiceChatVolLevel += 0.1;
      if(gIniSettings.voiceChatVolLevel > 1.0)
         gIniSettings.voiceChatVolLevel = 1.0;
      break;
   case 8:
      gIniSettings.echoVoice = !gIniSettings.echoVoice;
      break;
   };
   setupMenus();
}

// Just like the function above, but for when the user hits the left arrow.
void OptionsMenuUserInterface::processShiftSelection(U32 index)   // Handler for shifted menu shortcut key
{
   switch(index)
   {
      case 3:
         gIniSettings.joystickType--;
         if(gIniSettings.joystickType < 0 )
            gIniSettings.joystickType = ControllerTypeCount - 1;
      break;

      case 5:
         gIniSettings.sfxVolLevel -= 0.1;
         if (gIniSettings.sfxVolLevel < 0)
            gIniSettings.sfxVolLevel = 0;
      break;

      case 6:
         gIniSettings.musicVolLevel -= 0.1;
         if (gIniSettings.musicVolLevel < 0)
            gIniSettings.musicVolLevel = 0;
      break;

      case 7:
         gIniSettings.voiceChatVolLevel -= 0.1;
         if (gIniSettings.voiceChatVolLevel < 0)
            gIniSettings.voiceChatVolLevel = 0;
      break;

      default:
         processSelection(index);      // The other options are two-value toggles, so cycling
                                       // left is the same as cycling right.
         return;
   }
   setupMenus();
}

// Save options to INI file, and return to our regularly scheduled program
void OptionsMenuUserInterface::onEscape()
{
   saveSettingsToINI();
   UserInterface::reactivatePrevUI();      //gGameUserInterface
}

//////////
GameMenuUserInterface gGameMenuUserInterface;

// Constructor
GameMenuUserInterface::GameMenuUserInterface()
{
   setMenuID(GameMenuUI);
   dSprintf(menuTitle, sizeof(menuTitle), "GAME MENU:");
}

void GameMenuUserInterface::idle(U32 timeDelta)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(gc && gc->waitingForPermissionsReply() && gc->gotPermissionsReply())      // We're waiting for a reply, and it has arrived
   {
      gc->setWaitingForPermissionsReply(false);
      buildMenu();                              // Update menu to reflect newly available options
   }
}


void GameMenuUserInterface::onActivate()
{
   Parent::onActivate();
   buildMenu();
   menuSubTitle = "";
   menuSubTitleColor = Color(0,1,1);
}

void GameMenuUserInterface::onReactivate()
{
   menuSubTitle = "";
}


void GameMenuUserInterface::buildMenu()
{
   menuItems.clear();
   mAdminActiveOption = 0;

   lastInputMode = gIniSettings.inputMode;      // Save here so we can see if we need to display alert msg if input mode changes

   menuItems.push_back(MenuItem("OPTIONS",      1, KEY_O, KEY_UNKNOWN));
   menuItems.push_back(MenuItem("INSTRUCTIONS", 2, KEY_I, keyHELP));
   GameType *theGameType = gClientGame->getGameType();

   // Add any game-specific menu items
   if(theGameType)
   {
      mGameType = theGameType;
      theGameType->addClientGameMenuOptions(menuItems);
   }

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(gc)
   {
      if(gc->isLevelChanger())
      {
         mAdminActiveOption = 7;
         menuItems.push_back(MenuItem("PLAY DIFFERENT LEVEL", mAdminActiveOption, KEY_L, KEY_P));
      }
      else
      {
         mAdminActiveOption = 8;
         menuItems.push_back(MenuItem("ENTER LEVEL CHANGE PASSWORD", mAdminActiveOption, KEY_L, KEY_P));
      }

      if(gc->isAdmin())
      {
         mAdminActiveOption = 4;
         GameType *theGameType = gClientGame->getGameType();

         // Add any game-specific menu items
         if(theGameType)
         {
            mGameType = theGameType;
            theGameType->addAdminGameMenuOptions(menuItems);
         }

         menuItems.push_back(MenuItem("KICK A PLAYER", 4, KEY_K, KEY_UNKNOWN));
         menuItems.push_back(MenuItem("ADD TIME (2 MINS)", 5, KEY_T, KEY_2));
      }
      else
      {
         mAdminActiveOption = 6;
         menuItems.push_back(MenuItem("ENTER ADMIN PASSWORD", mAdminActiveOption, KEY_A, KEY_E));
      }
   }

   if(cameFromEditor())    // Came from editor
      menuItems.push_back(MenuItem("RETURN TO EDITOR", 3, KEY_Q, (cameFromEditor() ? KEY_R : KEY_UNKNOWN) ));
   else
      menuItems.push_back(MenuItem("QUIT GAME", 3, KEY_Q, KEY_UNKNOWN));
}


void GameMenuUserInterface::processSelection(U32 index)        // Handler for unshifted menu shortcut key
{
   switch(index)
   {
      case 1:
         gOptionsMenuUserInterface.activate();
         break;
      case 2:
         gInstructionsUserInterface.activate();
         break;
      case 3:
         endGame();            // No matter where we came from, end the game
         break;
      case 4:
         gPlayerMenuUserInterface.action = PlayerMenuUserInterface::Kick;
         gPlayerMenuUserInterface.activate();
         break;
      case 5:     // Add 2 mins to game
         if(gClientGame->getGameType())
            gClientGame->getGameType()->addTime(2 * 60 * 1000);
            reactivatePrevUI();     // And back to our regularly scheduled programming!
         break;
      case 6:
         gAdminPasswordEntryUserInterface.activate();
         break;
      case 7:
         gLevelMenuUserInterface.activate();
         break;
      case 8:
         gLevelChangePasswordEntryUserInterface.activate();
         break;
      default:    // A game-specific menu option must have been selected
         if(mGameType.isValid())
            mGameType->processClientGameMenuOption(index);        // Process the selected option
         break;
   }
}

void GameMenuUserInterface::processShiftSelection(U32 index)      // Handler for shifted menu shortcut key
{
   processSelection(index);
}


extern GameUserInterface gGameUserInterface;

void GameMenuUserInterface::onEscape()
{
   reactivatePrevUI();      //gGameUserInterface

   // Show alert about input mode changing, if needed
   if(gClientGame->getGameType())
      gClientGame->getGameType()->mInputModeChangeAlertDisplayTimer.reset((lastInputMode == gIniSettings.inputMode) ? 0 : 2800);

}

// Sorts alphanumerically by menuItem.value
S32 QSORT_CALLBACK menuItemValueSort(MenuItem *a, MenuItem *b)
{
   return stricmp((a)->mText, (b)->mText);
}

//////////
LevelMenuUserInterface gLevelMenuUserInterface;

// Constructor
LevelMenuUserInterface::LevelMenuUserInterface()
{
   setMenuID(LevelTypeUI);
}

#define ALL_LEVELS "All Levels"

void LevelMenuUserInterface::onActivate()
{
   Parent::onActivate();
   dSprintf(menuTitle, sizeof(menuTitle), "CHOOSE LEVEL TYPE:");

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(!gc || !gc->mLevelTypes.size())
      return;

   menuItems.clear();

   char c[] = "A";   // Shortcut key
   menuItems.push_back( MenuItem(ALL_LEVELS, 9999, stringToKeyCode(c), KEY_UNKNOWN) );

   // Cycle through all levels, looking for unique type strings
   for(S32 i = 0; i < gc->mLevelTypes.size(); i++)
   {
      S32 j;
      for(j = 0; j < menuItems.size(); j++)
         if( !strcmp(gc->mLevelNames[i].getString(), "") || !stricmp(gc->mLevelTypes[i].getString(), menuItems[j].mText) )      // Skip over levels with blank names
            break;
      if(j == menuItems.size())     // Must be a new type
      {
         strncpy(c, gc->mLevelTypes[i].getString(), 1);
         menuItems.push_back(MenuItem(gc->mLevelTypes[i].getString(), i, stringToKeyCode(c), KEY_UNKNOWN));
      }
   }

   menuItems.sort(menuItemValueSort);
}

void LevelMenuUserInterface::processSelection(U32 index)        // Handler for unshifted menu shortcut key
{
   //Parent::onActivate();
   GameConnection *gc = gClientGame->getConnectionToServer();

   // Index 9999 is the all levels index
   if(index == 9999)
      gLevelMenuSelectUserInterface.category  = ALL_LEVELS;
   else
      gLevelMenuSelectUserInterface.category = gc->mLevelTypes[index].getString();

   gLevelMenuSelectUserInterface.activate();
}

void LevelMenuUserInterface::processShiftSelection(U32 index)   // Handler for shifted menu shortcut key
{
   processSelection(index);
}

void LevelMenuUserInterface::onEscape()
{
   reactivatePrevUI();    // to gGameUserInterface
}

//////////
LevelMenuSelectUserInterface gLevelMenuSelectUserInterface;

// Constructor
LevelMenuSelectUserInterface::LevelMenuSelectUserInterface()
{
   setMenuID(LevelUI);
}

void LevelMenuSelectUserInterface::onActivate()
{
   Parent::onActivate();
   dSprintf(menuTitle, sizeof(menuTitle), "CHOOSE LEVEL: [%s]", category.c_str());

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(!gc || !gc->mLevelTypes.size())
      return;

   menuItems.clear();

   char c[] = "A";
   for(S32 i = 0; i < gc->mLevelTypes.size(); i++)
      if(strcmp(gc->mLevelNames[i].getString(), ""))                       // Skip levels with blank names --> but all should have names now!
         if(!strcmp( gc->mLevelTypes[i].getString(), category.c_str() ) || !strcmp(category.c_str(), ALL_LEVELS) )
         {
            strncpy(c, gc->mLevelNames[i].getString(), 1);
            menuItems.push_back(MenuItem(gc->mLevelNames[i].getString(), i, stringToKeyCode(c), KEY_UNKNOWN));
         }

   menuItems.sort(menuItemValueSort);
   currOffset = 0;

   if(itemSelectedWithMouse)
      onMouseMoved(gMousePos.x, gMousePos.y);
   else
      selectedIndex = 0;
}

#undef ALL_LEVELS


// Override parent, and make keys simply go to first level with that letter, rather than selecting it automatically
void LevelMenuSelectUserInterface::processMenuSpecificKeys(KeyCode keyCode)
{
   // First check for some shortcut keys
   for(S32 i = 0; i < menuItems.size(); i++)
   {
      // Lets us advance to next level with same starting letter  
      S32 indx = selectedIndex + i + 1;
      if(indx >= menuItems.size())
         indx -= menuItems.size();

      if(keyCode == menuItems[indx].key1 || keyCode == menuItems[indx].key2)
      {
         selectedIndex = indx;
         UserInterface::playBoop();

         return;
      }
   }

   // Still here?  Try the parent
   Parent::processMenuSpecificKeys(keyCode);
}


void LevelMenuSelectUserInterface::processSelection(U32 index)             // Handler for unshifted menu shortcut key
{
   Parent::onActivate();
   GameConnection *gc = gClientGame->getConnectionToServer();

   // The selection index is the level to load.
   gc->c2sRequestLevelChange(index, false);
   reactivateMenu(gGameUserInterface);    // Jump back to the game menu
}

void LevelMenuSelectUserInterface::processShiftSelection(U32 index)        // Handler for shifted menu shortcut key
{
   processSelection(index);
}

void LevelMenuSelectUserInterface::onEscape()
{
   reactivatePrevUI();    // to gLevelMenuUserInterface
}

//////////

PlayerMenuUserInterface gPlayerMenuUserInterface;

// Constructor
PlayerMenuUserInterface::PlayerMenuUserInterface()
{
   setMenuID(PlayerUI);
}

// By putting all this code in render, it allows menus to be dynamically updated.
void PlayerMenuUserInterface::render()
{
   menuItems.clear();
   GameType *gt = gClientGame->getGameType();
   if (!gt)
      return;

   GameConnection *gc = gClientGame->getConnectionToServer();
   if (!gc)
      return;

   char c[] = "A";      // Dummy shortcut key
   for(S32 i = 0; i < gt->mClientList.size(); i++)
   {
      strncpy(c, gt->mClientList[i]->name.getString(), 1);        // Grab first char of name for a shortcut key

      PlayerType pt = gt->mClientList[i]->isAdmin ? PlayerTypeAdmin : PlayerTypePlayer;    // Will be used to show admin/player/robot prefix on menu

      Color col = gt->getTeamColor(gt->mClientList[i]->teamId);
      menuItems.push_back(MenuItem(gt->mClientList[i]->name.getString(), i, stringToKeyCode(c), KEY_UNKNOWN, col, pt));
   }

   menuItems.sort(menuItemValueSort);

   if(action == Kick)
      dSprintf(menuTitle, sizeof(menuTitle), "CHOOSE PLAYER TO KICK:");
   else if(action == ChangeTeam)
      dSprintf(menuTitle, sizeof(menuTitle), "CHOOSE WHOSE TEAM TO CHANGE:");
   Parent::render();
}

void PlayerMenuUserInterface::onEscape()
{
   reactivatePrevUI();   //gGameUserInterface
}

void PlayerMenuUserInterface::processSelection(U32 index)        // Handler for unshifted menu shortcut key
{
   // Find selected player, and put that value into index
   for(S32 i = 0; i < menuItems.size(); i++)
      if(menuItems[i].mIndex == index)
      {
         index = i;
         break;
      }

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(action == ChangeTeam)
   {
      gTeamMenuUserInterface.activate();     // Show menu to let player select a new team
      gTeamMenuUserInterface.nameToChange = menuItems[index].mText;
   }
   else if(gc)    // action == Kick
   {
      StringTableEntry e(menuItems[index].mText);
      gc->c2sAdminPlayerAction(e, action, -1);
   }

   if(action != ChangeTeam)                     // Unless we need to move on to the change team screen...
      reactivateMenu(gGameUserInterface);       // ...it's back to the game!
}

void PlayerMenuUserInterface::processShiftSelection(U32 index)   // Handler for shifted menu shortcut key
{
   processSelection(index);
}


//////////

TeamMenuUserInterface gTeamMenuUserInterface;

// Constructor
TeamMenuUserInterface::TeamMenuUserInterface()
{
   setMenuID(TeamUI);
}

// By putting all this code in render, it allows menus to be dynamically updated.
void TeamMenuUserInterface::render()
{
   menuItems.clear();
   GameType *gt = gClientGame->getGameType();
   if (!gt)
      return;

   gt->countTeamPlayers();    // Make sure numPlayers is correctly populated

   char c[] = "A";      // Dummy shortcut key, will change below
   for(S32 i = 0; i < gt->mTeams.size(); i++)
   {
      strncpy(c, gt->mTeams[i].name.getString(), 1);     // Grab first char of name for a shortcut key

      Color col = gt->mTeams[i].color;
      S32 players = gt->mTeams[i].numPlayers;
      S32 score = gt->mTeams[i].score;
      bool isCurrent = (i == gt->getTeam(nameToChange));
      menuItems.push_back(MenuItem(gt->mTeams[i].name.getString(), i, stringToKeyCode(c), KEY_UNKNOWN, col, isCurrent, players, score));
   }

   string name = "";
   if(gClientGame->getConnectionToServer() && gClientGame->getConnectionToServer()->getControlObject())
   {
      Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
      if(ship)
         name = ship->getName().getString();
   }

   if(strcmp(name.c_str(), nameToChange))    // i.e. names differ, this isn't the local player
   {
      name = nameToChange;
      name += " ";
   }
   else
      name = "";

   // Finally, set menu title
   dSprintf(menuTitle, sizeof(menuTitle), "TEAM TO SWITCH %sTO:", name.c_str());       // No space btwn %s and TO!

   Parent::render();
}

void TeamMenuUserInterface::onEscape()
{
   reactivatePrevUI();
}

void TeamMenuUserInterface::processSelection(U32 index)        // Handler for unshifted menu shortcut key
{
   // Make sure user isn't just changing to the team they're already on...

   GameType *gt = gClientGame->getGameType();
   GameConnection *gc = gClientGame->getConnectionToServer();
   if(!gc || !gt)
      return;

   if (index != gt->getTeam(nameToChange))
   {
      if(getPrevMenuID() == PlayerUI)     // Initiated by an admin (PlayerUI is the kick/change team player-pick admin menu)
      {
         StringTableEntry e(nameToChange);
         gc->c2sAdminPlayerAction(e, PlayerMenuUserInterface::ChangeTeam, index);   // Index will be the team index
      }
      else                                // Came from player changing own team
      {
         gt->c2sChangeTeams(index);
      }
   }

   reactivateMenu(gGameUserInterface);    // Back to the game!
}

void TeamMenuUserInterface::processShiftSelection(U32 index)   // Handler for shifted menu shortcut key
{
   processSelection(index);
}


};
