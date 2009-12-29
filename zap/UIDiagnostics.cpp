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

#include "tnl.h"

#include "../glut/glutInclude.h"
#include <stdio.h>

namespace Zap
{

extern string gHostName;
extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;
extern F32 gSimulatedPacketLoss;
extern const char *gServerPassword;
extern const char *gAdminPassword;
extern const char *gLevelChangePassword;
extern U32 gSimulatedLag;
extern S32 gMaxPlayers;
extern U32 gRawJoystickButtonInputs;
extern void renderDPad(Point center, F32 radius, bool upStat, bool downStat, bool leftStat, bool rightStat, const char *msg1, const char *msg2);
extern Address gBindAddress;

DiagnosticUserInterface gDiagnosticInterface;

// Constructor
DiagnosticUserInterface::DiagnosticUserInterface()
{
   setMenuID(DiagnosticsScreenUI);
}

void DiagnosticUserInterface::onActivate()
{
   mActive = true;
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
   if (keyCode == KEY_ESCAPE || keyCode == keyDIAG)
      quit();                          // Quit the interface
}

extern ClientGame *gClientGame;
extern const S32 gJoystickNameLength;
extern char gJoystickName[gJoystickNameLength];
extern ControllerTypeType gAutoDetectedJoystickType;
extern U32 gSticksFound;
extern Color gMasterServerBlue;
extern string gLevelDir;
extern Vector<StringTableEntry> gLevelList;

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
   glColor3f(1,1,1);
   drawCenteredString(vertMargin, 30, "Diagnostic Screen");
   drawCenteredString(vertMargin + 35, 18, "Is something wrong?");


   S32 vertFooterPos = canvasHeight - vertMargin - 20;
   drawCenteredStringf(vertFooterPos, 18, "Press ESC or %s to exit", keyCodeToString(keyDIAG));

   const S32 ts = 14;
   S32 x;
   x = getCenteredStringStartingPosf(ts, "Can't control your ship? Check your input mode (Options>Primary Input) [currently %s]", gIniSettings.inputMode == Keyboard ? "Keyboard" : "Joystick");
   glColor3f(0,1,0);
   drawString(x, vertMargin + 60, ts, "Can't control your ship? Check your input mode (Options>Primary Input) [currently ");
   x += getStringWidth(ts, "Can't control your ship? Check your input mode (Options>Primary Input) [currently ");
   glColor3f(1,0,0);
   drawString(x, vertMargin + 60, ts, gIniSettings.inputMode == Keyboard ? "Keyboard" : "Joystick");
   x += getStringWidth(ts, gIniSettings.inputMode == Keyboard ? "Keyboard" : "Joystick");
   glColor3f(0,1,0);
   drawString(x, vertMargin + 60, ts, "]");


   glColor3f(1,1,1);

   //////////
   // Display a bunch of settings using our new 2Col rendering functions (hooray!)

   S32 vpos = 100;
   S32 textsize = 14;
   const S32 gap = 5;

   x = getCenteredStringStartingPosf(textsize, "M/C Ver: %d | C/S Ver: %d | Build: %d | CPU: %s | OS: %s | Cmplr: %s",
         MASTER_PROTOCOL_VERSION, CS_PROTOCOL_VERSION, BUILD_VERSION, TNL_CPU_STRING, TNL_OS_STRING, TNL_COMPILER_STRING);

      glColor3f(1,1,1);
      x += drawStringAndGetWidthf(x, vpos, textsize, "M/C Ver: ");
      glColor3f(1,1,0);
      x += drawStringAndGetWidthf(x, vpos, textsize, "%d", MASTER_PROTOCOL_VERSION);

      glColor3f(1,1,1);
      x += drawStringAndGetWidthf(x, vpos, textsize, " | C/S Ver: ");
      glColor3f(1,1,0);
      x += drawStringAndGetWidthf(x, vpos, textsize, "%d", CS_PROTOCOL_VERSION);

      glColor3f(1,1,1);
      x += drawStringAndGetWidthf(x, vpos, textsize, " | Build: ");
      glColor3f(1,1,0);
      x += drawStringAndGetWidthf(x, vpos, textsize, "%d", BUILD_VERSION);

      glColor3f(1,1,1);
      x += drawStringAndGetWidthf(x, vpos, textsize, " | CPU: ");
      glColor3f(1,1,0);
      x += drawStringAndGetWidthf(x, vpos, textsize, "%s", TNL_CPU_STRING);

      glColor3f(1,1,1);
      x += drawStringAndGetWidthf(x, vpos, textsize, " | OS: ");
      glColor3f(1,1,0);
      x += drawStringAndGetWidthf(x, vpos, textsize, "%s", TNL_OS_STRING);

      glColor3f(1,1,1);
      x += drawStringAndGetWidthf(x, vpos, textsize, " | Cmplr: ");
      glColor3f(1,1,0);
      x += drawStringAndGetWidthf(x, vpos, textsize, "%s", TNL_COMPILER_STRING);

   glColor3f(1,1,1);

   vpos += textsize + gap;

   textsize = 16;
   // Can have a left-value here
   drawCenteredString2Colf(vpos, textsize, false, "%s", gMainMenuUserInterface.getNeedToUpgrade() ? "<<Update available>>" : "<<Current version>>");

   // This following line is a bit of a beast, but it will return a valid result at any stage of being in or out of a game.
   // If the server modifies a user name to make it unique, this will display the modified version.
   drawCenteredStringPair2Colf(vpos, textsize, true, "Nickname:", "%s", gClientGame && gClientGame->getGameType() && gClientGame->getGameType()->mClientList.size() ?
                                                                        gClientGame->getGameType()->mLocalClient->name.getString() : gNameEntryUserInterface.getText());


   vpos += textsize + gap;
   drawCenteredStringPair2Colf(vpos, textsize, false, "Host Name:", "%s", gHostName.c_str());
   drawCenteredStringPair2Colf(vpos, textsize, true, "Master Srvr Addr:", "%s", gMasterAddress.toString());
   vpos += textsize + gap;
   if(gClientGame && gClientGame->getConnectionToMaster() && gClientGame->getConnectionToMaster()->isEstablished())
   {
      glColor(gMasterServerBlue);
      drawCenteredString2Colf(vpos, 14, true, "Connected to [%s]", gClientGame->getConnectionToMaster()->getMasterName().c_str() );
   }
   else
   {
      glColor3f(1, 0, 0);
      drawCenteredString2Col(vpos, 14, true, "Could not establish connection with Master Server" );
   }
   drawCenteredStringPair2Colf(vpos, textsize, false, "Host Addr:", "%s", gBindAddress.toString());
   vpos += textsize + gap;

   glColor3f(1,1,1);
   drawCenteredStringPair2Colf(vpos, textsize, false, "Admin/Level Ch. PW:", "%s/%s", gAdminPassword, gLevelChangePassword);
   vpos += textsize + gap;

   glColor3f(0,1,1);
   drawCenteredStringPair2Colf(vpos, textsize, true, "Input Mode:", "%s", gIniSettings.inputMode == Joystick ? "Joystick" : "Keyboard");


   x = getCenteredString2ColStartingPosf(textsize, false, "Max Players: %d | Server PW: %s", gMaxPlayers, gServerPassword);

      glColor3f(1,1,1);
      x += drawStringAndGetWidthf(x, vpos, textsize, "Max Players: ");
      glColor3f(1,1,0);
      x += drawStringAndGetWidthf(x, vpos, textsize, "%d", gMaxPlayers);

      glColor3f(1,1,1);
      x += drawStringAndGetWidthf(x, vpos, textsize, " | Server PW: ");
      glColor3f(1,1,0);
      x += drawStringAndGetWidthf(x, vpos, textsize, "%s", gServerPassword);

   glColor3f(1,1,1);

   vpos += textsize + gap;
   drawCenteredStringPair2Colf(vpos, textsize, true, "Curr. Joystick:", "%s", joystickTypeToString(gIniSettings.joystickType).c_str());
   vpos += textsize + gap;

   drawCenteredStringPair2Colf(vpos, textsize, true, "Autodetect Str.:", "%s", strcmp(gJoystickName,"") ? gJoystickName : "<None>");
   drawCenteredStringPair2Colf(vpos, textsize, false, "Sim. Lag/Pkt. Loss:", "%dms/%2.0f%%", gSimulatedLag, gSimulatedPacketLoss * 100);
   vpos += textsize + gap;
   vpos += textsize + gap;

   // Dump out names of loaded levels...
   glColor3f(1,1,1);
   string allLevels = "Levels: [" + gLevelDir + "] ";
   for(S32 i = 0; i < gLevelList.size(); i++)
      allLevels += string(gLevelList[i].getString()) + "; ";

   U32 i, j, k;
   i = j = k = 0;
   for(j = 0; j < 3 && i < allLevels.length(); j++)
   {
      for(; getStringWidth(textsize - 6, allLevels.substr(k, i - k).c_str()) < gScreenWidth - 2 * horizMargin && i < allLevels.length(); i++)   // first arg empty
      {
         ;     // Do nothing...
      }

      drawString(horizMargin, vpos, textsize - 6, allLevels.substr(k, i - k).c_str());
      k = i;
      vpos += textsize + gap - 5;
   }

   vpos += (textsize + gap) * (3 - j);

  glColor3f(1, 1, 1);
  U32 hpos = horizMargin;

   // Key states
   glColor3f(1, 1, 0);
   drawString( hpos, vpos, textsize, "Keys down: ");
   hpos += getStringWidth(textsize, "Keys down: ");

  for (U32 i = 0; i < MAX_KEYS; i++)
     if(getKeyState((KeyCode) i))
        hpos += drawStringAndGetWidthf( hpos, vpos, textsize - 2, "[%s]", keyCodeToString((KeyCode) i) ) + 5;

   glColor3f(1, 0, 1);
   vpos += textsize + gap;
   hpos = horizMargin;
   hpos += drawStringAndGetWidthf( hpos, vpos, textsize - 2, "Raw Controller Input [%d]: ", gSticksFound);

   for(S32 i = 0; i < MaxJoystickButtons; i++)
      if(gRawJoystickButtonInputs & (1 << i))
         hpos += drawStringAndGetWidthf( hpos, vpos, textsize - 2, "(%d)", i ) + 5;

   vpos += textsize + gap + 10;

   glColor3f(0, 1, 0);
   drawCenteredString(vpos, textsize, "Hint: If you're having joystick problems, check your controller's 'mode' button.");

   //////////
   // Draw joystick and button map
   hpos = 100;
   vpos = gScreenHeight - vertMargin - 110;
   //S32 butts = gControllerButtonCounts[something here];

   renderDPad(Point(hpos, vpos), 25, getKeyState(BUTTON_DPAD_UP), getKeyState(BUTTON_DPAD_DOWN),
              getKeyState(BUTTON_DPAD_LEFT), getKeyState(BUTTON_DPAD_RIGHT), "DPad", "(Menu Nav)");
   hpos += 75;

   renderDPad(Point(hpos, vpos), 25, getKeyState(STICK_1_UP), getKeyState(STICK_1_DOWN),
              getKeyState(STICK_1_LEFT), getKeyState(STICK_1_RIGHT), "L Stick", "(Move)");
   hpos += 75;

   renderDPad(Point(hpos, vpos), 25, getKeyState(STICK_2_UP), getKeyState(STICK_2_DOWN),
              getKeyState(STICK_2_LEFT), getKeyState(STICK_2_RIGHT), "R Stick", "(Fire)");
   hpos += 55;

   renderControllerButton(hpos, vpos, BUTTON_1, getKeyState(BUTTON_1));
   hpos += 40;
   renderControllerButton(hpos, vpos, BUTTON_2, getKeyState(BUTTON_2));
   hpos += 40;
   renderControllerButton(hpos, vpos, BUTTON_3, getKeyState(BUTTON_3));
   hpos += 40;
   renderControllerButton(hpos, vpos, BUTTON_4, getKeyState(BUTTON_4));
   hpos += 40;
   renderControllerButton(hpos, vpos, BUTTON_5, getKeyState(BUTTON_5));
   hpos += 40;
   renderControllerButton(hpos, vpos, BUTTON_6, getKeyState(BUTTON_6));
   hpos += 40;
   renderControllerButton(hpos, vpos, BUTTON_7, getKeyState(BUTTON_7));
   hpos += 40;
   renderControllerButton(hpos, vpos, BUTTON_8, getKeyState(BUTTON_8));
   hpos += 40;
   renderControllerButton(hpos, vpos, BUTTON_BACK, getKeyState(BUTTON_BACK));
   hpos += 40;
   renderControllerButton(hpos, vpos, BUTTON_START, getKeyState(BUTTON_START));
}

};

