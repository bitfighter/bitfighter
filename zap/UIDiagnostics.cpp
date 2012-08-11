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

#include "config.h"
#include "UIDiagnostics.h"
#include "UINameEntry.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "masterConnection.h"
#include "input.h"
#include "version.h"
#include "ScreenInfo.h"
#include "Joystick.h"
#include "JoystickRender.h"
#include "ClientGame.h"
#include "ClientInfo.h"

#include "gameObjectRender.h"    // For drawCircle in badge rendering below

#include "ServerGame.h"          // For gServerGame refs

#include "tnl.h"

#include "OpenglUtils.h"

#include <stdio.h>
#include <cmath>

namespace Zap
{

static const char *pageHeaders[] = {
   "PLAYING",
   "FOLDERS",
   "HOSTING"
};

static const S32 NUM_PAGES = 3;



// Constructor
DiagnosticUserInterface::DiagnosticUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(DiagnosticsScreenUI);
}


void DiagnosticUserInterface::onActivate()
{
   mActive = true;
   mCurPage = 0;
}


void DiagnosticUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);
}


bool DiagnosticUserInterface::isActive()
{
   return mActive;
}


void DiagnosticUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();  // Back to our previously scheduled program!
   mActive = false;
}


bool DiagnosticUserInterface::onKeyDown(InputCode inputCode)
{
   if(checkInputCode(getGame()->getSettings(), InputCodeManager::BINDING_DIAG, inputCode))
   {
      mCurPage++;
      if(mCurPage >= NUM_PAGES)
         mCurPage = 0;
   }
   else if(checkInputCode(getGame()->getSettings(), InputCodeManager::BINDING_OUTGAMECHAT, inputCode))
   {
      // Do nothing -- no global chat from diagnostics screen... it's perverse!
   }
   else if(Parent::onKeyDown(inputCode))
   { 
      // Do nothing -- key handled
   }
   else if(inputCode == KEY_ESCAPE)
      quit();                          // Quit the interface
   else
      return false;

   // A key was handled
   return true;
}


S32 findLongestString(F32 size, const Vector<const char *> *strings)
{
   F32 maxLen = 0;
   S32 longest = 0;

   for(S32 i = 0; i < strings->size(); i++)
   {
      F32 len = (F32)UserInterface::getStringWidth(size, strings->get(i));
      if(len > maxLen)
      {
         maxLen = len;
         longest = i;
      }
   }
   return longest;
}


static Vector<const char *>names;
static Vector<const char *>vals;

static S32 longestName;
static S32 nameWidth;
static S32 spaceWidth;
static S32 longestVal;
static S32 totLen;

static void initFoldersBlock(FolderManager *folderManager, S32 textsize)
{
   names.push_back("Level Dir:");
   vals.push_back(folderManager->levelDir == "" ? "<<Unresolvable>>" : folderManager->levelDir.c_str());

   names.push_back("");
   vals.push_back("");

   names.push_back("INI Dir:");
   vals.push_back(folderManager->iniDir.c_str());
      
   names.push_back("Log Dir:");
   vals.push_back(folderManager->logDir.c_str());
      
   names.push_back("Lua Dir:");
   vals.push_back(folderManager->luaDir.c_str());
      
   names.push_back("Robot Dir:");
   vals.push_back(folderManager->robotDir.c_str());
      
   names.push_back("Screenshot Dir:");
   vals.push_back(folderManager->screenshotDir.c_str());
      
   names.push_back("SFX Dir:");
   vals.push_back(folderManager->sfxDir.c_str());

   names.push_back("Music Dir:");
   vals.push_back(folderManager->musicDir.c_str());

   names.push_back("");
   vals.push_back("");

   names.push_back("Root Data Dir:");
   vals.push_back(folderManager->rootDataDir == "" ? "None specified" : folderManager->rootDataDir.c_str());

   longestName = findLongestString((F32)textsize, &names);
   nameWidth   = UserInterface::getStringWidth(textsize, names[longestName]);
   spaceWidth  = UserInterface::getStringWidth(textsize, " ");
   longestVal  = findLongestString((F32)textsize, &vals);

   totLen = nameWidth + spaceWidth + UserInterface::getStringWidth(textsize, vals[longestVal]);
}


static S32 showFoldersBlock(FolderManager *folderManager, F32 textsize, S32 ypos, S32 gap)
{
   if(names.size() == 0)      // Lazy init
      initFoldersBlock(folderManager, (S32)textsize);

   for(S32 i = 0; i < names.size(); i++)
   {
      S32 xpos = (gScreenInfo.getGameCanvasWidth() - totLen) / 2;
      glColor(Colors::cyan);
      UserInterface::drawString(xpos, ypos, (S32)textsize, names[i]);
      xpos += nameWidth + spaceWidth;
      glColor(Colors::white);
      UserInterface::drawString(xpos, ypos, (S32)textsize, vals[i]);

      ypos += (S32)textsize + gap;
   }

   return ypos;
}


static S32 showVersionBlock(S32 ypos, S32 textsize, S32 gap)
{
   glColor(Colors::white);

   S32 x = UserInterface::getCenteredStringStartingPosf(textsize, "M/C Ver: %d | C/S Ver: %d | Build: %s/%d | CPU: %s | OS: %s | Cmplr: %s",
           MASTER_PROTOCOL_VERSION, CS_PROTOCOL_VERSION, ZAP_GAME_RELEASE, BUILD_VERSION, TNL_CPU_STRING, TNL_OS_STRING, TNL_COMPILER_STRING);

   glColor(Colors::white);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "M/C Ver: ");
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%d", MASTER_PROTOCOL_VERSION);

   glColor(Colors::white);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | C/S Ver: ");
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%d", CS_PROTOCOL_VERSION);

   glColor(Colors::white);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | Build: ");
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%d", BUILD_VERSION);
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "/");
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", ZAP_GAME_RELEASE);

   glColor(Colors::white);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | CPU: ");
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", TNL_CPU_STRING);

   glColor(Colors::white);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | OS: ");
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", TNL_OS_STRING);

   glColor(Colors::white);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | Cmplr: ");
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", TNL_COMPILER_STRING);

   return ypos + textsize + gap * 2;
}


static S32 showNameDescrBlock(const string &hostName, const string &hostDescr, S32 ypos, S32 textsize, S32 gap)
{
   S32 x = UserInterface::getCenteredStringStartingPosf(textsize, "Server Name: %s | Descr: %s", hostName.c_str(), hostDescr.c_str());

   glColor(Colors::white);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "Server Name: ");
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", hostName.c_str());

   glColor(Colors::white);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | Descr: ");
   glColor(Colors::yellow);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", hostDescr.c_str());

   return ypos + textsize + gap;
}


extern Color gMasterServerBlue;

static S32 showMasterBlock(ClientGame *game, S32 textsize, S32 ypos, S32 gap, bool leftcol)
{
   UserInterface::drawCenteredStringPair2Colf(ypos, textsize, leftcol, "Master Srvr Addr:", "%s", 
                                              game->getSettings()->getMasterServerList()->size() > 0 ? 
                                                         game->getSettings()->getMasterServerList()->get(0).c_str() : "None");

   ypos += textsize + gap;

   if(game->getConnectionToMaster() && game->getConnectionToMaster()->isEstablished())
   {
      glColor(gMasterServerBlue);
      UserInterface::drawCenteredString2Colf(ypos, textsize, leftcol, "Connected to [%s]", 
                                             game->getConnectionToMaster()->getMasterName().c_str() );
   }
   else
   {
      glColor(Colors::red);
      UserInterface::drawCenteredString2Col(ypos, textsize, leftcol, "Not connected to Master Server" );
   }

   return ypos + textsize + gap;
}


extern void drawHorizLine(S32 x1, S32 x2, S32 y);

void DiagnosticUserInterface::render()
{
   // Draw title, subtitle, and footer
   glColor(Colors::red);
   drawStringf(  3, 3, 25, "DIAGNOSTICS - %s", pageHeaders[mCurPage]);
   drawStringf(625, 3, 25, "PAGE %d/%d",       mCurPage + 1, NUM_PAGES);
 
   drawCenteredStringf(571, 20, "%s - next page  ESC exits", getInputCodeString(getGame()->getSettings(), InputCodeManager::BINDING_DIAG));

   glColor(0.7f);
   drawHorizLine(0, gScreenInfo.getGameCanvasWidth(), 31);
   drawHorizLine(0, gScreenInfo.getGameCanvasWidth(), 569);

   S32 textsize = 14;

   if(mCurPage == 0)
   {
      string inputMode = getGame()->getSettings()->getInputCodeManager()->getInputModeString();

      glColor(Colors::red);
      drawCenteredString(vertMargin + 37, 18, "Is something wrong?");

      S32 x, y;
      x = getCenteredStringStartingPosf(textsize, "Can't control your ship? Check your input mode "
                                                  "(Options>Primary Input) [currently %s]", inputMode.c_str());
      glColor(Colors::green);
      y = vertMargin + 63;
      x += drawStringAndGetWidth(x, y, textsize, "Can't control your ship? Check your input mode (Options>Primary Input) [currently ");

      glColor(Colors::red);
      x += drawStringAndGetWidth(x, y, textsize, inputMode.c_str());

      glColor(Colors::green);
      drawString(x, y, textsize, "]");

      // Box around something wrong? block
      glColor(Colors::cyan);
      drawHollowRect(horizMargin, vertMargin + 27, gScreenInfo.getGameCanvasWidth() - horizMargin, vertMargin + 90);

      const S32 gap = 5;

      S32 ypos = showVersionBlock(120, textsize, gap);

      glColor(Colors::white);

      textsize = 16;

      bool needToUpgrade = getUIManager()->getMainMenuUserInterface()->getNeedToUpgrade();

      drawCenteredString2Colf(ypos, textsize, false, "%s", needToUpgrade ? "<<Update available>>" : "<<Current version>>");
      ypos += textsize + gap;

      ClientInfo *clientInfo = getGame()->getClientInfo();

      // This following line is a bit of a beast, but it will return a valid result at any stage of being in or out of a game.
      // If the server modifies a user name to make it unique, this will display the modified version.
      drawCenteredStringPair2Colf(ypos, textsize, true, "Nickname:", "%s (%s)", 
                                  clientInfo->getName().getString(), 
                                  clientInfo->isAuthenticated() ? 
                                       string("Verified - " + itos(clientInfo->getBadges())).c_str() : "Not verified");

      ypos += textsize + gap;

      showMasterBlock(getGame(), textsize, ypos, gap, false);

      ypos += textsize + gap;
      drawCenteredStringPair2Colf(ypos, textsize, true, "Input Mode:", "%s", inputMode.c_str());
  
      ypos += textsize + gap;
      
      S32 index = Joystick::UseJoystickNumber;

      bool joystickDetected = Joystick::DetectedJoystickNameList.size() > 0;

      if(!joystickDetected)
         drawCenteredString2Col(ypos, textsize, true, "No joysticks detected");
      else
      {
         // Draw which profile we're using
         drawCenteredStringPair2Colf(ypos, textsize, true, "Current Profile:", "%s", Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].name.c_str());

         // Draw the raw SDL detection string
         drawCenteredStringPair2Colf(ypos + textsize + gap, textsize, true, Colors::magenta, Colors::cyan, "Autodetect String:", "%s",
               (U32(index) >= U32(Joystick::DetectedJoystickNameList.size()) || Joystick::DetectedJoystickNameList[index] == "") ? "<None>" : Joystick::DetectedJoystickNameList[index].c_str());
      }

      ypos += 6 * (textsize + gap);

      if(joystickDetected)
      {
         S32 x = 500;
         S32 y = 290;

         glColor(Colors::white);
         drawString(x, y, textsize - 2, "Raw Analog Axis Values:");

         y += 25;

         for(S32 i = 0; i < Joystick::rawAxisCount; i++)
         {
            F32 a = Joystick::rawAxis[i];    // Range: -1 to 1
            if(fabs(a) > .1f)
            {
               glColor(Colors::cyan);
               S32 len = drawStringAndGetWidthf(x, y, textsize - 2, "Axis %d", i);

               glColor(Colors::red);
               drawHorizLine(x, x + len, y + textsize + 3);

               glColor(Colors::yellow);
               drawHorizLine(x + len / 2, x + len / 2 + S32(a * F32(len / 2)), y + textsize + 3);

               x += len + 8;
            }
         }
      }

      // Key states
      glColor(Colors::yellow);
      S32 hpos = horizMargin;

      hpos += drawStringAndGetWidth(hpos, ypos, textsize, "Keys down: ");

      glColor(Colors::red);
      for(U32 i = 0; i < MAX_INPUT_CODES; i++)
         if(InputCodeManager::getState((InputCode) i))
            hpos += drawStringAndGetWidthf( hpos, ypos, textsize - 2, "[%s]", InputCodeManager::inputCodeToString(InputCode(i)) ) + 5;

      glColor(Colors::cyan);
      hpos += drawStringAndGetWidth(hpos, ypos, textsize, " | ");

      glColor(Colors::yellow);
      hpos += drawStringAndGetWidth(hpos, ypos, textsize, "Input strings: ");

      glColor(Colors::magenta);
      for(U32 i = 0; i < MAX_INPUT_CODES; i++)
         if(InputCodeManager::getState((InputCode) i))
         {
            string in = InputCodeManager::makeInputString(InputCode(i));

            if(in != "")
               hpos += drawStringAndGetWidthf( hpos, ypos, textsize - 2, "[%s]", in.c_str() ) + 5;
         }

      if(joystickDetected)
      {
         glColor(Colors::magenta);
         ypos += textsize + gap;
         hpos = horizMargin;

         hpos += drawStringAndGetWidthf(hpos, ypos, textsize - 2, "Raw Controller Input [%d]: ", Joystick::UseJoystickNumber);

         for(U32 i = 0; i < 32; i++)  // there are 32 bit in U32
            if(Joystick::ButtonMask & BIT(i))
               hpos += drawStringAndGetWidthf( hpos, ypos, textsize - 2, "(%d)", i ) + 5;

         // TODO render raw D-pad (SDL Hat)

         ypos += textsize + gap + 10;

         glColor(Colors::green);
         drawCenteredString(ypos, textsize, "Hint: If you're having joystick problems, check your controller's 'mode' button.");

         //////////
         // Draw joystick and button map
         hpos = 100;
         ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - 110;

         JoystickRender::renderDPad(Point(hpos, ypos), 25, 
               InputCodeManager::getState(BUTTON_DPAD_UP),   InputCodeManager::getState(BUTTON_DPAD_DOWN),
               InputCodeManager::getState(BUTTON_DPAD_LEFT), InputCodeManager::getState(BUTTON_DPAD_RIGHT), 
               "DPad", "(Menu Nav)");
         hpos += 75;

         JoystickRender::renderDPad(Point(hpos, ypos), 25, 
               InputCodeManager::getState(STICK_1_UP),   InputCodeManager::getState(STICK_1_DOWN),
               InputCodeManager::getState(STICK_1_LEFT), InputCodeManager::getState(STICK_1_RIGHT), 
               "L Stick", "(Move)");
         hpos += 75;

         JoystickRender::renderDPad(Point(hpos, ypos), 25, 
               InputCodeManager::getState(STICK_2_UP),   InputCodeManager::getState(STICK_2_DOWN),
               InputCodeManager::getState(STICK_2_LEFT), InputCodeManager::getState(STICK_2_RIGHT), 
               "R Stick", "(Fire)");
         hpos += 55;

         U32 joystickIndex = Joystick::SelectedPresetIndex;

         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_1, InputCodeManager::getState(BUTTON_1));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_2, InputCodeManager::getState(BUTTON_2));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_3, InputCodeManager::getState(BUTTON_3));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_4, InputCodeManager::getState(BUTTON_4));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_5, InputCodeManager::getState(BUTTON_5));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_6, InputCodeManager::getState(BUTTON_6));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_7, InputCodeManager::getState(BUTTON_7));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_8, InputCodeManager::getState(BUTTON_8));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_BACK, InputCodeManager::getState(BUTTON_BACK));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_START, InputCodeManager::getState(BUTTON_START));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_9, InputCodeManager::getState(BUTTON_9));
         hpos += 40;
         JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, BUTTON_10, InputCodeManager::getState(BUTTON_10));
      }
   }
   else if(mCurPage == 1)
   {
      S32 ypos = vertMargin + 35;
      S32 textsize = 15;
      S32 gap = 5;

      drawString(horizMargin, ypos, textsize, "Folders are either an absolute path or a path relative to the program execution folder");
      ypos += textsize + gap;
      drawString(horizMargin, ypos, textsize, "or local folder, depending on OS.  If an entry is blank, Bitfighter will look for files");
      ypos += textsize + gap;
      drawString(horizMargin, ypos, textsize, "in the program folder or local folder, depending on OS.");
      ypos += textsize + gap;
      ypos += textsize + gap;
      drawString(horizMargin, ypos, textsize, "See the Command line parameters section of the wiki at bitfighter.org for more information.");
      ypos += textsize + gap;
      ypos += textsize + gap;

      glColor(Colors::red);
      drawCenteredString(ypos, textsize, "Currently reading data and settings from:");
      ypos += textsize + gap + gap;

      FolderManager *folderManager = getGame()->getSettings()->getFolderManager();
      ypos = showFoldersBlock(folderManager, (F32)textsize, ypos, gap+2);
   }
   else if(mCurPage == 2)
   {
      S32 gap = 5;
      S32 textsize = 16;
      S32 smallText = 14;

      S32 ypos = vertMargin + 35;

      glColor(Colors::white);

      GameSettings *settings = getGame()->getSettings();

      ypos += showNameDescrBlock(settings->getHostName(), settings->getHostDescr(), ypos, textsize, gap);

      drawCenteredStringPair2Colf(ypos, textsize, true, "Host Addr:", "%s", settings->getHostAddress().c_str());
      drawCenteredStringPair2Colf(ypos, smallText, false, "Lvl Change PW:", "%s", settings->getLevelChangePassword() == "" ?
                                                                    "None - anyone can change" : settings->getLevelChangePassword().c_str());
      ypos += textsize + gap;

      
      drawCenteredStringPair2Colf(ypos, smallText, false, "Admin PW:", "%s", settings->getAdminPassword() == "" ? 
                                                                     "None - no one can get admin" : settings->getAdminPassword().c_str());
      ypos += textsize + gap;

      drawCenteredStringPair2Colf(ypos, textsize, false, "Server PW:", "%s", settings->getServerPassword() == "" ? 
                                                                             "None needed to play" : settings->getServerPassword().c_str());

      ypos += textsize + gap;
      ypos += textsize + gap;

      S32 x = getCenteredString2ColStartingPosf(textsize, false, "Max Players: %d", settings->getMaxPlayers());

      glColor(Colors::white);
      x += drawStringAndGetWidthf(x, ypos, textsize, "Max Players: ");
      glColor(Colors::yellow);
      x += drawStringAndGetWidthf(x, ypos, textsize, "%d", settings->getMaxPlayers());

      ypos += textsize + gap;

      drawCenteredStringPair2Colf(ypos, textsize, false, "Sim. Lag/Pkt. Loss:", "%dms /%2.0f%%", 
                                  getGame()->getSettings()->getSimulatedLag(), getGame()->getSettings()->getSimulatedLoss() * 100);
      ypos += textsize + gap;
      ypos += textsize + gap;
      

      // Dump out names of loaded levels...
      glColor(Colors::white);
      string allLevels = "Levels: ";

      if(!gServerGame)
         allLevels += " >>> Level list won't be resolved until you start hosting <<<"; 
      else
         for(S32 i = 0; i < gServerGame->getLevelCount(); i++)
            allLevels += string(gServerGame->getLevelNameFromIndex(i).getString()) + "; ";

      U32 i, j, k;
      i = j = k = 0;
      
      for(j = 0; j < 4 && i < allLevels.length(); j++)
      {
         while(getStringWidth(textsize - 6, allLevels.substr(k, i - k).c_str()) < 
               gScreenInfo.getGameCanvasWidth() - 2 * horizMargin && i < allLevels.length()) 
         {
            i++;
         }

         drawString(horizMargin, ypos, textsize - 6, allLevels.substr(k, i - k).c_str());
         k = i;
         ypos += textsize + gap - 5;
      }

      ypos += (textsize + gap) * (3 - j);


      // Temporary placeholder for badge testing -- centered at xpos, ypos, with radius of rad (i.e. 2 x rad across)
#ifdef TNL_DEBUG
      for(S32 i = 0; i < 2; i++)
      {
         F32 x;
         F32 y;

         x = horizMargin + 10;
         y = 500 + 20 * i;

         F32 rad = 10;
         F32 smallSize = .6;
            
         glPushMatrix();
         glScale(i ? smallSize : 1);
         y *= (i ? 1/smallSize : 1);

         
         F32 rm2 = rad - 2;
         F32 r3 = rad * .333;
         F32 rm23 = rm2 * .333;

         glColor(Colors::white);
         drawPolygon(Point(x,y), rad *.666, rm2, 0);
         glColor(Colors::red);
         drawCircle(Point(x, y), rad);

         x += 3*rad;

         glColor(Colors::yellow);
         drawPolygon(Point(x,y), 3, rm2, FloatTau/12);
         glColor(Colors::red);
         drawCircle(Point(x, y), rad);

         x += 3*rad;
         glColor(Colors::green);
         drawHollowRect(x - rad, y - r3,  x + rad, y + r3);
         drawHollowRect(x - r3,  y - rad, x + r3,  y + rad);


         // Use rm2 and rm23 to make squares a little smaller to balance size of the circles

         x += 3*rad;
         glColor(Colors::red);
         drawFilledRect(x - rm2, y - rm2, x + rm2, y + rm2);
         glColor(Colors::white);
         drawFilledRect(x - rm23,  y - rm2, x + rm23, y - rm23);
         drawFilledRect(x - rm23,  y + rm2, x + rm23, y + rm23);
         drawFilledRect(x + rm2, y - rm23,  x + rm23, y + rm23);
         drawFilledRect(x - rm2, y - rm23,  x - rm23, y + rm23);

         x += 3*rad;
         glColor(Colors::red);
         drawHollowRect(x - rm2, y - rm2, x + rm2, y + rm2);
         drawCircle(Point(x, y), rm2);
         glColor(Colors::orange67);
         drawCircle(Point(x, y), rad / 2);

         x += 3*rad;
         glColor(Colors::red);
         drawHollowRect(x - rm2, y - rm2, x + rm2, y + rm2);
         drawCircle(Point(x, y), rm2);
         glColor(Colors::yellow);
         drawFilledCircle(Point(x, y), rad / 2);
         glColor(Colors::orange67);
         drawCircle(Point(x, y), rad / 2);


         x += 3*rad;
         glColor(Colors::red);
         drawCircle(Point(x, y), rad);
         glColor(Colors::white);
         drawCircle(Point(x, y), r3 * 2);
         glColor(Colors::red);
         drawCircle(Point(x, y), r3);


         x += 3*rad;
         glColor(Colors::paleBlue);
         drawPolygon(Point(x,y + r3), 3, rad * 1.2, FloatTau/12);
         glColor(Colors::cyan);
         drawPolygon(Point(x,y + r3), 3, rad * .6, FloatTau/4);


         x += 3*rad;
         glColor(Colors::red);
         drawCircle(Point(x, y), rad);
         glColor(Colors::white);
         drawStar(Point(x,y), 7, rad - 1, rad/2);

         x += 3*rad;
         renderBadge(x, y, rad, DEVELOPER_BADGE);

         x += 3*rad;
         renderBadge(x, y, rad, BADGE_TWENTY_FIVE_FLAGS);

         x += 3*rad;
         renderBadge(x, y, rad, BADGE_BBB_GOLD);

         x += 3*rad;
         renderBadge(x, y, rad, BADGE_BBB_SILVER);

         x += 3*rad;
         renderBadge(x, y, rad, BADGE_BBB_BRONZE);

         x += 3*rad;
         renderBadge(x, y, rad, BADGE_LEVEL_DESIGN_WINNER);

         // Level design contest winner badge
         x += 3*rad;
         Vector<Point> points;
         points.push_back(Point(x - rm2, y - rm2));
         points.push_back(Point(x - rm2, y + rm2));
         points.push_back(Point(x + rm2, y + rm2));
         points.push_back(Point(x + rm2, y - rm2));
         glColor(0.5f, 0.5f, 1.0f);
         renderWallFill(&points, false);
         renderPolygonOutline(&points, &Colors::blue);
         glColor(Colors::yellow);
         drawStar(Point(x,y), 5, rad * .5, rad * .25);

         ///// After all badge rendering
         glPopMatrix();
      }
#endif // TNL_DEBUG
   }
}

};


