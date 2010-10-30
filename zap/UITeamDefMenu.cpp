////-----------------------------------------------------------------------------------
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
#include "UITeamDefMenu.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UIEditor.h"
#include "input.h"
#include "keyCode.h"
#include "IniFile.h"
#include "config.h"
#include "gameType.h"      // For gMaxTeams

#include "../glut/glutInclude.h"
#include <string>

namespace Zap
{


// Note: Do not make any of the following team names longer than MAX_TEAM_NAME_LENGTH, which is currently 32
// Note: Make sure we have at least 9 presets below...  (instructions are wired for keys 1-9)
TeamPreset gTeamPresets[] = {
   { "Blue",        0,     0,    1 },
   { "Red",         1,     0,    0 },
   { "Yellow",      1,     1,    0 },
   { "Green",       0,     1,    0 },
   { "Pink",        1,   .45, .875 },
   { "Orange",      1,   .67,    0 },
   { "Lilac",      .79,   .5,  .96 },
   { "LightBlue",  .45, .875,    1 },
   { "Ruby",       .67,    0,    0 },
};

// Other ideas
//Team Blue 0 0 1
//Team Red 1 0 0
//Team Green 0 1 0
//Team Yellow 1 1 0
//Team Turquoise 0 1 1
//Team Pink 1 0 1
//Team Orange 1 0.5 0
//Team Black 0 0 0
//Team White 1 1 1
//Team Sapphire 0 0 0.7
//Team Ruby 0.7 0 0
//Team Emerald 0 0.7 0
//Team Lime 0.8 1 0
//Team DarkAngel 0 0.7 0.7
//Team Purple 0.7 0 0.7
//Team Peach 1 0.7 0


TeamDefUserInterface gTeamDefUserInterface;

// Constructor
TeamDefUserInterface::TeamDefUserInterface()
{
   setMenuID(TeamDefUI);
   mMenuTitle = "Configure Teams";
   mMenuSubTitle = "For quick configuration, press [ALT]1-9 to specify number of teams";
   mMenuSubTitleColor = Color(1,1,1);
}

static const U32 errorMsgDisplayTime = 4000; // 4 seconds
static const S32 fontsize = 19;
static const S32 fontgap = 12;
static const U32 yStart = UserInterface::vertMargin + 90;
static const U32 itemHeight = fontsize + 5;

void TeamDefUserInterface::onActivate()
{
   selectedIndex = 0;                     // First item selected when we begin
   mEditing = false;                      // Not editing anything by default
   gEditorUserInterface.mOldTeams = gEditorUserInterface.mTeams;

   // Display an intitial message to users
   errorMsgTimer.reset(errorMsgDisplayTime);
   errorMsg = "";
   glutSetCursor(GLUT_CURSOR_NONE);
}

void TeamDefUserInterface::idle(U32 timeDelta)
{
   if(errorMsgTimer.update(timeDelta))
      errorMsg = "";

   LineEditor::updateCursorBlink(timeDelta);
}


extern Color gNeutralTeamColor;
extern Color gHostileTeamColor;

// TODO: Clean this up a bit...  this menu was two-cols before, and some of that garbage is still here...
void TeamDefUserInterface::render()
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   glColor3f(1, 1, 1);
   drawCenteredString(vertMargin, 30, mMenuTitle);
   drawCenteredString(vertMargin + 35, 18, mMenuSubTitle);

   glColor3f(0, 1, 0);
   drawCenteredString(canvasHeight - vertMargin - 115, 16, "[1] - [9] selects a team preset for current slot");
   drawCenteredString(canvasHeight - vertMargin - 92,  16, "[Enter] edits team name");
   drawCenteredString(canvasHeight - vertMargin - 69,  16, "[R] [G] [B] to change preset color (with or without [Shift])");
   drawCenteredString(canvasHeight - vertMargin - 46,  16, "[Ins] or [+] to insert team | [Del] or [-] to remove selected team");

   glColor3f(1, 1, 1);
   drawCenteredString(canvasHeight - vertMargin - 20, 18, "Arrow Keys to choose | ESC to exit");

   S32 size = gEditorUserInterface.mTeams.size();

   if(selectedIndex >= size)
      selectedIndex = 0;


   // Draw the fixed teams
   glColor(gNeutralTeamColor);
   drawCenteredStringf(yStart, fontsize, "Neutral Team (can't change)");
   glColor(gHostileTeamColor);
   drawCenteredStringf(yStart + fontsize + fontgap, fontsize, "Hostile Team (can't change)");

   for(S32 j = 0; j < size; j++)
   {
      S32 i = j + 2;    // Take account of the two fixed teams (neutral & hostile)

      U32 y = yStart + i * (fontsize + fontgap);

      if(selectedIndex == j)       // Highlight selected item
         for(S32 i = 1; i >= 0; i--)
         {
            glColor(i ? Color(0,0,0.4) : Color(0,0,1));     // Fill : Outline
            glBegin(i ? GL_POLYGON : GL_LINES);
               glVertex2f(0, y - 2);
               glVertex2f(canvasWidth, y - 2);
               glVertex2f(canvasWidth, y + itemHeight + 2);    // + 2 to allow room for cursor when editing team name
               glVertex2f(0, y + itemHeight + 2);
            glEnd();
         }

      if(j < gEditorUserInterface.mTeams.size())
      {
         char numstr[10];
         dSprintf(numstr, sizeof(numstr), "Team %d: ", j+1);

         char namestr[MAX_NAME_LEN + 20];    // Added a little extra, just to cover any contingency...
         dSprintf(namestr, sizeof(namestr), "%s%s", numstr, gEditorUserInterface.mTeams[j].getName().getString());

         char colorstr[16];                  // "(100, 100, 100)" + 1 for null
         dSprintf(colorstr, sizeof(colorstr), "(%d, %d, %d)", S32(gEditorUserInterface.mTeams[j].color.r * 100),
                                                              S32(gEditorUserInterface.mTeams[j].color.g * 100),
                                                              S32(gEditorUserInterface.mTeams[j].color.b * 100));
         static const char *nameColorStr = "%s  %s";

         // Draw item text
         glColor(gEditorUserInterface.mTeams[j].color);
         drawCenteredStringf(y, fontsize, nameColorStr, namestr, colorstr);

         // Draw cursor if we're editing
         if(mEditing && j == selectedIndex)
         {
            S32 x = getCenteredStringStartingPosf(fontsize, nameColorStr, namestr, colorstr) + getStringWidth(fontsize, numstr);
            gEditorUserInterface.mTeams[j].getLineEditor()->drawCursor(x, y, fontsize);
         }
      }
   }

   // Draw the help string
   glColor3f(0, 1, 0);

   if(errorMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (errorMsgTimer.getCurrent() < 1000)
         alpha = (F32) errorMsgTimer.getCurrent() / 1000;

      glEnable(GL_BLEND); 
      glColor4f(1, 0, 0, alpha);
      drawCenteredString(canvasHeight - vertMargin - 141, fontsize, errorMsg.c_str());
      glDisable(GL_BLEND);
   }
}

// Run this as we're exiting the menu
void TeamDefUserInterface::onEscape()
{
   // Make sure there is at least one team left...
   gEditorUserInterface.makeSureThereIsAtLeastOneTeam();
   gEditorUserInterface.teamsHaveChanged();

   UserInterface::reactivatePrevUI();
}


class Team;
string origName;
extern bool isPrintable(char c);

void TeamDefUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_ENTER)
   {
      mEditing = !mEditing;
      if(mEditing)
         origName = gEditorUserInterface.mTeams[selectedIndex].getName().getString();
   }
   else if(mEditing)                // Editing, send keystroke to editor
   {
      if(keyCode == KEY_ESCAPE)     // Stop editing, and restore the original value
      {
         gEditorUserInterface.mTeams[selectedIndex].setName(origName.c_str());
         mEditing = false;
      }
      else if(keyCode == KEY_BACKSPACE || keyCode == KEY_DELETE)
         gEditorUserInterface.mTeams[selectedIndex].getLineEditor()->handleBackspace(keyCode);
      else if(isPrintable(ascii))
         gEditorUserInterface.mTeams[selectedIndex].getLineEditor()->addChar(ascii);
   }
   else if(ascii >= '1' && ascii <= '9')        // Keys 1-9 --> use preset
   {
      if(getKeyState(KEY_ALT))      // Replace all teams with # of teams based on presets
      {
         U32 count = (ascii - '0');
         gEditorUserInterface.mTeams.clear();
         for(U32 i = 0; i < count; i++)
         {
            TeamEditor t;
            t.setName(gTeamPresets[i].name);
            t.color.set(gTeamPresets[i].r, gTeamPresets[i].g, gTeamPresets[i].b);
            gEditorUserInterface.mTeams.push_back(t);
         }
      }
      else                          // Replace selection with preset of number pressed
      {
         U32 indx = (ascii - '1');
         gEditorUserInterface.mTeams[selectedIndex].setName(gTeamPresets[indx].name);
         gEditorUserInterface.mTeams[selectedIndex].color.set(gTeamPresets[indx].r, gTeamPresets[indx].g, gTeamPresets[indx].b);
      }
   }

   else if(keyCode == KEY_DELETE || keyCode == KEY_MINUS)            // Del or Minus - Delete current team
   {
      gEditorUserInterface.mTeams.erase(selectedIndex);
      if(selectedIndex >= gEditorUserInterface.mTeams.size())
         selectedIndex = gEditorUserInterface.mTeams.size() - 1;
   }
  
   else if(keyCode == KEY_INSERT || keyCode == KEY_EQUALS)           // Ins or Plus (equals) - Add new item
   {
      S32 maxTeams = GameType::gMaxTeams;    // A bit pedantic, perhaps, but using this fixes an odd link error in Linux
      if(gEditorUserInterface.mTeams.size() >= maxTeams)
      {
         errorMsgTimer.reset(errorMsgDisplayTime);
         errorMsg = "Too many teams for this interface";
         return;
      }
      gEditorUserInterface.mTeams.insert(selectedIndex);

      gEditorUserInterface.mTeams[selectedIndex].setName(gTeamPresets[selectedIndex].name);
      gEditorUserInterface.mTeams[selectedIndex].color.set(gTeamPresets[selectedIndex].r, gTeamPresets[selectedIndex].g, gTeamPresets[selectedIndex].b);

      if(selectedIndex < 0)      // It can happen with too many deletes
         selectedIndex = 0;
   }

   else if(keyCode == KEY_R)
      if(getKeyState(KEY_SHIFT))
      {
         gEditorUserInterface.mTeams[selectedIndex].color.r -= .01;
         if(gEditorUserInterface.mTeams[selectedIndex].color.r < 0)
            gEditorUserInterface.mTeams[selectedIndex].color.r = 0;
      }
      else
      {
         gEditorUserInterface.mTeams[selectedIndex].color.r += .01;
         if (gEditorUserInterface.mTeams[selectedIndex].color.r > 1)
            gEditorUserInterface.mTeams[selectedIndex].color.r = 1;
      }
   else if(keyCode == KEY_G)
      if(getKeyState(KEY_SHIFT))
      {
         gEditorUserInterface.mTeams[selectedIndex].color.g -= .01;
         if(gEditorUserInterface.mTeams[selectedIndex].color.g < 0)
            gEditorUserInterface.mTeams[selectedIndex].color.g = 0;
      }
      else
      {
         gEditorUserInterface.mTeams[selectedIndex].color.g += .01;
         if (gEditorUserInterface.mTeams[selectedIndex].color.g > 1)
            gEditorUserInterface.mTeams[selectedIndex].color.g = 1;
      }
   else if(keyCode == KEY_B)
      if(getKeyState(KEY_SHIFT))
      {
         gEditorUserInterface.mTeams[selectedIndex].color.b -= .01;
         if(gEditorUserInterface.mTeams[selectedIndex].color.b < 0)
            gEditorUserInterface.mTeams[selectedIndex].color.b = 0;
      }
      else
      {
         gEditorUserInterface.mTeams[selectedIndex].color.b += .01;
         if (gEditorUserInterface.mTeams[selectedIndex].color.b > 1)
            gEditorUserInterface.mTeams[selectedIndex].color.b = 1;
      }

   else if(keyCode == KEY_ESCAPE || keyCode == BUTTON_BACK)       // Quit
   {
      UserInterface::playBoop();
      onEscape();
   }
   else if(keyCode == KEY_UP || keyCode == BUTTON_DPAD_UP)        // Prev item
   {
      selectedIndex--;
      if(selectedIndex < 0)
         selectedIndex = gEditorUserInterface.mTeams.size() - 1;
      UserInterface::playBoop();
      glutSetCursor(GLUT_CURSOR_NONE);

   }
   else if(keyCode == KEY_DOWN || keyCode == BUTTON_DPAD_DOWN)    // Next item
   {
      selectedIndex++;
      if(selectedIndex >= gEditorUserInterface.mTeams.size())
         selectedIndex = 0;
      UserInterface::playBoop();
      glutSetCursor(GLUT_CURSOR_NONE);
   }
   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
   {
      gChatInterface.activate();
      UserInterface::playBoop();
   }
}


void TeamDefUserInterface::onMouseMoved(S32 x, S32 y)
{
   glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);            // Show cursor when user moves mouse

   Point mousePos = convertWindowToCanvasCoord(Point(x, y));
   
   S32 teams = gEditorUserInterface.mTeams.size();

   selectedIndex = (S32)((mousePos.y - yStart + 6) / (fontsize + fontgap)) - 2; 

   if(selectedIndex >= teams)
      selectedIndex = teams - 1;

   if(selectedIndex < 0)
      selectedIndex = 0;
}


};

