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
#include "UIEditorMenus.h"    // For access to menu methods such as setObject
#include "EditorObject.h"

#include "UINameEntry.h"
#include "UIEditorInstructions.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UITeamDefMenu.h"
#include "UIGameParameters.h"
#include "UIErrorMessage.h"
#include "UIYesNo.h"
#include "gameObjectRender.h"
#include "ClientGame.h"  
#include "gameType.h"
#include "soccerGame.h"          // For Soccer ball radius
#include "huntersGame.h"         // For HuntersNexusObject def
#include "EngineeredItem.h"      // For Turret properties
#include "PickupItem.h"          // For RepairItem
#include "barrier.h"             // For DEFAULT_BARRIER_WIDTH
#include "item.h"                // For Asteroid defs
#include "teleporter.h"          // For Teleporter def
#include "speedZone.h"           // For Speedzone def
#include "loadoutZone.h"         // For LoadoutZone def
#include "huntersGame.h"         // For HuntersNexusObject def
#include "config.h"
#include "goalZone.h"

#include "gameLoader.h"          // For LevelLoadException def

#include "Colors.h"
#include "GeomUtils.h"
#include "textItem.h"            // For MAX_TEXTITEM_LEN and MAX_TEXT_SIZE
#include "luaLevelGenerator.h"
#include "stringUtils.h"

#include "oglconsole.h"          // Our console object
#include "ScreenInfo.h"

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"

#include <boost/shared_ptr.hpp>

#include <ctype.h>
#include <exception>
#include <algorithm>             // For sort
#include <math.h>

using namespace boost;

namespace Zap
{

const S32 DOCK_WIDTH = 50;          // Width of dock, in pixels
const F32 MIN_SCALE = .05f;         // Most zoomed-in scale
const F32 MAX_SCALE = 2.5;          // Most zoomed-out scale
const F32 STARTING_SCALE = 0.5;

//static Vector<boost::shared_ptr<EditorObject> > *mLoadTarget;
static EditorObjectDatabase *mLoadTarget;

static EditorObjectDatabase mLevelGenDatabase;     // Database for inserting objects when running a levelgen script in the editor

enum EntryMode {
   EntryID,          // Entering an objectID
   EntryAngle,       // Entering an angle
   EntryScale,       // Entering a scale
   EntryNone         // Not in a special entry mode
};


static EntryMode entryMode;
static Vector<ZoneBorder> zoneBorders;

static void saveLevelCallback(ClientGame *game)
{
   UIManager *uiManager = game->getUIManager();

   if(uiManager->getEditorUserInterface()->saveLevel(true, true))
      uiManager->reactivateMenu(uiManager->getMainMenuUserInterface());   
   else
      uiManager->getEditorUserInterface()->reactivate();
}


void backToMainMenuCallback(ClientGame *game)
{
   game->getUIManager()->reactivateMenu(game->getUIManager()->getMainMenuUserInterface());    
}


// Constructor
EditorUserInterface::EditorUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(EditorUI);

   // Create some items for the dock...  One of each, please!
   mShowMode = ShowAllObjects; 
   mWasTesting = false;

   mSnapObject = NULL;
   mSnapVertexIndex = NONE;
   mItemHit = NULL;
   mEdgeHit = NONE;

   setNeedToSave(false);

   mNewItem = NULL;

   mLastUndoStateWasBarrierWidthChange = false;

   mUndoItems.resize(UNDO_STATES);     // Create slots for all our undos... also creates a ton of empty dbs.  Maybe we should be using pointers?
}


// Encapsulate some ugliness
const Vector<EditorObject *> *EditorUserInterface::getObjectList()
{
   return getGame()->getEditorDatabase()->getObjectList();
}


void EditorUserInterface::addToDock(EditorObject* object)
{
   mDockItems.push_back(boost::shared_ptr<EditorObject>(object));
}


void EditorUserInterface::addDockObject(EditorObject *object, F32 xPos, F32 yPos)
{
   object->prepareForDock(getGame(), Point(xPos, yPos));       
   object->setTeam(mCurrentTeam);

   addToDock(object);
}


void EditorUserInterface::populateDock()
{
   mDockItems.clear();

   if(mShowMode == ShowAllObjects)
   {
      F32 xPos = (F32)gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH / 2;
      F32 yPos = 35;
      const F32 spacer = 35;

      addDockObject(new RepairItem(), xPos, yPos);
      //addDockObject(new ItemEnergy(), xPos + 10, yPos);
      yPos += spacer;

      addDockObject(new Spawn(), xPos, yPos);
      yPos += spacer;

      addDockObject(new ForceFieldProjector(), xPos, yPos);
      yPos += spacer;

      addDockObject(new Turret(), xPos, yPos);
      yPos += spacer;

      addDockObject(new Teleporter(), xPos, yPos);
      yPos += spacer;

      addDockObject(new SpeedZone(), xPos, yPos);
      yPos += spacer;

      addDockObject(new TextItem(), xPos, yPos);
      yPos += spacer;

      if(getGame()->getGameType()->getGameType() == GameType::SoccerGame)
         addDockObject(new SoccerBallItem(), xPos, yPos);
      else
         addDockObject(new FlagItem(), xPos, yPos);
      yPos += spacer;

      addDockObject(new FlagSpawn(), xPos, yPos);
      yPos += spacer;

      addDockObject(new Mine(), xPos - 10, yPos);
      addDockObject(new SpyBug(), xPos + 10, yPos);
      yPos += spacer;

      // These two will share a line
      addDockObject(new Asteroid(), xPos - 10, yPos);
      addDockObject(new AsteroidSpawn(), xPos + 10, yPos);
      yPos += spacer;

      addDockObject(new CircleSpawn(), xPos - 10, yPos);
      addDockObject(new Reactor(), xPos + 10, yPos);
      yPos += spacer;


      // These two will share a line
      addDockObject(new TestItem(), xPos - 10, yPos);
      addDockObject(new ResourceItem(), xPos + 10, yPos);
      yPos += 25;

      
      addDockObject(new LoadoutZone(), xPos, yPos);
      yPos += 25;

      if(getGame()->getGameType()->getGameType() == GameType::NexusGame)
      {
         addDockObject(new HuntersNexusObject(), xPos, yPos);
         yPos += 25;
      }
      else
      {
         addDockObject(new GoalZone(), xPos, yPos);
         yPos += 25;
      }

      addDockObject(new PolyWall(), xPos, yPos);
      yPos += spacer;
   }
}


//static Vector<DatabaseObject *> fillVector;     // Reusable container, now global in gridDB.h, having this here may cause linux compile errors

// Destructor -- unwind things in an orderly fashion
EditorUserInterface::~EditorUserInterface()
{
   clearDatabase(getGame()->getEditorDatabase());

   mDockItems.clear();
   mLevelGenItems.clear();
   mClipboard.clear();
   delete mNewItem;
}


void EditorUserInterface::clearDatabase(GridDatabase *database)
{
   fillVector.clear();
   database->findObjects(fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      database->removeFromDatabase(fillVector[i], fillVector[i]->getExtent());
      //delete fillVector[i];
   }
}


static const S32 NO_NUMBER = -1;

// Draw a vertex of a selected editor item  -- still used for snapping vertex
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
      glColor(*HIGHLIGHT_COLOR, alpha);
   else if(style == SelectedVertex)
      glColor(*SELECT_COLOR, alpha);
   else if(style == SnappingVertex)
      glColor(Colors::magenta, alpha);
   else
      glColor(Colors::red, alpha);

   drawSquare(v, (F32)size, !hollow);

   if(number != NO_NUMBER)     // Draw vertex numbers
   {
      glColor(Colors::white, alpha);
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


// Replaces the need to do a convertLevelToCanvasCoord on every point before rendering
void EditorUserInterface::setLevelToCanvasCoordConversion()
{
   F32 scale =  getCurrentScale();
   Point offset = getCurrentOffset();

   glTranslate(offset);
   glScale(scale);
} 


// Draws a line connecting points in mVerts
void EditorUserInterface::renderPolyline(const Vector<Point> *verts)
{
   glPushMatrix();
      setLevelToCanvasCoordConversion();
      renderPointVector(verts, GL_LINE_STRIP);
   glPopMatrix();
}


////////////////////////////////////
////////////////////////////////////

// Objects created with this method MUST be deleted!
// Returns NULL if className is invalid
static EditorObject *newEditorObject(const char *className)
{
   Object *theObject = Object::create(className);        // Create an object of the specified type
   TNLAssert(dynamic_cast<EditorObject *>(theObject), "This is not an EditorObject!");

   return dynamic_cast<EditorObject *>(theObject);       // Force our new object to be an EditorObject
}


// Removes most recent undo state from stack --> won't actually delete items on stack until we need the slot, or we quit
void EditorUserInterface::deleteUndoState()
{
   mLastUndoIndex--;
   mLastRedoIndex--; 
}


// Save the current state of the editor objects for later undoing
void EditorUserInterface::saveUndoState()
{
   // Use case: We do 5 actions, save, undo 2, redo 1, then do some new action.  
   // Our "no need to save" undo point is lost forever.
   if(mAllUndoneUndoLevel > mLastRedoIndex)     
      mAllUndoneUndoLevel = NONE;

   //copyItems(getObjectList(), mUndoItems[mLastUndoIndex % UNDO_STATES]);

   EditorObjectDatabase *eod = getGame()->getEditorDatabase();
   TNLAssert(eod, "bad!");

   EditorObjectDatabase *newDB = eod;     
   mUndoItems[mLastUndoIndex % UNDO_STATES] = boost::shared_ptr<EditorObjectDatabase>(new EditorObjectDatabase(*newDB));  // Make a copy

   mLastUndoIndex++;
   //mLastRedoIndex++; 
   mLastRedoIndex = mLastUndoIndex;

   if(mLastUndoIndex % UNDO_STATES == mFirstUndoIndex % UNDO_STATES)           // Undo buffer now full...
   {
      mFirstUndoIndex++;
      mAllUndoneUndoLevel -= 1;     // If this falls below 0, then we can't undo our way out of needing to save
   }
   
   setNeedToSave(mAllUndoneUndoLevel != mLastUndoIndex);
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

   mSnapObject = NULL;
   mSnapVertexIndex = NONE;

   if(mLastUndoIndex == mLastRedoIndex && !mRedoingAnUndo)
   {
      saveUndoState();
      mLastUndoIndex--;
      mLastRedoIndex--;
      mRedoingAnUndo = true;
   }

   mLastUndoIndex--;

   getGame()->setEditorDatabase(boost::dynamic_pointer_cast<GridDatabase>(mUndoItems[mLastUndoIndex % UNDO_STATES]));

   rebuildEverything();

   mLastUndoStateWasBarrierWidthChange = false;
   validateLevel();
}
   

void EditorUserInterface::redo()
{
   if(mLastRedoIndex != mLastUndoIndex)      // If there's a redo state available...
   {
      mSnapObject = NULL;
      mSnapVertexIndex = NONE;

      mLastUndoIndex++;
      getGame()->setEditorDatabase(mUndoItems[mLastUndoIndex % UNDO_STATES]);
      TNLAssert(mUndoItems[mLastUndoIndex % UNDO_STATES], "null!");

      rebuildEverything();
      validateLevel();
   }
}


void EditorUserInterface::rebuildEverything()
{
   Game *game = getGame();

   game->getWallSegmentManager()->recomputeAllWallGeometry(game->getEditorDatabase());
   resnapAllEngineeredItems();

   setNeedToSave(mAllUndoneUndoLevel != mLastUndoIndex);
   mItemToLightUp = NULL;
   autoSave();
}


void EditorUserInterface::resnapAllEngineeredItems()
{
   fillVector.clear();

   getGame()->getEditorDatabase()->findObjects((TestFunc)isEngineeredType, fillVector);

   WallSegmentManager *wallSegmentManager = getGame()->getWallSegmentManager();

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      EngineeredItem *engrObj = dynamic_cast<EngineeredItem *>(fillVector[i]);
      engrObj->mountToWall(engrObj->getVert(0), wallSegmentManager->getWallEdgeDatabase(), wallSegmentManager->getWallSegmentDatabase());
   }
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


extern TeamPreset gTeamPresets[];

void EditorUserInterface::setLevelFileName(string name)
{
   if(name == "")
      mEditFileName = "";
   else
      if(name.find('.') == string::npos)      // Append extension, if one is needed
         mEditFileName = name + ".level";
      // else... what?
}


void EditorUserInterface::setLevelGenScriptName(string line)
{
   mScriptLine = trim(line);
}


void EditorUserInterface::makeSureThereIsAtLeastOneTeam()
{
   if(getGame()->getTeamCount() == 0)
   {
      boost::shared_ptr<AbstractTeam> team = boost::shared_ptr<AbstractTeam>(new TeamEditor);
      team->setName(gTeamPresets[0].name);
      team->setColor(gTeamPresets[0].r, gTeamPresets[0].g, gTeamPresets[0].b);

      getGame()->addTeam(team);
   }
}


extern S32 gMaxPolygonPoints;

// Loads a level
void EditorUserInterface::loadLevel()
{
   ClientGame *game = getGame();

   // Initialize
   clearDatabase(game->getEditorDatabase());
   game->clearTeams();
   mSnapObject = NULL;
   mSnapVertexIndex = NONE;
   mAddingVertex = false;
   clearLevelGenItems();
   mLoadTarget = game->getEditorDatabase();
   mGameTypeArgs.clear();

   game->resetLevelInfo();

   GameType *gameType = new GameType;
   gameType->addToGame(game, game->getEditorDatabase());

   ConfigDirectories *folderManager = game->getSettings()->getConfigDirs();
   string fileName = joindir(folderManager->levelDir, mEditFileName).c_str();


   // Process level file --> returns true if file found and loaded, false if not (assume it's a new level)
   if(game->loadLevelFromFile(fileName, true, game->getEditorDatabase()))   
   {
      // Loaded a level!
      makeSureThereIsAtLeastOneTeam(); // Make sure we at least have one team
      validateTeams();                 // Make sure every item has a valid team
      validateLevel();                 // Check level for errors (like too few spawns)
   }
   else     
   {
      // New level!
      makeSureThereIsAtLeastOneTeam();                               // Make sure we at least have one team, like the man said.
   }

   clearUndoHistory();                 // Clean out undo/redo buffers
   clearSelection();                   // Nothing starts selected
   mShowMode = ShowAllObjects;         // Turn everything on
   setNeedToSave(false);               // Why save when we just loaded?
   mAllUndoneUndoLevel = mLastUndoIndex;
   populateDock();                     // Add game-specific items to the dock

   // Bulk-process new items, walls first
   const Vector<EditorObject *> *objList = getObjectList();

   //for(S32 i = 0; i < objList->size(); i++)
   //   objList->get(i)->processEndPoints();

   game->getWallSegmentManager()->recomputeAllWallGeometry(game->getEditorDatabase());
   
   // Snap all engineered items to the closest wall, if one is found
   resnapAllEngineeredItems();

   // Run onGeomChanged for all non-wall items (engineered items already had onGeomChanged run during resnap operation)
   /*fillVector.clear();
   mGame->getEditorDatabase()->findObjects(~(BarrierType | EngineeredType), fillVector);
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      EditorObject *obj = dynamic_cast<EditorObject *>(fillVector[i]);
      obj->onGeomChanged();
   }*/
}


extern OGLCONSOLE_Console gConsole;

void EditorUserInterface::clearLevelGenItems()
{
   mLevelGenItems.clear();
}


void EditorUserInterface::copyScriptItemsToEditor()
{
   if(mLevelGenItems.size() == 0)
      return;     // Print error message?

   saveUndoState();

   for(S32 i = 0; i < mLevelGenItems.size(); i++)
      mLevelGenItems[i]->addToEditor(getGame());
      
   mLevelGenItems.clear();    // Don't want to delete these objects... we just handed them off to the database!

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

   // Set the load target to the levelgen db, as that's where we want our items stored
   mLoadTarget = &mLevelGenDatabase;

   ConfigDirectories *folderManager = getGame()->getSettings()->getConfigDirs();
   runScript(folderManager, scriptName, scriptArgs);

   // Reset the target
   mLoadTarget = getGame()->getEditorDatabase();
}


// Runs an arbitrary lua script.  Command is first item in cmdAndArgs, subsequent items are the args, if any
void EditorUserInterface::runScript(const ConfigDirectories *folderManager, const string &scriptName, const Vector<string> &args)
{
   string name = folderManager->findLevelGenScript(scriptName);  // Find full name of levelgen script

   if(name == "")
   {
      logprintf(LogConsumer::LogWarning, "Warning: Could not find script \"%s\"",  scriptName.c_str());
      // TODO: Show an error to the user
      return;
   }

   // TODO: Uncomment the following, make it work again (commented out during refactor of editor load process)
   // Load the items
   //LuaLevelGenerator(name, args, mGame->getGridSize(), getEditorDatabase(), this, gConsole);
   
   // Process new items
   // Not sure about all this... may need to test
   // Bulk-process new items, walls first

   fillVector.clear();
   mLoadTarget->findObjects((TestFunc)isWallType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      EditorObject *obj = dynamic_cast<EditorObject *>(fillVector[i]);

      if(obj->getVertCount() < 2)      // Invalid item; delete
         mLoadTarget->removeFromDatabase(obj, obj->getExtent());

      if(obj->getObjectTypeNumber() == PolyWallTypeNumber)
         dynamic_cast<PolyWall *>(obj)->processEndPoints();
      else
         dynamic_cast<WallItem *>(obj)->processEndPoints();
   }

   // When I came through here in early june, there was nothing else here... shouldn't there be some handling of non-wall objects?  -CE
}


void EditorUserInterface::validateLevel()
{
   bool hasError = false;
   mLevelErrorMsgs.clear();
   mLevelWarnings.clear();

   bool foundSoccerBall = false;
   bool foundNexus = false;
   bool foundFlags = false;
   bool foundTeamFlags = false;
   bool foundTeamFlagSpawns = false;
   bool foundNeutralSpawn = false;

   vector<bool> foundSpawn;
   char buf[32];

   string teamList, teams;

   // First, catalog items in level
   S32 teamCount = getGame()->getTeamCount();
   foundSpawn.resize(teamCount);

   for(S32 i = 0; i < teamCount; i++)      // Initialize vector
      foundSpawn[i] = false;

   GridDatabase *gridDatabase = getGame()->getEditorDatabase();
      
   fillVector.clear();
   gridDatabase->findObjects(ShipSpawnTypeNumber, fillVector);
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      Spawn *spawn = dynamic_cast<Spawn *>(fillVector[i]);
      S32 team = spawn->getTeam();

      if(team == TEAM_NEUTRAL)
         foundNeutralSpawn = true;
      else if(team >= 0)
         foundSpawn[team] = true;
   }

   fillVector.clear();
   gridDatabase->findObjects(SoccerBallItemTypeNumber, fillVector);
   if(fillVector.size() > 0)
      foundSoccerBall = true;

   fillVector.clear();
   gridDatabase->findObjects(NexusTypeNumber, fillVector);
   if(fillVector.size() > 0)
      foundNexus = true;

   fillVector.clear();
   gridDatabase->findObjects(FlagTypeNumber, fillVector);
   for (S32 i = 0; i < fillVector.size(); i++)
   {
      foundFlags = true;
      FlagItem *flag = dynamic_cast<FlagItem *>(fillVector[i]);
      if(flag->getTeam() >= 0)
      {
         foundTeamFlags = true;
         break;
      }
   }

   fillVector.clear();
   gridDatabase->findObjects(FlagSpawnTypeNumber, fillVector);
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      FlagSpawn *flagSpawn = dynamic_cast<FlagSpawn *>(fillVector[i]);
      if(flagSpawn->getTeam() >= 0)
      {
         foundTeamFlagSpawns = true;
         break;
      }
   }

   // "Unversal errors" -- levelgens can't (yet) change gametype

   GameType *gameType = getGame()->getGameType();

   // Check for soccer ball in a a game other than SoccerGameType. Doesn't crash no more.
   if(foundSoccerBall && gameType->getGameType() != GameType::SoccerGame)
      mLevelWarnings.push_back("WARNING: Soccer ball can only be used in soccer game.");

   // Check for the nexus object in a non-hunter game. Does not affect gameplay in non-hunter game.
   if(foundNexus && gameType->getGameType() != GameType::NexusGame)
      mLevelWarnings.push_back("WARNING: Nexus object can only be used in Hunters game.");

   // Check for missing nexus object in a hunter game.  This cause mucho dolor!
   if(!foundNexus && gameType->getGameType() == GameType::NexusGame)
      mLevelErrorMsgs.push_back("ERROR: Nexus game must have a Nexus.");

   if(foundFlags && !gameType->isFlagGame())
      mLevelWarnings.push_back("WARNING: This game type does not use flags.");
   else if(foundTeamFlags && !gameType->isTeamFlagGame())
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


void EditorUserInterface::validateTeams()
{
   fillVector.clear();
   getGame()->getEditorDatabase()->findObjects(fillVector);
   
   validateTeams(fillVector);
}


// Check that each item has a valid team  (fixes any problems it finds)
void EditorUserInterface::validateTeams(const Vector<DatabaseObject *> &dbObjects)
{
   S32 teams = getGame()->getTeamCount();

   for(S32 i = 0; i < dbObjects.size(); i++)
   {
      EditorObject *obj = dynamic_cast<EditorObject *>(dbObjects[i]);
      S32 team = obj->getTeam();

      if(obj->hasTeam() && ((team >= 0 && team < teams) || team == TEAM_NEUTRAL || team == TEAM_HOSTILE))  
         continue;      // This one's OK

      if(team == TEAM_NEUTRAL && obj->canBeNeutral())
         continue;      // This one too

      if(team == TEAM_HOSTILE && obj->canBeHostile())
         continue;      // This one too

      if(obj->hasTeam())
         obj->setTeam(0);               // We know there's at least one team, so there will always be a team 0
      else if(obj->canBeHostile() && !obj->canBeNeutral())
         obj->setTeam(TEAM_HOSTILE); 
      else
         obj->setTeam(TEAM_NEUTRAL);    // We won't consider the case where hasTeam == canBeNeutral == canBeHostile == false
   }
}


// Search through editor objects, to make sure everything still has a valid team.  If not, we'll assign it a default one.
// Note that neutral/hostile items are on team -1/-2, and will be unaffected by this loop or by the number of teams we have.
void EditorUserInterface::teamsHaveChanged()
{
   bool teamsChanged = false;

   if(getGame()->getTeamCount() != mOldTeams.size())     // Number of teams has changed
      teamsChanged = true;
   else
      for(S32 i = 0; i < getGame()->getTeamCount(); i++)
      {
         AbstractTeam *team = getGame()->getTeam(i);

         if(mOldTeams[i].color != team->getColor() || mOldTeams[i].name != team->getName().getString()) // Color(s) or names(s) have changed
         {
            teamsChanged = true;
            break;
         }
      }

   if(!teamsChanged)       // Nothing changed, we're done here
      return;

   validateTeams();

   // TODO: I hope we can get rid of this in future... perhaps replace with mDockItems being stored in a database, and pass the database?
   Vector<DatabaseObject *> hackyjunk;
   hackyjunk.resize(mDockItems.size());
   for(S32 i = 0; i < mDockItems.size(); i++)
      hackyjunk[i] = mDockItems[i].get();

   validateTeams(hackyjunk);

   validateLevel();          // Revalidate level -- if teams have changed, requirements for spawns have too
   setNeedToSave(true);
   autoSave();
   mAllUndoneUndoLevel = -1; // This change can't be undone
}


string EditorUserInterface::getLevelFileName()
{
   return mEditFileName;
}


// Handle console input
// Valid commands: help, run, clear, quit, exit
void processEditorConsoleCommand(void *gamePtr, OGLCONSOLE_Console console, char *cmdline)
{
   ClientGame *game = (ClientGame *)gamePtr;

   Vector<string> words = parseString(cmdline);
   if(words.size() == 0)
      return;

   string cmd = lcase(words[0]);
   EditorUserInterface *ui = game->getUIManager()->getEditorUserInterface();

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
         ui->saveUndoState();
         words.erase(0);         // Get rid of "run", leaving script name and args

         string name = words[0];
         words.erase(0);

         ui->onBeforeRunScriptFromConsole();
         ui->runScript(game->getSettings()->getConfigDirs(), name, words);
         ui->onAfterRunScriptFromConsole();
      }
   }   

   else if(cmd == "clear")
      ui->clearLevelGenItems();

   else
      OGLCONSOLE_Output(console, "Unknown command: %s\n", cmd.c_str());
}


void EditorUserInterface::onBeforeRunScriptFromConsole()
{
   const Vector<EditorObject *> *objList = getObjectList();

   // Use selection as a marker -- will have to change in future
   for(S32 i = 0; i < objList->size(); i++)
      objList->get(i)->setSelected(true);
}


void EditorUserInterface::onAfterRunScriptFromConsole()
{
   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
      objList->get(i)->setSelected(!objList->get(i)->isSelected());

   rebuildEverything();
}


extern void actualizeScreenMode(bool);

void EditorUserInterface::onActivate()
{
   ConfigDirectories *folderManager = getGame()->getSettings()->getConfigDirs();

   if(folderManager->levelDir == "")      // Never did resolve a leveldir... no editing for you!
   {
      getUIManager()->reactivatePrevUI();     // Must come before the error msg, so it will become the previous UI when that one exits

      ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();
      ui->reset();
      ui->setTitle("HOUSTON, WE HAVE A PROBLEM");
      ui->setMessage(1, "No valid level folder was found..."); 
      ui->setMessage(2, "cannot start the level editor");
      ui->setMessage(4, "Check the LevelDir parameter in your INI file,");
      ui->setMessage(5, "or your command-line parameters to make sure");
      ui->setMessage(6, "you have correctly specified a valid folder.");
      ui->activate();

      return;
   }

   // Check if we have a level name:
   if(getLevelFileName() == "")         // We need to take a detour to get a level name
   {
      // Don't save this menu (false, below).  That way, if the user escapes out, and is returned to the "previous"
      // UI, they will get back to where they were before (prob. the main menu system), not back to here.
      getUIManager()->getLevelNameEntryUserInterface()->activate(false);

      return;
   }

   mLevelErrorMsgs.clear();
   mLevelWarnings.clear();

   mSaveMsgTimer.clear();

   mGameTypeArgs.clear();

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

   mSaveMsgTimer = 0;

   OGLCONSOLE_EnterKey(processEditorConsoleCommand);     // Setup callback for processing console commands


   actualizeScreenMode(false); 
}


void EditorUserInterface::onDeactivate()
{
   mDockItems.clear();     // Free some memory -- dock will be rebuilt when editor restarts
   actualizeScreenMode(true);
}


void EditorUserInterface::onReactivate()     // Run when user re-enters the editor after testing, among other things
{
   mDraggingObjects = false;  

   if(mWasTesting)
   {
      mWasTesting = false;
      mSaveMsgTimer.clear();

      getGame()->setGameType(mEditorGameType); 

      remove("editor.tmp");      // Delete temp file
   }


   if(mCurrentTeam >= getGame()->getTeamCount())
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
   F32 factor = (showMinorGridLines() ? 0.1f : 0.5f) * getGame()->getGridSize();     // Tenths or halves -- major gridlines are gridsize pixels apart

   return Point(floor(p.x / factor + 0.5) * factor, floor(p.y / factor + 0.5) * factor);
}


Point EditorUserInterface::snapPoint(Point const &p, bool snapWhileOnDock)
{
   if(mouseOnDock() && !snapWhileOnDock) 
      return p;      // No snapping!

   const Vector<EditorObject *> *objList = getObjectList();

   Point snapPoint(p);

   WallSegmentManager *wallSegmentManager = getGame()->getWallSegmentManager();

   if(mDraggingObjects)
   {
      // Mark all items being dragged as no longer being snapped -- only our primary "focus" item will be snapped
      for(S32 i = 0; i < objList->size(); i++)
         if(objList->get(i)->isSelected())
            objList->get(i)->setSnapped(false);
   
      // Turrets & forcefields: Snap to a wall edge as first (and only) choice
      if(isEngineeredType(mSnapObject->getObjectTypeNumber()))
      {
         EngineeredItem *engrObj = dynamic_cast<EngineeredItem *>(mSnapObject);
         return engrObj->mountToWall(snapPointToLevelGrid(p), wallSegmentManager->getWallEdgeDatabase(), 
                                                              wallSegmentManager->getWallSegmentDatabase());
      }
   }

   F32 maxSnapDist = 2 / (mCurrentScale * mCurrentScale);
   F32 minDist = maxSnapDist;

   // Where will we be snapping things?
   bool snapToWallCorners = getSnapToWallCorners();
   bool snapToLevelGrid = !mSnapDisabled;

   if(snapToLevelGrid)     // Lowest priority
   {
      snapPoint = snapPointToLevelGrid(p);
      minDist = snapPoint.distSquared(p);
   }

   // Now look for other things we might want to snap to
   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);
      // Don't snap to selected items or items with selected verts
      if(obj->isSelected() || obj->anyVertsSelected())    
         continue;

      for(S32 j = 0; j < obj->getVertCount(); j++)
      {
         F32 dist = obj->getVert(j).distSquared(p);
         if(dist < minDist)
         {
            minDist = dist;
            snapPoint.set(obj->getVert(j));
         }
      }
   }

   // Search for a corner to snap to - by using wall edges, we'll also look for intersections between segments
   if(snapToWallCorners)
      checkCornersForSnap(p, wallSegmentManager->mWallEdges, minDist, snapPoint);

   return snapPoint;
}


bool EditorUserInterface::getSnapToWallCorners()
{
   return !mSnapDisabled && mDraggingObjects && !(isWallType(mSnapObject->getObjectTypeNumber()));
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

// Not currently used 

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


// Not currently used 

S32 EditorUserInterface::checkEdgesForSnap(const Point &clickPoint, const Vector<WallEdge *> &edges, bool abcFormat,
                                           F32 &minDist, Point &snapPoint )
{
   S32 inc = abcFormat ? 1 : 2;   
   S32 segFound = NONE;

   for(S32 i = 0; i < edges.size(); i++)
      if(checkEdge(clickPoint, *edges[i]->getStart(), *edges[i]->getEnd(), minDist, snapPoint))
         segFound = i;

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

static bool fillRendered = false;


bool EditorUserInterface::showMinorGridLines()
{
   return mCurrentScale >= .5;
}


// Render background snap grid
void EditorUserInterface::renderGrid()
{
   if(mShowingReferenceShip)
      return;   

   F32 colorFact = mSnapDisabled ? .5f : 1;
   
   // Minor grid lines
   for(S32 i = 1; i >= 0; i--)
   {
      if((i && showMinorGridLines()) || !i)      // Minor then major gridlines
      {
         F32 gridScale = mCurrentScale * getGame()->getGridSize() * (i ? 0.1f : 1);    // Major gridlines are gridSize() pixels apart   
         F32 color = (i ? .2f : .4f) * colorFact;

         F32 xStart = fmod(mCurrentOffset.x, gridScale);
         F32 yStart = fmod(mCurrentOffset.y, gridScale);

         glColor3f(color, color, color);
         glBegin(GL_LINES);
            while(yStart < gScreenInfo.getGameCanvasHeight())
            {
               glVertex2f(0, yStart);
               glVertex2f((F32)gScreenInfo.getGameCanvasWidth(), yStart);
               yStart += gridScale;
            }
            while(xStart < gScreenInfo.getGameCanvasWidth())
            {
               glVertex2f(xStart, 0);
               glVertex2f(xStart, (F32)gScreenInfo.getGameCanvasHeight());
               xStart += gridScale;
            }
         glEnd();
      }
   }

   // Draw axes
   glColor3f(0.7f * colorFact, 0.7f * colorFact, 0.7f * colorFact);
   glLineWidth(gLineWidth3);

   Point origin = convertLevelToCanvasCoord(Point(0,0));

   glBegin(GL_LINES);
      glVertex2f(0, origin.y);
      glVertex2f((F32)gScreenInfo.getGameCanvasWidth(), origin.y);
      glVertex2f(origin.x, 0);
      glVertex2f(origin.x, (F32)gScreenInfo.getGameCanvasHeight());
   glEnd();

   glLineWidth(gDefaultLineWidth);
}


S32 getDockHeight(ShowMode mode)
{
   if(mode == ShowWallsOnly)
      return 62;
   else  // mShowMode == ShowAllObjects
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
      glColor(i ? Colors::black : (mouseOnDock() ? Colors::yellow : Colors::white));

      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2i(canvasWidth - DOCK_WIDTH - horizMargin, canvasHeight - vertMargin);
         glVertex2i(canvasWidth - horizMargin,              canvasHeight - vertMargin);
         glVertex2i(canvasWidth - horizMargin,              canvasHeight - vertMargin - dockHeight);
         glVertex2i(canvasWidth - DOCK_WIDTH - horizMargin, canvasHeight - vertMargin - dockHeight);
      glEnd();
   }

   // Draw coordinates on dock -- if we're moving an item, show the coords of the snap vertex, otherwise show the coords of the
   // snapped mouse position
   Point pos;
   if(mSnapObject)
      pos = mSnapObject->getVert(mSnapVertexIndex);
   else
      pos = snapPoint(convertCanvasToLevelCoord(mMousePos));

   F32 xpos = gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH / 2.f;

   char text[50];
   glColor(Colors::white);
   dSprintf(text, sizeof(text), "%2.2f|%2.2f", pos.x, pos.y);
   drawStringc(xpos, (F32)gScreenInfo.getGameCanvasHeight() - vertMargin - 15, 8, text);

   // And scale
   dSprintf(text, sizeof(text), "%2.2f", mCurrentScale);
   drawStringc(xpos, (F32)gScreenInfo.getGameCanvasHeight() - vertMargin - 25, 8, text);

   // Show number of teams
   dSprintf(text, sizeof(text), "Teams: %d",  getGame()->getTeamCount());
   drawStringc(xpos, (F32)gScreenInfo.getGameCanvasHeight() - vertMargin - 35, 8, text);

   glColor(mNeedToSave ? Colors::red : Colors::green);     // Color level name by whether it needs to be saved or not
   dSprintf(text, sizeof(text), "%s%s", mNeedToSave ? "*" : "", mEditFileName.substr(0, mEditFileName.find_last_of('.')).c_str());    // Chop off extension
   drawStringc(xpos, (F32)gScreenInfo.getGameCanvasHeight() - vertMargin - 45, 8, text);

   // And wall width as needed
   if(width != NONE)
   {
      glColor(Colors::white);
      dSprintf(text, sizeof(text), "Width: %2.0f", width);
      drawStringc(xpos, (F32)gScreenInfo.getGameCanvasHeight() - vertMargin - 55, 8, text);
   }
}


void EditorUserInterface::renderTextEntryOverlay()
{
   // Render id-editing overlay
   if(entryMode != EntryNone)
   {
      static const S32 fontsize = 16;
      static const S32 inset = 9;
      static const S32 boxheight = fontsize + 2 * inset;
      static const Color color(0.9, 0.9, 0.9);
      static const Color errorColor(Colors::red);

      bool errorFound = false;

      // Check for duplicate IDs if we're in ID entry mode
      if(entryMode == EntryID)
      {
         S32 id = atoi(mEntryBox.c_str());      // mEntryBox has digits only filter applied; ids can only be positive ints

         if(id != 0)    // Check for duplicates
         {
            const Vector<EditorObject *> *objList = getObjectList();

            for(S32 i = 0; i < objList->size(); i++)
               if(objList->get(i)->getItemId() == id && !objList->get(i)->isSelected())
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
         glColor(Color(.3f,.6f,.3f), i ? .85f : 1);

         glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
            glVertex2i(xpos,            ypos);
            glVertex2i(xpos + boxwidth, ypos);
            glVertex2i(xpos + boxwidth, ypos + boxheight);
            glVertex2i(xpos,            ypos + boxheight);
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
      glTranslate(mMousePos);
      glScale(mCurrentScale);
      glRotatef(90, 0, 0, 1);
      renderShip(&Colors::red, 1, thrusts, 1, 5, 0, false, false, false, false);
      glRotatef(-90, 0, 0, 1);

      // And show how far it can see
      F32 horizDist = Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL;
      F32 vertDist = Game::PLAYER_VISUAL_DISTANCE_VERTICAL;

      glEnableBlend;     // Enable transparency
      glColor4f(.5, .5, 1, .35f);
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
   return isScriptItem ? .6f : 1;     // Script items will appear somewhat translucent
}


const char *getModeMessage(ShowMode mode)
{
   if(mode == ShowWallsOnly)
      return "Wall editing mode.  Hit Ctrl-A to change.";
   else     // Normal mode
      return "";
}


// TODO: Need to render things in geometric order
void EditorUserInterface::render()
{
   mouseIgnore = false; // Needed to avoid freezing effect from too many mouseMoved events without a render in between (sam)

   renderGrid();        // Render grid first, so it's at the bottom

   // Render any items generated by the levelgen script... these will be rendered below normal items. 
   //glPushMatrix();
   //   setLevelToCanvasCoordConversion();

   //   glColor(Color(0,.25,0));
   //   for(S32 i = 0; i < mLevelGenItems.size(); i++)
   //      if(mLevelGenItems[i]->getObjectTypeMask() & BarrierType)
   //         for(S32 j = 0; j < mLevelGenItems[i]->extendedEndPoints.size(); j+=2)
   //            renderTwoPointPolygon(mLevelGenItems[i]->extendedEndPoints[j], mLevelGenItems[i]->extendedEndPoints[j+1], 
   //                                  mLevelGenItems[i]->getWidth() / getGridSize() / 2, GL_POLYGON);
   //glPopMatrix();

   for(S32 i = 0; i < mLevelGenItems.size(); i++)
      mLevelGenItems[i]->renderInEditor(mCurrentScale, mCurrentOffset, mSnapVertexIndex, true, mShowingReferenceShip, mShowMode);
   
   // Render polyWall item fill just before rendering regular walls.  This will create the effect of all walls merging together.  
   // PolyWall outlines are already part of the wallSegmentManager, so will be rendered along with those of regular walls.
   const Vector<EditorObject *> *objList = getObjectList();

   glPushMatrix();  
      setLevelToCanvasCoordConversion();
      for(S32 i = 0; i < objList->size(); i++)
      {
         EditorObject *obj = objList->get(i);
         if(obj->getObjectTypeNumber() == PolyWallTypeNumber)
         {
            PolyWall *wall = dynamic_cast<PolyWall *>(obj);
            wall->renderFill();
         }
      }
   
      getGame()->getWallSegmentManager()->renderWalls(
                     mDraggingObjects, mShowingReferenceShip, getSnapToWallCorners(), getRenderingAlpha(false/*isScriptItem*/));
   glPopMatrix();


   // == Normal items ==
   // Draw map items (teleporters, etc.) that are not being dragged, and won't have any text labels  (below the dock)
   // Don't render polywalls, as we've alrady drawn those.
   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);

      if(obj->getObjectTypeNumber() != PolyWallTypeNumber)
         if(!(mDraggingObjects && obj->isSelected()))
            obj->renderInEditor(mCurrentScale, mCurrentOffset, mSnapVertexIndex, false, mShowingReferenceShip, mShowMode);
   }


   // == Selected items ==
   // Draw map items (teleporters, etc.) that are are selected and/or lit up, so label is readable (still below the dock)
   // Do this as a separate operation to ensure that these are drawn on top of those drawn above.
   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);
      if(obj->isSelected() || obj->isLitUp())
         obj->renderInEditor(mCurrentScale, mCurrentOffset, mSnapVertexIndex, false, mShowingReferenceShip, mShowMode);
   }


   fillRendered = false;
   F32 width = NONE;

   if(mCreatingPoly || mCreatingPolyline)    // Draw geomPolyLine features under construction
   {
      mNewItem->addVert(snapPoint(convertCanvasToLevelCoord(mMousePos)));
      glLineWidth(gLineWidth3);

      if(mCreatingPoly) // Wall
         glColor(*SELECT_COLOR);
      else              // LineItem
         glColor(getTeamColor(mNewItem->getTeam()));

      renderPolyline(mNewItem->getOutline());

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
      fillVector.clear();
      getGame()->getEditorDatabase()->findObjects((TestFunc)isLineItemType, fillVector);

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         LineItem *obj = dynamic_cast<LineItem *>(fillVector[i]);   // Walls are a subclass of LineItem, so this will work for both
			TNLAssert(obj, "LineItem NULL?");
         if(obj && (obj->isSelected() || (obj->isLitUp() && obj->isVertexLitUp(NONE))))
         {
            width = (F32)obj->getWidth();
            break;
         }
      }
   }

   if(mShowingReferenceShip)
      renderReferenceShip();
   else
      renderDock(width);

   // Draw map items (teleporters, etc.) that are being dragged  (above the dock).  But don't draw walls here, or
   // we'll lose our wall centernlines.
   if(mDraggingObjects)
      for(S32 i = 0; i < objList->size(); i++)
      {
         EditorObject *obj = objList->get(i);
         if(obj->isSelected() && !isWallType(obj->getObjectTypeNumber()))    // Object is selected and is not a wall
            obj->renderInEditor(mCurrentScale, mCurrentOffset, mSnapVertexIndex, false, mShowingReferenceShip, mShowMode);
      }

   // Render our snap vertex as a hollow magenta box
   if(!mShowingReferenceShip && mSnapObject && mSnapObject->isSelected() && mSnapVertexIndex != NONE)      
      renderVertex(SnappingVertex, mSnapObject->getVert(mSnapVertexIndex) * mCurrentScale + mCurrentOffset, NO_NUMBER/*, alpha*/);  


   if(mDragSelecting)      // Draw box for selecting items
   {
      glColor(Colors::white);
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

         glColor3f(.1f, 1, .1f);

         // Center string between left side of screen and edge of dock
         S32 x = (S32)(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH - getStringWidth(15, helpString)) / 2;
         drawString(x, gScreenInfo.getGameCanvasHeight() - vertMargin - 15, 15, helpString);
      }
   }

   // Render dock items
   if(!mShowingReferenceShip)
      for(S32 i = 0; i < mDockItems.size(); i++)
      {
         mDockItems[i]->renderInEditor(mCurrentScale, mCurrentOffset, mSnapVertexIndex, false, false, mShowMode);
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

      glColor(Colors::yellow);

      for(S32 i = 0; i < mLevelWarnings.size(); i++)
      {
         drawCenteredString(ypos, 20, mLevelWarnings[i].c_str());
         ypos += 25;
      }
   }

   glColor(Colors::cyan);
   drawCenteredString(vertMargin, 14, getModeMessage(mShowMode));

   renderTextEntryOverlay();

   renderConsole();  // Rendered last, so it's always on top
}


const Color *EditorUserInterface::getTeamColor(S32 team)
{
   return getGame()->getTeamColor(team);
}


void EditorUserInterface::renderSnapTarget(const Point &target)
{
   glLineWidth(gLineWidth1);

   glColor(Colors::magenta);
   drawCircle(target, 5);

   glLineWidth(gDefaultLineWidth);
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


void EditorUserInterface::clearSelection()
{
   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
      objList->get(i)->unselect();
}


// Copy selection to the clipboard
void EditorUserInterface::copySelection()
{
   if(!anyItemsSelected())
      return;

   bool alreadyCleared = false;

   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
   {
      if(objList->get(i)->isSelected())
      {
         EditorObject *newItem =  objList->get(i)->newCopy();   
         newItem->setSelected(false);

         if(!alreadyCleared)  // Make sure we only purge the existing clipboard if we'll be putting someting new there
         {
            mClipboard.clear();
            alreadyCleared = true;
         }

         mClipboard.push_back(boost::shared_ptr<EditorObject>(newItem));
      }
   }
}


// Paste items on the clipboard
void EditorUserInterface::pasteSelection()
{
   if(mDraggingObjects)      // Pasting while dragging can cause crashes!!
      return;

   S32 itemCount = mClipboard.size();

    if(itemCount == 0)       // Nothing on clipboard, nothing to do
      return;

   saveUndoState();      // So we can undo the paste

   clearSelection();     // Only the pasted items should be selected

   Point pos = snapPoint(convertCanvasToLevelCoord(mMousePos));

   Point firstPoint = mClipboard[0]->getVert(0);
   Point offset;

   for(S32 i = 0; i < itemCount; i++)
   {
      offset = firstPoint - mClipboard[i]->getVert(0);

      EditorObject *newObject = mClipboard[i]->newCopy();
      newObject->assignNewSerialNumber();
      newObject->setExtent();
      newObject->addToDatabase(getGame()->getEditorDatabase());

      newObject->setSelected(true);
      newObject->moveTo(pos - offset);
      newObject->onGeomChanged();
   }

   validateLevel();
   setNeedToSave(true);
   autoSave();
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

   if(scale > 1 && min.distanceTo(max) * scale  > 50 * getGame()->getGridSize())    // If walls get too big, they'll bog down the db
      return;

   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
      if(objList->get(i)->isSelected())
         objList->get(i)->scale(ctr, scale);

   setNeedToSave(true);
   autoSave();
}


// Rotate selected objects around their center point by angle
void EditorUserInterface::rotateSelection(F32 angle)
{
   if(!anyItemsSelected())
      return;

   saveUndoState();

   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
      if(objList->get(i)->isSelected())
         objList->get(i)->rotateAboutPoint(Point(0,0), angle);

   setNeedToSave(true);
   autoSave();
}


// Find all objects in bounds 
// TODO: This should be a database function!
void EditorUserInterface::computeSelectionMinMax(Point &min, Point &max)
{
   min.set(F32_MAX, F32_MAX);
   max.set(-F32_MAX, -F32_MAX);

   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);

      if(obj->isSelected())
      {
         for(S32 j = 0; j < obj->getVertCount(); j++)
         {
            Point v = obj->getVert(j);

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

   if(anythingSelected())
      saveUndoState();

   if(currentTeam >= getGame()->getTeamCount())
   {
      char msg[255];

      if(getGame()->getTeamCount() == 1)
         dSprintf(msg, sizeof(msg), "Only 1 team has been configured.");
      else
         dSprintf(msg, sizeof(msg), "Only %d teams have been configured.", getGame()->getTeamCount());

      setWarnMessage(msg, "Hit [F2] to configure teams.");

      return;
   }

   // Update all dock items to reflect new current team
   for(S32 i = 0; i < mDockItems.size(); i++)
   {
      if(!mDockItems[i]->hasTeam())
         continue;

      if(currentTeam == TEAM_NEUTRAL && !mDockItems[i]->canBeNeutral())
         continue;

      if(currentTeam == TEAM_HOSTILE && !mDockItems[i]->canBeHostile())
         continue;

      mDockItems[i]->setTeam(currentTeam);
   }


   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);
      if(obj->isSelected())
      {
         if(!obj->hasTeam())
            continue;

         if(currentTeam == TEAM_NEUTRAL && !obj->canBeNeutral())
            continue;

         if(currentTeam == TEAM_HOSTILE && !obj->canBeHostile())
            continue;

         if(!anyChanged)
            saveUndoState();

         obj->setTeam(currentTeam);
         anyChanged = true;
      }
   }

   // Overwrite any warnings set above.  If we have a group of items selected, it makes no sense to show a
   // warning if one of those items has the team set improperly.  The warnings are more appropriate if only
   // one item is selected, or none of the items are given a valid team setting.

   if(anyChanged)
   {
      setWarnMessage("", "");
      validateLevel();
      setNeedToSave(true);
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
   F32 centerX = (min.x + max.x) / 2;

   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
      if(objList->get(i)->isSelected())
         objList->get(i)->flipHorizontal(centerX);

   setNeedToSave(true);
   autoSave();
}


void EditorUserInterface::flipSelectionVertical()
{
   if(!anyItemsSelected())
      return;

   saveUndoState();

   Point min, max;
   computeSelectionMinMax(min, max);
   F32 centerY = (min.y + max.y) / 2;

   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
      if(objList->get(i)->isSelected())
         objList->get(i)->flipVertical(centerY);

   setNeedToSave(true);
   autoSave();
}


static const S32 POINT_HIT_RADIUS = 9;
static const S32 EDGE_HIT_RADIUS = 6;

void EditorUserInterface::findHitItemAndEdge()
{
   mItemHit = NULL;
   mEdgeHit = NONE;
   mVertexHit = NONE;

   const Vector<EditorObject *> *objList = getObjectList();

   // Do this in two passes -- the first we only consider selected items, the second pass will consider all targets.
   // This will give priority to moving vertices of selected items
   for(S32 firstPass = 1; firstPass >= 0; firstPass--)     // firstPass will be true the first time through, false the second time
   {
      for(S32 i = objList->size() - 1; i >= 0; i--)        // Go in reverse order to prioritize items drawn on top
      {
         EditorObject *obj = objList->get(i);
         if(firstPass && !obj->isSelected() && !obj->anyVertsSelected())     // First pass is for selected items only
            continue;
         
         // Only select walls in CTRL-A mode...
         U8 type = obj->getObjectTypeNumber();
         if(mShowMode == ShowWallsOnly && !(isWallType(type)))        // Only select walls in CTRL-A mode
            continue;                                                              // ...so if it's not a wall, proceed to next item

         F32 radius = obj->getEditorRadius(mCurrentScale);

         for(S32 j = obj->getVertCount() - 1; j >= 0; j--)
         {
            // p represents pixels from mouse to obj->getVert(j), at any zoom
            Point p = mMousePos - mCurrentOffset - obj->getVert(j) * mCurrentScale;    

            if(fabs(p.x) < radius && fabs(p.y) < radius)
            {
               mItemHit = obj;
               mVertexHit = j;
               return;
            }
         }

         // This is all we can check on point items -- it makes no sense to check edges or other higher order geometry
         if(obj->getGeomType() == geomPoint)
            continue;

         /////
         // Didn't find a vertex hit... now we look for an edge

         // Make a copy of the items vertices that we can add to in the case of a loop
         Vector<Point> verts = *obj->getOutline();    

         if(obj->getGeomType() == geomPolygon)   // Add first point to the end to create last side on poly
            verts.push_back(verts.first());

         Point p1 = convertLevelToCanvasCoord(obj->getVert(0));
         Point closest;
         
         for(S32 j = 0; j < verts.size() - 1; j++)
         {
            Point p2 = convertLevelToCanvasCoord(verts[j+1]);
            
            if(findNormalPoint(mMousePos, p1, p2, closest))
            {
               F32 distance = (mMousePos - closest).len();
               if(distance < EDGE_HIT_RADIUS)
               {
                  mItemHit = obj;
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

   /////
   // If we're still here, it means we didn't find anything yet.  Make one more pass, and see if we're in any polys.
   // This time we'll loop forward, though I don't think it really matters.
   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);

      if(obj->getGeomType() == geomPolygon)
      {
         Vector<Point> verts;
         for(S32 j = 0; j < obj->getVertCount(); j++)
            verts.push_back(convertLevelToCanvasCoord(obj->getVert(j)));

         if(PolygonContains2(verts.address(), verts.size(), mMousePos))
         {
            mItemHit = obj;
            return;
         }
      }
   }
}


S32 EditorUserInterface::findHitItemOnDock(Point canvasPos)
{
   if(mShowMode == ShowWallsOnly)     // Only add dock items when objects are visible
      return NONE;

   for(S32 i = mDockItems.size() - 1; i >= 0; i--)     // Go in reverse order because the code we copied did ;-)
   {
      Point pos = mDockItems[i]->getVert(0);

      if(fabs(canvasPos.x - pos.x) < POINT_HIT_RADIUS && fabs(canvasPos.y - pos.y) < POINT_HIT_RADIUS)
         return i;
   }

   // Now check for polygon interior hits
   for(S32 i = 0; i < mDockItems.size(); i++)
      if(mDockItems[i]->getGeomType() == geomPolygon)
      {
         Vector<Point> verts;
         for(S32 j = 0; j < mDockItems[i]->getVertCount(); j++)
            verts.push_back(mDockItems[i]->getVert(j));

         if(PolygonContains2(verts.address(),verts.size(), canvasPos))
            return i;
      }

   return NONE;
}


// Incoming calls from GLUT come here...
void EditorUserInterface::onMouseMoved(S32 x, S32 y)
{
   onMouseMoved();      //... and go here
}


void EditorUserInterface::onMouseMoved()
{
   if(mouseIgnore)  // Needed to avoid freezing effect from too many mouseMoved events without a render in between
      return;

   mouseIgnore = true;

   // Doing this with MOUSE_RIGHT allows you to drag a vertex you just placed by holding the right-mouse button
   if(getKeyState(MOUSE_LEFT) || getKeyState(MOUSE_RIGHT))
   {
      onMouseDragged();
      return;
   }

   mMousePos.set(gScreenInfo.getMousePos());

   if(mCreatingPoly || mCreatingPolyline)
      return;

   //findHitVertex(mMousePos, vertexHitObject, vertexHit);      // Sets vertexHitObject and vertexHit
   findHitItemAndEdge();                                      //  Sets mItemHit, mVertexHit, and mEdgeHit

   // Unhighlight the currently lit up object, if any
   if(mItemToLightUp)
      mItemToLightUp->setLitUp(false);

   S32 vertexToLightUp = NONE;
   mItemToLightUp = NULL;

   // We hit a vertex that wasn't already selected
   if(mVertexHit != NONE && !mItemHit->vertSelected(mVertexHit))   
   {
      mItemToLightUp = mItemHit;
      mItemToLightUp->setVertexLitUp(mVertexHit);
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

   bool showMoveCursor = (mItemHit || mVertexHit != NONE || mItemHit || mEdgeHit != NONE || 
                         (mouseOnDock() && findHitItemOnDock(mMousePos) != NONE));


   findSnapVertex();

   // TODO:  was GLUT_CURSOR_SPRAY : GLUT_CURSOR_RIGHT_ARROW
   SDL_ShowCursor((showMoveCursor && !mShowingReferenceShip) ? SDL_ENABLE : SDL_ENABLE);
}


void EditorUserInterface::onMouseDragged()
{
   //if(mouseIgnore)  // Needed to avoid freezing effect from too many mouseMoved events without a render in between (sam)
   //   return;

   //mouseIgnore = true;

   mMousePos.set(gScreenInfo.getMousePos());

   if(mCreatingPoly || mCreatingPolyline || mDragSelecting)
      return;

   if(mDraggingDockItem != NONE)      // We just started dragging an item off the dock
       startDraggingDockItem();  

   findSnapVertex();

   if(!mSnapObject || mSnapVertexIndex == NONE)
      return;
   
   
   if(!mDraggingObjects)      // Just started dragging
   {
      mMoveOrigin = mSnapObject->getVert(mSnapVertexIndex);
      mOriginalVertLocations.clear();

      const Vector<EditorObject *> *objList = getObjectList();

      for(S32 i = 0; i < objList->size(); i++)
      {
         EditorObject *obj = objList->get(i);

         if(obj->isSelected() || obj->anyVertsSelected())
            for(S32 j = 0; j < obj->getVertCount(); j++)
               if(obj->isSelected() || obj->vertSelected(j))
                  mOriginalVertLocations.push_back(obj->getVert(j));
      }

      saveUndoState();
   }

   mDraggingObjects = true;



   Point delta;

   // The thinking here is that for large items -- walls, polygons, etc., we may grab an item far from its snap vertex, and we
   // want to factor that offset into our calculations.  For point items (and vertices), we don't really care about any slop
   // in the selection, and we just want the damn thing where we put it.
   if(mSnapObject->getGeomType() == geomPoint || (mItemHit && mItemHit->anyVertsSelected()))
      delta = snapPoint(convertCanvasToLevelCoord(mMousePos)) - mMoveOrigin;
   else
      delta = (snapPoint(convertCanvasToLevelCoord(mMousePos) + mMoveOrigin - mMouseDownPos) - mMoveOrigin );


   // Update coordinates of dragged item
   const Vector<EditorObject *> *objList = getObjectList();

   S32 count = 0;

   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);

      if(obj->isSelected() || obj->anyVertsSelected())
      {
         for(S32 j = 0; j < obj->getVertCount(); j++)
            if(obj->isSelected() || obj->vertSelected(j))
            {
               obj->setVert(mOriginalVertLocations[count] + delta, j);
               count++;
            }

         // If we are dragging a vertex, and not the entire item, we are changing the geom, so notify the item
         if(obj->anyVertsSelected())
            obj->onGeomChanging();  

         if(obj->isSelected())     
            obj->onItemDragging();      // Make sure this gets run after we've updated the item's location
      }
   }
}


EditorObject *EditorUserInterface::copyDockItem(S32 index)
{
   // Instantiate object so we are in essence dragging a non-dock item
   EditorObject *newObject = mDockItems[index]->newCopy();
   newObject->newObjectFromDock(getGame()->getGridSize());
   newObject->setExtent();
   newObject->setDockItem(false);
   newObject->clearGame();

   return newObject;
}


// User just dragged an item off the dock
void EditorUserInterface::startDraggingDockItem()
{
   EditorObject *item = copyDockItem(mDraggingDockItem);

   //item->initializeEditor(getGridSize());    // Override this to define some initial geometry for your object... 

   // Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
   Point pos = snapPoint(convertCanvasToLevelCoord(mMousePos), true) - item->getInitialPlacementOffset(getGame()->getGridSize());
   item->moveTo(pos);
      
   //item->setWidth((mDockItems[mDraggingDockItem]->getGeomType() == geomPolygon) ? .7 : 1);      // TODO: Still need this?
   item->addToEditor(getGame());          

   clearSelection();            // No items are selected...
   item->setSelected(true);     // ...except for the new one
   mDraggingDockItem = NONE;    // Because now we're dragging a real item
   validateLevel();             // Check level for errors


   // Because we sometimes have trouble finding an item when we drag it off the dock, after it's been sorted,
   // we'll manually set mItemHit based on the selected item, which will always be the one we just added.
   // TODO: Still needed?

   const Vector<EditorObject *> *objList = getObjectList();

   mEdgeHit = NONE;
   for(S32 i = 0; i < objList->size(); i++)
      if(objList->get(i)->isSelected())
      {
         mItemHit = objList->get(i);
         break;
      }
}


// Sets mSnapObject and mSnapVertexIndex based on the vertex closest to the cursor that is part of the selected set
// What we really want is the closest vertex in the closest feature
void EditorUserInterface::findSnapVertex()
{
   F32 closestDist = F32_MAX;

   if(mDraggingObjects)    // Don't change snap vertex once we're dragging
      return;

   mSnapObject = NULL;
   mSnapVertexIndex = NONE;

   Point mouseLevelCoord = convertCanvasToLevelCoord(mMousePos);

   // If we have a hit item, and it's selected, find the closest vertex in the item
   if(mItemHit && mItemHit->isSelected())   
   {
      // If we've hit an edge, restrict our search to the two verts that make up that edge
      if(mEdgeHit != NONE)
      {
         mSnapObject = mItemHit;     // Regardless of vertex, this is our hit item
         S32 v1 = mEdgeHit;
         S32 v2 = mEdgeHit + 1;

         // Handle special case of looping item
         if(mEdgeHit == mItemHit->getVertCount() - 1)
            v2 = 0;

         // Find closer vertex: v1 or v2
         mSnapVertexIndex = (mItemHit->getVert(v1).distSquared(mouseLevelCoord) < 
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
            mSnapObject = mItemHit;
            mSnapVertexIndex = j;
         }
      }
      return;
   } 

   const Vector<EditorObject *> *objList = getObjectList();

   // Otherwise, we don't have a selected hitItem -- look for a selected vertex
   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);

      for(S32 j = 0; j < obj->getVertCount(); j++)
      {
         // If we find a selected vertex, there will be only one, and this is our snap point
         if(obj->vertSelected(j))
         {
            mSnapObject = obj;
            mSnapVertexIndex = j;
            return;     
         }
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

   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = objList->size()-1; i >= 0; i--)  // Reverse to avoid having to have i-- in middle of loop
   {
      EditorObject *obj = objList->get(i);

      if(obj->isSelected())
      {  
         // Since indices change as items are deleted, this will keep incorrect items from being deleted
         if(obj->isLitUp())
            mItemToLightUp = NULL;

         if(!deleted)
            saveUndoState();

         deleteItem(i);
         deleted = true;
      }
      else if(!objectsOnly)      // Deleted any selected vertices
      {
         bool geomChanged = false;

         for(S32 j = 0; j < obj->getVertCount(); j++) 
         {
            if(obj->vertSelected(j))
            {
               
               if(!deleted)
                  saveUndoState();
              
               obj->deleteVert(j);
               deleted = true;

               geomChanged = true;
               mSnapObject = NULL;
               mSnapVertexIndex = NONE;
            }
         }

         // Deleted last vertex, or item can't lose a vertex... it must go!
         if(obj->getVertCount() == 0 || (obj->getGeomType() == geomSimpleLine && obj->getVertCount() < 2)
                                     || (obj->getGeomType() == geomPolyLine       && obj->getVertCount() < 2)
                                     || (obj->getGeomType() == geomPolygon       && obj->getVertCount() < 2))
         {
            deleteItem(i);
            deleted = true;
         }
         else if(geomChanged)
            obj->onGeomChanged();

      }  // else if(!objectsOnly) 
   }  // for

   if(deleted)
   {
      setNeedToSave(true);
      autoSave();

      mItemToLightUp = NULL;     // In case we just deleted a lit item; not sure if really needed, as we do this above
      //vertexToLightUp = NONE;
   }
}


// Increase selected wall thickness by amt
void EditorUserInterface::changeBarrierWidth(S32 amt)
{
   if(!mLastUndoStateWasBarrierWidthChange)
      saveUndoState(); 

   fillVector.clear();
   getGame()->getEditorDatabase()->findObjects((TestFunc)isLineItemType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      LineItem *obj = dynamic_cast<LineItem *>(fillVector[i]);   // Walls are a subclass of LineItem, so this will work for both
      if((obj->isSelected() || (obj->isLitUp() && obj->isVertexLitUp(NONE))))
         obj->changeWidth(amt);     
   }

   mLastUndoStateWasBarrierWidthChange = true;
}


// Split wall/barrier on currently selected vertex/vertices
void EditorUserInterface::splitBarrier()
{
   bool split = false;

   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);

      if(obj->getGeomType() == geomPolyLine)
          for(S32 j = 1; j < obj->getVertCount() - 1; j++)     // Can't split on end vertices!
            if(obj->vertSelected(j))
            {
               if(!split)
                  saveUndoState();
               split = true;

               // Create a poor man's copy
               EditorObject *newItem = obj->newCopy();
               //newItem->setTeam(-1);
               //newItem->setWidth(obj->getWidth());
               newItem->clearVerts();

               for(S32 k = j; k < obj->getVertCount(); k++) 
               {
                  newItem->addVert(obj->getVert(k));
                  if (k > j)
                  {
                     obj->deleteVert(k);     // Don't delete j == k vertex -- it needs to remain as the final vertex of the old wall
                     k--;
                  }
               }


               // Tell the new segments that they have new geometry
               obj->onGeomChanged();
               newItem->onGeomChanged();
               //mItems.push_back(boost::shared_ptr<EditorObject>(newItem));
               newItem->addToEditor(getGame());

               goto done2;                         // Yes, gotos are naughty, but they just work so well sometimes...
            }
   }
done2:
   if(split)
   {
      clearSelection();
      setNeedToSave(true);
      autoSave();
   }
}


// Join two or more sections of wall that have coincident end points.  Will ignore invalid join attempts.
void EditorUserInterface::joinBarrier()
{
   S32 joinedItem = NONE;

   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size()-1; i++)
   {
      EditorObject *obj_i = objList->get(i);

      if(obj_i->getGeomType() == geomPolyLine && (obj_i->isSelected()))
      {
         for(S32 j = i + 1; j < objList->size(); j++)
         {
            EditorObject *obj_j = objList->get(i);

            if(obj_j->getObjectTypeNumber() && obj_i->getObjectTypeNumber() && (obj_j->isSelected()))
            {
               if(obj_i->getVert(0).distanceTo(obj_j->getVert(0)) < .01)    // First vertices are the same  1 2 3 | 1 4 5
               {
                  if(joinedItem == NONE)
                     saveUndoState();
                  joinedItem = i;

                  for(S32 a = 1; a < obj_j->getVertCount(); a++)             // Skip first vertex, because it would be a dupe
                     obj_i->addVertFront(obj_j->getVert(a));

                  deleteItem(j);
                  i--;  j--;
               }
               // First vertex conincides with final vertex 3 2 1 | 5 4 3
               else if(obj_i->getVert(0).distanceTo(obj_j->getVert(obj_j->getVertCount()-1)) < .01)     
               {
                  if(joinedItem == NONE)
                     saveUndoState();

                  joinedItem = i;
                  for(S32 a = obj_j->getVertCount()-2; a >= 0; a--)
                     obj_i->addVertFront(obj_j->getVert(a));

                  deleteItem(j);
                  i--;  j--;

               }
               // Last vertex conincides with first 1 2 3 | 3 4 5
               else if(obj_i->getVert(obj_i->getVertCount()-1).distanceTo(obj_j->getVert(0)) < .01)     
               {
                  if(joinedItem == NONE)
                     saveUndoState();

                  joinedItem = i;

                  for(S32 a = 1; a < obj_j->getVertCount(); a++)  // Skip first vertex, because it would be a dupe         
                     obj_i->addVert(obj_j->getVert(a));

                  deleteItem(j);
                  i--;  j--;
               }
               else if(obj_i->getVert(obj_i->getVertCount()-1).distanceTo(obj_j->getVert(obj_j->getVertCount()-1)) < .01)     // Last vertices coincide  1 2 3 | 5 4 3
               {
                  if(joinedItem == NONE)
                     saveUndoState();

                  joinedItem = i;

                  for(S32 a = obj_j->getVertCount()-2; a >= 0; a--)
                     obj_i->addVert(obj_j->getVert(a));

                  deleteItem(j);
                  i--;  j--;
               }
            }
         }
      }
   }

   if(joinedItem != NONE)
   {
      clearSelection();
      setNeedToSave(true);
      autoSave();
      objList->get(joinedItem)->onGeomChanged();
   }
}


void EditorUserInterface::deleteItem(S32 itemIndex)
{
   const Vector<EditorObject *> *objList = getObjectList();
   EditorObject *obj = objList->get(itemIndex);

   Game *game = getGame();
   WallSegmentManager *wallSegmentManager = game->getWallSegmentManager();

   U8 type = obj->getObjectTypeNumber();

   if(isWallType(type))
   {
      // Need to recompute boundaries of any intersecting walls
      wallSegmentManager->invalidateIntersectingSegments(game->getEditorDatabase(), obj); // Mark intersecting segments invalid
      wallSegmentManager->deleteSegments(obj->getItemId());                             // Delete the segments associated with the wall

      game->getEditorDatabase()->removeFromDatabase(obj, obj->getExtent());

      wallSegmentManager->recomputeAllWallGeometry(game->getEditorDatabase());            // Recompute wall edges
      resnapAllEngineeredItems();         // Really only need to resnap items that were attached to deleted wall... but we
                                          // don't yet have a method to do that, and I'm feeling lazy at the moment
   }
   else
      game->getEditorDatabase()->removeFromDatabase(obj, obj->getExtent());

   // Reset a bunch of things
   mSnapObject = NULL;
   mSnapVertexIndex = NONE;
   mItemToLightUp = NULL;

   validateLevel();

   onMouseMoved();   // Reset cursor  
}


void EditorUserInterface::insertNewItem(U8 itemTypeNumber)
{
   if(mShowMode == ShowWallsOnly || mDraggingObjects)     // No inserting when items are hidden or being dragged!
      return;

   clearSelection();
   saveUndoState();

   S32 team = TEAM_NEUTRAL;

   EditorObject *newObject = NULL;

   // Find a dockItem to copy
   for(S32 i = 0; i < mDockItems.size(); i++)
      if(mDockItems[i]->getObjectTypeNumber() == itemTypeNumber)
      {
         newObject = copyDockItem(i);
         break;
      }
   TNLAssert(newObject, "Couldn't create object in insertNewItem()");

   newObject->moveTo(snapPoint(convertCanvasToLevelCoord(mMousePos)));
   newObject->addToEditor(getGame());    
   newObject->onGeomChanged();

   validateLevel();
   setNeedToSave(true);
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
   const Vector<EditorObject *> *objList = getObjectList();

   if(objList->size() || mLevelGenItems.size())
   {
      F32 minx =  F32_MAX,   miny =  F32_MAX;
      F32 maxx = -F32_MAX,   maxy = -F32_MAX;

      for(S32 i = 0; i < objList->size(); i++)
      {
         EditorObject *obj = objList->get(i);

         for(S32 j = 0; j < obj->getVertCount(); j++)
         {
            if(obj->getVert(j).x < minx)
               minx = obj->getVert(j).x;
            if(obj->getVert(j).x > maxx)
               maxx = obj->getVert(j).x;
            if(obj->getVert(j).y < miny)
               miny = obj->getVert(j).y;
            if(obj->getVert(j).y > maxy)
               maxy = obj->getVert(j).y;
         }
      }

      for(S32 i = 0; i < mLevelGenItems.size(); i++)
      {
         EditorObject *obj = mLevelGenItems[i].get();

         for(S32 j = 0; j < obj->getVertCount(); j++)
         {
            if(obj->getVert(j).x < minx)
               minx = obj->getVert(j).x;
            if(obj->getVert(j).x > maxx)
               maxx = obj->getVert(j).x;
            if(obj->getVert(j).y < miny)
               miny = obj->getVert(j).y;
            if(obj->getVert(j).y > maxy)
               maxy = obj->getVert(j).y;
         }
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
         mCurrentScale /= 1.3f;      // Zoom out a bit
         mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2  - mCurrentScale * midx, 
                            gScreenInfo.getGameCanvasHeight() / 2 - mCurrentScale * midy);
      }
   }
   else     // Put (0,0) at center of screen
   {
      mCurrentScale = STARTING_SCALE;
      mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2, gScreenInfo.getGameCanvasHeight() / 2);
   }
}


// Gets run when user exits special-item editing mode, called from attribute editors
void EditorUserInterface::doneEditingAttributes(EditorAttributeMenuUI *editor, EditorObject *object)
{
   object->onAttrsChanged();

   const Vector<EditorObject *> *objList = getObjectList();

   // Find any other selected items of the same type of the item we just edited, and update their values too
   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj = objList->get(i);

      if(obj != object && obj->isSelected() && obj->getObjectTypeNumber() == object->getObjectTypeNumber())
      {
         editor->doneEditing(obj);  // Transfer attributes from editor to object
         obj->onAttrsChanged();     // And notify the object that its attributes have changed
      }
   }
}


// Handle key presses
void EditorUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(OGLCONSOLE_ProcessBitfighterKeyEvent(keyCode, ascii))      // Pass the key on to the console for processing
      return;

   // TODO: Make this stuff work like the attribute entry stuff; use a real menu and not this ad-hoc code
   // This is where we handle entering things like rotation angle and other data that requires a special entry box.
   // NOT for editing an item's attributes.  Still used, but untested in refactor.
   if(entryMode != EntryNone)
      textEntryKeyHandler(keyCode, ascii);

   else if(keyCode == KEY_ENTER)       // Enter - Edit props
      startAttributeEditor();

   // Regular key handling from here on down
   else if(getKeyState(KEY_SHIFT) && keyCode == KEY_0)  // Shift-0 -> Set team to hostile
      setCurrentTeam(-2);

   else if(ascii == '#' || ascii == '!')
   {
      S32 selected = NONE;

      const Vector<EditorObject *> *objList = getObjectList();

      // Find first selected item, and just work with that.  Unselect the rest.
      for(S32 i = 0; i < objList->size(); i++)
      {
         if(objList->get(i)->isSelected())
         {
            if(selected == NONE)
            {
               selected = i;
               continue;
            }
            else
               objList->get(i)->setSelected(false);
         }
      }

      if(selected == NONE)      // Nothing selected, nothing to do!
         return;

      mEntryBox = getNewEntryBox(objList->get(selected)->getItemId() <= 0 ? "" : itos(objList->get(selected)->getItemId()), 
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
      if(getKeyState(MOUSE_LEFT) && !getKeyState(KEY_CTRL))        // Prevent weirdness
         return;  

      mMousePos.set(gScreenInfo.getMousePos());

      if(mCreatingPoly || mCreatingPolyline)
      {
         if(mNewItem->getVertCount() < gMaxPolygonPoints)          // Limit number of points in a polygon/polyline
         {
            mNewItem->addVert(snapPoint(convertCanvasToLevelCoord(mMousePos)));
            mNewItem->onGeomChanging();
         }
         
         return;
      }

      saveUndoState();     // Save undo state before we clear the selection
      clearSelection();    // Unselect anything currently selected

      // Can only add new vertices by clicking on item's edge, not it's interior (for polygons, that is)
      if(mEdgeHit != NONE && mItemHit && (mItemHit->getGeomType() == geomPolyLine || mItemHit->getGeomType() >= geomPolygon))
      {
         if(mItemHit->getVertCount() >= gMaxPolygonPoints)     // Polygon full -- can't add more
            return;

         Point newVertex = snapPoint(convertCanvasToLevelCoord(mMousePos));      // adding vertex w/ right-mouse

         mAddingVertex = true;

         // Insert an extra vertex at the mouse clicked point, and then select it.
         mItemHit->insertVert(newVertex, mEdgeHit + 1);
         mItemHit->selectVert(mEdgeHit + 1);

         // Alert the item that its geometry is changing
         mItemHit->onGeomChanging();

         mMouseDownPos = newVertex;
         
      }
      else     // Start creating a new poly or new polyline (tilda key + right-click ==> start polyline)
      {
         S32 width;

         if(getKeyState(KEY_TILDE))
         {
            mCreatingPolyline = true;
            mNewItem = new LineItem();
            width = 2;
         }
         else
         {
            mCreatingPoly = true;
            width = Barrier::DEFAULT_BARRIER_WIDTH;
            mNewItem = new WallItem();
         }

         mNewItem->initializeEditor();
         mNewItem->setTeam(mCurrentTeam);
         mNewItem->addVert(snapPoint(convertCanvasToLevelCoord(mMousePos)));
         mNewItem->setDockItem(false);
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

         if(mNewItem->getVertCount() < 2)
            delete mNewItem;
         else
         {
            mNewItem->addToEditor(getGame());
            mNewItem->onGeomChanged();          // Walls need to be added to editor BEFORE onGeomChanged() is run!
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

        /* S32 vertexHit;
         EditorObject *vertexHitPoly;
*/
         //findHitVertex(mMousePos, vertexHitPoly, vertexHit);
         //findHitItemAndEdge();      //  Sets mItemHit, mVertexHit, and mEdgeHit


         if(!getKeyState(KEY_SHIFT))      // Shift key is not down
         {
            // If we hit a vertex of an already selected item --> now we can move that vertex w/o losing our selection.
            // Note that in the case of a point item, we want to skip this step, as we don't select individual vertices.
            if(mVertexHit != NONE && mItemHit->isSelected() && mItemHit->getGeomType() != geomPoint)    
            {
               clearSelection();
               mItemHit->selectVert(mVertexHit);
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
            else if(mVertexHit != NONE && (!mItemHit || !mItemHit->isSelected()))      // Hit a vertex of an unselected item
            {        // (braces required)
               if(!mItemHit->vertSelected(mVertexHit))
               {
                  clearSelection();
                  mItemHit->selectVert(mVertexHit);
               }
            }
            else if(mItemHit)                                                          // Hit a non-point item, but not a vertex
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
            if(mVertexHit != NONE)
            {
               if(mItemHit->vertSelected(mVertexHit))
                  mItemHit->unselectVert(mVertexHit);
               else
                  mItemHit->aselectVert(mVertexHit);
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
      setSaveMessage("Reloaded " + getLevelFileName(), true);
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
         rotateSelection(getKeyState(KEY_SHIFT) ? 15.f : -15.f); // Shift-R - Rotate CW, R - Rotate CCW
   else if((keyCode == KEY_I) && getKeyState(KEY_CTRL))  // Ctrl-I - Insert items generated with script into editor
   {
      copyScriptItemsToEditor();
   }

   else if(((keyCode == KEY_UP) && !getKeyState(KEY_CTRL)) || keyCode == KEY_W)  // W or Up - Pan up
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
         saveLevel(true, true);
      else                                // S - Pan down
         mDown = true;
   }
   else if(keyCode == KEY_A && getKeyState(KEY_CTRL))            // Ctrl-A - toggle see all objects
   {
      mShowMode = (ShowMode) ((U32)mShowMode + 1);

      if(mShowMode == ShowModesCount)
         mShowMode = (ShowMode) 0;     // First mode

      if(mShowMode == ShowWallsOnly && !mDraggingObjects)
         SDL_ShowCursor(SDL_ENABLE);  // TODO:  was GLUT_CURSOR_RIGHT_ARROW

      populateDock();   // Different modes have different items

      onMouseMoved();   // Reset mouse to spray if appropriate
   }
   else if(keyCode == KEY_LEFT || keyCode == KEY_A)   // Left or A - Pan left
      mLeft = true;
   else if(keyCode == KEY_EQUALS)         // Plus (+) - Increase barrier width
   {
      if(getKeyState(KEY_SHIFT))          // SHIFT --> by 1
         changeBarrierWidth(1);
      else                                // unshifted --> by 5
         changeBarrierWidth(5);
   }
   else if(keyCode == KEY_MINUS)          // Minus (-)  - Decrease barrier width
   {
      if(getKeyState(KEY_SHIFT))          // SHIFT --> by 1
         changeBarrierWidth(-1);
      else                                // unshifted --> by 5
         changeBarrierWidth(-5);
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
      playBoop();
      getUIManager()->getGameParamUserInterface()->activate();
   }
   else if(keyCode == KEY_F2)             // F2 - Team Editor Menu
   {
      getUIManager()->getTeamDefUserInterface()->activate();
      playBoop();
   }
   else if(keyCode == KEY_T)              // T - Teleporter
      insertNewItem(TeleportTypeNumber);
   else if(keyCode == KEY_P)              // P - Speed Zone
      insertNewItem(SpeedZoneTypeNumber);
   else if(keyCode == KEY_G)              // G - Spawn
      insertNewItem(ShipSpawnTypeNumber);
   else if(keyCode == KEY_B && getKeyState(KEY_CTRL)) // Ctrl-B - Spy Bug
      insertNewItem(SpyBugTypeNumber);
   else if(keyCode == KEY_B)              // B - Repair
      insertNewItem(RepairItemTypeNumber);
   else if(keyCode == KEY_Y)              // Y - Turret
      insertNewItem(TurretTypeNumber);
   else if(keyCode == KEY_M)              // M - Mine
      insertNewItem(MineTypeNumber);
   else if(keyCode == KEY_F)              // F - Force Field
      insertNewItem(ForceFieldProjectorTypeNumber);
   else if(keyCode == KEY_BACKSPACE || keyCode == KEY_DELETE)
         deleteSelection(false);
   else if(keyCode == keyHELP)            // Turn on help screen
   {
      getGame()->getUIManager()->getEditorInstructionsUserInterface()->activate();
      playBoop();
   }
   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
      getGame()->getUIManager()->getChatUserInterface()->activate();
   else if(keyCode == keyDIAG)            // Turn on diagnostic overlay
      getGame()->getUIManager()->getDiagnosticUserInterface()->activate();
   else if(keyCode == KEY_ESCAPE)           // Activate the menu
   {
      playBoop();
      getGame()->getUIManager()->getEditorMenuUserInterface()->activate();
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
         const Vector<EditorObject *> *objList = getObjectList();

         for(S32 i = 0; i < objList->size(); i++)
         {
            EditorObject *obj = objList->get(i);

            if(obj->isSelected())             // Should only be one
            {
               U32 id = atoi(mEntryBox.c_str());
               if(obj->getItemId() != (S32)id)     // Did the id actually change?
               {
                  obj->setItemId(id);
                  mAllUndoneUndoLevel = -1;        // If so, it can't be undone
               }
               break;
            }
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


void EditorUserInterface::startAttributeEditor()
{
   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
   {
      EditorObject *obj_i = objList->get(i);
      if(obj_i->isSelected())
      {
         // Force item i to be the one and only selected item type.  This will clear up some problems that
         // might otherwise occur.  If you have multiple items selected, all will end up with the same values.
         for(S32 j = 0; j < objList->size(); j++)
         {
            EditorObject *obj_j = objList->get(j);

            if(obj_j->isSelected() && obj_j->getObjectTypeNumber() != obj_i->getObjectTypeNumber())
               obj_j->unselect();
         }


         // Activate the attribute editor if there is one
         EditorAttributeMenuUI *menu = obj_i->getAttributeMenu();
         if(menu)
         {
            obj_i->setIsBeingEdited(true);
            menu->startEditing(obj_i);
            menu->activate();

            saveUndoState();
         }

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
         // fall-through OK  ...why?
      case KEY_W:
         mUp = false;
         break;
      case KEY_DOWN:
         mOut = false;
         // fall-through OK  ...why?
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

            fillVector.clear();

            if(mShowMode == ShowWallsOnly)
               getGame()->getEditorDatabase()->findObjects((TestFunc)isWallType, fillVector);
            else
               getGame()->getEditorDatabase()->findObjects(fillVector);


            for(S32 i = 0; i < fillVector.size(); i++)
            {
               EditorObject *obj = dynamic_cast<EditorObject *>(fillVector[i]);

               // Make sure that all vertices of an item are inside the selection box; basically means that the entire 
               // item needs to be surrounded to be included in the selection
               for(j = 0; j < obj->getVertCount(); j++)
                  if(!r.contains(obj->getVert(j)))
                     break;
               if(j == obj->getVertCount())
                  obj->setSelected(true);
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

            onFinishedDragging();
         }

         break;
   }     // case
}


// Called when user has been dragging an object and then releases it
void EditorUserInterface::onFinishedDragging()
{
   mDraggingObjects = false;

   if(mouseOnDock())                      // Mouse is over the dock -- either dragging to or from dock
   {
      if(mDraggingDockItem == NONE)       // This was really a delete (item dragged to dock)
      {
         const Vector<EditorObject *> *objList = getObjectList();

         for(S32 i = 0; i < objList->size(); i++)    //  Delete all selected items
            if(objList->get(i)->isSelected())
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
         bool itemsMoved = mSnapObject->getVert(mSnapVertexIndex) != mMoveOrigin;

         if(itemsMoved)    // Move consumated... update any moved items, and save our autosave
         {
            const Vector<EditorObject *> *objList = getObjectList();

            for(S32 i = 0; i < objList->size(); i++)
               if(objList->get(i)->isSelected() || objList->get(i)->anyVertsSelected())
                  objList->get(i)->onGeomChanged();

            setNeedToSave(true);
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
   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
      if(objList->get(i)->isSelected())
         return true;

   return false;
}


bool EditorUserInterface::anythingSelected()
{
   const Vector<EditorObject *> *objList = getObjectList();

   for(S32 i = 0; i < objList->size(); i++)
      if(objList->get(i)->isSelected() || objList->get(i)->anyVertsSelected() )
         return true;

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
      mCurrentScale *= 1 + timeDelta * 0.002f;
   else if(mOut && !mIn)
      mCurrentScale *= 1 - timeDelta * 0.002f;

   if(mCurrentScale < MIN_SCALE)
     mCurrentScale = MIN_SCALE;
   else if(mCurrentScale > MAX_SCALE)
      mCurrentScale = MAX_SCALE;

   Point newMousePoint = convertLevelToCanvasCoord(mouseLevelPoint);
   mCurrentOffset += mMousePos - newMousePoint;

   mSaveMsgTimer.update(timeDelta);
   mWarnMsgTimer.update(timeDelta);

   LineEditor::updateCursorBlink(timeDelta);
}


void EditorUserInterface::setSaveMessage(string msg, bool savedOK)
{
   mSaveMsg = msg;
   mSaveMsgTimer = saveMsgDisplayTime;
   mSaveMsgColor = (savedOK ? Colors::green : Colors::red);
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
         ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

         ui->reset();
         ui->setTitle("INVALID FILE NAME");
         ui->setMessage(1, "The level file name is invalid or empty.  The level cannot be saved.");
         ui->setMessage(2, "To correct the problem, please change the file name using the");
         ui->setMessage(3, "Game Parameters menu, which you can access by pressing [F3].");

         ui->activate();

         return false;
      }

      ConfigDirectories *folderManager = getGame()->getSettings()->getConfigDirs();

      string fileName = joindir(folderManager->levelDir, saveName).c_str();

      FILE *f = fopen(fileName.c_str(), "w");
      if(!f)
         throw(SaveException("Could not open file for writing"));

      // Write out basic game parameters, including gameType info
      s_fprintf(f, "%s", getGame()->toString().c_str());    // Note that this toString appends a newline char; most don't


      // Write out all level items (do two passes; walls first, non-walls next, so turrets & forcefields have something to grab onto)
      const Vector<EditorObject *> *objList = getObjectList();

      for(S32 j = 0; j < 2; j++)
         for(S32 i = 0; i < objList->size(); i++)
         {
            EditorObject *p = objList->get(i);

            // Writing wall items on first pass, non-wall items next -- that will make sure mountable items have something to grab onto
            if((j == 0 && isWallType(p->getObjectTypeNumber())) ||
               (j == 1 && ! isWallType(p->getObjectTypeNumber())) )
               p->saveItem(f, getGame()->getGridSize());
         }
      fclose(f);
   }
   catch (SaveException &e)
   {
      if(showFailMessages)
         setSaveMessage("Error Saving: " + string(e.what()), false);
      return false;
   }

   if(!autosave)     // Doesn't count as a save!
   {
      mNeedToSave = false;
      mAllUndoneUndoLevel = mLastUndoIndex;     // If we undo to this point, we won't need to save
   }

   if(showSuccessMessages)
      setSaveMessage("Saved " + getLevelFileName(), true);

   return true;
}


// We need some local hook into the testLevelStart() below.  Ugly but apparently necessary.
void testLevelStart_local(ClientGame *game)
{
   game->getUIManager()->getEditorUserInterface()->testLevelStart();
}


extern void initHostGame(GameSettings *settings, Vector<string> &levelList, bool testMode, bool dedicatedServer);

void EditorUserInterface::testLevel()
{
   bool gameTypeError = false;
   if(!getGame()->getGameType())     // Not sure this could really happen anymore...  TODO: Make sure we always have a valid gametype
      gameTypeError = true;

   // With all the map loading error fixes, game should never crash!
   validateLevel();
   if(mLevelErrorMsgs.size() || mLevelWarnings.size() || gameTypeError)
   {
      YesNoUserInterface *ui = getUIManager()->getYesNoUserInterface();

      ui->reset();
      ui->setTitle("LEVEL HAS PROBLEMS");

      S32 line = 1;
      for(S32 i = 0; i < mLevelErrorMsgs.size(); i++)
         ui->setMessage(line++, mLevelErrorMsgs[i].c_str());

      for(S32 i = 0; i < mLevelWarnings.size(); i++)
         ui->setMessage(line++, mLevelWarnings[i].c_str());

      if(gameTypeError)
      {
         ui->setMessage(line++, "ERROR: GameType is invalid.");
         ui->setMessage(line++, "(Fix in Level Parameters screen [F3])");
      }

      ui->setInstr("Press [Y] to start, [ESC] to cancel");
      ui->registerYesFunction(testLevelStart_local);   // testLevelStart_local() just calls testLevelStart() below
      ui->activate();

      return;
   }

   testLevelStart();
}


void EditorUserInterface::testLevelStart()
{
   string tmpFileName = mEditFileName;
   mEditFileName = "editor.tmp";

   SDL_ShowCursor(SDL_DISABLE);    // Turn off cursor
   bool nts = mNeedToSave;             // Save these parameters because they are normally reset when a level is saved.
   S32 auul = mAllUndoneUndoLevel;     // Since we're only saving a temp copy, we really shouldn't reset them...

   mEditorGameType = getGame()->getGameType();     // Sock our current gametype away, will use it when we reenter the editor

   if(saveLevel(true, false))
   {
      mEditFileName = tmpFileName;     // Restore the level name

      mWasTesting = true;

      Vector<string> levelList;
      levelList.push_back("editor.tmp");
      initHostGame(getGame()->getSettings(), levelList, true, false);
   }

   mNeedToSave = nts;                  // Restore saved parameters
   mAllUndoneUndoLevel = auul;
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
EditorMenuUserInterface::EditorMenuUserInterface(ClientGame *game) : Parent(game)
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
extern MenuItem *getWindowModeMenuItem(ClientGame *game);

//////////
// Editor menu callbacks
//////////

void reactivatePrevUICallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->reactivatePrevUI();
}


static void testLevelCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getEditorUserInterface()->testLevel();
}


void returnToEditorCallback(ClientGame *game, U32 unused)
{
   EditorUserInterface *ui = game->getUIManager()->getEditorUserInterface();

   ui->saveLevel(true, true);                                     // Save level
   ui->setSaveMessage("Saved " + ui->getLevelFileName(), true);   // Setup a message for the user
   game->getUIManager()->reactivatePrevUI();                      // Return to editor
}


static void activateHelpCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getEditorInstructionsUserInterface()->activate();
}


static void activateLevelParamsCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getGameParamUserInterface()->activate();
}


static void activateTeamDefCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getTeamDefUserInterface()->activate();
}


void quitEditorCallback(ClientGame *game, U32 unused)
{
   EditorUserInterface *editorUI = game->getUIManager()->getEditorUserInterface();

   if(editorUI->getNeedToSave())
   {
      YesNoUserInterface *ui = game->getUIManager()->getYesNoUserInterface();

      ui->reset();
      ui->setInstr("Press [Y] to save, [N] to quit [ESC] to cancel");
      ui->setTitle("SAVE YOUR EDITS?");
      ui->setMessage(1, "You have not saved your edits to the level.");
      ui->setMessage(3, "Do you want to?");
      ui->registerYesFunction(saveLevelCallback);
      ui->registerNoFunction(backToMainMenuCallback);
      ui->activate();
   }
   else
     backToMainMenuCallback(game);

   editorUI->clearUndoHistory();        // Clear up a little memory
}

//////////

void EditorMenuUserInterface::setupMenus()
{
   menuItems.clear();
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "RETURN TO EDITOR", reactivatePrevUICallback,    "", KEY_R)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(getWindowModeMenuItem(getGame())));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "TEST LEVEL",       testLevelCallback,           "", KEY_T)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "SAVE LEVEL",       returnToEditorCallback,      "", KEY_S)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "INSTRUCTIONS",     activateHelpCallback,        "", KEY_I, keyHELP)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "LEVEL PARAMETERS", activateLevelParamsCallback, "", KEY_L, KEY_F3)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "MANAGE TEAMS",     activateTeamDefCallback,     "", KEY_M, KEY_F2)));
   menuItems.push_back(boost::shared_ptr<MenuItem>(new MenuItem(getGame(), 0, "QUIT",             quitEditorCallback,          "", KEY_Q, KEY_UNKNOWN)));
}


void EditorMenuUserInterface::onEscape()
{
   SDL_ShowCursor(SDL_DISABLE);
   getUIManager()->reactivatePrevUI();
}


};

