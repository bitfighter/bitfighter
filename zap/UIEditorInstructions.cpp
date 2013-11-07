//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIEditorInstructions.h"

#include "UIManager.h"

#include "ClientGame.h"       // For usage with getGame()
#include "barrier.h"     
#include "BotNavMeshZone.h"   // For Border class def
#include "gameObjectRender.h"
#include "ScreenInfo.h"
#include "VertexStylesEnum.h"
#include "FontManager.h"

#include "Colors.h"

#include "GeomUtils.h"        // For polygon triangulation
#include "RenderUtils.h"
#include "OpenglUtils.h"


namespace Zap
{

static S32 col1 = UserInterface::horizMargin;
static S32 col2 = UserInterface::horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.25) + 45;     // + 45 to make a little more room for Binding column
static S32 col3 = UserInterface::horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.5);
static S32 col4 = UserInterface::horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.75) + 45;


using UI::SymbolString;
using UI::SymbolStringSet;

// Constructor
EditorInstructionsUserInterface::EditorInstructionsUserInterface(ClientGame *game) : Parent(game), 
                                                                                     mConsoleInstructions(10),
                                                                                     mPluginInstructions(10),
                                                                                     mAnimTimer(1000)
{
   GameSettings *settings = getGame()->getSettings();
   mCurPage = 0;
   mAnimStage = 0;

   Vector<UI::SymbolShapePtr> symbols;

   // Two pages, two columns, two groups in each column
   SymbolStringSet keysInstrLeft1(LineGap),  keysBindingsLeft1(LineGap);
   SymbolStringSet keysInstrRight1(LineGap), keysBindingsRight1(LineGap);
   SymbolStringSet keysInstrLeft2(LineGap),  keysBindingsLeft2(LineGap);
   SymbolStringSet keysInstrRight2(LineGap), keysBindingsRight2(LineGap);


   // Add some headers to our 4 columns
   symbols.clear();
   symbols.push_back(SymbolString::getSymbolText("Action", HeaderFontSize, HelpContext, secColor));
   keysInstrLeft1 .add(SymbolString(symbols));
   keysInstrRight1.add(SymbolString(symbols));
   keysInstrLeft2 .add(SymbolString(symbols));
   keysInstrRight2.add(SymbolString(symbols));

   symbols.clear();
   symbols.push_back(SymbolString::getSymbolText("Control", HeaderFontSize, HelpContext, secColor));
   keysBindingsLeft1 .add(SymbolString(symbols));
   keysBindingsRight1.add(SymbolString(symbols));
   keysBindingsLeft2 .add(SymbolString(symbols));
   keysBindingsRight2.add(SymbolString(symbols));

   // Add horizontal line to first column (will draw across all)
   symbols.clear();
   symbols.push_back(SymbolString::getHorizLine(735, -14, 8, &Colors::gray70));
   keysInstrLeft1.add(SymbolString(symbols));
   keysInstrLeft2.add(SymbolString(symbols));

   symbols.clear();
   symbols.push_back(SymbolString::getBlankSymbol(0, 5));
   keysInstrRight1.add   (SymbolString(symbols));
   keysInstrRight2.add   (SymbolString(symbols));
   keysBindingsLeft1.add (SymbolString(symbols));
   keysBindingsLeft2.add (SymbolString(symbols));
   keysBindingsRight1.add(SymbolString(symbols));
   keysBindingsRight2.add(SymbolString(symbols));


   // For page 1 of general instructions
   ControlStringsEditor controls1Left[] = {
   { "HEADER", "Navigation" },
         { "Pan Map",                  "[[W]]/[[A]]/[[S]]/[[D]] or"},
         { " ",                         "Arrow Keys"},
         { "Zoom In",                  "[[E]] or [[Ctrl+Up Arrow]]" },
         { "Zoom Out",                 "[[C]] or [[Ctrl+Down Arrow]]" },
         { "Center Display",           "[[Z]]" },
         { "Toggle Script Results",    "[[Ctrl+K]]" },
         { "Copy Results Into Editor", "[[Ctrl+I]]" },
         { "Show/Hide Plugins Pane",   "[[F9]]"},
      { "-", "" },         // Horiz. line
   { "HEADER", "Editing" },
         { "Cut/Copy/Paste",           "[[Ctrl+X]]/[[C]]/[[V]]"},
         { "Delete Selection",         "[[Del]]" },
         { "Undo",                     "[[Ctrl+Z]]" },
         { "Redo",                     "[[Ctrl+Shift+Z]]" }
   };

   ControlStringsEditor controls1Right[] = {
   { "HEADER", "Object Shortcuts" },
         { "Insert Teleporter",     "[[T]]" },
         { "Insert Spawn Point",    "[[G]]" },
         { "Insert Repair",         "[[B]]" },
         { "Insert Turret",         "[[Y]]" },
         { "Insert Force Field",    "[[F]]" },
         { "Insert Mine",           "[[M]]" },
      { "-", "" },         // Horiz. line
   { "HEADER", "Assigning Teams" },
         { "Set object's team",     "[[1]]-[[9]]" },
         { "Set object to neutral", "[[0]]" },
         { "Set object to hostile", "[[Shift+0]]" },
      { "-", "" },         // Horiz. line
         { "Save",                  "[[Ctrl+S]]" },
         { "Reload from file",      "[[Ctrl+Shift+L]]" }
   };


   // For page 2 of general instructions
   ControlStringsEditor controls2Left[] = {
   { "HEADER", "Size & Rotation" },
         { "Flip horizontal/vertical", "[[H]], [[V]]" },
         { "Rotate object",            "[[R]], [[Shift+R]]" },
         { "Free rotate",              "[[Alt+R]]" },
         { "Rotate about (0,0)",       "[[Ctrl+R]], [[Ctrl+Shift+R]]" },
         { "Free rotate about (0,0)",  "[[Ctrl+Alt+R]]" },
         { "Scale selection",          "[[Ctrl+Shift+X]]" },

      { "-", "" },      // Horiz. line
         { "Hold [[Space]] to suspend grid snapping",    "" },
         { "[[Shift+Space]] to suspend vertex snapping", "" },
         { "Hold [[Tab]] to view a reference ship",      "" }
   };


   pack(keysInstrLeft1,  keysBindingsLeft1, 
        controls1Left, ARRAYSIZE(controls1Left), settings);

   pack(keysInstrRight1, keysBindingsRight1, 
        controls1Right, ARRAYSIZE(controls1Right), settings);

   pack(keysInstrLeft2,  keysBindingsLeft2, 
        controls2Left, ARRAYSIZE(controls2Left), settings);


   S32 centeringOffset = getStringWidth(HelpContext, HeaderFontSize, "Control") / 2;

   mSymbolSets1Left.addSymbolStringSet(keysInstrLeft1,      UI::AlignmentLeft,   col1);
   mSymbolSets1Left.addSymbolStringSet(keysBindingsLeft1,   UI::AlignmentCenter, col2 + centeringOffset);
   mSymbolSets1Right.addSymbolStringSet(keysInstrRight1,    UI::AlignmentLeft,   col3);
   mSymbolSets1Right.addSymbolStringSet(keysBindingsRight1, UI::AlignmentCenter, col4 + centeringOffset);

   mSymbolSets2Left.addSymbolStringSet(keysInstrLeft2,      UI::AlignmentLeft,   col1);
   mSymbolSets2Left.addSymbolStringSet(keysBindingsLeft2,   UI::AlignmentCenter, col2 + centeringOffset);
   mSymbolSets2Right.addSymbolStringSet(keysInstrRight2,    UI::AlignmentLeft,   col3);
   mSymbolSets2Right.addSymbolStringSet(keysBindingsRight2, UI::AlignmentCenter, col4 + centeringOffset);


   // Prepare special instructions
   ControlStringsEditor helpBindLeft[] = 
   { 
      { "Help",               "[[Help]]"       },
      { "Team Editor",        "[[TeamEditor]]" }
   };

   pack(mSpecialKeysInstrLeft,  mSpecialKeysBindingsLeft, 
   helpBindLeft, ARRAYSIZE(helpBindLeft), settings);
   
   ControlStringsEditor helpBindRight[] = 
   {
      { "Game Params Editor", "[[GameParameterEditor]]" },
      { "Universal Chat",     "[[OutOfGameChat]]"       }
   };

   pack(mSpecialKeysInstrRight, mSpecialKeysBindingsRight, 
        helpBindRight, ARRAYSIZE(helpBindRight), settings);


   ControlStringsEditor wallInstructions[] =
   {
      { "[[BULLET]] Create walls with right mouse button; hold [[~]] to create line", "" },
      { "[[BULLET]] Finish wall by left-clicking mouse", "" },
      { "[[BULLET]] Drag and drop individual vertices or an entire wall", "" },
      { "[[BULLET]] Split wall at selected vertex with [[\\]]", "" },
      { "[[BULLET]] Join contiguous wall segments, polywalls, or zones with [[J]]", "" },
      { "[[BULLET]] Change wall thickness with [[+]] & [[-]] (use [[Shift]] for smaller changes)", "" }
   };

   pack(mWallInstr, mWallBindings, wallInstructions, ARRAYSIZE(wallInstructions), settings);

   
   const S32 instrSize = 18;

   symbols.clear();
   SymbolString::symbolParse(settings->getInputCodeManager(), "Open the console by pressing [[/]]", 
                             symbols, HelpContext, FontSize, &Colors::green, keyColor);

   mConsoleInstructions.add(SymbolString(symbols));

   ///// 

   string tabstr = "[[TAB_STOP:200]]";

   symbols.clear();
   SymbolString::symbolParse(settings->getInputCodeManager(), "Plugins are scripts that can manipuate items in the editor.",
                             symbols, HelpContext, FontSize, &Colors::green, keyColor);
   mPluginInstructions.add(SymbolString(symbols));

   symbols.clear();
   SymbolString::symbolParse(settings->getInputCodeManager(), "See the Bitfighter wiki for info on creating your own.",
                             symbols, HelpContext, FontSize, &Colors::green, keyColor);
   mPluginInstructions.add(SymbolString(symbols));

   // Using TAB_STOP:0 below will cause the text and the horiz. line to be printed in the same space, creating a underline effect
   symbols.clear();
   symbols.push_back(SymbolString::getHorizLine(735, FontSize, FontSize + 4, &Colors::gray70));
   SymbolString::symbolParse(settings->getInputCodeManager(), "[[TAB_STOP:0]]Key" + tabstr + "Description",
                             symbols, HelpContext, FontSize, &Colors::yellow, keyColor);
   mPluginInstructions.add(SymbolString(symbols));


   const Vector<PluginBinding> *plugins = settings->getPluginBindings();

   for(S32 i = 0; i < plugins->size(); i++)
   {
      string key   = "[[" + plugins->get(i).key + "]]";  // Add the [[ & ]] to make it parsable
      string instr = plugins->get(i).help;

      symbols.clear();
      SymbolString::symbolParse(settings->getInputCodeManager(), key + tabstr + instr,
                                symbols, HelpContext, FontSize, txtColor, keyColor);
      mPluginInstructions.add(SymbolString(symbols));
   }
}


// Destructor
EditorInstructionsUserInterface::~EditorInstructionsUserInterface()
{
   // Do nothing
}


void EditorInstructionsUserInterface::onActivate()
{
   mCurPage = 0;     // Start at the beginning, silly!
   onPageChanged();
}


const char *pageHeadersEditor[] = {
   "BASIC COMMANDS",
   "ADVANCED COMMANDS",
   "WALLS AND LINES",
   "SCRIPTING CONSOLE",
   "PLUGINS"
};


S32 EditorInstructionsUserInterface::getPageCount()
{
   return 4 + (getGame()->getSettings()->getPluginBindings()->size() > 0 ? 1 : 0);
}


void EditorInstructionsUserInterface::render()
{
   FontManager::pushFontContext(HelpContext);

   Parent::render(pageHeadersEditor[mCurPage], mCurPage + 1, getPageCount());

   static ControlStringsEditor consoleCommands[] = {
      { "Coming soon...", "Coming soon..." },
      { "", "" },      // End of list
   };


   switch(mCurPage)
   {
      case 0:
         renderPageCommands(1);
         break;
      case 1:
         renderPageCommands(2);
         break;
      case 2:
         renderPageWalls();
         break;
      case 3:
         renderConsoleCommands(mConsoleInstructions, consoleCommands);
         break;
      case 4:
         mPluginInstructions.render(horizMargin, 60, UI::AlignmentLeft);
         break;
   }

   FontManager::popFontContext();
}


// This has become rather ugly and inelegant.  But you shuold see UIInstructions.cpp!!!
void EditorInstructionsUserInterface::renderPageCommands(S32 page)
{
   S32 y = 60;             // Is 65 in UIInstructions::render()...

   if(page == 1)
   {
      mSymbolSets1Left.render(y);
      mSymbolSets1Right.render(y);
   }
   else
   {      
      mSymbolSets2Left.render(y);
      mSymbolSets2Right.render(y);
   }

   y = 486;
   glColor(secColor);
   drawCenteredString(y, 20, "These special keys are also usually active:");

   y += 45;

   mSpecialKeysInstrLeft.render (col1, y, UI::AlignmentLeft);
   mSpecialKeysInstrRight.render(col3, y, UI::AlignmentLeft);

   S32 centeringOffset = getStringWidth(HelpContext, HeaderFontSize, "Control") / 2;

   mSpecialKeysBindingsLeft.render (col2 + centeringOffset, y, UI::AlignmentCenter);
   mSpecialKeysBindingsRight.render(col4 + centeringOffset, y, UI::AlignmentCenter);
}


// Draw animated creation of walls
void EditorInstructionsUserInterface::renderPageWalls()
{
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


   FontManager::pushFontContext(OldSkoolContext);

   for(S32 i = 0; i < points.size(); i++)
   {
      S32 vertNum = S32(((F32)i  / 2) + 0.5);     // Ick!

      if(i < (points.size() - ((mAnimStage > 6) ? 0 : 1) ) && !(i == 4 && (mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)))
         renderVertex(SelectedItemVertex, points[i], vertNum, 1);
      else if(mAnimStage == 9 || mAnimStage == 10 || mAnimStage == 11)
         renderVertex(SelectedVertex, points[i], vertNum);
      else  // mAnimStage > 11, moving vertices about
         renderVertex(HighlightedVertex, points[i], -1, 1);
   }

   FontManager::popFontContext();

   mWallInstr.render(50, 300, UI::AlignmentLeft);     // The written instructions block
}


void EditorInstructionsUserInterface::onPageChanged()
{
   mAnimTimer.reset();
   mAnimStage = 0;
}


void EditorInstructionsUserInterface::nextPage()
{
   mCurPage++;
   if(mCurPage == getPageCount())
      mCurPage = 0;

   onPageChanged();
}


void EditorInstructionsUserInterface::prevPage()
{
   mCurPage--;
   if(mCurPage < 0)
      mCurPage = getPageCount() - 1;

   onPageChanged();
}


void EditorInstructionsUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   if(mAnimTimer.update(timeDelta))
   {
      mAnimTimer.reset();
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
      if(mCurPage < getPageCount())
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


