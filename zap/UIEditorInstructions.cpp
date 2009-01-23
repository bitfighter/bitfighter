//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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


#include "game.h"
#include "input.h"
#include "config.h"
#include "UIEditorInstructions.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UIEditor.h"
#include "../glut/glutInclude.h"

namespace Zap
{

EditorInstructionsUserInterface gEditorInstructionsUserInterface;
extern void glColor(Color c, float alpha = 1);

// Constructor
EditorInstructionsUserInterface::EditorInstructionsUserInterface()
{
   setMenuID(EditorInstructionsUI);
}

void EditorInstructionsUserInterface::onActivate()
{
   mCurPage = 1;
}

enum {
   NumPages = 2,
};


const char *pageHeadersEditor[] = {
   "EDITOR KEYS",
   "MAKING WALLS",
};

extern EditorUserInterface gEditorUserInterface;

void EditorInstructionsUserInterface::render()
{
   glColor3f(1,1,1);
   drawStringf(3, 3, 25, "INSTRUCTIONS - %s", pageHeadersEditor[mCurPage - 1]);
   drawStringf(650, 3, 25, "PAGE %d/%d", mCurPage, NumPages);
   drawCenteredString(571, 20, "LEFT - previous page  RIGHT, SPACE - next page  ESC exits");
   glColor3f(0.7, 0.7, 0.7);

   glBegin(GL_LINES);
      glVertex2f(0, 31);
      glVertex2f(800, 31);
      glVertex2f(0, 569);
      glVertex2f(800, 569);
   glEnd();

   switch(mCurPage)
   {
      case 1:
         renderPage1();
         break;
      case 2:
         renderPage2();
         break;
   }
}

struct ControlStringsEditor
{
   const char *actionString;
   const char *keyString;
};

static ControlStringsEditor gControls[] = {
         { "Pan Map", "W/A/S/D or"},
         { " ", "Arrow Keys"},
         { "Zoom In", "E or Ctrl-Up" },
         { "Zoom Out", "C or Ctrl-Dwn" },
         { "Center Display", "Z" },
         { "Edit text TextItem", "Ctrl-T" },
         { "-", NULL },       // Horiz. line
         { "Cut/Copy/Paste", "Ctrl-X/C/V"},
         { "Flip Horiz/Vertical", "F/V" },
         { "Rotate Sel. about (0,0)", "R, Shift-R" },
         { "Delete Selection", "Del" },
         { "-", NULL },       // Horiz. line
         { "Hold [Space] to suspend snapping", "" },
         { "Hold [Tab] to view a reference ship", "" },
         { NULL, NULL },      // End of col1
         { "Insert Teleport", "T" },
         { "Insert Spawn Point", "G" },
         { "Insert Repair", "B" },
         { "Insert Turret", "Y" },
         { "Insert Force Field", "H" },
         { "Insert Mine", "M" },
         { "Set object's team", "1-9" },
         { "Set object to neutral", "0" },
         { "Set object to hostile", "Shift-0" },
         { "-", NULL },       // Horiz. line
         { "Save", "Ctrl-S" },
         { "Reload from file", "Ctrl-Shift-L" },
         { "Undo", "Ctrl-Z" },
         { "Redo", "Ctrl-Shift-Z" },
         { NULL, NULL },      // End of col2
      };

// This has become rather ugly and inelegant.  But you shuold see UIInstructions.cpp!!!
void EditorInstructionsUserInterface::renderPage1()
{
   S32 starty = 50;
   S32 y;
   S32 col1 = horizMargin;
   S32 col2 = horizMargin + S32(canvasWidth * 0.25) + 45;     // +45 to make a little more room for Action column
   S32 col3 = horizMargin + S32(canvasWidth * 0.5);
   S32 col4 = horizMargin + S32(canvasWidth * 0.75) + 45;
   S32 actCol = col1;      // Action column
   S32 contCol = col2;     // Control column
   bool firstCol = true;
   bool done = false;

   glBegin(GL_LINES);
      glVertex2f(col1, starty + 26);
      glVertex2f(750, starty + 26);
   glEnd();

   Color txtColor = Color(0, 1, 1);
   Color keyColor = Color (1, 1, 1);
   Color secColor = Color(1, 1, 0);

   glColor(secColor);
   drawString(col1, starty, 20, "Action");
   drawString(col2, starty, 20, "Control");
   drawString(col3, starty, 20, "Action");
   drawString(col4, starty, 20, "Control");

   y = starty + 28;
   for(S32 i = 0; !done; i++)
   {
      if(!gControls[i].actionString)
      {
         if(!firstCol)
            done = true;
         else
         {  // Start second column
            firstCol = false;
            y = starty + 2;
            actCol = col3;
            contCol = col4;

            glColor(secColor);
         }
      }
      else if(!strcmp(gControls[i].actionString, "-"))      // Horiz spacer
      {
         glColor3f(0.4, 0.4, 0.4);
         glBegin(GL_LINES);
            glVertex2f(actCol, y + 13);
            glVertex2f(actCol + 335, y + 13);
         glEnd();
      }
      else
      {
         glColor(txtColor);
         drawString(actCol, y, 18, gControls[i].actionString);      // Textual description of function (1st arg in lists above)
         glColor(keyColor);
         drawString(contCol, y, 18, gControls[i].keyString);
      }
      y += 26;
   }

   glColor(secColor);
   drawCenteredString(y, 20, "These special keys are also usually active:");
   y+=40;
   glColor(txtColor);
   drawString(col1, y, 18, "Help");
   glColor(keyColor);
   drawStringf(col2, y, 18, "[%s]", keyCodeToString(keyHELP));

   glColor(txtColor);
   drawString(col3, y, 18, "Team Editor");
   glColor(keyColor);
   drawStringf(col4, y, 18, "[%s]", keyCodeToString(keyDIAG));

   y+=26;
   glColor(txtColor);
   drawString(col1, y, 18, "Game Params Editor");
   glColor(keyColor);
   drawStringf(col2, y, 18, "[%s]", keyCodeToString(keyDIAG));
   glColor(txtColor);
   drawString(col3, y, 18, "Universal Chat");
   glColor(keyColor);
   drawStringf(col4, y, 18, "[%s]", keyCodeToString(keyOUTGAMECHAT));
}

static const char *page2Strings[] =
{
   "Create walls with right mouse button",
   "Finish wall by left-clicking",
   "Drag and drop individual vertices or an entire wall",
   "Split wall at selected vertex with [/]",
   "Join contiguous wall segments with [J]",
   "Toggle display of non-wall objects with [Ctrl-A]",
   "Change wall thickness with [+] & [-] (use [Shift] for smaller changes)",
   ""    // Last item must be ""
};

void EditorInstructionsUserInterface::renderPage2()
{
   // Draw animated creation of walls

   //drawStringf(400, 100, 25, "%d", mAnimStage);

   S32 vertOffset = 20;

   Vector<Point> points;
   points.push_back(Point(150, 100 + vertOffset));
   points.push_back(Point(220, 190 + vertOffset));
   if(mAnimStage > 0 && mAnimStage < 10)
      points.push_back(Point(350, 80 + vertOffset));
   else if(mAnimStage == 10)
      points.push_back(Point(350, 150 + vertOffset));
   else if(mAnimStage >= 11)
      points.push_back(Point(350, 200 + vertOffset));
   if(mAnimStage > 1)
      points.push_back(Point(470, 140 + vertOffset));
   if(mAnimStage > 2)
      points.push_back(Point(550, 120 + vertOffset));
   if(mAnimStage == 4)
      points.push_back(Point(650, 100 + vertOffset));
   else if(mAnimStage == 5)
      points.push_back(Point(690, 130 + vertOffset));
   else if(mAnimStage >= 6)
      points.push_back(Point(650, 170 + vertOffset));

   if(mAnimStage <= 6)      // Constructing wall, use thin yellow line
   {
      glLineWidth(3);
      glColor3f(1, 1, 0);    // yellow

      glBegin(GL_LINE_STRIP);
         for(S32 i = 0; i < points.size(); i++)
            glVertex2f(points[i].x, points[i].y);
      glEnd();
      glLineWidth(gDefaultLineWidth);
   }
   else
      gEditorUserInterface.renderBarrier(points, false, 25, true);

   for(S32 i = 0; i < points.size(); i++)
      if(i < (points.size() - ((mAnimStage > 6) ? 0 : 1) ) && !(i == 2 && (mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)))
         gEditorUserInterface.renderVertex(SelectedItemVertex, points[i], i);
      else if(mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)
         gEditorUserInterface.renderVertex(HighlightedVertex, points[i], i);
      else
         gEditorUserInterface.renderVertex(HighlightedVertex, points[i], -1);

   // And now some written instructions

   S32 x = 50 + getStringWidth(18, "* ");
   S32 y = 300;
   bool done = false;

   for(S32 i = 0; !done; i++)
   {
      if(!strcmp(page2Strings[i], ""))
         done = true;
      else
      {
         glColor3f(1, 0, 0);     // red
         drawString(50, y, 18, "*");

         glColor3f(1, 1, 1);     // white
         drawString(x, y, 18, page2Strings[i]);
         y += 26;
      }
   }
}

void EditorInstructionsUserInterface::nextPage()
{
   mCurPage++;
   if(mCurPage > NumPages)
      mCurPage = 1;

   mAnimTimer.reset(1000);
   mAnimStage = 0;
}

void EditorInstructionsUserInterface::prevPage()
{
   if(mCurPage > 1)
      mCurPage--;
   else
      mCurPage = NumPages;

   mAnimTimer.reset(1000);
   mAnimStage = 0;
}

void EditorInstructionsUserInterface::idle(U32 timeDelta)
{


   if(mAnimTimer.update(timeDelta))
   {
      mAnimTimer.reset(1000);
      mAnimStage++;
      if(mAnimStage >= 18)
         mAnimStage = 0;
   }
}

void EditorInstructionsUserInterface::exitInstructions()
{
   UserInterface::playBoop();
   reactivatePrevUI();      //gEditorUserInterface, probably
}

void EditorInstructionsUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_LEFT || keyCode == BUTTON_DPAD_LEFT || keyCode == BUTTON_DPAD_UP || keyCode == KEY_UP)
   {
      UserInterface::playBoop();
      prevPage();
   }
   else if(keyCode == KEY_RIGHT || keyCode == KEY_SPACE || keyCode == BUTTON_DPAD_RIGHT ||
           keyCode == BUTTON_DPAD_DOWN || keyCode == KEY_ENTER || keyCode == KEY_DOWN)
   {
      UserInterface::playBoop();
      nextPage();
   }
   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
      gChatInterface.activate();
//   else if(keyCode == keyDIAG)            // Turn on Diagnostic overlay
//      gDiagnosticInterface.activate();
   else if(keyCode == keyHELP || keyCode == KEY_ESCAPE  || keyCode == BUTTON_BACK)
      exitInstructions();
}

};

