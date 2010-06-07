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


#include "game.h"
#include "input.h"
#include "config.h"
#include "UIEditorInstructions.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UIEditor.h"
#include "gameObjectRender.h"

#include "SweptEllipsoid.h"      // For polygon triangulation

#include "../glut/glutInclude.h"

namespace Zap
{

EditorInstructionsUserInterface gEditorInstructionsUserInterface;


static Vector<Point> sample1o, sample2o, sample3o, sample4o;     // outline
static Vector<Point> sample1f, sample2f, sample3f, sample4f;     // fill

// Constructor
EditorInstructionsUserInterface::EditorInstructionsUserInterface()
{
   setMenuID(EditorInstructionsUI);
}


void EditorInstructionsUserInterface::onActivate()
{
   mCurPage = 1;

   // We really should be setting this up in the constructor, but... it doesn't seem to "stick".
   // Do it here instead, but it will get run every time we load the instructions.  Better than every 
   // cycle, for sure, but not really right.
   sample1o.clear();
   sample2o.clear();

   sample1o.push_back(Point(-70, -50));
   sample1o.push_back(Point(70, -50));
   sample1o.push_back(Point(70, 50));
   sample1o.push_back(Point(-70, 50));

   sample2o.push_back(Point(-70, -50));
   sample2o.push_back(Point(0, -10));   
   sample2o.push_back(Point(70, -50));
   sample2o.push_back(Point(70, 50));
   sample2o.push_back(Point(-70, 50));

   Triangulate::Process(sample1o, sample1f);
   Triangulate::Process(sample2o, sample2f);

}

static const S32 NUM_PAGES = 4;


const char *pageHeadersEditor[] = {
   "BASIC COMMANDS",
   "ADVANCED COMMANDS",
   "WALLS AND LINES",
   "BOT NAV ZONES",
};

extern EditorUserInterface gEditorUserInterface;

void EditorInstructionsUserInterface::render()
{
   glColor3f(1,1,1);
   drawStringf(3, 3, 25, "INSTRUCTIONS - %s", pageHeadersEditor[mCurPage - 1]);
   drawStringf(650, 3, 25, "PAGE %d/%d", mCurPage, NUM_PAGES);
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
         renderPageCommands(1);
         break;
      case 2:
         renderPageCommands(2);
         break;
      case 3:
         renderPageWalls();
         break;
      case 4:
         renderPageZones();
         break;
   }
}

struct ControlStringsEditor
{
   const char *actionString;
   const char *keyString;
};


// For page 1 of general instructions
static ControlStringsEditor gControls1[] = {
   { "Navigation", "HEADER" },
         { "Pan Map", "W/A/S/D or"},
         { " ", "Arrow Keys"},
         { "Zoom In", "E or Ctrl-Up" },
         { "Zoom Out", "C or Ctrl-Dwn" },
         { "Center Display", "Z" },
         { "Toggle script results", "Ctrl-R" },
      { "-", NULL },       // Horiz. line
         { "Cut/Copy/Paste", "Ctrl-X/C/V"},
         { "Delete Selection", "Del" },
      { "-", NULL },       // Horiz. line
         { "Undo", "Ctrl-Z" },
         { "Redo", "Ctrl-Shift-Z" },
         { NULL, NULL },      // End of col1
   { "Object Shortcuts", "HEADER" },
         { "Insert Teleport", "T" },
         { "Insert Spawn Point", "G" },
         { "Insert Repair", "B" },
         { "Insert Turret", "Y" },
         { "Insert Force Field", "F" },
         { "Insert Mine", "M" },
      { "-", NULL },       // Horiz. line
   { "Assigning Teams", "HEADER" },
         { "Set object's team", "1-9" },
         { "Set object to neutral", "0" },
         { "Set object to hostile", "Shift-0" },
      { "-", NULL },       // Horiz. line
         { "Save", "Ctrl-S" },
         { "Reload from file", "Ctrl-Shift-L" },
         { NULL, NULL },      // End of col2
      };


// For page 2 of basic instructions
static ControlStringsEditor gControls2[] = {
         { "Flip Horiz/Vertical", "H, V" },
         { "Rotate about (0,0)", "R, Shift-R" },
         { "Arbitrary rotate", "Ctrl-Shift-R" },
         { "Scale selection", "Ctrl-Shift-X" },

         { "-", NULL },       // Horiz. line
         { "Hold [Space] to suspend snapping", "" },
         { "Hold [Tab] to view a reference ship", "" },
         { NULL, NULL },      // End of col1

         { "Cycle edit mode", "Ctrl-A" },
         { NULL, NULL },      // End of col2
   };


// This has become rather ugly and inelegant.  But you shuold see UIInstructions.cpp!!!
void EditorInstructionsUserInterface::renderPageCommands(S32 page)
{
   ControlStringsEditor *controls = (page == 1) ? gControls1 : gControls2;

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

   static const Color txtColor = Color(0, 1, 1);
   static const Color keyColor = Color (1, 1, 1);           // White
   static const Color secColor = Color(1, 1, 0);
   static const Color groupHeaderColor = Color(1, 0, 0);    // Red

   glColor(secColor);
   drawString(col1, starty, 20, "Action");
   drawString(col2, starty, 20, "Control");
   drawString(col3, starty, 20, "Action");
   drawString(col4, starty, 20, "Control");

   y = starty + 28;
   for(S32 i = 0; !done; i++)
   {
      if(!controls[i].actionString)
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
      else if(!strcmp(controls[i].actionString, "-"))      // Horiz spacer
      {
         glColor3f(0.4, 0.4, 0.4);
         glBegin(GL_LINES);
            glVertex2f(actCol, y + 13);
            glVertex2f(actCol + 335, y + 13);
         glEnd();
      }
      else if(!strcmp(controls[i].keyString, "HEADER"))
      {
         glColor(groupHeaderColor);
         drawString(actCol, y, 18, controls[i].actionString);
      }
      else
      {
         glColor(txtColor);
         drawString(actCol, y, 18, controls[i].actionString);      // Textual description of function (1st arg in lists above)
         glColor(keyColor);
         drawString(contCol, y, 18, controls[i].keyString);
      }
      y += 26;
   }

   y = 470;
   glColor(secColor);
   drawCenteredString(y, 20, "These special keys are also usually active:");
   y+=40;
   glColor(txtColor);
   drawString(col1, y, 18, "Help");
   glColor(keyColor);
   drawStringf(col2, y, 18, "[%s]", keyCodeToString(keyHELP));

   glColor(txtColor);
   drawString(col3, y, 18, "Game Params Editor");
   glColor(keyColor);
   drawStringf(col4, y, 18, "[%s]", keyCodeToString(KEY_F3));

   y+=26;
   glColor(txtColor);
   drawString(col1, y, 18, "Team Editor");
   glColor(keyColor);
   drawStringf(col2, y, 18, "[%s]", keyCodeToString(KEY_F2));

   glColor(txtColor);
   drawString(col3, y, 18, "Universal Chat");
   glColor(keyColor);
   drawStringf(col4, y, 18, "[%s]", keyCodeToString(keyOUTGAMECHAT));
}


static const char *wallInstructions[] =
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


extern void constructBarrierOutlinePoints(const Vector<Point> &verts, F32 width, Vector<Point> &barrierPoints);

void EditorInstructionsUserInterface::renderPageWalls()
{
   // Draw animated creation of walls

   //drawStringf(400, 100, 25, "%d", mAnimStage);

   S32 vertOffset = 20;
   S32 textSize = 18;

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

   if(mAnimStage > 6)
   {
      Vector<Point> barPoints;
      F32 width = 25;
      //constructBarrierOutlinePoints(points, width, barPoints);  // Populates barPoints with points representing barrier

      gEditorUserInterface.renderPolylineFill(ItemBarrierMaker, points, barPoints, false, -1, 1.0, false);
   }

   glColor(mAnimStage <= 11 ? Color(1,1,0) : gEditorUserInterface.getTeamColor(-1));

   glLineWidth(3);

   glBegin(GL_LINE_STRIP);
      for(S32 i = 0; i < points.size(); i++)
         glVertex2f(points[i].x, points[i].y);
   glEnd();
   glLineWidth(gDefaultLineWidth);

   for(S32 i = 0; i < points.size(); i++)
      if(i < (points.size() - ((mAnimStage > 6) ? 0 : 1) ) && !(i == 2 && (mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)))
         gEditorUserInterface.renderVertex(SelectedItemVertex, points[i], i);
      else if(mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)
         gEditorUserInterface.renderVertex(HighlightedVertex, points[i], i);
      else
         gEditorUserInterface.renderVertex(HighlightedVertex, points[i], -1);

   // And now some written instructions
   S32 x = 50 + getStringWidth(textSize, "* ");
   S32 y = 300;
   bool done = false;

   for(S32 i = 0; !done; i++)
   {
      if(!strcmp(wallInstructions[i], ""))
         done = true;
      else
      {
         glColor3f(1, 0, 0);     // red
         drawString(50, y, textSize, "*");

         glColor3f(1, 1, 1);     // white
         drawString(x, y, textSize, wallInstructions[i]);
         y += 26;
      }
   }
}


void EditorInstructionsUserInterface::renderPageZones()
{
   // 4 static images, followed by some text
   S32 col1x = 200;
   S32 col2x = 600;

   S32 ypos = 100;
   S32 textSize = 18;
   F32 scale = .7;

   //////////

   glPushMatrix();
      glTranslatef(col1x, ypos, 0);
      glScalef(scale, scale, 1);
      renderNavMeshZone(sample1o, sample1f, findCentroid(sample1o), -1, true);
   glPopMatrix();

   glColor3f(0,1,0);      // Green
   drawStringc(col1x, ypos + 50, textSize, "Convex - Good!");

   //////////

   glPushMatrix();
      glTranslatef(col2x, ypos, 0);
      glScalef(scale, scale, 1);
      renderNavMeshZone(sample2o, sample2f, findCentroid(sample2o), -1, false);
   glPopMatrix();

   glColor3f(1,0,0);      // Red
   drawStringc(col2x, ypos + 50, textSize, "Not Convex - Bad!");
   
   //////////

   ypos += 200;

   glColor3f(0,1,0);      // Green
   drawStringc(col1x, ypos + 50, textSize, "Snapped - Good!");

   glColor3f(1,0,0);      // Red
   drawStringc(col2x, ypos + 50, textSize, "Not Snapped - Bad!");
}


void EditorInstructionsUserInterface::nextPage()
{
   mCurPage++;
   if(mCurPage > NUM_PAGES)
      mCurPage = 1;

   mAnimTimer.reset(1000);
   mAnimStage = 0;
}


void EditorInstructionsUserInterface::prevPage()
{
   if(mCurPage > 1)
      mCurPage--;
   else
      mCurPage = NUM_PAGES;

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


