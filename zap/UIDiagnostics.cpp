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

#include "tnl.h"

#include "../glut/glutInclude.h"
#include <stdio.h>

namespace Zap
{

extern string gHostName, gHostDescr;
extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;
extern F32 gSimulatedPacketLoss;
extern string gServerPassword;
extern string gAdminPassword;
extern string gLevelChangePassword;
extern U32 gSimulatedLag;
extern U32 getServerMaxPlayers();
extern U32 gRawJoystickButtonInputs;
extern void renderDPad(Point center, F32 radius, bool upStat, bool downStat, bool leftStat, bool rightStat, const char *msg1, const char *msg2);
extern Address gBindAddress;

DiagnosticUserInterface gDiagnosticInterface;


static const char *pageHeaders[] = {
   "PLAYING",
   "FOLDERS",
   "HOSTING"
};

static const S32 NUM_PAGES = 3;



// Constructor
DiagnosticUserInterface::DiagnosticUserInterface()
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
   UserInterface::reactivatePrevUI();  // Back to our previously scheduled program!
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
   S32 maxLen = 0;
   S32 longest = 0;

   for(S32 i = 0; i < strings->size(); i++)
   {
      S32 len = UserInterface::getStringWidth(size, strings->get(i));
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

extern ConfigDirectories gConfigDirs;

static void initFoldersBlock(S32 textsize)
{
   names.push_back("Level Dir:");
   vals.push_back(gConfigDirs.levelDir == "" ? "<<Unresolvable>>" : gConfigDirs.levelDir.c_str());

   names.push_back("");
   vals.push_back("");
      
   names.push_back("Cache Dir:");
   vals.push_back(gConfigDirs.cacheDir.c_str());

   names.push_back("INI Dir:");
   vals.push_back(gConfigDirs.iniDir.c_str());
      
   names.push_back("Log Dir:");
   vals.push_back(gConfigDirs.logDir.c_str());
      
   names.push_back("Lua Dir:");
   vals.push_back(gConfigDirs.luaDir.c_str());
      
   names.push_back("Robot Dir:");
   vals.push_back(gConfigDirs.robotDir.c_str());
      
   names.push_back("Screenshot Dir:");
   vals.push_back(gConfigDirs.screenshotDir.c_str());
      
   names.push_back("SFX Dir:");
   vals.push_back(gConfigDirs.sfxDir.c_str());

   names.push_back("");
   vals.push_back("");

   names.push_back("Root Data Dir:");
   vals.push_back(gCmdLineSettings.dirs.rootDataDir == "" ? "None specified" : gCmdLineSettings.dirs.rootDataDir.c_str());

   longestName = findLongestString(textsize, &names);
   nameWidth = UserInterface::getStringWidth(textsize, names[longestName]);
   spaceWidth = UserInterface::getStringWidth(textsize, " ");
   longestVal = findLongestString(textsize, &vals);

   totLen = nameWidth + spaceWidth + UserInterface::getStringWidth(textsize, vals[longestVal]);
}


static S32 showFoldersBlock(F32 textsize, S32 ypos, S32 gap)
{
   if(names.size() == 0)      // Lazy init
      initFoldersBlock((S32)textsize);

   for(S32 i = 0; i < names.size(); i++)
   {
      S32 xpos = (gScreenInfo.getGameCanvasWidth() - totLen) / 2;
      glColor3f(0,1,1);
      UserInterface::drawString(xpos, ypos, (S32)textsize, names[i]);
      xpos += nameWidth + spaceWidth;
      glColor3f(1,1,1);
      UserInterface::drawString(xpos, ypos, (S32)textsize, vals[i]);

      ypos += (S32)textsize + gap;
   }

   return ypos;
}


static S32 showVersionBlock(S32 ypos, S32 textsize, S32 gap)
{
   glColor3f(1,1,1);

   S32 x = UserInterface::getCenteredStringStartingPosf(textsize, "M/C Ver: %d | C/S Ver: %d | Build: %s/%d | CPU: %s | OS: %s | Cmplr: %s",
           MASTER_PROTOCOL_VERSION, CS_PROTOCOL_VERSION, ZAP_GAME_RELEASE, BUILD_VERSION, TNL_CPU_STRING, TNL_OS_STRING, TNL_COMPILER_STRING);

   glColor3f(1,1,1);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "M/C Ver: ");
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%d", MASTER_PROTOCOL_VERSION);

   glColor3f(1,1,1);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | C/S Ver: ");
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%d", CS_PROTOCOL_VERSION);

   glColor3f(1,1,1);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | Build: ");
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%d", BUILD_VERSION);
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "/");
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", ZAP_GAME_RELEASE);

   glColor3f(1,1,1);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | CPU: ");
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", TNL_CPU_STRING);

   glColor3f(1,1,1);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | OS: ");
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", TNL_OS_STRING);

   glColor3f(1,1,1);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | Cmplr: ");
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", TNL_COMPILER_STRING);

   return ypos + textsize + gap * 2;
}


static S32 showNameDescrBlock(S32 ypos, S32 textsize, S32 gap)
{
   glColor3f(1,1,1);

   S32 x = UserInterface::getCenteredStringStartingPosf(textsize, "Server Name: %s | Descr: %s", gHostName.c_str(), gHostDescr.c_str());

   glColor3f(1,1,1);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "Server Name: ");
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", gHostName.c_str());

   glColor3f(1,1,1);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, " | Descr: ");
   glColor3f(1,1,0);
   x += UserInterface::drawStringAndGetWidthf(x, ypos, textsize, "%s", gHostDescr.c_str());

   return ypos + textsize + gap;
}


extern Color gMasterServerBlue;

static S32 showMasterBlock(S32 textsize, S32 ypos, S32 gap, bool leftcol)
{
   UserInterface::drawCenteredStringPair2Colf(ypos, textsize, leftcol, "Master Srvr Addr:", "%s", gMasterAddress.toString());
      
   ypos += textsize + gap;
   if(gClientGame && gClientGame->getConnectionToMaster() && gClientGame->getConnectionToMaster()->isEstablished())
   {
      glColor(gMasterServerBlue);
      UserInterface::drawCenteredString2Colf(ypos, textsize, leftcol, "Connected to [%s]", 
                                             gClientGame->getConnectionToMaster()->getMasterName().c_str() );
   }
   else
   {
      glColor3f(1, 0, 0);
      UserInterface::drawCenteredString2Col(ypos, textsize, leftcol, "Not connected to Master Server" );
   }

   return ypos + textsize + gap;
}


extern ClientGame *gClientGame;
extern const S32 gJoystickNameLength;
extern char gJoystickName[gJoystickNameLength];
extern ControllerTypeType gAutoDetectedJoystickType;
extern U32 gSticksFound;
extern string gLevelDir;
extern string gPlayerPassword;
extern ClientInfo gClientInfo;

void DiagnosticUserInterface::render()
{
   /*
   if (prevUIs.size())           // If there is an underlying menu...
   {
      prevUIs.last()->render();  // ...render it...

      glColor4f(0, 0, 0, 0.75);  // ... and dim it out a bit, nay, a lot
      glEnable(GL_BLEND);
         glBegin(GL_POLYGON);
            glVertex2f(0, 0);
            glVertex2f(canvasWidth, 0);
            glVertex2f(canvasWidth, canvasHeight);
            glVertex2f(0, canvasHeight);
         glEnd();
      glDisable(GL_BLEND);
   }
   */

   // Draw title, subtitle, and footer
   glColor3f(1,0,0);
   drawStringf(3, 3, 25, "DIAGNOSTICS - %s", pageHeaders[mCurPage]);
   drawStringf(625, 3, 25, "PAGE %d/%d", mCurPage + 1, NUM_PAGES);
 
   drawCenteredStringf(571, 20, "%s - next page  ESC exits", keyCodeToString(keyDIAG));
   glColor3f(0.7, 0.7, 0.7);
   glBegin(GL_LINES);
      glVertex2f(0, 31);
      glVertex2f(800, 31);
      glVertex2f(0, 569);
      glVertex2f(800, 569);
   glEnd();

   const S32 ts = 14;

   if(mCurPage == 0)
   {
      glColor3f(1,0,0);
      drawCenteredString(vertMargin + 37, 18, "Is something wrong?");

      S32 x;
      x = getCenteredStringStartingPosf(ts, "Can't control your ship? Check your input mode (Options>Primary Input) [currently %s]",            
                                        gIniSettings.inputMode == Keyboard ? "Keyboard" : "Joystick");
      glColor3f(0,1,0);
      drawString(x, vertMargin + 63, ts, "Can't control your ship? Check your input mode (Options>Primary Input) [currently ");
      x += getStringWidth(ts, "Can't control your ship? Check your input mode (Options>Primary Input) [currently ");
      glColor3f(1,0,0);
      drawString(x, vertMargin + 63, ts, gIniSettings.inputMode == Keyboard ? "Keyboard" : "Joystick");
      x += getStringWidth(ts, gIniSettings.inputMode == Keyboard ? "Keyboard" : "Joystick");
      glColor3f(0,1,0);
      drawString(x, vertMargin + 63, ts, "]");

      // Box around something wrong? block
      glColor3f(0,1,1);
      glBegin(GL_LINE_LOOP);
         S32 x1 = horizMargin;
         S32 x2 = gScreenInfo.getGameCanvasWidth() - horizMargin;
         S32 y1 = vertMargin + 27;
         S32 y2 = vertMargin + 90;

         glVertex2f(x1, y1);     glVertex2f(x2, y1);
         glVertex2f(x2, y2);     glVertex2f(x1, y2);
      glEnd();


      const S32 gap = 5;

      S32 ypos = showVersionBlock(120, 14, gap);

      glColor3f(1,1,1);

      S32 textsize = 16;

      drawCenteredString2Colf(ypos, textsize, false, "%s", gMainMenuUserInterface.getNeedToUpgrade() ? 
                                                           "<<Update available>>" : "<<Current version>>");
      ypos += textsize + gap;
      // This following line is a bit of a beast, but it will return a valid result at any stage of being in or out of a game.
      // If the server modifies a user name to make it unique, this will display the modified version.
      drawCenteredStringPair2Colf(ypos, textsize, true, "Nickname:", "%s (%s)", 
                  /*gClientGame && gClientGame->getGameType() && gClientGame->getGameType()->mClientList.size() ?
                        gClientGame->getGameType()->mLocalClient->name.getString() : */gClientInfo.name.c_str(), 
                        gClientInfo.authenticated ? "Verified" : "Not verified");

      ypos += textsize + gap;

      showMasterBlock(textsize, ypos, gap, false);

      ypos += textsize + gap;
      drawCenteredStringPair2Colf(ypos, textsize, true, "Input Mode:", "%s", gIniSettings.inputMode == Joystick ? "Joystick" : "Keyboard");
      ypos += textsize + gap;
      drawCenteredStringPair2Colf(ypos, textsize, true, "Curr. Joystick:", "%s", joystickTypeToString(gIniSettings.joystickType).c_str());

      ypos += textsize + gap;
      drawCenteredStringPair2Colf(ypos, textsize, true, "Autodetect Str.:", "%s", strcmp(gJoystickName,"") ? gJoystickName : "<None>");

      ypos += textsize + gap;
      ypos += textsize + gap;
      ypos += textsize + gap;
      ypos += textsize + gap;
      ypos += textsize + gap;
      ypos += textsize + gap;

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
      hpos += drawStringAndGetWidthf( hpos, ypos, textsize - 2, "Raw Controller Input [%d]: ", gSticksFound);

      for(S32 i = 0; i < MaxJoystickButtons; i++)
         if(gRawJoystickButtonInputs & (1 << i))
            hpos += drawStringAndGetWidthf( hpos, ypos, textsize - 2, "(%d)", i ) + 5;

      ypos += textsize + gap + 10;

      glColor3f(0, 1, 0);
      drawCenteredString(ypos, textsize, "Hint: If you're having joystick problems, check your controller's 'mode' button.");

      //////////
      // Draw joystick and button map
      hpos = 100;
      ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - 110;
      //S32 butts = gControllerButtonCounts[something here];

      renderDPad(Point(hpos, ypos), 25, getKeyState(BUTTON_DPAD_UP), getKeyState(BUTTON_DPAD_DOWN),
                 getKeyState(BUTTON_DPAD_LEFT), getKeyState(BUTTON_DPAD_RIGHT), "DPad", "(Menu Nav)");
      hpos += 75;

      renderDPad(Point(hpos, ypos), 25, getKeyState(STICK_1_UP), getKeyState(STICK_1_DOWN),
                 getKeyState(STICK_1_LEFT), getKeyState(STICK_1_RIGHT), "L Stick", "(Move)");
      hpos += 75;

      renderDPad(Point(hpos, ypos), 25, getKeyState(STICK_2_UP), getKeyState(STICK_2_DOWN),
                 getKeyState(STICK_2_LEFT), getKeyState(STICK_2_RIGHT), "R Stick", "(Fire)");
      hpos += 55;

      renderControllerButton(hpos, ypos, BUTTON_1, getKeyState(BUTTON_1));
      hpos += 40;
      renderControllerButton(hpos, ypos, BUTTON_2, getKeyState(BUTTON_2));
      hpos += 40;
      renderControllerButton(hpos, ypos, BUTTON_3, getKeyState(BUTTON_3));
      hpos += 40;
      renderControllerButton(hpos, ypos, BUTTON_4, getKeyState(BUTTON_4));
      hpos += 40;
      renderControllerButton(hpos, ypos, BUTTON_5, getKeyState(BUTTON_5));
      hpos += 40;
      renderControllerButton(hpos, ypos, BUTTON_6, getKeyState(BUTTON_6));
      hpos += 40;
      renderControllerButton(hpos, ypos, BUTTON_7, getKeyState(BUTTON_7));
      hpos += 40;
      renderControllerButton(hpos, ypos, BUTTON_8, getKeyState(BUTTON_8));
      hpos += 40;
      renderControllerButton(hpos, ypos, BUTTON_BACK, getKeyState(BUTTON_BACK));
      hpos += 40;
      renderControllerButton(hpos, ypos, BUTTON_START, getKeyState(BUTTON_START));
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

      ypos = showFoldersBlock(textsize, ypos, gap+2);
   }
   else if(mCurPage == 2)
   {
      S32 gap = 5;
      S32 textsize = 16;

      S32 ypos = vertMargin + 35;

      glColor3f(1,1,1);
      ypos += showNameDescrBlock(ypos, textsize, gap);

      drawCenteredStringPair2Colf(ypos, textsize, true, "Host Addr:", "%s", gBindAddress.toString());
      drawCenteredStringPair2Colf(ypos, 14, false, "Lvl Change PW:", "%s", gLevelChangePassword == "" ?
                                                                    "None - anyone can change" : gLevelChangePassword.c_str());
      ypos += textsize + gap;

      
      drawCenteredStringPair2Colf(ypos, 14, false, "Admin PW:", "%s", gAdminPassword == "" ? 
                                                                     "None - no one can get admin" : gAdminPassword.c_str());
      ypos += textsize + gap;

      drawCenteredStringPair2Colf(ypos, textsize, false, "Server PW:", "%s", gServerPassword == "" ? 
                                                                             "None needed to play" : gServerPassword.c_str());

      ypos += textsize + gap;
      ypos += textsize + gap;

      S32 x = getCenteredString2ColStartingPosf(textsize, false, "Max Players: %d", getServerMaxPlayers());
      glColor3f(1,1,1);
      x += drawStringAndGetWidthf(x, ypos, textsize, "Max Players: ");
      glColor3f(1,1,0);
      x += drawStringAndGetWidthf(x, ypos, textsize, "%d", getServerMaxPlayers());

      ypos += textsize + gap;

      drawCenteredStringPair2Colf(ypos, textsize, false, "Sim. Lag/Pkt. Loss:", "%dms /%2.0f%%", 
                                  gClientInfo.simulatedLag, gClientInfo.simulatedPacketLoss * 100);
      ypos += textsize + gap;
      ypos += textsize + gap;
      

      // Dump out names of loaded levels...
      glColor3f(1,1,1);
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


