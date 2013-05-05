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

#include "UIEditorMenus.h"       // For access to menu methods such as setObject
#include "UIYesNo.h"

#include "gridDB.h"
#include "WallSegmentManager.h"

#include "ClientGame.h"  
#include "CoreGame.h"            // For CoreItem def
#include "NexusGame.h"           // For NexusZone def
#include "soccerGame.h"          // For Soccer ball radius

#include "barrier.h"             // For DEFAULT_BARRIER_WIDTH
#include "EngineeredItem.h"      // For Turret properties
#include "goalZone.h"
#include "item.h"                // For Asteroid defs
#include "loadoutZone.h"         // For LoadoutZone def
#include "PickupItem.h"          // For RepairItem
#include "speedZone.h"           // For Speedzone def
#include "teleporter.h"          // For Teleporter def
#include "textItem.h"            // For MAX_TEXTITEM_LEN and MAX_TEXT_SIZE

#include "config.h"
#include "Cursor.h"              // For various editor cursor
#include "Colors.h"

#include "gameLoader.h"          // For LevelLoadException def

#include "luaLevelGenerator.h"
#include "LevelDatabaseUploadThread.h"
#include "gameObjectRender.h"

#include "Console.h"          // Our console object
#include "ScreenInfo.h"
#include "VideoSystem.h"

#include "stringUtils.h"
#include "GeomUtils.h"
#include "RenderUtils.h"
#include "OpenglUtils.h"


using namespace boost;

namespace Zap
{

const S32 DOCK_WIDTH = 50;          // Width of dock, in pixels
const F32 MIN_SCALE = .05f;         // Most zoomed-in scale
const F32 MAX_SCALE = 2.5;          // Most zoomed-out scale
const F32 STARTING_SCALE = 0.5;

static GridDatabase *mLoadTarget;


//Vector<string> EditorUserInterface::robots;        // List of robot lines in the level file

enum EntryMode {
   EntryID,          // Entering an objectID
   EntryAngle,       // Entering an angle
   EntryScale,       // Entering a scale
   EntryNone         // Not in a special entry mode
};


static EntryMode entryMode;

static void saveLevelCallback(ClientGame *game)
{
   UIManager *uiManager = game->getUIManager();

   if(uiManager->getEditorUserInterface()->saveLevel(true, true))
      uiManager->reactivate(MainUI);   
   else
      uiManager->reactivate(EditorUI);
}


void backToMainMenuCallback(ClientGame *game)
{
   game->getUIManager()->getEditorUserInterface()->onQuitted();
   game->getUIManager()->reactivate(MainUI);    
}


// Constructor
EditorUserInterface::EditorUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(EditorUI);

   mWasTesting = false;
   mouseIgnore = false;

   clearSnapEnvironment();

   mHitItem     = NULL;
   mNewItem     = NULL;
   mDockItemHit = NULL;

   mHitVertex = NONE;
   mEdgeHit   = NONE;

   mEditorDatabase = boost::shared_ptr<GridDatabase>(new GridDatabase());

   setNeedToSave(false);

   mLastUndoStateWasBarrierWidthChange = false;

   mUndoItems.resize(UNDO_STATES);     // Create slots for all our undos... also creates a ton of empty dbs.  Maybe we should be using pointers?
   mAutoScrollWithMouse = false;
   mAutoScrollWithMouseReady = false;

   mEditorAttributeMenuItemBuilder.initialize(game);
}


GridDatabase *EditorUserInterface::getDatabase()  
{ 
   return mEditorDatabase.get();
}  


void EditorUserInterface::setDatabase(boost::shared_ptr<GridDatabase> database)
{
   TNLAssert(database.get(), "Database should not be NULL!");
   mEditorDatabase = boost::dynamic_pointer_cast<GridDatabase>(database);
}



// Really quitting... no going back!
void EditorUserInterface::onQuitted()
{
   cleanUp();
   getGame()->clearAddTarget();
}


void EditorUserInterface::addDockObject(BfObject *object, F32 xPos, F32 yPos)
{
   object->prepareForDock(getGame(), Point(xPos, yPos), mCurrentTeam);     // Prepare object   
   mDockItems.push_back(boost::shared_ptr<BfObject>(object));          // Add item to our list of dock objects
}


void EditorUserInterface::populateDock()
{
   mDockItems.clear();

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

   if(getGame()->getGameType()->getGameTypeId() == SoccerGame)
      addDockObject(new SoccerBallItem(), xPos, yPos);
   else if(getGame()->getGameType()->getGameTypeId() == CoreGame)
      addDockObject(new CoreItem(), xPos, yPos);
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

   //addDockObject(new CircleSpawn(), xPos - 10, yPos);
//   addDockObject(new Core(), xPos /*+ 10*/, yPos);
//   yPos += spacer;


   // These two will share a line
   addDockObject(new TestItem(), xPos - 10, yPos);
   addDockObject(new ResourceItem(), xPos + 10, yPos);
   yPos += 25;

      
   addDockObject(new LoadoutZone(), xPos, yPos);
   yPos += 25;

   if(getGame()->getGameType()->getGameTypeId() == NexusGame)
   {
      addDockObject(new NexusZone(), xPos, yPos);
      yPos += 25;
   }
   else
   {
      addDockObject(new GoalZone(), xPos, yPos);
      yPos += 25;
   }

   addDockObject(new PolyWall(), xPos, yPos);
   yPos += spacer;

   addDockObject(new Zone(), xPos, yPos);
   yPos += spacer;

}


// Destructor -- unwind things in an orderly fashion.  Note that mLevelGenDatabase will clear itself as the referenced object is deleted.
EditorUserInterface::~EditorUserInterface()
{
   mDockItems.clear();
   mClipboard.clear();

   delete mNewItem.getPointer();
}


// Removes most recent undo state from stack --> won't actually delete items on stack until we need the slot, or we quit
void EditorUserInterface::deleteUndoState()
{
   mLastUndoIndex--;
   mLastRedoIndex--; 
}


// Save the current state of the editor objects for later undoing
void EditorUserInterface::saveUndoState(bool forceSelectionOfTargetObject)
{
   // Use case: We do 5 actions, save, undo 2, redo 1, then do some new action.  
   // Our "no need to save" undo point is lost forever.
   if(mAllUndoneUndoLevel > mLastRedoIndex)     
      mAllUndoneUndoLevel = NONE;


   // Select item so when we undo, it will be selected, which looks better
   bool unselHitItem = false;
   if(forceSelectionOfTargetObject && mHitItem && !mHitItem->isSelected())
   {
      mHitItem->setSelected(true);
      unselHitItem = true;
   }


   GridDatabase *newDB = new GridDatabase();    // Make a copy

   newDB->copyObjects(getDatabase());

   mUndoItems[mLastUndoIndex % UNDO_STATES] = boost::shared_ptr<GridDatabase>(newDB);  

   mLastUndoIndex++;
   mLastRedoIndex = mLastUndoIndex;

   if(mLastUndoIndex % UNDO_STATES == mFirstUndoIndex % UNDO_STATES)           // Undo buffer now full...
   {
      mFirstUndoIndex++;
      mAllUndoneUndoLevel -= 1;     // If this falls below 0, then we can't undo our way out of needing to save
   }
   
   setNeedToSave(mAllUndoneUndoLevel != mLastUndoIndex);
   mRedoingAnUndo = false;
   mLastUndoStateWasBarrierWidthChange = false;

   if(unselHitItem)
      mHitItem->setSelected(false);
}


// Remove and discard the most recently saved undo state 
void EditorUserInterface::removeUndoState()
{
   mLastUndoIndex--;
   mLastRedoIndex = mLastUndoIndex;

   if(mLastUndoIndex % UNDO_STATES == mFirstUndoIndex % UNDO_STATES)           // Undo buffer now full...
   {
      mFirstUndoIndex++;
      mAllUndoneUndoLevel -= 1;     // If this falls below 0, then we can't undo our way out of needing to save
   }
   
   setNeedToSave(mAllUndoneUndoLevel != mLastUndoIndex);
}


void EditorUserInterface::clearSnapEnvironment()
{
   mSnapObject = NULL;
   mSnapVertexIndex = NONE;
}


void EditorUserInterface::undo(bool addToRedoStack)
{
   if(!undoAvailable())
      return;

   clearSnapEnvironment();

   if(mLastUndoIndex == mLastRedoIndex && !mRedoingAnUndo)
   {
      saveUndoState();
      mLastUndoIndex--;
      mLastRedoIndex--;
      mRedoingAnUndo = true;
   }

   mLastUndoIndex--;

   setDatabase(mUndoItems[mLastUndoIndex % UNDO_STATES]);
   GridDatabase *database = getDatabase();
   mLoadTarget = database;

   rebuildEverything(database);    // Well, rebuild segments from walls at least

   onSelectionChanged();

   mLastUndoStateWasBarrierWidthChange = false;
   validateLevel();
}
   

void EditorUserInterface::redo()
{
   if(mLastRedoIndex != mLastUndoIndex)      // If there's a redo state available...
   {
      clearSnapEnvironment();

      mLastUndoIndex++;

      // Perform a little trick here... if we're redoing, and it's our final step of the redo tree, and there's only one item selected,
      // we'll make sure that same item is selected when we're finished.  That will help keep the focus on the item that's being modified
      // during the redo step, and make the redo feel more natural.
      // Act I:
      S32 selectedItem = NONE;

      const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();
      if(mLastRedoIndex == mLastUndoIndex && getItemSelectedCount() == 1)
         for(S32 i = 0; i < objList->size(); i++)
         {
            BfObject *obj = static_cast<BfObject *>(objList->get(i));

            if(obj->isSelected())
            {
               selectedItem = obj->getSerialNumber();
               break;
            }
         }

      setDatabase(mUndoItems[mLastUndoIndex % UNDO_STATES]);
      GridDatabase *database = mUndoItems[mLastUndoIndex % UNDO_STATES].get();
      mLoadTarget = database;

      // Act II:
      if(selectedItem != NONE)
      {
         const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

         clearSelection(getDatabase());
         for(S32 i = 0; i < objList->size(); i++)
         {
            BfObject *obj = static_cast<BfObject *>(objList->get(i));

            if(obj->getSerialNumber() == selectedItem)
            {
               obj->setSelected(true);
               break;
            }
         }
      }


      TNLAssert(mUndoItems[mLastUndoIndex % UNDO_STATES], "null!");

      rebuildEverything(database);   // Needed?  Yes, for now, but theoretically no, because we should be restoring everything fully reconstituted...
      onSelectionChanged();
      validateLevel();

      onMouseMoved();               // If anything gets undeleted under the mouse, make sure it's highlighted
   }
}


void EditorUserInterface::rebuildEverything(GridDatabase *database)
{
   database->getWallSegmentManager()->recomputeAllWallGeometry(database);
   resnapAllEngineeredItems(database);

   // If we're rebuilding items in our levelgen database, no need to save anything!
   if(database != &mLevelGenDatabase)
   {
      setNeedToSave(mAllUndoneUndoLevel != mLastUndoIndex);
      autoSave();
   }
}


// Resnaps all engineered items in database
void EditorUserInterface::resnapAllEngineeredItems(GridDatabase *database)
{
   fillVector.clear();
   database->findObjects((TestFunc)isEngineeredType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      EngineeredItem *engrObj = dynamic_cast<EngineeredItem *>(fillVector[i]);
      engrObj->mountToWall(engrObj->getPos(), database->getWallSegmentManager());
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


void EditorUserInterface::makeSureThereIsAtLeastOneTeam()
{
   if(getTeamCount() == 0)
   {
      EditorTeam *team = new EditorTeam(gTeamPresets[0]);

      getGame()->addTeam(team);
   }
}


void EditorUserInterface::cleanUp()
{
   ClientGame *game = getGame();

   clearUndoHistory();     // Clear up a little memory
   mDockItems.clear();     // Free a little more -- dock will be rebuilt when editor restarts
   
   mLoadTarget = getDatabase();
   mLoadTarget->removeEverythingFromDatabase();    // Deletes all objects   was clearDatabase(mLoadTarget);

   game->clearTeams();
   
   clearSnapEnvironment();
   
   mAddingVertex = false;
   clearLevelGenItems();
   mGameTypeArgs.clear();

   game->resetLevelInfo();

   if(game->getGameType())
      delete game->getGameType();
}


// Loads a level
void EditorUserInterface::loadLevel()
{
   ClientGame *game = getGame();

   cleanUp();

   FolderManager *folderManager = game->getSettings()->getFolderManager();
   string fileName = joindir(folderManager->levelDir, mEditFileName).c_str();


   // Process level file --> returns true if file found and loaded, false if not (assume it's a new level)
   bool levelLoaded = game->loadLevelFromFile(fileName, mLoadTarget);

   if(!game->getGameType())  // make sure we have GameType
   {
      GameType *gameType = new GameType;
      gameType->addToGame(game, mLoadTarget);   
   }

   makeSureThereIsAtLeastOneTeam(); // Make sure we at least have one team

   if(levelLoaded)   
   {
      // Loaded a level!
      validateTeams();                 // Make sure every item has a valid team
      validateLevel();                 // Check level for errors (like too few spawns)
   }
   else     
   {
      // New level!
      game->getGameType()->setLevelCredits(getGame()->getClientInfo()->getName());  // Set default author
   }

   clearUndoHistory();                 // Clean out undo/redo buffers
   clearSelection(mLoadTarget);        // Nothing starts selected
   setNeedToSave(false);               // Why save when we just loaded?
   mAllUndoneUndoLevel = mLastUndoIndex;
   populateDock();                     // Add game-specific items to the dock

   // Bulk-process new items, walls first
   mLoadTarget->getWallSegmentManager()->recomputeAllWallGeometry(mLoadTarget);
   
   // Snap all engineered items to the closest wall, if one is found
   resnapAllEngineeredItems(mLoadTarget);
}


void EditorUserInterface::clearLevelGenItems()
{
   mLevelGenDatabase.removeEverythingFromDatabase();
}


void EditorUserInterface::copyScriptItemsToEditor()
{
   if(mLevelGenDatabase.getObjectCount() == 0)
      return;     

   // Duplicate EditorObject pointer list to avoid unsynchronized loop removal
   Vector<DatabaseObject *> tempList(*mLevelGenDatabase.findObjects_fast());

   saveUndoState();

   // We can't call addToEditor immediately because it calls addToGame which will trigger
   // an assert since the levelGen items are already added to the game.  We must therefore
   // remove them from the game first
   for(S32 i = 0; i < tempList.size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(tempList[i]);

      obj->removeFromGame(false);     // False ==> do not delete object
      addToEditor(obj);
   }
      
   mLevelGenDatabase.removeEverythingFromDatabase();    // Don't want to delete these objects... we just handed them off to the database!

   rebuildEverything(getDatabase());

   mLastUndoStateWasBarrierWidthChange = false;
}


void EditorUserInterface::addToEditor(BfObject *obj)
{
   obj->addToGame(getGame(), getDatabase());     
   obj->onGeomChanged();                        // Generic way to get PolyWalls to build themselves after being dragged from the dock
}


// User has pressed Ctrl+R -- run the levelgen script and insert any resulting items into the editor in a separate database
void EditorUserInterface::runLevelGenScript()
{
   GameType *gameType = getGame()->getGameType();
   string scriptName = gameType->getScriptName();

   if(scriptName == "")      // No script included!!
      return;

   gConsole.output("Running script %s\n", gameType->getScriptLine().c_str());

   const Vector<string> *scriptArgs = gameType->getScriptArgs();

   clearLevelGenItems();      // Clear out any items from the last run

   FolderManager *folderManager = getGame()->getSettings()->getFolderManager();
   runScript(&mLevelGenDatabase, folderManager, scriptName, *scriptArgs);
}


// Runs an arbitrary lua script.  Command is first item in cmdAndArgs, subsequent items are the args, if any
void EditorUserInterface::runScript(GridDatabase *database, const FolderManager *folderManager, const string &scriptName, const Vector<string> &args)
{
   string name = folderManager->findLevelGenScript(scriptName);  // Find full name of levelgen script

   if(name == "")
   {
      gConsole.output("Could not find script %s; looked in folders: %s\n", 
                      scriptName.c_str(), concatenate(folderManager->getScriptFolderList()).c_str());
      return;
   }
   
   // Load the items
   LuaLevelGenerator levelGen(name, args, getGame()->getGridSize(), database, getGame());

   // Error reporting handled within -- we won't cache these scripts for easier development   
   bool error = !levelGen.runScript(false);      

   if(error)
   {
      ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

      ui->reset();
      ui->setTitle("SCRIPT ERROR");
      ui->setMessage(2, "The levelgen script you ran threw an error.");
      ui->setMessage(4, "See the console (press [/]) or the logfile for details.");
      getUIManager()->activate(ui);
   }

   // Even if we had an error, continue on so we can process what does work -- this will make it more consistent with how the script will 
   // perform in-game.  Though perhaps we should show an error to the user...


   // Process new items that need it (walls need processing so that they can render properly).
   // Items that need no extra processing will be kept as-is.
   fillVector.clear();
   database->findObjects((TestFunc)isWallType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      BfObject *obj = dynamic_cast<BfObject *>(fillVector[i]);

      if(obj->getVertCount() < 2)      // Invalid item; delete  --> aren't 1 point walls already excluded, making this check redundant?
         database->removeFromDatabase(obj, true);

      if(obj->getObjectTypeNumber() != PolyWallTypeNumber)
         dynamic_cast<WallItem *>(obj)->processEndPoints();
   }

   // Also find any teleporters and make sure their destinations are in order.  Teleporters with no dests will be deleted.
   // Those with multiple dests will be broken down into multiple single dest versions.
   fillVector.clear();
   database->findObjects(TeleporterTypeNumber, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      Teleporter *teleporter = static_cast<Teleporter *>(fillVector[i]);
      if(teleporter->getDestCount() == 0)
         database->removeFromDatabase(teleporter, true);
      else
      {
         for(S32 i = 1; i < teleporter->getDestCount(); i++)
         {
            Teleporter *newTel = new Teleporter;
            newTel->setPos(teleporter->getPos());
            newTel->setEndpoint(teleporter->getDest(i));
            newTel->addDest(teleporter->getDest(i));

            newTel->addToGame(getGame(), database);     
         }

         // Delete any destinations past the first one
         for(S32 i = 1; i < teleporter->getDestCount(); i++)
            teleporter->delDest(i);
      }
   }

   rebuildEverything(database);
}


static void showPluginError(const ClientGame *game, const char *msg)
{
   Vector<StringTableEntry> messages;
   messages.push_back("");
   messages.push_back(string("This plugin encountered an error ") + msg + ".");
   messages.push_back("It has probably been misconfigured.");
   messages.push_back("");
   messages.push_back("See the Bitfighter logfile or console for details.");

   game->displayMessageBox("Problem With Plugin", "Press any key to return to the editor", messages);
}


// Try to create some sort of uniqeish signature for the plugin
string EditorUserInterface::getPluginSignature()
{
   string key = mPluginRunner->getScriptName();

   if(mPluginMenu)
      for(S32 i = 0; i < mPluginMenu->getMenuItemCount(); i++)
      {
         MenuItem *menuItem = mPluginMenu->getMenuItem(i);
         key += itos(menuItem->getItemType()) + "-";
      }

   return key;
}


void EditorUserInterface::runPlugin(const FolderManager *folderManager, const string &scriptName, const Vector<string> &args)
{
   string fullName = folderManager->findPlugin(scriptName);     // Find full name of plugin script

   if(fullName == "")
   {
      showCouldNotFindScriptMessage(scriptName);
      return;
   }


   // Create new plugin, will be deleted by boost
   EditorPlugin *plugin = new EditorPlugin(fullName, args, getGame()->getGridSize(), mLoadTarget, getGame());

   mPluginRunner = boost::shared_ptr<EditorPlugin>(plugin);

   // Loads the script and runs it to get everything loaded into memory.  Does not run main().
   // We won't cache scripts here because the performance impact should be relatively small, and it will
   // make it easier to develop them.  If circumstances change, we might want to start caching.
   if(!mPluginRunner->prepareEnvironment() || !mPluginRunner->loadScript(false)) 
   {
      showPluginError(getGame(), "during loading");
      mPluginRunner.reset();
      return;
   }

   string title;
   Vector<MenuItem *> menuItems;

   bool error = plugin->runGetArgsMenu(title, menuItems);     // Fills menuItems

   if(error)
   {
      showPluginError(getGame(), "configuring its options menu.");
      mPluginRunner.reset();
      return;
   }


   if(menuItems.size() == 0)
   {
      onPluginMenuClosed(Vector<string>());        // No menu items?  Let's run the script directly!
      mPluginRunner.reset();
      return;     
   }


   // There are menu items!
   // Build a menu from the menuItems returned by the plugin
   mPluginMenu.reset(new PluginMenuUI(getGame(), title));      // Using a smart pointer here, for auto deletion

   for(S32 i = 0; i < menuItems.size(); i++)
      mPluginMenu->addMenuItem(menuItems[i]);

   
   mPluginMenu->addSaveAndQuitMenuItem("Run plugin", "Saves values and runs plugin");

   mPluginMenu->setMenuCenterPoint(Point(gScreenInfo.getGameCanvasWidth() / 2, gScreenInfo.getGameCanvasHeight() / 2));  

   // Restore previous values, if available
   string key = getPluginSignature();

   if(mPluginMenuValues.count(key) == 1)    // i.e. the key exists; use count to avoid creating new entry if it does not exist
      for(S32 i = 0; i < mPluginMenuValues[key].size(); i++)
         mPluginMenu->getMenuItem(i)->setValue(mPluginMenuValues[key].get(i));

   getGame()->getUIManager()->activate(mPluginMenu.get());
}


void EditorUserInterface::onPluginMenuClosed(const Vector<string> &args)
{
   TNLAssert(mPluginRunner, "NULL PluginRunner!");
   
   saveUndoState();

   // Save menu values for next time -- using a key that includes both the script name and the type of menu items
   // provides some protection against the script being changed while Bitfighter is running.  Probably not realy
   // necessary, but we can afford it here.
   string key = getPluginSignature();

   mPluginMenuValues[key] = args;

   if(!mPluginRunner->runMain(args))
      setSaveMessage("Plugin Error: press [/] for details", false);

   rebuildEverything(getDatabase());

   mPluginRunner.reset();
}


void EditorUserInterface::showCouldNotFindScriptMessage(const string &scriptName)
{
   string pluginDir = getGame()->getSettings()->getFolderManager()->pluginDir;

   Vector<StringTableEntry> messages;
   messages.push_back("");
   messages.push_back("Could not find the plugin called " + scriptName);
   messages.push_back("I looked in the " + pluginDir + " folder.");
   messages.push_back("");
   messages.push_back("You likely have a typo in the [EditorPlugins]");
   messages.push_back("section of your INI file.");

   getGame()->displayMessageBox("Plugin not Found", "Press any key to return to the editor", messages);
}


static bool TeamListToString(string &output, Vector<bool> teamVector)
{
   string teamList;
   bool hasError = false;
   char buf[16];

   // Make sure each team has a spawn point
   for(S32 i = 0; i < (S32)teamVector.size(); i++)
      if(!teamVector[i])
      {
         dSprintf(buf, sizeof(buf), "%d", i+1);

         if(!hasError)     // This is our first error
         {
            output = "team ";
            teamList = buf;
         }
         else
         {
            output = "teams ";
            teamList += ", ";
            teamList += buf;
         }
         hasError = true;
      }
   if(hasError)
   {
      output += teamList;
      return true;
   }
   return false;
}


static bool hasTeamFlags(GridDatabase *database)
{
   const Vector<DatabaseObject *> *flags = database->findObjects_fast(FlagTypeNumber);

   for(S32 i = 0; i < flags->size(); i++)
      if(static_cast<FlagItem *>(flags->get(i))->getTeam() > TEAM_NEUTRAL)
         return false;

   return false;     
}


static bool hasTeamSpawns(GridDatabase *database)
{
   fillVector.clear();
   database->findObjects(FlagSpawnTypeNumber, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
      if(dynamic_cast<FlagSpawn *>(fillVector[i])->getTeam() >= 0)
         return true;

   return false;
}


void EditorUserInterface::validateLevel()
{
   mLevelErrorMsgs.clear();
   mLevelWarnings.clear();

   bool foundNeutralSpawn = false;

   Vector<bool> foundSpawn;

   string teamList;

   // First, catalog items in level
   S32 teamCount = getTeamCount();
   foundSpawn.resize(teamCount);

   for(S32 i = 0; i < teamCount; i++)      // Initialize vector
      foundSpawn[i] = false;

   GridDatabase *gridDatabase = getDatabase();
      
   fillVector.clear();
   gridDatabase->findObjects(ShipSpawnTypeNumber, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      Spawn *spawn = static_cast<Spawn *>(fillVector[i]);
      const S32 team = spawn->getTeam();

      if(team == TEAM_NEUTRAL)
         foundNeutralSpawn = true;
      else if(team > TEAM_NEUTRAL && team < teamCount)
         foundSpawn[team] = true;
   }

   bool foundSoccerBall = gridDatabase->hasObjectOfType(SoccerBallItemTypeNumber);
   bool foundNexus      = gridDatabase->hasObjectOfType(NexusTypeNumber);
   bool foundFlags      = gridDatabase->hasObjectOfType(FlagTypeNumber);

   bool foundTeamFlags      = hasTeamFlags (gridDatabase);
   bool foundTeamFlagSpawns = hasTeamSpawns(gridDatabase);

   // "Unversal errors" -- levelgens can't (yet) change gametype

   GameType *gameType = getGame()->getGameType();

   // Check for soccer ball in a a game other than SoccerGameType. Doesn't crash no more.
   if(foundSoccerBall && gameType->getGameTypeId() != SoccerGame)
      mLevelWarnings.push_back("WARNING: Soccer ball can only be used in soccer game.");

   // Check for the nexus object in a non-hunter game. Does not affect gameplay in non-hunter game.
   if(foundNexus && gameType->getGameTypeId() != NexusGame)
      mLevelWarnings.push_back("WARNING: Nexus object can only be used in Nexus game.");

   // Check for missing nexus object in a hunter game.  This cause mucho dolor!
   if(!foundNexus && gameType->getGameTypeId() == NexusGame)
      mLevelErrorMsgs.push_back("ERROR: Nexus game must have a Nexus.");

   if(foundFlags && !gameType->isFlagGame())
      mLevelWarnings.push_back("WARNING: This game type does not use flags.");

   // Check for team flag spawns on games with no team flags
   if(foundTeamFlagSpawns && !foundTeamFlags)
      mLevelWarnings.push_back("WARNING: Found team flag spawns but no team flags.");

   // Errors that may be corrected by levelgen -- script could add spawns
   // Neutral spawns work for all; if there's one, then that will satisfy our need for spawns for all teams
   if(getGame()->getGameType()->getScriptName() == "" && !foundNeutralSpawn)
   {
      if(TeamListToString(teamList, foundSpawn))     // Compose error message
         mLevelErrorMsgs.push_back("ERROR: Need spawn point for " + teamList);
   }


   if(gameType->getGameTypeId() == CoreGame)
   {
      for(S32 i = 0; i < teamCount; i++)      // Initialize vector
         foundSpawn[i] = false;

      fillVector.clear();
      gridDatabase->findObjects(CoreTypeNumber, fillVector);
      for(S32 i = 0; i < fillVector.size(); i++)
      {
         CoreItem *core = static_cast<CoreItem *>(fillVector[i]);
         const S32 team = core->getTeam();
         if(U32(team)< U32(foundSpawn.size()))
            foundSpawn[team] = true;
      }
      if(TeamListToString(teamList, foundSpawn))     // Compose error message
         mLevelErrorMsgs.push_back("ERROR: Need Core for " + teamList);
   }
}


void EditorUserInterface::validateTeams()
{
   validateTeams(getDatabase()->findObjects_fast());
}


// Check that each item has a valid team  (fixes any problems it finds)
void EditorUserInterface::validateTeams(const Vector<DatabaseObject *> *dbObjects)
{
   S32 teams = getTeamCount();

   for(S32 i = 0; i < dbObjects->size(); i++)
   {
      BfObject *obj = dynamic_cast<BfObject *>(dbObjects->get(i));
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

   if(getTeamCount() != mOldTeams.size())     // Number of teams has changed
      teamsChanged = true;
   else
      for(S32 i = 0; i < getTeamCount(); i++)
      {
         EditorTeam *team = getTeam(i);

         if(mOldTeams[i].color != *team->getColor() || mOldTeams[i].name != team->getName().getString()) // Color(s) or names(s) have changed
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

   validateTeams(&hackyjunk);

   validateLevel();          // Revalidate level -- if teams have changed, requirements for spawns have too
   setNeedToSave(true);
   autoSave();
   mAllUndoneUndoLevel = -1; // This change can't be undone
}


string EditorUserInterface::getLevelFileName()
{
   return mEditFileName;
}


void EditorUserInterface::onSelectionChanged()
{
   GridDatabase *database = getDatabase();
   WallSegmentManager *wallSegmentManager = database->getWallSegmentManager();

   wallSegmentManager->clearSelected();
   
   // Update wall segment manager with what's currently selected
   fillVector.clear();
   database->findObjects((TestFunc)isWallType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      TNLAssert(dynamic_cast<BfObject *>(fillVector[i]), "Bad cast!");
      BfObject *obj = static_cast<BfObject *>(fillVector[i]);

      if(obj->isSelected())
         wallSegmentManager->setSelected(obj->getSerialNumber(), true);
   }

   wallSegmentManager->rebuildSelectedOutline();
}


void EditorUserInterface::onBeforeRunScriptFromConsole()
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   // Use selection as a marker -- will have to change in future
   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));
      obj->setSelected(true);
   }
}


void EditorUserInterface::onAfterRunScriptFromConsole()
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   // Since all our original objects were marked as selected before the script was run, and since the objects generated by
   // the script are not selected, if we invert the selection, our script items will now be selected.
   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));
      obj->setSelected(!obj->isSelected());
   }

   rebuildEverything(getDatabase());
   onSelectionChanged();
}


void EditorUserInterface::onActivate()
{
   GameSettings *settings = getGame()->getSettings();

   FolderManager *folderManager = settings->getFolderManager();

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

      getUIManager()->activate(ui);

      return;
   }

   // Check if we have a level name:
   if(getLevelFileName() == "")         // We need to take a detour to get a level name
   {
      // Don't save this menu (false, below).  That way, if the user escapes out, and is returned to the "previous"
      // UI, they will get back to where they were before (prob. the main menu system), not back to here.
      getUIManager()->activate(LevelNameEntryUI, false);

      return;
   }

   mLevelErrorMsgs.clear();
   mLevelWarnings.clear();

   mSaveMsgTimer.clear();

   mGameTypeArgs.clear();

   getGame()->setActiveTeamManager(&mTeamManager);

   loadLevel();
   setCurrentTeam(0);

   mSnapContext = FULL_SNAPPING;      // Hold [space/shift+space] to temporarily disable snapping

   // Reset display parameters...
   mDragSelecting = false;
   mUp = mDown = mLeft = mRight = mIn = mOut = false;
   mCreatingPoly = false;
   mCreatingPolyline = false;
   mDraggingObjects = false;
   mDraggingDockItem = NULL;
   mCurrentTeam = 0;
   mPreviewMode = false;
   mDragCopying = false;
   mJustInsertedVertex = false;
   entryMode = EntryNone;

   Cursor::enableCursor();

   mSaveMsgTimer.clear();

   getGame()->setAddTarget();    // When a Lua script does an addToGame(), objects should be added to this game

   VideoSystem::actualizeScreenMode(settings, true, usesEditorScreenMode());

   centerView();
}


void EditorUserInterface::renderMasterStatus()
{
   // Do nothing, don't render this in editor 
}


bool EditorUserInterface::usesEditorScreenMode()
{
   return true;
}


void EditorUserInterface::onReactivate()     // Run when user re-enters the editor after testing, among other things
{
   mDraggingObjects = false;  
   Cursor::enableCursor();

   if(mWasTesting)
   {
      mWasTesting = false;
      mSaveMsgTimer.clear();

      getGame()->setGameType(mEditorGameType); 

      remove("editor.tmp");      // Delete temp file
   }


   getGame()->setAddTarget();    // When a Lua script does an addToGame(), objects should be added to this game

   getGame()->setActiveTeamManager(&mTeamManager);

   if(mCurrentTeam >= getTeamCount())
      mCurrentTeam = 0;

   if(UserInterface::getUIManager()->getPrevUI()->usesEditorScreenMode() != usesEditorScreenMode())
      VideoSystem::actualizeScreenMode(getGame()->getSettings(), true, usesEditorScreenMode());

   mDockItemHit = NULL;
}


S32 EditorUserInterface::getTeamCount()
{
   return getGame()->getTeamCount();
}


EditorTeam *EditorUserInterface::getTeam(S32 teamId)
{
   TNLAssert(dynamic_cast<EditorTeam *>(getGame()->getTeam(teamId)), "Expected a EditorTeam");
   return static_cast<EditorTeam *>(getGame()->getTeam(teamId));
}


void EditorUserInterface::clearTeams()
{
   getGame()->clearTeams();
}


bool EditorUserInterface::getNeedToSave() const
{
   return mNeedToSave;
}


void EditorUserInterface::setNeedToSave(bool needToSave)
{
   mNeedToSave = needToSave;
}


void EditorUserInterface::addTeam(EditorTeam *team)
{
   getGame()->addTeam(team);
}


void EditorUserInterface::addTeam(EditorTeam *team, S32 teamIndex)
{
   getGame()->addTeam(team, teamIndex);
}


void EditorUserInterface::removeTeam(S32 teamIndex)
{
   getGame()->removeTeam(teamIndex);
}


Point EditorUserInterface::convertCanvasToLevelCoord(Point p)
{
   return (p - mCurrentOffset) * (1 / mCurrentScale);
}


Point EditorUserInterface::convertLevelToCanvasCoord(Point p, bool convert)
{
   return convert ? p * mCurrentScale + mCurrentOffset : p;
}


// Called when we shift between windowed and fullscreen mode, after change is made
void EditorUserInterface::onDisplayModeChange()
{
   static S32 previousXSize = -1;
   static S32 previousYSize = -1;

   if(previousXSize != gScreenInfo.getGameCanvasWidth() || previousYSize != gScreenInfo.getGameCanvasHeight())
   {
      // Recenter canvas -- note that canvasWidth may change during displayMode change
      mCurrentOffset.set(mCurrentOffset.x - previousXSize / 2 + gScreenInfo.getGameCanvasWidth() / 2, 
                         mCurrentOffset.y - previousYSize / 2 + gScreenInfo.getGameCanvasHeight() / 2);
   }

   // Need to populate the dock here because dock items are tied to a particular screen x,y; 
   // maybe it would be better to give them a dock x,y instead?
   if(getGame()->getGameType())
      populateDock();               // If game type has changed, items on dock will change

   previousXSize = gScreenInfo.getGameCanvasWidth();
   previousYSize = gScreenInfo.getGameCanvasHeight();
}


Point EditorUserInterface::snapPointToLevelGrid(Point const &p)
{
   if(mSnapContext != FULL_SNAPPING)
      return p;

   // First, find a snap point based on our grid
   F32 factor = (showMinorGridLines() ? 0.1f : 0.5f) * getGame()->getGridSize();     // Tenths or halves -- major gridlines are gridsize pixels apart

   return Point(floor(p.x / factor + 0.5) * factor, floor(p.y / factor + 0.5) * factor);
}


Point EditorUserInterface::snapPoint(GridDatabase *database, Point const &p, bool snapWhileOnDock)
{
   if(mouseOnDock() && !snapWhileOnDock) 
      return p;      // No snapping!

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   Point snapPoint(p);

   WallSegmentManager *wallSegmentManager = database->getWallSegmentManager();

   if(mDraggingObjects)
   {
      // Mark all items being dragged as no longer being snapped -- only our primary "focus" item will be snapped
      for(S32 i = 0; i < objList->size(); i++)
      {
         BfObject *obj = static_cast<BfObject *>(objList->get(i));

         if(obj->isSelected())
            obj->setSnapped(false);
      }
   
      // Turrets & forcefields: Snap to a wall edge as first (and only) choice, regardless of whether snapping is on or off
      if(isEngineeredType(mSnapObject->getObjectTypeNumber()))
      {
         EngineeredItem *engrObj = dynamic_cast<EngineeredItem *>(mSnapObject.getPointer());
         return engrObj->mountToWall(snapPointToLevelGrid(p), wallSegmentManager);
      }
   }

   F32 minDist = 255 / mCurrentScale;    // 255 just seems to work well, not related to gridsize; only has an impact when grid is off

   if(mSnapContext == FULL_SNAPPING)     // Only snap to grid when full snapping is enabled; lowest priority snaps go first
   {
      snapPoint = snapPointToLevelGrid(p);
      minDist = snapPoint.distSquared(p);
   }

   if(mSnapContext != NO_SNAPPING)
   {
      // Where will we be snapping things?
      bool snapToWallCorners = getSnapToWallCorners();

      // Now look for other things we might want to snap to
      for(S32 i = 0; i < objList->size(); i++)
      {
         BfObject *obj = static_cast<BfObject *>(objList->get(i));

         // Don't snap to selected items or items with selected verts (keeps us from snapping to ourselves, which is usually trouble)
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
         checkCornersForSnap(p, wallSegmentManager->getWallEdgeDatabase()->findObjects_fast(), minDist, snapPoint);
   }

   return snapPoint;
}


bool EditorUserInterface::getSnapToWallCorners()
{
   // Allow snapping to wall corners when we're dragging items.  Disallow for all wall types other than PolyWall
   return mSnapContext != NO_SNAPPING && mDraggingObjects &&
         (!isWallType(mSnapObject->getObjectTypeNumber()) || mSnapObject->getObjectTypeNumber() == PolyWallTypeNumber);
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


S32 EditorUserInterface::checkCornersForSnap(const Point &clickPoint, const Vector<DatabaseObject *> *edges, F32 &minDist, Point &snapPoint)
{
   const Point *vert;

   for(S32 i = 0; i < edges->size(); i++)
      for(S32 j = 0; j < 1; j++)
      {
         WallEdge *edge = static_cast<WallEdge *>(edges->get(i));
         vert = (j == 0) ? edge->getStart() : edge->getEnd();
         if(checkPoint(clickPoint, *vert, minDist, snapPoint))
            return i;
      }

   return NONE;
}


////////////////////////////////////
////////////////////////////////////
// Rendering routines

extern Color gErrorMessageTextColor;


bool EditorUserInterface::showMinorGridLines()
{
   return mCurrentScale >= .5;
}


void EditorUserInterface::renderTurretAndSpyBugRanges(GridDatabase *editorDb)
{
   const Vector<DatabaseObject *> *spyBugs = editorDb->findObjects_fast(SpyBugTypeNumber);

   if(spyBugs->size() != 0)
   {
      // Use Z Buffer to make use of not drawing overlap visible area of same team SpyBug, but does overlap different team
      glClear(GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_DEPTH_WRITEMASK);
      glDepthFunc(GL_LESS);
      glPushMatrix();
      glTranslatef(0, 0, -0.95f);

      // This blending works like this, source(SRC) * GL_ONE_MINUS_DST_COLOR + destination(DST) * GL_ONE
      glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);  

      // Draw spybug visibility ranges first, underneath everything else
      for(S32 i = 0; i < spyBugs->size(); i++)
      {
         SpyBug *sb = static_cast<SpyBug *>(spyBugs->get(i));
         F32 translation = 0.05f * sb->getTeam();  // This way, each team ends up with a consistent but unique z-pos

         glTranslatef(0, 0, translation);

         Point pos = sb->getPos();
         pos *= mCurrentScale;
         pos += mCurrentOffset;
         renderSpyBugVisibleRange(pos, *sb->getColor(), mCurrentScale);

         glTranslatef(0, 0, -translation);         // Reset translation back to where it was
      }

      setDefaultBlendFunction();

      glPopMatrix();
      glDisable(GL_DEPTH_WRITEMASK);
      glDisable(GL_DEPTH_TEST);
   }

   // Next draw turret firing ranges for selected or highlighted turrets only
   fillVector.clear();
      
   editorDb->findObjects(TurretTypeNumber, fillVector);
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      BfObject *editorObj = dynamic_cast<BfObject *>(fillVector[i]);
      if(editorObj->isSelected() || editorObj->isLitUp())
      {
         Point pos = editorObj->getPos();
         pos *= mCurrentScale;
         pos += mCurrentOffset;
         renderTurretFiringRange(pos, *editorObj->getColor(), mCurrentScale);
      }
   }
}


S32 getDockHeight()
{
   return gScreenInfo.getGameCanvasHeight() - 2 * EditorUserInterface::vertMargin;
}


void EditorUserInterface::renderDock()    
{
   // Render item dock down RHS of screen
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   S32 dockHeight = getDockHeight();

   drawFilledRect(canvasWidth - DOCK_WIDTH - horizMargin, canvasHeight - vertMargin, 
                  canvasWidth - horizMargin,              canvasHeight - vertMargin - dockHeight, 
                  Colors::red30, .7f, (mouseOnDock() ? Colors::yellow : Colors::white));
}


const S32 PANEL_TEXT_SIZE = 10;
const S32 PANEL_SPACING = S32(PANEL_TEXT_SIZE * 1.3);

static S32 PanelBottom, PanelTop, PanelLeft, PanelRight, PanelInnerMargin;

void EditorUserInterface::renderInfoPanel() 
{
   // Recalc dimensions in case screen mode changed
   PanelBottom = gScreenInfo.getGameCanvasHeight() - EditorUserInterface::vertMargin;
   PanelTop    = PanelBottom - (4 * PANEL_SPACING + 9);
   PanelLeft   = EditorUserInterface::horizMargin;
   PanelRight  = PanelLeft + 180;      // left + width
   PanelInnerMargin = 4;

   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   drawFilledRect(PanelLeft, PanelBottom, PanelRight, PanelTop, Colors::richGreen, .7f, Colors::white);

   // Draw coordinates on panel -- if we're moving an item, show the coords of the snap vertex, otherwise show the coords of the
   // snapped mouse position
   Point pos;

   if(mSnapObject)
      pos = mSnapObject->getVert(mSnapVertexIndex);
   else
      pos = snapPoint(getDatabase(), convertCanvasToLevelCoord(mMousePos));


   glColor(Colors::white);
   renderPanelInfoLine(1, "Cursor X,Y: %2.1f,%2.1f", pos.x, pos.y);

   // And scale
   renderPanelInfoLine(2, "Zoom Scale: %2.2f", mCurrentScale);

   // Show number of teams
   renderPanelInfoLine(3, "Team Count: %d", getTeamCount());

   glColor(mNeedToSave ? Colors::red : Colors::green);     // Color level name by whether it needs to be saved or not

   // Filename without extension
   renderPanelInfoLine(4, "Filename: %s%s", mNeedToSave ? "*" : "", mEditFileName.substr(0, mEditFileName.find_last_of('.')).c_str());
}


void EditorUserInterface::renderPanelInfoLine(S32 line, const char *format, ...)
{
   const S32 xpos = horizMargin + PanelInnerMargin;

   va_list args;
   static char text[512];  // reusable buffer

   va_start(args, format);
   vsnprintf(text, sizeof(text), format, args); 
   va_end(args);

   drawString(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - PANEL_TEXT_SIZE - line * PANEL_SPACING + 6, PANEL_TEXT_SIZE, text);
}


// Helper to render attributes in a colorful and lady-like fashion
static void renderAttribText(S32 xpos, S32 ypos, S32 textsize, 
                             const Color &keyColor, const Color &valColor, 
                             const Vector<string> &keys, const Vector<string> &vals)
{
   TNLAssert(keys.size() == vals.size(), "Expected equal number of keys and values!");
   for(S32 i = 0; i < keys.size(); i++)
   {
      glColor(keyColor);
      xpos += drawStringAndGetWidth(xpos, ypos, textsize, keys[i].c_str());
      xpos += drawStringAndGetWidth(xpos, ypos, textsize, ": ");

      glColor(valColor);
      xpos += drawStringAndGetWidth(xpos, ypos, textsize, vals[i].c_str());
      if(i < keys.size() - 1)
         xpos += drawStringAndGetWidth(xpos, ypos, textsize, "; ");
   }
}


// Shows selected item attributes, or, if we're hovering over dock item, shows dock item info string
void EditorUserInterface::renderItemInfoPanel()
{
   string attribsText, hoverText, itemName, attribs;     // All intialized to ""

   S32 hitCount = 0;
   bool multipleKindsOfObjectsSelected = false;
   bool dockItem = false;

   static Vector<string> keys, values;    // Reusable containers
   keys.clear();
   values.clear();

   const char *instructs = "";

   if(mDockItemHit)
   {
      itemName    = mDockItemHit->getOnScreenName();
      attribsText = mDockItemHit->getEditorHelpString();
      dockItem = true;
   }
   else
   {
      // Cycle through all our objects to find the selected ones
      const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

      for(S32 i = 0; i < objList->size(); i++)
      {
         BfObject *obj = static_cast<BfObject *>(objList->get(i));

         if(obj->isSelected())
         {
            if(hitCount == 0)       // This is the first object we've hit
            {
               itemName   = obj->getOnScreenName();
               obj->fillAttributesVectors(keys, values);
               instructs  = obj->getInstructionMsg(keys.size());      // Various objects have different instructions

               S32 id = obj->getUserAssignedId();
               keys.push_back("Id");
               values.push_back(id > 0 ? itos(id) : "Unassigned");
            }
            else                    // Second or subsequent selected object found
            {
               if(multipleKindsOfObjectsSelected || itemName != obj->getOnScreenName())    // Different type of object
               {
                  itemName = "Multiple object types selected";
                  multipleKindsOfObjectsSelected = true;
               }
            }

            hitCount++;
         }  // end if obj is selected

         else if(obj->isLitUp())
            hoverText = string("Hover: ") + obj->getOnScreenName();
      }
   }

   /////
   // Now render the info we collected above

   // Green for dock item, yellow for regular item
   const Color *textColor = (mDockItemHit ? &Colors::green : &Colors::yellow);

   S32 xpos = PanelRight + 9;
   S32 ypos = PanelBottom - PANEL_TEXT_SIZE - PANEL_SPACING + 6;
   S32 upperLineTextSize = 14;

   if(hitCount == 1)
   {
      glColor(textColor);
      S32 w = drawStringAndGetWidth(xpos, ypos, PANEL_TEXT_SIZE, instructs);
      if(w > 0)
         w += drawStringAndGetWidth(xpos + w, ypos, PANEL_TEXT_SIZE, "; ");
      drawString(xpos + w, ypos, PANEL_TEXT_SIZE, "[#] to edit Id");

      if(!dockItem)
         renderAttribText(xpos, ypos - PANEL_SPACING, PANEL_TEXT_SIZE, Colors::cyan, Colors::white, keys, values);
   }

   ypos -= PANEL_SPACING + S32(upperLineTextSize * 1.3);

   if(hitCount > 0)
   {
      if(!multipleKindsOfObjectsSelected)
         itemName = (mDraggingObjects ? "Dragging " : "Selected ") + itemName;

      if(hitCount > 1)
         itemName += " (" + itos(hitCount) + ")";

      glColor(textColor);
      drawString(xpos, ypos, upperLineTextSize, itemName.c_str());
   }

   ypos -= S32(upperLineTextSize * 1.3);
   if(hoverText != "" && !mDockItemHit)
   {
      glColor(Colors::white);
      drawString(xpos, ypos, upperLineTextSize, hoverText.c_str());
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
            const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

            for(S32 i = 0; i < objList->size(); i++)
            {
               BfObject *obj = static_cast<BfObject *>(objList->get(i));

               if(obj->getUserAssignedId() == id && !obj->isSelected())
               {
                  errorFound = true;
                  break;
               }
            }
         }
      }

      // Calculate box width
      S32 boxwidth = 2 * inset + getStringWidth(fontsize, mEntryBox.getPrompt().c_str()) + 
          mEntryBox.getMaxLen() * getStringWidth(fontsize, "-") + 25;

      // Render entry box
      TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

      S32 xpos = (gScreenInfo.getGameCanvasWidth()  - boxwidth) / 2;
      S32 ypos = (gScreenInfo.getGameCanvasHeight() - boxheight) / 2;

      Color boxColor = Color(.3f,.6f,.3f);
      drawFilledRect(xpos, ypos, xpos + boxwidth, ypos + boxheight, boxColor, .85f, boxColor);

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
      renderShip(ShipShape::Normal, &Colors::red, 1, thrusts, 1, 5, 0, false, false, false, false);
      glRotatef(-90, 0, 0, 1);

      // And show how far it can see
      const S32 horizDist = Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL;
      const S32 vertDist = Game::PLAYER_VISUAL_DISTANCE_VERTICAL;

      TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

      glColor4f(.5, .5, 1, .35f);
      drawFilledRect(-horizDist, -vertDist, horizDist, vertDist);

   glPopMatrix();
}


static F32 getRenderingAlpha(bool isScriptItem)
{
   return isScriptItem ? .4f : 1;     // Script items will appear somewhat translucent
}


void EditorUserInterface::render()
{
   GridDatabase *editorDb = getDatabase();

   mouseIgnore = false;                // Avoid freezing effect from too many mouseMoved events without a render in between (sam)

   TNLAssert(glIsEnabled(GL_BLEND), "Blending should be on here!");

   // Render bottom-most layer of our display
   if(mPreviewMode)
      renderTurretAndSpyBugRanges(editorDb);    // Render range of all turrets and spybugs in editorDb
   else
      renderGrid(mCurrentScale, mCurrentOffset, convertLevelToCanvasCoord(Point(0,0)), 
                 getGame()->getGridSize(), mSnapContext == FULL_SNAPPING, showMinorGridLines());                    

   glPushMatrix();
      glTranslate(getCurrentOffset());
      glScale(getCurrentScale());

      // mSnapDelta only gets recalculated during a dragging event -- if an item is no longer being dragged, we
      // don't want to use the now stale value in mSnapDelta, but rather (0,0) to reflect the rahter obvoius fact
      // that walls that are not being dragged should be rendered in place.
      static Point delta;
      delta = mDraggingObjects ? mSnapDelta : Point(0,0);

      // == Render walls and polyWalls ==
      renderWallsAndPolywalls(&mLevelGenDatabase, delta, false, true );
      renderWallsAndPolywalls(editorDb, delta, false, false);

      // == Normal, unselected items ==
      // Draw map items (teleporters, etc.) that are not being dragged, and won't have any text labels  (below the dock)
      renderObjects(editorDb, RENDER_UNSELECTED_NONWALLS, false);             // Render our normal objects
      renderObjects(&mLevelGenDatabase, RENDER_UNSELECTED_NONWALLS, true);    // Render any levelgen objects being overlaid

      // == Selected items ==
      // Draw map items (teleporters, etc.) that are are selected and/or lit up, so label is readable (still below the dock)
      // Do this as a separate operation to ensure that these are drawn on top of those drawn above.
      // We do render polywalls here because this is what draws the highlighted outline when the polywall is selected.
      renderObjects(editorDb, RENDER_SELECTED_NONWALLS, false);               // Render selected objects 

      renderWallsAndPolywalls(editorDb, delta, true, false);   

      // == Draw geomPolyLine features under construction ==
      if(mCreatingPoly || mCreatingPolyline)    
         renderObjectsUnderConstruction();

      // Since we're not constructing a barrier, if there are any barriers or lineItems selected, 
      // get the width for display at bottom of dock
      else  
      {
         fillVector.clear();
         editorDb->findObjects((TestFunc)isLineItemType, fillVector);

         for(S32 i = 0; i < fillVector.size(); i++)
         {
            LineItem *obj = dynamic_cast<LineItem *>(fillVector[i]);   // Walls are a subclass of LineItem, so this will work for both

            if(obj && (obj->isSelected() || (obj->isLitUp() && obj->isVertexLitUp(NONE))))
               break;
         }
      }

      // Render our snap vertex as a hollow magenta box...
      if(!mPreviewMode && mSnapObject && mSnapObject->isSelected() && mSnapVertexIndex != NONE &&        // ...but not in preview mode...
         mSnapObject->getGeomType() != geomPoint &&                                                      // ...and not on point objects...
         !mSnapObject->isVertexLitUp(mSnapVertexIndex) && !mSnapObject->vertSelected(mSnapVertexIndex))  // ...or selected vertices
      {
         renderVertex(SnappingVertex, mSnapObject->getVert(mSnapVertexIndex), NO_NUMBER, mCurrentScale/*, alpha*/);  
      }

    glPopMatrix(); 


   if(mPreviewMode)
      renderReferenceShip();
   else
   {
      // The following items are hidden in preview mode:
      renderDock();
      renderDockItems();
      renderInfoPanel();
      renderItemInfoPanel();

      if(mouseOnDock() && mDockItemHit)
         mDockItemHit->setLitUp(true);       // Will trigger a selection highlight to appear around dock item
   }

   renderDragSelectBox();

   if(mAutoScrollWithMouse)
   {
      glColor(Colors::white);
      drawFourArrows(mScrollWithMouseLocation);
   }

   renderSaveMessage();
   renderWarnings();

   renderTextEntryOverlay();

   renderConsole();        // Rendered last, so it's always on top
}


static void setColor(bool isSelected, bool isLitUp, bool isScriptItem)
{
   F32 alpha = isScriptItem ? .6f : 1;     // So script items will appear somewhat translucent

   if(isSelected)
      glColor(Colors::EDITOR_SELECT_COLOR, alpha);       // yellow
   else if(isLitUp)
      glColor(Colors::EDITOR_HIGHLIGHT_COLOR, alpha);    // white
   else  // Normal
      glColor(Colors::EDITOR_PLAIN_COLOR, alpha);
}


// Render objects in the specified database
void EditorUserInterface::renderObjects(GridDatabase *database, RenderModes renderMode, bool isLevelgenOverlay)
{
   const Vector<DatabaseObject *> *objList = database->findObjects_fast();

   bool wantSelected = (renderMode == RENDER_SELECTED_NONWALLS || renderMode == RENDER_SELECTED_WALLS);
   bool wantWalls =    (renderMode == RENDER_UNSELECTED_WALLS  || renderMode == RENDER_SELECTED_WALLS);

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      bool isSelected = obj->isSelected() || obj->isLitUp();
      bool isWall = isWallType(obj->getObjectTypeNumber());

      if(isSelected == wantSelected && isWall == wantWalls)     
      {
         // Items are rendered in index order, so those with a higher index get drawn later, and hence, on top
         setColor(obj->isSelected(), obj->isLitUp(), isLevelgenOverlay);

         if(mPreviewMode)
         {
            BfObject *bfObj = dynamic_cast<BfObject *>(obj);
            if(bfObj)
               bfObj->render();
         }
         else
         {
            obj->renderEditor(mCurrentScale, getSnapToWallCorners());
            obj->renderAndLabelHighlightedVertices(mCurrentScale);
         }
      }
   }
}


// Render walls (both normal walls and polywalls, outlines and fills) and centerlines
void EditorUserInterface::renderWallsAndPolywalls(GridDatabase *database, const Point &offset, bool drawSelected, bool isLevelGenDatabase)
{
   GameSettings *settings = getGame()->getSettings();

   WallSegmentManager *wsm = database->getWallSegmentManager();
   const Color &fillColor = mPreviewMode ? *settings->getWallFillColor() : Colors::EDITOR_WALL_FILL_COLOR;

   renderWalls(wsm->getWallSegmentDatabase(), *wsm->getWallEdgePoints(), *wsm->getSelectedWallEdgePoints(), *settings->getWallOutlineColor(),  
               fillColor, mCurrentScale, mDraggingObjects, drawSelected, offset, mPreviewMode, 
               getSnapToWallCorners(), getRenderingAlpha(isLevelGenDatabase));


   // Render walls as ordinary objects; this will draw wall centerlines
   if(!isLevelGenDatabase)
      renderObjects(database, drawSelected ? RENDER_SELECTED_WALLS : RENDER_UNSELECTED_WALLS, false);  
}


void EditorUserInterface::renderObjectsUnderConstruction()
{
   // Add a vert (and deleted it later) to help show what this item would look like if the user placed the vert in the current location
   mNewItem->addVert(snapPoint(getDatabase(), convertCanvasToLevelCoord(mMousePos)));
   glLineWidth(gLineWidth3);

   if(mCreatingPoly) // Wall
      glColor(Colors::EDITOR_SELECT_COLOR);
   else              // LineItem --> Caution! we're rendering an object that doesn't exist yet; its game is NULL
      glColor(getGame()->getTeamColor(mCurrentTeam));

   renderLine(mNewItem->getOutline());

   glLineWidth(gDefaultLineWidth);

   for(S32 j = mNewItem->getVertCount() - 1; j >= 0; j--)      // Go in reverse order so that placed vertices are drawn atop unplaced ones
   {
      Point v = mNewItem->getVert(j);
            
      // Draw vertices
      if(j == mNewItem->getVertCount() - 1)                    // This is our most current vertex
         renderVertex(HighlightedVertex, v, NO_NUMBER, mCurrentScale);
      else
         renderVertex(SelectedItemVertex, v, j, mCurrentScale);
   }
   mNewItem->deleteVert(mNewItem->getVertCount() - 1); 
}


// Draw box for selecting items
void EditorUserInterface::renderDragSelectBox()
{
   if(!mDragSelecting)   
      return;
   
   glColor(Colors::white);
   Point downPos = convertLevelToCanvasCoord(mMouseDownPos);
   drawHollowRect(downPos, mMousePos);
}


static const S32 DOCK_LABEL_SIZE = 9;      // Size to label items on the dock

// Forward declarations to allow us to put functions in human readable order
static void renderDockItemLabel(const Point &pos, const char *label);
static void renderDockItem(BfObject *object, F32 currentScale, S32 snapVertexIndex);

void EditorUserInterface::renderDockItems()
{
   for(S32 i = 0; i < mDockItems.size(); i++)
      renderDockItem(mDockItems[i].get(), mCurrentScale, mSnapVertexIndex);
}


static void renderDockItem(BfObject *object, F32 currentScale, S32 snapVertexIndex)
{
   glColor(Colors::EDITOR_PLAIN_COLOR);

   object->renderDock();
   renderDockItemLabel(object->getDockLabelPos(), object->getOnDockName());

   if(object->isLitUp())
     object->highlightDockItem();

   object->setLitUp(false);
}


static void renderDockItemLabel(const Point &pos, const char *label)
{
   F32 xpos = pos.x;
   F32 ypos = pos.y - DOCK_LABEL_SIZE / 2;
   glColor(Colors::white);
   drawStringc(xpos, ypos + (F32)DOCK_LABEL_SIZE, (F32)DOCK_LABEL_SIZE, label);
}


void EditorUserInterface::renderSaveMessage()
{
   if(mSaveMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if(mSaveMsgTimer.getCurrent() < 1000)
         alpha = (F32) mSaveMsgTimer.getCurrent() / 1000;

      TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

      glColor(mSaveMsgColor, alpha);
      drawCenteredString(gScreenInfo.getGameCanvasHeight() - vertMargin - 65, 25, mSaveMsg.c_str());
   }
}


void EditorUserInterface::renderWarnings()
{
   if(mWarnMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (mWarnMsgTimer.getCurrent() < 1000)
         alpha = (F32) mWarnMsgTimer.getCurrent() / 1000;

      TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

      glColor(mWarnMsgColor, alpha);
      drawCenteredString(gScreenInfo.getGameCanvasHeight() / 4, 25, mWarnMsg1.c_str());
      drawCenteredString(gScreenInfo.getGameCanvasHeight() / 4 + 30, 25, mWarnMsg2.c_str());
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


// Mark all objects in database as unselected
void EditorUserInterface::clearSelection(GridDatabase *database)
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));
      obj->unselect();
   }
}


// Mark everything as selected
void EditorUserInterface::selectAll(GridDatabase *database)
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));
      obj->setSelected(true);
   }
}


bool EditorUserInterface::anyItemsSelected(GridDatabase *database)
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));
      if(obj->isSelected())
         return true;
   }

   return false;
}


// Copy selection to the clipboard
void EditorUserInterface::copySelection()
{
   GridDatabase *database = getDatabase();

   if(!anyItemsSelected(database))
      return;

   mClipboard.clear();     

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->isSelected())
         mClipboard.push_back(boost::shared_ptr<BfObject>(obj->copy()));
   }
}


// Paste items on the clipboard
void EditorUserInterface::pasteSelection()
{
   if(mDraggingObjects)    // Pasting while dragging can cause crashes!!
      return;

   S32 objCount = mClipboard.size();

    if(objCount == 0)         // Nothing on clipboard, nothing to do
      return;

   saveUndoState();           // So we can undo the paste

   GridDatabase *database = getDatabase();
   clearSelection(database);  // Only the pasted items should be selected when we're done

   Point pastePos = snapPoint(database, convertCanvasToLevelCoord(mMousePos));

   Point firstPoint = mClipboard[0]->getVert(0);

   Point offsetFromFirstPoint;
   Vector<DatabaseObject *> copiedObjects;

   for(S32 i = 0; i < objCount; i++)
   {
      offsetFromFirstPoint = firstPoint - mClipboard[i]->getVert(0);

      BfObject *newObject = mClipboard[i]->newCopy();
      newObject->setSelected(true);
      newObject->moveTo(pastePos - offsetFromFirstPoint);

      // addToGame is first so setSelected and onGeomChanged have mGame (at least barriers need it)
      newObject->addToGame(getGame(), NULL);    // Passing NULL keeps item out of any databases... will add in bulk below  

      copiedObjects.push_back(newObject);
   }

   getDatabase()->addToDatabase(copiedObjects);
      
   for(S32 i = 0; i < copiedObjects.size(); i++)   
      copiedObjects[i]->onGeomChanged();

   onSelectionChanged();

   validateLevel();
   setNeedToSave(true);
   autoSave();
}


// Expand or contract selection by scale (i.e. resize)
void EditorUserInterface::scaleSelection(F32 scale)
{
   GridDatabase *database = getDatabase();

   if(!anyItemsSelected(database) || scale < .01 || scale == 1)    // Apply some sanity checks; limits here are arbitrary
      return;

   saveUndoState();

   // Find center of selection
   Point min, max;                        
   database->computeSelectionMinMax(min, max);
   Point ctr = (min + max) * 0.5;

   if(scale > 1 && min.distanceTo(max) * scale  > 50 * getGame()->getGridSize())    // If walls get too big, they'll bog down the db
      return;

   bool modifiedWalls = false;
   WallSegmentManager *wallSegmentManager = database->getWallSegmentManager();

   wallSegmentManager->beginBatchGeomUpdate();

   const Vector<DatabaseObject *> *objList = database->findObjects_fast();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->isSelected())
      {
         obj->scale(ctr, scale);
         obj->onGeomChanged();

         if(isWallType(obj->getObjectTypeNumber()))
            modifiedWalls = true;
      }
   }

   wallSegmentManager->endBatchGeomUpdate(database, modifiedWalls);

   setNeedToSave(true);
   autoSave();
}


// Rotate selected objects around their center point by angle
void EditorUserInterface::rotateSelection(F32 angle)
{
   GridDatabase *database = getDatabase();

   if(!anyItemsSelected(database))
      return;

   saveUndoState();

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();


   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->isSelected())
      {
         obj->rotateAboutPoint(Point(0,0), angle);
         obj->onGeomChanged();
      }
   }

   setNeedToSave(true);
   autoSave();
}


// Set the team affiliation of any selected items
void EditorUserInterface::setCurrentTeam(S32 currentTeam)
{
   mCurrentTeam = currentTeam;
   bool anyChanged = false;

   if(anythingSelected())
      saveUndoState();

   if(currentTeam >= getTeamCount())
   {
      char msg[255];

      if(getTeamCount() == 1)
         dSprintf(msg, sizeof(msg), "Only 1 team has been configured.");
      else
         dSprintf(msg, sizeof(msg), "Only %d teams have been configured.", getTeamCount());

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


   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

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
   Point min, max;
   getDatabase()->computeSelectionMinMax(min, max);
   F32 centerX = (min.x + max.x) / 2;

   flipSelection(centerX, true);
}


void EditorUserInterface::flipSelectionVertical()
{
   Point min, max;
   getDatabase()->computeSelectionMinMax(min, max);
   F32 centerY = (min.y + max.y) / 2;

   flipSelection(centerY, false);
}


void EditorUserInterface::flipSelection(F32 center, bool isHoriz)
{
   GridDatabase *database = getDatabase();

   if(!anyItemsSelected(database))
      return;

   saveUndoState();

   Point min, max;
   database->computeSelectionMinMax(min, max);
//   F32 centerX = (min.x + max.x) / 2;

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   bool modifiedWalls = false;
   WallSegmentManager *wallSegmentManager = database->getWallSegmentManager();

   wallSegmentManager->beginBatchGeomUpdate();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->isSelected())
      {
         obj->flip(center, isHoriz);
         obj->onGeomChanged();

         if(isWallType(obj->getObjectTypeNumber()))
            modifiedWalls = true;
      }
   }

   wallSegmentManager->endBatchGeomUpdate(database, modifiedWalls);

   setNeedToSave(true);
   autoSave();
}


static const S32 POINT_HIT_RADIUS = 9;
static const S32 EDGE_HIT_RADIUS = 6;

void EditorUserInterface::findHitItemAndEdge()
{
   mHitItem = NULL;
   mEdgeHit = NONE;
   mHitVertex = NONE;

   // Make hit rectangle larger than 1x1 -- when we consider point items, we need to make sure that we grab the item even when we're not right
   // on top of it, as the point item's hit target is much larger than the item itself.  100 is a guess that seems to work well.
   // Note that this is only used for requesting a candidate list from the database, actual hit detection is more precise.
   const Rect cursorRect((mMousePos - mCurrentOffset) / mCurrentScale, 100); 

   fillVector.clear();
   GridDatabase *editorDb = getDatabase();
   editorDb->findObjects((TestFunc)isAnyObjectType, fillVector, cursorRect);

   Point mouse = convertCanvasToLevelCoord(mMousePos);      // Figure out where the mouse is in level coords

   // Do this in two passes -- the first we only consider selected items, the second pass will consider all targets.
   // This will give priority to hitting vertices of selected items.
   for(S32 firstPass = 1; firstPass >= 0; firstPass--)     // firstPass will be true the first time through, false the second time
      for(S32 i = fillVector.size() - 1; i >= 0; i--)      // Go in reverse order to prioritize items drawn on top
      {
         BfObject *obj = dynamic_cast<BfObject *>(fillVector[i]);

         TNLAssert(obj, "Expected a BfObject!");

         if(firstPass == (!obj->isSelected() && !obj->anyVertsSelected()))  // First pass is for selected items only
            continue;                                                       // Second pass only for unselected items
         
         if(checkForVertexHit(obj) || checkForEdgeHit(mouse, obj)) 
            return;                 
      }

   // We've already checked for wall vertices; now we'll check for hits in the interior of walls
   GridDatabase *wallDb = editorDb->getWallSegmentManager()->getWallSegmentDatabase();
   fillVector2.clear();

   wallDb->findObjects((TestFunc)isAnyObjectType, fillVector2, cursorRect);

   for(S32 i = 0; i < fillVector2.size(); i++)
      if(checkForWallHit(mouse, fillVector2[i]))
         return;

   // If we're still here, it means we didn't find anything yet.  Make one more pass, and see if we're in any polys.
   // This time we'll loop forward, though I don't think it really matters.
   for(S32 i = 0; i < fillVector.size(); i++)
     if(checkForPolygonHit(mouse, dynamic_cast<BfObject *>(fillVector[i])))
        return;
}


// Vertex is weird because we don't always do thing in level coordinates -- some of our hit computation is based on
// absolute screen coordinates; some things, like wall vertices, are the same size at every zoom scale.  
bool EditorUserInterface::checkForVertexHit(BfObject *object)
{
   F32 radius = object->getEditorRadius(mCurrentScale);

   for(S32 i = object->getVertCount() - 1; i >= 0; i--)
   {
      // p represents pixels from mouse to obj->getVert(j), at any zoom
      Point p = mMousePos - mCurrentOffset - (object->getVert(i) + object->getEditorSelectionOffset(mCurrentScale)) * mCurrentScale;    

      if(fabs(p.x) < radius && fabs(p.y) < radius)
      {
         mHitItem = object;
         mHitVertex = i;
         return true;
      }
   }

   return false;
}


bool EditorUserInterface::checkForEdgeHit(const Point &point, BfObject *object)
{
   // Points have no edges, and walls are checked via another mechanism
   if(object->getGeomType() == geomPoint) 
      return false;

   const Vector<Point> &verts = *object->getOutline(); 
   TNLAssert(verts.size(), "Empty vertex problem");

   bool loop = (object->getGeomType() == geomPolygon);

   Point closest;

   S32 j_prev  = loop ? (verts.size() - 1) : 0;
         
   for(S32 j = loop ? 0 : 1; j < verts.size(); j++)
   {
      if(findNormalPoint(point, verts[j_prev], verts[j], closest))
      {
         F32 distance = (point - closest).len();
         if(distance < EDGE_HIT_RADIUS / mCurrentScale) 
         {
            mHitItem = object;
            mEdgeHit = j_prev;

            return true;
         }
      }
      j_prev = j;
   }

   return false;
}


bool EditorUserInterface::checkForWallHit(const Point &point, DatabaseObject *object)
{
   TNLAssert(dynamic_cast<WallSegment *>(object), "Expected a WallSegment!");
   WallSegment *wallSegment = static_cast<WallSegment *>(object);

   if(triangulatedFillContains(wallSegment->getTriangulatedFillPoints(), point))
   {
      // Now that we've found a segment that our mouse is over, we need to find the wall object that it belongs to.  Chances are good
      // that it will be one of the objects sitting in fillVector.
      for(S32 i = 0; i < fillVector.size(); i++)
      {
         if(isWallType(fillVector[i]->getObjectTypeNumber()))
         {
            BfObject *eobj = dynamic_cast<BfObject *>(fillVector[i]);

            if(eobj->getSerialNumber() == wallSegment->getOwner())
            {
               mHitItem = eobj;
               return true;
            }
         }
      }

      // Note, if we get to here, we have a problem.

      // This code does a less efficient but more thorough job finding a wall that matches the segment we hit... if the above assert
      // keeps going off, and we can't fix it, this code here should take care of the problem.  But using it is an admission of failure.

      const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

      for(S32 i = 0; i < objList->size(); i++)
      {
         BfObject *obj = static_cast<BfObject *>(objList->get(i));

         if(isWallType(obj->getObjectTypeNumber()))
         {
            if(obj->getSerialNumber() == wallSegment->getOwner())
            {
               mHitItem = obj;
               return true;
            }
         }
      }
   }

   return false;
}


bool EditorUserInterface::checkForPolygonHit(const Point &point, BfObject *object)
{
   if(object->getGeomType() == geomPolygon && triangulatedFillContains(object->getFill(), point))
   {
      mHitItem = object;
      return true;
   }

   return false;
}


// Sets mDockItemHit
void EditorUserInterface::findHitItemOnDock()
{
   mDockItemHit = NULL;

   for(S32 i = mDockItems.size() - 1; i >= 0; i--)     // Go in reverse order because the code we copied did ;-)
   {
      Point pos = mDockItems[i]->getPos();

      if(fabs(mMousePos.x - pos.x) < POINT_HIT_RADIUS && fabs(mMousePos.y - pos.y) < POINT_HIT_RADIUS)
      {
         mDockItemHit = mDockItems[i].get();
         return;
      }
   }

   // Now check for polygon interior hits
   for(S32 i = 0; i < mDockItems.size(); i++)
      if(mDockItems[i]->getGeomType() == geomPolygon)
      {
         Vector<Point> verts;
         for(S32 j = 0; j < mDockItems[i]->getVertCount(); j++)
            verts.push_back(mDockItems[i]->getVert(j));

         if(polygonContainsPoint(verts.address(),verts.size(), mMousePos))
         {
            mDockItemHit = mDockItems[i].get();
            return;
         }
      }

   return;
}


void EditorUserInterface::onMouseMoved()
{
   Parent::onMouseMoved();

   if(mouseIgnore)  // Needed to avoid freezing effect from too many mouseMoved events without a render in between
      return;

   mouseIgnore = true;

   mMousePos.set(gScreenInfo.getMousePos());

   // Doing this with MOUSE_RIGHT allows you to drag a vertex you just placed by holding the right-mouse button
   if(InputCodeManager::getState(MOUSE_LEFT) || InputCodeManager::getState(MOUSE_RIGHT) || InputCodeManager::getState(MOUSE_MIDDLE))
   {
      onMouseDragged();
      return;
   }

   if(mCreatingPoly || mCreatingPolyline)
      return;

   // Turn off highlight on selected item -- will be turned back on for this object or another below
   if(mHitItem)
      mHitItem->setLitUp(false);

   findHitItemAndEdge();      //  Sets mHitItem, mHitVertex, and mEdgeHit
   findHitItemOnDock();

   // We hit a vertex that wasn't already selected
   if(mHitItem && mHitVertex != NONE && !mHitItem->vertSelected(mHitVertex))   
      mHitItem->setVertexLitUp(mHitVertex);

   // Highlight currently selected item
   if(mHitItem)
      mHitItem->setLitUp(true);

   findSnapVertex();
   Cursor::enableCursor();
}


// During the snapping process, when an engineered item is snapped to the wall, it is translated over to its new position.  This
// function looks at an object and determines if it has already been translated or not.
static bool alreadyTranslated(BfObject *object)
{
   return isEngineeredType(object->getObjectTypeNumber()) && static_cast<EngineeredItem *>(object)->isSnapped();
}


void EditorUserInterface::onMouseDragged()
{
   if(InputCodeManager::getState(MOUSE_MIDDLE) && mMousePos != mScrollWithMouseLocation)
   {
      mCurrentOffset += mMousePos - mScrollWithMouseLocation;
      mScrollWithMouseLocation = mMousePos;
      mAutoScrollWithMouseReady = false;

      return;
   }

   if(mCreatingPoly || mCreatingPolyline || mDragSelecting)
      return;


   bool needToSaveUndoState = true;

   // I assert we never need to save an input state here if we are right-mouse dragging
   if(InputCodeManager::getState(MOUSE_RIGHT))
      needToSaveUndoState = false;

   if(mDraggingDockItem != NULL)    // We just started dragging an item off the dock
   {
       startDraggingDockItem();  
       needToSaveUndoState = false;
   }

   findSnapVertex();                               // Sets mSnapObject and mSnapVertexIndex
   if(!mSnapObject || mSnapVertexIndex == NONE)    // If we've just started dragging a dock item, this will be it
      return;


   if(!mDraggingObjects)            // Just started dragging
   {
      if(needToSaveUndoState)
         saveUndoState(true);                 // Save undo state before we clear the selection

      mMoveOrigin = mSnapObject->getVert(mSnapVertexIndex);

      const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

#ifdef TNL_OS_MAC_OSX 
      bool ctrlDown = InputCodeManager::getState(KEY_META);
#else
      bool ctrlDown = InputCodeManager::getState(KEY_CTRL);
#endif

      if(ctrlDown)     // Ctrl+Drag ==> copy and drag (except for Mac)
      {
         Vector<DatabaseObject *> copiedObjects;

         for(S32 i = 0; i < objList->size(); i++)
         {
            BfObject *obj = static_cast<BfObject *>(objList->get(i));

            if(obj->isSelected())
            {
               BfObject *newObject = obj->newCopy();   
               newObject->setSelected(true);
               newObject->addToGame(getGame(), NULL);    // NULL keeps object out of database... will be added in bulk below 
               
               copiedObjects.push_back(newObject);

               // Make mHitItem be the new copy of the old mHitItem
               if(mHitItem == obj)
                  mHitItem = newObject;

               if(mSnapObject == obj)
                  mSnapObject = newObject;
            }
         }

         mDragCopying = true;

         // Now mark source objects as unselected
         for(S32 i = 0; i < objList->size(); i++)
         {
            BfObject *obj = static_cast<BfObject *>(objList->get(i));

            obj->setSelected(false);
            obj->setLitUp(false);
         }

         // Now add copied objects to our database; these were marked as selected when they were created
         getDatabase()->addToDatabase(copiedObjects);

         // Running onGeomChanged causes any copied walls to have a full body while we're dragging them 
         for(S32 i = 0; i < copiedObjects.size(); i++)   
            copiedObjects[i]->onGeomChanged(); 
      }

      onSelectionChanged();
      mDraggingObjects = true; 
      mSnapDelta.set(0,0);
   }


   SDL_SetCursor(Cursor::getSpray());

   Point lastSnapDelta = mSnapDelta;
   // The thinking here is that for large items -- walls, polygons, etc., we may grab an item far from its snap vertex, and we
   // want to factor that offset into our calculations.  For point items (and vertices), we don't really care about any slop
   // in the selection, and we just want the damn thing where we put it.
   if(mSnapObject->getGeomType() == geomPoint || (mHitItem && mHitItem->anyVertsSelected()))
      mSnapDelta = snapPoint(getDatabase(), convertCanvasToLevelCoord(mMousePos)) - mMoveOrigin;
   else  // larger items
      mSnapDelta = snapPoint(getDatabase(), convertCanvasToLevelCoord(mMousePos) + mMoveOrigin - mMouseDownPos) - mMoveOrigin;

   // Update coordinates of dragged item -- unless it's a snapped engineered item, in which case its coordinates have already been updated
   if(!alreadyTranslated(mSnapObject.getPointer()))
      translateSelectedItems(getDatabase(), mSnapDelta - lastSnapDelta);
}


void EditorUserInterface::translateSelectedItems(GridDatabase *database, const Point &offset)
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   S32 count = 0;

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->isSelected() || obj->anyVertsSelected())
      {
         Point newVert;    // Reusable container

         for(S32 j = 0; j < obj->getVertCount(); j++)
            if(obj->isSelected() || obj->vertSelected(j))
            {
               newVert = obj->getVert(j) + offset;
               obj->setVert(newVert, j);
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


BfObject *EditorUserInterface::copyDockItem(BfObject *source)
{
   // Instantiate object so we are essentially dragging a non-dock item
   BfObject *newObject = source->newCopy();
   newObject->newObjectFromDock(getGame()->getGridSize());     // Do things particular to creating an object that came from dock

   return newObject;
}


// User just dragged an item off the dock
void EditorUserInterface::startDraggingDockItem()
{
   saveUndoState();     // Save our undo state before we create a new item

   BfObject *item = copyDockItem(mDraggingDockItem);

   // Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
   Point pos = convertCanvasToLevelCoord(mMousePos) - item->getInitialPlacementOffset(getGame()->getGridSize());
   item->moveTo(pos);
      
   GridDatabase *database = getDatabase();

   addToEditor(item); 
//   database->dumpObjects();
   
   clearSelection(database);    // No items are selected...
   item->setSelected(true);     // ...except for the new one
   onSelectionChanged();
   mDraggingDockItem = NULL;    // Because now we're dragging a real item
   validateLevel();             // Check level for errors


   // Because we sometimes have trouble finding an item when we drag it off the dock, after it's been sorted,
   // we'll manually set mHitItem based on the selected item, which will always be the one we just added.
   // TODO: Still needed?

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   mEdgeHit = NONE;
   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->isSelected())
      {
         mHitItem = obj;
         break;
      }
   }
}


// Sets mSnapObject and mSnapVertexIndex based on the vertex closest to the cursor that is part of the selected set
// What we really want is the closest vertex in the closest feature
void EditorUserInterface::findSnapVertex()
{
   F32 closestDist = F32_MAX;

   if(mDraggingObjects)    // Don't change snap vertex once we're dragging
      return;

   clearSnapEnvironment();

   Point mouseLevelCoord = convertCanvasToLevelCoord(mMousePos);

   // If we have a hit item, and it's selected, find the closest vertex in the item
   if(mHitItem && mHitItem->isSelected())   
   {
      // If we've hit an edge, restrict our search to the two verts that make up that edge
      if(mEdgeHit != NONE)
      {
         mSnapObject = mHitItem;     // Regardless of vertex, this is our hit item
         S32 v1 = mEdgeHit;
         S32 v2 = mEdgeHit + 1;

         // Handle special case of looping item
         if(mEdgeHit == mHitItem->getVertCount() - 1)
            v2 = 0;

         // Find closer vertex: v1 or v2
         mSnapVertexIndex = (mHitItem->getVert(v1).distSquared(mouseLevelCoord) < 
                          mHitItem->getVert(v2).distSquared(mouseLevelCoord)) ? v1 : v2;

         return;
      }

      // Didn't hit an edge... find the closest vertex anywhere in the item
      for(S32 j = 0; j < mHitItem->getVertCount(); j++)
      {
         F32 dist = mHitItem->getVert(j).distSquared(mouseLevelCoord);

         if(dist < closestDist)
         {
            closestDist = dist;
            mSnapObject = mHitItem;
            mSnapVertexIndex = j;
         }
      }
      return;
   } 

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   // Otherwise, we don't have a selected hitItem -- look for a selected vertex
   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

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


// Delete selected items (true = items only, false = items & vertices)
void EditorUserInterface::deleteSelection(bool objectsOnly)
{
   if(mDraggingObjects)     // No deleting while we're dragging, please...
      return;

   if(!anythingSelected())  // Nothing to delete
      return;

   bool deleted = false, deletedWall = false;

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = objList->size() - 1; i >= 0; i--)  // Reverse to avoid having to have i-- in middle of loop
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->isSelected())
      {  
         // Since indices change as items are deleted, this will keep incorrect items from being deleted
         if(obj->isLitUp())
            mHitItem = NULL;

         if(!deleted)
            saveUndoState();

         if(isWallType(obj->getObjectTypeNumber()))
            deletedWall = true;

         deleteItem(i, true);
         deleted = true;
      }
      else if(!objectsOnly)      // Deleted only selected vertices
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
               clearSnapEnvironment();
            }
         }

         // Check if item has too few vertices left to be viable
         if(obj->getVertCount() < obj->getMinVertCount())
         {
            if(isWallType(obj->getObjectTypeNumber()))
               deletedWall = true;

            deleteItem(i, true);
            deleted = true;
         }
         else if(geomChanged)
            obj->onGeomChanged();

      }  // else if(!objectsOnly) 
   }  // for


   if(deletedWall)
      doneDeleteingWalls();

   if(deleted)
   {
      setNeedToSave(true);
      autoSave();

      doneDeleting();
   }
}


// Increase selected wall thickness by amt
void EditorUserInterface::changeBarrierWidth(S32 amt)
{
   if(!mLastUndoStateWasBarrierWidthChange)
      saveUndoState(); 

   fillVector2.clear();    // fillVector gets modified in some child function, so use our secondary reusable container
   getDatabase()->findObjects((TestFunc)isWallItemType, fillVector2);

   for(S32 i = 0; i < fillVector2.size(); i++)
   {
      WallItem *obj = static_cast<WallItem *>(fillVector2[i]);
      if(obj->isSelected())
         obj->changeWidth(amt);     
   }

   mLastUndoStateWasBarrierWidthChange = true;
}


// Split wall/barrier on currently selected vertex/vertices
// Or, if entire wall is selected, split on snapping vertex -- this seems a natural way to do it
void EditorUserInterface::splitBarrier()
{
   bool split = false;

   GridDatabase *database = getDatabase();

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->getGeomType() == geomPolyLine)
          for(S32 j = 1; j < obj->getVertCount() - 1; j++)     // Can't split on end vertices!
            if(obj->vertSelected(j))
            {
               saveUndoState();
               
               doSplit(obj, j);

               split = true;
               goto done2;                         // Yes, gotos are naughty, but they just work so well sometimes...
            }
   }

   // If we didn't find a suitable selected vertex to split on, look for a selected line with a magenta vertex
   if(!split && mSnapObject && mSnapObject->getGeomType() == geomPolyLine && mSnapObject->isSelected() && 
         mSnapVertexIndex != NONE && mSnapVertexIndex != 0 && mSnapVertexIndex != mSnapObject->getVertCount() - 1)
   {         
      saveUndoState();

      doSplit(mSnapObject, mSnapVertexIndex);

      split = true;
   }

done2:
   if(split)
   {
      clearSelection(database);
      setNeedToSave(true);
      autoSave();
   }
}


// Split wall or line -- will probably crash on other geom types
void EditorUserInterface::doSplit(BfObject *object, S32 vertex)
{
   BfObject *newObj = object->newCopy();    // Copy the attributes
   newObj->clearVerts();                        // Wipe out the geometry

   // Note that it would be more efficient to start at the end and work backwards, but the direction of our numbering would be
   // reversed in the new object compared to what it was.  This isn't important at the moment, but it just seems wrong from a
   // geographic POV.  Which I have.
   for(S32 i = vertex; i < object->getVertCount(); i++) 
   {
      newObj->addVert(object->getVert(i), true);      // true: If wall already has more than max number of points, let children have more as well
      if(i != vertex)               // i.e. if this isn't the first iteration
      {
         object->deleteVert(i);     // Don't delete first vertex -- we need it to remain as final vertex of old feature
         i--;
      }
   }

   addToEditor(newObj);     // Needs to happen before onGeomChanged, so mGame will not be NULL

   // Tell the new segments that they have new geometry
   object->onGeomChanged();
   newObj->onGeomChanged();            
}


// Join two or more sections of wall that have coincident end points.  Will ignore invalid join attempts.
// Will also merge two or more overlapping polygons.
void EditorUserInterface::joinBarrier()
{
   BfObject *joinedObj = NULL;

   GridDatabase *database = getDatabase();

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = 0; i < objList->size()-1; i++)
   {
      BfObject *obj_i = static_cast<BfObject *>(objList->get(i));

      // Will work for both lines and walls, or any future polylines
      if((obj_i->getGeomType() == geomPolyLine) && obj_i->isSelected())  
      {
         joinedObj = doMergeLines(obj_i, i);
         break;
      }
      else if(obj_i->getGeomType() == geomPolygon && obj_i->isSelected())
      {
         joinedObj = doMergePolygons(obj_i, i);
         break;
      }
   }

   if(joinedObj)     // We had a successful merger
   {
      clearSelection(database);
      setNeedToSave(true);
      autoSave();
      joinedObj->onGeomChanged();
      joinedObj->setSelected(true);

      onSelectionChanged();
   }
}


BfObject *EditorUserInterface::doMergePolygons(BfObject *firstItem, S32 firstItemIndex)
{
   Vector<const Vector<Point> *> inputPolygons;
   Vector<Vector<Point> > outputPolygons;
   Vector<S32> deleteList;

   saveUndoState();

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   inputPolygons.push_back(firstItem->getOutline());

   bool cw = isWoundClockwise(*firstItem->getOutline());       // Make sure all our polys are wound the same direction as the first
                                                               
   for(S32 i = firstItemIndex + 1; i < objList->size(); i++)   // Compare against remaining objects
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));
      if(obj->getObjectTypeNumber() == firstItem->getObjectTypeNumber() && obj->isSelected())
      {
         // We can just reverse the winding in place -- if merge succeeds, the poly will be deleted,
         // and if it fails we'll just undo and revert everything to the way it was
         if(isWoundClockwise(*obj->getOutline()) != cw)
            obj->reverseWinding();

         inputPolygons.push_back(obj->getOutline());
         deleteList.push_back(i);
      }
   }

   bool ok = mergePolys(inputPolygons, outputPolygons);

   if(ok && outputPolygons.size() == 1)
   {
      // Clear out the polygon
      while(firstItem->getVertCount())
         firstItem->deleteVert(firstItem->getVertCount() - 1);

      // Add the new points
      for(S32 i = 0; i < outputPolygons[0].size(); i++)
         ok &= firstItem->addVert(outputPolygons[0][i]);    // Will return false if polygon overflows

      if(ok)
      {
         // Delete the constituent parts; work backwards to avoid queering the deleteList indices
         for(S32 i = deleteList.size() - 1; i >= 0; i--)
            deleteItem(deleteList[i]);

         return firstItem;
      }
   }

   undo(false);   // Merge failed for some reason.  Revert everything to undo state saved at the top of method.
   return NULL;
}


BfObject *EditorUserInterface::doMergeLines(BfObject *firstItem, S32 firstItemIndex)
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();
   BfObject *joinedObj = NULL;

   for(S32 i = firstItemIndex + 1; i < objList->size(); i++)              // Compare against remaining objects
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->getObjectTypeNumber() == firstItem->getObjectTypeNumber() && obj->isSelected())
      {
         // Don't join if resulting object would be too big!
         if(firstItem->getVertCount() + obj->getVertCount() > Geometry::MAX_POLY_POINTS)
            continue;

         if(firstItem->getVert(0).distSquared(obj->getVert(0)) < .0001)   // First vertices are the same  1 2 3 | 1 4 5
         {
            if(!joinedObj)          // This is first join candidate found; something's going to merge, so save an undo state
               saveUndoState();
               
            joinedObj = firstItem;

            for(S32 a = 1; a < obj->getVertCount(); a++)           // Skip first vertex, because it would be a dupe
               firstItem->addVertFront(obj->getVert(a));

            deleteItem(i);
            i--;
         }

         // First vertex conincides with final vertex 3 2 1 | 5 4 3
         else if(firstItem->getVert(0).distSquared(obj->getVert(obj->getVertCount() - 1)) < .0001)     
         {
            if(!joinedObj)
               saveUndoState();

            joinedObj = firstItem;
                  
            for(S32 a = obj->getVertCount() - 2; a >= 0; a--)
               firstItem->addVertFront(obj->getVert(a));

            deleteItem(i);    // i has been merged into firstItem; don't need i anymore!
            i--;
         }

         // Last vertex conincides with first 1 2 3 | 3 4 5
         else if(firstItem->getVert(firstItem->getVertCount() - 1).distSquared(obj->getVert(0)) < .0001)     
         {
            if(!joinedObj)
               saveUndoState();

            joinedObj = firstItem;

            for(S32 a = 1; a < obj->getVertCount(); a++)  // Skip first vertex, because it would be a dupe         
               firstItem->addVert(obj->getVert(a));

            deleteItem(i);
            i--;
         }

         // Last vertices coincide  1 2 3 | 5 4 3
         else if(firstItem->getVert(firstItem->getVertCount() - 1).distSquared(obj->getVert(obj->getVertCount() - 1)) < .0001)     
         {
            if(!joinedObj)
               saveUndoState();

            joinedObj = firstItem;

            for(S32 j = obj->getVertCount() - 2; j >= 0; j--)
               firstItem->addVert(obj->getVert(j));

            deleteItem(i);
            i--;
         }
      }
   }

   return joinedObj;
}


void EditorUserInterface::deleteItem(S32 itemIndex, bool batchMode)
{
   GridDatabase *database = getDatabase();
   WallSegmentManager *wallSegmentManager = database->getWallSegmentManager();

   BfObject *obj = static_cast<BfObject *>(getDatabase()->findObjects_fast()->get(itemIndex));

   if(isWallType(obj->getObjectTypeNumber()))
   {
      // Need to recompute boundaries of any intersecting walls
      wallSegmentManager->deleteSegments(obj->getSerialNumber());          // Delete the segments associated with the wall
      database->removeFromDatabase(obj, true);

      if(!batchMode)
         doneDeleteingWalls();
   }
   else
      database->removeFromDatabase(obj, true);

   if(!batchMode)
      doneDeleting();
}


// After deleting a bunch of items, clean up
void EditorUserInterface::doneDeleteingWalls()
{
   WallSegmentManager *wallSegmentManager = mLoadTarget->getWallSegmentManager();

   wallSegmentManager->recomputeAllWallGeometry(mLoadTarget);   // Recompute wall edges
   resnapAllEngineeredItems(mLoadTarget);         
}


void EditorUserInterface::doneDeleting()
{
   // Reset a bunch of things
   clearSnapEnvironment();
   validateLevel();
   onMouseMoved();   // Reset cursor  
}


// Only called when user presses a hotkey to insert an item -- may crash if item to be inserted is not currently on the dock!
void EditorUserInterface::insertNewItem(U8 itemTypeNumber)
{
   if(mDraggingObjects)     // No inserting when items are being dragged!
      return;

   GridDatabase *database = getDatabase();

   clearSelection(database);
   saveUndoState();

   BfObject *newObject = NULL;

   // Find a dockItem to copy
   for(S32 i = 0; i < mDockItems.size(); i++)
      if(mDockItems[i]->getObjectTypeNumber() == itemTypeNumber)
      {
         newObject = copyDockItem(mDockItems[i].get());
         break;
      }

   // May occur if requested item is not currently on the dock
   TNLAssert(newObject, "Couldn't create object in insertNewItem()");
   if(!newObject)
      return;


   newObject->moveTo(snapPoint(database, convertCanvasToLevelCoord(mMousePos)));
   addToEditor(newObject);    
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
//   const Vector<BfObject *> *objList = getDatabase()->getObjectList();
//   const Vector<BfObject *> *levelGenObjList = mLevelGenDatabase.getObjectList();

   Rect extents = getDatabase()->getExtents();
   extents.unionRect(mLevelGenDatabase.getExtents());


   //if(objList->size() || levelGenObjList->size())
   //{
   //   F32 minx =  F32_MAX,   miny =  F32_MAX;
   //   F32 maxx = -F32_MAX,   maxy = -F32_MAX;

   //   for(S32 i = 0; i < objList->size(); i++)
   //   {
   //      EditorObject *obj = objList->get(i);

   //      for(S32 j = 0; j < obj->getVertCount(); j++)
   //      {
   //         if(obj->getVert(j).x < minx)
   //            minx = obj->getVert(j).x;
   //         if(obj->getVert(j).x > maxx)
   //            maxx = obj->getVert(j).x;
   //         if(obj->getVert(j).y < miny)
   //            miny = obj->getVert(j).y;
   //         if(obj->getVert(j).y > maxy)
   //            maxy = obj->getVert(j).y;
   //      }
   //   }

   //   for(S32 i = 0; i < levelGenObjList->size(); i++)
   //   {
   //      EditorObject *obj = levelGenObjList->get(i);

   //      for(S32 j = 0; j < obj->getVertCount(); j++)
   //      {
   //         if(obj->getVert(j).x < minx)
   //            minx = obj->getVert(j).x;
   //         if(obj->getVert(j).x > maxx)
   //            maxx = obj->getVert(j).x;
   //         if(obj->getVert(j).y < miny)
   //            miny = obj->getVert(j).y;
   //         if(obj->getVert(j).y > maxy)
   //            maxy = obj->getVert(j).y;
   //      }
   //   }


   F32 x = extents.getCenter().x;
   F32 y = extents.getCenter().y;

   // If we have nothing, or maybe only one point object in our level
   if(extents.getWidth() < 1 && extents.getHeight() < 1)    // e.g. a single point item
   {
      mCurrentScale = STARTING_SCALE;
      mCurrentOffset.set(gScreenInfo.getGameCanvasWidth()  / 2 - mCurrentScale * x, 
                         gScreenInfo.getGameCanvasHeight() / 2 - mCurrentScale * y);
   }
   else
   {
      mCurrentScale = min(gScreenInfo.getGameCanvasWidth()  / extents.getWidth(), 
                          gScreenInfo.getGameCanvasHeight() / extents.getHeight());

      mCurrentScale /= 1.3f;      // Zoom out a bit

      mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2  - mCurrentScale * x, 
                           gScreenInfo.getGameCanvasHeight() / 2 - mCurrentScale * y);
   }
   //}
   //else     // Put (0,0) at center of screen
   //{
   //   mCurrentScale = STARTING_SCALE;
   //   mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2, gScreenInfo.getGameCanvasHeight() / 2);
   //}
}


F32 EditorUserInterface::getCurrentScale()
{
   return mCurrentScale;
}


Point EditorUserInterface::getCurrentOffset()
{
   return mCurrentOffset;
}


// Gets run when user exits special-item editing mode, called from attribute editors
void EditorUserInterface::doneEditingAttributes(EditorAttributeMenuUI *editor, BfObject *object)
{
   object->onAttrsChanged();

   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   // Find any other selected items of the same type of the item we just edited, and update their attributes too
   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj != object && obj->isSelected() && obj->getObjectTypeNumber() == object->getObjectTypeNumber())
      {
         editor->doneEditingAttrs(obj);  // Transfer attributes from editor to object
         obj->onAttrsChanged();          // And notify the object that its attributes have changed
      }
   }
}


void EditorUserInterface::zoom(F32 zoomAmount)
{
   Point mouseLevelPoint = convertCanvasToLevelCoord(mMousePos);

   mCurrentScale *= 1 + zoomAmount;

   if(mCurrentScale < MIN_SCALE)
      mCurrentScale = MIN_SCALE;
   else if(mCurrentScale > MAX_SCALE)
      mCurrentScale = MAX_SCALE;
   
   Point newMousePoint = convertLevelToCanvasCoord(mouseLevelPoint);

   mCurrentOffset += mMousePos - newMousePoint;
}


void EditorUserInterface::onTextInput(char ascii)
{
   // Pass the key on to the console for processing
   if(gConsole.onKeyDown(ascii))
      return;

   if(entryMode != EntryNone)
      textEntryTextInputHandler(ascii);

   else if(ascii == '#' || ascii == '!')
   {
      S32 selected = NONE;

      const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

      // Find first selected item, and work with that.  Unselect the rest.
      for(S32 i = 0; i < objList->size(); i++)
      {
         BfObject *obj = static_cast<BfObject *>(objList->get(i));

         if(obj->isSelected())
         {
            if(selected == NONE)
            {
               selected = i;
               continue;
            }
            else
               obj->setSelected(false);
         }
      }

      onSelectionChanged();

      if(selected == NONE)      // Nothing selected, nothing to do!
         return ;

      BfObject *selectedObj = static_cast<BfObject *>(objList->get(selected));

      S32 id = selectedObj->getUserAssignedId();
      mEntryBox = getNewEntryBox( id <= 0 ? "" : itos(id), "Item ID:", 10, LineEditor::digitsOnlyFilter);
      entryMode = EntryID;
   }
}


// Handle key presses
bool EditorUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;

   if(gConsole.onKeyDown(inputCode))      // Pass the key on to the console for processing
      return true;

   // If console is open, then we want to capture text, so return false
   if(gConsole.isVisible())
      return false;

   string inputString = InputCodeManager::getCurrentInputString(inputCode);


   // TODO: Make this stuff work like the attribute entry stuff; use a real menu and not this ad-hoc code
   // This is where we handle entering things like rotation angle and other data that requires a special entry box.
   // NOT for editing an item's attributes.  Still used, but untested in refactor.
   if(entryMode != EntryNone)
      return textEntryInputCodeHandler(inputCode);

   else if(inputCode == KEY_ENTER || inputCode == KEY_KEYPAD_ENTER)       // Enter - Edit props
      startAttributeEditor();

   // Mouse wheel zooms in and out

   else if(inputCode == MOUSE_WHEEL_UP)
      zoom(0.2f);
   else if(inputCode == MOUSE_WHEEL_DOWN)
      zoom(-0.2f);
   else if(inputCode == MOUSE_MIDDLE)     // Click wheel to drag
   {
      mScrollWithMouseLocation = mMousePos;
      mAutoScrollWithMouseReady = !mAutoScrollWithMouse; // Ready to scroll when button is released
      mAutoScrollWithMouse = false;  // turn off in case we were already auto scrolling.
   }

   // Regular key handling from here on down
   else if(InputCodeManager::checkModifier(KEY_SHIFT) && inputCode == KEY_0)  // Shift-0 -> Set team to hostile
      setCurrentTeam(-2);
   else if(inputCode >= KEY_0 && inputCode <= KEY_9 && InputCodeManager::checkModifier(KEY_NONE))  // Change team affiliation of selection with 0-9 keys
   {
      setCurrentTeam((inputCode - KEY_0) - 1);
      return true;
   }

#ifdef TNL_OS_MAC_OSX 
   // Ctrl-left click is same as right click for Mac users
   else if(inputCode == MOUSE_RIGHT || (inputCode == MOUSE_LEFT && InputCodeManager::checkModifier(KEY_CTRL)))
#else
   else if(inputCode == MOUSE_RIGHT)
#endif
      onMouseClicked_right();

   else if(inputCode == MOUSE_LEFT)
      onMouseClicked_left();

   // Neither mouse button, let's try some keys
   else if(inputString == "D"|| inputString == "Shift+D")            // Pan right
      mRight = true;
   else if(inputString == "Right Arrow")  // Pan right
      mRight = true;
   else if(inputString == "H")            // Flip horizontal
      flipSelectionHorizontal();
   else if(inputString == "Ctrl+V")       // Paste selection
      pasteSelection();
   else if(inputString == "V")            // Flip vertical
      flipSelectionVertical();
   else if(inputString == "/" || inputString == "Keypad /")
   {
      if(gConsole.isOk())
         gConsole.show();
      //else do what???
   }
   else if(inputString == "Ctrl+Shift+L") // Reload level
   {
      loadLevel();                        
      setSaveMessage("Reloaded " + getLevelFileName(), true);
   }
   else if(inputString == "Ctrl+Shift+Z") // Redo
   {
      if(!mCreatingPolyline && !mCreatingPoly && !mDraggingObjects && !mDraggingDockItem)
         redo();
   }
   else if(inputString == "Ctrl+Z")       // Undo
   {
      if(!mCreatingPolyline && !mCreatingPoly && !mDraggingObjects && !mDraggingDockItem)
         undo(true);
   }
   else if(inputString == "Z")            // Reset veiw
      centerView();
   else if(inputString == "Ctrl+Shift+R") // Rotate by arbitrary amount
   {
      if(!anyItemsSelected(getDatabase()))
         return true;

      mEntryBox = getNewEntryBox("", "Rotation angle:", 10, LineEditor::numericFilter);
      entryMode = EntryAngle;
   }
   else if(inputString == "Ctrl+R")       // Run levelgen script, or clear last results
   {
      // Ctrl+R is a toggle -- we either add items or clear them
      if(mLevelGenDatabase.getObjectCount() == 0)
         runLevelGenScript();
      else
         clearLevelGenItems();
   }
   else if(inputString == "R")            // Rotate CCW
      rotateSelection(-15.f); 
   else if(inputString == "Shift+R")      // Rotate CW
      rotateSelection(15.f); 

   else if(inputString == "Ctrl+I")       // Insert items generated with script into editor
      copyScriptItemsToEditor();

   else if(inputString == "Up Arrow" || inputString == "W" || inputString == "Shift+W")  // W or Up - Pan up
      mUp = true;
   else if(inputString == "Ctrl+Up Arrow")      // Zoom in
      mIn = true;
   else if(inputString == "Ctrl+Down Arrow")    // Zoom out
      mOut = true;
   else if(inputString == "Down Arrow")         // Pan down
      mDown = true;
   else if(inputString == "Ctrl+S")             // Save
      saveLevel(true, true);
   else if(inputString == "S"|| inputString == "Shift+S")                  // Pan down
      mDown = true;
   else if(inputString == "Left Arrow" || inputString == "A"|| inputString == "Shift+A")   // Left or A - Pan left
      mLeft = true;
   else if(inputString == "Shift+=" || inputString == "Shift+Keypad +")    // Shifted - Increase barrier width by 1
      changeBarrierWidth(1);
   else if(inputString == "=" || inputString == "Keypad +")                // Unshifted + --> by 5
      changeBarrierWidth(5);
   else if(inputString == "Shift+-" || inputString == "Shift+Keypad -")    // Shifted - Decrease barrier width by 1
      changeBarrierWidth(-1);
   else if(inputString == "-" || inputString == "Keypad -")                // Unshifted --> by 5
      changeBarrierWidth(-5);
   else if(inputString == "E")            // Zoom In
      mIn = true;
   else if(inputString == "\\")           // Split barrier on selected vertex
      splitBarrier();
   else if(inputString == "J")            // Join selected barrier segments or polygons
      joinBarrier();
   else if(inputString == "Ctrl+A")       // Select everything
      selectAll(getDatabase());
   else if(inputString == "Ctrl+Shift+X") // Resize selection
   {
      if(anyItemsSelected(getDatabase()))
      {
         mEntryBox = getNewEntryBox("", "Resize factor:", 10, LineEditor::numericFilter);
         entryMode = EntryScale;
      }
   }
   else if(inputString == "Ctrl+X")     // Cut selection
   {
      copySelection();
      deleteSelection(true);
   }
   else if(inputString == "Ctrl+C")    // Copy selection to clipboard
      copySelection();
   else if(inputString == "C")         // Zoom out
      mOut = true;
   else if(inputString == "F3")        // Level Parameter Editor
   {
      getUIManager()->activate(GameParamsUI);
      playBoop();
   }
   else if(inputString == "F2")               // Team Editor Menu
   {
      getUIManager()->activate(TeamDefUI);
      playBoop();
   }
   else if(inputString == "T")                // Teleporter
      insertNewItem(TeleporterTypeNumber);
   else if(inputString == "P")                // SpeedZone
      insertNewItem(SpeedZoneTypeNumber);
   else if(inputString == "G")                // Spawn
      insertNewItem(ShipSpawnTypeNumber);
   else if(inputString == "Ctrl+B")           // Spybug
      insertNewItem(SpyBugTypeNumber);
   else if(inputString == "B")                // Repair
      insertNewItem(RepairItemTypeNumber);
   else if(inputString == "Y")                // Turret
      insertNewItem(TurretTypeNumber);
   else if(inputString == "M")                // Mine
      insertNewItem(MineTypeNumber);
   else if(inputString == "F")                // Forcefield
      insertNewItem(ForceFieldProjectorTypeNumber);
   else if(inputString == "Backspace" || inputString == "Del" || inputString == "Keypad .")     // Keypad . is the keypad's del key
      deleteSelection(false);
   else if(checkInputCode(getGame()->getSettings(), InputCodeManager::BINDING_HELP, inputCode)) // Turn on help screen
   {
      getGame()->getUIManager()->activate(EditorInstructionsUI);
      playBoop();
   }
   else if(inputCode == KEY_ESCAPE)          // Activate the menu
   {
      playBoop();
      getGame()->getUIManager()->activate(EditorMenuUI);
   }
   else if(inputString == "Space")           // No snapping to grid, but still to other things
      mSnapContext = NO_GRID_SNAPPING;
   else if(inputString == "Shift+Space")     // Completely disable snapping
      mSnapContext = NO_SNAPPING;
   else if(inputString == "Tab")             // Turn on preview mode
      mPreviewMode = true;
   else if(checkPluginKeyBindings(inputString))
   {
      // Do nothing
   }
   else
      return false;

   // A key was handled
   return true;
}


void EditorUserInterface::onMouseClicked_left()
{
   if(InputCodeManager::getState(MOUSE_RIGHT))  // Prevent weirdness
      return;

   mDraggingDockItem = NULL;
   mMousePos.set(gScreenInfo.getMousePos());
   mJustInsertedVertex = false;

   if(mCreatingPoly || mCreatingPolyline)       // Save any polygon/polyline we might be creating
   {
      if(mNewItem->getVertCount() < 2)
      {
         delete mNewItem.getPointer();
         removeUndoState();
      }
      else
         addToEditor(mNewItem);

      mNewItem = NULL;

      mCreatingPoly = false;
      mCreatingPolyline = false;
   }

   mMouseDownPos = convertCanvasToLevelCoord(mMousePos);

   if(mouseOnDock())    // On the dock?  Did we hit something to start dragging off the dock?
   {
      clearSelection(getDatabase());
      mDraggingDockItem = mDockItemHit;      // Could be NULL

      if(mDraggingDockItem)
         SDL_SetCursor(Cursor::getSpray());
   }
   else                 // Mouse is not on dock
   {
      mDraggingDockItem = NULL;
      SDL_SetCursor(Cursor::getDefault());

      // rules for mouse down:
      // if the click has no shift- modifier, then
      //   if the click was on something that was selected
      //     do nothing
      //   else
      //     clear the selection
      //     add what was clicked to the selection
      //  else
      //    toggle the selection of what was clicked

      if(InputCodeManager::checkModifier(KEY_SHIFT))  // ==> Shift key is down
      {
         // Check for vertices
         if(mHitItem && mHitVertex != NONE && mHitItem->getGeomType() != geomPoint)
         {
            if(mHitItem->vertSelected(mHitVertex))
               mHitItem->unselectVert(mHitVertex);
            else
               mHitItem->aselectVert(mHitVertex);
         }
         else if(mHitItem)
         {
            mHitItem->setSelected(!mHitItem->isSelected());    // Toggle selection of hit item
            onSelectionChanged();
         }
         else
            mDragSelecting = true;
      }
      else                                            // ==> Shift key is NOT down
      {
         // If we hit a vertex of an already selected item --> now we can move that vertex w/o losing our selection.
         // Note that in the case of a point item, we want to skip this step, as we don't select individual vertices.
         if(mHitVertex != NONE && mHitItem && mHitItem->isSelected() && mHitItem->getGeomType() != geomPoint)
         {
            clearSelection(getDatabase());
            mHitItem->selectVert(mHitVertex);
            onSelectionChanged();
         }

         if(mHitItem && mHitItem->isSelected())    // Hit an already selected item
         {
            // Do nothing so user can drag a group of items that's already been selected
         }
         else if(mHitItem && mHitItem->getGeomType() == geomPoint)  // Hit a point item
         {
            clearSelection(getDatabase());
            mHitItem->setSelected(true);
            onSelectionChanged();
         }
         else if(mHitVertex != NONE && (mHitItem && !mHitItem->isSelected()))      // Hit a vertex of an unselected item
         {        // (braces required)
            if(!(mHitItem->vertSelected(mHitVertex)))
            {
               clearSelection(getDatabase());
               mHitItem->selectVert(mHitVertex);
               onSelectionChanged();
            }
         }
         else if(mHitItem)                                                          // Hit a non-point item, but not a vertex
         {
            clearSelection(getDatabase());
            mHitItem->setSelected(true);
            onSelectionChanged();
         }
         else     // Clicked off in space.  Starting to draw a bounding rectangle?
         {
            mDragSelecting = true;
            clearSelection(getDatabase());
            onSelectionChanged();
         }
      }
   }     // end mouse not on dock block, doc

   findSnapVertex();     // Update snap vertex in the event an item was selected
}


void EditorUserInterface::onMouseClicked_right()
{
   if(InputCodeManager::getState(MOUSE_LEFT) && !InputCodeManager::checkModifier(KEY_CTRL))  // Prevent weirdness
      return;  

   mMousePos.set(gScreenInfo.getMousePos());

   if(mCreatingPoly || mCreatingPolyline)
   {
      if(mNewItem->getVertCount() < Geometry::MAX_POLY_POINTS)    // Limit number of points in a polygon/polyline
      {
         mNewItem->addVert(snapPoint(getDatabase(), convertCanvasToLevelCoord(mMousePos)));
         mNewItem->onGeomChanging();
      }
         
      return;
   }

   saveUndoState(true);             // Save undo state before we clear the selection

   clearSelection(getDatabase());   // Unselect anything currently selected
   onSelectionChanged();

   // Can only add new vertices by clicking on item's edge, not it's interior (for polygons, that is)
   if(mEdgeHit != NONE && mHitItem && (mHitItem->getGeomType() == geomPolyLine || mHitItem->getGeomType() >= geomPolygon))
   {
      if(mHitItem->getVertCount() >= Geometry::MAX_POLY_POINTS)     // Polygon full -- can't add more
         return;

      Point newVertex = snapPoint(getDatabase(), convertCanvasToLevelCoord(mMousePos));   // adding vertex w/ right-mouse

      mAddingVertex = true;

      // Insert an extra vertex at the mouse clicked point, and then select it
      mHitItem->insertVert(newVertex, mEdgeHit + 1);
      mHitItem->selectVert(mEdgeHit + 1);
      mJustInsertedVertex = true;

      // Alert the item that its geometry is changing -- needed by polygons so they can recompute their fill
      mHitItem->onGeomChanging();

      // The user might just insert a vertex and be done; in that case we'll need to rebuild the wall outlines to account
      // for the new vertex.  If the user continues to drag the vertex to a new location, this will be wasted effort...
      mHitItem->onGeomChanged();


      mMouseDownPos = newVertex;
   }
   else     // Start creating a new poly or new polyline (tilde key + right-click ==> start polyline)
   { 
      if(InputCodeManager::getState(KEY_BACKQUOTE))   // Tilde  
      {
         mCreatingPolyline = true;
         mNewItem = new LineItem();
      }
      else
      {
         mCreatingPoly = true;
         mNewItem = new WallItem();
      }

      mNewItem->initializeEditor();
      mNewItem->setTeam(mCurrentTeam);
      mNewItem->addVert(snapPoint(getDatabase(), convertCanvasToLevelCoord(mMousePos)));
   }
}


// Returns true if key was handled, false if not
bool EditorUserInterface::checkPluginKeyBindings(string inputString)
{
   GameSettings *settings = getGame()->getSettings();
   const Vector<PluginBinding> *bindings = settings->getPluginBindings(); 

   for(S32 i = 0; i < bindings->size(); i++)
      if(inputString == bindings->get(i).key)
      {
         runPlugin(settings->getFolderManager(), bindings->get(i).script, Vector<string>());
         return true;
      }

   return false;
}


void EditorUserInterface::textEntryTextInputHandler(char ascii)
{
   mEntryBox.addChar(ascii);
}


// Handle keyboard activity when we're editing an item's attributes
bool EditorUserInterface::textEntryInputCodeHandler(InputCode inputCode)
{
   if(inputCode == KEY_ENTER || inputCode == KEY_KEYPAD_ENTER)
   {
      if(entryMode == EntryID)
      {
         const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

         for(S32 i = 0; i < objList->size(); i++)
         {
            BfObject *obj = static_cast<BfObject *>(objList->get(i));

            if(obj->isSelected())               // Should only be one
            {
               S32 id = atoi(mEntryBox.c_str());
               if(obj->getUserAssignedId() != id)     // Did the id actually change?
               {
                  obj->setUserAssignedId(id, true);
                  mAllUndoneUndoLevel = -1;     // If so, it can't be undone
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
      return true;
   }
   else if(inputCode == KEY_ESCAPE)
   {
      entryMode = EntryNone;
      return true;
   }
   else if(inputCode == KEY_BACKSPACE || inputCode == KEY_DELETE)
   {
      mEntryBox.handleBackspace(inputCode);
      return true;
   }

   // Else ignore keystroke
   return false;
}


void EditorUserInterface::startAttributeEditor()
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj_i = static_cast<BfObject *>(objList->get(i));

      if(obj_i->isSelected())
      {
         // Force item i to be the one and only selected item type.  This will clear up some problems that might otherwise
         // occur if you had different item types selected while you were editing attributes.   If you have multiple
         // items selected, all will end up with the same values, which only make sense if they are the same kind
         // of object.  So after this runs, there may be multiple items selected, but they'll all  be the same type.
         for(S32 j = 0; j < objList->size(); j++)
         {
            BfObject *obj_j = static_cast<BfObject *>(objList->get(j));

            if(obj_j->isSelected() && obj_j->getObjectTypeNumber() != obj_i->getObjectTypeNumber())
               obj_j->unselect();
         }

         // Activate the attribute editor if there is one
         EditorAttributeMenuUI *menu = mEditorAttributeMenuItemBuilder.getAttributeMenu(obj_i);
         if(menu)
         {
            menu->startEditingAttrs(obj_i);
            getUIManager()->activate(menu);

            saveUndoState();
         }

         break;
      }
   }
}


void EditorUserInterface::onKeyUp(InputCode inputCode)
{
   switch(inputCode)
   {
      case KEY_UP:
         mIn = false;
         mUp = false;
         break;
      case KEY_W:
         mUp = false;
         break;
      case KEY_DOWN:
         mOut = false;
         mDown = false;
         break;
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
         mSnapContext = FULL_SNAPPING;
         break;
      case KEY_SHIFT:
         // Check if space is down... if so, modify snapping accordingly
         // This is a little special-casey, but it is, after all, a special case.
         if(InputCodeManager::getState(KEY_SPACE) && mSnapContext == NO_SNAPPING)
            mSnapContext = NO_GRID_SNAPPING;
         break;

      case KEY_TAB:
         mPreviewMode = false;
         break;
      case MOUSE_MIDDLE:
         mAutoScrollWithMouse = mAutoScrollWithMouseReady;
         break;
      case MOUSE_LEFT:
      case MOUSE_RIGHT:  
         mMousePos.set(gScreenInfo.getMousePos());

         if(mDragSelecting)      // We were drawing a rubberband selection box
         {
            Rect r(convertCanvasToLevelCoord(mMousePos), mMouseDownPos);

            fillVector.clear();

            getDatabase()->findObjects(fillVector);


            for(S32 i = 0; i < fillVector.size(); i++)
            {
               BfObject *obj = dynamic_cast<BfObject *>(fillVector[i]);

               // Make sure that all vertices of an item are inside the selection box; basically means that the entire 
               // item needs to be surrounded to be included in the selection
               S32 j;

               for(j = 0; j < obj->getVertCount(); j++)
                  if(!r.contains(obj->getVert(j)))
                     break;
               if(j == obj->getVertCount())
                  obj->setSelected(true);
            }
            mDragSelecting = false;
            onSelectionChanged();
         }

         else if(mDraggingObjects || mAddingVertex)     // We were dragging and dropping.  Could have been a move or a delete (by dragging to dock).
         {
            if(mAddingVertex)
            {
               //deleteUndoState();
               mAddingVertex = false;
            }
            
            onFinishedDragging();
         }

         break;

      default:
         break;
   }     // case
}


// Called when user has been dragging an object and then releases it
void EditorUserInterface::onFinishedDragging()
{
   mDraggingObjects = false;
   SDL_SetCursor(Cursor::getDefault());

   // Dragged item off the dock, then back on  ==> nothing changed; restore to unmoved state, which was stored on undo stack
   if(mouseOnDock() && mDraggingDockItem != NULL)
   {
      undo(false);
      return;
   }

   // Mouse is over the dock and we dragged something to the dock (probably a delete)
   if(mouseOnDock() && !mDraggingDockItem)
   {
      const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();
      bool deletedSomething = false, deletedWall = false;

      for(S32 i = 0; i < objList->size(); i++)    //  Delete all selected items
      {
         BfObject *obj = static_cast<BfObject *>(objList->get(i));

         if(obj->isSelected())
         {
            if(isWallType(obj->getObjectTypeNumber()))
               deletedWall = true;

            deleteItem(i, true);
            i--;
            deletedSomething = true;
         }
      }

      // We deleted something, do some clean up and our job is done
      if(deletedSomething)
      {
         if(deletedWall)
            doneDeleteingWalls();

         doneDeleting();

         return;
      }
   }

   // Mouse not on dock, we are either:
   // 1. dragging from the dock,
   // 2. moving something,
   // 3. or we moved something to the dock and nothing was deleted, e.g. when dragging a vertex
   // need to save an undo state if anything changed
   if(mDraggingDockItem == NULL)    // Not dragging from dock - user is moving object around screen, or dragging vertex to dock
   {
      // If our snap vertex has moved then all selected items have moved
      bool itemsMoved = mDragCopying || (mSnapObject && mSnapObject->getVert(mSnapVertexIndex) != mMoveOrigin);

      if(itemsMoved)    // Move consumated... update any moved items, and save our autosave
      {
         const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

         for(S32 i = 0; i < objList->size(); i++)
         {
            BfObject *obj = static_cast<BfObject *>(objList->get(i));

            if(obj->isSelected() || objList->get(i)->anyVertsSelected())
               obj->onGeomChanged();
         }

         setNeedToSave(true);
         autoSave();

         mDragCopying = false;

         return;
      }
      else if(!mJustInsertedVertex)    // We started our move, then didn't end up moving anything... remove associated undo state
         deleteUndoState();
      else
         mJustInsertedVertex = false;
   }
}


bool EditorUserInterface::mouseOnDock()
{
   return (mMousePos.x >= gScreenInfo.getGameCanvasWidth() - DOCK_WIDTH - horizMargin &&
           mMousePos.x <= gScreenInfo.getGameCanvasWidth() - horizMargin &&
           mMousePos.y >= gScreenInfo.getGameCanvasHeight() - vertMargin - getDockHeight() &&
           mMousePos.y <= gScreenInfo.getGameCanvasHeight() - vertMargin);
}


S32 EditorUserInterface::getItemSelectedCount()
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   S32 count = 0;

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));

      if(obj->isSelected())
         count++;
   }

   return count;
}


bool EditorUserInterface::anythingSelected()
{
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   for(S32 i = 0; i < objList->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objList->get(i));
      if(obj->isSelected() || obj->anyVertsSelected() )
         return true;
   }

   return false;
}


void EditorUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   F32 pixelsToScroll = timeDelta * (InputCodeManager::getState(KEY_SHIFT) ? 1.0f : 0.5f);    // Double speed when shift held down

   if(mLeft && !mRight)
      mCurrentOffset.x += pixelsToScroll;
   else if(mRight && !mLeft)
      mCurrentOffset.x -= pixelsToScroll;
   if(mUp && !mDown)
      mCurrentOffset.y += pixelsToScroll;
   else if(mDown && !mUp)
      mCurrentOffset.y -= pixelsToScroll;

   if(mAutoScrollWithMouse)
   {
      mCurrentOffset += (mScrollWithMouseLocation - mMousePos) * pixelsToScroll / 128.f;
      onMouseMoved();  // Prevents skippy problem while dragging something
   }

   if(mIn && !mOut)
      zoom(timeDelta * 0.002f);
   else if(mOut && !mIn)
      zoom(timeDelta * -0.002f);

   mSaveMsgTimer.update(timeDelta);
   mWarnMsgTimer.update(timeDelta);
}


void EditorUserInterface::setSaveMessage(string msg, bool savedOK)
{
   mSaveMsg = msg;
   mSaveMsgTimer.reset(4000, 4000);    // Display for 4 seconds
   mSaveMsgColor = (savedOK ? Colors::green : Colors::red);
}


void EditorUserInterface::setWarnMessage(string msg1, string msg2)
{
   mWarnMsg1 = msg1;
   mWarnMsg2 = msg2;
   mWarnMsgTimer.reset(4000, 4000);    // Display for 4 seconds
   mWarnMsgColor = gErrorMessageTextColor;
}


void EditorUserInterface::autoSave()
{
   doSaveLevel("auto.save", false);
}


bool EditorUserInterface::saveLevel(bool showFailMessages, bool showSuccessMessages)
{
   // Check if we have a valid (i.e. non-null) filename ==> should never happen!
   if(mEditFileName == "")
   {
      ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

      ui->reset();
      ui->setTitle("INVALID FILE NAME");
      ui->setMessage(1, "The level file name is invalid or empty.  The level cannot be saved.");
      ui->setMessage(2, "To correct the problem, please change the file name using the");
      ui->setMessage(3, "Game Parameters menu, which you can access by pressing [F3].");

      getUIManager()->activate(ui);

      return false;
   }

   if(!doSaveLevel(mEditFileName, showFailMessages))
      return false;

   mNeedToSave = false;
   mAllUndoneUndoLevel = mLastUndoIndex;     // If we undo to this point, we won't need to save

   if(showSuccessMessages)
      setSaveMessage("Saved " + getLevelFileName(), true);

   return true;
}


string EditorUserInterface::getLevelText() 
{
   string result;

   // Write out basic game parameters, including gameType info
   result += getGame()->toLevelCode();    // Note that this toLevelCode appends a newline char; most don't

   const Vector<string> *robots = getGame()->getLevelRobotLines();

   // Next come the robots
   for(S32 i = 0; i < robots->size(); i++)
      result += robots->get(i);

   // Write out all level items (do two passes; walls first, non-walls next, so turrets & forcefields have something to grab onto)
   const Vector<DatabaseObject *> *objList = getDatabase()->findObjects_fast();

   F32 gridSize = getGame()->getGridSize();

   for(S32 j = 0; j < 2; j++)
   {
      for(S32 i = 0; i < objList->size(); i++)
      {
         BfObject *obj = static_cast<BfObject *>(objList->get(i));

         // Writing wall items on first pass, non-wall items next -- that will make sure mountable items have something to grab onto
         if((j == 0 && isWallType(obj->getObjectTypeNumber())) || (j == 1 && ! isWallType(obj->getObjectTypeNumber())) )
            result += obj->toLevelCode(gridSize) + "\n";
      }
   }

   return result;
}


// Returns true if successful, false otherwise
bool EditorUserInterface::doSaveLevel(const string &saveName, bool showFailMessages)
{
   try
   {
      FolderManager *folderManager = getGame()->getSettings()->getFolderManager();

      string fileName = joindir(folderManager->levelDir, saveName);
      if(!writeFile(fileName, getLevelText()))
         throw(SaveException("Could not open file for writing"));
   }
   catch (SaveException &e)
   {
      if(showFailMessages)
         setSaveMessage("Error Saving: " + string(e.what()), false);
      return false;
   }

   return true;      // Saved OK
}


// We need some local hook into the testLevelStart() below.  Ugly but apparently necessary.
void testLevelStart_local(ClientGame *game)
{
   game->getUIManager()->getEditorUserInterface()->testLevelStart();
}


extern void initHostGame(GameSettings *settings, const Vector<string> &levelList, bool testMode, bool dedicatedServer);

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

      getUIManager()->activate(ui);

      return;
   }

   testLevelStart();
}


void EditorUserInterface::testLevelStart()
{
   static const string TestFileName = "editor.tmp";   // Temp file where we'll save current level while testing

   Cursor::disableCursor();                           // Turn off cursor

   mEditorGameType = getGame()->getGameType();        // Sock our current gametype away, will use it when we reenter the editor

   if(!doSaveLevel(TestFileName, true))
      getGame()->getUIManager()->reactivatePrevUI();  // Saving failed, can't test, reactivate editor
   else
   {
      mWasTesting = true;

      if(getGame()->idlingObjects.nextList)           // to avoid Asteroids and a few other objects getting moved in editor...
         getGame()->idlingObjects.unlinkFromIdleList();

      Vector<string> levelList;
      levelList.push_back(TestFileName);
      initHostGame(getGame()->getSettings(), levelList, true, false);
   }
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


extern MenuItem *getWindowModeMenuItem(U32 displayMode);

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
   game->getUIManager()->activate(EditorInstructionsUI);
}


static void activateLevelParamsCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(GameParamsUI);
}


static void activateTeamDefCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate(TeamDefUI);
}

void uploadToDbCallback(ClientGame *game, U32 unused)
{
   static Thread* uploadThread;
   uploadThread = new LevelDatabaseUploadThread(game);
   uploadThread->start();
   game->getUIManager()->reactivatePrevUI();
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
      ui->setMessage(2, "You have not saved your changes to this level.");
      ui->setMessage(4, "Do you want to?");
      ui->registerYesFunction(saveLevelCallback);
      ui->registerNoFunction(backToMainMenuCallback);

      game->getUIManager()->activate(ui);
   }
   else
     backToMainMenuCallback(game);
}

//////////

void EditorMenuUserInterface::setupMenus()
{
   GameSettings *settings = getGame()->getSettings();
   clearMenuItems();
   addMenuItem(new MenuItem("RETURN TO EDITOR", reactivatePrevUICallback,    "", KEY_R));
   addMenuItem(getWindowModeMenuItem((U32)settings->getIniSettings()->displayMode));
   addMenuItem(new MenuItem("TEST LEVEL",       testLevelCallback,           "", KEY_T));
   addMenuItem(new MenuItem("SAVE LEVEL",       returnToEditorCallback,      "", KEY_S));
   addMenuItem(new MenuItem("INSTRUCTIONS",     activateHelpCallback,        "", KEY_I, getInputCode(settings, InputCodeManager::BINDING_HELP)));
   addMenuItem(new MenuItem("LEVEL PARAMETERS", activateLevelParamsCallback, "", KEY_L, KEY_F3));
   addMenuItem(new MenuItem("MANAGE TEAMS",     activateTeamDefCallback,     "", KEY_M, KEY_F2));
   addMenuItem(new MenuItem("UPLOAD TO DB",     uploadToDbCallback,          "", KEY_U));
   addMenuItem(new MenuItem("QUIT",             quitEditorCallback,          "", KEY_Q, KEY_UNKNOWN));
}


void EditorMenuUserInterface::onEscape()
{
   Cursor::disableCursor();
   getUIManager()->reactivatePrevUI();
}


};

