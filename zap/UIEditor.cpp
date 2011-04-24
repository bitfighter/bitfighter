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

#include "UIEditor.h"
#include "UINameEntry.h"
#include "UIEditorInstructions.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UITeamDefMenu.h"
#include "UIGameParameters.h"
#include "UIErrorMessage.h"
#include "UIYesNo.h"
#include "gameObjectRender.h"
#include "game.h"                // For EditorGame def
#include "gameType.h"
#include "soccerGame.h"          // For soccer ball radius
#include "engineeredObjects.h"   // For Turret properties
#include "barrier.h"             // For BarrierWidth
#include "speedZone.h"           // For default speed
#include "gameItems.h"           // For Asteroid defs
#include "teleporter.h"          // For teleporter radius
#include "config.h"
#include "GeomUtils.h"
#include "textItem.h"            // For MAX_TEXTITEM_LEN and MAX_TEXT_SIZE
#include "luaLevelGenerator.h"
#include "stringUtils.h"
#include "../glut/glutInclude.h"

#include "oglconsole.h"          // Our console object

#include <ctype.h>
#include <exception>

namespace Zap
{

EditorUserInterface gEditorUserInterface;

const S32 DOCK_WIDTH = 50;
const S32 MIN_SCALE = 300;    // Most zoomed-in scale
const S32 MAX_SCALE = 10;     // Most zoomed-out scale

// Some colors

extern Color gNexusOpenColor;
extern Color EDITOR_WALL_FILL_COLOR;


static const Color inactiveSpecialAttributeColor = Color(.6, .6, .6);
static const Color white = Color(1,1,1);
static const Color red = Color(1,0,0);
static const Color yellow = Color(1,1,0);
static const Color blue = Color(0,0,1);
static const Color cyan = Color(0,1,1);
static const Color green = Color(0,1,0);
static const Color magenta = Color(1,0,1);
static const Color black = Color(0,0,0);

static const Color SELECT_COLOR = yellow;
static const Color HIGHLIGHT_COLOR = white;

const Color EditorUserInterface::DOCK_LABEL_COLOR = white;

static const S32 TEAM_NEUTRAL = Item::TEAM_NEUTRAL;
static const S32 TEAM_HOSTILE = Item::TEAM_HOSTILE;

static Vector<EditorObject *> *mLoadTarget;

enum EntryMode {
   EntryID,          // Entering an objectID
   EntryAngle,       // Entering an angle
   EntryScale,       // Entering a scale
   EntryNone         // Not in a special entry mode
};


static EntryMode entryMode;
static Vector<ZoneBorder> zoneBorders;

void saveLevelCallback()
{
   if(gEditorUserInterface.saveLevel(true, true))
      UserInterface::reactivateMenu(gMainMenuUserInterface);
   else
      gEditorUserInterface.reactivate();
}


void backToMainMenuCallback()
{
   UserInterface::reactivateMenu(gMainMenuUserInterface);
}


extern EditorGame *gEditorGame;

// Constructor
EditorUserInterface::EditorUserInterface() : mGridDatabase(GridDatabase(false))     // false --> not using game coords
{
   setMenuID(EditorUI);

   // Create some items for the dock...  One of each, please!
   mShowMode = ShowAllObjects; 
   mWasTesting = false;

   mSnapVertex_i = NULL;
   mSnapVertex_j = NONE;
   mItemHit = NULL;
   mEdgeHit = NONE;

   mLastUndoStateWasBarrierWidthChange = false;

   mUndoItems.resize(UNDO_STATES);

   // Pass the gridDatabase on to these other objects, so they can have local access
   //EditorObject::setGridDatabase(&mGridDatabase);
   WallSegment::setGridDatabase(&mGridDatabase);      // Still needed?  Can do this via editorGame?
   WallEdge::setGridDatabase(&mGridDatabase);
   WallSegmentManager::setGridDatabase(&mGridDatabase);

   //editorGame = gEditorGame;     // TODO: we should be passing this in rather than relying on the global
}


static const S32 DOCK_POLY_HEIGHT = 20;
static const S32 DOCK_POLY_WIDTH = DOCK_WIDTH - 10;

void EditorUserInterface::populateDock()
{
   mDockItems.deleteAndClear();

   if(mShowMode == ShowAllObjects || mShowMode == ShowAllButNavZones)
   {
      S32 xPos = gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH / 2;
      S32 yPos = 35;
      const S32 spacer = 35;

      /*
      // Reinstate in 014!
      mDockItems.push_back(WorldItem(ItemRepair, Point(xPos /*- 10* /, yPos), mCurrentTeam, true, 0, 0));
      //mDockItems.push_back(WorldItem(ItemEnergy, Point(xPos + 10, yPos), mCurrentTeam, true, 0, 0));

      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemForceField, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemSpawn, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemTurret, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemTeleporter, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemSpeedZone, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      */
      TextItem *textItem = new TextItem();

      Point pos(xPos, yPos);

      textItem->setVert(pos, 0);
      textItem->setVert(pos, 1);
      textItem->setTeam(mCurrentTeam);

      textItem->addToDock(gEditorGame);

      yPos += spacer;
      /*
      if(!strcmp(mGameType, "SoccerGameType"))
         mDockItems.push_back(WorldItem(ItemSoccerBall, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      else
         mDockItems.push_back(WorldItem(ItemFlag, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;

      mDockItems.push_back(WorldItem(ItemFlagSpawn, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;


      mDockItems.push_back(WorldItem(ItemMine, Point(xPos - 10, yPos), mCurrentTeam, true, 0, 0));
      mDockItems.push_back(WorldItem(ItemSpyBug, Point(xPos + 10, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;

      // These two will share a line
      mDockItems.push_back(WorldItem(ItemAsteroid, Point(xPos - 10, yPos), mCurrentTeam, true, 0, 0));
      mDockItems.push_back(WorldItem(ItemAsteroidSpawn, Point(xPos + 10, yPos), mCurrentTeam, true, 0, 0));

      yPos += spacer;

      // These two will share a line
      mDockItems.push_back(WorldItem(ItemBouncyBall, Point(xPos - 10, yPos), mCurrentTeam, true, 0, 0));
      mDockItems.push_back(WorldItem(ItemResource, Point(xPos + 10, yPos), mCurrentTeam, true, 0, 0));

      yPos += 25;
      mDockItems.push_back(WorldItem(ItemLoadoutZone, Point(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH + 5, yPos), 
                                     mCurrentTeam, true, DOCK_POLY_WIDTH, DOCK_POLY_HEIGHT));
      yPos += 25;

      if(!strcmp(mGameType, "HuntersGameType"))
      {
         mDockItems.push_back(WorldItem(ItemNexus, Point(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH + 5, yPos), 
                                        mCurrentTeam, true, DOCK_POLY_WIDTH, DOCK_POLY_HEIGHT));
         yPos += 25;
      }
      else
      {
         mDockItems.push_back(WorldItem(ItemGoalZone, Point(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH + 5, yPos), 
                                        mCurrentTeam, true, DOCK_POLY_WIDTH, DOCK_POLY_HEIGHT));
         yPos += 25;
      }

      mDockItems.push_back(WorldItem(ItemPolyWall, Point(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH + 5, yPos), 
                                     mCurrentTeam, true, DOCK_POLY_WIDTH, DOCK_POLY_HEIGHT));
      yPos += spacer;
      */
   }
   /*
   else if(mShowMode == NavZoneMode)
   {
      mDockItems.push_back(WorldItem(ItemNavMeshZone, Point(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH + 5, 
                                                            gScreenInfo.getGameCanvasHeight() - vertMargin - 82), 
                                     TEAM_NEUTRAL, true, DOCK_POLY_WIDTH, DOCK_POLY_HEIGHT));
   }
   */
}


//struct GameItemRec
//{
//   const char *name;    // Item's name, as seen in save file
//   bool hasWidth;       // Does item have width?
//   bool hasTeam;        // Item can be associated with team
//   bool canBeNeutral;   // Item can be neutral 
//   bool canBeHostile;   // Item can be hostile 
//   bool hasText;        // Item has a text string attached to it
//   bool hasRepop;       // Item has a repop delay that can be set
//   GeomType geom;
//   bool specialTabKeyRendering;  // true if item is rendered in a special way when tab is down
//   const char *prettyNamePlural;
//   const char *onDockName;       // Briefer, pretty looking label to label things on dock
//   const char *onScreenName;     // Brief, pretty looking label to label things on screen
//   const char *helpText;         // Help string displayed when hovering over item on the dock
//};
//
////                            x        x         x          x                            x                                                                              x
//// Remember to keep these properly aligned with GameItems enum        x        x                      
////   Name,                 hasWidth, hasTeam, canBeNeut, canBeHos, hasText, hasRepop,   geom,        special, prettyNamePlural        onDockName   onScreenName      description
//GameItemRec itemDef[] = {
//   { "Spawn",               false,    true,      true,     false,   false,   false,   geomPoint,       true,   "Spawn points",           "Spawn",    "Spawn",        "Location where ships start.  At least one per team is required. [G]" },
//   { "SpeedZone",           false,    false,     true,     true,    false,   false,   geomSimpleLine,  false,  "GoFasts",                "GoFast",   "GoFast",       "Makes ships go fast in direction of arrow. [P]" },
//   { "SoccerBallItem",      false,    false,     false,    false,   false,   false,   geomPoint,       true,   "Soccer balls",           "Ball",     "Ball",         "Soccer ball, can only be used in Soccer games." },
//   { "FlagItem",            false,    true,      true,     true,    false,   false,   geomPoint,       false,  "Flags",                  "Flag",     "Flag",         "Flag item, used by a variety of game types." },
//   { "FlagSpawn",           false,    true,      true,     true,    false,   true,    geomPoint,       true,   "Flag spawn points",      "FlagSpawn","FlagSpawn",    "Location where flags (or balls in Soccer) spawn after capture." },
//   { "BarrierMaker",        true,     false,     false,    false,   false,   false,   geomLine,        false,  "Barrier makers",         "Wall",     "Wall",         "Run-of-the-mill wall item." },
//   { "PolyWall",            false,    false,     false,    false,   false,   false,   geomPoly,        false,  "PolyWalls",              "Wall",     "Wall",         "Polygon wall barrier; create linear walls with right mouse click." },
//   { "LineItem",            true,     true,      true,     true,    false,   false,   geomLine,        false,  "Decorative Lines",       "LineItem", "LineItem",     "Decorative linework." },
//   { "Teleporter",          false,    false,     false,    false,   false,   false,   geomSimpleLine,  false,  "Teleporters",            "Teleport", "Teleport",     "Teleports ships from one place to another. [T]" },
//   { "RepairItem",          false,    false,     false,    false,   false,   true,    geomPoint,       false,  "Repair items",           "Repair",    "Repair",       "Repairs damage to ships. [B]" },
//   { "EnergyItem",          false,    false,     false,    false,   false,   true,    geomPoint,       false,  "Energy items",           "Enrg",     "Energy",       "Restores energy to ships" },
//   { "TestItem",            false,    false,     false,    false,   false,   false,   geomPoint,       true,   "Test items",             "Test",     "Test Item",    "Bouncy object that floats around and gets in the way." },
//   { "Asteroid",            false,    false,     false,    false,   false,   false,   geomPoint,       true,   "Asteroids",              "Ast.",     "Asteroid",     "Shootable asteroid object.  Just like the arcade game." },
//   { "AsteroidSpawn",       false,    false,     false,    false,   false,   true,    geomPoint,       true,   "Asteroid spawn points",  "ASP",      "AsteroidSpawn","Periodically spawns a new asteroid." },
//   { "Mine",                false,    false,     true,     true,    false,   false,   geomPoint,       true,   "Mines",                  "Mine",     "Mine",         "Mines can be prepositioned, and are are \"hostile to all\". [M]" },
//   { "SpyBug",              false,    true,      true,     true,    false,   false,   geomPoint,       false,  "Spy bugs",               "Bug",      "Spy Bug",      "Remote monitoring device that shows enemy ships on the commander's map. [Ctrl-B]" },
//   { "ResourceItem",        false,    false,     false,    false,   false,   false,   geomPoint,       true,   "Resource items",         "Res.",     "Resource",     "Small bouncy object that floats around and gets in the way." },
//   { "LoadoutZone",         false,    true,      true,     true,    false,   false,   geomPoly,        false,  "Loadout zones",          "Loadout",  "Loadout",      "Area to finalize ship modifications.  Each team should have at least one." },
//   { "HuntersNexusObject",  false,    false,     true,     true,    false,   false,   geomPoly,        false,  "Nexus zones",            "Nexus",    "Nexus",        "Area to bring flags in Hunter game.  Cannot be used in other games." },
//   { "SlipZone",            false,    false,     true,     true,    false,   false,   geomPoly,        false,  "Slip zones",             "Slip Zone","Slip Zone",    "Not yet implemented." },
//   { "Turret",              false,    true,      true,     true,    false,   true,    geomPoint,       false,  "Turrets",                "Turret",   "Turret",       "Creates shooting turret.  Can be on a team, neutral, or \"hostile to all\". [Y]" },
//   { "ForceFieldProjector", false,    true,      true,     true,    false,   true ,   geomPoint,       false,  "Force field projectors", "ForceFld", "ForceFld",     "Creates a force field that lets only team members pass. [F]" },
//   { "GoalZone",            false,    true,      true,     true,    false,   false,   geomPoly,        false,  "Goal zones",             "Goal",     "Goal",         "Target area used in a variety of games." },
//   { "TextItem",            false,    true,      true,     true,    true,    false,   geomSimpleLine,  false,  "Text Items",             "TextItem", "Text",         "Draws a bit of text on the map.  Visible only to team, or to all if neutral." },
//   { "BotNavMeshZone",      false,    false,     true,     true,    false,   false,   geomPoly,        false,  "NavMesh Zones",          "NavMesh",  "NavMesh",      "Creates navigational mesh zone for robots." },
//
//   { NULL,                  false,    false,     false,   false,    false,   false,   geomNone,        false,  "",                       "",         "",             "" },
//};


// Destructor -- unwind things in an orderly fashion
EditorUserInterface::~EditorUserInterface()
{
   mItems.deleteAndClear();
   mDockItems.deleteAndClear();
   mLevelGenItems.deleteAndClear();
   mClipboard.deleteAndClear();
   delete mNewItem;

   for(S32 i = 0; i < UNDO_STATES; i++)
      mUndoItems[i].deleteAndClear();
}


static const S32 NO_NUMBER = -1;

// Draw a vertex of a selected editor item
void renderVertex(VertexRenderStyles style, const Point &v, S32 number, F32 alpha, S32 size)
{
   bool hollow = style == HighlightedVertex || style == SelectedVertex || style == SelectedItemVertex || style == SnappingVertex;

   // Fill the box with a dark gray to make the number easier to read
   if(hollow && number != NO_NUMBER)
   {
      glColor3f(.25, .25, .25);
      drawFilledSquare(v, size);
   }
      
   if(style == HighlightedVertex)
      glColor(HIGHLIGHT_COLOR, alpha);
   else if(style == SelectedVertex)
      glColor(SELECT_COLOR, alpha);
   else if(style == SnappingVertex)
      glColor(magenta, alpha);
   else
      glColor(red, alpha);

   drawSquare(v, size, !hollow);

   if(number != NO_NUMBER)     // Draw vertex numbers
   {
      glColor(white, alpha);
      UserInterface::drawStringf(v.x - UserInterface::getStringWidthf(6, "%d", number) / 2, v.y - 3, 6, "%d", number);
   }
}


void renderVertex(VertexRenderStyles style, const Point &v, S32 number, F32 alpha)
{
   renderVertex(style, v, number, alpha, 5);
}


void renderVertex(VertexRenderStyles style, const Point &v, S32 number)
{
   renderVertex(style, v, number, 1);
}


inline F32 getGridSize()
{
   return gEditorGame->getGridSize();
}


// Replaces the need to do a convertLevelToCanvasCoord on every point before rendering
static void setLevelToCanvasCoordConversion()
{
   F32 scale =  gEditorUserInterface.getCurrentScale()/* / getGridSize()*/;
   Point offset = gEditorUserInterface.getCurrentOffset()/* / scale*/;

   glTranslatef(offset.x, offset.y, 0);
   glScalef(scale, scale, 1);
} 


// Draws a line connecting points in mVerts
static void renderPolyline(const Vector<Point> verts)
{
   glPushMatrix();
      setLevelToCanvasCoordConversion();
      renderPointVector(verts, GL_LINE_STRIP);
   glPopMatrix();
}


// Returns true if we should use the in-game rendering, false if we should use iconified editor rendering
// TODO: get rid of this fn
static bool renderFull(U32 index, F32 scale, bool dockItem, bool snapped)
{
   if(dockItem)
      return false;

   if(index == ItemTurret || index == ItemForceField)
      return(snapped && scale > 70);
   
   return true;
}


static void drawLetter(char letter, const Point &pos, const Color &color, F32 alpha)
{
   // Mark the item with a letter, unless we're showing the reference ship
   S32 vertOffset = 8;
   if (letter >= 'a' && letter <= 'z')    // Better position lowercase letters
      vertOffset = 10;

   glColor(color, alpha);
   F32 xpos = pos.x - UserInterface::getStringWidthf(15, "%c", letter) / 2;

   UserInterface::drawStringf(xpos, pos.y - vertOffset, 15, "%c", letter);
}


static void renderGenericItem(const Point &pos, const Color &c, F32 alpha, const Color &letterColor, char letter)
{
   glColor(c, alpha);
   drawFilledSquare(pos, 8);  // Draw filled box in which we'll put our letter
   drawLetter(letter, pos, letterColor, alpha);
}


extern Color gNeutralTeamColor;
extern Color gHostileTeamColor;


inline F32 getCurrentScale()
{
   return gEditorUserInterface.getCurrentScale();
}


inline Point convertLevelToCanvasCoord(const Point &point, bool convert = true) 
{ 
   return gEditorUserInterface.convertLevelToCanvasCoord(point, convert); 
}


////////////////////////////////////
////////////////////////////////////

// Removes most recent undo state from stack --> won't actually delete items on stack until we need the slot, or we quit
void EditorUserInterface::deleteUndoState()
{
   mLastUndoIndex--;
   mLastRedoIndex--; 
}


static void copyItems(const Vector<EditorObject *> &from, Vector<EditorObject *> &to)
{
   to.deleteAndClear();
   
   to.resize(from.size());      // Preallocation makes things go faster

   for(S32 i = 0; i < from.size(); i++)
      to[i] = from[i]->newCopy();
}


// Save the current state of the editor objects for later undoing
void EditorUserInterface::saveUndoState()
{
   // Use case: We do 5 actions, save, undo 2, redo 1, then do some new action.  
   // Our "no need to save" undo point is lost forever.
   if(mAllUndoneUndoLevel > mLastRedoIndex)     
      mAllUndoneUndoLevel = NONE;

   copyItems(mItems, mUndoItems[mLastUndoIndex % UNDO_STATES]);

   mLastUndoIndex++;
   mLastRedoIndex++; 


   if(mLastUndoIndex % UNDO_STATES == mFirstUndoIndex % UNDO_STATES)           // Undo buffer now full...
   {
      mFirstUndoIndex++;
      mAllUndoneUndoLevel -= 1;     // If this falls below 0, then we can't undo our way out of needing to save
   }
   
   mNeedToSave = (mAllUndoneUndoLevel != mLastUndoIndex);
   mRedoingAnUndo = false;
   mLastUndoStateWasBarrierWidthChange = false;
}


void EditorUserInterface::autoSave()
{
   saveLevel(false, false, true);
}


void EditorUserInterface::undo(bool addToRedoStack)
{
   if(!undoAvailable())
      return;

   mSnapVertex_i = NULL;
   mSnapVertex_j = NONE;

   if(mLastUndoIndex == mLastRedoIndex && !mRedoingAnUndo)
   {
      saveUndoState();
      mLastUndoIndex--;
      mLastRedoIndex--;
      mRedoingAnUndo = true;
   }

   mLastUndoIndex--;
   copyItems(mUndoItems[mLastUndoIndex % UNDO_STATES], mItems);

   rebuildEverything();

   mLastUndoStateWasBarrierWidthChange = false;
   validateLevel();
}
   

void EditorUserInterface::redo()
{
   if(mLastRedoIndex != mLastUndoIndex)      // If there's a redo state available...
   {
      mSnapVertex_i = NULL;
      mSnapVertex_j = NONE;

      mLastUndoIndex++;
      copyItems(mUndoItems[mLastUndoIndex % UNDO_STATES], mItems);   

      rebuildEverything();
      validateLevel();
   }
}


void EditorUserInterface::rebuildEverything()
{
   wallSegmentManager.recomputeAllWallGeometry();
   recomputeAllEngineeredItems();
   rebuildAllBorderSegs();

   mNeedToSave = (mAllUndoneUndoLevel != mLastUndoIndex);
   mItemToLightUp = NULL;
   autoSave();
}


// Find mount point or turret or forcefield closest to pos
static Point snapEngineeredObject(EditorObject *object, const Point &pos)
{  
   EngineeredObject *engrObj = dynamic_cast<EngineeredObject *>(object);
   TNLAssert(engrObj, "snapEngineeredObject should only be called with an EngineeredObject!");

   Point anchor, nrml;

   DatabaseObject *mountSeg = engrObj->findAnchorPointAndNormal(object->getGridDatabase(), pos, 
                     EngineeredObject::MAX_SNAP_DISTANCE / getGridSize(), false, EditorWallSegmentType, anchor, nrml);

   if(mountSeg)   // Found a segment we can mount to
   {
      object->setVert(anchor, 0);
      object->mAnchorNormal.set(nrml);
      object->findForceFieldEnd();
      object->forceFieldMountSegment = dynamic_cast<WallSegment *>(mountSeg);
      object->mSnapped = true;
      return anchor;
   }
   else           // No suitable segments found
   {
      object->mSnapped = false;
      return pos;
   }
}


void EditorUserInterface::recomputeAllEngineeredItems()
{
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->getObjectTypeMask() & ItemTurret || mItems[i]->getObjectTypeMask() & ItemForceField)
         snapEngineeredObject(mItems[i], mItems[i]->getVert(0));
}


bool EditorUserInterface::undoAvailable()
{
   return mLastUndoIndex - mFirstUndoIndex != 1;
}


// Wipe undo/redo history
void EditorUserInterface::clearUndoHistory()
{
   mFirstUndoIndex = 0;
   mLastUndoIndex = 1;
   mLastRedoIndex = 1;
   mRedoingAnUndo = false;
}


bool EditorUserInterface::isFlagGame(char *mGameType)
{
   TNL::Object *theObject = TNL::Object::create(mGameType);  // Instantiate a gameType object
   GameType *gameType = dynamic_cast<GameType*>(theObject);  // and cast it

   if(!gameType)
      return false;
   else
      return gameType->isFlagGame();
}


bool EditorUserInterface::isTeamFlagGame(char *mGameType)
{
   TNL::Object *theObject = TNL::Object::create(mGameType);  // Instantiate a gameType object
   GameType *gameType = dynamic_cast<GameType*>(theObject);  // and cast it

   if(!gameType)
      return false;
   else
      return gameType->isTeamFlagGame();
}


extern TeamPreset gTeamPresets[];

void EditorUserInterface::setLevelFileName(string name)
{
   if(name == "")
      mEditFileName = "";
   else
      if(name.find('.') == std::string::npos)      // Append extension, if one is needed
         mEditFileName = name + ".level";
      // else... what?
}


void EditorUserInterface::setLevelGenScriptName(string line)
{
   mScriptLine = trim(line);
}


void EditorUserInterface::makeSureThereIsAtLeastOneTeam()
{
   if(mTeams.size() == 0)
   {
      TeamEditor t;
      t.setName(gTeamPresets[0].name);
      t.color.set(gTeamPresets[0].r, gTeamPresets[0].g, gTeamPresets[0].b);
      mTeams.push_back(t);
   }
}


// This sort will put points on top of lines on top of polygons...  as they should be
// NavMeshZones are now drawn on top, to make them easier to see.  Disable with Ctrl-A!
// We'll also put walls on the bottom, as this seems to work best in practice
S32 QSORT_CALLBACK geometricSort(EditorObject **a, EditorObject **b)
{
   if((*a)->getObjectTypeMask() & ItemBarrierMaker)
      return -1;
   if((*b)->getObjectTypeMask() & ItemBarrierMaker)
      return 1;

   return( (*b)->getGeomType() - (*a)->getGeomType() );
}


extern const char *gGameTypeNames[];
extern S32 gDefaultGameTypeIndex;
extern S32 gMaxPolygonPoints;
extern ConfigDirectories gConfigDirs;

// Loads a level
void EditorUserInterface::loadLevel()
{
   // Initialize
   mItems.clear();
   mTeams.clear();
   mSnapVertex_i = NULL;
   mSnapVertex_j = NONE;
   mAddingVertex = false;
   clearLevelGenItems();
   mLoadTarget = &mItems;
   mGameTypeArgs.clear();
   gGameParamUserInterface.gameParams.clear();
   gGameParamUserInterface.savedMenuItems.clear();          // clear() because this is not a pointer vector
   gGameParamUserInterface.menuItems.deleteAndClear();      // Keeps interface from using our menuItems to rebuild savedMenuItems
   gEditorGame->setGridSize(Game::DefaultGridSize);         // Used in editor for scaling walls and text items appropriately

   mGameType[0] = 0;                   // Clear mGameType
   char fileBuffer[1024];
   dSprintf(fileBuffer, sizeof(fileBuffer), "%s/%s", gConfigDirs.levelDir.c_str(), mEditFileName.c_str());

   if(loadLevelFromFile(fileBuffer))   // Process level file --> returns true if file found and loaded, false if not (assume it's a new level)
   {
      // Loaded a level!
      makeSureThereIsAtLeastOneTeam(); // Make sure we at least have one team
      validateTeams();                 // Make sure every item has a valid team
      validateLevel();                 // Check level for errors (like too few spawns)
      mItems.sort(geometricSort);
      gGameParamUserInterface.ignoreGameParams = false;
   }
   else     
   {
      // New level!
      makeSureThereIsAtLeastOneTeam();                               // Make sure we at least have one team, like the man said.
      strcpy(mGameType, gGameTypeNames[gDefaultGameTypeIndex]);
      gGameParamUserInterface.gameParams.push_back("GameType 10 8"); // A nice, generic game type that we can default to

      if(gIniSettings.name != gIniSettings.defaultName)
         gGameParamUserInterface.gameParams.push_back("LevelCredits " + gIniSettings.name);  // Prepoluate level credits

      gGameParamUserInterface.ignoreGameParams = true;               // Don't rely on the above for populating GameParameters menus... only to make sure something is there if we save
   }
   clearUndoHistory();                 // Clean out undo/redo buffers
   clearSelection();                   // Nothing starts selected
   mShowMode = ShowAllObjects;         // Turn everything on
   mNeedToSave = false;                // Why save when we just loaded?
   mAllUndoneUndoLevel = mLastUndoIndex;
   populateDock();                     // Add game-specific items to the dock

   // Bulk-process new items, walls first
   for(S32 i = 0; i < mItems.size(); i++)
      mItems[i]->processEndPoints();

   wallSegmentManager.recomputeAllWallGeometry();
   
   // Bulk-process bot nav mesh zone boundaries
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->getObjectTypeMask() & ItemNavMeshZone)
          mItems[i]->initializePolyGeom();

   gEditorUserInterface.rebuildAllBorderSegs();


   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->getObjectTypeMask() & ItemTurret || mItems[i]->getObjectTypeMask() & ItemForceField)
        snapEngineeredObject(mItems[i], mItems[i]->getVert(0));

   // And hand-process all other items
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->getObjectTypeMask() & ~ItemBarrierMaker && mItems[i]->getObjectTypeMask() & ~ItemNavMeshZone)
         mItems[i]->onGeomChanged(getCurrentScale());
}


// Objects created with this method MUST be deleted!
// Returns NULL if className is invalid
static EditorObject *newEditorObject(const char *className)
{
   Object *theObject = Object::create(className);        // Create an object of the specified type
   TNLAssert(dynamic_cast<EditorObject *>(theObject), "invalid object!");

   return dynamic_cast<EditorObject *>(theObject);       // Force our new object to be an EditorObject
}


// Process a line read from level file
//void EditorUserInterface::processLevelLoadLine(U32 argc, U32 id, const char **argv)
//{
//   S32 strlenCmd = (S32) strlen(argv[0]);
//
//   // Parse GameType line... All game types are of form XXXXGameType
//   if(strlenCmd >= 8 && !strcmp(argv[0] + strlenCmd - 8, "GameType"))
//   {
//      strcpy(gEditorUserInterface.mGameType, GameType::validateGameType(argv[0]) );    // validateGameType will return a valid game type, regradless of what's put in
//
//      if(strcmp(gEditorUserInterface.mGameType, argv[0]))      // If these differ, then what we put in was invalid
//         gEditorUserInterface.setWarnMessage("Invalid or missing GameType parameter", "Press [F3] to configure level");
//
//      // Save the args (which we already have split out) for easier handling in the Game Parameter Editor
//      for(U32 i = 1; i < argc; i++)
//         gEditorUserInterface.mGameTypeArgs.push_back(argv[i]);
//   }
//
//   else if(!strcmp(argv[0], "GridSize"))
//   {
//      if(argc >= 1)
//         editorGame->setGridSize((F32) atof(argv[1]));
//   }
//
//   else if(!strcmp(argv[0], "Script"))
//   {
//      gEditorUserInterface.mScriptLine = "";
//      // Munge everything into a string.  We'll have to parse after editing in GameParamsMenu anyway.
//      for(U32 i = 1; i < argc; i++)
//      {
//         if(i > 1)
//            gEditorUserInterface.mScriptLine += " ";
//
//         gEditorUserInterface.mScriptLine += argv[i];
//      }
//   }
//
//   // Parse Team definition line
//   else if(!strcmp(argv[0], "Team"))
//   {
//      if(mTeams.size() >= GameType::gMaxTeams)     // Ignore teams with numbers higher than 9
//         return;
//
//      TeamEditor team;
//      team.readTeamFromLevelLine(argc, argv);
//
//      // If team was read and processed properly, numPlayers will be 0
//      if(team.numPlayers != -1)
//         mTeams.push_back(team);
//   }
//
//   else
//   {
//      string objectType = argv[0];
//      S32 skipArgs = 0;
//
//      GameItems itemType = ItemInvalid;
//
//      if(objectType == "BarrierMakerS")
//      {
//         objectType = "PolyWall";
//         skipArgs = 1;
//      }
//
//      for(S32 index = 0; itemDef[index].name != NULL; index++)
//         if(objectType == itemDef[index].name)
//         {
//            itemType = static_cast<GameItems>(index);
//            break;
//         }
//
//      if(itemType != ItemInvalid)     
//      {
//         EditorObject *newItem = newEditorObject(objectType.c_str());
//         newItem->setItemId(id);
//
//         if(newItem->processArguments(argc - 1 - skipArgs, argv + 1 + skipArgs))
//            mLoadTarget->push_back(newItem);           // Don't add to editor if not valid...
//         else
//            delete newItem;
//
//         return;
//      }
//   }
//
//   // What remains are various game parameters...  Note that we will hit this block even if we already looked at gridSize and such...
//   // Before copying, we'll make a dumb copy, which will be overwritten if the user goes into the GameParameters menu
//   // This will cover us if the user comes in, edits the level, saves, and exits without visiting the GameParameters menu
//   // by simply echoing all the parameters back out to the level file without further processing or review.
//   string temp;
//   for(U32 i = 0; i < argc; i++)
//   {
//      temp += argv[i];
//      if(i < argc - 1)
//         temp += " ";
//   }
//
//   gGameParamUserInterface.gameParams.push_back(temp);
//}    


extern OGLCONSOLE_Console gConsole;

void EditorUserInterface::clearLevelGenItems()
{
   mLevelGenItems.deleteAndClear();
}


extern void removeCollinearPoints(Vector<Point> &points, bool isPolygon);

void EditorUserInterface::copyScriptItemsToEditor()
{
   if(mLevelGenItems.size() == 0)
      return;     // Print error message?

   saveUndoState();

   Vector<EditorObject> zones;

   for(S32 i = 0; i < mLevelGenItems.size(); i++)
      mItems.push_back(mLevelGenItems[i]);

   mLevelGenItems.clear();    // Don't want to delete these objects... we just handed them off to mItems!

   rebuildEverything();

   mLastUndoStateWasBarrierWidthChange = false;
}


void EditorUserInterface::runLevelGenScript()
{
   // Parse mScriptLine 
   if(mScriptLine == "")      // No script included!!
      return;

   OGLCONSOLE_Output(gConsole, "Running script %s\n", mScriptLine.c_str());

   Vector<string> scriptArgs = parseString(mScriptLine);
   
   string scriptName = scriptArgs[0];
   scriptArgs.erase(0);

   clearLevelGenItems();      // Clear out any items from the last run

   // Set the load target to the levelgen list, as that's where we want our items stored
   mLoadTarget = &mLevelGenItems;

   runScript(scriptName, scriptArgs);

   // Reset the target
   mLoadTarget = &mItems;
}


// Runs an arbitrary lua script.  Command is first item in cmdAndArgs, subsequent items are the args, if any
void EditorUserInterface::runScript(const string &scriptName, const Vector<string> &args)
{
   string name = ConfigDirectories::findLevelGenScript(scriptName);  // Find full name of levelgen script

   if(name == "")
   {
      logprintf(LogConsumer::LogWarning, "Warning: Could not find script \"%s\"",  scriptName.c_str());
      // TODO: Show an error to the user
      return;
   }

   // Load the items
   LuaLevelGenerator(name, args, gEditorGame->getGridSize(), getGridDatabase(), this, gConsole);
   
   // Process new items
   // Not sure about all this... may need to test
   // Bulk-process new items, walls first
   for(S32 i = 0; i < mLoadTarget->size(); i++)
      if((*mLoadTarget)[i]->getObjectTypeMask() & ItemBarrierMaker || (*mLoadTarget)[i]->getObjectTypeMask() & ItemPolyWall)
      {
         if((*mLoadTarget)[i]->getVertCount() < 2)      // Invalid item; delete
         {
            delete (*mLoadTarget)[i];
            (*mLoadTarget).erase_fast(i);
            i--;
         }

         (*mLoadTarget)[i]->processEndPoints();
      }
}


void EditorUserInterface::validateLevel()
{
   bool hasError = false;
   mLevelErrorMsgs.clear();
   mLevelWarnings.clear();

   bool foundSoccerBall = false;
   bool foundNexus = false;
   bool foundTeamFlags = false;
   bool foundTeamFlagSpawns = false;
   bool foundFlags = false;
   S32 foundFlagCount = 0;
   bool foundNeutralSpawn = false;

   vector<bool> foundSpawn;
   char buf[32];

   string teamList, teams;

   // First, catalog items in level
   for(S32 i = 0; i < mTeams.size(); i++)      // Initialize vector
      foundSpawn.push_back(false);

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i]->getObjectTypeMask() & ItemSpawn && mItems[i]->getTeam() == TEAM_NEUTRAL)
         foundNeutralSpawn = true;
      else if(mItems[i]->getObjectTypeMask() & ItemSpawn && mItems[i]->getTeam() >= 0)
         foundSpawn[mItems[i]->getTeam()] = true;
      else if(mItems[i]->getObjectTypeMask() & ItemSoccerBall)
         foundSoccerBall = true;
      else if(mItems[i]->getObjectTypeMask() & ItemNexus)
         foundNexus = true;
      else if(mItems[i]->getObjectTypeMask() & ItemFlag)
      {
         foundFlags = true;
         foundFlagCount++;
         if(mItems[i]->getTeam() >= 0)
            foundTeamFlags = true;
      }
      else if(mItems[i]->getObjectTypeMask() & ItemFlagSpawn)
      {
         if(mItems[i]->getTeam() >= 0)
            foundTeamFlagSpawns = true;
      }
   }


   // "Unversal errors" -- levelgens can't (yet) change gametype

   // Check for soccer ball in a a game other than SoccerGameType. Doesn't crash no more.
   if(foundSoccerBall && strcmp(mGameType, "SoccerGameType"))
      mLevelWarnings.push_back("WARNING: Soccer ball can only be used in soccer game.");

   // Check for the nexus object in a non-hunter game. Does not affect gameplay in non-hunter game.
   if(foundNexus && strcmp(mGameType, "HuntersGameType"))
      mLevelWarnings.push_back("WARNING: Nexus object can only be used in Hunters game.");

   // Check for missing nexus object in a hunter game.  This cause mucho dolor!
   if(!foundNexus && !strcmp(mGameType, "HuntersGameType"))
      mLevelErrorMsgs.push_back("ERROR: Nexus game must have a Nexus.");

   if(foundFlags && !isFlagGame(mGameType))
      mLevelWarnings.push_back("WARNING: This game type does not use flags.");
   else if(foundTeamFlags && !isTeamFlagGame(mGameType))
      mLevelWarnings.push_back("WARNING: This game type does not use team flags.");

   // Check for team flag spawns on games with no team flags
   if(foundTeamFlagSpawns && !foundTeamFlags)
      mLevelWarnings.push_back("WARNING: Found team flag spawns but no team flags.");

   // Errors that may be corrected by levelgen -- script could add spawns
   // Neutral spawns work for all; if there's one, then that will satisfy our need for spawns for all teams
   if(mScriptLine == "" && !foundNeutralSpawn)
   {
      // Make sure each team has a spawn point
      for(S32 i = 0; i < (S32)foundSpawn.size(); i++)
         if(!foundSpawn[i])
         {
            dSprintf(buf, sizeof(buf), "%d", i+1);

            if(!hasError)     // This is our first error
            {
               teams = "team ";
               teamList = buf;
            }
            else
            {
               teams = "teams ";
               teamList += ", ";
               teamList += buf;
            }
            hasError = true;
         }
   }

   if(hasError)     // Compose error message
      mLevelErrorMsgs.push_back("ERROR: Need spawn point for " + teams + teamList);
}


// Check that each item has a valid team  (fixes any problems it finds)
void EditorUserInterface::validateTeams()
{
   S32 teams = mTeams.size();

   for(S32 i = 0; i < mItems.size(); i++)
   {
      S32 team = mItems[i]->getTeam();

      if(mItems[i]->hasTeam() && ((team >= 0 && team < teams) || team == TEAM_NEUTRAL || team == TEAM_HOSTILE))  
         continue;      // This one's OK

      if(team == TEAM_NEUTRAL && mItems[i]->canBeNeutral())
         continue;      // This one too

      if(team == TEAM_HOSTILE && mItems[i]->canBeHostile())
         continue;      // This one too

      if(mItems[i]->hasTeam())
         mItems[i]->setTeam(0);               // We know there's at least one team, so there will always be a team 0
      else if(mItems[i]->canBeHostile() && !mItems[i]->canBeNeutral())
         mItems[i]->setTeam(TEAM_HOSTILE); 
      else
         mItems[i]->setTeam(TEAM_NEUTRAL);    // We won't consider the case where hasTeam == canBeNeutral == canBeHostile == false
   }
}


// Search through editor objects, to make sure everything still has a valid team.  If not, we'll assign it a default one.
// Note that neutral/hostile items are on team -1/-2, and will be unaffected by this loop or by the number of teams we have.
void EditorUserInterface::teamsHaveChanged()
{
   bool teamsChanged = false;

   if(mTeams.size() != mOldTeams.size())
      teamsChanged = true;
   else
      for(S32 i = 0; i < mTeams.size(); i++)
         if(mTeams[i].color != mOldTeams[i].color || mTeams[i].getName() != mOldTeams[i].getName())
         {
            teamsChanged = true;
            break;
         }

   if(!teamsChanged)       // Nothing changed, we're done here
      return;

   for (S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->getTeam() >= mTeams.size())       // Team no longer valid?
         mItems[i]->setTeam(0);                    // Then set it to first team

   // And the dock items too...
   for (S32 i = 0; i < mDockItems.size(); i++)
      if(mDockItems[i]->getTeam() >= mTeams.size())
         mDockItems[i]->setTeam(0);

   validateLevel();          // Revalidate level -- if teams have changed, requirements for spawns have too
   mNeedToSave = true;
   autoSave();
   mAllUndoneUndoLevel = -1; // This change can't be undone
}


string EditorUserInterface::getLevelFileName()
{
   return mEditFileName;
}


// Handle console input
// Valid commands: help, run, clear, quit, exit
void processEditorConsoleCommand(OGLCONSOLE_Console console, char *cmdline)
{
   Vector<string> words = parseString(cmdline);
   if(words.size() == 0)
      return;

   string cmd = lcase(words[0]);

   if(cmd == "quit" || cmd == "exit") 
      OGLCONSOLE_HideConsole();

   else if(cmd == "help" || cmd == "?") 
      OGLCONSOLE_Output(console, "Commands: help; run; clear; quit\n");

   else if(cmd == "run")
   {
      if(words.size() == 1)      // Too few args
         OGLCONSOLE_Output(console, "Usage: run <script_name> {args}\n");
      else
      {
         gEditorUserInterface.saveUndoState();
         words.erase(0);         // Get rid of "run", leaving script name and args

         string name = words[0];
         words.erase(0);

         gEditorUserInterface.onBeforeRunScriptFromConsole();
         gEditorUserInterface.runScript(name, words);
         gEditorUserInterface.onAfterRunScriptFromConsole();
      }
   }   

   else if(cmd == "clear")
      gEditorUserInterface.clearLevelGenItems();

   else
      OGLCONSOLE_Output(console, "Unknown command: %s\n", cmd.c_str());
}


void EditorUserInterface::onBeforeRunScriptFromConsole()
{
   // Use selection as a marker -- will have to change in future
   for(S32 i = 0; i < mItems.size(); i++)
      mItems[i]->setSelected(true);
}


void EditorUserInterface::onAfterRunScriptFromConsole()
{
   for(S32 i = 0; i < mItems.size(); i++)
      mItems[i]->setSelected(!mItems[i]->isSelected());

   rebuildEverything();
}


extern void actualizeScreenMode(bool, bool = false);

void EditorUserInterface::onActivate()
{
   if(gConfigDirs.levelDir == "")      // Never did resolve a leveldir... no editing for you!
   {
      gEditorUserInterface.reactivatePrevUI();     // Must come before the error msg, so it will become the previous UI when that one exits

      gErrorMsgUserInterface.reset();
      gErrorMsgUserInterface.setTitle("HOUSTON, WE HAVE A PROBLEM");
      gErrorMsgUserInterface.setMessage(1, "No valid level folder was found..."); 
      gErrorMsgUserInterface.setMessage(2, "cannot start the level editor");
      gErrorMsgUserInterface.setMessage(4, "Check the LevelDir parameter in your INI file,");
      gErrorMsgUserInterface.setMessage(5, "or your command-line parameters to make sure");
      gErrorMsgUserInterface.setMessage(6, "you have correctly specified a valid folder.");
      gErrorMsgUserInterface.activate();

      return;
   }

   // Check if we have a level name:
   if(getLevelFileName() == "")         // We need to take a detour to get a level name
   {
      // Don't save this menu (false, below).  That way, if the user escapes out, and is returned to the "previous"
      // UI, they will get back to where they were before (prob. the main menu system), not back to here.
      gLevelNameEntryUserInterface.activate(false);

      return;
   }

   mLevelErrorMsgs.clear();
   mLevelWarnings.clear();

   mSaveMsgTimer.clear();

   mGameTypeArgs.clear();

   mHasBotNavZones = false;

   loadLevel();
   setCurrentTeam(0);

   mSnapDisabled = false;      // Hold [space] to temporarily disable snapping

   // Reset display parameters...
   centerView();
   mDragSelecting = false;
   mUp = mDown = mLeft = mRight = mIn = mOut = false;
   mCreatingPoly = false;
   mCreatingPolyline = false;
   mDraggingObjects = false;
   mDraggingDockItem = NONE;
   mCurrentTeam = 0;
   mShowingReferenceShip = false;
   entryMode = EntryNone;

   mItemToLightUp = NULL;    

   mEditingSpecialAttrItem = NULL;
   mSpecialAttribute = NoAttribute;

   mSaveMsgTimer = 0;

   OGLCONSOLE_EnterKey(processEditorConsoleCommand);     // Setup callback for processing console commands


   actualizeScreenMode(false); 
}


void EditorUserInterface::onDeactivate()
{
   mDockItems.deleteAndClear();     // Free some memory -- dock will be rebuilt when editor restarts
   actualizeScreenMode(true);
}


void EditorUserInterface::onReactivate()
{
   mDraggingObjects = false;  

   mEditingSpecialAttrItem = NULL;     // Probably not necessary
   mSpecialAttribute = NoAttribute;

   if(mWasTesting)
   {
      mWasTesting = false;
      mSaveMsgTimer.clear();
   }

   remove("editor.tmp");      // Delete temp file

   if(mCurrentTeam >= mTeams.size())
      mCurrentTeam = 0;

   OGLCONSOLE_EnterKey(processEditorConsoleCommand);     // Restore callback for processing console commands

   actualizeScreenMode(true);
}


static Point sCenter;

// Called when we shift between windowed and fullscreen mode, before change is made
void EditorUserInterface::onPreDisplayModeChange()
{
   sCenter.set(mCurrentOffset.x - gScreenInfo.getGameCanvasWidth() / 2, mCurrentOffset.y - gScreenInfo.getGameCanvasHeight() / 2);
}

// Called when we shift between windowed and fullscreen mode, after change is made
void EditorUserInterface::onDisplayModeChange()
{
   // Recenter canvas -- note that canvasWidth may change during displayMode change
   mCurrentOffset.set(sCenter.x + gScreenInfo.getGameCanvasWidth() / 2, sCenter.y + gScreenInfo.getGameCanvasHeight() / 2);

   populateDock();               // If game type has changed, items on dock will change
}


Point EditorUserInterface::snapPointToLevelGrid(Point const &p)
{
   if(mSnapDisabled)
      return p;

   // First, find a snap point based on our grid
   F32 mulFactor, divFactor;
   if(mCurrentScale >= 100)
   {
      mulFactor = 10;
      divFactor = 0.1;
   }
   else
   {
      mulFactor = 2;
      divFactor = 0.5;
   }

   return Point(floor(p.x * mulFactor + 0.5) * divFactor, floor(p.y * mulFactor + 0.5) * divFactor);
}


Point EditorUserInterface::snapPoint(Point const &p, bool snapWhileOnDock)
{
   if(mouseOnDock() && !snapWhileOnDock) 
      return p;      // No snapping!

   Point snapPoint(p);

   if(mDraggingObjects)
   {
      // Mark all items being dragged as no longer being snapped -- only our primary "focus" item will be snapped
      for(S32 i = 0; i < mItems.size(); i++)
         if(mItems[i]->isSelected())
            mItems[i]->setSnapped(false);
   }
   
   // Turrets & forcefields: Snap to a wall edge as first (and only) choice
   if(mDraggingObjects &&
            (mSnapVertex_i->getObjectTypeMask() & ItemTurret || mSnapVertex_i->getObjectTypeMask() & ItemForceField))
      return snapEngineeredObject(mSnapVertex_i, snapPointToLevelGrid(p));


   F32 maxSnapDist = 100 / (mCurrentScale * mCurrentScale);
   F32 minDist = maxSnapDist;

   // Where will we be snapping things?
   bool snapToWallCorners = !mSnapDisabled && mDraggingObjects && !(mSnapVertex_i->getObjectTypeMask() & ItemBarrierMaker) && mSnapVertex_i->getGeomType() != geomPoly;
bool snapToWallEdges = !mSnapDisabled && mSnapVertex_i && false; // TODO: Can delete?
   bool snapToNavZoneEdges = mSnapVertex_i && mSnapVertex_i->getObjectTypeMask() & ItemNavMeshZone;
   bool snapToLevelGrid = !snapToNavZoneEdges && !mSnapDisabled;


   if(snapToLevelGrid)     // Lowest priority
   {
      snapPoint = snapPointToLevelGrid(p);
      minDist = snapPoint.distSquared(p);
   }


   // Now look for other things we might want to snap to
   for(S32 i = 0; i < mItems.size(); i++)
   {
      // Don't snap to selected items or items with selected verts
      if(mItems[i]->isSelected() || mItems[i]->anyVertsSelected())    
         continue;

      for(S32 j = 0; j < mItems[i]->getVertCount(); j++)
      {
         F32 dist = mItems[i]->getVert(j).distSquared(p);
         if(dist < minDist)
         {
            minDist = dist;
            snapPoint.set(mItems[i]->getVert(j));
         }
      }
   }


   // Build a list of walls we might be snapping to if we're snapping to either the edges or corners
   static Vector<DatabaseObject *> foundObjects;

   // Search for a corner to snap to - by using wall edges, we'll also look for intersections between segments
   if(snapToWallCorners)
      checkCornersForSnap(p, WallSegmentManager::mWallEdges, minDist, snapPoint);

   // If we're editing a vertex of a polygon, and if we're outside of some threshold distance, see if we can 
   // snap to the edge of a another zone or wall.  Decreasing value in minDist test will favor snapping to walls, 
   // decreasing(increasing??) it will require being closer to a wall to snap to it.
   if(minDist >= 90 / (mCurrentScale * mCurrentScale))
   {
      if(snapToWallEdges)
         checkEdgesForSnap(p, WallSegmentManager::mWallEdges, false, minDist, snapPoint);
   }

   // Will overwrite snapPoint if a zone corner or edge is found, thus prioritizing zone edges to other things when 
   // snapToNavZoneEdges is true
   if(snapToNavZoneEdges)
   {
      Rect vertexRect(snapPoint, .25); 
      Vector<EditorObject> candidates;
      Point edgeSnapPoint;
      F32 minCornerDist = maxSnapDist;
      F32 minEdgeDist = maxSnapDist;

      for(S32 i = 0; i < mItems.size(); i++)
      {
         if(mItems[i]->getObjectTypeMask() & ~ItemNavMeshZone || mItems[i]->isSelected() || mItems[i]->anyVertsSelected())
            continue;

         if(!mItems[i]->getExtent().intersectsOrBorders(vertexRect))
            continue;
         
         // To close the polygon, we need to repeat our first point at the end
         Vector<Point> verts = mItems[i]->getVerts();     // Makes copy -- TODO: alter checkEdgesforsnap to make
                                                         // copy unnecessary, only needed when 3rd param is true
         verts.push_back(verts.first());

         // Combine these two checks in an awkward manner to reduce cost of checks above
         checkEdgesForSnap(p, verts, true, minEdgeDist, edgeSnapPoint);
         checkCornersForSnap(p, mItems[i]->getVerts(), minCornerDist, snapPoint);

         if(minCornerDist == maxSnapDist && minEdgeDist < maxSnapDist)     // i.e. found edge, but not corner
            snapPoint.set(edgeSnapPoint);
      }
   }

   return snapPoint;
}


extern bool findNormalPoint(const Point &p, const Point &s1, const Point &s2, Point &closest);

static Point closest;      // Reusable container

static bool checkEdge(const Point &clickPoint, const Point &start, const Point &end, F32 &minDist, Point &snapPoint)
{
   if(findNormalPoint(clickPoint, start, end, closest))    // closest is point on line where clickPoint normal intersects
   {
      F32 dist = closest.distSquared(clickPoint);
      if(dist < minDist)
      {
         minDist = dist;
         snapPoint.set(closest);  
         return true;
      }
   }

   return false;
}


// Checks for snapping against a series of edges defined by verts in A-B-C-D format if abcFormat is true, or A-B B-C C-D if false
// Sets snapPoint and minDist.  Returns index of closest segment found if closer than minDist.
S32 EditorUserInterface::checkEdgesForSnap(const Point &clickPoint, const Vector<Point> &verts, bool abcFormat,
                                           F32 &minDist, Point &snapPoint )
{
   S32 inc = abcFormat ? 1 : 2;   
   S32 segFound = NONE;

   for(S32 i = 0; i < verts.size() - 1; i += inc)
      if(checkEdge(clickPoint, verts[i], verts[i+1], minDist, snapPoint))
         segFound = i;

   return segFound;
}


S32 EditorUserInterface::checkEdgesForSnap(const Point &clickPoint, const Vector<WallEdge *> &edges, bool abcFormat,
                                           F32 &minDist, Point &snapPoint )
{
   S32 inc = abcFormat ? 1 : 2;   
   S32 segFound = NONE;

   for(S32 i = 0; i < edges.size(); i++)
   {
      if(checkEdge(clickPoint, *edges[i]->getStart(), *edges[i]->getEnd(), minDist, snapPoint))
         segFound = i;
   }

   return segFound;
}



static bool checkPoint(const Point &clickPoint, const Point &point, F32 &minDist, Point &snapPoint)
{
   F32 dist = point.distSquared(clickPoint);
   if(dist < minDist)
   {
      minDist = dist;
      snapPoint = point;
      return true;
   }

   return false;
}


S32 EditorUserInterface::checkCornersForSnap(const Point &clickPoint, const Vector<Point> &verts, F32 &minDist, Point &snapPoint)
{
   S32 vertFound = NONE;

   for(S32 i = 0; i < verts.size(); i++)
      if(checkPoint(clickPoint, verts[i], minDist, snapPoint))
         vertFound = i;

   return vertFound;
}


S32 EditorUserInterface::checkCornersForSnap(const Point &clickPoint, const Vector<WallEdge *> &edges, F32 &minDist, Point &snapPoint)
{
   S32 vertFound = NONE;
   const Point *vert;

   for(S32 i = 0; i < edges.size(); i++)
      for(S32 j = 0; j < 1; j++)
      {
         vert = (j == 0) ? edges[i]->getStart() : edges[i]->getEnd();
         if(checkPoint(clickPoint, *vert, minDist, snapPoint))
            vertFound = i;
      }

   return vertFound;
}


////////////////////////////////////
////////////////////////////////////
// Rendering routines

extern Color gErrorMessageTextColor;

static const Color grayedOutColorBright = Color(.5, .5, .5);
static const Color grayedOutColorDim = Color(.25, .25, .25);
static bool fillRendered = false;

// Render background snap grid
void EditorUserInterface::renderGrid()
{
   if(mShowingReferenceShip)
      return;   

   F32 colorFact = mSnapDisabled ? .5 : 1;

   // Minor grid lines
   if(mCurrentScale >= 100)
   {
      F32 gridScale = mCurrentScale * 0.1;      // Draw tenths

      F32 xStart = fmod(mCurrentOffset.x, gridScale);
      F32 yStart = fmod(mCurrentOffset.y, gridScale);

      glColor3f(.2 * colorFact, .2 * colorFact, .2 * colorFact);
      glBegin(GL_LINES);
         while(yStart < gScreenInfo.getGameCanvasHeight())
         {
            glVertex2f(0, yStart);
            glVertex2f(gScreenInfo.getGameCanvasWidth(), yStart);
            yStart += gridScale;
         }
         while(xStart < gScreenInfo.getGameCanvasWidth())
         {
            glVertex2f(xStart, 0);
            glVertex2f(xStart, gScreenInfo.getGameCanvasHeight());
            xStart += gridScale;
         }
      glEnd();
   }

   // Major grid lines
   if(mCurrentScale >= 10)
   {
      F32 xStart = fmod(mCurrentOffset.x, mCurrentScale);
      F32 yStart = fmod(mCurrentOffset.y, mCurrentScale);

      glColor3f(0.4 * colorFact, 0.4 * colorFact, 0.4 * colorFact);
      glBegin(GL_LINES);
         while(yStart < gScreenInfo.getGameCanvasHeight())
         {
            glVertex2f(0, yStart);
            glVertex2f(gScreenInfo.getGameCanvasWidth(), yStart);
            yStart += mCurrentScale;
         }
         while(xStart < gScreenInfo.getGameCanvasWidth())
         {
            glVertex2f(xStart, 0);
            glVertex2f(xStart, gScreenInfo.getGameCanvasHeight());
            xStart += mCurrentScale;
         }
      glEnd();
   }

   // Draw main axes through origin
   glColor3f(0.7 * colorFact, 0.7 * colorFact, 0.7 * colorFact);
   glLineWidth(gLineWidth3);
   Point origin = convertLevelToCanvasCoord(Point(0,0));
   glBegin(GL_LINES );
      glVertex2f(0, origin.y);
      glVertex2f(gScreenInfo.getGameCanvasWidth(), origin.y);
      glVertex2f(origin.x, 0);
      glVertex2f(origin.x, gScreenInfo.getGameCanvasHeight());
   glEnd();
   glLineWidth(gDefaultLineWidth);
}


extern ScreenInfo gScreenInfo;

S32 getDockHeight(ShowMode mode)
{
   if(mode == ShowWallsOnly)
      return 62;
   else if(mode == NavZoneMode)
      return 92;
   else  // mShowMode == ShowAllObjects || mShowMode == ShowAllButNavZones
      return gScreenInfo.getGameCanvasHeight() - 2 * EditorUserInterface::vertMargin;
}


void EditorUserInterface::renderDock(F32 width)    // width is current wall width, used for displaying info on dock
{
   // Render item dock down RHS of screen
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   S32 dockHeight = getDockHeight(mShowMode);

   for(S32 i = 1; i >= 0; i--)
   {
      glColor(i ? black : (mouseOnDock() ? yellow : white));       

      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f(canvasWidth - DOCK_WIDTH - horizMargin, canvasHeight - vertMargin);
         glVertex2f(canvasWidth - horizMargin,              canvasHeight - vertMargin);
         glVertex2f(canvasWidth - horizMargin,              canvasHeight - vertMargin - dockHeight);
         glVertex2f(canvasWidth - DOCK_WIDTH - horizMargin, canvasHeight - vertMargin - dockHeight);
      glEnd();
   }

   // Draw coordinates on dock -- if we're moving an item, show the coords of the snap vertex, otherwise show the coords of the
   // snapped mouse position
   Point pos;
   if(mSnapVertex_i)
      pos = mSnapVertex_i->getVert(mSnapVertex_j);
   else
      pos = snapPoint(convertCanvasToLevelCoord(mMousePos));

   F32 xpos = gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH / 2;

   char text[50];
   glColor(white);
   dSprintf(text, sizeof(text), "%2.2f|%2.2f", pos.x, pos.y);
   drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 15, 8, text);

   // And scale
   dSprintf(text, sizeof(text), "%2.2f", mCurrentScale);
   drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 25, 8, text);

   // Show number of teams
   dSprintf(text, sizeof(text), "Teams: %d",  mTeams.size());
   drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 35, 8, text);

   glColor(mNeedToSave ? red : green);     // Color level name by whether it needs to be saved or not
   dSprintf(text, sizeof(text), "%s%s", mNeedToSave ? "*" : "", mEditFileName.substr(0, mEditFileName.find_last_of('.')).c_str());    // Chop off extension
   drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 45, 8, text);

   // And wall width as needed
   if(width != NONE)
   {
      glColor(white);
      dSprintf(text, sizeof(text), "Width: %2.0f", width);
      drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 55, 8, text);
   }
}


void EditorUserInterface::renderTextEntryOverlay()
{
   // Render id-editing overlay
   if(entryMode != EntryNone)
   {
      static const U32 fontsize = 16;
      static const S32 inset = 9;
      static const S32 boxheight = fontsize + 2 * inset;
      static const Color color(0.9, 0.9, 0.9);
      static const Color errorColor(1, 0, 0);

      bool errorFound = false;

      // Check for duplicate IDs if we're in ID entry mode
      if(entryMode == EntryID)
      {
         U32 id = atoi(mEntryBox.c_str());      // mEntryBox has digits only filter applied; ids can only be positive ints

         if(id != 0)    // Check for duplicates
         {
            for(S32 i = 0; i < mItems.size(); i++)
               if(mItems[i]->getItemId() == id && !mItems[i]->isSelected())
               {
                  errorFound = true;
                  break;
               }
         }
      }

      // Calculate box width
      S32 boxwidth = 2 * inset + getStringWidth(fontsize, mEntryBox.getPrompt().c_str()) + 
          mEntryBox.getMaxLen() * getStringWidth(fontsize, "-") + 25;

      // Render entry box    
      glEnableBlend;
      S32 xpos = (gScreenInfo.getGameCanvasWidth()  - boxwidth) / 2;
      S32 ypos = (gScreenInfo.getGameCanvasHeight() - boxheight) / 2;

      for(S32 i = 1; i >= 0; i--)
      {
         glColor(Color(.3,.6,.3), i ? .85 : 1);

         glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
            glVertex2f(xpos,            ypos);
            glVertex2f(xpos + boxwidth, ypos);
            glVertex2f(xpos + boxwidth, ypos + boxheight);
            glVertex2f(xpos,            ypos + boxheight);
         glEnd();
      }
      glDisableBlend;

      xpos += inset;
      ypos += inset;
      glColor(errorFound ? errorColor : color);
      xpos += drawStringAndGetWidthf(xpos, ypos, fontsize, "%s ", mEntryBox.getPrompt().c_str());
      drawString(xpos, ypos, fontsize, mEntryBox.c_str());
      mEntryBox.drawCursor(xpos, ypos, fontsize);
   }
}


void EditorUserInterface::renderReferenceShip()
{
   // Render ship at cursor to show scale
   static F32 thrusts[4] =  { 1, 0, 0, 0 };

   glPushMatrix();
      glTranslatef(mMousePos.x, mMousePos.y, 0);
      glScalef(mCurrentScale / getGridSize(), mCurrentScale / getGridSize(), 1);
      glRotatef(90, 0, 0, 1);
      renderShip(red, 1, thrusts, 1, 5, 0, false, false, false, false);
      glRotatef(-90, 0, 0, 1);

      // And show how far it can see
      F32 horizDist = Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL;
      F32 vertDist = Game::PLAYER_VISUAL_DISTANCE_VERTICAL;

      glEnableBlend;     // Enable transparency
      glColor4f(.5, .5, 1, .35);
      glBegin(GL_POLYGON);
         glVertex2f(-horizDist, -vertDist);
         glVertex2f(horizDist, -vertDist);
         glVertex2f(horizDist, vertDist);
         glVertex2f(-horizDist, vertDist);
      glEnd();
      glDisableBlend;

   glPopMatrix();
}


static F32 getRenderingAlpha(bool isScriptItem)
{
   return isScriptItem ? .6 : 1;     // Script items will appear somewhat translucent
}


const char *getModeMessage(ShowMode mode)
{
   if(mode == ShowWallsOnly)
      return "Wall editing mode.  Hit Ctrl-A to change.";
   else if(mode == ShowAllButNavZones)
      return "NavMesh zones hidden.  Hit Ctrl-A to change.";
   else if(mode == NavZoneMode)
      return "NavMesh editing mode.  Hit Ctrl-A to change.";
   else     // Normal mode
      return "";
}


extern void renderTriangulatedPolygonFill(const Vector<Point> &fill);
extern void renderPolygonOutline(const Vector<Point> &outline);
extern void renderPolygonOutline(const Vector<Point> &outlinePoints, Color &outlineColor, F32 alpha = 1);

extern void renderPolygonFill(const Vector<Point> *fillPoints, const Color &fillColor, F32 alpha = 1);
extern void renderPolygonFill(const Vector<Point> &fillPoints, const Color &fillColor, F32 alpha = 1);

void EditorUserInterface::render()
{
   mouseIgnore = false; // Needed to avoid freezing effect from too many mouseMoved events without a render in between (sam)

   renderGrid();     // Render first, so it's at the bottom

   // Render any items generated by the levelgen script... these will be rendered below normal items. 
   glPushMatrix();
      setLevelToCanvasCoordConversion();

      glColor(Color(0,.25,0));
      for(S32 i = 0; i < mLevelGenItems.size(); i++)
         if(mLevelGenItems[i]->getObjectTypeMask() & ItemBarrierMaker)
            for(S32 j = 0; j < mLevelGenItems[i]->extendedEndPoints.size(); j+=2)
               renderTwoPointPolygon(mLevelGenItems[i]->extendedEndPoints[j], mLevelGenItems[i]->extendedEndPoints[j+1], 
                                     mLevelGenItems[i]->getWidth() / getGridSize() / 2, GL_POLYGON);
   glPopMatrix();

   for(S32 i = 0; i < mLevelGenItems.size(); i++)
      mLevelGenItems[i]->render(true, mShowingReferenceShip, mShowMode);
   
   // Render polyWall item fill just before rendering regular walls.  This will create the effect of all walls merging together.  
   // PolyWall outlines are already part of the wallSegmentManager, so will be rendered along with those of regular walls.

   glPushMatrix();  
      setLevelToCanvasCoordConversion();
      for(S32 i = 0; i < mItems.size(); i++)
         if(mItems[i]->getObjectTypeMask() & ItemPolyWall)
            renderPolygonFill(mItems[i]->getPolyFillPoints(), EDITOR_WALL_FILL_COLOR, 1);
   
      wallSegmentManager.renderWalls(true, getRenderingAlpha(false/*isScriptItem*/));
   glPopMatrix();

   // Draw map items (teleporters, etc.) that are not being dragged, and won't have any text labels  (below the dock)
   for(S32 i = 0; i < mItems.size(); i++)
      if(!(mDraggingObjects && mItems[i]->isSelected()))
         mItems[i]->render(false, mShowingReferenceShip, mShowMode);

   // Draw map items (teleporters, etc.) that are are selected and/or lit up, so label is readable (still below the dock)
   // Do this as a separate operation to ensure that these are drawn on top of those drawn above.
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->isSelected() || mItems[i]->isLitUp())
         mItems[i]->render(false, mShowingReferenceShip, mShowMode);


   // Go through and render any borders between navMeshZones -- these need to be rendered after the zones themselves so they
   // don't get covered up.
   if(showingNavZones())
   {
      glPushMatrix();  
         setLevelToCanvasCoordConversion();
         renderNavMeshBorders(zoneBorders, 1 / getGridSize());
      glPopMatrix();
      
      if(!mShowingReferenceShip)
         for(S32 i = 0; i < mItems.size(); i++)
            if(mItems[i]->getObjectTypeMask() & ItemNavMeshZone)
               mItems[i]->renderLinePolyVertices(gEditorUserInterface.getCurrentScale()); 
   }

   fillRendered = false;
   F32 width = NONE;

   if(mCreatingPoly || mCreatingPolyline)    // Draw geomLine features under construction
   {
      mNewItem->addVert(snapPoint(convertCanvasToLevelCoord(mMousePos)));
      glLineWidth(gLineWidth3);

      if(mCreatingPoly) // Wall
         glColor(SELECT_COLOR);
      else              // LineItem
         glColor(getTeamColor(mNewItem->getTeam()));

      renderPolyline(mNewItem->getVerts());

      glLineWidth(gDefaultLineWidth);

      for(S32 j = mNewItem->getVertCount() - 1; j >= 0; j--)      // Go in reverse order so that placed vertices are drawn atop unplaced ones
      {
         Point v = convertLevelToCanvasCoord(mNewItem->getVert(j));

         // Draw vertices
         if(j == mNewItem->getVertCount() - 1)           // This is our most current vertex
            renderVertex(HighlightedVertex, v, NO_NUMBER);
         else
            renderVertex(SelectedItemVertex, v, j);
      }
      mNewItem->deleteVert(mNewItem->getVertCount() - 1);
   }
   // Since we're not constructing a barrier, if there are any barriers or lineItems selected, 
   // get the width for display at bottom of dock
   else  
   {
      for(S32 i = 0; i < mItems.size(); i++)
         if(mItems[i]->hasWidth() && (mItems[i]->isSelected() || (mItems[i]->isLitUp() && mItems[i]->isVertexLitUp(NONE))) )
         {
            width =  mItems[i]->getWidth();
            break;
         }
   }


   if(mShowMode == NavZoneMode)
   {
      glPushMatrix();  
         setLevelToCanvasCoordConversion();
  
         for(S32 i = 0; i < zoneBorders.size(); i++)
            renderNavMeshBorder(zoneBorders[i], 1 / getGridSize(), yellow, .5, .05 * getGridSize());
      glPopMatrix();
   }


   if(mShowingReferenceShip)
      renderReferenceShip();
   else
      renderDock(width);

   // Draw map items (teleporters, etc.) that are being dragged  (above the dock).  But don't draw walls here, or
   // we'll lose our wall centernlines.
   for(S32 i = 0; i < mItems.size(); i++)
      if(!(mItems[i]->getObjectTypeMask() & ItemBarrierMaker) && mDraggingObjects && mItems[i]->isSelected())
         mItems[i]->render(false, mShowingReferenceShip, mShowMode);

   if(mDragSelecting)      // Draw box for selecting items
   {
      glColor(white);
      Point downPos = convertLevelToCanvasCoord(mMouseDownPos);
      glBegin(GL_LINE_LOOP);
         glVertex2f(downPos.x,   downPos.y);
         glVertex2f(mMousePos.x, downPos.y);
         glVertex2f(mMousePos.x, mMousePos.y);
         glVertex2f(downPos.x,   mMousePos.y);
      glEnd();
   }

   // Render messages at bottom of screen
   if(mouseOnDock())    // On the dock?  If so, render help string if hovering over item
   {
      S32 hoverItem = findHitItemOnDock(mMousePos);

      if(hoverItem != NONE)
      {
         mDockItems[hoverItem]->setLitUp(true);    // Will trigger a selection highlight to appear around dock item

         const char *helpString = mDockItems[hoverItem]->getEditorHelpString();

         glColor3f(.1, 1, .1);

         // Center string between left side of screen and edge of dock
         S32 x = (S32)(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH - getStringWidth(15, helpString)) / 2;
         drawString(x, gScreenInfo.getGameCanvasHeight() - vertMargin - 15, 15, helpString);
      }
   }

   // Render dock items
   if(!mShowingReferenceShip)
      for(S32 i = 0; i < mDockItems.size(); i++)
      {
         mDockItems[i]->render(false, mShowingReferenceShip, mShowMode);
         mDockItems[i]->setLitUp(false);
      }

   if(mSaveMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if(mSaveMsgTimer.getCurrent() < 1000)
         alpha = (F32) mSaveMsgTimer.getCurrent() / 1000;

      glEnableBlend;
         glColor(mSaveMsgColor, alpha);
         drawCenteredString(gScreenInfo.getGameCanvasHeight() - vertMargin - 65, 25, mSaveMsg.c_str());
      glDisableBlend;
   }

   if(mWarnMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (mWarnMsgTimer.getCurrent() < 1000)
         alpha = (F32) mWarnMsgTimer.getCurrent() / 1000;

      glEnableBlend;
         glColor(mWarnMsgColor, alpha);
         drawCenteredString(gScreenInfo.getGameCanvasHeight() / 4, 25, mWarnMsg1.c_str());
         drawCenteredString(gScreenInfo.getGameCanvasHeight() / 4 + 30, 25, mWarnMsg2.c_str());
      glDisableBlend;
   }


   if(mLevelErrorMsgs.size() || mLevelWarnings.size())
   {
      S32 ypos = vertMargin + 50;

      glColor(gErrorMessageTextColor);

      for(S32 i = 0; i < mLevelErrorMsgs.size(); i++)
      {
         drawCenteredString(ypos, 20, mLevelErrorMsgs[i].c_str());
         ypos += 25;
      }

      glColor(Color(1,1,0));

      for(S32 i = 0; i < mLevelWarnings.size(); i++)
      {
         drawCenteredString(ypos, 20, mLevelWarnings[i].c_str());
         ypos += 25;
      }
   }

   glColor3f(0,1,1);
   drawCenteredString(vertMargin, 14, getModeMessage(mShowMode));

   renderTextEntryOverlay();

   renderConsole();  // Rendered last, so it's always on top
}


// TODO: Merge with nearly identical version in gameType
Color EditorUserInterface::getTeamColor(S32 team)
{
   if(team == Item::TEAM_NEUTRAL || team >= mTeams.size() || team < Item::TEAM_HOSTILE)
      return gNeutralTeamColor;
   else if(team == Item::TEAM_HOSTILE)
      return gHostileTeamColor;
   else
      return mTeams[team].color;
}


Color EditorObject::getDrawColor()
{
   F32 alpha = 1;

   if(mSelected)
      return SELECT_COLOR;
   else if(mLitUp, alpha)
      return HIGHLIGHT_COLOR;
   else  // Normal
      return Color(.75, .75, .75);
}


// Return a pointer to a new copy of the object.  You will have to delete this copy when you are done with it!
EditorObject *EditorObject::newCopy()
{
   TextItem *textItem = dynamic_cast<TextItem *>(this);
   if(textItem != NULL)
   {
      TextItem *newTextItem = new TextItem(*textItem);
      newTextItem->onGeomChanged(getCurrentScale());
      return newTextItem;
   }

   TNLAssert(false, "Unhandled object in newCopy!");
   EditorObject *newObject = newEditorObject(this->getClassName());
   return newObject;
}


Color EditorObject::getTeamColor(S32 teamId) 
{ 
   return gEditorUserInterface.getTeamColor(teamId);
}


// Draw the vertices for a polygon or line item (i.e. walls)
void EditorObject::renderLinePolyVertices(F32 scale, F32 alpha)
{
   // Draw the vertices of the wall or the polygon area
   for(S32 j = 0; j < getVertCount(); j++)
   {
      Point v = convertLevelToCanvasCoord(mVerts[j]);

      if(mVertSelected[j])
         renderVertex(SelectedVertex, v, j, alpha);             // Hollow yellow boxes with number
      else if(mLitUp && isVertexLitUp(j))
         renderVertex(HighlightedVertex, v, j, alpha);          // Hollow yellow boxes with number
      else if(mSelected || mLitUp || mAnyVertsSelected)
         renderVertex(SelectedItemVertex, v, j, alpha);         // Hollow red boxes with number
      else
         renderVertex(UnselectedItemVertex, v, NO_NUMBER, alpha, scale > 35 ? 2 : 1);   // Solid red boxes, no number
   }
}


////////////////////////////////////////
////////////////////////////////////////
/*
1. User creates wall by drawing line
2. Each line segment is converted to a series of endpoints, who's location is adjusted to improve rendering (extended)
3. Those endpoints are used to generate a series of WallSegment objects, each with 4 corners
4. Those corners are used to generate a series of edges on each WallSegment.  Initially, each segment has 4 corners
   and 4 edges.
5. Segments are intsersected with one another, punching "holes", and creating a series of shorter edges that represent
   the dark blue outlines seen in the game and in the editor.

If wall shape or location is changed steps 1-5 need to be repeated
If intersecting wall is changed, only steps 4 and 5 need to be repeated
If wall thickness is changed, steps 3-5 need to be repeated
*/

static inline void labelSimpleLineItem(Point pos, F32 labelSize, const char *itemLabelTop, const char *itemLabelBottom)
{
   UserInterface::drawStringc(pos.x, pos.y + labelSize + 2, labelSize, itemLabelTop);
   UserInterface::drawStringc(pos.x, pos.y + 2 * labelSize + 5, labelSize, itemLabelBottom);
}


// Will set the correct translation and scale to render items at correct location and scale as if it were a real level.
// Unclear enough??
void EditorUserInterface::setTranslationAndScale(const Point &pos)
{
   F32 gridSize = getGridSize();
   F32 scale = gEditorUserInterface.getCurrentScale();

   glScalef(scale / gridSize, scale / gridSize, 1);
   glTranslatef(pos.x * gridSize / scale - pos.x, pos.y * gridSize / scale - pos.y, 0);
}


bool EditorUserInterface::showingNavZones()
{
   return (mShowMode == ShowAllObjects || mShowMode == NavZoneMode) && !mShowingReferenceShip;
}


extern void renderPolygon(const Vector<Point> &fillPoints, const Vector<Point> &outlinePoints, const Color &fillColor, const Color &outlineColor, F32 alpha = 1);

static const S32 asteroidDesign = 2;      // Design we'll use for all asteroids in editor

// Items are rendered in index order, so those with a higher index get drawn later, and hence, on top
void EditorObject::render(bool isScriptItem, bool showingReferenceShip, ShowMode showMode)    // TODO: pass scale
{
   const S32 instrSize = 9;      // Size of instructions for special items
   const S32 attrSize = 10;

   Point pos, dest;
   F32 alpha = getRenderingAlpha(isScriptItem);

   bool hideit = (showMode == ShowWallsOnly) && !(showingReferenceShip && !mDockItem);

   Color drawColor;
   if(hideit)
      glColor(grayedOutColorBright, alpha);
   else if(mSelected)
      drawColor = SELECT_COLOR;
   else if(mLitUp, alpha)
      drawColor = HIGHLIGHT_COLOR;
   else  // Normal
      drawColor = Color(.75, .75, .75);

   if(getObjectTypeMask() & ItemTextItem)
   {
      TextItem *textItem = dynamic_cast<TextItem *>(this);
      pos = textItem->getVert(0);
   }
   else
      pos = convertLevelToCanvasCoord(getVert(0), !mDockItem);

   glEnableBlend;        // Enable transparency

   // Render snapping vertex; if it is the same as a highlighted vertex, highlight will overwrite this
   if(gEditorUserInterface.getSnapItem() && gEditorUserInterface.getSnapVertexIndex() != NONE && 
            gEditorUserInterface.getSnapItem()->isSelected() && !showingReferenceShip)      
      // Render snapping vertex as hollow magenta box
      renderVertex(SnappingVertex, 
                   convertLevelToCanvasCoord(gEditorUserInterface.getSnapItem()->getVert(gEditorUserInterface.getSnapVertexIndex())), 
                   NO_NUMBER, alpha);   

   // Draw "two-point" items (teleporter, gofast, textitem)
   if(getGeomType() == geomSimpleLine && (showMode != ShowWallsOnly || mDockItem || showingReferenceShip))    
   {
      // Override drawColor for this special case
      if(mAnyVertsSelected)
         drawColor = SELECT_COLOR;
     
         if(mDockItem)
            renderDock();
         else if(showingReferenceShip)
         {
            glPushMatrix();
               setLevelToCanvasCoordConversion();
               GameObject::render();
            glPopMatrix();
         }
         else
         {
            glPushMatrix();
               setLevelToCanvasCoordConversion();
               renderEditor(getCurrentScale());
            glPopMatrix();
         }

   }     //  End draw "two-point" items (teleporter, gofast, textitem)

   //////////

   else if(getObjectTypeMask() & ItemLineItem)
   {
      glColor(getTeamColor(getTeam()), alpha);
      renderPolylineCenterline(alpha);

      if(!showingReferenceShip)
         renderLinePolyVertices(gEditorUserInterface.getCurrentScale(), alpha);
   }

   //////////

   else if(getObjectTypeMask() & ItemBarrierMaker)      
   {
      if(!showingReferenceShip && getObjectTypeMask() & ItemBarrierMaker)
         renderPolylineCenterline(alpha);

      if(!showingReferenceShip)
         renderLinePolyVertices(gEditorUserInterface.getCurrentScale(), alpha);
   } 
   else if(getGeomType() == geomPoly)    // Draw regular line objects and poly objects
   {
      // Hide everything in ShowWallsOnly mode, and hide navMeshZones in ShowAllButNavZones mode, 
      // unless it's a dock item or we're showing the reference ship.  NavMeshZones are hidden when reference ship is shown
      if((showMode != ShowWallsOnly && (gEditorUserInterface.showingNavZones() && getObjectTypeMask() & ItemNavMeshZone || getObjectTypeMask() & ~ItemNavMeshZone)) &&
            !showingReferenceShip || mDockItem || showingReferenceShip && getObjectTypeMask() & ~ItemNavMeshZone)   
      {
         // A few items will get custom colors; most will get their team color
         if(hideit)
            glColor(grayedOutColorDim, alpha);
         else if(getObjectTypeMask() & ItemNexus)
            glColor(gNexusOpenColor, alpha);      // Render Nexus items in pale green to match the actual thing
         else if(getObjectTypeMask() & ItemPolyWall)
            glColor(EDITOR_WALL_FILL_COLOR);
         else
            glColor(getTeamColor(getTeam()), alpha);


         F32 ang = angleOfLongestSide(mVerts);

         if(mDockItem)    // Old school rendering on the dock
         {
            glPushMatrix();
               setLevelToCanvasCoordConversion();

               // Render the fill triangles
               renderTriangulatedPolygonFill(*getPolyFillPoints());

               glColor(hideit ? grayedOutColorBright : drawColor, alpha);
               glLineWidth(gLineWidth3);  
               renderPolygonOutline(mVerts);
               glLineWidth(gDefaultLineWidth);        // Restore line width
            glPopMatrix();

            // Let's add a label
            glColor(hideit ? grayedOutColorBright : drawColor, alpha);
            renderPolygonLabel(convertLevelToCanvasCoord(getCentroid(), !mDockItem), ang, EditorUserInterface::DOCK_LABEL_SIZE, getOnScreenName());
         }
         else     // Not a dock item
         {
            glPushMatrix();  
               setLevelToCanvasCoordConversion();

               if(getObjectTypeMask() & ItemLoadoutZone)
                  renderLoadoutZone(getTeamColor(getTeam()), mVerts, *getPolyFillPoints(), 
                                    getCentroid() * getGridSize(), ang, 1 / getGridSize());

               else if(getObjectTypeMask() & ItemGoalZone)
                  renderGoalZone(getTeamColor(getTeam()), mVerts, *getPolyFillPoints(),  
                                    getCentroid() * getGridSize(), ang, false, 0, getScore(), 1 / getGridSize());

               else if(getObjectTypeMask() & ItemNexus)
                  renderNexus(getVerts(), *getPolyFillPoints(), 
                                    getCentroid() * getGridSize(), ang, true, 0, 1 / getGridSize());

               else if(getObjectTypeMask() & ItemNavMeshZone)
                  renderNavMeshZone(getVerts(), *getPolyFillPoints(), getCentroid(), showMode == NavZoneMode ? -2 : -1, 
                                    true, mSelected);

               else if(getObjectTypeMask() & ItemSlipZone)
                  renderSlipZone(mVerts, *getPolyFillPoints(), getExtent());


               //else if(item.getObjectTypeMask() & ItemBarrierMaker)
               //   renderPolygon(item.fillPoints, item->getVerts(), gIniSettings.wallFillColor, gIniSettings.wallOutlineColor, 1);

               // If item is selected, and we're not in preview mode, draw a border highlight
               if(!showingReferenceShip && (mSelected || mLitUp || (gEditorUserInterface.mDraggingObjects && mAnyVertsSelected)))
               {        
                  glColor(hideit ? grayedOutColorBright : drawColor, alpha);
                  glLineWidth(gLineWidth3);  
                  renderPolygonOutline(mVerts);
                  glLineWidth(gDefaultLineWidth);        // Restore line width
               }

            glPopMatrix();
         }
      }

      // NavMeshZone verts will be drawn elsewhere
      if((getGeomType() == geomLine || showMode != ShowWallsOnly) && 
                  !mDockItem && !showingReferenceShip)  
         renderLinePolyVertices(gEditorUserInterface.getCurrentScale(), alpha);                               
   }
 
   if(showMode != ShowWallsOnly ||  mDockItem || showingReferenceShip)   // Draw the various point items
   {
      Color c = hideit ? grayedOutColorDim : getTeamColor(getTeam());           // And a color (based on team affiliation)

      if(getObjectTypeMask() & ItemFlag)             // Draw flag
      {
         glPushMatrix();
            glTranslatef(pos.x, pos.y, 0);
            glScalef(0.6, 0.6, 1);
            renderFlag(0, 0, c, hideit ? &grayedOutColorDim : NULL, alpha);
         glPopMatrix();
      }
      else if(getObjectTypeMask() & ItemFlagSpawn)    // Draw flag spawn point
      {
         if(showingReferenceShip && !mDockItem)
         {
            // Do nothing -- hidden in preview mode
         }
         else
         {
            glPushMatrix();
               glTranslatef(pos.x + 1, pos.y, 0);
               glScalef(0.4, 0.4, 1);
               renderFlag(0, 0, c, hideit ? &grayedOutColorDim : NULL, alpha);

               glColor(hideit ? grayedOutColorDim : white, alpha);
               drawCircle(-4, 0, 26);
            glPopMatrix();
         }
      }
      else if(getObjectTypeMask() & ItemAsteroidSpawn)    // Draw asteroid spawn point
      {
         if(showingReferenceShip && !mDockItem)
         {
            // Do nothing -- hidden in preview mode
         }
         else
         {
            glPushMatrix();
               glTranslatef(pos.x, pos.y, 0);
               glScalef(0.8, 0.8, 1);
               renderAsteroid(Point(0,0), asteroidDesign, .1, hideit ? &grayedOutColorDim : NULL, alpha);

               glColor(hideit ? grayedOutColorDim : white, alpha);
               drawCircle(0, 0, 13);
            glPopMatrix();
         }
      }
      else if(getObjectTypeMask() & ItemBouncyBall)   // Draw testitem
      {
         if(!mDockItem)
         {
            glPushMatrix();
               gEditorUserInterface.setTranslationAndScale(pos);
               renderTestItem(pos, alpha);
            glPopMatrix();
         }
         else     // Dock item rendering
         {
            glColor(hideit ? grayedOutColorBright : Color(1,1,0), alpha);
            drawPolygon(pos, 7, 8, 0);
         }
      }
      else if(getObjectTypeMask() & ItemAsteroid)   // Draw asteroid
      {
         if(!mDockItem)
         {
            glPushMatrix();
               gEditorUserInterface.setTranslationAndScale(pos);
               renderAsteroid(pos, asteroidDesign, asteroidRenderSize[0], hideit ? &grayedOutColorDim : NULL, alpha);
            glPopMatrix();
         }
         else     // Dock item rendering
            renderAsteroid(pos, asteroidDesign, .1, hideit ? &grayedOutColorDim : NULL, alpha);
      }

      else if(getObjectTypeMask() & ItemResource)   // Draw resourceItem
      {
         if(!mDockItem)
         {
            glPushMatrix();
               gEditorUserInterface.setTranslationAndScale(pos);
               renderResourceItem(pos, alpha);
            glPopMatrix();
         }
         else     // Dock item rendering
             renderResourceItem(pos, .4, hideit ? &grayedOutColorDim : NULL, alpha);
      }
      else if(getObjectTypeMask() & ItemSoccerBall)  // Soccer ball, obviously
      {
         if(!mDockItem)
         {
            glPushMatrix();
               gEditorUserInterface.setTranslationAndScale(pos);
               renderSoccerBall(pos, alpha);
            glPopMatrix();
         }
         else
         {
            glColor(hideit ? grayedOutColorBright : Color(.7,.7,.7), alpha);
            drawCircle(pos, 9);
         }
      }
      else if(getObjectTypeMask() & ItemMine)  // And a mine
      {
         if(showingReferenceShip && !mDockItem) 
         {
             glPushMatrix();
               gEditorUserInterface.setTranslationAndScale(pos);
               renderMine(pos, true, true);
            glPopMatrix();
         }
         else
         {
            glColor(hideit ? grayedOutColorDim : Color(.7,.7,.7), alpha);
            drawCircle(pos, 9 - (mDockItem ? 2 : 0));

            glColor(hideit ? grayedOutColorDim : Color(.1,.3,.3), alpha);
            drawCircle(pos, 5 - (mDockItem ? 1 : 0));

            drawLetter('M', pos, hideit ? grayedOutColorBright : drawColor, alpha);
         }
      }
      else if(getObjectTypeMask() & ItemSpyBug)  // And a spy bug
      {
         glColor(hideit ? grayedOutColorDim : Color(.7,.7,.7), alpha);
         drawCircle(pos, 9 - (mDockItem ? 2 : 0));

         glColor(hideit ? grayedOutColorDim : getTeamColor(getTeam()), alpha);
         drawCircle(pos, 5 - (mDockItem ? 1 : 0));

         drawLetter('S', pos, hideit ? grayedOutColorBright : drawColor, alpha);

         // And show how far it can see... unless, of course, it's on the dock, and assuming the tab key has been pressed
         if(!mDockItem && showingReferenceShip && (mSelected || mLitUp))
         {
            glColor(getTeamColor(getTeam()), .25 * alpha);

            F32 size = getCurrentScale() / getGridSize() * F32(gSpyBugRange);

            drawFilledSquare(pos, size);
         }
      }

      else if(getObjectTypeMask() & ItemRepair)
         renderRepairItem(pos, true, hideit ? &grayedOutColorDim : NULL, alpha);

      else if(getObjectTypeMask() & ItemEnergy)
         renderEnergyItem(pos, true, hideit ? &grayedOutColorDim : NULL, alpha);

      else if(getObjectTypeMask() & ItemTurret || getObjectTypeMask() & ItemForceField)
      { 
         if(renderFull(getObjectTypeMask(), getCurrentScale(), mDockItem, mSnapped))      
         {
            if(getObjectTypeMask() & ItemTurret)
            {
               glPushMatrix();
                  gEditorUserInterface.setTranslationAndScale(pos);
                  renderTurret(c, pos, mAnchorNormal, true, 1.0, mAnchorNormal.ATAN2());
               glPopMatrix();
            }
            else   
            {
               glPushMatrix();
                  gEditorUserInterface.setTranslationAndScale(pos);
                  renderForceFieldProjector(pos, mAnchorNormal, c, true);
               glPopMatrix();

               F32 scaleFact = 1 / getGridSize(); 

               glPushMatrix();
                  setLevelToCanvasCoordConversion();

                  renderForceField(ForceFieldProjector::getForceFieldStartPoint(getVert(0), mAnchorNormal, scaleFact), 
                                   forceFieldEnd, c, true, scaleFact);
               glPopMatrix();
            }
         }
         else
            renderGenericItem(pos, c, alpha, hideit ? grayedOutColorBright : drawColor, getObjectTypeMask() & ItemTurret ? 'T' : '>');  
      }
      else if(getObjectTypeMask() & ItemSpawn)
         renderGenericItem(pos, c, alpha, hideit ? grayedOutColorBright : drawColor, 'S');  




      // If this is an item that has a repop attribute, and the item is selected, draw the text
      if(!mDockItem && getHasRepop())
      {
         if(showMode != ShowWallsOnly && 
            ((mSelected || mLitUp) && !gEditorUserInterface.isEditingSpecialAttrItem()) &&
            (getObjectTypeMask() & ~ItemFlagSpawn || !strcmp(gEditorUserInterface.mGameType, "HuntersGameType")) || isBeingEdited())
         {
            glColor(white);

            const char *healword = (getObjectTypeMask() & ItemTurret || getObjectTypeMask() & ItemForceField) ? "10% Heal" : 
                                   ((getObjectTypeMask() & ItemFlagSpawn || getObjectTypeMask() & ItemAsteroidSpawn) ? "Spawn Time" : "Regen");

            Point offset = getEditorSelectionOffset(getCurrentScale()).rotate(mAnchorNormal.ATAN2()) * getCurrentScale();

            S32 radius = getRadius(getCurrentScale());
            offset.y += ((radius == NONE || mDockItem) ? 10 : (radius * getCurrentScale() / getGridSize())) - 6;

            if(repopDelay == 0)
               UserInterface::drawStringfc(pos.x + offset.x, pos.y + offset.y + attrSize, attrSize, "%s: Disabled", healword);
            else
               UserInterface::drawStringfc(pos.x + offset.x, pos.y + offset.y + 10, attrSize, "%s: %d sec%c", 
                                           healword, repopDelay, repopDelay != 1 ? 's' : 0);


            const char *msg;

            if(gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::NoAttribute))
               msg = "[Enter] to edit";
            else if(isBeingEdited() && gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::RepopDelay))
               msg = "Up/Dn to change";
            else
               msg = "???";
            UserInterface::drawStringc(pos.x + offset.x, pos.y + offset.y + instrSize + 13, instrSize, msg);
         }
      }

      // If we have a turret, render it's range (if tab is depressed)
      if(getObjectTypeMask() & ItemTurret)
      {
         if(!mDockItem && showingReferenceShip && (mSelected || mLitUp))
         {
            glColor(getTeamColor(getTeam()), .25 * alpha);

            F32 size = getCurrentScale() / getGridSize() * (gWeapons[WeaponTurret].projLiveTime * gWeapons[WeaponTurret].projVelocity / 1000);
            drawFilledSquare(pos, size);
         }
      }
   }

   glDisableBlend;
}

      // Draw highlighted border around item if selected
      //if(showMode != ShowWallsOnly && (mSelected || mLitUp))  
      //{
      //   // Dock items are never selected, but they can be highlighted
      //   Point pos = mDockItem ? getVert(0) : convertLevelToCanvasCoord(getVert(0));   

      //   glColor(drawColor);

      //   S32 radius = getRadius(getCurrentScale());
      //   S32 highlightRadius = (radius == NONE || mDockItem) ? 10 : S32(radius * getCurrentScale() / getGridSize() + 0.5f);

      //   Point ctr = pos + getEditorSelectionOffset(getCurrentScale()).rotate(mAnchorNormal.ATAN2()) * getCurrentScale();   

      //   drawSquare(ctr, highlightRadius);
      //}

   //   // Add a label if we're hovering over it (or not, unless it's on the dock, where we've already labeled our items)
   //   // For the moment, we need special handling for turrets & forcefields :-(
   //   if(showMode != ShowWallsOnly && (mSelected || mLitUp) && 
   //         getOnScreenName() && !mDockItem &&
   //         !((getObjectTypeMask() & ItemTurret || getObjectTypeMask() & ItemForceField) && renderFull(getObjectTypeMask(), getCurrentScale(), mDockItem, mSnapped)))
   //   {
   //      glColor(drawColor);
   //      //UserInterface::drawStringc(pos.x, pos.y - EditorUserInterface::DOCK_LABEL_SIZE * 2 - 5, EditorUserInterface::DOCK_LABEL_SIZE, getOnScreenName()); // Label on top
   //   }
   // }

   //// Label our dock items
   //if(mDockItem && getGeomType() != geomPoly)      // Polys are already labeled internally
   //{
   //   glColor(hideit ? grayedOutColorBright : drawColor);
   //   F32 maxy = -F32_MAX;
   //   for(S32 j = 0; j < getVertCount(); j++)
   //      if(getVert(j).y > maxy)
   //         maxy = getVert(j).y;

   //   // Make some label position adjustments
   //   if(getGeomType() == geomSimpleLine)
   //      maxy -= 2;
   //   else if(getObjectTypeMask() & ItemSoccerBall)
   //      maxy += 1;

   //   F32 xpos = pos.x - UserInterface::getStringWidth(EditorUserInterface::DOCK_LABEL_SIZE, getOnDockName())/2;
   //   UserInterface::drawString(xpos, maxy + 8, EditorUserInterface::DOCK_LABEL_SIZE, getOnDockName());
   //}



void EditorUserInterface::clearSelection()
{
   for(S32 i = 0; i < mItems.size(); i++)
      unselectItem(i);
}


void EditorUserInterface::unselectItem(S32 i)
{
   mItems[i]->setSelected(false);
   mItems[i]->setLitUp(false);

   mItems[i]->unselectVerts();
}


static S32 getNextItemId()
{
   static S32 nextItemId = 0;
   return nextItemId++;
}


// Paste items on the clipboard
void EditorUserInterface::pasteSelection()
{
   if(mDraggingObjects)     // Pasting while dragging can cause crashes!!
      return;

   S32 itemCount = mClipboard.size();

    if(!itemCount)       // Nothing on clipboard, nothing to do
      return;

   saveUndoState();      // So we can undo the paste

   clearSelection();     // Only the pasted items should be selected

   Point pos = snapPoint(convertCanvasToLevelCoord(mMousePos));

   // Diff between mouse pos and original object (item will be pasted such that the first vertex is at mouse pos)
   Point offset = pos - mClipboard[0]->getVert(0);    

   for(S32 i = 0; i < itemCount; i++)
   {
      mItems.push_back(mClipboard[i]);
      mItems.last()->setSerialNumber(getNextItemId());
      mItems.last()->setSelected(true);
      for(S32 j = 0; j < mItems.last()->getVertCount(); j++)
         mItems.last()->setVert(mItems.last()->getVert(j) += offset, j);
      mItems.last()->onGeomChanged(getCurrentScale());
   }
   mItems.sort(geometricSort);
   validateLevel();
   mNeedToSave = true;
   autoSave();
}


// Copy selection to the clipboard
void EditorUserInterface::copySelection()
{
   if(!anyItemsSelected())
      return;

   bool alreadyCleared = false;

   S32 itemCount = mItems.size();
   for(S32 i = 0; i < itemCount; i++)
   {
      if(mItems[i]->isSelected())
      {
         EditorObject *newItem =  mItems[i]->newCopy();      
         newItem->setSelected(false);
         for(S32 j = 0; j < newItem->getVertCount(); j++)
            newItem->getVert(j) += Point(0.5, 0.5);

         if(!alreadyCleared)  // Make sure we only purge the existing clipboard if we'll be putting someting new there
         {
            mClipboard.deleteAndClear();
            alreadyCleared = true;
         }

         mClipboard.push_back(newItem);
      }
   }
}


// Expand or contract selection by scale
void EditorUserInterface::scaleSelection(F32 scale)
{
   if(!anyItemsSelected() || scale < .01 || scale == 1)    // Apply some sanity checks
      return;

   saveUndoState();

   // Find center of selection
   Point min, max;                        
   computeSelectionMinMax(min, max);
   Point ctr = (min + max) * 0.5;

   if(scale > 1 && min.distanceTo(max) * scale > 50)    // If walls get too big, they'll bog down the db
      return;

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->isSelected())
         mItems[i]->scale(ctr, scale);

   mNeedToSave = true;
   autoSave();
}


// Rotate selected objects around their center point by angle
void EditorUserInterface::rotateSelection(F32 angle)
{
   if(!anyItemsSelected())
      return;

   saveUndoState();

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i]->isSelected())
         mItems[i]->rotateAboutPoint(Point(0,0), angle);
   }
   mNeedToSave = true;
   autoSave();
}


// Find all objects in bounds 
// TODO: This should be a database function!
void EditorUserInterface::computeSelectionMinMax(Point &min, Point &max)
{
   min.set(F32_MAX, F32_MAX);
   max.set(F32_MIN, F32_MIN);

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i]->isSelected())
      {
         EditorObject *item = mItems[i];
         for(S32 j = 0; j < item->getVertCount(); j++)
         {
            Point v = item->getVert(j);

            if(v.x < min.x)   min.x = v.x;
            if(v.x > max.x)   max.x = v.x;
            if(v.y < min.y)   min.y = v.y;
            if(v.y > max.y)   max.y = v.y;
         }
      }
   }
}


// Set the team affiliation of any selected items
void EditorUserInterface::setCurrentTeam(S32 currentTeam)
{
   mCurrentTeam = currentTeam;
   bool anyChanged = false;

   saveUndoState();

   if(currentTeam >= mTeams.size())
   {
      char msg[255];
      if(mTeams.size() == 1)
         dSprintf(msg, sizeof(msg), "Only 1 team has been configured.");
      else
         dSprintf(msg, sizeof(msg), "Only %d teams have been configured.", mTeams.size());
      gEditorUserInterface.setWarnMessage(msg, "Hit [F2] to configure teams.");
      return;
   }

   // Update all dock items to reflect new current team
   for(S32 i = 0; i < mDockItems.size(); i++)
   {
      if(!mDockItems[i]->hasTeam())
         continue;

      if(currentTeam == TEAM_NEUTRAL && !mItems[i]->canBeNeutral())
         continue;

      if(currentTeam == TEAM_HOSTILE && !mItems[i]->canBeHostile())
         continue;

      mDockItems[i]->setTeam(currentTeam);
   }

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i]->isSelected())
      {
         if(!mItems[i]->hasTeam())
            continue;

         if(currentTeam == TEAM_NEUTRAL && !mItems[i]->canBeNeutral())
            continue;

         if(currentTeam == TEAM_HOSTILE && !mItems[i]->canBeHostile())
            continue;

         if(!anyChanged)
            saveUndoState();

         mItems[i]->setTeam(currentTeam);
         anyChanged = true;
      }
   }

   // Overwrite any warnings set above.  If we have a group of items selected, it makes no sense to show a
   // warning if one of those items has the team set improperly.  The warnings are more appropriate if only
   // one item is selected, or none of the items are given a valid team setting.

   if(anyChanged)
   {
      gEditorUserInterface.setWarnMessage("", "");
      validateLevel();
      mNeedToSave = true;
      autoSave();
   }
}


void EditorUserInterface::flipSelectionHorizontal()
{
   if(!anyItemsSelected())
      return;

   saveUndoState();

   Point min, max;
   computeSelectionMinMax(min, max);

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->isSelected())
         mItems[i]->flipHorizontal(min, max);

   mNeedToSave = true;
   autoSave();
}


void EditorUserInterface::flipSelectionVertical()
{
   if(!anyItemsSelected())
      return;

   saveUndoState();

   Point min, max;
   computeSelectionMinMax(min, max);

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->isSelected())
         mItems[i]->flipVertical(min, max);

   mNeedToSave = true;
   autoSave();
}


void EditorUserInterface::findHitVertex(const Point &canvasPos, EditorObject *&hitObject, S32 &hitVertex)
{
   hitObject = NULL;
   hitVertex = NONE;

   if(mEditingSpecialAttrItem)    // If we're editing a text special attribute, disable this functionality
      return;

   const S32 VERTEX_HIT_RADIUS = 8;

   for(S32 x = 1; x >= 0; x--)    // Two passes... first for selected item, second for all items
   {
      for(S32 i = mItems.size() - 1; i >= 0; i--)     // Reverse order so we get items "from the top down"
      { 
         if(x && !mItems[i]->isSelected() && !mItems[i]->anyVertsSelected())
            continue;

         U32 type = mItems[i]->getObjectTypeMask();
         if(mShowMode == ShowWallsOnly && !(type & ItemBarrierMaker) && !(type & ItemPolyWall) ||
            mShowMode == NavZoneMode && !(type & ItemNavMeshZone) )        // Only select walls in CTRL-A mode
            continue;

         if(mItems[i]->getGeomType() <= geomPoint)
            continue;

         for(S32 j = mItems[i]->getVertCount() - 1; j >= 0; j--)
         {
            Point v = convertLevelToCanvasCoord(mItems[i]->getVert(j));
            if(fabs(v.x - canvasPos.x) < VERTEX_HIT_RADIUS && fabs(v.y - canvasPos.y) < VERTEX_HIT_RADIUS)
            {
               hitObject = mItems[i];
               hitVertex = j;
               return;
            }
         }
      }
   }
}


static const S32 POINT_HIT_RADIUS = 9;
static const S32 EDGE_HIT_RADIUS = 6;

void EditorUserInterface::findHitItemAndEdge()
{
   mItemHit = NULL;
   mEdgeHit = NONE;

   if(mEditingSpecialAttrItem)                        // If we're editing special attributes, disable this functionality
      return;

   // Do this in two passes -- the first we only consider selected items, the second pass will consider all targets.
   // This will give priority to moving vertices of selected items
   for(S32 x = 1; x >= 0; x--)      // x will be true the first time through, false the second time
   {
      for(S32 i = mItems.size() - 1; i >= 0; i--)     // Go in reverse order to prioritize items drawn on top
      {
         if(x && !mItems[i]->isSelected() && !mItems[i]->anyVertsSelected())     // First pass is for selected items only
            continue;
         
          // Only select walls in CTRL-A mode...
         if(mShowMode == ShowWallsOnly && mItems[i]->getObjectTypeMask() & ~ItemBarrierMaker && !(mItems[i]->getObjectTypeMask() & ItemPolyWall)) 
            continue;                                                              // ...so if it's not a wall, proceed to next item

         if(mShowMode == NavZoneMode && mItems[i]->getObjectTypeMask() & ~ItemNavMeshZone)   // Only select zones in CTRL-A mode...
            continue;                                                                        // ...so if it's not a bot nav zone, proceed to next item

         if(mItems[i]->getGeomType() == geomPoint)
         {
            S32 radius = mItems[i]->getRadius(mCurrentScale);
            S32 targetRadius = (radius == NONE) ? POINT_HIT_RADIUS : S32(radius * mCurrentScale / getGridSize() + 0.5f);

            F32 ang = mItems[i]->mAnchorNormal.ATAN2();
            Point pos = convertLevelToCanvasCoord(mItems[i]->getVert(0) + mItems[i]->getEditorSelectionOffset(mCurrentScale).rotate(ang));

            if(fabs(mMousePos.x - pos.x) < targetRadius && fabs(mMousePos.y - pos.y) < targetRadius)
            {
               mItemHit = mItems[i];
               return;
            }
         }

         // Make a copy of the items vertices that we can add to in the case of a loop
         Vector<Point> verts = mItems[i]->getVerts();    

         if(mItems[i]->getGeomType() == geomPoly)   // Add first point to the end to create last side on poly
            verts.push_back(verts.first());

         Point p1 = convertLevelToCanvasCoord(mItems[i]->getVert(0));
         Point closest;
         
         for(S32 j = 0; j < mItems[i]->getVertCount() - 1; j++)
         {
            Point p2 = convertLevelToCanvasCoord(verts[j+1]);
            
            if(findNormalPoint(mMousePos, p1, p2, closest))
            {
               F32 distance = (mMousePos - closest).len();
               if(distance < EDGE_HIT_RADIUS)
               {
                  mItemHit = mItems[i];
                  mEdgeHit = j;
                  return;
               }
            }
            p1.set(p2);
         }
      }
   }

   if(mShowMode == ShowWallsOnly) 
      return;

   // If we're still here, it means we didn't find anything yet.  Make one more pass, and see if we're in any polys.
   // This time we'll loop forward, though I don't think it really matters.
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mShowMode == ShowAllButNavZones && mItems[i]->getObjectTypeMask() & ItemNavMeshZone)     // Don't select NavMeshZones while they're hidden
         continue;
      if(mShowMode == NavZoneMode && mItems[i]->getObjectTypeMask() & ~ItemNavMeshZone)
         continue;

      if(mItems[i]->getGeomType() == geomPoly)
      {
         Vector<Point> verts;
         for(S32 j = 0; j < mItems[i]->getVertCount(); j++)
            verts.push_back(convertLevelToCanvasCoord(mItems[i]->getVert(j)));

         if(PolygonContains2(verts.address(), verts.size(), mMousePos))
         {
            mItemHit = mItems[i];
            return;
         }
      }
   }
}


S32 EditorUserInterface::findHitItemOnDock(Point canvasPos)
{
   if(mShowMode == ShowWallsOnly)     // Only add dock items when objects are visible
      return NONE;

   if(mEditingSpecialAttrItem)        // If we're editing a text item, disable this functionality
      return NONE;

   for(S32 i = mDockItems.size() - 1; i >= 0; i--)     // Go in reverse order because the code we copied did ;-)
   {
      Point pos = mDockItems[i]->getVert(0);

      if(fabs(canvasPos.x - pos.x) < POINT_HIT_RADIUS && fabs(canvasPos.y - pos.y) < POINT_HIT_RADIUS)
         return i;
   }

   // Now check for polygon interior hits
   for(S32 i = 0; i < mDockItems.size(); i++)
      if(mDockItems[i]->getGeomType() == geomPoly)
      {
         Vector<Point> verts;
         for(S32 j = 0; j < mDockItems[i]->getVertCount(); j++)
            verts.push_back(mDockItems[i]->getVert(j));

         if(PolygonContains2(verts.address(),verts.size(), canvasPos))
            return i;
      }

   return NONE;
}


void EditorUserInterface::onMouseMoved(S32 x, S32 y)
{
   onMouseMoved();
}


void EditorUserInterface::onMouseMoved()
{
   if(mouseIgnore)  // Needed to avoid freezing effect from too many mouseMoved events without a render in between
      return;

   mouseIgnore = true;

   mMousePos.set(gScreenInfo.getMousePos());

   if(mCreatingPoly || mCreatingPolyline)
      return;

   S32 vertexHit;
   EditorObject *vertexHitObject;

   findHitVertex(mMousePos, vertexHitObject, vertexHit);      // Sets vertexHitObject and vertexHit
   findHitItemAndEdge();                                      //  Sets mItemHit and mEdgeHit

   // Unhighlight the currently lit up object, if any
   if(mItemToLightUp)
      mItemToLightUp->setLitUp(false);

   S32 vertexToLightUp = NONE;
   mItemToLightUp = NULL;

   // We hit a vertex that wasn't already selected
   if(vertexHit != NONE && !vertexHitObject->vertSelected(vertexHit))   
   {
      mItemToLightUp = vertexHitObject;
      mItemToLightUp->setVertexLitUp(vertexHit);
   }

   // We hit an item that wasn't already selected
   else if(mItemHit && !mItemHit->isSelected())                   
      mItemToLightUp = mItemHit;

   // Check again, and take a point object in preference to a vertex
   if(mItemHit && !mItemHit->isSelected() && mItemHit->getGeomType() == geomPoint)  
   {
      mItemToLightUp = mItemHit;
      vertexToLightUp = NONE;
   }

   if(mItemToLightUp)
      mItemToLightUp->setLitUp(true);

   bool showMoveCursor = (vertexHitObject || vertexHit != NONE || mItemHit || mEdgeHit != NONE || 
                         (mouseOnDock() && findHitItemOnDock(mMousePos) != NONE));


   findSnapVertex();

   glutSetCursor((showMoveCursor && !mShowingReferenceShip) ? GLUT_CURSOR_SPRAY : GLUT_CURSOR_RIGHT_ARROW);
}


void EditorUserInterface::onMouseDragged(S32 x, S32 y)
{
   if(mouseIgnore)  // Needed to avoid freezing effect from too many mouseMoved events without a render in between (sam)
      return;

   mouseIgnore = true;

   mMousePos.set(gScreenInfo.getMousePos());

   if(mCreatingPoly || mCreatingPolyline || mDragSelecting || mEditingSpecialAttrItem)
      return;

   if(mDraggingDockItem != NONE)      // We just started dragging an item off the dock
   {
      // Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement
      // seem more natural.
      Point offset;

      if(mDockItems[mDraggingDockItem]->getGeomType() == geomPoly)
         offset.set(.25, .15);
      else if(mDockItems[mDraggingDockItem]->getObjectTypeMask() & ItemSpeedZone)
         offset.set(.15, 0);
      else if(mDockItems[mDraggingDockItem]->getObjectTypeMask() & ItemTextItem)     
         offset.set(.4, 0);

      // Instantiate object so we are in essence dragging a non-dock item
      Point pos = snapPoint(convertCanvasToLevelCoord(mMousePos) - offset, true);

      
      EditorObject *item = newEditorObject(mDockItems[mDraggingDockItem]->getClassName());

      // Put this in item itself?
      item->setVert(pos, 0);
      item->setVert(pos + Point(1,0), 1);
      item->setTeam(mDockItems[mDraggingDockItem]->getTeam());
      

      item->setWidth((mDockItems[mDraggingDockItem]->getGeomType() == geomPoly) ? .7 : 1);

      item->addToEditor(gEditorGame);

      // A little hack here to keep the polywall fill to appear to be left behind behind the dock
      if(item->getObjectTypeMask() & ItemPolyWall)
         item->clearPolyFillPoints();

      clearSelection();            // No items are selected...
      item->setSelected(true);     // ...except for the new one
      mItems.sort(geometricSort);  // So things will render in the proper order
      mDraggingDockItem = NONE;    // Because now we're dragging a real item
      validateLevel();             // Check level for errors


      // Because we sometimes have trouble finding an item when we drag it off the dock, after it's been sorted,
      // we'll manually set mItemHit based on the selected item, which will always be the one we just added.
      mEdgeHit = NONE;
      for(S32 i = 0; i < mItems.size(); i++)
         if(mItems[i]->isSelected())
         {
            mItemHit = mItems[i];
            break;
         }
   } // if(draggingDockItem)


   findSnapVertex();
   if(!mSnapVertex_i || mSnapVertex_j == NONE)
      return;

   
   if(!mDraggingObjects)      // Just started dragging
   {
      mMoveOrigin = mSnapVertex_i->getVert(mSnapVertex_j);
      saveUndoState();

      //// Select any turrets/ffs that are attached to any selected walls
      //for(S32 i = 0; i < mItems.size(); i++)
      //   if(mItems[i]->getObjectTypeMask() & ItemBarrierMaker && mItems[i]->selected)
      //   {
      //      for(S32 j = 0; j < mItems.size(); j++)
      //         if((mItems[j]->getObjectTypeMask() & ItemTurret || mItems[j]->getObjectTypeMask() & ItemForceField) && 
      //                     mItems[j]->forceFieldMountSegment && mItems[j]->forceFieldMountSegment->mOwner == mItems[i]->mId)
      //            mItems[j]->selected = true;
      //   }
   }

   mDraggingObjects = true;


   Point delta;

   // The thinking here is that for large items -- walls, polygons, etc., we may grab an item far from its snap vertex, and we
   // want to factor that offset into our calculations.  For point items (and vertices), we don't really care about any slop
   // in the selection, and we just want the damn thing where we put it.
   // (*origPoint - mMouseDownPos) represents distance from item's snap vertex where we "grabbed" it
   if(mSnapVertex_i->getGeomType() == geomPoint || (mItemHit && mItemHit->anyVertsSelected()))
      delta = snapPoint(convertCanvasToLevelCoord(mMousePos)) - mMoveOrigin;
   else
      delta = snapPoint(convertCanvasToLevelCoord(mMousePos) + mMoveOrigin - mMouseDownPos) - mMoveOrigin;


   // Update the locations of all items we're moving to show them being dragged.  Note that an item cannot be
   // selected if one of its vertices are.
   for(S32 i = 0; i < mItems.size(); i++)
   {
      // Skip selected item if it's a turret or forcefield -- snapping will have already been applied
      if(mItems[i] == mSnapVertex_i && (mItems[i]->getObjectTypeMask() & ItemForceField || mItems[i]->getObjectTypeMask() & ItemTurret) && mItems[i]->isSnapped())
         continue;

      // Update coordinates of dragged item
      for(S32 j = 0; j < mItems[i]->getVertCount(); j++)
         if(mItems[i]->isSelected() || mItems[i]->vertSelected(j))
         {
            mItems[i]->setVert(mUndoItems[(mLastUndoIndex - 1) % UNDO_STATES][i]->getVert(j) + delta, j);

            // If we are dragging a vertex, and not the entire item, we are changing the geom, so notify the item
            if(mItems[i]->vertSelected(j))
               mItems[i]->onGeomChanging(getCurrentScale());     
         }

      if(mItems[i]->isSelected())     
         mItems[i]->onItemDragging();      // Make sure this gets run after we've updated the item's location
   }
}


// Sets mSnapVertex_i and mSnapVertex_j based on the vertex closest to the cursor that is part of the selected set
// What we really want is the closest vertex in the closest feature
void EditorUserInterface::findSnapVertex()
{
   F32 closestDist = F32_MAX;

   if(mDraggingObjects)    // Don't change snap vertex once we're dragging
      return;

   mSnapVertex_i = NULL;
   mSnapVertex_j = NONE;

   Point mouseLevelCoord = convertCanvasToLevelCoord(mMousePos);

   // If we have a hit item, and it's selected, find the closest vertex in the item
   if(mItemHit && mItemHit->isSelected())   
   {
      // If we've hit an edge, restrict our search to the two verts that make up that edge
      if(mEdgeHit != NONE)
      {
         mSnapVertex_i = mItemHit;     // Regardless of vertex, this is our hit item
         S32 v1 = mEdgeHit;
         S32 v2 = mEdgeHit + 1;

         // Handle special case of looping item
         if(mEdgeHit == mItemHit->getVertCount() - 1)
            v2 = 0;

         // Find closer vertex: v1 or v2
         mSnapVertex_j = (mItemHit->getVert(v1).distSquared(mouseLevelCoord) < 
                          mItemHit->getVert(v2).distSquared(mouseLevelCoord)) ? v1 : v2;

         return;
      }

      // Didn't hit an edge... find the closest vertex anywhere in the item
      for(S32 j = 0; j < mItemHit->getVertCount(); j++)
      {
         F32 dist = mItemHit->getVert(j).distSquared(mouseLevelCoord);

         if(dist < closestDist)
         {
            closestDist = dist;
            mSnapVertex_i = mItemHit;
            mSnapVertex_j = j;
         }
      }
      return;
   } 

   // Otherwise, we don't have a selected hitItem -- look for a selected vertex
   for(S32 i = 0; i < mItems.size(); i++)
      for(S32 j = 0; j < mItems[i]->getVertCount(); j++)
      {
         // If we find a selected vertex, there will be only one, and this is our snap point
         if(mItems[i]->vertSelected(j))
         {
            mSnapVertex_i = mItems[i];
            mSnapVertex_j = j;
            return;     
         }
      }
}


void EditorUserInterface::deleteSelection(bool objectsOnly)
{
   if(mDraggingObjects)     // No deleting while we're dragging, please...
      return;

   if(!anythingSelected())  // Nothing to delete
      return;

   bool deleted = false;

   for(S32 i = mItems.size()-1; i >= 0; i--)  // Reverse to avoid having to have i-- in middle of loop
   {
      if(mItems[i]->isSelected())
      {  
         // Since indices change as items are deleted, this will keep incorrect items from being deleted
         if(mItems[i]->isLitUp())
            mItemToLightUp = NULL;

         if(!deleted)
            saveUndoState();

         deleteItem(i);
         deleted = true;
      }
      else if(!objectsOnly)      // Deleted any selected vertices
      {
         bool geomChanged = false;

         for(S32 j = 0; j < mItems[i]->getVertCount(); j++) 
         {
            if(mItems[i]->vertSelected(j))
            {
               
               if(!deleted)
                  saveUndoState();
              
               mItems[i]->deleteVert(j);
               deleted = true;

               geomChanged = true;
               mSnapVertex_i = NULL;
               mSnapVertex_j = NONE;
            }
         }

         // Deleted last vertex, or item can't lose a vertex... it must go!
         if(mItems[i]->getVertCount() == 0 || (mItems[i]->getGeomType() == geomSimpleLine && mItems[i]->getVertCount() < 2)
                                       || (mItems[i]->getGeomType() == geomLine && mItems[i]->getVertCount() < 2)
                                       || (mItems[i]->getGeomType() == geomPoly && mItems[i]->getVertCount() < 2))
         {
            deleteItem(i);
            deleted = true;
         }
         else if(geomChanged)
            mItems[i]->onGeomChanged(getCurrentScale());

      }  // else if(!objectsOnly) 
   }  // for

   if(deleted)
   {
      mNeedToSave = true;
      autoSave();

      mItemToLightUp = NULL;     // In case we just deleted a lit item; not sure if really needed, as we do this above
      //vertexToLightUp = NONE;
   }
}

// Increase selected wall thickness by amt
void EditorUserInterface::incBarrierWidth(S32 amt)
{
   if(!mLastUndoStateWasBarrierWidthChange)
      saveUndoState(); 

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->isSelected())
         mItems[i]->increaseWidth(amt);

   mLastUndoStateWasBarrierWidthChange = true;
}


// Decrease selected wall thickness by amt
void EditorUserInterface::decBarrierWidth(S32 amt)
{
   if(!mLastUndoStateWasBarrierWidthChange)
      saveUndoState(); 

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->isSelected())
         mItems[i]->decreaseWidth(amt);

   mLastUndoStateWasBarrierWidthChange = true;
}


// Split wall/barrier on currently selected vertex/vertices
void EditorUserInterface::splitBarrier()
{
   bool split = false;

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->getGeomType() == geomLine)
          for(S32 j = 1; j < mItems[i]->getVertCount() - 1; j++)     // Can't split on end vertices!
            if(mItems[i]->vertSelected(j))
            {
               if(!split)
                  saveUndoState();
               split = true;

               // Create a poor man's copy
               EditorObject *newItem = mItems[i]->newCopy();
               newItem->setTeam(-1);
               newItem->setWidth(mItems[i]->getWidth());
               newItem->clearVerts();

               for(S32 k = j; k < mItems[i]->getVertCount(); k++) 
               {
                  newItem->addVert(mItems[i]->getVert(k));
                  if (k > j)
                  {
                     mItems[i]->deleteVert(k);     // Don't delete j == k vertex -- it needs to remain as the final vertex of the old wall
                     k--;
                  }
               }

               mItems.push_back(newItem);

               // Tell the new segments that they have new geometry
               mItems[i]->onGeomChanged(getCurrentScale());
               mItems.last()->onGeomChanged(getCurrentScale());

               // And get them in the right order
               mItems.sort(geometricSort);         
               goto done2;                         // Yes, gotos are naughty, but they just work so well sometimes...
            }
done2:
   if(split)
   {
      clearSelection();
      mNeedToSave = true;
      autoSave();
   }
}


// Join two or more sections of wall that have coincident end points.  Will ignore invalid join attempts.
void EditorUserInterface::joinBarrier()
{
   S32 joinedItem = NONE;

   for(S32 i = 0; i < mItems.size()-1; i++)
      if(mItems[i]->getGeomType() == geomLine && (mItems[i]->isSelected()))
      {
         for(S32 j = i + 1; j < mItems.size(); j++)
         {
            if(mItems[j]->getObjectTypeMask() & mItems[i]->getObjectTypeMask() && (mItems[j]->isSelected()))
            {
               if(mItems[i]->getVert(0).distanceTo(mItems[j]->getVert(0)) < .01)    // First vertices are the same  1 2 3 | 1 4 5
               {
                  if(joinedItem == NONE)
                     saveUndoState();
                  joinedItem = i;

                  for(S32 a = 1; a < mItems[j]->getVertCount(); a++)             // Skip first vertex, because it would be a dupe
                     mItems[i]->addVertFront(mItems[j]->getVert(a));

                  deleteItem(j);
                  i--;  j--;
               }
               // First vertex conincides with final vertex 3 2 1 | 5 4 3
               else if(mItems[i]->getVert(0).distanceTo(mItems[j]->getVert(mItems[j]->getVertCount()-1)) < .01)     
               {
                  if(joinedItem == NONE)
                     saveUndoState();

                  joinedItem = i;
                  for(S32 a = mItems[j]->getVertCount()-2; a >= 0; a--)
                     mItems[i]->addVertFront(mItems[j]->getVert(a));

                  deleteItem(j);
                  i--;  j--;

               }
               // Last vertex conincides with first 1 2 3 | 3 4 5
               else if(mItems[i]->getVert(mItems[i]->getVertCount()-1).distanceTo(mItems[j]->getVert(0)) < .01)     
               {
                  if(joinedItem == NONE)
                     saveUndoState();

                  joinedItem = i;

                  for(S32 a = 1; a < mItems[j]->getVertCount(); a++)  // Skip first vertex, because it would be a dupe         
                     mItems[i]->addVert(mItems[j]->getVert(a));

                  deleteItem(j);
                  i--;  j--;
               }
               else if(mItems[i]->getVert(mItems[i]->getVertCount()-1).distanceTo(mItems[j]->getVert(mItems[j]->getVertCount()-1)) < .01)     // Last vertices coincide  1 2 3 | 5 4 3
               {
                  if(joinedItem == NONE)
                     saveUndoState();

                  joinedItem = i;

                  for(S32 a = mItems[j]->getVertCount()-2; a >= 0; a--)
                     mItems[i]->addVert(mItems[j]->getVert(a));

                  deleteItem(j);
                  i--;  j--;
               }
            }
         }
      }

   if(joinedItem != NONE)
   {
      clearSelection();
      mNeedToSave = true;
      autoSave();
      mItems[joinedItem]->onGeomChanged(getCurrentScale());
   }
}


void EditorUserInterface::deleteItem(S32 itemIndex)
{
   S32 mask = mItems[itemIndex]->getObjectTypeMask();
   if(mask & ItemBarrierMaker || mask & ItemPolyWall)
   {
      // Need to recompute boundaries of any intersecting walls
      wallSegmentManager.invalidateIntersectingSegments(mItems[itemIndex]);  // Mark intersecting segments invalid
      wallSegmentManager.deleteSegments(mItems[itemIndex]->getItemId());       // Delete the segments associated with the wall

      mItems.deleteAndErase(itemIndex);

      wallSegmentManager.recomputeAllWallGeometry();                          // Recompute wall edges
      recomputeAllEngineeredItems();         // Really only need to recompute items that were attached to deleted wall... but we
                                             // don't yet have a method to do that, and I'm feeling lazy at the moment
   }
   else if(mask & ItemNavMeshZone)
   {
      deleteBorderSegs(mItems[itemIndex]->getItemId());
      mItems.deleteAndErase(itemIndex);
   }
   else
      mItems.deleteAndErase(itemIndex);


   // Reset a bunch of things
   mSnapVertex_i = NULL;
   mSnapVertex_j = NONE;
   mItemToLightUp = NULL;

   validateLevel();

   onMouseMoved();   // Reset cursor  
}


void EditorUserInterface::insertNewItem(GameItems itemType)
{
   if(mShowMode == ShowWallsOnly || mDraggingObjects)     // No inserting when items are hidden or being dragged!
      return;

   clearSelection();
   saveUndoState();

   Point pos = snapPoint(convertCanvasToLevelCoord(mMousePos));
   S32 team = -1;

   // Get team affiliation from dockItem of same type
   for(S32 i = 0; i < mDockItems.size(); i++)
      if (mDockItems[i]->getObjectTypeMask() & itemType)
      {
         team = mDockItems[i]->getTeam();
         break;
      }

   EditorObject *newItem = new EditorObject(itemType);
   newItem->setVert(pos, 0);
   newItem->setTeam(team);

   newItem->addToEditor(gEditorGame);     // Adds newItem to mItems

   mItems.sort(geometricSort);
   validateLevel();
   mNeedToSave = true;
   autoSave();
}


static LineEditor getNewEntryBox(string value, string prompt, S32 length, LineEditor::LineEditorFilter filter)
{
   LineEditor entryBox(length);
   entryBox.setPrompt(prompt);
   entryBox.setString(value);
   entryBox.setFilter(filter);

   return entryBox;
}


void EditorUserInterface::centerView()
{
   if(mItems.size() || mLevelGenItems.size())
   {
      F32 minx =  F32_MAX,   miny =  F32_MAX;
      F32 maxx = -F32_MAX,   maxy = -F32_MAX;

      for(S32 i = 0; i < mItems.size(); i++)
         for(S32 j = 0; j < mItems[i]->getVertCount(); j++)
         {
            if(mItems[i]->getVert(j).x < minx)
               minx = mItems[i]->getVert(j).x;
            if(mItems[i]->getVert(j).x > maxx)
               maxx = mItems[i]->getVert(j).x;
            if(mItems[i]->getVert(j).y < miny)
               miny = mItems[i]->getVert(j).y;
            if(mItems[i]->getVert(j).y > maxy)
               maxy = mItems[i]->getVert(j).y;
         }

      for(S32 i = 0; i < mLevelGenItems.size(); i++)
         for(S32 j = 0; j < mLevelGenItems[i]->getVertCount(); j++)
         {
            if(mLevelGenItems[i]->getVert(j).x < minx)
               minx = mLevelGenItems[i]->getVert(j).x;
            if(mLevelGenItems[i]->getVert(j).x > maxx)
               maxx = mLevelGenItems[i]->getVert(j).x;
            if(mLevelGenItems[i]->getVert(j).y < miny)
               miny = mLevelGenItems[i]->getVert(j).y;
            if(mLevelGenItems[i]->getVert(j).y > maxy)
               maxy = mLevelGenItems[i]->getVert(j).y;
         }

      // If we have only one point object in our level, the following will correct
      // for any display weirdness.
      if(minx == maxx && miny == maxy)    // i.e. a single point item
      {
         mCurrentScale = MIN_SCALE;
         mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2  - mCurrentScale * minx, 
                            gScreenInfo.getGameCanvasHeight() / 2 - mCurrentScale * miny);
      }
      else
      {
         F32 midx = (minx + maxx) / 2;
         F32 midy = (miny + maxy) / 2;

         mCurrentScale = min(gScreenInfo.getGameCanvasWidth() / (maxx - minx), gScreenInfo.getGameCanvasHeight() / (maxy - miny));
         mCurrentScale /= 1.3;      // Zoom out a bit
         mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2  - mCurrentScale * midx, 
                            gScreenInfo.getGameCanvasHeight() / 2 - mCurrentScale * midy);
      }
   }
   else     // Put (0,0) at center of screen
   {
      mCurrentScale = 100;
      mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2, gScreenInfo.getGameCanvasHeight() / 2);
   }
}


// Save selection mask, which can be retrieved later, as long as mItems hasn't changed.
void EditorUserInterface::saveSelection()
{
   mSelectedSet = Selection(mItems);
}


// Restore selection mask
void EditorUserInterface::restoreSelection()
{
   mSelectedSet.restore(mItems);
}


U32 EditorUserInterface::getNextAttr(S32 item)       // Not sure why this fn can't return a SpecialAttribute...  hrm...
{
   // Advance to the next attribute. If we were at NoAttribute, start with the first.
   U32 curr = (mSpecialAttribute == NoAttribute) ? 0 : mSpecialAttribute + 1;

   // Find next attribute that applies to selected object
   for(U32 i = curr; i <= NoAttribute; i++)
   {
      if( ((i == Text) && mItems[item]->hasText()) ||
          ((i == RepopDelay) && mItems[item]->getHasRepop()) ||
          ((i == GoFastSpeed || i == GoFastSnap) && mItems[item]->getObjectTypeMask() & ItemSpeedZone) ||  
          (i == NoAttribute ) )
         return i;
   }
   return NoAttribute;      // Should never get here...
}


// Gets run when user exits special-item editing mode
void EditorUserInterface::doneEditingSpecialItem(bool saveChanges)
{
   // Find any other selected items of the same type of the item we just edited, and update their values too

   if(!saveChanges)
      undo(false);
   else
      for(S32 i = 0; i < mItems.size(); i++)
      {
         if(mEditingSpecialAttrItem == mItems[i])
            continue;

         else if(mItems[i]->isSelected() && mItems[i]->getObjectTypeMask() == mEditingSpecialAttrItem->getObjectTypeMask())
         {
            // We'll ignore text here, because that really makes less sense
            mItems[i]->repopDelay = mEditingSpecialAttrItem->repopDelay;
            mItems[i]->speed = mEditingSpecialAttrItem->speed;
            mItems[i]->boolattr = mEditingSpecialAttrItem->boolattr;
            mItems[i]->onAttrsChanged(getCurrentScale());
         }
      }


   mEditingSpecialAttrItem->setIsBeingEdited(false);
   mEditingSpecialAttrItem = NULL;
   mSpecialAttribute = NoAttribute;
}


extern string itos(S32);
extern string itos(U32);
extern string itos(U64);

extern string ftos(F32, S32);

// Handle key presses
void EditorUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(OGLCONSOLE_ProcessBitfighterKeyEvent(keyCode, ascii))      // Pass the key on to the console for processing
      return;

   if(entryMode != EntryNone)
      textEntryKeyHandler(keyCode, ascii);

   else if(keyCode == KEY_ENTER)       // Enter - Edit props
      itemPropertiesEnterKeyHandler();

   // This first section is the key handlers for when we're editing the special attributes of an item.  Regular
   // key actions are handled below.
   else if(mEditingSpecialAttrItem)
      specialAttributeKeyHandler(keyCode, ascii);

   // Regular key handling from here on down
   else if(getKeyState(KEY_SHIFT) && keyCode == KEY_0)  // Shift-0 -> Set team to hostile
      setCurrentTeam(-2);

   else if(ascii == '#' || ascii == '!')
   {
      S32 selected = NONE;

      // Find first selected item, and just work with that.  Unselect the rest.
      for(S32 i = 0; i < mItems.size(); i++)
      {
         if(mItems[i]->isSelected())
         {
            if(selected == NONE)
            {
               selected = i;
               continue;
            }
            else
               mItems[i]->setSelected(false);
         }
      }

      if(selected == NONE)      // Nothing selected, nothing to do!
         return;

      mEntryBox = getNewEntryBox(mItems[selected]->getItemId() <= 0 ? "" : itos(mItems[selected]->getItemId()), 
                                 "Item ID:", 10, LineEditor::digitsOnlyFilter);
      entryMode = EntryID;
   }

   else if(ascii >= '0' && ascii <= '9')           // Change team affiliation of selection with 0-9 keys
   {
      setCurrentTeam(ascii - '1');
      return;
   }

   // Ctrl-left click is same as right click for Mac users
   else if(keyCode == MOUSE_RIGHT || (keyCode == MOUSE_LEFT && getKeyState(KEY_CTRL)))
   {
      if(getKeyState(MOUSE_LEFT) && !getKeyState(KEY_CTRL))    // Prevent weirdness
         return;  

      mMousePos.set(gScreenInfo.getMousePos());

      if(mCreatingPoly || mCreatingPolyline)
      {
         if(mNewItem->getVertCount() < gMaxPolygonPoints)          // Limit number of points in a polygon/polyline
         {
            mNewItem->addVert(snapPoint(convertCanvasToLevelCoord(mMousePos)));
            mNewItem->onGeomChanging(getCurrentScale());
         }
         
         return;
      }

      saveUndoState();     // Save undo state before we clear the selection
      clearSelection();    // Unselect anything currently selected

      // Can only add new vertices by clicking on item's edge, not it's interior (for polygons, that is)
      if(mEdgeHit != NONE && mItemHit && (mItemHit->getGeomType() == geomLine || mItemHit->getGeomType() >= geomPoly))
      {
         if(mItemHit->getVertCount() >= gMaxPolygonPoints)     // Polygon full -- can't add more
            return;

         Point newVertex = snapPoint(convertCanvasToLevelCoord(mMousePos));      // adding vertex w/ right-mouse

         mAddingVertex = true;

         // Insert an extra vertex at the mouse clicked point, and then select it.
         mItemHit->insertVert(newVertex, mEdgeHit + 1);
         mItemHit->selectVert(mEdgeHit + 1);

         // Alert the item that its geometry is changing
         mItemHit->onGeomChanging(getCurrentScale());

         mMouseDownPos = newVertex;
         
      }
      else     // Start creating a new poly or new polyline (tilda key + right-click ==> start polyline)
      {
         if(mShowMode != NavZoneMode)     // Unless we're in navZoneMode, that is
         {
            const char *type;
            S32 width;

            if(getKeyState(KEY_TILDE))
            {
               mCreatingPolyline = true;
               type = "LineItem";
               width = 2;
            }
            else
            {
               mCreatingPoly = true;
               type = "BarrierMaker";
               width = Barrier::BarrierWidth;
            }

            mNewItem = newEditorObject(type);

            mNewItem->setVert(convertCanvasToLevelCoord(mMousePos), 0);
            mNewItem->setTeam(TEAM_NEUTRAL);
            mNewItem->setWidth(width);
         }
      }
   }
   else if(keyCode == MOUSE_LEFT)
   {
      if(getKeyState(MOUSE_RIGHT))              // Prevent weirdness
         return;

      mDraggingDockItem = NONE;
      mMousePos.set(gScreenInfo.getMousePos());

      if(mCreatingPoly || mCreatingPolyline)    // Save any polygon/polyline we might be creating
      {
         saveUndoState();                       // Save state prior to addition of new polygon

         if(mNewItem->getVertCount() <= 1)
            delete mNewItem;
         else
         {
            mNewItem->addToEditor(gEditorGame);

            mItems.push_back(mNewItem);
            mItems.last()->onGeomChanged(getCurrentScale());      // Walls need to be added to mItems BEFORE onGeomChanged() is run!
            mItems.sort(geometricSort);
         }

         mNewItem = NULL;

         mCreatingPoly = false;
         mCreatingPolyline = false;
      }

      mMouseDownPos = convertCanvasToLevelCoord(mMousePos);

      if(mouseOnDock())    // On the dock?  Did we hit something to start dragging off the dock?
      {
         clearSelection();
         mDraggingDockItem = findHitItemOnDock(mMousePos);
      }
      else                 // Mouse is not on dock
      {
         mDraggingDockItem = NONE;

         // rules for mouse down:
         // if the click has no shift- modifier, then
         //   if the click was on something that was selected
         //     do nothing
         //   else
         //     clear the selection
         //     add what was clicked to the selection
         //  else
         //    toggle the selection of what was clicked

         S32 vertexHit;
         EditorObject *vertexHitPoly;

         findHitVertex(mMousePos, vertexHitPoly, vertexHit);

         if(!getKeyState(KEY_SHIFT))      // Shift key is not down
         {
            // If we hit a vertex of an already selected item --> now we can move that vertex w/o losing our selection
            if(vertexHit != NONE && vertexHitPoly->isSelected())    
            {
               clearSelection();
               vertexHitPoly->selectVert(vertexHit);
            }
            if(mItemHit && mItemHit->isSelected())   // Hit an already selected item
            {
               // Do nothing
            }
            else if(mItemHit && mItemHit->getGeomType() == geomPoint)  // Hit a point item
            {
               clearSelection();
               mItemHit->setSelected(true);
            }
            else if(vertexHit != NONE && (!mItemHit || !mItemHit->isSelected()))      // Hit a vertex of an unselected item
            {        // (braces required)
               if(!vertexHitPoly->vertSelected(vertexHit))
               {
                  clearSelection();
                  vertexHitPoly->selectVert(vertexHit);
               }
            }
            else if(mItemHit)                                                        // Hit a non-point item, but not a vertex
            {
               clearSelection();
               mItemHit->setSelected(true);
            }
            else     // Clicked off in space.  Starting to draw a bounding rectangle?
            {
               mDragSelecting = true;
               clearSelection();
            }
         }
         else     // Shift key is down
         {
            if(vertexHit != NONE)
            {
               if(vertexHitPoly->vertSelected(vertexHit))
                  vertexHitPoly->unselectVert(vertexHit);
               else
                  vertexHitPoly->aselectVert(vertexHit);
            }
            else if(mItemHit)
               mItemHit->setSelected(!mItemHit->isSelected());    // Toggle selection of hit item
            else
               mDragSelecting = true;
         }
     }     // end mouse not on dock block, doc

     findSnapVertex();     // Update snap vertex in the event an item was selected

   }     // end if keyCode == MOUSE_LEFT

   // Neither mouse button, let's try some keys
   else if(keyCode == KEY_D)              // D - Pan right
      mRight = true;
   else if(keyCode == KEY_RIGHT)          // Right - Pan right
      mRight = true;
   else if(keyCode == KEY_H)              // H - Flip horizontal
      flipSelectionHorizontal();
   else if(keyCode == KEY_V && getKeyState(KEY_CTRL))    // Ctrl-V - Paste selection
      pasteSelection();
   else if(keyCode == KEY_V)              // V - Flip vertical
      flipSelectionVertical();
   else if(keyCode == KEY_SLASH)
      OGLCONSOLE_ShowConsole();

   else if(keyCode == KEY_L && getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT))
   {
      loadLevel();                        // Ctrl-Shift-L - Reload level
      gEditorUserInterface.setSaveMessage("Reloaded " + getLevelFileName(), true);
   }
   else if(keyCode == KEY_Z)
   {
      if(getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT))   // Ctrl-Shift-Z - Redo
         redo();
      else if(getKeyState(KEY_CTRL))    // Ctrl-Z - Undo
         undo(true);
      else                              // Z - Reset veiw
        centerView();
   }
   else if(keyCode == KEY_R)
      if(getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT))      // Ctrl-Shift-R - Rotate by arbitrary amount
      {
         if(!anyItemsSelected())
            return;

         mEntryBox = getNewEntryBox("", "Rotation angle:", 10, LineEditor::numericFilter);
         entryMode = EntryAngle;
      }
      else if(getKeyState(KEY_CTRL))        // Ctrl-R - Run levelgen script, or clear last results
      {
         if(mLevelGenItems.size() == 0)
            runLevelGenScript();
         else
            clearLevelGenItems();
      }
      else
         rotateSelection(getKeyState(KEY_SHIFT) ? 15 : -15); // Shift-R - Rotate CW, R - Rotate CCW
   else if((keyCode == KEY_I) && getKeyState(KEY_CTRL))  // Ctrl-I - Insert items generated with script into editor
   {
      copyScriptItemsToEditor();
   }

   else if((keyCode == KEY_UP) && !getKeyState(KEY_CTRL) || keyCode == KEY_W)  // W or Up - Pan up
      mUp = true;
   else if(keyCode == KEY_UP && getKeyState(KEY_CTRL))      // Ctrl-Up - Zoom in
      mIn = true;
   else if(keyCode == KEY_DOWN)
   { /* braces required */
      if(getKeyState(KEY_CTRL))           // Ctrl-Down - Zoom out
         mOut = true;
      else                                // Down - Pan down
         mDown = true;
   }
   else if(keyCode == KEY_S)
   {
      if(getKeyState(KEY_CTRL))           // Ctrl-S - Save
         gEditorUserInterface.saveLevel(true, true);
      else                                // S - Pan down
         mDown = true;
   }
   else if(keyCode == KEY_A && getKeyState(KEY_CTRL))            // Ctrl-A - toggle see all objects
   {
      mShowMode = (ShowMode) ((U32)mShowMode + 1);
      //if(mShowMode == ShowAllButNavZones && !mHasBotNavZones)    // Skip hiding NavZones if we don't have any
      //   mShowMode = (ShowMode) ((U32)mShowMode + 1);

      if(mShowMode == ShowModesCount)
         mShowMode = (ShowMode) 0;     // First mode

      if(mShowMode == ShowWallsOnly && !mDraggingObjects)
         glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);

      populateDock();   // Different modes have different items

      onMouseMoved();   // Reset mouse to spray if appropriate
   }
   else if(keyCode == KEY_LEFT || keyCode == KEY_A)   // Left or A - Pan left
      mLeft = true;
   else if(keyCode == KEY_EQUALS)         // Plus (+) - Increase barrier width
   {
      if(getKeyState(KEY_SHIFT))          // SHIFT --> by 1
         incBarrierWidth(1);
      else                                // unshifted --> by 5
         incBarrierWidth(5);
   }
   else if(keyCode == KEY_MINUS)          // Minus (-)  - Decrease barrier width
   {
      if(getKeyState(KEY_SHIFT))          // SHIFT --> by 1
         decBarrierWidth(1);
      else                                // unshifted --> by 5
         decBarrierWidth(5);
   }

   else if(keyCode == KEY_E)              // E - Zoom In
         mIn = true;
   else if(keyCode == KEY_BACKSLASH)      // \ - Split barrier on selected vertex
      splitBarrier();
   else if(keyCode == KEY_J)
      joinBarrier();
   else if(keyCode == KEY_X && getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT)) // Ctrl-Shift-X - Resize selection
   {
      if(!anyItemsSelected())
         return;

      mEntryBox = getNewEntryBox("", "Resize factor:", 10, LineEditor::numericFilter);
      entryMode = EntryScale;
   }
   else if(keyCode == KEY_X && getKeyState(KEY_CTRL))     // Ctrl-X - Cut selection
   {
      copySelection();
      deleteSelection(true);
   }
   else if(keyCode == KEY_C && getKeyState(KEY_CTRL))    // Ctrl-C - Copy selection to clipboard
      copySelection();
   else if(keyCode == KEY_C )             // C - Zoom out
      mOut = true;
   else if(keyCode == KEY_F3)             // F3 - Level Parameter Editor
   {
      UserInterface::playBoop();
      gGameParamUserInterface.activate();
   }
   else if(keyCode == KEY_F2)             // F2 - Team Editor Menu
   {
      gTeamDefUserInterface.activate();
      UserInterface::playBoop();
   }
   else if(keyCode == KEY_T)              // T - Teleporter
      insertNewItem(ItemTeleporter);
   else if(keyCode == KEY_P)              // P - Speed Zone
      insertNewItem(ItemSpeedZone);
   else if(keyCode == KEY_G)              // G - Spawn
      insertNewItem(ItemSpawn);
   else if(keyCode == KEY_B && getKeyState(KEY_CTRL)) // Ctrl-B - Spy Bug
      insertNewItem(ItemSpyBug);
   else if(keyCode == KEY_B)              // B - Repair
      insertNewItem(ItemRepair);
   else if(keyCode == KEY_Y)              // Y - Turret
      insertNewItem(ItemTurret);
   else if(keyCode == KEY_M)              // M - Mine
      insertNewItem(ItemMine);
   else if(keyCode == KEY_F)              // F - Force Field
      insertNewItem(ItemForceField);
   else if(keyCode == KEY_BACKSPACE || keyCode == KEY_DELETE)
         deleteSelection(false);
   else if(keyCode == keyHELP)            // Turn on help screen
   {
      gEditorInstructionsUserInterface.activate();
      UserInterface::playBoop();
   }
   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
      gChatInterface.activate();
//   else if(keyCode == keyDIAG)            // Turn on diagnostic overlay -- why not here??
//      gDiagnosticInterface.activate();
   else if(keyCode == KEY_ESCAPE)           // Activate the menu
   {
      UserInterface::playBoop();
      gEditorMenuUserInterface.activate();
   }
   else if(keyCode == KEY_SPACE)
      mSnapDisabled = true;
   else if(keyCode == KEY_TAB)
      mShowingReferenceShip = true;
}


// Handle keyboard activity when we're editing an item's attributes
void EditorUserInterface::textEntryKeyHandler(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_ENTER)
   {
      if(entryMode == EntryID)
      {
         for(S32 i = 0; i < mItems.size(); i++)
            if(mItems[i]->isSelected())             // Should only be one
            {
               U32 id = atoi(mEntryBox.c_str());
               if(mItems[i]->getItemId() != id)     // Did the id actually change?
               {
                  mItems[i]->setItemId(id);
                  mAllUndoneUndoLevel = -1;        // If so, it can't be undone
               }
               break;
            }
      }
      else if(entryMode == EntryAngle)
      {
         F32 angle = (F32) atof(mEntryBox.c_str());
         rotateSelection(-angle);      // Positive angle should rotate CW, negative makes that happen
      }
      else if(entryMode == EntryScale)
      {
         F32 scale = (F32) atof(mEntryBox.c_str());
         scaleSelection(scale);
      }

      entryMode = EntryNone;
   }
   else if(keyCode == KEY_ESCAPE)
   {
      entryMode = EntryNone;
   }
   else if(keyCode == KEY_BACKSPACE || keyCode == KEY_DELETE)
      mEntryBox.handleBackspace(keyCode);

   else
      mEntryBox.addChar(ascii);

   // else ignore keystroke
}


static const S32 MAX_REPOP_DELAY = 600;      // That's 10 whole minutes!

void EditorUserInterface::specialAttributeKeyHandler(KeyCode keyCode, char ascii)
{
   if( keyCode == KEY_J && getKeyState(KEY_CTRL) )
   { /* Do nothing */ }
   else if(keyCode == KEY_ESCAPE || keyCode == MOUSE_LEFT || keyCode == MOUSE_RIGHT)      // End editing, revert
   {
      doneEditingSpecialItem(false); 
      return;
   }
   else if(mSpecialAttribute == Text)     // TODO: Move this to textitem
   {
      TextItem *textItem = dynamic_cast<TextItem *>(mEditingSpecialAttrItem);
      TNLAssert(textItem, "bad cast");

      if(keyCode == KEY_BACKSPACE || keyCode == KEY_DELETE)
         textItem->lineEditor.handleBackspace(keyCode);

      else if(ascii)       // User typed a character -- add it to the string
         textItem->lineEditor.addChar(ascii);

      textItem->onAttrsChanging(getCurrentScale());

   }
   else if(mSpecialAttribute == RepopDelay)
   {
      if(keyCode == KEY_UP)         // Up - increase delay
      {  /* braces required */
         if(mEditingSpecialAttrItem->repopDelay < MAX_REPOP_DELAY)
            mEditingSpecialAttrItem->repopDelay++;
      }
      else if(keyCode == KEY_DOWN)  // Down - decrease delay
      {  /* braces required */
         if(mEditingSpecialAttrItem->repopDelay > 0)
            mEditingSpecialAttrItem->repopDelay--;
      }
   }
   else if(mSpecialAttribute == GoFastSpeed)
   {
      if(keyCode == KEY_UP)         // Up - increase speed
      {  /* braces required */
         if(mEditingSpecialAttrItem->speed < SpeedZone::maxSpeed)
            mEditingSpecialAttrItem->speed += 10;
      }
      else if(keyCode == KEY_DOWN)  // Down - decrease speed
      {  /* braces required */
         if(mEditingSpecialAttrItem->speed > SpeedZone::minSpeed)
            mEditingSpecialAttrItem->speed -= 10;
      }
   }
   else if(mSpecialAttribute == GoFastSnap)
   {
      if(keyCode == KEY_UP || keyCode == KEY_DOWN)   // Up/Down - toggle snapping
      {  /* braces required */
         mEditingSpecialAttrItem->boolattr = !mEditingSpecialAttrItem->boolattr;
      }
   }

   mEditingSpecialAttrItem->onAttrsChanging(getCurrentScale());
}


void EditorUserInterface::itemPropertiesEnterKeyHandler()
{
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i]->isSelected())
      {
         // Force item i to be the one and only selected item type.  This will clear up some problems that
         // might otherwise occur.  If you have multiple items selected, all will end up with the same values
         //mItems[i]->setSelected(true);

         for(S32 j = 0; j < mItems.size(); j++)
            if(mItems[j]->isSelected() && mItems[j]->getObjectTypeMask() != mItems[i]->getObjectTypeMask())
               unselectItem(j);

         if(mEditingSpecialAttrItem)
            mEditingSpecialAttrItem->setIsBeingEdited(false);
         mEditingSpecialAttrItem = mItems[i];
         mEditingSpecialAttrItem->setIsBeingEdited(true);
         mSpecialAttribute = (SpecialAttribute)getNextAttr(i);

         if(mSpecialAttribute != NoAttribute)
            saveUndoState();
         else
            doneEditingSpecialItem(true);

         break;
      }
   }
}


void EditorUserInterface::onKeyUp(KeyCode keyCode)
{
   switch(keyCode)
   {
      case KEY_UP:
         mIn = false;
         // fall-through OK
      case KEY_W:
         mUp = false;
         break;
      case KEY_DOWN:
         mOut = false;
         // fall-through OK
      case KEY_S:
         mDown = false;
         break;
      case KEY_LEFT:
      case KEY_A:
         mLeft = false;
         break;
      case KEY_RIGHT:
      case KEY_D:
         mRight = false;
         break;
      case KEY_E:
         mIn = false;
         break;
      case KEY_C:
         mOut = false;
         break;
      case KEY_SPACE:
         mSnapDisabled = false;
         break;
      case KEY_TAB:
         mShowingReferenceShip = false;
         break;
      case MOUSE_LEFT:
      case MOUSE_RIGHT:  
         mMousePos.set(gScreenInfo.getMousePos());

         if(mDragSelecting)      // We were drawing a rubberband selection box
         {
            Rect r(convertCanvasToLevelCoord(mMousePos), mMouseDownPos);
            S32 j;

            for(S32 i = 0; i < mItems.size(); i++)
            {
               // Skip hidden items
               if(mShowMode == ShowWallsOnly)
               {
                  if(mItems[i]->getObjectTypeMask() & ~ItemBarrierMaker && mItems[i]->getObjectTypeMask() & ~ItemPolyWall)
                     continue;
               }
               else if(mShowMode == ShowAllButNavZones)
               {
                  if(mItems[i]->getObjectTypeMask() & ItemNavMeshZone)
                     continue;
               }

               // Make sure that all vertices of an item are inside the selection box; basically means that the entire 
               // item needs to be surrounded to be included in the selection
               for(j = 0; j < mItems[i]->getVertCount(); j++)
                  if(!r.contains(mItems[i]->getVert(j)))
                     break;
               if(j == mItems[i]->getVertCount())
                  mItems[i]->setSelected(true);
            }
            mDragSelecting = false;
         }
         else if(mDraggingObjects)     // We were dragging and dropping.  Could have been a move or a delete (by dragging to dock).
         {
            if(mAddingVertex)
            {
               deleteUndoState();
               mAddingVertex = false;
            }

            finishedDragging();
         }

         break;
   }     // case
}


// Called when user has been dragging an object and then releases it
void EditorUserInterface::finishedDragging()
{
   mDraggingObjects = false;

   if(mouseOnDock())                      // Mouse is over the dock -- either dragging to or from dock
   {
      if(mDraggingDockItem == NONE)       // This was really a delete (item dragged to dock)
      {
         for(S32 i = 0; i < mItems.size(); i++)    //  Delete all selected items
            if(mItems[i]->isSelected())
            {
               deleteItem(i);
               i--;
            }
      }
      else        // Dragged item off the dock, then back on  ==> nothing changed; restore to unmoved state, which was stored on undo stack
         undo(false);
   }
   else    // Mouse not on dock... we were either dragging from the dock or moving something, 
   {       // need to save an undo state if anything changed
      if(mDraggingDockItem == NONE)    // Not dragging from dock - user is moving object around screen
      {
         // If our snap vertex has moved then all selected items have moved
         bool itemsMoved = mSnapVertex_i->getVert(mSnapVertex_j) != mMoveOrigin;

         if(itemsMoved)    // Move consumated... update any moved items, and save our autosave
         {
            for(S32 i = 0; i < mItems.size(); i++)
               if(mItems[i]->isSelected() || mItems[i]->anyVertsSelected())
                  mItems[i]->onGeomChanged(getCurrentScale());

            mNeedToSave = true;
            autoSave();
            return;
         }
         else     // We started our move, then didn't end up moving anything... remove associated undo state
            deleteUndoState();
      }
   }
}


bool EditorUserInterface::mouseOnDock()
{
   return (mMousePos.x >= gScreenInfo.getGameCanvasWidth() - DOCK_WIDTH - horizMargin &&
           mMousePos.x <= gScreenInfo.getGameCanvasWidth() - horizMargin &&
           mMousePos.y >= gScreenInfo.getGameCanvasHeight() - vertMargin - getDockHeight(mShowMode) &&
           mMousePos.y <= gScreenInfo.getGameCanvasHeight() - vertMargin);
}


bool EditorUserInterface::anyItemsSelected()
{
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->isSelected())
         return true;

   return false;
}


bool EditorUserInterface::anythingSelected()
{
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i]->isSelected() || mItems[i]->anyVertsSelected() )
         return true;
   }

   return false;
}


void EditorUserInterface::idle(U32 timeDelta)
{
   F32 pixelsToScroll = timeDelta * (getKeyState(KEY_SHIFT) ? 1.0f : 0.5f);    // Double speed when shift held down

   if(mLeft && !mRight)
      mCurrentOffset.x += pixelsToScroll;
   else if(mRight && !mLeft)
      mCurrentOffset.x -= pixelsToScroll;
   if(mUp && !mDown)
      mCurrentOffset.y += pixelsToScroll;
   else if(mDown && !mUp)
      mCurrentOffset.y -= pixelsToScroll;

   Point mouseLevelPoint = convertCanvasToLevelCoord(mMousePos);

   if(mIn && !mOut)
      mCurrentScale *= 1 + timeDelta * 0.002;
   else if(mOut && !mIn)
      mCurrentScale *= 1 - timeDelta * 0.002;

   if(mCurrentScale > MIN_SCALE)
     mCurrentScale = MIN_SCALE;
   else if(mCurrentScale < MAX_SCALE)
      mCurrentScale = MAX_SCALE;

   Point newMousePoint = convertLevelToCanvasCoord(mouseLevelPoint);
   mCurrentOffset += mMousePos - newMousePoint;

   mSaveMsgTimer.update(timeDelta);
   mWarnMsgTimer.update(timeDelta);

   LineEditor::updateCursorBlink(timeDelta);
}

// Unused??
static void escapeString(const char *src, char dest[1024])
{
   S32 i;
   for(i = 0; src[i]; i++)
      if(src[i] == '\"' || src[i] == ' ' || src[i] == '\n' || src[i] == '\t')
         break;

   if(!src[i])
   {
      strcpy(dest, src);
      return;
   }
   char *dptr = dest;
   *dptr++ = '\"';
   char c;
   while((c = *src++) != 0)
   {
      switch(c)
      {
         case '\"':
            *dptr++ = '\\';
            *dptr++ = '\"';
            break;
         case '\n':
            *dptr++ = '\\';
            *dptr++ = 'n';
            break;
         case '\t':
            *dptr++ = '\\';
            *dptr++ = 't';
            break;
         default:
            *dptr++ = c;
      }
   }
   *dptr++ = '\"';
   *dptr++ = 0;
}

void EditorUserInterface::setSaveMessage(string msg, bool savedOK)
{
   mSaveMsg = msg;
   mSaveMsgTimer = saveMsgDisplayTime;
   mSaveMsgColor = (savedOK ? Color(0, 1, 0) : red);
}

void EditorUserInterface::setWarnMessage(string msg1, string msg2)
{
   mWarnMsg1 = msg1;
   mWarnMsg2 = msg2;
   mWarnMsgTimer = warnMsgDisplayTime;
   mWarnMsgColor = gErrorMessageTextColor;
}


bool EditorUserInterface::saveLevel(bool showFailMessages, bool showSuccessMessages, bool autosave)
{
   string saveName = autosave ? "auto.save" : mEditFileName;

   try
   {
      // Check if we have a valid (i.e. non-null) filename
      if(saveName == "")
      {
         gErrorMsgUserInterface.reset();
         gErrorMsgUserInterface.setTitle("INVALID FILE NAME");
         gErrorMsgUserInterface.setMessage(1, "The level file name is invalid or empty.  The level cannot be saved.");
         gErrorMsgUserInterface.setMessage(2, "To correct the problem, please change the file name using the");
         gErrorMsgUserInterface.setMessage(3, "Game Parameters menu, which you can access by pressing [F3].");

         gErrorMsgUserInterface.activate();

         return false;
      }

      char fileNameBuffer[256];
      dSprintf(fileNameBuffer, sizeof(fileNameBuffer), "%s/%s", gConfigDirs.levelDir.c_str(), saveName.c_str());
      FILE *f = fopen(fileNameBuffer, "w");
      if(!f)
         throw(SaveException("Could not open file for writing"));

      // Write out our game parameters --> first one will be the gameType, along with all required parameters
      for(S32 i = 0; i < gGameParamUserInterface.gameParams.size(); i++)
         if(gGameParamUserInterface.gameParams[i].substr(0, 5) != "Team ")  // Don't write out teams here... do it below!
            s_fprintf(f, "%s\n", gGameParamUserInterface.gameParams[i].c_str());

      for(S32 i = 0; i < mTeams.size(); i++)
         s_fprintf(f, "Team %s %g %g %g\n", mTeams[i].getName().getString(),
            mTeams[i].color.r, mTeams[i].color.g, mTeams[i].color.b);

      // Write out all level items (do two passes; walls first, non-walls next, so turrets & forcefields have something to grab onto)
      for(S32 j = 0; j < 2; j++)
         for(S32 i = 0; i < mItems.size(); i++)
         {
            EditorObject *p = mItems[i];

            // Make sure we are writing wall items on first pass, non-wall items next
            if((p->getObjectTypeMask() & ItemBarrierMaker || p->getObjectTypeMask() & ItemPolyWall) != (j == 0))
               continue;

            p->saveItem(f);


            //s_fprintf(f, "%s", itemDef[mItems[i]->index].name);

            //// Write id if it's not 0
            //if(mItems[i]->id > 0)
            //   s_fprintf(f, "!%d", mItems[i]->id);

            //if(mItems[i]->hasTeam())
            //   s_fprintf(f, " %d", mItems[i]->getTeam());
            //if(mItems[i]->hasWidth())
            //   s_fprintf(f, " %g", mItems[i]->getWidth());
            //for(S32 j = 0; j < p.vertCount(); j++)
            //   s_fprintf(f, " %g %g ", p->getVert(j).x, p->getVert(j).y);

            //if(mItems[i]->getHasRepop() && mItems[i]->repopDelay != -1)
            //   s_fprintf(f, " %d", mItems[i]->repopDelay);
            //if(mItems[i]->getObjectTypeMask() & ItemSpeedZone)
            //   s_fprintf(f, " %d %s", mItems[i]->speed, mItems[i]->boolattr ? "SnapEnabled" : "");

            //s_fprintf(f, "\n");
         }
      fclose(f);
   }
   catch (SaveException &e)
   {
      if(showFailMessages)
         gEditorUserInterface.setSaveMessage("Error Saving: " + string(e.what()), false);
      return false;
   }

   if(!autosave)     // Doesn't count as a save!
   {
      mNeedToSave = false;
      mAllUndoneUndoLevel = mLastUndoIndex;     // If we undo to this point, we won't need to save
   }

   if(showSuccessMessages)
      gEditorUserInterface.setSaveMessage("Saved " + getLevelFileName(), true);

   return true;
}


// We need some local hook into the testLevelStart() below.  Ugly but apparently necessary.
void testLevelStart_local()
{
   gEditorUserInterface.testLevelStart();
}


extern void initHostGame(Address bindAddress, Vector<string> &levelList, bool testMode);
extern CmdLineSettings gCmdLineSettings;

void EditorUserInterface::testLevel()
{
   bool gameTypeError = false;

   if(strcmp(mGameType, GameType::validateGameType(mGameType)))
      gameTypeError = true;

   // With all the map loading error fixes, game should never crash!
   validateLevel();
   if(mLevelErrorMsgs.size() || mLevelWarnings.size() || gameTypeError)
   {
      gYesNoUserInterface.reset();
      gYesNoUserInterface.setTitle("LEVEL HAS PROBLEMS");

      for(S32 i = 0; i < mLevelErrorMsgs.size(); i++)
         gYesNoUserInterface.setMessage(i + 1, mLevelErrorMsgs[i].c_str());

      for(S32 i = 0; i < mLevelWarnings.size(); i++)
         gYesNoUserInterface.setMessage(i + 1, mLevelWarnings[i].c_str());

      S32 i = mLevelErrorMsgs.size() + mLevelWarnings.size() - 2;

      if(gameTypeError)
      {
         gYesNoUserInterface.setMessage(i + 1, "ERROR: GameType is invalid.");
         gYesNoUserInterface.setMessage(i + 2, "(Fix in Level Parameters screen [F3])");
         i+=2;
      }
      gYesNoUserInterface.setInstr("Press [Y] to start, [ESC] to cancel");
      gYesNoUserInterface.registerYesFunction(testLevelStart_local);   // testLevelStart_local() just calls testLevelStart() below
      gYesNoUserInterface.activate();

      return;
   }

   testLevelStart();
}


void EditorUserInterface::testLevelStart()
{
   string tmpFileName = mEditFileName;
   mEditFileName = "editor.tmp";

   glutSetCursor(GLUT_CURSOR_NONE);    // Turn off cursor
   bool nts = mNeedToSave;             // Save these parameters because they are normally reset when a level is saved.
   S32 auul = mAllUndoneUndoLevel;     // Since we're only saving a temp copy, we really shouldn't reset them...

   if(saveLevel(true, false))
   {
      mEditFileName = tmpFileName;     // Restore the level name

      mWasTesting = true;

      Vector<string> levelList;
      levelList.push_back("editor.tmp");
      initHostGame(Address(IPProtocol, Address::Any, 28000), levelList, true);
   }

   mNeedToSave = nts;                  // Restore saved parameters
   mAllUndoneUndoLevel = auul;
}


// Puts 
void EditorUserInterface::buildAllWallSegmentEdgesAndPoints()
{
   wallSegmentManager.deleteAllSegments();

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->getObjectTypeMask() & ItemBarrierMaker || mItems[i]->getObjectTypeMask() & ItemPolyWall)
         wallSegmentManager.buildWallSegmentEdgesAndPoints(mItems[i]);
}

////////////////////////////////////////
////////////////////////////////////////

EditorMenuUserInterface gEditorMenuUserInterface;

// Constructor
EditorMenuUserInterface::EditorMenuUserInterface()
{
   setMenuID(EditorMenuUI);
   mMenuTitle = "EDITOR MENU";
}


void EditorMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
}


extern IniSettings gIniSettings;
extern MenuItem *getWindowModeMenuItem();

//////////
// Editor menu callbacks
//////////

void reactivatePrevUICallback(U32 unused)
{
   gEditorUserInterface.reactivatePrevUI();
}

static void testLevelCallback(U32 unused)
{
   gEditorUserInterface.testLevel();
}


void returnToEditorCallback(U32 unused)
{
   gEditorUserInterface.saveLevel(true, true);  // Save level
   gEditorUserInterface.setSaveMessage("Saved " + gEditorUserInterface.getLevelFileName(), true);
   gEditorUserInterface.reactivatePrevUI();        // Return to editor
}

static void activateHelpCallback(U32 unused)
{
   gEditorInstructionsUserInterface.activate();
}

static void activateLevelParamsCallback(U32 unused)
{
   gGameParamUserInterface.activate();
}

static void activateTeamDefCallback(U32 unused)
{
   gTeamDefUserInterface.activate();
}

void quitEditorCallback(U32 unused)
{
   if(gEditorUserInterface.mNeedToSave)
   {
      gYesNoUserInterface.reset();
      gYesNoUserInterface.setInstr("Press [Y] to save, [N] to quit [ESC] to cancel");
      gYesNoUserInterface.setTitle("SAVE YOUR EDITS?");
      gYesNoUserInterface.setMessage(1, "You have not saved your edits to the level.");
      gYesNoUserInterface.setMessage(3, "Do you want to?");
      gYesNoUserInterface.registerYesFunction(saveLevelCallback);
      gYesNoUserInterface.registerNoFunction(backToMainMenuCallback);
      gYesNoUserInterface.activate();
   }
   else
      gEditorUserInterface.reactivateMenu(gMainMenuUserInterface);

   gEditorUserInterface.clearUndoHistory();        // Clear up a little memory
}

//////////

void EditorMenuUserInterface::setupMenus()
{
   menuItems.deleteAndClear();
   menuItems.push_back(new MenuItem(0, "RETURN TO EDITOR", reactivatePrevUICallback,    "", KEY_R));
   menuItems.push_back(getWindowModeMenuItem());
   menuItems.push_back(new MenuItem(0, "TEST LEVEL",       testLevelCallback,           "", KEY_T));
   menuItems.push_back(new MenuItem(0, "SAVE LEVEL",       returnToEditorCallback,      "", KEY_S));
   menuItems.push_back(new MenuItem(0, "INSTRUCTIONS",     activateHelpCallback,        "", KEY_I, keyHELP));
   menuItems.push_back(new MenuItem(0, "LEVEL PARAMETERS", activateLevelParamsCallback, "", KEY_L, KEY_F3));
   menuItems.push_back(new MenuItem(0, "MANAGE TEAMS",     activateTeamDefCallback,     "", KEY_M, KEY_F2));
   menuItems.push_back(new MenuItem(0, "QUIT",             quitEditorCallback,          "", KEY_Q, KEY_UNKNOWN));
}


void EditorMenuUserInterface::onEscape()
{
   glutSetCursor(GLUT_CURSOR_NONE);
   reactivatePrevUI();
}


// See if item with specified id is selected
bool EditorUserInterface::itemIsSelected(U32 id)
{
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i]->isSelected() && mItems[i]->getItemId() == id)
         return true;

   return false;
}


// Clear any borders associated with the specified zone
void EditorUserInterface::deleteBorderSegs(S32 zoneId)
{
 
   for(S32 i = 0; i < zoneBorders.size(); i++)
      if(zoneBorders[i].mOwner1 == zoneId || zoneBorders[i].mOwner2 == zoneId)
      {
         zoneBorders.erase_fast(i);
         i--;
      }
}


void EditorUserInterface::rebuildBorderSegs(U32 zoneId)
{
   deleteBorderSegs(zoneId);
   S32 i = NONE;

   for(S32 j = 0; j < mItems.size(); j++)
      if(mItems[j]->getItemId() == zoneId)
      {
         i = j;
         break;
      }

   if(i == NONE)  // Couldn't find any matching zones...
      return;

   for(S32 j = 0; j < mItems.size(); j++)
      checkZones(i, j);
}


void EditorUserInterface::rebuildAllBorderSegs()
{
   zoneBorders.clear();

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i]->getObjectTypeMask() & ~ItemNavMeshZone)
         continue;

      for(S32 j = i + 1; j < mItems.size(); j++)
         checkZones(i, j);   
   }
}


void EditorUserInterface::checkZones(S32 i, S32 j)
{
   static ZoneBorder zoneBorder;

   if(i == j || mItems[j]->getObjectTypeMask() & ~ItemNavMeshZone)
      return;      // Don't check self...

   // Do zones i and j touch?  First a quick and dirty bounds check:
   if(!mItems[i]->getExtent().intersectsOrBorders(mItems[j]->getExtent()))
      return;

   if(zonesTouch(mItems[i]->getVerts(), mItems[j]->getVerts(), 1 / getGridSize(), zoneBorder.borderStart, zoneBorder.borderEnd))
   {
      zoneBorder.mOwner1 = mItems[i]->getItemId();
      zoneBorder.mOwner2 = mItems[j]->getItemId();

      zoneBorders.push_back(zoneBorder);
   }
}


////////////////////////////////////////
////////////////////////////////////////

// Declare/initialize static variables
GridDatabase *WallEdge::mGridDatabase; 

// Constructor
WallEdge::WallEdge(const Point &start, const Point &end) 
{ 
   mStart = start; 
   mEnd = end; 

   setExtent(Rect(start, end)); 

   // Set some things required by DatabaseObject
   mObjectTypeMask = BarrierType;
}


// Destructor
WallEdge::~WallEdge()
{
    // Make sure object is out of the database
   getGridDatabase()->removeFromDatabase(this, this->getExtent()); 
}

////////////////////////////////////////
////////////////////////////////////////

// Declare/initialize static variables
Vector<WallEdge *> WallSegmentManager::mWallEdges; 
Vector<Point>  WallSegmentManager::mWallEdgePoints;
GridDatabase *WallSegmentManager::mGridDatabase;   


void WallSegmentManager::recomputeAllWallGeometry()
{
   gEditorUserInterface.buildAllWallSegmentEdgesAndPoints();

   mWallEdgePoints.clear();

   // Clip mWallSegments to create wallEdgePoints, which will be used below to create a new set of WallEdge objects
   clipAllWallEdges(mWallSegments, mWallEdgePoints);    


   mWallEdges.deleteAndClear();
   mWallEdges.resize(mWallEdgePoints.size() / 2);

   // Add clipped wallEdges to the spatial database
   for(S32 i = 0; i < mWallEdgePoints.size(); i+=2)
   {
      WallEdge *newEdge = new WallEdge(mWallEdgePoints[i], mWallEdgePoints[i+1]);    // Create the edge object
      
      newEdge->addToDatabase();

      mWallEdges[i/2] = newEdge;
   }
}


void WallSegmentManager::buildWallSegmentEdgesAndPoints(EditorObject *item)
{
   // Find any forcefields that terminate on this wall, and mark them for recalculation later
   Vector<EditorObject *> forcefields;    // A list of forcefields terminating on the wall segment that we'll be deleting

   S32 count = mWallSegments.size();                
   for(S32 i = 0; i < count; i++)
      if(mWallSegments[i]->mOwner == item->getItemId())     // Segment belongs to item
         for(S32 j = 0; j < gEditorUserInterface.mItems.size(); j++)
            if(gEditorUserInterface.mItems[j]->getObjectTypeMask() & ItemForceField && 
                  ( gEditorUserInterface.mItems[j]->forceFieldEndSegment == mWallSegments[i] ||
                    gEditorUserInterface.mItems[j]->forceFieldMountSegment == mWallSegments[i] ) )
               forcefields.push_back(gEditorUserInterface.mItems[j]);

   // Get rid of any existing segments that correspond to our item; we'll be building new ones
   deleteSegments(item->getSerialNumber());

   Rect allSegExtent;

   if(item->getObjectTypeMask() & ItemPolyWall)
   {
      WallSegment *newSegment = new WallSegment(item->getVerts(), item->getSerialNumber());
      mWallSegments.push_back(newSegment);
   }
   else
   {
      // Create a series of WallSegments, each representing a sequential pair of vertices on our wall
      for(S32 i = 0; i < item->extendedEndPoints.size(); i += 2)
      {
         WallSegment *newSegment = new WallSegment(item->extendedEndPoints[i], item->extendedEndPoints[i+1], 
                                                   item->getWidth() / getGridSize(), item->getSerialNumber() );    // Create the segment
         mWallSegments.push_back(newSegment);            // And add it to our master segment list
      }
   }

   for(S32 i = 0; i < mWallSegments.size(); i++)
   {
      mWallSegments[i]->addToDatabase();              // Add it to our spatial database

      Rect segExtent(mWallSegments[i]->corners);      // Calculate a bounding box around the segment
      if(i == 0)
         allSegExtent.set(segExtent);
      else
         allSegExtent.unionRect(segExtent);
   }

   item->setExtent(allSegExtent);

   // Alert all forcefields terminating on any of the wall segments we deleted and potentially recreated
   for(S32 j = 0; j < forcefields.size(); j++)  
      forcefields[j]->onGeomChanged(getCurrentScale());
}


// Static method, used above and from instructions
void WallSegmentManager::clipAllWallEdges(const Vector<WallSegment *> &wallSegments, Vector<Point> &wallEdges)
{
   Vector<Vector<Point> > inputPolygons, solution;

   for(S32 i = 0; i < wallSegments.size(); i++)
      inputPolygons.push_back(wallSegments[i]->corners);

   mergePolys(inputPolygons, solution);      // Merged wall segments are placed in solution

   unpackPolygons(solution, wallEdges);
}


// Takes a wall, finds all intersecting segments, and marks them invalid
void WallSegmentManager::invalidateIntersectingSegments(EditorObject *item)
{
   static Vector<DatabaseObject *> intersectingSegments;

   intersectingSegments.clear();    // TODO: Should be deleteAndClear?

   // Before we update our edges, we need to mark all intersecting segments using the invalid flag.
   // These will need new walls after we've moved our segment.
   for(S32 i = 0; i < mWallSegments.size(); i++)
      if(mWallSegments[i]->mOwner == item->getSerialNumber())      // Segment belongs to our item; look it up in the database
         getGridDatabase()->findObjects(EditorWallSegmentType, intersectingSegments, mWallSegments[i]->getExtent());

   for(S32 i = 0; i < intersectingSegments.size(); i++)
   {
      WallSegment *intersectingSegment = dynamic_cast<WallSegment *>(intersectingSegments[i]);

      // Reset the edges of all invalidated segments to their factory settings
      intersectingSegment->resetEdges();   
      intersectingSegment->invalid = true;
   }

   buildWallSegmentEdgesAndPoints(item);

   // Invalidate all segments that potentially intersect the changed segment in its new location
   intersectingSegments.clear();
   for(S32 i = 0; i < mWallSegments.size(); i++)
      if(mWallSegments[i]->mOwner == item->getSerialNumber())      // Segment belongs to our item, compare to all others
         getGridDatabase()->findObjects(EditorWallSegmentType, intersectingSegments, mWallSegments[i]->getExtent());

   for(S32 i = 0; i < intersectingSegments.size(); i++)
      dynamic_cast<WallSegment *>(intersectingSegments[i])->invalid = true;
}


// Called when a wall segment has somehow changed.  All current and previously intersecting segments 
// need to be recomputed.
void WallSegmentManager::computeWallSegmentIntersections(EditorObject *item)
{
   invalidateIntersectingSegments(item);     // TODO: Is this step still needed?
   recomputeAllWallGeometry();
}


void WallSegmentManager::deleteAllSegments()
{
   mWallSegments.deleteAndClear();
}


// Delete all wall segments owned by specified owner.
void WallSegmentManager::deleteSegments(U32 owner)
{
   S32 count = mWallSegments.size();

   for(S32 i = 0; i < count; i++)
      if(mWallSegments[i]->mOwner == owner)
      {
         delete mWallSegments[i];    // Destructor will remove segment from database
         mWallSegments.erase_fast(i);
         i--;
         count--;
      }
}


void WallSegmentManager::renderWalls(bool convert, F32 alpha)
{
   for(S32 i = 0; i < mWallSegments.size(); i++)
   {
      bool beingDragged = gEditorUserInterface.mDraggingObjects && gEditorUserInterface.itemIsSelected(mWallSegments[i]->mOwner);

      mWallSegments[i]->renderFill(beingDragged);
   }

   renderWallEdges(mWallEdgePoints);      // Render wall outlines
}


////////////////////////////////////////
////////////////////////////////////////


//// Default constructor -- only used when loading a file
//WorldItem::WorldItem(GameItems itemType, S32 itemId) : lineEditor(MAX_TEXTITEM_LEN)
//{
//   init(itemType, TEAM_NEUTRAL, 1, itemId, false);
//}


//// Primary constructor -- only used when constructing dock items
//WorldItem::WorldItem(GameItems itemType, Point pos, S32 xteam, bool isDockItem, F32 width, F32 height, U32 itemId) : lineEditor(MAX_TEXTITEM_LEN)
//{
//   init(itemType, xteam, width, itemId, isDockItem);
//
//   addVert(pos);
//
//   // Handle multiple-point items
//   if(itemDef[itemType].geom == geomSimpleLine)       // Start with diagonal line
//      addVert(pos + Point(width, height));
//
//   else if(getGeomType() == geomPoly)                    // Start with a height x width rect
//   {
//      addVert(pos + Point(width, 0));
//      addVert(pos + Point(width, height));
//      addVert(pos + Point(0, height));
//
//      initializePolyGeom();
//   }
//}


//void WorldItem::init(GameItems itemType, S32 xteam, F32 xwidth, U32 itemid, bool isDockItem)
//{
//   index = itemType;
//   team = xteam;
//   id = itemid;
//   mSelected = false;
//   mAnyVertsSelected = false;
//   mLitUp = false;
//   width = xwidth;
//   mId = getNextItemId();
//   mDockItem = isDockItem;
//   mScore = 1;
//
//   if(itemDef[itemType].hasText)
//   {
//      textSize = 30;
//      lineEditor.setString("Your text here");
//   }
//
//   repopDelay = getDefaultRepopDelay(itemType);
//
//   if(itemType == ItemSpeedZone)
//   {
//      speed = SpeedZone::defaultSpeed;
//      boolattr = SpeedZone::defaultSnap;
//   }
//   else
//   {
//      speed = -1;
//      boolattr = false;
//   }
//}


void EditorObject::initializePolyGeom()
{
   // TODO: Use the same code already in polygon
   if(getGeomType() == geomPoly)
   {
      Triangulate::Process(mVerts, *getPolyFillPoints());   // Populates fillPoints from polygon outline
      //TNLAssert(fillPoints.size() > 0, "Bogus polygon geometry detected!");

      setCentroid(findCentroid(mVerts));
      setExtent(Rect(mVerts));
   }

   forceFieldMountSegment = NULL;
}


void EditorObject::increaseWidth(S32 amt)
{
   S32 width = getWidth();

   width += amt - (S32) width % amt;    // Handles rounding

   if(width > Barrier::MAX_BARRIER_WIDTH)
      width = Barrier::MAX_BARRIER_WIDTH;

   setWidth(width);

   onGeomChanged(getCurrentScale());
}


void EditorObject::decreaseWidth(S32 amt)
{
   S32 width = getWidth();
   
   width -= ((S32) width % amt) ? (S32) width % amt : amt;      // Dirty, ugly thing

   if(width < Barrier::MIN_BARRIER_WIDTH)
      width = Barrier::MIN_BARRIER_WIDTH;

   setWidth(width);

   onGeomChanged(getCurrentScale());
}


// Radius of item in editor
S32 EditorObject::getRadius(F32 scale)
{
   if(getObjectTypeMask() & ItemBouncyBall)
      return TestItem::TEST_ITEM_RADIUS;
   else if(getObjectTypeMask() & ItemResource)
      return ResourceItem::RESOURCE_ITEM_RADIUS;
   else if(getObjectTypeMask() & ItemAsteroid)
      return S32((F32)Asteroid::ASTEROID_RADIUS * 0.75f);
   else if(getObjectTypeMask() & ItemSoccerBall)
      return SoccerBallItem::SOCCER_BALL_RADIUS;
   else if(getObjectTypeMask() & ItemTurret && renderFull(getObjectTypeMask(), scale, mDockItem, mSnapped))
      return 25;
   else return NONE;    // Use default
}


// Account for the fact that the apparent selection center and actual object center are not quite aligned
// Should be pushed down to the objects themselves
Point EditorObject::getEditorSelectionOffset(F32 scale)
{
   if(getObjectTypeMask() & ItemTurret && renderFull(getObjectTypeMask(), scale, mDockItem, mSnapped))
      return Point(0.0, .075);
   else if(getObjectTypeMask() & ItemForceField && renderFull(getObjectTypeMask(), scale, mDockItem, mSnapped))
      return Point(0.0, .035);     
   else
      return Point(0, 0);     // No offset for most items
}


S32 EditorObject::getDefaultRepopDelay(GameItems itemType)  
{
   if(itemType == ItemFlagSpawn)
      return FlagSpawn::defaultRespawnTime;
   else if(itemType == ItemAsteroidSpawn)
      return AsteroidSpawn::defaultRespawnTime;
   else if(itemType == ItemTurret)
      return Turret::defaultRespawnTime;
   else if(itemType == ItemForceField)
      return ForceFieldProjector::defaultRespawnTime;
   else if(itemType == ItemRepair)
      return RepairItem::defaultRespawnTime;
   else if(itemType == ItemEnergy)
      return EnergyItem::defaultRespawnTime;
   else
      return -1;
}


// First argument has already been removed before we get this.  This is just the parameter list.
bool EditorObject::processArguments(S32 argc, const char **argv)
{
   //// Figure out how many arguments an item should have
   //S32 minArgs = 2;
   //if(getGeomType() >= geomLine)
   //   minArgs += 2;
   //if(hasTeam())
   //   minArgs++;
   //if(hasText())
   //   minArgs += 2;        // Size and message
   //if(argc < minArgs)      // Not enough args, time to bail
   //   return false;

   //// Parse most game objects
   //if(getName())       // Item is listed in itemDef, near top of this file
   //{
   //   //index = static_cast<GameItems>(index);
   //   S32 arg = 0;

   //   // Should the following be moved to the constructor?  Probably...
   //   setTeam(TEAM_NEUTRAL);
   //   mSelected = false;

   //   if(hasTeam())
   //   {
   //      setTeam(atoi(argv[arg]));
   //      arg++;
   //   }
   //   if(hasWidth())
   //   {
   //      S32 width = atof(argv[arg]);

   //      // Enforce max wall width so things are consistent between editor and game
   //      if(getObjectTypeMask() & ItemBarrierMaker && width > Barrier::MAX_BARRIER_WIDTH)
   //         width = Barrier::MAX_BARRIER_WIDTH;

   //      setWidth(width);

   //      arg++;
   //   }

   //   if(getObjectTypeMask() & ItemTextItem)
   //   {
   //      for(S32 j = 0; j < 2; j++)       // Read two points...
   //      {
   //         Point p;
   //         p.read(argv + arg);
   //         addVert(p);
   //         arg+=2;
   //      }

   //      textSize = atoi(argv[arg]);    // ...and a textsize...
   //      arg++;

   //      string str;
   //      for(;arg < argc; arg ++)         // (no first part of for)
   //      {
   //         str += argv[arg];             // ...and glob the rest together as a string
   //         if (arg < argc - 1)
   //            str += " ";
   //      }

   //      lineEditor.setString(str);
   //   }
   //   else if(getObjectTypeMask() & ItemNexus && argc == 4)     // Old-school Zap! style Nexus definition --> note parallel code in HuntersNexusObject::processArguments
   //   {
   //      // Arg 0 will be HuntersNexusObject
   //      Point pos;
   //      pos.read(argv + 1);

   //      Point ext(50, 50);
   //      ext.set(atoi(argv[2]), atoi(argv[3]));
   //      ext /= getGridSize();

   //      addVert(Point(pos.x - ext.x, pos.y - ext.y));   // UL corner
   //      addVert(Point(pos.x + ext.x, pos.y - ext.y));   // UR corner
   //      addVert(Point(pos.x + ext.x, pos.y + ext.y));   // LR corner
   //      addVert(Point(pos.x - ext.x, pos.y + ext.y));   // LL corner
   //   }
   //   else        // Anything but a textItem or old-school NexusObject
   //   {
   //      S32 coords = argc;
   //      if(getObjectTypeMask() & ItemSpeedZone)
   //         coords = 4;    // 2 pairs of coords = 2 * 2 = 4

   //      for(;arg < coords; arg += 2) // (no first arg)
   //      {
   //         // Put a cap on the number of vertices in a polygon
   //         if(getGeom() == geomPoly && getVertCount() >= gMaxPolygonPoints)
   //            break;
   //         
   //         if(arg != argc - 1)
   //         {
   //            Point p;
   //            p.read(argv + arg);
   //            addVert(p);
   //         }
   //      }
   //   }

   //   // Add a default spawn time, which may well be overridden below
   //   repopDelay = getDefaultRepopDelay();

   //   // Repair, Energy, Turrets, Forcefields, FlagSpawns, AsteroidSpawns all have optional additional argument dealing with repair or repopulation
   //   if((mObjectTypeMask & ItemRepair || mObjectTypeMask & ItemEnergy || mObjectTypeMask & ItemAsteroidSpawn) && argc == 3)
   //      repopDelay = atoi(argv[2]);

   //   if( (mObjectTypeMask & ItemTurret || mObjectTypeMask & ItemForceField || mObjectTypeMask & ItemFlagSpawn) && argc == 4)
   //      repopDelay = atoi(argv[3]);

   //   // SpeedZones have 2 optional extra arguments
   //   if(mObjectTypeMask & ItemSpeedZone)
   //   {
   //      if(argc >= 5)
   //         speed = atoi(argv[4]);
   //      else
   //         speed = SpeedZone::defaultSpeed;

   //      if(argc >= 6)
   //         boolattr = true;
   //    }
   //}

   return true;
}


// Except for commented lines, this is the same as GameObject's addtoEditor; can probably be merged
void EditorObject::addToEditor(Game *game)
{
   TNLAssert(mGame == NULL, "Error: Object already in a game in GameObject::addToGame.");
   TNLAssert(game != NULL,  "Error: theGame is NULL in GameObject::addToGame.");

   game->addToGameObjectList(this);
   //mCreationTime = theGame->getCurrentTime();
   mGame = game;
   addToDatabase();
   //onAddedToGame(game);
   gEditorUserInterface.mItems.push_back(this);
}


void EditorObject::addToDock(Game *game)
{
   mGame = game;
   mDockItem = true;
   gEditorUserInterface.mDockItems.push_back(this);
}


void EditorObject::processEndPoints()
{
   if(getObjectTypeMask() & ItemBarrierMaker)
      Barrier::constructBarrierEndPoints(mVerts, getWidth() / getGridSize(), extendedEndPoints);

   else if(getObjectTypeMask() & ItemPolyWall)
   {
      extendedEndPoints.clear();
      for(S32 i = 1; i < getVertCount(); i++)
      {
         extendedEndPoints.push_back(mVerts[i-1]);
         extendedEndPoints.push_back(mVerts[i]);
      }

      // Close the loop
      extendedEndPoints.push_back(mVerts.last());
      extendedEndPoints.push_back(mVerts.first());
   }
}




// Draw barrier centerlines; wraps renderPolyline()  ==> lineItem, barrierMaker only
void EditorObject::renderPolylineCenterline(F32 alpha)
{
   // Render wall centerlines
   if(mSelected)
      glColor(SELECT_COLOR, alpha);
   else if(mLitUp && !mAnyVertsSelected)
      glColor(HIGHLIGHT_COLOR, alpha);
   else
      glColor(getTeamColor(mTeam), alpha);

   glLineWidth(WALL_SPINE_WIDTH);
   renderPolyline(getVerts());
   glLineWidth(gDefaultLineWidth);
}


// Select a single vertex.  This is the default selection we use most of the time
void EditorObject::selectVert(S32 vertIndex) 
{ 
   unselectVerts();
   aselectVert(vertIndex);
}


// Select an additional vertex (remember command line ArcInfo?)
void EditorObject::aselectVert(S32 vertIndex)
{
   mVertSelected[vertIndex] = true;
   mAnyVertsSelected = true;
}


// Unselect a single vertex, considering the possibility that there may be other selected vertices as well
void EditorObject::unselectVert(S32 vertIndex) 
{ 
   mVertSelected[vertIndex] = false;

   bool anySelected = false;
   for(S32 j = 0; j < (S32)mVertSelected.size(); j++)
      if(mVertSelected[j])
      {
         anySelected = true;
         break;
      }
   mAnyVertsSelected = anySelected;
}


// Unselect all vertices
void EditorObject::unselectVerts() 
{ 
   for(S32 j = 0; j < getVertCount(); j++) 
      mVertSelected[j] = false; 
   mAnyVertsSelected = false;
}


bool EditorObject::vertSelected(S32 vertIndex) 
{ 
   return mVertSelected[vertIndex]; 
}


void EditorObject::addVert(const Point &vert)
{
   mVerts.push_back(vert);
   mVertSelected.push_back(false);
}


void EditorObject::addVertFront(Point vert)
{
   mVerts.push_front(vert);
   mVertSelected.insert(mVertSelected.begin(), false);
}


void EditorObject::insertVert(Point vert, S32 vertIndex)
{
   mVerts.insert(vertIndex);
   mVerts[vertIndex] = vert;

   mVertSelected.insert(mVertSelected.begin() + vertIndex, false);
}


void EditorObject::setVert(const Point &vert, S32 vertIndex)
{
   mVerts[vertIndex] = vert;
}


void EditorObject::deleteVert(S32 vertIndex)
{
   mVerts.erase(vertIndex);
   mVertSelected.erase(mVertSelected.begin() + vertIndex);
}


void EditorObject::onGeomChanging(F32 currentScale)
{
   if(getGeomType() == geomPoly)
      onGeomChanged(currentScale);           // Allows poly fill to get reshaped as vertices move
}


// Item is being actively dragged
void EditorObject::onItemDragging()
{
   if(getObjectTypeMask() & ItemForceField)
     onGeomChanged(getCurrentScale());

   else if(getGeomType() == geomPoly && getObjectTypeMask() & ~ItemPolyWall)
      onGeomChanged(getCurrentScale());     // Allows poly fill to get dragged around with outline
}


WallSegmentManager *getWallSegmentManager()
{
   return gEditorUserInterface.getWallSegmentManager();
}


void EditorObject::onGeomChanged(F32 currentScale)
{
   // TODO: Delegate all this to the member objects
   if(getObjectTypeMask() & ItemBarrierMaker || getObjectTypeMask() & ItemPolyWall)
   {  
      // Fill extendedEndPoints from the vertices of our wall's centerline, or from PolyWall edges
      processEndPoints();

      if(getObjectTypeMask() & ItemPolyWall)     // Prepare interior fill triangulation
         initializePolyGeom();          // Triangulate, find centroid, calc extents

      getWallSegmentManager()->computeWallSegmentIntersections(this);

      gEditorUserInterface.recomputeAllEngineeredItems();      // Seems awfully lazy...  should only recompute items attached to altered wall

      // But if we're doing the above, we don't need to bother with the below... unless we stop being lazy
      //// Find any forcefields that might intersect our new wall segment and recalc them
      //for(S32 i = 0; i < gEditorUserInterface.mItems.size(); i++)
      //   if(gEditorUserInterface.mItems[i]->index == ItemForceField &&
      //                           gEditorUserInterface.mItems[i]->getExtent().intersects(getExtent()))
      //      gEditorUserInterface.mItems[i]->findForceFieldEnd();
   }

   else if(getObjectTypeMask() & ItemForceField)
   {
      findForceFieldEnd();    // Find the end-point of the projected forcefield
   }

   else if(getGeomType() == geomPoly)
      initializePolyGeom();

   if(getObjectTypeMask() & ItemNavMeshZone)
      gEditorUserInterface.rebuildBorderSegs(getItemId());
}


// TODO: Move this to forcefield
void EditorObject::findForceFieldEnd()
{
   // Load the corner points of a maximum-length forcefield into geom
   Vector<Point> geom;
   DatabaseObject *collObj;

   F32 scale = 1 / getGridSize();
   
   Point start = ForceFieldProjector::getForceFieldStartPoint(getVert(0), mAnchorNormal, scale);

   if(ForceField::findForceFieldEnd(getGridDatabase(), start, mAnchorNormal, scale, forceFieldEnd, &collObj))
      forceFieldEndSegment = dynamic_cast<WallSegment *>(collObj);
   else
      forceFieldEndSegment = NULL;

   ForceField::getGeom(start, forceFieldEnd, geom, scale);    
   setExtent(Rect(geom));
}



////////////////////////////////////////
////////////////////////////////////////

GridDatabase *WallSegment::mGridDatabase; // Declare static variable


// Regular constructor
WallSegment::WallSegment(const Point &start, const Point &end, F32 width, S32 owner) 
{ 
   // Calculate segment corners by expanding the extended end points into a rectangle
   Barrier::expandCenterlineToOutline(start, end, width, corners);   
   init(owner);
}


// PolyWall constructor
WallSegment::WallSegment(const Vector<Point> &points, S32 owner)
{
   corners = points;

   if(isWoundClockwise(points))
      corners.reverse();

   init(owner);
}


// Intialize, only called from constructors above
void WallSegment::init(S32 owner)
{
   // Recompute the edges based on our new corner points
   resetEdges();                                            

   Rect extent(corners);
   setExtent(extent); 

   // Drawing walls filled requires that points be triangluated
   Triangulate::Process(corners, triangulatedFillPoints);

   mOwner = owner; 
   invalid = false; 

   // Set some things required by DatabaseObject
   mObjectTypeMask = EditorWallSegmentType;
}

// Destructor
WallSegment::~WallSegment()
{ 
   // Make sure object is out of the database
   getGridDatabase()->removeFromDatabase(this, this->getExtent()); 

   // Find any forcefields that were using this as an end point and let them know the segment is gone.  Since 
   // segment is no longer in database, when we recalculate the forcefield, our endSegmentPointer will be reset.
   // This is a last-ditch effort to ensure that the pointers point at something real.
   for(S32 i = 0; i < gEditorUserInterface.mItems.size(); i++)
      if(gEditorUserInterface.mItems[i]->getObjectTypeMask() & ItemForceField && 
               (gEditorUserInterface.mItems[i]->forceFieldEndSegment == this || 
                gEditorUserInterface.mItems[i]->forceFieldMountSegment == this) )
         gEditorUserInterface.mItems[i]->onGeomChanged(getCurrentScale());       // Will force recalculation of mount and endpoint
   }

 
// Resets edges of a wall segment to their factory settings; i.e. 4 simple walls representing a simple outline
void WallSegment::resetEdges()
{
   Barrier::resetEdges(corners, edges);
}


void WallSegment::renderFill(bool renderLight)
{
   // We'll use the editor color most of the time; only in preview mode in the editor do we use the game color
   bool useGameColor = UserInterface::current && UserInterface::current->getMenuID() == EditorUI && 
                       gEditorUserInterface.isShowingReferenceShip();

   glDisableBlendfromLineSmooth;
   
   // Use true below because all segments are triangulated
   renderWallFill(triangulatedFillPoints, true, (useGameColor ? gIniSettings.wallFillColor : EDITOR_WALL_FILL_COLOR) * (renderLight ? 0.5 : 1));   
   glEnableBlendfromLineSmooth;
}


////////////////////////////////////////
////////////////////////////////////////
// Stores the selection state of a particular EditorObject.  Does not store the item itself
// Primary constructor
SelectionItem::SelectionItem(EditorObject *item)
{
   mSelected = item->isSelected();

   for(S32 i = 0; i < item->getVertCount(); i++)
      mVertSelected.push_back(item->vertSelected(i));
}


void SelectionItem::restore(EditorObject *item)
{
   item->setSelected(mSelected);
   item->unselectVerts();

   for(S32 i = 0; i < item->getVertCount(); i++)
      item->aselectVert(mVertSelected[i]);
}


////////////////////////////////////////
////////////////////////////////////////
// Selection stores the selection state of group of EditorObjects
// Constructor
Selection::Selection(Vector<EditorObject *> &items)
{
   for(S32 i = 0; i < items.size(); i++)
      mSelection.push_back(SelectionItem(items[i]));
}


void Selection::restore(Vector<EditorObject *> &items)
{
   for(S32 i = 0; i < items.size(); i++)
      mSelection[i].restore(items[i]);
}


////////////////////////////////////////
////////////////////////////////////////

 bool EditorObject::hasTeam()
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return true;
      case ItemSpeedZone: return false;
      case ItemSoccerBall: return false;
      case ItemFlag: return true;
      case ItemFlagSpawn: return true;
      case ItemBarrierMaker: return false;
      case ItemPolyWall: return false;
      case ItemLineItem: return true;
      case ItemTeleporter: return false;
      case ItemRepair: return false;
      case ItemEnergy: return false;
      case ItemBouncyBall: return false;
      case ItemAsteroid: return false;
      case ItemAsteroidSpawn: return false;
      case ItemMine: return false;
      case ItemSpyBug: return true;
      case ItemResource: return false;
      case ItemLoadoutZone: return true;
      case ItemNexus: return false;
      case ItemSlipZone: return false;
      case ItemTurret: return true;
      case ItemForceField: return true;
      case ItemGoalZone: return true;
      case ItemNavMeshZone: return false;
   }
   return true;
}


bool EditorObject::canBeNeutral()
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return true;
      case ItemSpeedZone: return true;
      case ItemSoccerBall: return false;
      case ItemFlag: return true;
      case ItemFlagSpawn: return true;
      case ItemBarrierMaker: return false;
      case ItemPolyWall: return false;
      case ItemLineItem: return true;
      case ItemTeleporter: return false;
      case ItemRepair: return false;
      case ItemEnergy: return false;
      case ItemBouncyBall: return false;
      case ItemAsteroid: return false;
      case ItemAsteroidSpawn: return false;
      case ItemMine: return true;
      case ItemSpyBug: return true;
      case ItemResource: return false;
      case ItemLoadoutZone: return true;
      case ItemNexus: return true;
      case ItemSlipZone: return true;
      case ItemTurret: return true;
      case ItemForceField: return true;
      case ItemGoalZone: return true;
      case ItemNavMeshZone: return true;
   }
   return true;
}

bool EditorObject::canBeHostile()
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return false;
      case ItemSpeedZone: return true;
      case ItemSoccerBall: return false;
      case ItemFlag: return true;
      case ItemFlagSpawn: return true;
      case ItemBarrierMaker: return false;
      case ItemPolyWall: return false;
      case ItemLineItem: return true;
      case ItemTeleporter: return false;
      case ItemRepair: return false;
      case ItemEnergy: return false;
      case ItemBouncyBall: return false;
      case ItemAsteroid: return false;
      case ItemAsteroidSpawn: return false;
      case ItemMine: return true;
      case ItemSpyBug: return true;
      case ItemResource: return false;
      case ItemLoadoutZone: return true;
      case ItemNexus: return true;
      case ItemSlipZone: return true;
      case ItemTurret: return true;
      case ItemForceField: return true;
      case ItemGoalZone: return true;
      case ItemNavMeshZone: return true;
   }
   return true;
}

GeomType EditorObject::getGeomType()     
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return geomPoint;
      case ItemSpeedZone: return geomSimpleLine;
      case ItemSoccerBall: return geomPoint;
      case ItemFlag: return geomPoint;
      case ItemFlagSpawn: return geomPoint;
      case ItemBarrierMaker: return geomLine;
      case ItemPolyWall: return geomPoly;
      case ItemLineItem: return geomLine;
      case ItemTeleporter: return geomSimpleLine;
      case ItemRepair: return geomPoint;
      case ItemEnergy: return geomPoint;
      case ItemBouncyBall: return geomPoint;
      case ItemAsteroid: return geomPoint;
      case ItemAsteroidSpawn: return geomPoint;
      case ItemMine: return geomPoint;
      case ItemSpyBug: return geomPoint;
      case ItemResource: return geomPoint;
      case ItemLoadoutZone: return geomPoly;
      case ItemNexus: return geomPoly;
      case ItemSlipZone: return geomPoly;
      case ItemTurret: return geomPoint;
      case ItemForceField: return geomPoint;
      case ItemGoalZone: return geomPoly;
      case ItemNavMeshZone: return geomPoly;
   }
   return geomNone;
}

bool EditorObject::getHasRepop()     
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return false;
      case ItemSpeedZone: return false;
      case ItemSoccerBall: return false;
      case ItemFlag: return false;
      case ItemFlagSpawn: return true;
      case ItemBarrierMaker: return false;
      case ItemPolyWall: return false;
      case ItemLineItem: return false;
      case ItemTeleporter: return false;
      case ItemRepair: return true;
      case ItemEnergy: return true;
      case ItemBouncyBall: return false;
      case ItemAsteroid: return false;
      case ItemAsteroidSpawn: return true;
      case ItemMine: return false;
      case ItemSpyBug: return false;
      case ItemResource: return false;
      case ItemLoadoutZone: return false;
      case ItemNexus: return false;
      case ItemSlipZone: return false;
      case ItemTurret: return true;
      case ItemForceField: return true;
      case ItemGoalZone: return false;
      case ItemNavMeshZone: return false;
   }
   return false;
}

const char *EditorObject::getEditorHelpString()     
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return "Location where ships start.  At least one per team is required. [G]";
      case ItemSpeedZone: return "Makes ships go fast in direction of arrow. [P]";
      case ItemSoccerBall: return "Soccer ball, can only be used in Soccer games.";
      case ItemFlag: return "Flag item, used by a variety of game types.";
      case ItemFlagSpawn: return "Location where flags (or balls in Soccer) spawn after capture.";
      case ItemBarrierMaker: return "Run-of-the-mill wall item.";
      case ItemPolyWall: return "Polygon wall barrier; create linear walls with right mouse click.";
      case ItemLineItem: return "Decorative linework.";
      case ItemTeleporter: return "Teleports ships from one place to another. [T]";
      case ItemRepair:  return "Repairs damage to ships. [B]";
      case ItemEnergy: return "Restores energy to ships";
      case ItemBouncyBall: return "Bouncy object that floats around and gets in the way.";
      case ItemAsteroid: return "Shootable asteroid object.  Just like the arcade game.";
      case ItemAsteroidSpawn: return "Periodically spawns a new asteroid.";
      case ItemMine: return "Mines can be prepositioned, and are are \"hostile to all\". [M]";
      case ItemSpyBug: return "Remote monitoring device that shows enemy ships on the commander's map. [Ctrl-B]";
      case ItemResource: return "Small bouncy object that floats around and gets in the way.";
      case ItemLoadoutZone: return "Area to finalize ship modifications.  Each team should have at least one.";
      case ItemNexus: return "Area to bring flags in Hunter game.  Cannot be used in other games.";
      case ItemSlipZone: return "Not yet implemented.";
      case ItemTurret: return "Creates shooting turret.  Can be on a team, neutral, or \"hostile to all\". [Y]";
      case ItemForceField: return "Creates a force field that lets only team members pass. [F]";
      case ItemGoalZone: return "Target area used in a variety of games.";
      case ItemNavMeshZone: return "Creates navigational mesh zone for robots.";
   }
   return "blug";
}


bool EditorObject::getSpecial()     
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return true;          
      case ItemSoccerBall: return true;     
      case ItemFlagSpawn: return true;      
      case ItemBouncyBall: return true;     
      case ItemAsteroid: return true;       
      case ItemAsteroidSpawn: return true;  
      case ItemMine: return true;   		
      case ItemResource: return true;   	
   }
   return false;
}


const char *EditorObject::getPrettyNamePlural()     
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return "Spawn points";   	
      case ItemSpeedZone: return "GoFasts";           	
      case ItemSoccerBall: return "Soccer balls";     	
      case ItemFlag: return "Flags";                  	
      case ItemFlagSpawn: return "Flag spawn points"; 	
      case ItemBarrierMaker: return "Barrier makers"; 	
      case ItemPolyWall: return "PolyWalls";          	
      case ItemLineItem: return "Decorative Lines";   	
      case ItemTeleporter: return "Teleporters";      	
      case ItemRepair: return "Repair items";         	
      case ItemEnergy: return "Energy items";         	
      case ItemBouncyBall: return "Test items";       	
      case ItemAsteroid: return "Asteroids";          	
      case ItemAsteroidSpawn: return "Asteroid spawn points";
      case ItemMine: return "Mines";                  	
      case ItemSpyBug: return "Spy bugs";             	
      case ItemResource: return "Resource items";     	
      case ItemLoadoutZone: return "Loadout zones";   	
      case ItemNexus: return "Nexus zones";           	
      case ItemSlipZone: return "Slip zones";         	
      case ItemTurret: return "Turrets";              	
      case ItemForceField: return "Force field projectors"; 	
      case ItemGoalZone: return "Goal zones";             	
      case ItemNavMeshZone: return "NavMesh Zones";       	
   }
   return "blug";
}


const char *EditorObject::getOnDockName()     
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return "Spawn";    		
      case ItemSpeedZone: return "GoFast";   		
      case ItemSoccerBall: return "Ball";     		
      case ItemFlag: return "Flag";     			
      case ItemFlagSpawn: return "FlagSpawn";		
      case ItemBarrierMaker: return "Wall";     		
      case ItemPolyWall: return "Wall";     			
      case ItemLineItem: return "LineItem"; 			
      case ItemTeleporter: return "Teleport"; 		
      case ItemRepair: return "Repair";    			
      case ItemEnergy: return "Enrg";     			
      case ItemBouncyBall: return "Test";     		
      case ItemAsteroid: return "Ast.";     			
      case ItemAsteroidSpawn: return "ASP";      		
      case ItemMine: return "Mine";     			
      case ItemSpyBug: return "Bug";      			
      case ItemResource: return "Res.";     			
      case ItemLoadoutZone: return "Loadout";  		
      case ItemNexus: return "Nexus";    			
      case ItemSlipZone: return "Slip Zone";			
      case ItemTurret: return "Turret";   			
      case ItemForceField: return "ForceFld"; 		
      case ItemGoalZone: return "Goal";     			
      case ItemNavMeshZone: return "NavMesh";  		
   }

   return "blug";
}


const char *EditorObject::getOnScreenName()     
{
   switch(getObjectTypeMask()) {
      case ItemSpawn: return "Spawn";        			
      case ItemSpeedZone: return "GoFast";       			
      case ItemSoccerBall: return "Ball";         			
      case ItemFlag: return "Flag";         				
      case ItemFlagSpawn: return "FlagSpawn";    			
      case ItemBarrierMaker: return "Wall";         			
      case ItemPolyWall: return "Wall";         			
      case ItemLineItem: return "LineItem";     			
      case ItemTeleporter: return "Teleport";     			
      case ItemRepair: return "Repair";      			
      case ItemEnergy: return "Energy";       			
      case ItemBouncyBall: return "Test Item";    			
      case ItemAsteroid: return "Asteroid";     			
      case ItemAsteroidSpawn: return "AsteroidSpawn";		
      case ItemMine: return "Mine";         				
      case ItemSpyBug: return "Spy Bug";      			
      case ItemResource: return "Resource";     			
      case ItemLoadoutZone: return "Loadout";      			
      case ItemNexus: return "Nexus";        			
      case ItemSlipZone: return "Slip Zone";    			
      case ItemTurret: return "Turret";       			
      case ItemForceField: return "ForceFld";     			
      case ItemGoalZone: return "Goal";         			
      case ItemNavMeshZone: return "NavMesh";      			
   }return "blug";
}

//
//const char *EditorObject::getName()     
//{
//   switch(getObjectTypeMask()) {			
//      case ItemSpawn: return "Spawn";        			
//      case ItemSpeedZone: return "SpeedZone";       	
//      case ItemSoccerBall: return "SoccerBallItem";    	
//      case ItemFlag: return "FlagItem";         		
//      case ItemFlagSpawn: return "FlagSpawn";    		
//      case ItemBarrierMaker: return "BarrierMaker";      	
//      case ItemPolyWall: return "PolyWall";         	
//      case ItemLineItem: return "LineItem";     		
//      case ItemTeleporter: return "Teleporter";     	
//      case ItemRepair: return "RepairItem";      		
//      case ItemEnergy: return "EnergyItem";       		
//      case ItemBouncyBall: return "TestItem";    		
//      case ItemAsteroid: return "Asteroid";     		
//      case ItemAsteroidSpawn: return "AsteroidSpawn";	
//      case ItemMine: return "Mine";         			  
//      case ItemSpyBug: return "SpyBug";      		 
//      case ItemResource: return "ResourceItem";     	
//      case ItemLoadoutZone: return "LoadoutZone";     
//      case ItemNexus: return "HuntersNexusObject";    
//      case ItemSlipZone: return "SlipZone";    		          	
//      case ItemTurret: return "Turret";       		             	
//      case ItemForceField: return "ForceFieldProjector"; 
//      case ItemGoalZone: return "GoalZone";         			
//      case ItemTextItem: return "TextItem";         			
//      case ItemNavMeshZone: return "BotNavMeshZone";     	
//   }return "blug";
//}


};

