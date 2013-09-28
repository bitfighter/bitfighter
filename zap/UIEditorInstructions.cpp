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

#include "UIEditorInstructions.h"

#include "UIManager.h"

#include "ClientGame.h"       // For usage with getGame()
#include "barrier.h"     
#include "BotNavMeshZone.h"   // For Border class def
#include "gameObjectRender.h"
#include "ScreenInfo.h"
#include "VertexStylesEnum.h"

#include "Colors.h"

#include "GeomUtils.h"        // For polygon triangulation
#include "RenderUtils.h"
#include "OpenglUtils.h"


namespace Zap
{

static Vector<Point> sample1o, sample2o, sample3o, sample4o, sample5o;     // outline
static Vector<Point> sample1f, sample2f, sample3f, sample4f, sample5f;     // fill
static Border border34;


// For page 1 of general instructions
static ControlStringsEditor controls1[] = {
   { "Navigation", "HEADER" },
         { "Pan Map", "[[W]]/[[A]]/[[S]]/[[D]] or"},
         { " ", "Arrow Keys"},
         { "Zoom In", "[[E]] or [[Ctrl+Up Arrow]]" },
         { "Zoom Out", "[[C]] or [[Ctrl+Down Arrow]]" },
         { "Center Display", "[[Z]]" },
         { "Toggle Script Results", "[[Ctrl+K]]" },
         { "Copy Results Into Editor", "[[Ctrl+I]]" },
         { "Show/Hide Plugins Pane", "[[F9]]"},
      { "-", "" },         // Horiz. line
   { "Editing", "HEADER" },
         { "Cut/Copy/Paste", "[[Ctrl+X]]/[[C]]/[[V]]"},
         { "Delete Selection", "[[Del]]" },
         { "Undo", "[[Ctrl+Z]]" },
         { "Redo", "[[Ctrl+Shift+Z]]" },
         { "", "" },       // End of col1
   { "Object Shortcuts", "HEADER" },
         { "Insert Teleporter", "[[T]]" },
         { "Insert Spawn Point", "[[G]]" },
         { "Insert Repair", "[[B]]" },
         { "Insert Turret", "[[Y]]" },
         { "Insert Force Field", "[[F]]" },
         { "Insert Mine", "[[M]]" },
      { "-", "" },         // Horiz. line
   { "Assigning Teams", "HEADER" },
         { "Set object's team", "[[1]]-[[9]]" },
         { "Set object to neutral", "[[0]]" },
         { "Set object to hostile", "[[Shift+0]]" },
      { "-", "" },         // Horiz. line
         { "Save", "[[Ctrl+S]]" },
         { "Reload from file", "[[Ctrl+Shift+L]]" },
         { "", "" },       // End of col2
      };


// For page 2 of general instructions
static ControlStringsEditor controls2[] = {
   { "Size & Rotation", "HEADER" },
         { "Flip Horiz/Vertical", "[[H]], [[V]]" },
         { "Spin", "[[R]], [[Shift+R]]" },
         { "Arbitrary spin", "[[Alt+R]]" },
         { "Rotate about (0,0)", "[[Ctrl+R]], [[Ctrl+Shift+R]]" },
         { "Arbitrary rotate about (0,0)", "[[Ctrl+Alt+R]]" },
         { "Scale selection", "[[Ctrl+Shift+X]]" },

         { "-", "" },      // Horiz. line
         { "Hold [[Space]] to suspend grid snapping",    "" },
         { "[[Shift+Space]] to suspend vertex snapping", "" },
         { "Hold [[Tab]] to view a reference ship",      "" },
         { "", "" },       // End of col1

         { "", "" },       // End of col2
   };


static S32 col1 = UserInterface::horizMargin;
static S32 col2 = UserInterface::horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.25) + 45;     // + 45 to make a little more room for Binding column
static S32 col3 = UserInterface::horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.5);
static S32 col4 = UserInterface::horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.75) + 45;


// Constructor
EditorInstructionsUserInterface::EditorInstructionsUserInterface(ClientGame *game) : Parent(game)
{
   mCurPage = 1;
   mAnimStage = 0;

   Vector<UI::SymbolShapePtr> symbols;

   // Two pages, two columns, two groups in each column
   UI::SymbolStringSet keysInstrLeft1(LineGap),  keysBindingsLeft1(LineGap);
   UI::SymbolStringSet keysInstrRight1(LineGap), keysBindingsRight1(LineGap);
   UI::SymbolStringSet keysInstrLeft2(LineGap),  keysBindingsLeft2(LineGap);
   UI::SymbolStringSet keysInstrRight2(LineGap), keysBindingsRight2(LineGap);


   // Add some headers to our 4 columns
   symbols.clear();
   symbols.push_back(UI::SymbolString::getSymbolText("Action", HeaderFontSize, HelpContext, secColor));
   keysInstrLeft1 .add(UI::SymbolString(symbols));
   keysInstrRight1.add(UI::SymbolString(symbols));
   keysInstrLeft2 .add(UI::SymbolString(symbols));
   keysInstrRight2.add(UI::SymbolString(symbols));

   symbols.clear();
   symbols.push_back(UI::SymbolString::getSymbolText("Control", HeaderFontSize, HelpContext, secColor));
   keysBindingsLeft1 .add(UI::SymbolString(symbols));
   keysBindingsRight1.add(UI::SymbolString(symbols));
   keysBindingsLeft2 .add(UI::SymbolString(symbols));
   keysBindingsRight2.add(UI::SymbolString(symbols));

   // Add horizontal line to first column (will draw across all)
   symbols.clear();
   symbols.push_back(UI::SymbolString::getHorizLine(735, -14, 8, &Colors::gray70));
   keysInstrLeft1.add(UI::SymbolString(symbols));
   keysInstrLeft2.add(UI::SymbolString(symbols));

   symbols.clear();
   symbols.push_back(UI::SymbolString::getBlankSymbol(0, 5));
   keysInstrRight1.add   (UI::SymbolString(symbols));
   keysInstrRight2.add   (UI::SymbolString(symbols));
   keysBindingsLeft1.add (UI::SymbolString(symbols));
   keysBindingsLeft2.add (UI::SymbolString(symbols));
   keysBindingsRight1.add(UI::SymbolString(symbols));
   keysBindingsRight2.add(UI::SymbolString(symbols));


   pack(keysInstrLeft1,  keysBindingsLeft1, 
        keysInstrRight1, keysBindingsRight1, 
        controls1, ARRAYSIZE(controls1), getGame()->getSettings());

   pack(keysInstrLeft2,  keysBindingsLeft2, 
        keysInstrRight2, keysBindingsRight2, 
        controls2, ARRAYSIZE(controls2), getGame()->getSettings());

   S32 centeringOffset = getStringWidth(HelpContext, HeaderFontSize, "Control") / 2;

   mSymbolSets1.addSymbolStringSet(keysInstrLeft1,     UI::AlignmentLeft,   col1);
   mSymbolSets1.addSymbolStringSet(keysBindingsLeft1,  UI::AlignmentCenter, col2 + centeringOffset);
   mSymbolSets1.addSymbolStringSet(keysInstrRight1,    UI::AlignmentLeft,   col3);
   mSymbolSets1.addSymbolStringSet(keysBindingsRight1, UI::AlignmentCenter, col4 + centeringOffset);

   mSymbolSets2.addSymbolStringSet(keysInstrLeft2,     UI::AlignmentLeft,   col1);
   mSymbolSets2.addSymbolStringSet(keysBindingsLeft2,  UI::AlignmentCenter, col2 + centeringOffset);
   mSymbolSets2.addSymbolStringSet(keysInstrRight2,    UI::AlignmentLeft,   col3);
   mSymbolSets2.addSymbolStringSet(keysBindingsRight2, UI::AlignmentCenter, col4 + centeringOffset);


   // Prepare special instructions
   HelpBind helpBind[] = { 
      { Left,  "Help",               InputCodeManager::BINDING_HELP },
      { Left,  "Team Editor",        InputCodeManager::BINDING_TEAM_EDITOR },
      { Right, "Game Params Editor", InputCodeManager::BINDING_GAME_PARAMS_EDITOR},
      { Right, "Universal Chat",     InputCodeManager::BINDING_OUTGAMECHAT }
   };

   pack(mSpecialKeysInstrLeft,  mSpecialKeysBindingsLeft, 
        mSpecialKeysInstrRight, mSpecialKeysBindingsRight, 
        helpBind, ARRAYSIZE(helpBind), getGame()->getSettings());
}


// Destructor
EditorInstructionsUserInterface::~EditorInstructionsUserInterface()
{
   // Do nothing
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

   //drawHorizLine(0, 800, 31);
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
      ctrl.binding   = plugins->get(i).help;

      ctrls[i] = ctrl;
   }

   ctrl.command = "";
   ctrl.binding = "";

   ctrls[plugins->size()] = ctrl;

   renderConsoleCommands("See the wiki for info on creating your own plugins", ctrls.address());
}


// This has become rather ugly and inelegant.  But you shuold see UIInstructions.cpp!!!
void EditorInstructionsUserInterface::renderPageCommands(S32 page)
{
   ControlStringsEditor *controls = (page == 1) ? controls1 : controls2;
   GameSettings *settings = getGame()->getSettings();

   S32 starty = 50;
   S32 y = starty;
   S32 actCol = col1;      // Action column
   S32 contCol = col2;     // Control column
   bool firstCol = true;
   bool done = false;

   //drawHorizLine(col1, 750, starty + 26);

   if(page == 1)
      y += mSymbolSets1.render(y);
   else
      y += mSymbolSets2.render(y);

   y = 480;
   glColor(secColor);
   drawCenteredString(y, 20, "These special keys are also usually active:");

   y += 45;

   mSpecialKeysInstrLeft.render (col1, y, UI::AlignmentLeft);
   mSpecialKeysInstrRight.render(col3, y, UI::AlignmentLeft);

   S32 centeringOffset = getStringWidth(HelpContext, HeaderFontSize, "Control") / 2;

   mSpecialKeysBindingsLeft.render (col2 + centeringOffset, y, UI::AlignmentCenter);
   mSpecialKeysBindingsRight.render(col4 + centeringOffset, y, UI::AlignmentCenter);
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


void EditorInstructionsUserInterface::renderPageWalls()
{
   // Draw animated creation of walls

   //drawStringf(400, 100, 25, "%d", mAnimStage);     // Useful to have around when things go wrong!

   S32 vertOffset = 20;
   S32 textSize = 18;

   Vector<Point> points;

   points.push_back(Point(150, 100 + vertOffset));
   points.push_back(Point(220, 190 + vertOffset));

   if(mAnimStage > 0 && mAnimStage < 10)
   {
      points.push_back(points.last());
      points.push_back(Point(350, 80 + vertOffset));
   }
   else if(mAnimStage == 10)
   {
      points.push_back(points.last());
      points.push_back(Point(350, 150 + vertOffset));
   }
   else if(mAnimStage >= 11)
   {
      points.push_back(points.last());
      points.push_back(Point(350, 200 + vertOffset));
   }

   if(mAnimStage > 1)
   {
      points.push_back(points.last());
      points.push_back(Point(470, 140 + vertOffset));
   }

   if(mAnimStage > 2)
   {
      points.push_back(points.last());
      points.push_back(Point(550, 120 + vertOffset));
   }

   if(mAnimStage == 4)
   {
      points.push_back(points.last());
      points.push_back(Point(650, 100 + vertOffset));
   }

   else if(mAnimStage == 5)
   {
      points.push_back(points.last());
      points.push_back(Point(690, 130 + vertOffset));
   }

   else if(mAnimStage >= 6)
   {
      points.push_back(points.last());
      points.push_back(Point(650, 170 + vertOffset));
   }

   if(mAnimStage > 6)
   {
      const F32 width = 25;
     
      // Extend end points --> populates extendedEndPoints
      Vector<Point> extendedEndPoints;
      constructBarrierEndPoints(&points, width, extendedEndPoints);

       Vector<DatabaseObject *> wallSegments;      

      // Create a series of WallSegments, each representing a sequential pair of vertices on our wall
      for(S32 i = 0; i < extendedEndPoints.size(); i += 2)
      {
         // Create a new segment, and add it to the list.  The WallSegment constructor will add it to the specified database.
         WallSegment *newSegment = new WallSegment(mWallSegmentManager.getWallSegmentDatabase(), 
                                                   extendedEndPoints[i], extendedEndPoints[i+1], width);    
         wallSegments.push_back(newSegment);            
      }

      Vector<Point> edges;
      mWallSegmentManager.clipAllWallEdges(&wallSegments, edges);      // Remove interior wall outline fragments

      for(S32 i = 0; i < wallSegments.size(); i++)
      {
         WallSegment *wallSegment = static_cast<WallSegment *>(wallSegments[i]);
         wallSegment->renderFill(Point(0,0), Colors::EDITOR_WALL_FILL_COLOR);
      }

      renderWallEdges(edges, *getGame()->getSettings()->getWallOutlineColor());

      for(S32 i = 0; i < wallSegments.size(); i++)
         delete wallSegments[i];
   }

   glColor(mAnimStage <= 11 ? Colors::yellow : Colors::NeutralTeamColor);

   glLineWidth(gLineWidth3);

   renderPointVector(&points, GL_LINES);

   glLineWidth(gDefaultLineWidth);


   for(S32 i = 0; i < points.size(); i++)
   {
      S32 vertNum = S32(((F32)i  / 2) + 0.5);     // Ick!

      if(i < (points.size() - ((mAnimStage > 6) ? 0 : 1) ) && !(i == 2 && (mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)))
         renderVertex(SelectedItemVertex, points[i], vertNum, 1);
      else if(mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)
         renderVertex(SelectedVertex, points[i], vertNum);
      else  // mAnimStage > 11, moving vertices about
         renderVertex(HighlightedVertex, points[i], -1, 1);
   }

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
   else if(checkInputCode(InputCodeManager::BINDING_HELP, inputCode))
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


