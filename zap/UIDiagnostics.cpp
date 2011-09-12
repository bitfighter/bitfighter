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


#include "tnl.h"

#include "SDL/SDL_opengl.h"

#include <stdio.h>

namespace Zap
{

extern CmdLineSettings gCmdLineSettings;

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


bool DiagnosticUserInterface::isActive()
{
   return mActive;
}


void DiagnosticUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();  // Back to our previously scheduled program!
   mActive = false;
}


void DiagnosticUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_ESCAPE)
      quit();                          // Quit the interface
   else if(keyCode == keyDIAG)
   {
      mCurPage++;
      if(mCurPage >= NUM_PAGES)
         mCurPage = 0;
   }
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

static void initFoldersBlock(ConfigDirectories *folderManager, S32 textsize)
{
   names.push_back("Level Dir:");
   vals.push_back(folderManager->levelDir == "" ? "<<Unresolvable>>" : folderManager->levelDir.c_str());

   names.push_back("");
   vals.push_back("");
      
   names.push_back("Cache Dir:");
   vals.push_back(folderManager->cacheDir.c_str());

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
   vals.push_back(gCmdLineSettings.dirs.rootDataDir == "" ? "None specified" : gCmdLineSettings.dirs.rootDataDir.c_str());

   longestName = findLongestString((F32)textsize, &names);
   nameWidth = UserInterface::getStringWidth(textsize, names[longestName]);
   spaceWidth = UserInterface::getStringWidth(textsize, " ");
   longestVal = findLongestString((F32)textsize, &vals);

   totLen = nameWidth + spaceWidth + UserInterface::getStringWidth(textsize, vals[longestVal]);
}


static S32 showFoldersBlock(ConfigDirectories *folderManager, F32 textsize, S32 ypos, S32 gap)
{
   if(names.size() == 0)      // Lazy init
      initFoldersBlock(folderManager, (S32)textsize);

   for(S32 i = 0; i < names.size(); i++)
   {
      S32 xpos = (gScreenInfo.getGameCanvasWidth() - totLen) / 2;
      glColor3f(0,1,1);
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
                                              game->getMasterAddressList().size() > 0 ? game->getMasterAddressList()[0].c_str() : "None");

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


void DiagnosticUserInterface::render()
{
   // Draw title, subtitle, and footer
   glColor3f(1,0,0);
   drawStringf(3, 3, 25, "DIAGNOSTICS - %s", pageHeaders[mCurPage]);
   drawStringf(625, 3, 25, "PAGE %d/%d", mCurPage + 1, NUM_PAGES);
 
   drawCenteredStringf(571, 20, "%s - next page  ESC exits", keyCodeToString(keyDIAG));
   glColor3f(0.7f, 0.7f, 0.7f);
   glBegin(GL_LINES);
      glVertex2f(0, 31);
      glVertex2f(800, 31);
      glVertex2f(0, 569);
      glVertex2f(800, 569);
   glEnd();

   S32 textsize = 14;

   if(mCurPage == 0)
   {
      string inputMode = gIniSettings.getInputMode();

      glColor3f(1,0,0);
      drawCenteredString(vertMargin + 37, 18, "Is something wrong?");

      S32 x;
      x = getCenteredStringStartingPosf(textsize, "Can't control your ship? Check your input mode "
                                                  "(Options>Primary Input) [currently %s]", inputMode.c_str());
      glColor3f(0,1,0);
      x += drawStringAndGetWidth(x, vertMargin + 63, textsize, "Can't control your ship? Check your input mode (Options>Primary Input) [currently ");

      glColor3f(1,0,0);
      x += drawStringAndGetWidthf(x, vertMargin + 63, textsize, "%s", inputMode.c_str());

      glColor3f(0,1,0);
      drawString(x, vertMargin + 63, textsize, "]");

      // Box around something wrong? block
      glColor3f(0,1,1);
      glBegin(GL_LINE_LOOP);
         S32 x1 = horizMargin;
         S32 x2 = gScreenInfo.getGameCanvasWidth() - horizMargin;
         S32 y1 = vertMargin + 27;
         S32 y2 = vertMargin + 90;

         glVertex2i(x1, y1);     glVertex2i(x2, y1);
         glVertex2i(x2, y2);     glVertex2i(x1, y2);
      glEnd();


      const S32 gap = 5;

      S32 ypos = showVersionBlock(120, 14, gap);

      glColor(Colors::white);

      textsize = 16;

      bool needToUpgrade = getUIManager()->getMainMenuUserInterface()->getNeedToUpgrade();

      drawCenteredString2Colf(ypos, textsize, false, "%s", needToUpgrade ? "<<Update available>>" : "<<Current version>>");
      ypos += textsize + gap;
      // This following line is a bit of a beast, but it will return a valid result at any stage of being in or out of a game.
      // If the server modifies a user name to make it unique, this will display the modified version.
      drawCenteredStringPair2Colf(ypos, textsize, true, "Nickname:", "%s (%s)", 
                                  getGame()->getClientInfo()->name.c_str(), 
                                  getGame()->getClientInfo()->authenticated ? "Verified" : "Not verified");

      ypos += textsize + gap;

      showMasterBlock(getGame(), textsize, ypos, gap, false);

      ypos += textsize + gap;
      drawCenteredStringPair2Colf(ypos, textsize, true, "Input Mode:", "%s", inputMode.c_str());
  
      ypos += textsize + gap;
      
      S32 index = Joystick::UseJoystickNumber == 0 ? 1 : Joystick::UseJoystickNumber - 1;

      if(Joystick::DetectedJoystickNameList.size() == 0)
         drawCenteredString2Col(ypos, textsize, true, "No joysticks detected");
      else
         drawCenteredStringPair2Colf(ypos, textsize, true, "Autodetect Str.:", "%s", 
               (U32(index) >= U32(Joystick::DetectedJoystickNameList.size()) || Joystick::DetectedJoystickNameList[index] == "") ? "<None>" : Joystick::DetectedJoystickNameList[index]);

      ypos += textsize + gap;
      ypos += textsize + gap;
      ypos += textsize + gap;
      ypos += textsize + gap;
      ypos += textsize + gap;
      ypos += textsize + gap;

      const S32 rawAxisPosX = 500;
      const S32 rawAxisPosY = 330;
      glColor3f(1, 0, 1);
      drawString(rawAxisPosX, rawAxisPosY-40, textsize - 2, "Raw Analog Axis:");
      glBegin(GL_LINES);
      for(S32 i = 0; i < Joystick::rawAxisCount; i++) // shows RAW axis inputs
      {
         glColor3f(0.5,0,0);
         glVertex2i(i*8+rawAxisPosX, rawAxisPosY - 20);
         glVertex2i(i*8+rawAxisPosX, rawAxisPosY + 20);
         glColor(Colors::yellow);
         glVertex2i(i*8+rawAxisPosX, rawAxisPosY);
         F32 a = Joystick::rawAxis[i];
         if(a < -1) a = -1;
         if(a > 1) a = 1;
         glVertex2f(F32(i*8+rawAxisPosX), rawAxisPosY + a * 20);
      }
      glEnd();

      // Key states
      glColor3f(1, 1, 0);
      S32 hpos = horizMargin;
      drawString( hpos, ypos, textsize, "Keys down: ");
      hpos += getStringWidth(textsize, "Keys down: ");

      for (U32 i = 0; i < MAX_KEYS; i++)
         if(getKeyState((KeyCode) i))
            hpos += drawStringAndGetWidthf( hpos, ypos, textsize - 2, "[%s]", keyCodeToString((KeyCode) i) ) + 5;

      glColor3f(1, 0, 1);
      ypos += textsize + gap;
      hpos = horizMargin;
      hpos += drawStringAndGetWidthf( hpos, ypos, textsize - 2, "Raw Controller Input [%d]: ", Joystick::UseJoystickNumber);

      for(U32 i = 0; i < MaxControllerButtons; i++)
         if(Joystick::ButtonMask & BIT(i))
            hpos += drawStringAndGetWidthf( hpos, ypos, textsize - 2, "(%d)", i ) + 5;

      ypos += textsize + gap + 10;

      glColor3f(0, 1, 0);
      drawCenteredString(ypos, textsize, "Hint: If you're having joystick problems, check your controller's 'mode' button.");

      //////////
      // Draw joystick and button map
      hpos = 100;
      ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - 110;

      JoystickRender::renderDPad(Point(hpos, ypos), 25, getKeyState(BUTTON_DPAD_UP), getKeyState(BUTTON_DPAD_DOWN),
                 getKeyState(BUTTON_DPAD_LEFT), getKeyState(BUTTON_DPAD_RIGHT), "DPad", "(Menu Nav)");
      hpos += 75;

      JoystickRender::renderDPad(Point(hpos, ypos), 25, getKeyState(STICK_1_UP), getKeyState(STICK_1_DOWN),
                 getKeyState(STICK_1_LEFT), getKeyState(STICK_1_RIGHT), "L Stick", "(Move)");
      hpos += 75;

      JoystickRender::renderDPad(Point(hpos, ypos), 25, getKeyState(STICK_2_UP), getKeyState(STICK_2_DOWN),
                 getKeyState(STICK_2_LEFT), getKeyState(STICK_2_RIGHT), "R Stick", "(Fire)");
      hpos += 55;

      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_1, getKeyState(BUTTON_1));
      hpos += 40;
      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_2, getKeyState(BUTTON_2));
      hpos += 40;
      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_3, getKeyState(BUTTON_3));
      hpos += 40;
      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_4, getKeyState(BUTTON_4));
      hpos += 40;
      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_5, getKeyState(BUTTON_5));
      hpos += 40;
      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_6, getKeyState(BUTTON_6));
      hpos += 40;
      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_7, getKeyState(BUTTON_7));
      hpos += 40;
      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_8, getKeyState(BUTTON_8));
      hpos += 40;
      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_BACK, getKeyState(BUTTON_BACK));
      hpos += 40;
      JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, BUTTON_START, getKeyState(BUTTON_START));
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

      glColor3f(1,0,0);
      drawCenteredString(ypos, textsize, "Currently reading data and settings from:");
      ypos += textsize + gap + gap;

      ypos = showFoldersBlock(getGame()->getSettings()->getConfigDirs(), (F32)textsize, ypos, gap+2);
   }
   else if(mCurPage == 2)
   {
      S32 gap = 5;
      S32 textsize = 16;

      S32 ypos = vertMargin + 35;

      glColor(Colors::white);

      GameSettings *settings = getGame()->getSettings();

      ypos += showNameDescrBlock(settings->getHostName(), settings->getHostDescr(), ypos, textsize, gap);

      drawCenteredStringPair2Colf(ypos, textsize, true, "Host Addr:", "%s", settings->getHostAddress().c_str());
      drawCenteredStringPair2Colf(ypos, 14, false, "Lvl Change PW:", "%s", settings->getLevelChangePassword() == "" ?
                                                                    "None - anyone can change" : settings->getLevelChangePassword().c_str());
      ypos += textsize + gap;

      
      drawCenteredStringPair2Colf(ypos, 14, false, "Admin PW:", "%s", settings->getAdminPassword() == "" ? 
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
                                  getGame()->getClientInfo()->simulatedLag, getGame()->getClientInfo()->simulatedPacketLoss * 100);
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
         for(; getStringWidth(textsize - 6, allLevels.substr(k, i - k).c_str()) < 
               gScreenInfo.getGameCanvasWidth() - 2 * horizMargin && i < allLevels.length(); i++)   // first arg empty
         {
            ;     // Do nothing...
         }

         drawString(horizMargin, ypos, textsize - 6, allLevels.substr(k, i - k).c_str());
         k = i;
         ypos += textsize + gap - 5;
      }

      ypos += (textsize + gap) * (3 - j);
   }
}

};


