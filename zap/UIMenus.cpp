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

// Sorts alphanumerically by menuItem.value
S32 QSORT_CALLBACK menuItemValueSort(MenuItem **a, MenuItem **b)
{
   return stricmp((*a)->getText(), (*b)->getText());
}


static const S32 FONT_SIZE = 25;


////////////////////////////////////
////////////////////////////////////

void MenuItem::render(S32 ypos)
{
   glColor(color);
   UserInterface::drawCenteredStringf(ypos, FONT_SIZE, "%s >", getText());
}

////////////////////////////////////
////////////////////////////////////

void ToggleMenuItem::render(S32 ypos)
{
   UserInterface::drawCenteredStringPair(ypos, FONT_SIZE, color, Color(0,1,1), getText(), mValue.c_str());
}

////////////////////////////////////
////////////////////////////////////

void PlayerMenuItem::render(S32 ypos)
{
   string temp = getText();

   // Add a player type prefix if requested
   if(mType == PlayerTypePlayer)
      temp = "[Player] " + temp;
   else if(mType == PlayerTypeAdmin)
      temp = "[Admin] " + temp;
   else if(mType == PlayerTypeRobot)
      temp = "[Robot] " + temp;

   glColor(color);
   UserInterface::drawCenteredString(ypos, 25, temp.c_str());
}


////////////////////////////////////
////////////////////////////////////

#include "teamInfo.h"      // For Team def

// Constructor
TeamMenuItem::TeamMenuItem(U32 index, Team team, KeyCode keyCode, bool isCurrent) :
                        MenuItem(index, team.getName().getString(), keyCode, KEY_UNKNOWN, team.color)
{
   mTeam = team;
   mIsCurrent = isCurrent;
}


void TeamMenuItem::render(S32 ypos)
{
   string temp = getText();

   char str[63];
   dSprintf(str, sizeof(str), " [%d / %d]", mTeam.numPlayers, mTeam.getScore());

   if(mIsCurrent)      // Draw indicator on current team
      temp = "-> " + temp;

   temp = temp + str;

   glColor(color);
   UserInterface::drawCenteredString(ypos, FONT_SIZE, temp.c_str());
}


////////////////////////////////////
////////////////////////////////////

void EditableMenuItem::render(S32 ypos)
{
   S32 xpos = UserInterface::drawCenteredStringPair(ypos, FONT_SIZE, color, Color(0,1,1), getText(), 
                                                    mLineEditor.getString() != "" ? mLineEditor.c_str() : mEmptyVal.c_str());
   if(mIsActive)
      mLineEditor.drawCursor(xpos, ypos, FONT_SIZE);
}


////////////////////////////////////
////////////////////////////////////

static const S32 MAX_MENU_SIZE = 8;      // Max number of menu items we show on screen before we go into scrolling mode

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
   return (canvasHeight - min(menuItems.size(), MAX_MENU_SIZE) * UserInterface::MenuItemHeight) / 2 + vertOff;
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

     glColor3f(1,1,1);
	  static const char *msg1 = "to choose | ";
	  drawString(x, y, size, msg1);
	  x += getStringWidth(size, msg1);

	  renderControllerButton(x, y + 4, BUTTON_START, false);
	  x += getControllerButtonRenderedSize(BUTTON_START);

     glColor3f(1,1,1);
	  static const char *msg2 = "to select | ";
	  drawString(x, y, size, msg2);
	  x += getStringWidth(size, msg2);

	  renderControllerButton(x + 4, y + 4, BUTTON_BACK, false);
	  x += getControllerButtonRenderedSize(BUTTON_BACK) + 4;

     glColor3f(1,1,1);
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

   if(count > MAX_MENU_SIZE)     // Need some sort of scrolling
      count = MAX_MENU_SIZE;

   S32 yStart = getYStart();
   S32 offset = getOffset();

   for(S32 i = 0; i < count; i++)
   {
      S32 y = yStart + i * UserInterface::MenuItemHeight;

      if(selectedIndex == i + offset)  // Highlight selected item
         for(S32 j = 1; j >= 0; j--)
         {
            glColor(j ? Color(0,0,0.4) : Color(0,0,1));         // Fill, then outline
            glBegin(j ? GL_POLYGON : GL_LINES);
               glVertex2f(0, y - 2);
               glVertex2f(canvasWidth, y - 2);
               glVertex2f(canvasWidth, y + 25 + 5);
               glVertex2f(0, y + 25 + 5);
            glEnd();
         }

      menuItems[i+offset]->render(y);
   }

   // Render an indicator that there are scrollable items above and/or below
   if(menuItems.size() > MAX_MENU_SIZE)
   {
      glColor3f(0, 0, 1);

      if(offset > 0)                                  // There are items above
         renderArrowAbove(yStart);

      if(offset < menuItems.size() - MAX_MENU_SIZE)    // There are items below
         renderArrowBelow(yStart + UserInterface::MenuItemHeight * MAX_MENU_SIZE);
   }

   renderExtras();  // Draw something unique on a menu.  Not currently used anywhere...
}

static const S32 ARROW_WIDTH = 100;
static const S32 ARROW_HEIGHT = 20;
static const S32 ARROW_MARGIN = 5;

void MenuUserInterface::renderArrowAbove(S32 pos)
{
   renderArrowAbove(pos, ARROW_HEIGHT);
}

void MenuUserInterface::renderArrowBelow(S32 pos)
{
   renderArrowBelow(pos, ARROW_HEIGHT);
}


void MenuUserInterface::renderArrowAbove(S32 pos, S32 height)
{
   for(S32 i = 1; i >= 0; i--)
   {
      // First create a black poly to blot out what's behind, then the arrow itself
      glColor(i ? Color(0, 0, 0) : Color(0, 0, 1));
      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f( (canvasWidth - ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7);
         glVertex2f( (canvasWidth + ARROW_WIDTH) / 2, pos - ARROW_MARGIN - 7);
         glVertex2f(canvasWidth / 2, pos - (height + ARROW_MARGIN ) - 7);
      glEnd();
   }
}

void MenuUserInterface::renderArrowBelow(S32 pos, S32 height)
{
   for(S32 i = 1; i >= 0; i--)
   {
      // First create a black poly to blot out what's behind, then the arrow itself
      glColor(i ? Color(0, 0, 0) : Color(0, 0, 1));
      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f( (canvasWidth - ARROW_WIDTH) / 2, pos + ARROW_MARGIN - 7);
         glVertex2f( (canvasWidth + ARROW_WIDTH) / 2, pos + ARROW_MARGIN - 7);
         glVertex2f(canvasWidth / 2, pos + (height + ARROW_MARGIN) - 7);
      glEnd();
   }
}


// Handle mouse input, figure out which menu item we're over, and highlight it
void MenuUserInterface::onMouseMoved(S32 x, S32 y)
{
   itemSelectedWithMouse = true;
   glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);            // Show cursor when user moves mouse

   Point mousePos = gEditorUserInterface.convertWindowToCanvasCoord(gMousePos);
   selectedIndex = (U32)(floor(( mousePos.y - getYStart() + 10 ) / UserInterface::MenuItemHeight)) + currOffset;

   processMouse();
}


void MenuUserInterface::processMouse()
{
   if(menuItems.size() > MAX_MENU_SIZE)    // We have a scrolling situation here...
   {
      //S32 yStart = getYStart();

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
         gHostMenuUserInterface.clearLevelLoadDisplay();
         endGame();
      }
      return;
   }

   gMainMenuUserInterface.firstTime = false;    // Stop animations if a key is pressed

   preprocessKeys(keyCode, ascii) || processMenuSpecificKeys(keyCode, ascii) || processKeys(keyCode, ascii);

   // Finally, since the user has indicated they want to use keyboard/controller input, hide the cursor
   if(keyCode != MOUSE_LEFT && keyCode != MOUSE_MIDDLE && keyCode != MOUSE_RIGHT && keyCode != KEY_ESCAPE)
      glutSetCursor(GLUT_CURSOR_NONE);
}


void MenuUserInterface::onKeyUp(KeyCode keyCode)
{
   mKeyDown = false;
   mRepeatMode = false;
}


bool MenuUserInterface::preprocessKeys(KeyCode keyCode, char ascii)
{
   return false;
}


// Generic handler looks for keystrokes and translates them into menu actions
bool MenuUserInterface::processMenuSpecificKeys(KeyCode keyCode, char ascii)
{
    if(!preprocessKeys(keyCode, ascii))
    {
      // First check for some shortcut keys

      for(S32 i = 0; i < menuItems.size(); i++)
      {
         if(keyCode == menuItems[i]->key1 || keyCode == menuItems[i]->key2)
         {
            selectedIndex = i;
            UserInterface::playBoop();

            if(getKeyState(KEY_SHIFT))
               processShiftSelection(menuItems[i]->getIndex());
            else
               processSelection(menuItems[i]->getIndex());

            return true;
         }
      }
   }

   return false;
}


// Process the keys that work on all menus
bool MenuUserInterface::processKeys(KeyCode keyCode, char ascii)
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
            return true;
      }
      processSelection(menuItems[selectedIndex]->getIndex());
   }
   else if(keyCode == KEY_LEFT || /*keyCode == BUTTON_DPAD_LEFT ||*/ keyCode == MOUSE_RIGHT)
   {
      UserInterface::playBoop();
      processShiftSelection(menuItems[selectedIndex]->getIndex());
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
         if((menuItems.size() > MAX_MENU_SIZE) && mRepeatMode)        // Allow wrapping on long menus only when not in repeat mode
         {
            selectedIndex = 0;               // No wrap --> (first item)
            return true;                     // (leave before playBoop)
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
         if((menuItems.size() > MAX_MENU_SIZE) && mRepeatMode)     // Allow wrapping on long menus only when not in repeat mode
         {
            selectedIndex = menuItems.size() - 1;                 // No wrap --> (last item)
            return true;                                          // (leave before playBoop)
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

   return true;      // Probably wrong, but doesn't really matter at this point
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

   menuItems.push_back(new MenuItem(0, "JOIN LAN/INTERNET GAME", KEY_J, KEY_UNKNOWN ));
   menuItems.push_back(new MenuItem(1, "HOST GAME",              KEY_H, KEY_UNKNOWN ));
   menuItems.push_back(new MenuItem(2, "INSTRUCTIONS",           KEY_I, keyHELP     ));
   menuItems.push_back(new MenuItem(3, "OPTIONS",                KEY_O, KEY_UNKNOWN ));
   menuItems.push_back(new MenuItem(4, "LEVEL EDITOR",           KEY_L, KEY_E       ));
   menuItems.push_back(new MenuItem(5, "CREDITS",                KEY_C, KEY_UNKNOWN ));
   menuItems.push_back(new MenuItem(6, "QUIT",                   KEY_Q, KEY_UNKNOWN ));
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

   if(firstTime)
      gSplashUserInterface.activate();          // Show splash screen the first time throug
}


// Set the MOTD we recieved from the master
void MainMenuUserInterface::setMOTD(const char *motdString)
{
   strcpy(motd, motdString);     // ???
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
      case 0:     // Join
         gQueryServersUserInterface.activate();
         break;
      case 1:     // Host
         gHostMenuUserInterface.activate();
         break;
      case 2:     // Help
         gInstructionsUserInterface.activate();
         break;      
      case 3:     // Options
         gOptionsMenuUserInterface.activate();
         break;
      case 4:     // Edit
         gEditorUserInterface.setLevelFileName("");      // Reset this so we get the level entry screen
         gEditorUserInterface.activate();
         break;
      case 5:     // Credits
         gCreditsUserInterface.activate();
         break;
      case 6:     // Quit
         exitGame();
         break;
   }
   firstTime = false;
}


void MainMenuUserInterface::onEscape()
{
   exitGame();    // Quit!
}


////////////////////////////////////////
////////////////////////////////////////

extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;
extern CIniFile gINI;

OptionsMenuUserInterface gOptionsMenuUserInterface;

// Constructor
OptionsMenuUserInterface::OptionsMenuUserInterface()
{
   setMenuID(OptionsUI);
   dSprintf(menuTitle, sizeof(menuTitle), "OPTIONS MENU:");
}


void OptionsMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
}


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


static string getVolMsg(F32 volume)
{
   S32 vol = U32((volume + 0.05) * 10.0);

   string msg = UserInterface::itos(vol);

   if(vol == 0)
      msg += " [MUTE]";

   return msg;
}


void OptionsMenuUserInterface::setupMenus()
{
   menuItems.deleteAndClear();

   menuItems.push_back(new ToggleMenuItem(0, "CONTROLS:", gIniSettings.controlsRelative ? "RELATIVE" : "ABSOLUTE", KEY_C, KEY_UNKNOWN));
   menuItems.push_back(new ToggleMenuItem(1, "GAME MODE:", gIniSettings.fullscreen ? "FULLSCREEN" : "WINDOWED", KEY_G, KEY_UNKNOWN));
   menuItems.push_back(new ToggleMenuItem(2, "PRIMARY INPUT:", gIniSettings.inputMode == Keyboard ? "KEYBOARD" : "JOYSTICK", KEY_P, KEY_I));
   menuItems.push_back(new ToggleMenuItem(3, "JOYSTICK:", joystickTypeToPrettyString(gIniSettings.joystickType).c_str(), KEY_J, KEY_UNKNOWN));

   menuItems.push_back(new MenuItem(4, "DEFINE KEYS / BUTTONS", KEY_D, KEY_K));

   menuItems.push_back(new ToggleMenuItem(5, "SFX VOLUME:", getVolMsg(gIniSettings.sfxVolLevel), KEY_S, KEY_UNKNOWN));
   menuItems.push_back(new ToggleMenuItem(7, "VOICE CHAT VOLUME:", getVolMsg(gIniSettings.voiceChatVolLevel), KEY_V, KEY_UNKNOWN));

   // No music yet, so keep this out to keep menus from getting too long.  Uncomment when we have music.
   //menuItems.push_back(new MenuItem("MUSIC VOLUME:", getVolMsg(gIniSettings.musicVolLevel), 6, KEY_M, KEY_UNKNOWN));

   menuItems.push_back(new ToggleMenuItem(8, "VOICE ECHO:", gIniSettings.echoVoice ? "ENABLED" : "DISABLED", KEY_E, KEY_UNKNOWN));
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
   reactivatePrevUI();      //gGameUserInterface
}


////////////////////////////////////////
////////////////////////////////////////

HostMenuUserInterface gHostMenuUserInterface;

// Constructor
HostMenuUserInterface::HostMenuUserInterface()
{
   setMenuID(HostingUI);
   dSprintf(menuTitle, sizeof(menuTitle), "HOST A GAME:");

   levelLoadDisplayFadeTimer.setPeriod(1000);
   gHostMenuUserInterface.levelLoadDisplayDisplay = true;
   mEditingIndex = -1;     // Not editing at the start
}


void HostMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();

   mLevelLoadDisplayTotal = 0;
   clearLevelLoadDisplay();
}


extern string gHostName;
extern string gHostDescr;
extern string gLevelChangePassword;
extern string gAdminPassword;
extern string gServerPassword;

void HostMenuUserInterface::setupMenus()
{
   menuItems.deleteAndClear();

   menuItems.push_back(new MenuItem        (OPT_HOST,       "START HOSTING", KEY_H));

   menuItems.push_back(new EditableMenuItem(OPT_NAME,       "SERVER NAME:",           gHostName,            "<Bitfighter Host>", 
                                            QueryServersUserInterface::MaxServerNameLen,  KEY_N));

   menuItems.push_back(new EditableMenuItem(OPT_DESCR,      "DESCRIPTION:",    gHostDescr,           "<Empty>",                    
                                           QueryServersUserInterface::MaxServerDescrLen, KEY_D));

   menuItems.push_back(new EditableMenuItem(OPT_LVL_PASS,   "LEVEL CHANGE PASSWORD:", gLevelChangePassword, "<Anyone can change levels>", 
                                            MAX_PASSWORD_LENGTH, KEY_L));

   menuItems.push_back(new EditableMenuItem(OPT_ADMIN_PASS, "ADMIN PASSWORD:",        gAdminPassword,       "<No remote admin access>",   
                                            MAX_PASSWORD_LENGTH, KEY_A));

   menuItems.push_back(new EditableMenuItem(OPT_PASS,       "CONNECTION PASSWORD:",   gServerPassword,      "<Anyone can connect>",       
                                            MAX_PASSWORD_LENGTH, KEY_C));

   menuItems.push_back(new EditableMenuItem(OPT_PORT,       "PORT:",                  "",                   "Defaults to 28000", 
                                            10, KEY_P));
}


extern void initHostGame(Address bindAddress, bool testMode);

void HostMenuUserInterface::processSelection(U32 index)        // Handler for unshifted menu shortcut key
{
   switch(index)
   {
   case OPT_HOST:
      saveSettings();
      initHostGame(Address(IPProtocol, Address::Any, 28000), false);
      break;
   case OPT_NAME:
   case OPT_DESCR:   
   case OPT_LVL_PASS:
   case OPT_ADMIN_PASS:
   case OPT_PASS:
   case OPT_PORT:   
      setActive(index);
      break;
   };
}


void HostMenuUserInterface::setActive(S32 i)
{
   if(mEditingIndex != -1)
      dynamic_cast<EditableMenuItem *>(menuItems[mEditingIndex])->setActive(false);

   mEditingIndex = i;

   if(mEditingIndex != -1)
   {
      dynamic_cast<EditableMenuItem *>(menuItems[mEditingIndex])->setActive(true);
      selectedIndex = mEditingIndex;
   }
}


// Save options to INI file, and return to our regularly scheduled program
// This only gets called when escape not already handled by preprocessKeys(), i.e. when we're not editing
void HostMenuUserInterface::onEscape()
{
   saveSettings();
   reactivatePrevUI();     
}


// Save parameters in INI file
void HostMenuUserInterface::saveSettings()
{
   gHostName            = gIniSettings.hostname            = dynamic_cast<EditableMenuItem *>(menuItems[OPT_NAME])->getValue();
   gHostDescr           = gIniSettings.hostdescr           = dynamic_cast<EditableMenuItem *>(menuItems[OPT_DESCR])->getValue();
   gLevelChangePassword = gIniSettings.levelChangePassword = dynamic_cast<EditableMenuItem *>(menuItems[OPT_LVL_PASS])->getValue();
   gAdminPassword       = gIniSettings.adminPassword       = dynamic_cast<EditableMenuItem *>(menuItems[OPT_ADMIN_PASS])->getValue();    
   gServerPassword      = gIniSettings.serverPassword      = dynamic_cast<EditableMenuItem *>(menuItems[OPT_PASS])->getValue();

   saveSettingsToINI();
}


// This gets run before any of the other process keys methods; used here to intercept keys intended for an active editor
bool HostMenuUserInterface::preprocessKeys(KeyCode keyCode, char ascii)
{
   if(mEditingIndex == -1)
      return false;

   // The following only get processed if we're editing a menu item
   if(keyCode == KEY_ESCAPE || keyCode == KEY_ENTER)
      setActive(-1);

   else if(keyCode == KEY_TAB)
   {
      setActive(mEditingIndex == OPT_COUNT - 1 ? FIRST_EDITABLE_ITEM : mEditingIndex + 1);
   }

   else if(keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE)
      dynamic_cast<EditableMenuItem *>(menuItems[mEditingIndex])->handleBackspace(keyCode);

   else dynamic_cast<EditableMenuItem *>(menuItems[mEditingIndex])->addChar(ascii);

   return true;
}


void HostMenuUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);   
   LineEditor::updateCursorBlink(timeDelta);
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
      glEnable(GL_BLEND);
      for(S32 i = 0; i < mLevelLoadDisplayNames.size(); i++)
      {
         glColor4f(1,1,1, (1.4 - ((F32) (mLevelLoadDisplayNames.size() - i) / 10.0)) * (levelLoadDisplayDisplay ? 1 : levelLoadDisplayFadeTimer.getFraction()) );
         drawStringf(100, canvasHeight - vertMargin - (mLevelLoadDisplayNames.size() - i) * 20, 15, "%s", mLevelLoadDisplayNames[i].c_str());
      }
      glDisable(GL_BLEND);
   }
}


////////////////////////////////////////
////////////////////////////////////////

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
   menuItems.deleteAndClear();

   lastInputMode = gIniSettings.inputMode;      // Save here so we can see if we need to display alert msg if input mode changes

   menuItems.push_back(new MenuItem(OPT_OPTIONS, "OPTIONS",      KEY_O));
   menuItems.push_back(new MenuItem(OPT_HELP,    "INSTRUCTIONS", KEY_I, keyHELP));
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
         menuItems.push_back(new MenuItem(OPT_CHOOSELEVEL, "PLAY DIFFERENT LEVEL", KEY_L, KEY_P));
         menuItems.push_back(new MenuItem(OPT_ADD2MINS,    "ADD TIME (2 MINS)",    KEY_T, KEY_2));
         menuItems.push_back(new MenuItem(OPT_RESTART,     "RESTART LEVEL",        KEY_R));
      }
      else
         menuItems.push_back(new MenuItem(OPT_LEVEL_CHANGE_PW, "ENTER LEVEL CHANGE PASSWORD", KEY_L, KEY_P));

      if(gc->isAdmin())
      {
         GameType *theGameType = gClientGame->getGameType();

         // Add any game-specific menu items
         if(theGameType)
         {
            mGameType = theGameType;
            theGameType->addAdminGameMenuOptions(menuItems);
         }

         menuItems.push_back(new MenuItem(OPT_KICK, "KICK A PLAYER", KEY_K));
      }
      else
         menuItems.push_back(new MenuItem(OPT_ADMIN, "ENTER ADMIN PASSWORD", KEY_A, KEY_E));
   }

   if(cameFromEditor())    // Came from editor
      menuItems.push_back(new MenuItem(OPT_QUIT, "RETURN TO EDITOR", KEY_Q, (cameFromEditor() ? KEY_R : KEY_UNKNOWN) ));
   else
      menuItems.push_back(new MenuItem(OPT_QUIT, "QUIT GAME", KEY_Q));
}


void GameMenuUserInterface::processSelection(U32 index)        // Handler for unshifted menu shortcut key
{
   switch(index)
   {
      case OPT_OPTIONS:
         gOptionsMenuUserInterface.activate();
         break;
      case OPT_HELP:
         gInstructionsUserInterface.activate();
         break;
      case OPT_QUIT:
         endGame();            // No matter where we came from, end the game
         break;
      case OPT_KICK:
         gPlayerMenuUserInterface.action = PlayerMenuUserInterface::Kick;
         gPlayerMenuUserInterface.activate();
         break;
      case OPT_ADD2MINS:     // Add 2 mins to game
         if(gClientGame->getGameType())
            gClientGame->getGameType()->addTime(2 * 60 * 1000);
            reactivatePrevUI();     // And back to our regularly scheduled programming!
         break;
      case OPT_ADMIN:
         gAdminPasswordEntryUserInterface.activate();
         break;
      case OPT_CHOOSELEVEL:
         gLevelMenuUserInterface.activate();
         break;
      case OPT_RESTART:
         gClientGame->getConnectionToServer()->c2sRequestLevelChange(-2, false);
         reactivatePrevUI();     // And back to our regularly scheduled programming! 
         break;
      case OPT_LEVEL_CHANGE_PW:
         gLevelChangePasswordEntryUserInterface.activate();
         break;
      default:    // A game-specific menu option must have been selected because it's not one we added here
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

////////////////////////////////////////
////////////////////////////////////////

LevelMenuUserInterface gLevelMenuUserInterface;

// Constructor
LevelMenuUserInterface::LevelMenuUserInterface()
{
   setMenuID(LevelTypeUI);
}


static const char *ALL_LEVELS = "All Levels";

void LevelMenuUserInterface::onActivate()
{
   Parent::onActivate();
   dSprintf(menuTitle, sizeof(menuTitle), "CHOOSE LEVEL TYPE:");

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(!gc || !gc->mLevelInfos.size())
      return;

   menuItems.deleteAndClear();

   char c[] = "A";   // Shortcut key
   menuItems.push_back(new MenuItem(9999, ALL_LEVELS, stringToKeyCode(c)));

   // Cycle through all levels, looking for unique type strings
   for(S32 i = 0; i < gc->mLevelInfos.size(); i++)
   {
      S32 j;
      for(j = 0; j < menuItems.size(); j++)
         if(gc->mLevelInfos[i].levelName == "" || !stricmp(gc->mLevelInfos[i].levelType.getString(), menuItems[j]->getText()) )     
         {
            break;                  // Skip over levels with blank names or duplicate entries
         }
      if(j == menuItems.size())     // Must be a new type
      {
         strncpy(c, gc->mLevelInfos[i].levelType.getString(), 1);
         menuItems.push_back(new MenuItem(i, gc->mLevelInfos[i].levelType.getString(), stringToKeyCode(c)));
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
      gLevelMenuSelectUserInterface.category = gc->mLevelInfos[index].levelType.getString();

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


////////////////////////////////////////
////////////////////////////////////////

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
   if(!gc || !gc->mLevelInfos.size())
      return;

   menuItems.deleteAndClear();
 
   char c[] = "A";  
   for(S32 i = 0; i < gc->mLevelInfos.size(); i++)
   {
      if(gc->mLevelInfos[i].levelName == "")   // Skip levels with blank names --> but all should have names now!
         continue;
      if(!strcmp( gc->mLevelInfos[i].levelType.getString(), category.c_str() ) || 
         !strcmp(category.c_str(), ALL_LEVELS) )
      {
         strncpy(c, gc->mLevelInfos[i].levelName.getString(), 1);
         menuItems.push_back(new MenuItem(i, gc->mLevelInfos[i].levelName.getString(), stringToKeyCode(c)));
      }
   }

   menuItems.sort(menuItemValueSort);
   currOffset = 0;

   if(itemSelectedWithMouse)
      onMouseMoved((S32)gMousePos.x, (S32)gMousePos.y);
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
         UserInterface::playBoop();

         return true;
      }
   }

   return false;
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


////////////////////////////////////////
////////////////////////////////////////

PlayerMenuUserInterface gPlayerMenuUserInterface;

// Constructor
PlayerMenuUserInterface::PlayerMenuUserInterface()
{
   setMenuID(PlayerUI);
}

// By putting all this code in render, it allows menus to be dynamically updated.
void PlayerMenuUserInterface::render()
{
   menuItems.deleteAndClear();
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

      // Will be used to show admin/player/robot prefix on menu
      PlayerType pt = gt->mClientList[i]->isRobot ? PlayerTypeRobot : (gt->mClientList[i]->isAdmin ? PlayerTypeAdmin : PlayerTypePlayer);    

      Color color = gt->getTeamColor(gt->mClientList[i]->getTeam());
      menuItems.push_back(new PlayerMenuItem(i, gt->mClientList[i]->name.getString(), stringToKeyCode(c), color, pt));
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
      if(menuItems[i]->getIndex() == index)
      {
         index = i;
         break;
      }

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(action == ChangeTeam)
   {
      gTeamMenuUserInterface.activate();     // Show menu to let player select a new team
      gTeamMenuUserInterface.nameToChange = menuItems[index]->getText();
   }
   else if(gc)    // action == Kick
   {
      StringTableEntry e(menuItems[index]->getText());
      gc->c2sAdminPlayerAction(e, action, -1);
   }

   if(action != ChangeTeam)                     // Unless we need to move on to the change team screen...
      reactivateMenu(gGameUserInterface);       // ...it's back to the game!
}

void PlayerMenuUserInterface::processShiftSelection(U32 index)   // Handler for shifted menu shortcut key
{
   processSelection(index);
}


////////////////////////////////////////
////////////////////////////////////////

TeamMenuUserInterface gTeamMenuUserInterface;

// Constructor
TeamMenuUserInterface::TeamMenuUserInterface()
{
   setMenuID(TeamUI);
}

// By reconstructing our menu at render time, changes to teams caused by others will be reflected immediately
void TeamMenuUserInterface::render()
{
   menuItems.deleteAndClear();

   GameType *gt = gClientGame->getGameType();
   if (!gt)
      return;

   gt->countTeamPlayers();    // Make sure numPlayers is correctly populated

   char c[] = "A";      // Dummy shortcut key, will change below
   for(S32 i = 0; i < gt->mTeams.size(); i++)
   {
      strncpy(c, gt->mTeams[i].getName().getString(), 1);     // Grab first char of name for a shortcut key

      bool isCurrent = (i == gt->getTeam(nameToChange));
      
      menuItems.push_back(new TeamMenuItem(i, gt->mTeams[i], stringToKeyCode(c), isCurrent));
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

   if(index != (U32)gt->getTeam(nameToChange))
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

