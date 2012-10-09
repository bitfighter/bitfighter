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


#include "ClientGame.h"    // For usage with getGame()
#include "input.h"
#include "config.h"
#include "barrier.h"            
#include "UIAbstractInstructions.h"
#include "UIEditorInstructions.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UIEditor.h"
#include "gameObjectRender.h"
#include "GeomUtils.h"      // For polygon triangulation
#include "ScreenInfo.h"

#include "OpenglUtils.h"

#include <math.h>

namespace Zap
{

static Vector<Point> sample1o, sample2o, sample3o, sample4o, sample5o;     // outline
static Vector<Point> sample1f, sample2f, sample3f, sample4f, sample5f;     // fill
static Border border34;

// Constructor
EditorInstructionsUserInterface::EditorInstructionsUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(EditorInstructionsUI);
}


void EditorInstructionsUserInterface::onActivate()
{
   mCurPage = 1;

   // We really should be setting this up in the constructor, but... it doesn't seem to "stick".
   // Do it here instead, but it will get run every time we load the instructions.  Better than every 
   // draw cycle, for sure, but not really right.
   sample1o.clear();
   sample1o.push_back(Point(-70, -50));
   sample1o.push_back(Point(70, -50));
   sample1o.push_back(Point(70, 50));
   sample1o.push_back(Point(-70, 50));

   sample2o.clear();
   sample2o.push_back(Point(-70, -50));
   sample2o.push_back(Point(0, -10));   
   sample2o.push_back(Point(70, -50));
   sample2o.push_back(Point(70, 50));
   sample2o.push_back(Point(-70, 50));

   sample3o.clear();
   sample3o.push_back(Point(-100, -50));
   sample3o.push_back(Point(0, -50));
   sample3o.push_back(Point(0, 50));
   sample3o.push_back(Point(-100, 50));

   sample4o.clear();
   sample4o.push_back(Point(0, -50));
   sample4o.push_back(Point(100, -50));
   sample4o.push_back(Point(100, 50));
   sample4o.push_back(Point(0, 50));

   sample5o.clear();
   sample5o.push_back(Point(-5, -50));
   sample5o.push_back(Point(100, -50));
   sample5o.push_back(Point(100, 50));
   sample5o.push_back(Point(5, 50));

   // Border between 3 and 4
   border34.borderStart.set(0, -50);
   border34.borderEnd.set(0, 50);

   Triangulate::Process(sample1o, sample1f);
   Triangulate::Process(sample2o, sample2f);
   Triangulate::Process(sample3o, sample3f);
   Triangulate::Process(sample4o, sample4f);
   Triangulate::Process(sample5o, sample5f);
}


const char *pageHeadersEditor[] = {
   "BASIC COMMANDS",
   "ADVANCED COMMANDS",
   "WALLS AND LINES",
   "SCRIPTING CONSOLE",
   "PLUGINS"
};


static ControlStringsEditor consoleCommands[] = {
   { "Coming soon...", "Coming soon..." },
   { "", "" },      // End of list
};


S32 EditorInstructionsUserInterface::getPageCount()
{
   return 4 + (getGame()->getSettings()->getPluginBindings()->size() > 0 ? 1 : 0);
}


void EditorInstructionsUserInterface::render()
{
   glColor(Colors::white);
   drawStringf(3, 3, 25, "INSTRUCTIONS - %s", pageHeadersEditor[mCurPage - 1]);
   drawStringf(650, 3, 25, "PAGE %d/%d", mCurPage, getPageCount());
   drawCenteredString(571, 20, "LEFT - previous page  RIGHT, SPACE - next page  ESC exits");
   glColor(0.7f);

   drawHorizLine(0, 800, 31);
   drawHorizLine(0, 800, 569);

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
         renderConsoleCommands("Open the console by pressing [/]", consoleCommands);
         break;
      case 5:
         renderPluginCommands();
         break;
   }
}


void EditorInstructionsUserInterface::renderPluginCommands()
{
   const Vector<PluginBinding> *plugins = getGame()->getSettings()->getPluginBindings();

   Vector<ControlStringsEditor> ctrls;
   ctrls.resize(plugins->size() + 1);

   ControlStringsEditor ctrl;    // Reusable container

   for(S32 i = 0; i < plugins->size(); i++)
   {
      ctrl.command = plugins->get(i).key;
      ctrl.descr   = plugins->get(i).help;

      ctrls[i] = ctrl;
   }

   ctrl.command = "";
   ctrl.descr = "";

   ctrls[plugins->size()] = ctrl;

   renderConsoleCommands("See the wiki for info on creating your own plugins", ctrls.address());
}


// For page 1 of general instructions
static ControlStringsEditor gControls1[] = {
   { "Navigation", "HEADER" },
         { "Pan Map", "W/A/S/D or"},
         { " ", "Arrow Keys"},
         { "Zoom In", "E or Ctrl-Up" },
         { "Zoom Out", "C or Ctrl-Dwn" },
         { "Center Display", "Z" },
         { "Toggle Script Results", "Ctrl-R" },
         { "Copy Results Into Editor", "Ctrl-I" },
      { "-", "" },         // Horiz. line
   { "Editing", "HEADER" },
         { "Cut/Copy/Paste", "Ctrl-X/C/V"},
         { "Delete Selection", "Del" },
         { "Undo", "Ctrl-Z" },
         { "Redo", "Ctrl-Shift-Z" },
         { "", "" },       // End of col1
   { "Object Shortcuts", "HEADER" },
         { "Insert Teleporter", "T" },
         { "Insert Spawn Point", "G" },
         { "Insert Repair", "B" },
         { "Insert Turret", "Y" },
         { "Insert Force Field", "F" },
         { "Insert Mine", "M" },
      { "-", "" },         // Horiz. line
   { "Assigning Teams", "HEADER" },
         { "Set object's team", "1-9" },
         { "Set object to neutral", "0" },
         { "Set object to hostile", "Shift-0" },
      { "-", "" },         // Horiz. line
         { "Save", "Ctrl-S" },
         { "Reload from file", "Ctrl-Shift-L" },
         { "", "" },       // End of col2
      };


// For page 2 of basic instructions
static ControlStringsEditor gControls2[] = {
   { "Size & Rotation", "HEADER" },
         { "Flip Horiz/Vertical", "H, V" },
         { "Rotate about (0,0)", "R, Shift-R" },
         { "Arbitrary rotate", "Ctrl-Shift-R" },
         { "Scale selection", "Ctrl-Shift-X" },

         { "-", "" },      // Horiz. line
         { "Hold [Space] to suspend grid snapping", "" },
         { "[Shift-Space] to suspend vertex snapping" },
         { "Hold [Tab] to view a reference ship", "" },
         { "", "" },       // End of col1

         { "", "" },       // End of col2
   };


// This has become rather ugly and inelegant.  But you shuold see UIInstructions.cpp!!!
void EditorInstructionsUserInterface::renderPageCommands(S32 page)
{
   ControlStringsEditor *controls = (page == 1) ? gControls1 : gControls2;
   GameSettings *settings = getGame()->getSettings();

   S32 starty = 50;
   S32 y;
   S32 col1 = horizMargin;
   S32 col2 = horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.25) + 45;     // + 45 to make a little more room for Action column
   S32 col3 = horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.5);
   S32 col4 = horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.75) + 45;
   S32 actCol = col1;      // Action column
   S32 contCol = col2;     // Control column
   bool firstCol = true;
   bool done = false;

   drawHorizLine(col1, 750, starty + 26);

   static const Color txtColor = Colors::cyan;
   static const Color keyColor = Colors::white;
   static const Color secColor = Colors::yellow;
   static const Color groupHeaderColor = Colors::red;

   glColor(secColor);
   drawString(col1, starty, 20, "Action");
   drawString(col2, starty, 20, "Control");
   drawString(col3, starty, 20, "Action");
   drawString(col4, starty, 20, "Control");

   y = starty + 28;
   for(S32 i = 0; !done; i++)
   {
      if(controls[i].command == "")
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
      else if(controls[i].command == "-")      // Horiz spacer
      {
         glColor(Colors::gray40);
         drawHorizLine(actCol, actCol + 335, y + 13);
      }
      else if(controls[i].descr == "HEADER")
      {
         glColor(groupHeaderColor);
         drawString(actCol, y, 18, controls[i].command.c_str());
      }
      else
      {
         glColor(txtColor);
         drawString(actCol, y, 18, controls[i].command.c_str());      // Textual description of function (1st arg in lists above)
         glColor(keyColor);
         drawString(contCol, y, 18, controls[i].descr.c_str());
      }
      y += 26;
   }

   y = 470;
   glColor(secColor);
   drawCenteredString(y, 20, "These special keys are also usually active:");

   y += 40;
   glColor(txtColor);
   drawString(col1, y, 18, "Help");
   glColor(keyColor);
   drawStringf(col2, y, 18, "[%s]", getInputCodeString(settings, InputCodeManager::BINDING_HELP));

   glColor(txtColor);
   drawString(col3, y, 18, "Game Params Editor");
   glColor(keyColor);
   drawStringf(col4, y, 18, "[%s]", InputCodeManager::inputCodeToString(KEY_F3));

   y += 26;
   glColor(txtColor);
   drawString(col1, y, 18, "Team Editor");
   glColor(keyColor);
   drawStringf(col2, y, 18, "[%s]", InputCodeManager::inputCodeToString(KEY_F2));

   glColor(txtColor);
   drawString(col3, y, 18, "Universal Chat");
   glColor(keyColor);
   drawStringf(col4, y, 18, "[%s]", getInputCodeString(settings, InputCodeManager::BINDING_OUTGAMECHAT));
}


static const char *wallInstructions[] =
{
   "Create walls with right mouse button; hold [~] to create line",
   "Finish wall by left-clicking",
   "Drag and drop individual vertices or an entire wall",
   "Split wall at selected vertex with [\\]",
   "Join contiguous wall segments, polywalls, or zones with [J]",
   "Change wall thickness with [+] & [-] (use [Shift] for smaller changes)",
};


extern Color gNeutralTeamColor;
extern Color EDITOR_WALL_FILL_COLOR;

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
      const F32 width = 25;
     
      // Extend end points --> populates extendedEndPoints
      Vector<Point> extendedEndPoints;
      Barrier::constructBarrierEndPoints(&points, width, extendedEndPoints);

       Vector<WallSegment *> wallSegments;

      // Create a series of WallSegments, each representing a sequential pair of vertices on our wall
      for(S32 i = 0; i < extendedEndPoints.size(); i += 2)
      {
         // Create a new segment, and add it to the list
         WallSegment *newSegment = new WallSegment(mWallSegmentManager.getWallSegmentDatabase(), 
                                                   extendedEndPoints[i], extendedEndPoints[i+1], width);    
         wallSegments.push_back(newSegment);            
      }

      Vector<Point> edges;
      mWallSegmentManager.clipAllWallEdges(wallSegments, edges);      // Remove interior wall outline fragments

      glColor(EDITOR_WALL_FILL_COLOR);
      for(S32 i = 0; i < wallSegments.size(); i++)
         wallSegments[i]->renderFill(Point(0,0));

      renderWallEdges(&edges, getGame()->getSettings()->getWallOutlineColor());

      for(S32 i = 0; i < wallSegments.size(); i++)
         delete wallSegments[i];
   }

   glColor(mAnimStage <= 11 ? Colors::yellow : gNeutralTeamColor);

   glLineWidth(WALL_SPINE_WIDTH);

   renderPointVector(&points, WALL_SPINE_WIDTH);

   glLineWidth(gDefaultLineWidth);


   //tex(char style, const Point &v, S32 number, F32 alpha, S32 size, F32 scale);

   for(S32 i = 0; i < points.size(); i++)
      if(i < (points.size() - ((mAnimStage > 6) ? 0 : 1) ) && !(i == 2 && (mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)))
         renderVertex(SelectedItemVertex, points[i], i, 1);
      else if(mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)
         renderVertex(SelectedVertex, points[i], i);
      else  // mAnimStage > 11, moving vertices about
         renderVertex(HighlightedVertex, points[i], -1, 1);

   // And now some written instructions
   S32 x = 50 + getStringWidth(textSize, "* ");
   S32 y = 300;

   for(U32 i = 0; i < ARRAYSIZE(wallInstructions); i++)
   {
      glColor(Colors::red);  
      drawString(50, y, textSize, "*");

      glColor(Colors::white); 
      drawString(x, y, textSize, wallInstructions[i]);
      y += 26;
   }
}


void EditorInstructionsUserInterface::nextPage()
{
   mCurPage++;
   if(mCurPage > getPageCount())
      mCurPage = 1;

   mAnimTimer.reset(1000);
   mAnimStage = 0;
}


void EditorInstructionsUserInterface::prevPage()
{
   if(mCurPage > 1)
      mCurPage--;
   else
      mCurPage = getPageCount();

   mAnimTimer.reset(1000);
   mAnimStage = 0;
}


void EditorInstructionsUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

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
   playBoop();
   getUIManager()->reactivatePrevUI();      // To EditorUserInterface, probably
}


bool EditorInstructionsUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode)) { /* Do nothing */ }

   else if(inputCode == KEY_LEFT || inputCode == BUTTON_DPAD_LEFT || inputCode == BUTTON_DPAD_UP || inputCode == KEY_UP)
   {
      playBoop();
      prevPage();
   }
   else if(inputCode == KEY_RIGHT        || inputCode == KEY_SPACE || inputCode == BUTTON_DPAD_RIGHT ||
           inputCode == BUTTON_DPAD_DOWN || inputCode == KEY_ENTER || inputCode == KEY_DOWN)
   {
      playBoop();
      nextPage();
   }
   // F1 has dual use... advance page, then quit out of help when done
   else if(checkInputCode(getGame()->getSettings(), InputCodeManager::BINDING_HELP, inputCode))
   {
      if(mCurPage != getPageCount())
         nextPage();
      else
         exitInstructions();
   }
   else if(inputCode == KEY_ESCAPE  || inputCode == BUTTON_BACK)
      exitInstructions();
   // Nothing was handled
   else
      return false;

   // A key was handled
   return true;
}

};


