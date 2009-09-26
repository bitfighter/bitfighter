// Level Info:Game Type:
/* Done

*/
/* test
Test various junk in level files and see how they load into the editor, and how they play
*/

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
#include "gameType.h"
#include "engineeredObjects.h"   // For Turret properties
#include "barrier.h"             // For BarrierWidth
#include "speedZone.h"           // For default speed
#include "gameItems.h"           // For Asteroid defs
#include "teleporter.h"          // For teleporter radius
#include "config.h"
#include "SweptEllipsoid.h"
#include "textItem.h"            // For MAX_TEXTITEM_LEN and MAX_TEXT_SIZE

#include "../glut/glutInclude.h"
#include <ctype.h>
#include <exception>

namespace Zap
{

EditorUserInterface gEditorUserInterface;

const U32 dockWidth = 50;

// Some colors

extern Color gNexusOpenColor;
static const Color inactiveSpecialAttributeColor = Color(.6, .6, .6);
static const Color white = Color(1, 1, 1);
static const Color red = Color(1, 0, 0);
static const Color yellow = Color(1, 1, 0);


void saveLevelCallback()
{
   gEditorUserInterface.saveLevel(true, true);
   UserInterface::reactivateMenu(gMainMenuUserInterface);
}

void backToMainMenuCallback()
{
   UserInterface::reactivateMenu(gMainMenuUserInterface);
}

//S32 gBoxesH;
//S32 gBoxesV;

// Constructor
EditorUserInterface::EditorUserInterface()
{
   setMenuID(EditorUI);

   // Create some items for the dock...  One of each, please!
   showAllObjects = true;
   mWasTesting = false;

   //MeshBox::init();

   //gBoxesH = 20;
   //gBoxesV = 20;

   //for(S32 i = 0; i < gBoxesH; i++)
   //   for(S32 j = 0; j < gBoxesV; j++)
   //      testBox[i][j] = MeshBox(Point(5 + i * .7, 4+ j * .7), .1);   // TODO: del
}

void EditorUserInterface::populateDock()
{
   WorldItem item;
   S32 xPos = canvasWidth - horizMargin - dockWidth / 2;
   S32 yPos = 40;
   const S32 spacer = 35;
   mDockItems.clear();
   //while(mDockItems.size())    // Don't just to a .clear() because we want to make sure destructors run and memory gets cleared, as in game.cpp
   //   delete mDockItems[0];

   item = constructItem(ItemRepair, Point(xPos, yPos), -1, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;
   item = constructItem(ItemForceField, Point(xPos, yPos), -1, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;
   item = constructItem(ItemSpawn, Point(xPos, yPos), -1, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;
   item = constructItem(ItemTurret, Point(xPos, yPos), 0, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;
   item = constructItem(ItemTeleporter, Point(xPos, yPos), -1, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;
   item = constructItem(ItemSpeedZone, Point(xPos, yPos), -1, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;
   item = constructItem(ItemTextItem, Point(xPos, yPos), 0, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;

   if(!strcmp(mGameType, "SoccerGameType"))
   {
      item = constructItem(ItemSoccerBall, Point(xPos, yPos), -1, 0, 0);
      mDockItems.push_back(item);
   }
   else
   {
      item = constructItem(ItemFlag, Point(xPos, yPos), 0, 0, 0);
      mDockItems.push_back(item);
   }
   yPos += spacer;

   item = constructItem(ItemFlagSpawn, Point(xPos, yPos), 0, 0, 0);
   mDockItems.push_back(item);


   yPos += spacer;
   item = constructItem(ItemMine, Point(xPos, yPos), 0, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;
   item = constructItem(ItemSpyBug, Point(xPos, yPos), 0, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;  

   // These two will share a line
   item = constructItem(ItemBouncyBall, Point(xPos - 10, yPos), -1, 0, 0);
   mDockItems.push_back(item);
   item = constructItem(ItemAsteroid, Point(xPos + 10, yPos), -1, 0, 0);
   mDockItems.push_back(item);
   yPos += spacer;

   item = constructItem(ItemResource, Point(xPos, yPos), -1, 0, 0);
   mDockItems.push_back(item);
   yPos += 25;
   item = constructItem(ItemLoadoutZone, Point(canvasWidth - horizMargin - dockWidth + 5, yPos), 0, dockWidth - 10, 20);
   mDockItems.push_back(item);
   yPos += 25;
   if(!strcmp(mGameType, "HuntersGameType"))
   {
      item = constructItem(ItemNexus, Point(canvasWidth - horizMargin - dockWidth + 5, yPos), 0, dockWidth - 10, 20);
      mDockItems.push_back(item);
      yPos += 25;
   }
   else
   {
      item = constructItem(ItemGoalZone, Point(canvasWidth - horizMargin - dockWidth + 5, yPos), 0, dockWidth - 10, 20);
      mDockItems.push_back(item);
      yPos += spacer;
   }
}

enum geomType {
   geomPoint,           // Single point feature (like a flag)
   geomSimpleLine,      // Two point line (like a teleport)
   geomLine,            // Many point line (like a wall)
   geomPoly,            // Polygon feature (like a loadout zone)
   geomNone,            // Other/unknown (not used, just here for completeness)
};

struct GameItemRec
{
   const char *name;    // Item's name, as seen in save file
   bool hasWidth;       // Does item have width?
   bool hasTeam;        // Item can be associated with team
   bool canHaveNoTeam;  // Item can be neutral or hostile
   bool hasText;        // Item has a text string attached to it
   bool hasRepop;       // Item has a repop delay that can be set
   geomType geom;
   char letter;         // How item is represented by editor
   bool specialTabKeyRendering;  // true if item is rendered in a special way when tab is down
   const char *prettyNamePlural;
   const char *onDockName;       // Briefer, pretty looking label to label things on dock
   const char *onScreenName;     // Brief, pretty looking label to label things on screen
   const char *helpText;         // Help string displayed when hovering over item on the dock
};


// Remember to keep these properly aligned with GameItems enum
//   Name,                 hasWidth, hasTeam, canHaveNoTeam, hasText, hasRepop,   geom,        letter, special, prettyNamePlural        onDockName   onScreenName    description
GameItemRec gGameItemRecs[] = {
   { "Spawn",               false,    true,      false,       false,   false,   geomPoint,      'S',    false,  "Spawn points",           "Spawn",    "Spawn",      "Location where ships start.  At least one per team is required. [G]" },
   { "SpeedZone",           false,    false,     true,        false,   false,   geomSimpleLine,  0,     false,  "GoFasts",                "GoFast",   "GoFast",     "Makes ships go fast in direction of arrow. [P]" },
   { "SoccerBallItem",      false,    false,     false,       false,   false,   geomPoint,      'B',    true,   "Soccer balls",           "Ball",     "Ball",       "Soccer ball, can only be used in Soccer games." },
   { "FlagItem",            false,    true,      true,        false,   false,   geomPoint,       0,     false,  "Flags",                  "Flag",     "Flag",       "Flag item, used by a variety of game types." },
   { "FlagSpawn",           false,    true,      true,        false,   true,    geomPoint,       0,     false,  "Flag spawn points",      "FlagSpawn","FlagSpawn",  "Location where flags (or balls in Soccer) spawn after capture." },
   { "BarrierMaker",        true,     false,     false,       false,   false,   geomLine,        0,     false,  "Barrier makers",         "Wall",     "Wall",       "Run-of-the-mill wall item." },
   { "Teleporter",          false,    false,     false,       false,   false,   geomSimpleLine,  0,     false,  "Teleporters",            "Teleport", "Teleport",   "Teleports ships from one place to another. [T]" },
   { "RepairItem",          false,    false,     false,       false,   true,    geomPoint,       0,     false,  "Repair items",           "Repair",   "Repair",     "Repairs damage to ships. [B]" },
   { "TestItem",            false,    false,     false,       false,   false,   geomPoint,      'x',    true,   "Test items",             "Test",     "Test Item",  "Bouncy object that floats around and gets in the way." },
   { "Asteroid",            false,    false,     false,       false,   false,   geomPoint,       0,     true,   "Asteroids",              "Ast.",     "Asteroid",   "Shootable asteroid object.  Just like the arcade game." },
   { "Mine",                false,    false,     true,        false,   false,   geomPoint,      'M',    false,  "Mines",                  "Mine",     "Mine",       "Mines can be prepositioned, and are are \"hostile to all\". [M]" },
   { "SpyBug",              false,    true,      true,        false,   false,   geomPoint,      'S',    false,  "Spy bugs",               "Spy Bug",  "Spy Bug",    "Remote monitoring device that shows enemy ships on the commander's map. [Ctrl-B]" },
   { "ResourceItem",        false,    false,     false,       false,   false,   geomPoint,      'r',    true,   "Resource items",         "Resource", "Resource",   "Small bouncy object that floats around and gets in the way." },
   { "LoadoutZone",         false,    true,      true,        false,   false,   geomPoly,        0,     false,  "Loadout zones",          "Loadout",  "Loadout",    "Area to finalize ship modifications.  Each team should have at least one." },
   { "HuntersNexusObject",  false,    false,     true,        false,   false,   geomPoly,        0,     false,  "Nexus zones",            "Nexus",    "Nexus",      "Area to bring flags in Hunter game.  Cannot be used in other games." },
   { "SlipZone",            false,    false,     true,        false,   false,   geomPoly,       'z',    false,  "Slip zones",             "Slip Zone","Slip Zone",  "Not yet implemented." },
   { "Turret",              false,    true,      true,        false,   true,    geomPoint,      'T',    false,  "Turrets",                "Turret",   "Turret",     "Creates shooting turret.  Can be on a team, neutral, or \"hostile to all\". [Y]" },
   { "ForceFieldProjector", false,    true,      true,        false,   true ,   geomPoint,      '>',    false,  "Force field projectors", "ForceFld", "ForceFld",   "Creates a force field that lets only team members pass. [H]" },
   { "GoalZone",            false,    true,      true,        false,   false,   geomPoly,        0,     false,  "Goal zones",             "Goal",     "Goal",       "Target area used in a variety of games." },
   { "TextItem",            false,    true,      true,        true,    false,   geomSimpleLine,  0,     false,  "Text Items",             "Text",     "Text",       "Draws a bit of text on the map.  Visible only to team, or to all if neutral." },
   { "BotNavMeshZone",      false,    false,     true,        false,   false,   geomPoly,        0,     false,  "NavMesh Zones",          "NavMesh",  "NavMesh",    "Creates navigational mesh zone for robots." },
  
   { NULL,                  false,    false,     false,       false,   false,   geomNone,        0,     false,  "",                       "",         "",           "" },
};



// Save the current state of the editor objects for later undoing
void EditorUserInterface::saveUndoState(Vector<WorldItem> items)
{
   if(mAllUndoneUndoLevel > mUndoItems.size())     // Use case: We do 5 actions, save, undo 2, redo 1, then do some new action.  Our "no need to save" undo point is lost forever.
      mAllUndoneUndoLevel = -1;

   mUndoItems.push_back(items);
   if(mUndoItems.size() > 128)      // Keep the undo state from getting too large.  This is quite a lot of undo.
   {
      mUndoItems.pop_front();
      mAllUndoneUndoLevel -= 1;     // If this falls below 0, then we can't undo our way out of needing to save.
      logprintf("Undo buffer full... discarding oldest undo state");
   }

   mRedoItems.clear();
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


// Wipe undo/redo history
void EditorUserInterface::clearUndoHistory()
{
   mUndoItems.clear();
   mRedoItems.clear();
}

extern TeamPreset gTeamPresets[];

void EditorUserInterface::setLevelFileName(string name)
{
   if (name == "")
      mEditFileName = "";
   else  
      if(mEditFileName.find('.') == std::string::npos)      // Append extension, if one is needed
         mEditFileName = name + ".level";
}


S32 EditorUserInterface::getDefaultRepopDelay(GameItems itemType)
{
   if(itemType == ItemFlagSpawn)
      return FlagSpawn::defaultRespawnTime;
   else if(itemType == ItemTurret)
      return Turret::defaultRespawnTime;
   else if(itemType == ItemForceField)
      return ForceFieldProjector::defaultRespawnTime;
   else if(itemType == ItemRepair)
      return RepairItem::defaultRespawnTime;
   else 
      return -1;
}


void EditorUserInterface::makeSureThereIsAtLeastOneTeam()
{
   if(mTeams.size() == 0)
   {
      Team t;
      t.name = gTeamPresets[0].name;
      t.color.set(gTeamPresets[0].r, gTeamPresets[0].g, gTeamPresets[0].b);
      mTeams.push_back(t);
   }
}


// This sort will put points on top of lines on top of polygons...  as they should be
S32 QSORT_CALLBACK sortItems(EditorUserInterface::WorldItem *a, EditorUserInterface::WorldItem *b)
{
   return (gGameItemRecs[a->index].geom < gGameItemRecs[b->index].geom);
}

extern const char *gGameTypeNames[];
extern S32 gDefaultGameTypeIndex;
extern S32 gMaxPolygonPoints;

// Loads a level
void EditorUserInterface::loadLevel()
{
   // Initialize
   mItems.clear();
   mTeams.clear();
   mGameTypeArgs.clear();
   gGameParamUserInterface.gameParams.clear();
   gGameParamUserInterface.savedMenuItems.clear();
   gGameParamUserInterface.menuItems.clear();      // Keeps interface from using our menuItems to rebuild savedMenuItems
   mGridSize = Game::DefaultGridSize;              // Used in editor for scaling walls and text items appropriately

   mGameType[0] = 0;                   // Clear mGameType
   char fileBuffer[512];
   dSprintf(fileBuffer, sizeof(fileBuffer), "levels/%s", mEditFileName.c_str());

   if(initLevelFromFile(fileBuffer))   // Process level file --> returns true if file found and loaded, false if not (assume it's a new level)
   {
      makeSureThereIsAtLeastOneTeam(); // Make sure we at least have one team
      validateTeams();                 // Make sure every item has a valid team
      validateLevel();                 // Check level for errors (like too few spawns)
      mItems.sort(sortItems);
      gGameParamUserInterface.ignoreGameParams = false;
   }
   else
   {
      makeSureThereIsAtLeastOneTeam();                               // Make sure we at least have one team, like the man said.
      strcpy(mGameType, gGameTypeNames[gDefaultGameTypeIndex]);
      gGameParamUserInterface.gameParams.push_back("GameType 10 8"); // A nice, generic game type that we can default to
      gGameParamUserInterface.ignoreGameParams = true;               // Don't rely on the above for populating GameParameters menus... only to make sure something is there if we save
   }
   clearUndoHistory();                 // Clean out undo/redo buffers
   showAllObjects = true;              // Turn everything on
   mNeedToSave = false;                // Why save when we just loaded?
   mAllUndoneUndoLevel = 0;
   populateDock();                     // Add game-specific items to the dock
}

extern S32 gMaxPlayers;

// Process a line read from level file
void EditorUserInterface::processLevelLoadLine(int argc, const char **argv)
{
   U32 index;
   U32 strlenCmd = (U32) strlen(argv[0]);
   for(index = 0; gGameItemRecs[index].name != NULL; index++)
   {
      // Figure out how many arguments an item should have
      if(!strcmp(argv[0], gGameItemRecs[index].name))
      {
         S32 minArgs = 3;
         if(gGameItemRecs[index].geom >= geomLine)
            minArgs += 2;
         if(gGameItemRecs[index].hasTeam)
            minArgs++;
         if(gGameItemRecs[index].hasText)
            minArgs += 2;        // Size and message
         if(argc >= minArgs)     // We have enough args, please proceed.
            break;
      }
   }

   // Parse most game objects
   if(gGameItemRecs[index].name)    // Item is listed in gGameItemRecs, near top of this file
   {
      WorldItem i;
      i.index = static_cast<GameItems>(index);
      S32 arg = 1;

      // Should the following be moved to the constructor?  Probably...
      i.team = -1;
      i.selected = false;
      i.width = 0;

      if(gGameItemRecs[index].hasWidth)
      {
         i.width = atof(argv[arg]);
         arg++;
      }
      if(gGameItemRecs[index].hasTeam)
      {
         i.team = atoi(argv[arg]);
         arg++;
      }
      if(index == ItemTextItem)
      {
         for(S32 j = 0; j < 2; j++)       // Read two points...
         {
            Point p;
            p.read(argv + arg);
            i.verts.push_back(p);
            i.vertSelected.push_back(false);
            arg+=2;
         }

         i.textSize = atoi(argv[arg]);    // ...and a textsize...
         arg++;

         for(;arg < argc; arg ++)         // (no first part of for)
         {
            i.text += argv[arg];          // ...and glob the rest together as a string
            if (arg < argc - 1)
               i.text += " ";
         }
      }
      else if(index == ItemNexus && argc == 5)     // Old-school Zap! style Nexus definition --> note parallel code in HuntersNexusObject::processArguments
      {
         // Arg 0 will be HuntersNexusObject
         Point pos;
         pos.read(argv + 1);

         Point ext(50, 50);
         ext.set(atoi(argv[3]), atoi(argv[4]));
         ext /= mGridSize;

         Point p;
         p.set(pos.x - ext.x, pos.y - ext.y);   // UL corner
         i.verts.push_back(p);
         p.set(pos.x + ext.x, pos.y - ext.y);   // UR corner
         i.verts.push_back(p);
         p.set(pos.x + ext.x, pos.y + ext.y);   // LR corner
         i.verts.push_back(p);
         p.set(pos.x - ext.x, pos.y + ext.y);   // LL corner
         i.verts.push_back(p);

         for(S32 j = 0; j < 4; j++)
            i.vertSelected.push_back(false);
      }
      else        // Anything but a textItem or old-school NexusObject
      {
         S32 coords = argc;
         if(index == ItemSpeedZone)
            coords = 4;    // 2 pairs of coords = 2 * 2 = 4

         for(;arg < coords; arg += 2) // (no first arg)
         {
            // Put a cap on the number of vertices in a polygon
            if(gGameItemRecs[index].geom == geomPoly && i.verts.size() >= gMaxPolygonPoints)
               break;
            Point p;
            if(arg != argc - 1)
            {
               p.read(argv + arg);
               i.verts.push_back(p);
               i.vertSelected.push_back(false);
            }
         }
      }

      // Add a default spawn time, which may well be overridden below
      i.repopDelay = getDefaultRepopDelay(i.index);

      // Repair, Turrets, Forcefields, FlagSpawns all have optional additional argument dealing with repair or repopulation
      if(index == ItemRepair && argc == 4)
         i.repopDelay = atoi(argv[3]);

      if( (index == ItemTurret || index == ItemForceField || index == ItemFlagSpawn) && argc == 5)
         i.repopDelay = atoi(argv[4]);

      // SpeedZones have 2 optional extra arguments
      if(index == ItemSpeedZone)
      {
         if(argc >= 6)
            i.speed = atoi(argv[5]);
         else
            i.speed = SpeedZone::defaultSpeed;
     
         if(argc >= 7)
            i.boolattr = true;
       }

      mItems.push_back(i);    // Save item
   }

   else  // What remains are various game parameters
   {
      // Before copying, we'll make a dumb copy, which will be overwritten if the user goes into the GameParameters menu
      // This will cover us if the user comes in, edits the level, saves, and exits without visiting the GameParameters menu
      // by simply echoing all the parameters back out to the level file without furter processing or review.
      string temp;
      for (S32 i = 0; i < argc; i++)
      {
         temp += argv[i];
         if(i < argc - 1)
            temp += " ";
      }
      gGameParamUserInterface.gameParams.push_back(temp);

      // Parse GameType line... All game types are of form XXXXGameType
      if(strlenCmd >= 8 && !strcmp(argv[0] + strlenCmd - 8, "GameType"))
      {
         strcpy(mGameType, GameType::validateGameType(argv[0]) );    // validateGameType will return a valid game type, regradless of what's put in

         if(strcmp(mGameType, argv[0]))      // If these differ, then what we put in was invalid
            gEditorUserInterface.setWarnMessage("Invalid or missing GameType parameter", "Press [F3] to configure level");

         // Save the args (which we already have split out) for easier handling in the Game Parameter Editor
         for(S32 i = 1; i < argc; i++)
            mGameTypeArgs.push_back(atoi(argv[i]));
      }

      else if(!strcmp(argv[0], "GridSize"))
      {
         if(argc >= 1)
            mGridSize = atof(argv[1]);
      }

      else if(!strcmp(argv[0], "Script"))
      {
         // Put the command and params into a vector for later reference.  We only run when playing
         for(S32 i = 1; i < argc; i++)
            mScriptArgs.push_back(argv[i]);
      }

      // Parse Team definition line
      else if(!strcmp(argv[0], "Team"))
      {
         if(mTeams.size() >= GameType::gMaxTeams)
            return;

         Team team = GameType::readTeamFromLevelLine(argc, argv);

         // If team was read and processed properly, numPlayers will be 0
         if(team.numPlayers != -1)
            mTeams.push_back(team);
      }
   }
}     // end processLevelLoadLine


void EditorUserInterface::validateLevel()
{
   bool hasError = false;
   mLevelErrorMsgs.clear();
   bool foundSoccerBall = false;
   bool foundNexus = false;
   bool foundTeamFlags = false;
   bool foundFlags = false;

   Vector<bool> foundSpawn;
   char buf[32];

   string teamList, teams;

   for(S32 i = 0; i < mTeams.size(); i++)      // Initialize vector
      foundSpawn.push_back(false);

   // First check to make sure every team has at least one spawn point
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemSpawn && mItems[i].team >= 0)
         foundSpawn[mItems[i].team] = true;
      else if(mItems[i].index == ItemSoccerBall)
         foundSoccerBall = true;
      else if(mItems[i].index == ItemNexus)
         foundNexus = true;
      else if(mItems[i].index == ItemFlag)
      {
         foundFlags = true;
         if(mItems[i].team >= 0)
            foundTeamFlags = true;
      }

   for(S32 i = 0; i < foundSpawn.size(); i++)
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

   if (hasError)     // Compose error message
      mLevelErrorMsgs.push_back("ERROR: Need spawn point for " + teams + teamList);

   // The other thing that can cause a crash is a soccer ball in a a game other than SoccerGameType
   if(foundSoccerBall && strcmp(mGameType, "SoccerGameType"))
      mLevelErrorMsgs.push_back("ERROR: Soccer ball can only be used in soccer game.");
   // Check for the nexus object in a non-hunter game.  This cause endless grief too!
   if(foundNexus && strcmp(mGameType, "HuntersGameType"))
      mLevelErrorMsgs.push_back("ERROR: Nexus object can only be used in Hunters game.");
   // Check for missing nexus object in a hunter game.  This cause mucho dolor!
   if(!foundNexus && !strcmp(mGameType, "HuntersGameType"))
      mLevelErrorMsgs.push_back("ERROR: Hunters game must have a Nexus.");
   if(foundFlags && !isFlagGame(mGameType))
      mLevelErrorMsgs.push_back("WARNING: This game type does not use flags.");
   else if(foundTeamFlags && !isTeamFlagGame(mGameType))
      mLevelErrorMsgs.push_back("WARNING: This game type does not use team flags.");
}

// Check that each item has a valid team  (fixes any problems it finds)
void EditorUserInterface::validateTeams()
{
   S32 teams = mTeams.size();

   for(S32 i = 0; i < mItems.size(); i++)
   {
      S32 team = mItems[i].team;

      if(gGameItemRecs[mItems[i].index].hasTeam && team >= -2 && team < teams)      // -1 = neutral, -2 = hostile
         continue;      // This one's OK

      if(team < 0 && gGameItemRecs[mItems[i].index].canHaveNoTeam)
         continue;      // This one too

      if(gGameItemRecs[mItems[i].index].hasTeam)
         mItems[i].team = 0;     // We know there's at least one team, so there will always be a team 0
      else
         mItems[i].team = -1;    // We won't consider the case where hasTeam == canHaveNoTeam == false
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
         if(mTeams[i].color != mOldTeams[i].color || mTeams[i].name != mOldTeams[i].name)
         {
            teamsChanged = true;
            break;
         }

   if(!teamsChanged)       // Nothing changed, we're done here
      return;

   for (S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].team >= mTeams.size())       // Team no longer valid?
         mItems[i].team = 0;                    // Then set it to first team

   // And the dock items too...
   for (S32 i = 0; i < mDockItems.size(); i++)
      if(mDockItems[i].team >= mDockItems.size())
         mDockItems[i].team = 0;

   validateLevel();          // Revalidate level -- if teams have changed, requirements for spawns have too
   mNeedToSave = true;
   mAllUndoneUndoLevel = -1; // This change can't be undone
}

extern Color gNeutralTeamColor;
extern Color gHostileTeamColor;

Color EditorUserInterface::getTeamColor(S32 team)
{
   if(team == -1)
      return gNeutralTeamColor;
   else if(team == -2)
      return gHostileTeamColor;
   else
      return mTeams[team].color;
}

string EditorUserInterface::getLevelFileName()
{
   return mEditFileName;
}

void EditorUserInterface::onActivate()
{
   // Check if we have a level name:
   if(getLevelFileName() == "")         // We need to take a detour to get a level name
   {
      // Don't save this menu (false, below).  That way, if the user escapes out, and is returned to the "previous"
      // UI, they will get back to where they were before (prob. the main menu system), not back to here.
      gLevelNameEntryUserInterface.activate(false);

      return;
   }
   mLevelErrorMsgs.clear();
   mGameTypeArgs.clear();

   loadLevel();
   setCurrentTeam(0);

   mUnmovedItems = mItems;

   snapDisabled = false;      // Hold [space] to temporarily disable snapping
   mSelectedSet.clear();

   // Reset display parameters...
   centerView();
   mDragSelecting = false;
   mUp = mDown = mLeft = mRight = mIn = mOut = false;
   mCreatingPoly = false;
   mDraggingObjects = false;
   mDraggingDockItem = -1;
   mShowingReferenceShip = false;

   itemToLightUp = -1;
   vertexToLightUp = -1;

   mEditingSpecialAttrItem = -1;
   mSpecialAttribute = None;
}

extern Vector<StringTableEntry> gLevelList;

void EditorUserInterface::onReactivate()
{
   mDraggingObjects = false;

   mEditingSpecialAttrItem = -1;     // Probably not necessary
   mSpecialAttribute = None;

//   mSaveMsgTimer = 0;         // Don't show the saved game message any more --> but now we reactivate editor automatically, so don't need this
   populateDock();            // If game type changed, we'll need to update the dock

   if(mWasTesting)
   {
      gLevelList = mgLevelList;        // Restore level list
      gLevelDir = mgLevelDir;          // Restore gLevelDir 
      mWasTesting = false;
   }

   remove("editor.tmp");      // Delete temp file
}

Point EditorUserInterface::snapToLevelGrid(Point p)
{
   if(snapDisabled || mouseOnDock())
      return p;

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

   return Point(floor(p.x * mulFactor + 0.5) * divFactor,
                floor(p.y * mulFactor + 0.5) * divFactor);
}

extern Color gErrorMessageTextColor;

static const Color grayedOutColorBright = Color(.5, .5, .5);
static const Color grayedOutColorDim = Color(.25, .25, .25);


void EditorUserInterface::render()
{
   glColor3f(0.0, 0.0, 0.0);
   if(mCurrentScale >= 100)
   {
      F32 gridScale = mCurrentScale * 0.1;      // Draw tenths
      F32 xStart = fmod(mCurrentOffset.x, gridScale);
      F32 yStart = fmod(mCurrentOffset.y, gridScale);

      glColor3f(0.2, 0.2, 0.2);
      glBegin(GL_LINES);
      while(yStart < canvasHeight)
      {
         glVertex2f(0, yStart);
         glVertex2f(canvasWidth, yStart);
         yStart += gridScale;
      }
      while(xStart < canvasWidth)
      {
         glVertex2f(xStart, 0);
         glVertex2f(xStart, canvasHeight);
         xStart += gridScale;
      }
      glEnd();
   }

   if(mCurrentScale >= 10)
   {
      F32 xStart = fmod(mCurrentOffset.x, mCurrentScale);
      F32 yStart = fmod(mCurrentOffset.y, mCurrentScale);

      glColor3f(0.4, 0.4, 0.4);
      glBegin(GL_LINES);
      while(yStart < canvasHeight)
      {
         glVertex2f(0, yStart);
         glVertex2f(canvasWidth, yStart);
         yStart += mCurrentScale;
      }
      while(xStart < canvasWidth)
      {
         glVertex2f(xStart, 0);
         glVertex2f(xStart, canvasHeight);
         xStart += mCurrentScale;
      }
      glEnd();
   }

   glColor3f(0.7, 0.7, 0.7);
   glLineWidth(3);
   Point origin = convertLevelToCanvasCoord(Point(0,0));
   glBegin(GL_LINES );
      glVertex2f(0, origin.y);
      glVertex2f(canvasWidth, origin.y);
      glVertex2f(origin.x, 0);
      glVertex2f(origin.x, canvasHeight);
   glEnd();
   glLineWidth(gDefaultLineWidth);

   // Draw map items (teleporters, etc.) that are not being dragged, and won't have any text labels  (below the dock)
   for(S32 i = 0; i < mItems.size(); i++)
      if(!(mDraggingObjects && mItems[i].selected) && i != itemToLightUp)
         renderItem(mItems[i], i, false);

   // Draw map items (teleporters, etc.) that are are selected and/or lit up, so label is readable (still below the dock)
   for(S32 i = 0; i < mItems.size(); i++)
      if(i == itemToLightUp)
         renderItem(mItems[i], i, false);

   F32 width = -1;

   if(mCreatingPoly)    // Draw polygon under construction
   {
      Point mouseVertex = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));
      mNewItem.verts.push_back(mouseVertex);
      glLineWidth(3);
      glColor3f(1, 1, 0);     // yellow
      renderPoly(mNewItem.verts, false);

      glLineWidth(gDefaultLineWidth);

      for(S32 j = mNewItem.verts.size() - 1; j >= 0; j--)      // Go in reverse order so that placed vertices are drawn atop unplaced ones
      {
         Point v = convertLevelToCanvasCoord(mNewItem.verts[j]);

         // Draw vertices
         if(j == mNewItem.verts.size() - 1)           // This is our most current vertex
            renderVertex(HighlightedVertex, v, -1);
         else
            renderVertex(SelectedItemVertex, v, j);
      }
      mNewItem.verts.erase_fast(mNewItem.verts.size() - 1);
   }
   else  // Since we're not constructing a barrier, if there are any barriers selected, get the width for display on dock
   {
      for(S32 i = 0; i < mItems.size(); i++)
      {
         if(mItems[i].index == ItemBarrierMaker && (mItems[i].selected  || (itemToLightUp == i && vertexToLightUp == -1)))
         {
            width =  mItems[i].width;
            break;
         }
      }
   }

   if(mShowingReferenceShip)
   {
      // Render ship at cursor to show scale
      static F32 thrusts[4] =  { 1, 0, 0, 0 };

      glPushMatrix();
         glTranslatef(mMousePos.x, mMousePos.y, 0);
         glScalef(mCurrentScale / mGridSize, mCurrentScale / mGridSize, 1);
         glRotatef(90, 0, 0, 1);
         renderShip(red, 1, thrusts, 1, 5, false, false);
         glRotatef(-90, 0, 0, 1);

         // And show how far it can see
         F32 horizDist = Game::PlayerHorizVisDistance;
         F32 vertDist = Game::PlayerVertVisDistance;

         glEnable(GL_BLEND);     // Enable transparency
         glColor4f(.5, .5, 1, .35);
         glBegin(GL_POLYGON);
            glVertex2f(-horizDist, -vertDist);
            glVertex2f(horizDist, -vertDist);
            glVertex2f(horizDist, vertDist);
            glVertex2f(-horizDist, vertDist);
         glEnd();
         glDisable(GL_BLEND);

      glPopMatrix();
   }

   // Now, render item dock down RHS of screen
    glColor3f(0, 0, 0);        // Black background

   glBegin(GL_POLYGON);
      glVertex2f(canvasWidth - dockWidth - horizMargin, vertMargin);
      glVertex2f(canvasWidth - horizMargin, vertMargin);
      glVertex2f(canvasWidth - horizMargin, canvasHeight - vertMargin);
      glVertex2f(canvasWidth - dockWidth - horizMargin, canvasHeight - vertMargin);
   glEnd();

   // Bounding box around dock
   if(!showAllObjects)
      glColor(grayedOutColorBright);
   else if(mouseOnDock())       
      glColor3f(1, 1, 0);
   else
      glColor(white);

   glBegin(GL_LINE_LOOP);
      glVertex2f(canvasWidth - dockWidth - horizMargin, vertMargin);
      glVertex2f(canvasWidth - horizMargin, vertMargin);
      glVertex2f(canvasWidth - horizMargin, canvasHeight - vertMargin);
      glVertex2f(canvasWidth - dockWidth - horizMargin, canvasHeight - vertMargin);
   glEnd();

   // Draw coordinates on dock
   Point pos = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));

   char text[50];
   glColor(white);
   dSprintf(text, sizeof(text), "%2.2f|%2.2f", pos.x, pos.y);
   drawString(canvasWidth - (dockWidth + getStringWidth(7, text)) / 2 - horizMargin, canvasHeight - vertMargin - 15, 7, text);

   // And scale
   glColor(white);
   dSprintf(text, sizeof(text), "%2.2f", mCurrentScale);
   drawString(canvasWidth - (dockWidth + getStringWidth(7, text)) / 2 - horizMargin, canvasHeight - vertMargin - 25, 7, text);

   // Show number of teams
   glColor(white);
   dSprintf(text, sizeof(text), "Teams: %d",  mTeams.size());
   drawString(canvasWidth - (dockWidth + getStringWidth(7, text)) / 2 - horizMargin, canvasHeight - vertMargin - 35, 7, text);

   // Show level name, and whether it needs to be saved or not
   if(mNeedToSave)
      glColor(red);
   else
      glColor(white);

   dSprintf(text, sizeof(text), "%s%s", mNeedToSave ? "*" : "", mEditFileName.substr(0, mEditFileName.find_last_of('.')).c_str());    // Chop off extension
   drawString(canvasWidth - (dockWidth + getStringWidth(7, text)) / 2 - horizMargin, canvasHeight - vertMargin - 45, 7, text);

   // And wall width as needed
   if (width > 0)
   {
      glColor(white);
      dSprintf(text, sizeof(text), "Width: %2.0f", width);
      drawString(canvasWidth - (dockWidth + getStringWidth(7, text)) / 2 - horizMargin, canvasHeight - vertMargin - 55, 7, text);
   }


   // Draw map items (teleporters, etc.) that are being dragged  (above the dock)
   for(S32 i = 0; i < mItems.size(); i++)
      if(mDraggingObjects && mItems[i].selected)
         renderItem(mItems[i], i, false);

   if(mDragSelecting)      // Draw box for selecting items
   {
      glColor(white);
      Point downPos = convertLevelToCanvasCoord(mMouseDownPos);
      glBegin(GL_LINE_LOOP);
         glVertex2f(downPos.x, downPos.y);
         glVertex2f(mMousePos.x, downPos.y);
         glVertex2f(mMousePos.x, mMousePos.y);
         glVertex2f(downPos.x, mMousePos.y);
      glEnd();
   }

   // Render dock items
   for(S32 i = 0; i < mDockItems.size(); i++)
      renderItem(mDockItems[i], -2, true);

   // Render messages
   if(mouseOnDock())    // On the dock?  Then render help string if hovering over item
   {
      S32 hoverItem;
      findHitItemOnDock(mMousePos, hoverItem);

      if(hoverItem != -1)
      {
         glColor3f(.1, 1, .1);
         // Center string between left side of screen and edge of dock
         S32 x = (S32)((S32) canvasWidth - horizMargin - dockWidth - getStringWidth(15, gGameItemRecs[mDockItems[hoverItem].index].helpText)) / 2;
         drawString(x, canvasHeight - vertMargin - 15, 15, gGameItemRecs[mDockItems[hoverItem].index].helpText);
      }
   }

   if(mSaveMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (mSaveMsgTimer.getCurrent() < 1000)
         alpha = (F32) mSaveMsgTimer.getCurrent() / 1000;

      glEnable(GL_BLEND);
         glColor4f(mSaveMsgColor.r, mSaveMsgColor.g, mSaveMsgColor.b, alpha);
         drawCenteredString(canvasHeight - vertMargin - 65, 25, mSaveMsg.c_str());
      glDisable(GL_BLEND);
   }

   if(mWarnMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (mWarnMsgTimer.getCurrent() < 1000)
         alpha = (F32) mWarnMsgTimer.getCurrent() / 1000;

      glEnable(GL_BLEND);
         glColor4f(mWarnMsgColor.r, mWarnMsgColor.g, mWarnMsgColor.b, alpha);
         drawCenteredString(canvasHeight / 4, 25, mWarnMsg1.c_str());
         drawCenteredString(canvasHeight / 4 + 30, 25, mWarnMsg2.c_str());
      glDisable(GL_BLEND);
   }

   if(mLevelErrorMsgs.size())
   {
      glColor(gErrorMessageTextColor);
      S32 ypos = vertMargin + 50;
      for(S32 i = 0; i < mLevelErrorMsgs.size(); i++)
      {
         drawCenteredString(ypos, 20, mLevelErrorMsgs[i].c_str());
         ypos += 25;
      }
   }
   if(!showAllObjects)
   {
      glColor3f(0,1,1);
      drawCenteredString(vertMargin, 14, "Non-wall objects hidden.  Hit Ctrl-A to restore display.");
   }

   //// TODO: del following block
   //glColor3f(1,0,0); // red for now

   //for(S32 i = 0; i < gBoxesH; i++)
   //   for(S32 j = 0; j < gBoxesV; j++)
   //   {

   //   glBegin(GL_LINE_LOOP);
   //      glVertex(convertLevelToCanvasCoord(Point(testBox[i][j].bounds.min.x, testBox[i][j].bounds.min.y)));
   //      glVertex(convertLevelToCanvasCoord(Point(testBox[i][j].bounds.max.x, testBox[i][j].bounds.min.y)));
   //      glVertex(convertLevelToCanvasCoord(Point(testBox[i][j].bounds.max.x, testBox[i][j].bounds.max.y)));
   //      glVertex(convertLevelToCanvasCoord(Point(testBox[i][j].bounds.min.x, testBox[i][j].bounds.max.y)));
   //   glEnd();
   //   }

}

// Draw the vertices for a polygon or line item (i.e. walls)
void EditorUserInterface::renderLinePolyVertices(WorldItem item, S32 itemID, bool isDockItem)
{
   bool anyVertSelected = false;
   for(S32 j = 0; j < item.verts.size(); j++)
      if(item.vertSelected[j])
      {
         anyVertSelected = true;
         break;
      }

   // Draw the vertices of the wall or the polygon area
   for(S32 j = 0; j < item.verts.size(); j++)
   {
      Point v;
      if(isDockItem)
         v = item.verts[j];
      else
         v = convertLevelToCanvasCoord(item.verts[j]);

      if(item.vertSelected[j] || (itemID == itemToLightUp && j == vertexToLightUp))
         renderVertex(HighlightedVertex, v, j);          // Hollow yellow boxes with number
      else if(item.selected || itemID == itemToLightUp || anyVertSelected)
         renderVertex(SelectedItemVertex, v, j);         // Hollow red boxes with number
      else
         renderVertex(UnselectedItemVertex, v, -1, mCurrentScale > 35 ? 2 : 1);   // Solid red boxes, no number
   }
}

// Draw a vertex of a selected editor item,
void EditorUserInterface::renderVertex(VertexRenderStyles style, Point v, S32 number, S32 size)
{
   glColor((style == HighlightedVertex) ? yellow : red);

   if(style == HighlightedVertex || style == SelectedItemVertex)  // Hollow boxes
   {
      glBegin(GL_LINE_LOOP);
         glVertex2f(v.x - size, v.y - size);
         glVertex2f(v.x + size, v.y - size);
         glVertex2f(v.x + size, v.y + size);
         glVertex2f(v.x - size, v.y + size);
      glEnd();
   }
   else     // Solid boxes
   {
      glColor(red);
      glBegin(GL_POLYGON);
         glVertex2f(v.x - size, v.y - size);
         glVertex2f(v.x + size, v.y - size);
         glVertex2f(v.x + size, v.y + size);
         glVertex2f(v.x - size, v.y + size);
      glEnd();
   }

   if(number != -1)     // Draw vertex numbers
   {
      glColor(white);
      drawStringf(v.x - getStringWidthf(6, "%d", number) / 2, v.y - 3, 6, "%d", number);
   }
}


extern void constructBarrierPoints(const Vector<Point> &vec, F32 width, Vector<Point> &barrierEnds);

void EditorUserInterface::renderBarrier(Vector<Point> verts, bool selected, F32 width, bool isDockItem)
{
   glColor3f(0, 0, 1);     // blue
   Vector<Point> barPoints;

   constructBarrierPoints(verts, width, barPoints);

   for(S32 j = 0; j < barPoints.size(); j += 2)
   {
      Point dir = barPoints[j+1] - barPoints[j];
      Point crossVec(dir.y, -dir.x);
      crossVec.normalize(width * 0.5);
      glBegin(GL_POLYGON);
         glColor3f(0.5, 0.5, 1);    // pale blue

         if(isDockItem)
         {
            glVertex(barPoints[j] + crossVec);
            glVertex(barPoints[j] - crossVec);
            glVertex(barPoints[j+1] - crossVec);
            glVertex(barPoints[j+1] + crossVec);
         }
         else
         {
            glVertex(convertLevelToCanvasCoord(barPoints[j] + crossVec));
            glVertex(convertLevelToCanvasCoord(barPoints[j] - crossVec));
            glVertex(convertLevelToCanvasCoord(barPoints[j+1] - crossVec));
            glVertex(convertLevelToCanvasCoord(barPoints[j+1] + crossVec));
         }
      glEnd();
   }

   if(selected)
      glColor3f(1, 1, 0);     // yellow
   else
      glColor3f(0, 0, 1);     // blue

   glLineWidth(3);
   renderPoly(verts, isDockItem);
   glLineWidth(gDefaultLineWidth);
}

void EditorUserInterface::renderPoly(Vector<Point> verts, bool isDockItem)
{
   glBegin(GL_LINE_STRIP);
      for(S32 j = 0; j < verts.size(); j++)
      {
         Point v;
         if(isDockItem)
            v = verts[j];
         else
            v = convertLevelToCanvasCoord(verts[j]);

         glVertex2f(v.x, v.y);
      }
   glEnd();
}


// Items are rendered in index order, so those with a higher index get drawn later, and hence, on top
void EditorUserInterface::renderItem(WorldItem &item, S32 indx, bool isDockItem)
{
   Point pos, dest;
   const S32 labelSize = 9;      // Size to label items we're hovering over
   const S32 instrSize = 9;      // Size of instructions for special items
   const S32 attrSize = 10;

   const Color labelColor = white;

   bool hideit = !showAllObjects && !(mShowingReferenceShip && !isDockItem);

   if(isDockItem)
      pos = item.verts[0];
   else
      pos = convertLevelToCanvasCoord(item.verts[0]);
   Color c;

   if(gGameItemRecs[item.index].geom == geomSimpleLine && (showAllObjects || isDockItem || mShowingReferenceShip))    // Draw "two-point" items
   {
      if(isDockItem)
         dest = item.verts[1];
      else
         dest = convertLevelToCanvasCoord(item.verts[1]);

      // First check if we should show the schematic version or the real object rendering
      if((mShowingReferenceShip && (item.index == ItemTeleporter || item.index == ItemSpeedZone)) && !isDockItem)
      {
         glPushMatrix();
            setTranslationAndScale(pos);
            if(item.index == ItemTeleporter)
            {
               Vector<Point> dest;
               dest.push_back(convertLevelToCanvasCoord(item.verts[1]));

               renderTeleporter(pos, 0, true, gClientGame->getCurrentTime(), 1, Teleporter::TeleporterRadius, 1, dest, false);
            }
            else if(item.index == ItemSpeedZone)
               renderSpeedZone(pos, convertLevelToCanvasCoord(item.verts[1]), gClientGame->getCurrentTime());
         glPopMatrix(); 
      }
      else
      {
         glLineWidth(4);
         if(hideit) 
            glColor(grayedOutColorDim);
         else if(item.index == ItemTeleporter)
            glColor3f(0, 1, 0);
         else if(item.index == ItemSpeedZone)
            glColor(red);
         else if(item.index == ItemTextItem)
            glColor3f(0, 0, 1);

         if(item.selected || (indx == itemToLightUp && vertexToLightUp == -1))
            glColor3f(1, 1, 0);

         glBegin(GL_POLYGON);
            glVertex2f(pos.x - 5, pos.y - 5);   // Draw origin of item (square box)
            glVertex2f(pos.x + 5, pos.y - 5);
            glVertex2f(pos.x + 5, pos.y + 5);
            glVertex2f(pos.x - 5, pos.y + 5);
         glEnd();

         if(!isDockItem)
         {
            glBegin(GL_LINES);
               glVertex2f(pos.x, pos.y);           // Draw connecting line
               glVertex2f(dest.x, dest.y);

               F32 ang = pos.angleTo(dest);
               const F32 al = 15;                  // Length of arrow-head
               const F32 angoff = .5;              // Pitch of arrow-head prongs

               glVertex2f(dest.x, dest.y);         // Draw arrow-head
               glVertex2f(dest.x - cos(ang + angoff) * al, dest.y - sin(ang + angoff) * al);
               glVertex2f(dest.x, dest.y);
               glVertex2f(dest.x - cos(ang - angoff) * al, dest.y - sin(ang - angoff) * al);
            glEnd();
         }

         glLineWidth(gDefaultLineWidth);

         // If this is a textItem, and either the item or either vertex is selected, draw the text
         if(!isDockItem && gGameItemRecs[item.index].hasText)
         {
            // Recalc text size -- TODO:  this shouldn't be here... this should only happen when text is created, not every time it's drawn

            F32 strWidth = getStringWidth(120, item.text.c_str());
            F32 lineLen = item.verts[0].distanceTo(item.verts[1]);

            item.textSize = 120 * lineLen  * mGridSize / max(strWidth, 80.0f);

            glColor(getTeamColor(item.team));
            F32 txtSize = 120 * lineLen * mCurrentScale / max(strWidth, 80.0f);   // Use this more precise F32 calculation of size for smoother interactive rendering.  We'll use U32 approximation in game.

            drawAngleStringf_fixed(pos.x, pos.y, txtSize, pos.angleTo(dest), "%s%c", item.text.c_str(), cursorBlink && mSpecialAttribute == Text && mEditingSpecialAttrItem == indx ? '_' : 0);

            if((item.selected || indx == itemToLightUp) && mSpecialAttribute == None)
            {
               glColor(white);
               drawStringf_2pt(pos, dest, instrSize, -22, "[Ctrl-Enter] to edit text");
            }
         }
         else if(!isDockItem && item.index == ItemSpeedZone)      // Special labeling for speedzones
         {
            if(item.selected || indx == itemToLightUp)
            {
               glColor((mSpecialAttribute != GoFastSnap) ? white : inactiveSpecialAttributeColor);
               drawStringf_2pt(pos, dest, attrSize, 10, "Speed: %d", item.speed);

               glColor((mSpecialAttribute != GoFastSpeed) ? white : inactiveSpecialAttributeColor);
               drawStringf_2pt(pos, dest, attrSize, -2, "Snapping: %s", item.boolattr ? "On" : "Off");

              glColor(white);

               const char *msg, *instr;

               if(mSpecialAttribute == None)
               { 
                  msg = "[Ctrl-Enter] to edit speed";
                  instr = "";
               }
               else if(mSpecialAttribute == GoFastSpeed)
               {
                  msg = "[Ctrl-Enter] to edit snapping";
                  instr = "Up/Dn to change speed";
               }
               else if(mSpecialAttribute == GoFastSnap)
               {
                  msg = "[Ctrl-Enter] to stop editing";
                  instr = "Up/Dn to toggle snapping";
               }
               else
               {
                  msg = "???";
                  instr = "???";
               }

               drawStringf_2pt(pos, dest, instrSize, -22, msg);
               drawStringf_2pt(pos, dest, instrSize, -22 - instrSize - 2, instr);
            } 
         }

         // If either end is selected, draw a little white box around it
         glColor(labelColor);
         if(item.vertSelected[0] || (indx == itemToLightUp && vertexToLightUp == 0))
         {
            glBegin(GL_LINE_LOOP);
               glVertex2f(pos.x - 7, pos.y - 7);
               glVertex2f(pos.x + 7, pos.y - 7);
               glVertex2f(pos.x + 7, pos.y + 7);
               glVertex2f(pos.x - 7, pos.y + 7);
            glEnd();

            if(item.index == ItemTeleporter)
            {
               drawString(pos.x - getStringWidth(labelSize, "Teleport") / 2, pos.y + labelSize + 2, labelSize, "Teleport");
               drawString(pos.x - getStringWidth(labelSize, "Intake Vortex") / 2, pos.y + 2 * labelSize + 5, labelSize, "Intake Vortex");
            }
            else if(item.index == ItemSpeedZone)
            {
               drawString(pos.x - getStringWidth(labelSize, "GoFast") / 2, pos.y + labelSize + 2, labelSize, "GoFast");
               drawString(pos.x - getStringWidth(labelSize, "Location") / 2, pos.y + 2 * labelSize + 5, labelSize, "Location");
            }
            else if(item.index == ItemTextItem)
            {
               drawString(pos.x - getStringWidth(labelSize, "Text Item") / 2, pos.y + labelSize + 2, labelSize, "Text Item");
               drawString(pos.x - getStringWidth(labelSize, "Start") / 2, pos.y + 2 * labelSize + 5, labelSize, "Start");
            }
         }

         // Outline selected vertex, and label it
         if(item.vertSelected[1] || (indx == itemToLightUp && vertexToLightUp == 1))
         {
            glColor(labelColor);
            glBegin(GL_LINE_LOOP);
               glVertex2f(dest.x - 7, dest.y - 7);
               glVertex2f(dest.x + 7, dest.y - 7);
               glVertex2f(dest.x + 7, dest.y + 7);
               glVertex2f(dest.x - 7, dest.y + 7);
            glEnd();

            if(item.index == ItemTeleporter)
            {
               drawStringc(dest.x, dest.y + labelSize + 2, labelSize, "Teleport");
               drawStringc(dest.x, dest.y + 2 * labelSize + 5, labelSize, "Destination");
            }
            else if(item.index == ItemSpeedZone)
            {
               drawStringc(dest.x, dest.y + labelSize + 2, labelSize, "GoFast");
               drawStringc(dest.x, dest.y + 2 * labelSize + 5, labelSize, "Direction");
            }
         }
      }     
   }
   else if(gGameItemRecs[item.index].geom == geomLine )   // Can only be barrierMaker -- it's the only geomLine we have
   {
      renderBarrier(item.verts, item.selected || (indx == itemToLightUp && vertexToLightUp == -1), item.width / mGridSize, false);
      renderLinePolyVertices(item, indx, false);
   }

   else if(gGameItemRecs[item.index].geom == geomPoly)    // Draw regular line objects and poly objects
   {
      if(showAllObjects || isDockItem || mShowingReferenceShip)   // Anything that is not a wall
      {
         Vector<Point> outline;
         Vector<Point> fill;

         // First, draw the polygon fill
         for(S32 i = 0; i < item.verts.size(); i++)
            outline.push_back(isDockItem ? item.verts[i] : convertLevelToCanvasCoord(item.verts[i]));

         Triangulate::Process(outline, fill);     // Horribly inefficient to do this every time we draw, but easy

         Color theFillColor = !hideit ? getTeamColor(item.team) : grayedOutColorDim;
         if(item.index == ItemNexus && !hideit)
            theFillColor = gNexusOpenColor;      // Render Nexus items in pale green to match the actual thing

         glColor(hideit ? grayedOutColorDim : theFillColor);

         for(S32 i = 0; i < fill.size(); i+=3)
         {
            glBegin(GL_POLYGON);
               for(S32 j = i; j < i + 3; j++)
                  glVertex2f(fill[j].x, fill[j].y);
            glEnd();
         }

         // Now the polygon outline
         if(hideit)
            glColor(grayedOutColorBright);
         else if(item.selected || (indx == itemToLightUp && vertexToLightUp == -1))
            glColor3f(1, 1, 0);     // yellow
         else
            glColor(white);

         glLineWidth(3);

         glBegin(GL_LINE_LOOP);
            for(S32 i = 0; i < outline.size(); i++)
               glVertex2f(outline[i].x, outline[i].y);
         glEnd();

         glLineWidth(gDefaultLineWidth);        // Restore line width

         // Let's add a label
         glColor(hideit ? grayedOutColorBright : labelColor);

         F32 ang = angleOfLongestSide(outline);
         Point cent = centroid(outline);

         renderPolygonLabel(cent, ang, labelSize, gGameItemRecs[item.index].onScreenName);
      }

      if((item.index == ItemBarrierMaker || showAllObjects) && !isDockItem)  // No verts on dock!
         renderLinePolyVertices(item, indx, false);      // Draw vertices for this polygon
   }

   else if((showAllObjects || isDockItem || mShowingReferenceShip))    // Draw remaining point objects
   {
      c = hideit ? grayedOutColorDim : getTeamColor(item.team);        // And a color (based on team affiliation)

      if(item.index == ItemFlag)             // Draw flag
      {
         glPushMatrix();
            glTranslatef(pos.x, pos.y, 0);
            glScalef(0.6, 0.6, 1);
            renderFlag(Point(0,0), c, !hideit ? NULL : grayedOutColorDim);
         glPopMatrix();
      }
      else if(item.index == ItemFlagSpawn)    // Draw flag spawn point
      {
         glPushMatrix();
            glTranslatef(pos.x+1, pos.y, 0);
            glScalef(0.4, 0.4, 1);
            renderFlag(Point(0,0), c, !hideit ? NULL : grayedOutColorDim);
            
            glColor(!hideit ? white : grayedOutColorDim);
            drawCircle(Point(-4,0), 26);
         glPopMatrix();
      }
      else if(item.index == ItemBouncyBall)   // Draw testitem
      {
         if(mShowingReferenceShip && !isDockItem)
         {
            glPushMatrix();
               setTranslationAndScale(pos);
               renderTestItem(pos);
            glPopMatrix(); 
         }
         else
         {
            glColor(hideit ? grayedOutColorBright : Color(.7,.7,.7));
            drawPolygon(pos, 7, 8, 0);
         } 
      }
      else if(item.index == ItemAsteroid)   // Draw asteroid
      {
         S32 design = 2;
         if(mShowingReferenceShip && !isDockItem)
         {
            glPushMatrix();
               setTranslationAndScale(pos);
               renderAsteroid(pos, design, asteroidRenderSize[0]);
            glPopMatrix(); 
         }
         else
            renderAsteroid(pos, design, .1, !hideit ? NULL : grayedOutColorDim);   
      }

      else if(item.index == ItemResource)   // Draw resourceItem
      {
         if(mShowingReferenceShip && !isDockItem)
         {
            glPushMatrix();
               setTranslationAndScale(pos);
               renderResourceItem(pos);
            glPopMatrix(); 
         }
         else
            renderGenericItem(pos, c);
      }
      else if(item.index == ItemSoccerBall)  // Soccer ball, obviously
      {
         if(mShowingReferenceShip && !isDockItem)
         {
            glPushMatrix();
               setTranslationAndScale(pos);
               renderSoccerBall(pos);
            glPopMatrix(); 
         }
         else  
         {
            glColor(hideit ? grayedOutColorBright : Color(.7,.7,.7));
            drawCircle(pos, 9);
         } 
      }
      else if(item.index == ItemMine)  // And a mine
      {
         glColor(!hideit ? Color(.7, .7, .7) : grayedOutColorDim);
         drawCircle(pos, 9);

         glColor(!hideit ? Color(.1, .3, .3) : grayedOutColorDim);
         drawCircle(pos, 5);
      }
      else if(item.index == ItemSpyBug)  // And a spy bug
      {
         glColor(!hideit ? Color(.7, .7, .7) : grayedOutColorDim);
         drawCircle(pos, 9);

         glColor(!hideit ? getTeamColor(item.team) : grayedOutColorDim);
         drawCircle(pos, 5);

         // And show how far it can see... unless, of course, it's on the dock, and assuming the tab key has been pressed
         if(!isDockItem && mShowingReferenceShip)
         {
            glEnable(GL_BLEND);     // Enable transparency
            glColor4f(getTeamColor(item.team).r, getTeamColor(item.team).g, getTeamColor(item.team).b, .25);

            glBegin(GL_POLYGON);
               F32 size = mCurrentScale / mGridSize * gSpyBugRange;
               glVertex2f(pos.x - size, pos.y - size);
               glVertex2f(pos.x + size, pos.y - size);
               glVertex2f(pos.x + size, pos.y + size);
               glVertex2f(pos.x - size, pos.y + size);
            glEnd();
            glDisable(GL_BLEND);
         }
      }
      else if(item.index == ItemRepair)
         renderRepairItem(pos, true, !hideit ? NULL : grayedOutColorDim);

      else                             // Draw anything else
         renderGenericItem(pos, c);


      // If this is an item that has a repop attirbute, and the item is selected, draw the text
      if(!isDockItem && gGameItemRecs[item.index].hasRepop)
      {
         if(showAllObjects && (item.selected || indx == itemToLightUp || mEditingSpecialAttrItem == indx))
         {
            glColor(white);

            const char *healword = (item.index == ItemTurret || item.index == ItemForceField) ? "10% Heal" : "Regen";

            if(item.repopDelay == 0)
               drawStringfc(pos.x, pos.y + attrSize, attrSize, "%s: Disabled", healword);
            else
               drawStringfc(pos.x, pos.y + 10, attrSize, "%s: %d sec%c", healword, item.repopDelay, item.repopDelay != 1 ? 's' : 0);


            const char *msg; 

            if(mSpecialAttribute == None)
               msg = "[Ctrl-Enter] to edit";
            else if(mEditingSpecialAttrItem == indx && mSpecialAttribute == RepopDelay)
               msg = "Up/Dn to change";
            else
               msg = "???";
            drawStringc(pos.x, pos.y + instrSize + 13, instrSize, msg);
         }
      }

      // If we have a turret, render it's range (if tab is depressed)
      if(item.index == ItemTurret)
      {
         if(!isDockItem && mShowingReferenceShip)
         {
            glEnable(GL_BLEND);     // Enable transparency
            glColor4f(getTeamColor(item.team).r, getTeamColor(item.team).g, getTeamColor(item.team).b, .25);

            glBegin(GL_POLYGON);
            F32 size = mCurrentScale / mGridSize * (gWeapons[WeaponTurret].projLiveTime * gWeapons[WeaponTurret].projVelocity / 1000);
               glVertex2f(pos.x - size, pos.y - size);
               glVertex2f(pos.x + size, pos.y - size);
               glVertex2f(pos.x + size, pos.y + size);
               glVertex2f(pos.x - size, pos.y + size);
            glEnd();
            glDisable(GL_BLEND);
         }
      }

      if(showAllObjects && (item.selected || indx == itemToLightUp))   // Draw highlighted border around item if selected
      {
         Point pos = convertLevelToCanvasCoord(item.verts[0]);         // note that dockItems are never selected!

         glColor(labelColor);
         glBegin(GL_LINE_LOOP);
            glVertex2f(pos.x - 10, pos.y - 10);
            glVertex2f(pos.x + 10, pos.y - 10);
            glVertex2f(pos.x + 10, pos.y + 10);
            glVertex2f(pos.x - 10, pos.y + 10);
         glEnd();
      }

      char letter = gGameItemRecs[item.index].letter;    // Get letter to represent object

      // Mark the item with a letter, unless we're showing the reference ship
      if(letter && !(gGameItemRecs[item.index].specialTabKeyRendering && mShowingReferenceShip) || isDockItem)
      {
         S32 vertOffset = 8;
         if (letter >= 'a' && letter <= 'z')    // Better position lowercase letters
            vertOffset = 10;

         glColor(!hideit ? labelColor : grayedOutColorBright);
         drawStringf(pos.x - getStringWidthf(15, "%c", letter) / 2, pos.y - vertOffset, 15, "%c", letter);
      }
      // And label it if we're hovering over it (or not)
      if(showAllObjects && (item.selected || indx == itemToLightUp) && gGameItemRecs[item.index].onScreenName)
      {
         glColor(!hideit ? labelColor : grayedOutColorBright);
         drawStringc(pos.x, pos.y - labelSize * 2 - 5, labelSize, gGameItemRecs[item.index].onScreenName);     // Label on top
      } 
   }
   // Label our dock items
   if(isDockItem && gGameItemRecs[item.index].geom != geomPoly)      // Polys are already labeled internally
   {
      glColor(!hideit ? labelColor : grayedOutColorBright);
      F32 maxy = -F32_MAX;
      for(S32 j = 0; j < item.verts.size(); j++)
         if (item.verts[j].y > maxy)
            maxy = item.verts[j].y;

      // Make some label position adjustments
      if(gGameItemRecs[item.index].geom == geomSimpleLine)
         maxy -= 2;
      else if(item.index == ItemSoccerBall)
         maxy += 1;
      drawString(pos.x - getStringWidth(labelSize, gGameItemRecs[item.index].onDockName)/2, maxy + 8, labelSize, gGameItemRecs[item.index].onDockName);
   }
}


void EditorUserInterface::renderGenericItem(Point pos, Color c)
{
   glColor(c);
   glBegin(GL_POLYGON);          // Draw box upon which we'll put our letter
      glVertex2f(pos.x - 8, pos.y - 8);
      glVertex2f(pos.x + 8, pos.y - 8);
      glVertex2f(pos.x + 8, pos.y + 8);
      glVertex2f(pos.x - 8, pos.y + 8);
   glEnd();
}


// Will set the correct translation and scale to render items at correct location and scale as if it were a real level.
// Unclear enough??

void EditorUserInterface::setTranslationAndScale(Point pos)
{
   glScalef(mCurrentScale / mGridSize, mCurrentScale / mGridSize, 1);
   glTranslatef(-pos.x + pos.x * mGridSize / mCurrentScale, -pos.y + pos.y * mGridSize / mCurrentScale, 0);
}


void EditorUserInterface::clearSelection()
{
   for(S32 i = 0; i < mItems.size(); i++)
   {
      mItems[i].selected = false;
      for(S32 j = 0; j < mItems[i].verts.size(); j++)
         mItems[i].vertSelected[j] = false;
   }
}

void EditorUserInterface::clearDockSelection()     // Not used?
{
   for(S32 i = 0; i < mDockItems.size(); i++)
      mDockItems[i].selected = false;
}


S32 EditorUserInterface::countSelectedItems()
{
   S32 count = 0;
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected)
         count++;
   return count;
}

S32 EditorUserInterface::countSelectedVerts()
{
   S32 count = 0;
   for(S32 i = 0; i < mItems.size(); i++)
      for(S32 j = 0; j < mItems[i].vertSelected.size(); j++)
         if(mItems[i].vertSelected[j])
            count++;
   return count;
}


// Paste items on the clipboard
void EditorUserInterface::pasteSelection()
{
   S32 itemCount = mClipboard.size();

    if (!itemCount)                           // Nothing on clipboard, nothing to do
      return;

   saveUndoState(mItems);                    // So we can undo the paste

   for(S32 i = 0; i < mItems.size(); i++)    // Unselect everything, so pasted items will be the new selection
      mItems[i].selected = false;

   Point pos = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));
   Point offset = pos - mClipboard[0].verts[0];    // Diff between mouse pos and original object (item will be pasted such that the first vertex is at mouse pos)

   for(S32 i = 0; i < itemCount; i++)
   {
      WorldItem newItem = mClipboard[i];
      newItem.selected = true;
      for(S32 j = 0; j < newItem.verts.size(); j++)
         newItem.verts[j] += offset;
      mItems.push_back(newItem);
   }
   mItems.sort(sortItems);
   validateLevel();
   mNeedToSave = true;
}

// Copy selection to the clipboard
void EditorUserInterface::copySelection()
{
   bool alreadyCleared = false;

   S32 itemCount = mItems.size();
   for(S32 i = 0; i < itemCount; i++)
   {
      if(mItems[i].selected || (itemToLightUp == i && vertexToLightUp == -1))
      {
         WorldItem newItem = mItems[i];
         newItem.selected = false;
         for(S32 j = 0; j < newItem.verts.size(); j++)
            newItem.verts[j] += Point(0.5, 0.5);

         if(!alreadyCleared)  // Make sure we only purge the existing clipboard if we'll be putting someting new there
         {
            mClipboard.clear();
            alreadyCleared = true;
         }
         mClipboard.push_back(newItem);
      }
   }
}

// Rotate selected objects around their center point by angle
void EditorUserInterface::rotateSelection(F32 angle)
{
   saveUndoState(mItems);

   Point min, max;
   computeSelectionMinMax(min, max);
   Point ctr = (min + max)*0.5;
   ctr.x = 0;  //floor(ctr.x * 10) * 0.1f;      //--> try rotating around 0,0
   ctr.y = 0; //floor(ctr.y * 10) * 0.1f;
   F32 sinTheta = sin(angle * Float2Pi / 360.0f);
   F32 cosTheta = cos(angle * Float2Pi / 360.0f);
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].selected || (itemToLightUp == i && vertexToLightUp == -1))
      {
         WorldItem &item = mItems[i];
         for(S32 j = 0; j < item.verts.size(); j++)
         {
            Point v = item.verts[j] - ctr;
            Point n(v.x * cosTheta + v.y * sinTheta, v.y * cosTheta - v.x * sinTheta);
            item.verts[j] = n + ctr;
         }
      }
   }
   mNeedToSave = true;
}

void EditorUserInterface::computeSelectionMinMax(Point &min, Point &max)
{
   min.set(F32_MAX, F32_MAX);
   max.set(F32_MIN, F32_MIN);

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].selected || (itemToLightUp == i && vertexToLightUp == -1))
      {
         WorldItem &item = mItems[i];
         for(S32 j = 0; j < item.verts.size(); j++)
         {
            Point v = item.verts[j];

            if(v.x < min.x)
               min.x = v.x;
            if(v.x > max.x)
               max.x = v.x;
            if(v.y < min.y)
               min.y = v.y;
            if(v.y > max.y)
               max.y = v.y;
         }
      }
   }
}

// Set the team affiliation of any selected items
void EditorUserInterface::setCurrentTeam(S32 currentTeam)
{
   bool anyOK = false;
   bool anyChanged = false;

   Vector<WorldItem> undoItems = mItems;      // Create a snapshot so we can later undo if we do anything here

   if(currentTeam >= mTeams.size())
   {
      char msg[255];
      if(mTeams.size() == 1)
         dSprintf(msg, sizeof(msg), "Only 1 team has been configured.");
      else
         dSprintf(msg, sizeof(msg), "Only %d teams have been configured.", mTeams.size());
      gEditorUserInterface.setWarnMessage(msg, "Hit [F3] to configure teams.");
      return;
   }

   // Update all dock items to reflect new current team
   for(S32 i = 0; i < mDockItems.size(); i++)
   {
      if(!gGameItemRecs[mDockItems[i].index].hasTeam)
         continue;

      if(currentTeam < 0 && !gGameItemRecs[mDockItems[i].index].canHaveNoTeam)
         continue;

      mDockItems[i].team = currentTeam;
   }

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].selected || (itemToLightUp == i && vertexToLightUp == -1))
      {
         // Get team affiliation from dockItem of same type
         for(S32 j = 0; j < mDockItems.size(); j++)
         {
            if(mDockItems[j].index == mItems[i].index)
            {
               if(mDockItems[j].team != mItems[i].team)
               {
                  mItems[i].team = mDockItems[j].team;
                  anyChanged = true;
               }
               break;
            }
         }
      }
   }

   // Overwrite any warnings set above.  If we have a group of items selected, it makes no sense to show a
   // warning if one of those items has the team set improperly.  The warnings are more appropriate if only
   // one item is selected, or none of the items are given a valid team setting.
   if(anyOK)
      gEditorUserInterface.setWarnMessage("", "");

   if(anyChanged)
   {
      saveUndoState(undoItems);      // If anything changed, push our temp state onto the undo stack
      validateLevel();
      mNeedToSave = true;
   }
}

void EditorUserInterface::flipSelectionHorizontal()
{
   saveUndoState(mItems);

   Point min, max;
   computeSelectionMinMax(min, max);
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected || (itemToLightUp == i && vertexToLightUp == -1))
         for(S32 j = 0; j < mItems[i].verts.size(); j++)
            mItems[i].verts[j].x = min.x + (max.x - mItems[i].verts[j].x);

   mNeedToSave = true;
}

void EditorUserInterface::flipSelectionVertical()
{
   saveUndoState(mItems);

   Point min, max;
   computeSelectionMinMax(min, max);
   for(S32 i = 0; i < mItems.size(); i++)  
      if(mItems[i].selected || (itemToLightUp == i && vertexToLightUp == -1))
         for(S32 j = 0; j < mItems[i].verts.size(); j++)
            mItems[i].verts[j].y = min.y + (max.y - mItems[i].verts[j].y);

   mNeedToSave = true;
}

void EditorUserInterface::findHitVertex(Point canvasPos, S32 &hitItem, S32 &hitVertex)
{
   const S32 targetSize = 8;
   hitItem = -1;
   hitVertex = -1;

   if(mEditingSpecialAttrItem != -1)    // If we're editing a text special attribute, disable this functionality
      return;

   for(S32 i = mItems.size() - 1; i >= 0; i--)     // Reverse order so we get items "from the top down"
   {
      if(!showAllObjects && mItems[i].index != ItemBarrierMaker)     // Only select walls in CTRL-A mode
         continue;
      WorldItem &p = mItems[i];
      if(gGameItemRecs[p.index].geom <= geomPoint)
         continue;

      for(S32 j = p.verts.size() - 1; j >= 0; j--)
      {
         Point v = convertLevelToCanvasCoord(p.verts[j]);
         if(fabs(v.x - canvasPos.x) < targetSize && fabs(v.y - canvasPos.y) < targetSize)
         {
            hitItem = i;
            hitVertex = j;
            return;
         }
      }
   }
}


void EditorUserInterface::findHitItemOnDock(Point canvasPos, S32 &hitItem)
{
   hitItem = -1;

   if(!showAllObjects)           // Only add dock items when objects are visible
      return;

   if(mEditingSpecialAttrItem != -1)    // If we're editing a text item, disable this functionality
      return;

   for(S32 i = mDockItems.size() - 1; i >= 0; i--)     // Go in reverse order because the code we copied did ;-)
   {
      Point pos = mDockItems[i].verts[0];

      if(fabs(canvasPos.x - pos.x) < 8 && fabs(canvasPos.y - pos.y) < 8)
      {
         hitItem = i;
         return;
      }
   }

   // Now check for polygon interior hits
   for(S32 i = 0; i < mDockItems.size(); i++)
      if(gGameItemRecs[mDockItems[i].index].geom == geomPoly)
      {
         Vector<Point> verts;
         for(S32 j = 0; j < mDockItems[i].verts.size(); j++)
            verts.push_back(mDockItems[i].verts[j]);

         if(PolygonContains2(verts.address(),verts.size(), canvasPos))
         {
            hitItem = i;
            return;
         }
      }
}


void EditorUserInterface::findHitItemAndEdge(Point canvasPos, S32 &hitItem, S32 &hitEdge)
{
   hitItem = -1;  
   hitEdge = -1;

   if(mEditingSpecialAttrItem  != -1)    // If we're editing special attributes, disable this functionality
      return;

   for(S32 i = mItems.size() - 1; i >= 0; i--)     // Go in reverse order to prioritize items drawn on top
   {
      if(!showAllObjects && mItems[i].index != ItemBarrierMaker)     // Only select walls in CTRL-A mode
         continue;

      WorldItem &p = mItems[i];
      if(gGameItemRecs[mItems[i].index].geom == geomPoint)  
      {
         Point pos = convertLevelToCanvasCoord(p.verts[0]);
         if(fabs(canvasPos.x - pos.x) < 8 && fabs(canvasPos.y - pos.y) < 8)
         {
            hitItem = i;
            return;
         }
      }

      Vector<Point> verts;
      verts = p.verts;
      if(gGameItemRecs[mItems[i].index].geom == geomPoly)   // Add first point to the end to create last side on poly
         verts.push_back(verts.first());

      Point p1 = convertLevelToCanvasCoord(verts[0]);
      for(S32 j = 0; j < verts.size() - 1; j++)
      {
         Point p2 = convertLevelToCanvasCoord(verts[j+1]);

         Point edgeDelta = p2 - p1;
         Point clickDelta = canvasPos - p1;
         float fraction = clickDelta.dot(edgeDelta);
         float lenSquared = edgeDelta.dot(edgeDelta);
         if(fraction > 0 && fraction < lenSquared)
         {
            // Compute the closest point:
            Point closest = p1 + edgeDelta * (fraction / lenSquared);
            float distance = (canvasPos - closest).len();
            if(distance < 5)
            {
               hitEdge = j;
               hitItem = i;
               return;
            }
         }
         p1 = p2;
      }
   }

   if(!showAllObjects)
      return; 

   // If we're still here, it means we didn't find anything yet.  Make one more pass, and see if we're in any polys.
   // This time we'll loop forward, though I don't think it really matters.

   for(S32 i = 0; i < mItems.size(); i++)
      if(gGameItemRecs[mItems[i].index].geom == geomPoly)
      {
         Vector<Point> verts;
         for(S32 j = 0; j < mItems[i].verts.size(); j++)
            verts.push_back(convertLevelToCanvasCoord(mItems[i].verts[j]));

         if(PolygonContains2(verts.address(),verts.size(), canvasPos))
         {
            hitItem = i;
            return;
         }
      }
}


extern Point gMousePos;

void EditorUserInterface::onMouseMoved(S32 x, S32 y)
{
   mMousePos = convertWindowToCanvasCoord(gMousePos);

   if(mCreatingPoly)
      return;
   
   S32 vertexHit, vertexHitPoly;
   S32 edgeHit, itemHit;
   bool showMoveCursor;

   findHitVertex(mMousePos, vertexHitPoly, vertexHit);
   findHitItemAndEdge(mMousePos, itemHit, edgeHit);

   showMoveCursor = (vertexHitPoly != -1 || vertexHit != -1 || itemHit != -1 || edgeHit != -1);

   vertexToLightUp = -1;
   itemToLightUp = -1;

   if(vertexHit != -1 && !mItems[vertexHitPoly].vertSelected[vertexHit])   // Hit a vertex that wasn't already selected
   {
      vertexToLightUp = vertexHit;
      itemToLightUp = vertexHitPoly;
   }
   else if(itemHit != -1 && !mItems[itemHit].selected)                     // We hit an item that wasn't already selected
   {
      itemToLightUp = itemHit;
   }

   if(itemHit != -1 && !mItems[itemHit].selected && gGameItemRecs[mItems[itemHit].index].geom == geomPoint)  // Check again, and take a point object in preference to a vertex
   {
      itemToLightUp = itemHit;
      vertexToLightUp = -1;
   }

   glutSetCursor(showMoveCursor ? GLUT_CURSOR_SPRAY : GLUT_CURSOR_RIGHT_ARROW);
}


void EditorUserInterface::onMouseDragged(S32 x, S32 y)
{
   mMousePos = convertWindowToCanvasCoord(Point(x,y));

   if(mCreatingPoly || mDragSelecting || mEditingSpecialAttrItem != -1)
      return;

   if(mDraggingDockItem != -1)      // We just started dragging an item off the dock
   {
      // Instantiate object so we are in essence dragging a non-dock item
      Point pos = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));

      WorldItem item;
      if(gGameItemRecs[mDockItems[mDraggingDockItem].index].geom == geomPoly)
         // For polygon items, try to match proportions of the dock rendering.  Size will vary by map scale.
         item = constructItem(mDockItems[mDraggingDockItem].index, pos, mDockItems[mDraggingDockItem].team, .68, .35);
      else
         // Non polygon item --> size only used for geomSimpleLine items (teleport et al), ignored for geomPoints
         item = constructItem(mDockItems[mDraggingDockItem].index, pos, mDockItems[mDraggingDockItem].team, 1, 0);

      clearSelection();          // No items are selected...
      item.selected = true;      // ...except for the new one
      mItems.push_back(item);    // Add our new item to the master item list
      mItems.sort(sortItems);    // So things will render in the proper order
      mDraggingDockItem = -1;    // Because now we're dragging a real item
      mUnmovedItems = mItems;    // So we know where things were so we know where to render them while being dragged
      validateLevel();           // Check level for errors
   }

   mDraggingObjects = true;
   Point delta = convertCanvasToLevelCoord(mMousePos);

   Point firstVert(0,0);

   // If an item is no longer aligned to the grid (either moved with snapping disabled, or at a smaller scale)
   // try and get it realigned with the grid when it is moved.  We'll focus on the item for point items, or on the
   // first selected vertex for a higher-order item.  (TODO: Make that the vertex closest to the mouse)
   for(S32 i = 0; i < mItems.size(); i++)
      for(S32 j = 0; j < mItems[i].verts.size(); j++)
         if(mItems[i].vertSelected[j])
         {
            firstVert = mUnmovedItems[i].verts[j];
            goto done;     // Is there a better way to exit two loops than a goto?
         }
         else if(mItems[i].selected && gGameItemRecs[mItems[i].index].geom == geomPoint)
         {
            firstVert = mUnmovedItems[i].verts[0];
            goto done;
         }
done:
   delta = snapToLevelGrid(delta - mMouseDownPos + firstVert) - firstVert;

   // Now update the locations of all items we're moving to show them being dragged
   for(S32 i = 0; i < mItems.size(); i++)
      for(S32 j = 0; j < mItems[i].verts.size(); j++)
         if(mItems[i].selected || mItems[i].vertSelected[j])
            mItems[i].verts[j] = mUnmovedItems[i].verts[j] + delta;
}


void EditorUserInterface::deleteSelection(bool objectsOnly)
{
   Vector<WorldItem> items = mItems;
   bool deleted = false;

   mSelectedSet.clear();         // Do this to avoid corrupting the heap... somehow...

   for(S32 i = 0; i < mItems.size(); ) // no i++
   {
      if(mItems[i].selected || (itemToLightUp == i && vertexToLightUp == -1))
      {
         mItems.erase(i);
         deleted = true;
         if(itemToLightUp == i)  // Since indices change as items are deleted, this will keep incorrect items from being deleted
            itemToLightUp = -1;
         else
            itemToLightUp--;
      }
      else if(!objectsOnly)      // Deleted any selected vertices
      {
         for(S32 j = 0; j < mItems[i].verts.size(); )  // no j++
         {
            if(mItems[i].vertSelected[j] || (itemToLightUp == i && vertexToLightUp == j))
            {
               mItems[i].verts.erase(j);
               mItems[i].vertSelected.erase(j);
               deleted = true;
               if(vertexToLightUp == j)
               {
                  vertexToLightUp = -1;
                  itemToLightUp = -1;
               }
               else
                  vertexToLightUp--;
            }
            else
               j++;
         }

         // Deleted last vertex, or item can't lose a vertex... it must go!
         if(mItems[i].verts.size() == 0 || (gGameItemRecs[mItems[i].index].geom == geomSimpleLine && mItems[i].verts.size() < 2)
                                        || (gGameItemRecs[mItems[i].index].geom == geomLine && mItems[i].verts.size() < 2)
                                        || (gGameItemRecs[mItems[i].index].geom == geomPoly && mItems[i].verts.size() < 2))
         {
            mItems.erase(i);
            deleted = true;
         }
         else
            i++;
      }  //else if
      else
         i++;
   }  // for

   if (deleted)
   {
      saveUndoState(items);
      validateLevel();
      mNeedToSave = true;
   }
}

// Increase selected wall thickness by amt
void EditorUserInterface::incBarrierWidth(S32 amt)
{
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemBarrierMaker && (mItems[i].selected || (itemToLightUp == i && vertexToLightUp == -1)))
      {
         mItems[i].width+= amt - (S32) mItems[i].width % amt;
         mNeedToSave = true;
         mAllUndoneUndoLevel = -1;    // This change can't be undone
      }
}

// Decrease selected wall thickness by amt
void EditorUserInterface::decBarrierWidth(S32 amt)
{
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemBarrierMaker && (mItems[i].selected || (itemToLightUp == i && vertexToLightUp == -1)))
      {
         mItems[i].width-= ((S32) mItems[i].width % amt) ? (S32) mItems[i].width % amt : amt;      // Dirty, ugly thing
         if(mItems[i].width < 1)
             mItems[i].width = 1;
         mNeedToSave = true;
         mAllUndoneUndoLevel = -1;    // This change can't be undone
      }
}

// Split wall/barrier on currently selected vertex/vertices
void EditorUserInterface::splitBarrier()
{
   bool split = false;
   Vector<WorldItem> undoItems = mItems;      // Create a snapshot so we can later undo if we do anything here

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemBarrierMaker)
          for(S32 j = 1; j < mItems[i].verts.size()-1; j++)     // Can't split on end vertices!
            if(mItems[i].vertSelected[j] || (itemToLightUp == i && vertexToLightUp == j))
            {
               split = true;
               WorldItem newItem;
               newItem.index = ItemBarrierMaker;
               newItem.team = -1;
               newItem.width = 50;
               for(S32 k = j; k < mItems[i].verts.size(); )    // No k++!
               {
                  newItem.verts.push_back(mItems[i].verts[k]);
                  newItem.vertSelected.push_back(false);
                  if (k > j)
                  {
                     mItems[i].verts.erase(k);     // Don't delete j == k vertex -- it needs to remain as the final vertex of the old wall
                     mItems[i].vertSelected.erase(k);
                  }
                  else
                     k++;
               }
               mItems.push_back(newItem);
               mItems.sort(sortItems);
               goto done2;                         // Yes, gotos are naughty, but they just work so well sometimes...
            }
done2:
   if(split)
   {
      clearSelection();
      saveUndoState(undoItems);
      mNeedToSave = true;
   }
}

// Join two or more sections of wall that have coincident end points.  Will ignore invalid join attempts.
void EditorUserInterface::joinBarrier()
{
   bool joined = false;
   Vector<WorldItem> undoItems = mItems;      // Create a snapshot so we can later undo if we do anything here

   for(S32 i = 0; i < mItems.size()-1; i++)
      if(mItems[i].index == ItemBarrierMaker && (mItems[i].selected || (i == itemToLightUp && vertexToLightUp == -1)))
      {
         for(S32 j = i + 1; j < mItems.size(); j++)
         {
            if(mItems[j].index == ItemBarrierMaker && (mItems[j].selected || (j == itemToLightUp && vertexToLightUp == -1)))
            {
               if(mItems[i].verts[0].distanceTo(mItems[j].verts[0]) < .01)    // First vertices are the same  1 2 3 | 1 4 5
               {
                  joined = true;
                  for(S32 a = 1; a < mItems[j].verts.size(); a++)             // Skip first vertex, because it would be a dupe
                  {
                     mItems[i].verts.push_front(mItems[j].verts[a]);
                     mItems[i].vertSelected.push_back(false);
                  }
                  mItems.erase(j);
                  i--;
                  j--;
                  if(itemToLightUp > j)
                     itemToLightUp--;
               }
               else if(mItems[i].verts[0].distanceTo(mItems[j].verts[mItems[j].verts.size()-1]) < .01)     // First vertex conincides with final vertex 3 2 1 | 5 4 3
               {
                  joined = true;
                  for(S32 a = mItems[j].verts.size()-2; a >= 0; a--)
                  {
                     mItems[i].verts.push_front(mItems[j].verts[a]);
                     mItems[i].vertSelected.push_back(false);
                  }
                  mItems.erase(j);
                  i--;
                  j--;
                  if(itemToLightUp > j)
                     itemToLightUp--;

               }
               else if(mItems[i].verts[mItems[i].verts.size()-1].distanceTo(mItems[j].verts[0]) < .01)     // Last vertex conincides with first 1 2 3 | 3 4 5
               {
                  joined = true;
                  for(S32 a = 1; a < mItems[j].verts.size(); a++)           // Skip first vertex, because it would be a dupe
                  {
                     mItems[i].verts.push_back(mItems[j].verts[a]);
                     mItems[i].vertSelected.push_back(false);
                  }
                  mItems.erase(j);
                  i--;
                  j--;
                  if(itemToLightUp > j)
                     itemToLightUp--;

               }
               else if(mItems[i].verts[mItems[i].verts.size()-1].distanceTo(mItems[j].verts[mItems[j].verts.size()-1]) < .01)     // Last vertices coincide  1 2 3 | 5 4 3
               {
                  joined = true;
                  for(S32 a = mItems[j].verts.size()-2; a >= 0; a--)
                  {
                     mItems[i].verts.push_back(mItems[j].verts[a]);
                     mItems[i].vertSelected.push_back(false);
                  }
                  mItems.erase(j);
                  i--;
                  j--;
                  if(itemToLightUp > j)
                     itemToLightUp--;
               }
            }
         }
      }

   if(joined)
   {
      clearSelection();
      saveUndoState(undoItems);
      mNeedToSave = true;
   }

}

void EditorUserInterface::insertNewItem(GameItems itemType)
{
   if(!showAllObjects)     // No inserting when items are hidden!
      return;

   clearSelection();
   saveUndoState(mItems);

   Point pos = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));
   S32 team = -1;

   // Get team affiliation from dockItem of same type
   for(S32 i = 0; i < mDockItems.size(); i++)
      if (mDockItems[i].index == itemType)
      {
         team = mDockItems[i].team;
         break;
      }

   WorldItem item;
   item = constructItem(itemType, pos, team, 1, 1);

   mItems.push_back(item);
   mItems.sort(sortItems);
   validateLevel();
   mNeedToSave = true;
}

void EditorUserInterface::centerView()
{
   if(mItems.size())
   {
      F32 minx = F32_MAX, miny = F32_MAX;
      F32 maxx = -F32_MAX, maxy = -F32_MAX;

      for(S32 i = 0; i < mItems.size(); i++)
         for(S32 j = 0; j < mItems[i].verts.size(); j++)
         {
            if(mItems[i].verts[j].x < minx)
               minx = mItems[i].verts[j].x;
            if(mItems[i].verts[j].x > maxx)
               maxx = mItems[i].verts[j].x;
            if(mItems[i].verts[j].y < miny)
               miny = mItems[i].verts[j].y;
            if(mItems[i].verts[j].y > maxy)
               maxy = mItems[i].verts[j].y;
         }

      F32 midx = (minx + maxx) / 2;
      F32 midy = (miny + maxy) / 2;

      mCurrentScale = min(canvasWidth / (maxx - minx), canvasHeight / (maxy - miny));
      mCurrentScale /= 1.3;      // Zoom out a bit
      mCurrentOffset.set(canvasWidth/2 - mCurrentScale * midx, canvasHeight/2 - mCurrentScale * midy);
   }
   else
   {
      mCurrentScale = 100;
      mCurrentOffset.set(0,0);
   }
}



EditorUserInterface::WorldItem EditorUserInterface::constructItem(GameItems itemType, Point pos, S32 team, F32 width, F32 height)
{
   WorldItem item;
   item.index = itemType;
   item.team = team;

   item.selected = false;
   item.verts.push_back(pos);
   item.vertSelected.push_back(false);

   // Handle multiple-point items
   if(gGameItemRecs[itemType].geom == geomSimpleLine)       // Start with diagonal line
   {
      item.verts.push_back(item.verts[0] + Point(width, height));
      item.vertSelected.push_back(false);
   }
   else if(gGameItemRecs[itemType].geom == geomPoly)        // Start with a size x size square
   {
      item.verts.push_back(item.verts[0] + Point(width, 0));
      item.verts.push_back(item.verts[0] + Point(width, height));
      item.verts.push_back(item.verts[0] + Point(0, height));
      item.vertSelected.push_back(false);
      item.vertSelected.push_back(false);
      item.vertSelected.push_back(false);
   }

   if(gGameItemRecs[itemType].hasText)
   {
      item.textSize = 30;
      item.text = "Your text here";
   }

   if(gGameItemRecs[itemType].hasRepop)
   {
      item.repopDelay = getDefaultRepopDelay(itemType);
   }

   if(itemType == ItemSpeedZone)
   {
      item.speed = SpeedZone::defaultSpeed;
      item.boolattr = SpeedZone::defaultSnap;
   }

   return item;
}

// Save selection mask, which can be retrieved later, as long as mItems hasn't changed.
// For now, we'll be lazy and just save mItems, but we could save a little memory by being smarter
void EditorUserInterface::saveSelection()
{
   mSelectedSet = mItems;
}

// Restore selection mask
void EditorUserInterface::restoreSelection()
{
   for(S32 i = 0; i < mSelectedSet.size(); i++)
   {
      mItems[i].selected = mSelectedSet[i].selected;
      for(S32 j = 0; j < mSelectedSet[i].verts.size(); j++)
         mItems[i].vertSelected[j] = mSelectedSet[i].vertSelected[j];
   }
}

U32 EditorUserInterface::getNextAttr(S32 item)       // Not sure why this fn can't return a SpecialAttribute...  hrm...
{
   // Advance to the next attribute. If we were at None, start with the first.
   U32 curr = (mSpecialAttribute == None) ? 0 : mSpecialAttribute + 1;      

   // Find next attribute that applies to selected object
   for(U32 i = curr; i <= None; i++)
   {
      if( ((i == Text) && gGameItemRecs[mItems[item].index].hasText) ||
          ((i == RepopDelay) && gGameItemRecs[mItems[item].index].hasRepop) ||
          ((i == GoFastSpeed || i == GoFastSnap) && !strcmp(gGameItemRecs[mItems[item].index].name, "SpeedZone")) ||   // strcmp ==> kind of janky
          (i == None ) )
         return i;
   }
   return None;      // Should never get here...
}


// Handle key presses
void EditorUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   // This first section is the key handlers for when we're editing the special attributes of an item.  Regular
   // key actions are handled below.
   if(mEditingSpecialAttrItem != -1)      
   {  /* braces required */
      if( keyCode == KEY_J && getKeyState(KEY_CTRL) )    // Let Ctrl-Enter, which GLUT reports as Ctrl-J, drop through
      { /* Do nothing */ }
      else if(keyCode == MOUSE_LEFT || keyCode == MOUSE_RIGHT)    // Trap mouse clicks... do nothing
         return;
      else if(keyCode == KEY_ESCAPE || keyCode == KEY_ENTER)      // End editing
      {
         mEditingSpecialAttrItem = -1;
         mSpecialAttribute = None;
         return;
      }

      else if(mSpecialAttribute == Text)
      {
         if(keyCode == KEY_BACKSPACE || keyCode == KEY_DELETE)
            mItems[mEditingSpecialAttrItem].text = mItems[mEditingSpecialAttrItem].text.substr(0, mItems[mEditingSpecialAttrItem].text.length() - 1);

         else if(ascii)       // User typed a character -- add it to the string
            if(mItems[mEditingSpecialAttrItem].text.length() < MAX_TEXTITEM_LEN)
               mItems[mEditingSpecialAttrItem].text += ascii;

         return;
      }
      else if(mSpecialAttribute == RepopDelay)
      {
         if(keyCode == KEY_UP)         // Up - increase delay
         {  /* braces required */
            if(mItems[mEditingSpecialAttrItem].repopDelay < 120)
               mItems[mEditingSpecialAttrItem].repopDelay++;
            return;
         }
         else if(keyCode == KEY_DOWN)  // Down - decrease delay
         {  /* braces required */
            if(mItems[mEditingSpecialAttrItem].repopDelay > 0)
               mItems[mEditingSpecialAttrItem].repopDelay--;
            return;
         }
      }
      else if(mSpecialAttribute == GoFastSpeed)   
      {
         if(keyCode == KEY_UP)         // Up - increase speed
         {  /* braces required */
            if(mItems[mEditingSpecialAttrItem].speed < SpeedZone::maxSpeed)
               mItems[mEditingSpecialAttrItem].speed += 10;
            return;
         }
         else if(keyCode == KEY_DOWN)  // Down - decrease speed
         {  /* braces required */
            if(mItems[mEditingSpecialAttrItem].speed > SpeedZone::minSpeed)
               mItems[mEditingSpecialAttrItem].speed -= 10;
            return;
         }
      }
      else if(mSpecialAttribute == GoFastSnap)   
      {
         if(keyCode == KEY_UP || keyCode == KEY_DOWN)   // Up/Down - toggle snapping
         {  /* braces required */
            mItems[mEditingSpecialAttrItem].boolattr = !mItems[mEditingSpecialAttrItem].boolattr;
            return;
         }
      }
   }

   // Regular key handling from here on down
   if(keyCode == KEY_J && getKeyState(KEY_CTRL))       // Ctrl-Enter - Edit props  (loser GLUT sees Ctrl-Enter as Ctrl-J)
   {
      for(S32 i = 0; i < mItems.size(); i++)
      {
         if(mItems[i].selected || itemToLightUp == i)
         {
            // Force item i to be the one and only selected item.  This will clear up some problems that
            // might otherwise occur.  If you have multiple items selected, this might not pick the right
            // one... but at least we'll force the issue.  And anyway, how could we know which one the 
            // user wants, anyway?
            clearSelection();
            mItems[i].selected = true;
            
            mEditingSpecialAttrItem = i;
            mSpecialAttribute = (SpecialAttribute) getNextAttr(i);

            if(mSpecialAttribute != None)
            {
               mEditingSpecialAttrItem = i;
               saveUndoState(mItems);
            }
            else
               mEditingSpecialAttrItem = -1;

            break;
         }
      }
   }
   else if(getKeyState(KEY_SHIFT) && keyCode == KEY_0)  // Shift-0 -> Set team to hostile
   {
      setCurrentTeam(-2);
      return;
   }

   else if(ascii >= '0' && ascii <= '9')           // Change team affiliation of selection with 0-9 keys
   {
      setCurrentTeam(ascii - '1');
      return;
   }
   // Ctrl-left click is same as right click for Mac users
   else if(keyCode == MOUSE_RIGHT || (keyCode == MOUSE_LEFT && getKeyState(KEY_CTRL)))
   {
      mMousePos = convertWindowToCanvasCoord(gMousePos);
      if(mCreatingPoly)
      {
         if(mNewItem.verts.size() >= gMaxPolygonPoints)     // Limit number of points in a polygon
            return;
         Point newVertex = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));
         mNewItem.verts.push_back(newVertex);
         mNewItem.vertSelected.push_back(false);
         return;
      }

      S32 edgeHit, itemHit;
      findHitItemAndEdge(mMousePos, itemHit, edgeHit);

      if(itemHit != -1 && (gGameItemRecs[mItems[itemHit].index].geom == geomLine ||
                           gGameItemRecs[mItems[itemHit].index].geom >= geomPoly   ))
      {

         if(mItems[itemHit].verts.size() >= gMaxPolygonPoints)     // Polygon full -- can't add more
            return;

         saveUndoState(mItems);
         clearSelection();

         Point newVertex = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));

         // Insert an extra vertex at the mouse clicked point, and then select it.
         mItems[itemHit].verts.insert(edgeHit + 1);
         mItems[itemHit].verts[edgeHit + 1] = newVertex;

         mItems[itemHit].vertSelected.insert(edgeHit + 1);
         mItems[itemHit].vertSelected[edgeHit + 1] = true;

         // Do the same for the mUnmovedItems list, to keep it in sync with mItems,
         // which allows us to drag our vertex around without wierd snapping action
         mUnmovedItems[itemHit].verts.insert(edgeHit + 1);
         mUnmovedItems[itemHit].verts[edgeHit + 1] = newVertex;

         mUnmovedItems[itemHit].vertSelected.insert(edgeHit + 1);
         mUnmovedItems[itemHit].vertSelected[edgeHit + 1] = true;

         mMouseDownPos = newVertex;
      }
      else     // Start creating a new poly
      {
         mCreatingPoly = true;
         mNewItem.verts.clear();
         mNewItem.index = ItemBarrierMaker;
         mNewItem.width = Barrier::BarrierWidth;
         mNewItem.team = -1;
         mNewItem.selected = false;
         mNewItem.vertSelected.clear();
         Point newVertex = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));
         mNewItem.verts.push_back(newVertex);
         mNewItem.vertSelected.push_back(false);
      }
   }

   if(keyCode == MOUSE_LEFT)
   {
      itemCameFromDock = false;
      mMousePos = convertWindowToCanvasCoord(gMousePos);
      if(mCreatingPoly)          // Save any polygon we might be creating
      {
         saveUndoState(mItems);

         if(mNewItem.verts.size() > 1)
            mItems.push_back(mNewItem);
         mNewItem.verts.clear();
         mCreatingPoly = false;
         mItems.sort(sortItems);
      }

      mMouseDownPos = convertCanvasToLevelCoord(mMousePos);

      if(mouseOnDock())    // On the dock?  Did we hit something to start dragging off the dock?
      {
         clearSelection();
         findHitItemOnDock(mMousePos, mDraggingDockItem);
         itemCameFromDock = true;

      }
      else                 // Mouse is not on dock
      {
         mDraggingDockItem = -1;

         // rules for mouse down:
         // if the click has no shift- modifier, then
         //   if the click was on something that was selected
         //     do nothing
         //   else
         //     clear the selection
         //     add what was clicked to the selection
         //  else
         //    toggle the selection of what was clicked

         S32 vertexHit, vertexHitPoly;
         S32 edgeHit, itemHit;

         findHitVertex(mMousePos, vertexHitPoly, vertexHit);
         findHitItemAndEdge(mMousePos, itemHit, edgeHit);

         if(!getKeyState(KEY_SHIFT))      // Shift key is not down
         {
            if(vertexHit != -1 && mItems[vertexHitPoly].selected)    // Hit a vertex of an already selected item --> now we can move that vertex w/o losing our selection
            {
               saveSelection();
               clearSelection();
               mItems[vertexHitPoly].vertSelected[vertexHit] = true;
               //vertexHit = -1;
               //itemHit = vertexHitPoly;
            }
            if (itemHit != -1 && gGameItemRecs[mItems[itemHit].index].geom == geomPoint)  // Hit a point item
            {
               if(!mItems[itemHit].selected)    // Item was not already selected
               {
                  clearSelection();
                  saveSelection();  // Basically, save the fact that nothing is selected
                  mItems[itemHit].selected = true;
               }
               else                             // Item was already selected
                  saveSelection();
            }
            else if(vertexHit != -1 && (itemHit == -1 || !mItems[itemHit].selected))      // Hit a vertex of an unselected item
            {        // (braces required)
               if(!mItems[vertexHitPoly].vertSelected[vertexHit])
               {
                  clearSelection();
                  saveSelection();     // Basically, save the fact that nothing is selected
                  mItems[vertexHitPoly].vertSelected[vertexHit] = true;
               }
            }
            else if(itemHit != -1)                                                        // Hit a non-point item, but not a vertex
            {        // (braces required)
               if(!mItems[itemHit].selected)    // Item was not already selected
               {
                  clearSelection();
                  saveSelection();  // Basically, save the fact that nothing is selected
                  mItems[itemHit].selected = true;
               }
               else                             // Item was already selected
                  saveSelection();
            }
            else     // Clicked off in space.  Starting to draw a bounding rectangle?
            {
               mDragSelecting = true;
               clearSelection();
               saveSelection();
           }
         }
         else     // Shift key is down
         {
            if(vertexHit != -1)
               mItems[vertexHitPoly].vertSelected[vertexHit] =
                  !mItems[vertexHitPoly].vertSelected[vertexHit];
            else if(itemHit != -1)
               mItems[itemHit].selected = !mItems[itemHit].selected;    // Toggle selection of hit item
            else
               mDragSelecting = true;
         }
         mMostRecentState = mItems;
         mUnmovedItems = mItems;
      }     // end mouse not on dock block, doc
   }     // end if keyCode == MOUSE_LEFT

   // Neither mouse button, let's try some keys
   else if(keyCode == KEY_D)              // D - Pan right
      mRight = true;
   else if(keyCode == KEY_RIGHT)          // Right - Pan right
      mRight = true;
   else if(keyCode == KEY_F)              // F - Flip horizontal
      flipSelectionHorizontal();
   else if(keyCode == KEY_V && getKeyState(KEY_CTRL))    // Ctrl-V - Paste selection
      pasteSelection();
   else if(keyCode == KEY_V)              // V - Flip vertical
      flipSelectionVertical();
   else if(keyCode == KEY_L && getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT))
   {
      loadLevel();                        // Ctrl-Shift-L - Reload level
      gEditorUserInterface.setSaveMessage("Reloaded " + getLevelFileName(), true);
   }
   else if(keyCode == KEY_Z)
   {
      if(getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT))   // Ctrl-Shift Z - Redo
      {
         if(mRedoItems.size())                     // If there's a redo state available...
         {
            mUndoItems.push_back(mItems);          // Save state onto undo buffer

            mItems = mRedoItems.last();            // Restore state from redo buffer
            mRedoItems.pop_back();

            if(mAllUndoneUndoLevel != mUndoItems.size())
               mNeedToSave = true;
            else if(mUndoItems.size() == mAllUndoneUndoLevel)
               mNeedToSave = false;
         }
      }

      else if(getKeyState(KEY_CTRL))      // Ctrl-Z - Undo
      {
         if(mUndoItems.size())                     // If there's an undo state available...
         {
            mRedoItems.push_back(mItems);          // Save state onto redo buffer

            mItems = mUndoItems.last();            // Restore state from undo buffer
            mUndoItems.pop_back();

            if(mAllUndoneUndoLevel != mUndoItems.size())
               mNeedToSave = true;
            else if(mUndoItems.size() == mAllUndoneUndoLevel)
               mNeedToSave = false;
         }
      }
      else                                // Z - Reset veiw
        centerView();
   }
   else if(keyCode == KEY_R)
      rotateSelection(getKeyState(KEY_SHIFT) ? 15 : -15); // Shift-R - Rotate CW, R - Rotate CCW

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
   else if(keyCode == KEY_A && getKeyState(KEY_CTRL))    // Ctrl-A - toggle see all objects
   {
      showAllObjects = !showAllObjects;
      if(!showAllObjects && !mDraggingObjects)
         glutSetCursor(GLUT_CURSOR_RIGHT_ARROW); 

      onMouseMoved(gMousePos.x, gMousePos.y);   // Reset mouse to spray if appropriate
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
   else if(keyCode == KEY_SLASH)          // / - Split barrier on selected vertex
      splitBarrier();
   else if(keyCode == KEY_J)
      joinBarrier();
   else if(keyCode == KEY_X && getKeyState(KEY_CTRL))    // Ctrl-X - Cut selection
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
   else if(keyCode == KEY_H)              // H - Force Field
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
   else if(keyCode == KEY_ESCAPE)         // Activate the menu
   {
      UserInterface::playBoop();
      gEditorMenuUserInterface.activate();
   }
   else if(keyCode == KEY_SPACE)
      snapDisabled = true;
   else if(keyCode == KEY_TAB)
      mShowingReferenceShip = true;

   //else if(keyCode = KEY_CLOSEBRACKET)
   //{
   //   //for( S32 x = 0; x < 10; x++)
   //   for(S32 i = 0; i < gBoxesH; i++)
   //      for(S32 j = 0; j < gBoxesV; j++)
   //      {
   //         if(testBox[i][j].done[MeshBox::all])
   //            continue;

   //         if(!testBox[i][j].done[MeshBox::up])
   //         {
   //            testBox[i][j].grow(MeshBox::up);
   //            if( ( (j > 0) && testBox[i][j].bounds.intersects(testBox[i][j-1].bounds) ||
   //                          ((i > 0) && testBox[i][j].bounds.intersects(testBox[i-1][j-1].bounds)) ||
   //                          ((i < gBoxesH-1) && testBox[i][j].bounds.intersects(testBox[i+1][j-1].bounds)) )
   //                || testBox[i][j].checkWallCollision() )

   //               testBox[i][j].revert(MeshBox::up);
   //         }

   //         if(!testBox[i][j].done[MeshBox::down])
   //         {
   //            testBox[i][j].grow(MeshBox::down);
   //            if( ( (j < gBoxesV-1) && testBox[i][j].bounds.intersects(testBox[i][j+1].bounds) ||
   //                          ((i > 0) && testBox[i][j].bounds.intersects(testBox[i-1][j+1].bounds)) ||
   //                          ((i < gBoxesH-1) && testBox[i][j].bounds.intersects(testBox[i+1][j+1].bounds))  )
   //
   //
   //               || testBox[i][j].checkWallCollision() )
   //               testBox[i][j].revert(MeshBox::down);
   //         }

   //         if(!testBox[i][j].done[MeshBox::left])
   //         {
   //            testBox[i][j].grow(MeshBox::left);
   //            if( ((i > 0) && testBox[i][j].bounds.intersects(testBox[i-1][j].bounds) ||
   //                          ((j > 0) && testBox[i][j].bounds.intersects(testBox[i-1][j-1].bounds)) ||
   //                          ((j < gBoxesV-1) && testBox[i][j].bounds.intersects(testBox[i-1][j+1].bounds)))

   //
   //               || testBox[i][j].checkWallCollision() )
   //               testBox[i][j].revert(MeshBox::left);
   //         }

   //         if(!testBox[i][j].done[MeshBox::right])
   //         {
   //            testBox[i][j].grow(MeshBox::right);
   //            if( ((i < gBoxesH-1) && testBox[i][j].bounds.intersects(testBox[i+1][j].bounds) ||
   //                           ((j > 0) && testBox[i][j].bounds.intersects(testBox[i+1][j-1].bounds)) ||
   //                           ((j < gBoxesV-1) && testBox[i][j].bounds.intersects(testBox[i+1][j+1].bounds)))

   //
   //               || testBox[i][j].checkWallCollision() )
   //               testBox[i][j].revert(MeshBox::right);
   //         }
   //      }
//   }
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
         snapDisabled = false;
         break;
      case KEY_TAB:
         mShowingReferenceShip = false;
         break;
      case MOUSE_LEFT:
         mMousePos = convertWindowToCanvasCoord(gMousePos);

         if(mDragSelecting)      // We were drawing a selection box
         {
            Rect r(convertCanvasToLevelCoord(mMousePos), mMouseDownPos);
            for(S32 i = 0; i < mItems.size(); i++)
            {
               S32 j;
               for(j = 0; j < mItems[i].verts.size(); j++)
                  if(!r.contains(mItems[i].verts[j]))
                     break;
               if(j == mItems[i].verts.size())
                  mItems[i].selected = true;
            }
            mDragSelecting = false;
         }
         else
         {
            if(mDraggingObjects)    // We were dragging and dropping.  Was it a move or a delete?
            {
               mDraggingObjects = false;

               if(mouseOnDock())             // This was really a delete (dragged to dock)
               {
                  if(itemCameFromDock)
                  {
                     for(S32 i = 0; i < mItems.size(); i++)
                        if(mItems[i].selected)
                        {
                           mItems.erase(i);
                           break;
                        }
                  }
                  else
                  {
                     mItems = mMostRecentState; // Essential undoes the dragging, so if we undo delete, our object will be back where it was before the delete
                     deleteSelection(true);
                     mNeedToSave = true;
                     validateLevel();
                  }
               }
               else                          // We were moving something... need to save an undo state if anything changed
               {
                  restoreSelection();

                  // Check if anything changed... (i.e. did we move?)
                  for(S32 i = 0; i < mItems.size(); i++)
                     for(S32 j = 0; j < mItems[i].verts.size(); j++)
                        if(mItems[i].verts[j] != mUnmovedItems[i].verts[j])
                        {
                           saveUndoState(mMostRecentState);    // Something changed... save an undo state!
                           mNeedToSave = true;
                           return;
                        }
               }
            }
         }
         break;
   }     // case
}

bool EditorUserInterface::mouseOnDock()
{
   return (mMousePos.x >= canvasWidth - dockWidth - horizMargin &&
           mMousePos.x <= canvasWidth - horizMargin &&
           mMousePos.y >= vertMargin &&
           mMousePos.y <= canvasHeight - vertMargin);
}

void EditorUserInterface::idle(U32 timeDelta)
{
   F32 pixelsToScroll = timeDelta * 0.5f;

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
   if(mOut && !mIn)
      mCurrentScale *= 1 - timeDelta * 0.002;
   if(mCurrentScale > 200)
     mCurrentScale = 200;
   else if(mCurrentScale < 10)
      mCurrentScale = 10;
   Point newMousePoint = convertLevelToCanvasCoord(mouseLevelPoint);
   mCurrentOffset += mMousePos - newMousePoint;

   mSaveMsgTimer.update(timeDelta);
   mWarnMsgTimer.update(timeDelta);

   updateCursorBlink(timeDelta);
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

bool EditorUserInterface::saveLevel(bool showFailMessages, bool showSuccessMessages)
{
   try
   {
      // Check if we have a valid (i.e. non-null) filename
      if(mEditFileName == "")
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
      dSprintf(fileNameBuffer, sizeof(fileNameBuffer), "levels/%s", mEditFileName.c_str());
      FILE *f = fopen(fileNameBuffer, "w");
      if(!f)
         throw("Could not open file for writing");

      // Write out our game parameters --> first one will be the gameType, along with all required parameters
      for(S32 i = 0; i < gGameParamUserInterface.gameParams.size(); i++)
      {
         if(gGameParamUserInterface.gameParams[i].substr(0, 5) != "Team ")  // Don't write out teams here... do it below!
         {
            fprintf(f, "%s", gGameParamUserInterface.gameParams[i].c_str());
            fprintf(f, "\n");
         }
      }

      for(S32 i = 0; i < mTeams.size(); i++)
         fprintf(f, "Team %s %g %g %g\n", mTeams[i].name.getString(),
            mTeams[i].color.r, mTeams[i].color.g, mTeams[i].color.b); 

      // Save script and parameters, if any.  If none, omit the line altogether.
      if(mScriptArgs.size() > 0) 
      {
         string scriptLine = "Script";
         for(S32 i = 0; i < mScriptArgs.size(); i++)
            scriptLine += string(" ") + mScriptArgs[i];
         fprintf(f, "%s%s", scriptLine.c_str(), "\n");
      }

      // Write out all maze items
      for(S32 i = 0; i < mItems.size(); i++)
      {
         WorldItem &p = mItems[i];
         fprintf(f, "%s", gGameItemRecs[mItems[i].index].name);

         if(gGameItemRecs[mItems[i].index].hasWidth)
            fprintf(f, " %g", mItems[i].width);
         if(gGameItemRecs[mItems[i].index].hasTeam)
            fprintf(f, " %d", mItems[i].team);
         for(S32 j = 0; j < p.verts.size(); j++)
            fprintf(f, " %g %g ", p.verts[j].x, p.verts[j].y);
         if(gGameItemRecs[mItems[i].index].hasText)
            fprintf(f, " %d %s", mItems[i].textSize, mItems[i].text.c_str());
         if(gGameItemRecs[mItems[i].index].hasRepop && mItems[i].repopDelay != -1)
            fprintf(f, " %d", mItems[i].repopDelay);
         if(mItems[i].index == ItemSpeedZone)
            fprintf(f, " %d %s", mItems[i].speed, mItems[i].boolattr ? "SnapEnabled" : "");

         fprintf(f, "\n");
      }
      fclose(f);
   }
   catch (char *msg)
   {
      if(showFailMessages)
         gEditorUserInterface.setSaveMessage("Error Saving: " + string(msg), false);
      return false;
   }
   catch (...)    // Catches any other errors not caught above
   {
      if(showFailMessages)
         gEditorUserInterface.setSaveMessage("Error Saving: Unknown Error", false);
      return false;
   }

   mNeedToSave = false;
   mAllUndoneUndoLevel = mUndoItems.size();     // If we undo to this point, we won't need to save.
   if(showSuccessMessages)
      gEditorUserInterface.setSaveMessage("Saved " + getLevelFileName(), true);
   return true;
}

extern void initHostGame(Address bindAddress, bool testMode);
extern CmdLineSettings gCmdLineSettings;

void EditorUserInterface::testLevel()
{
   bool gameTypeError = false;

   if(strcmp(mGameType, GameType::validateGameType(mGameType)))
      gameTypeError = true;

   validateLevel();
   if(mLevelErrorMsgs.size() || gameTypeError)
   {
      S32 i;
      gErrorMsgUserInterface.reset();
      gErrorMsgUserInterface.setTitle("LEVEL HAS CRITICAL PROBLEMS");

      for(i = 0; i < mLevelErrorMsgs.size(); i++)
         gErrorMsgUserInterface.setMessage(i + 1, mLevelErrorMsgs[i].c_str());
      if(gameTypeError)
      {
         gErrorMsgUserInterface.setMessage(i + 1, "ERROR: GameType is invalid.");
         gErrorMsgUserInterface.setMessage(i + 2, "(Fix in Level Parameters screen [F3])");
         i+=2;
      }
      gErrorMsgUserInterface.setMessage(i + 3, "Loading this level may cause the game to crash.");
      gErrorMsgUserInterface.activate();
      return;
   }

   string tmpFileName = mEditFileName;
   mEditFileName = "editor.tmp";
 
   glutSetCursor(GLUT_CURSOR_NONE);    // Turn off cursor
   bool nts = mNeedToSave;             // Save these parameters because they are normally reset when a level is saved.
   S32 auul = mAllUndoneUndoLevel;     // Since we're only saving a temp copy, we really shouldn't reset them...

   if(saveLevel(true, false))
   {
      mEditFileName = tmpFileName;     // Restore the level name

      mgLevelList = gLevelList;
      gLevelList.clear();

      mgLevelDir = gLevelDir;
      gLevelDir = "levels";            // Temporarily override gLevelDir -- we want to write to levels folder regardless of the -leveldir param

      mWasTesting = true;
 
      gLevelList.push_front("editor.tmp");
      initHostGame(Address(IPProtocol, Address::Any, 28000), true);
   }

   mNeedToSave = nts;                  // Restore saved parameters
   mAllUndoneUndoLevel = auul;
}

//////////////////// EditorMenuUserInterface ////////////////////

EditorMenuUserInterface gEditorMenuUserInterface;

// Constructor
EditorMenuUserInterface::EditorMenuUserInterface()
{
   setMenuID(EditorMenuUI);
   dSprintf(menuTitle, sizeof(menuTitle), "EDITOR MENU");
}

void EditorMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
}

extern IniSettings gIniSettings;

void EditorMenuUserInterface::setupMenus()
{
   menuItems.clear();
   menuItems.push_back(MenuItem("RETURN TO EDITOR", 0, KEY_R, KEY_UNKNOWN));
   if(gIniSettings.fullscreen)
      menuItems.push_back(MenuItem("GAME MODE: WINDOWED",   1, KEY_G, KEY_UNKNOWN));
   else
      menuItems.push_back(MenuItem("GAME MODE: FULLSCREEN", 1, KEY_G, KEY_UNKNOWN));

   menuItems.push_back(MenuItem("TEST LEVEL",   2, KEY_T, KEY_UNKNOWN));
   menuItems.push_back(MenuItem("SAVE LEVEL",   3, KEY_S, KEY_UNKNOWN));
   menuItems.push_back(MenuItem("INSTRUCTIONS", 4, KEY_I, keyHELP));
   menuItems.push_back(MenuItem("LEVEL PARAMETERS", 9, KEY_L, KEY_F3));
   menuItems.push_back(MenuItem("MANAGE TEAMS", 5, KEY_M, KEY_F2));
   menuItems.push_back(MenuItem("QUIT",         6, KEY_Q, KEY_UNKNOWN));
}

void EditorMenuUserInterface::processSelection(U32 index)
{
   switch(index)
   {
      case 0:
         //gEditorUserInterface.activate(false);
         reactivatePrevUI();
         break;
      case 1:
         gOptionsMenuUserInterface.toggleFullscreen();
         setupMenus();
         break;
      case 2:
         gEditorUserInterface.testLevel();
         break;
      case 3:
         gEditorUserInterface.saveLevel(true, true);  // Save level
         gEditorUserInterface.setSaveMessage("Saved " + gEditorUserInterface.getLevelFileName(), true);
         reactivatePrevUI();        // Return to editor
         break;
      case 4:                                         // Instructions
         gEditorInstructionsUserInterface.activate();
         break;
      case 9:
         gGameParamUserInterface.activate();
         break;
      case 5:                                         // Team editor
         gTeamDefUserInterface.activate();
         break;
      case 6:
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
            reactivateMenu(gMainMenuUserInterface);

         gEditorUserInterface.clearUndoHistory();  // Clear up a little memory
         break;
   }
}

void EditorMenuUserInterface::processShiftSelection(U32 index)
{
   processSelection(index);
}

void EditorMenuUserInterface::onEscape()
{
   glutSetCursor(GLUT_CURSOR_NONE);
   reactivatePrevUI();
}

void EditorMenuUserInterface::render()
{
   Parent::render();
}

//// Constructor
//MeshBox::MeshBox(Point center, F32 size)
//{
//   bounds = Rect(center, size);
//   done[all] = done[up] = done[down] = done[left] = done[right] = false;
// //  boxCount++;
//}
//
//
//void MeshBox::grow(Direction d)
//{
//   F32 amt = .01;
//   switch(d)
//   {
//       case up:
//          if(!done[up])
//            bounds.min.y -= amt;
//         break;
//      case down:
//         if(!done[down])
//            bounds.max.y += amt;
//         break;
//      case left:
//         if(!done[left])
//            bounds.min.x -= amt;
//         break;
//      case right:
//         if(!done[right])
//            bounds.max.x += amt;
//         break;
//   }
//}
//
//void MeshBox::revert(Direction d)
//{
//   F32 amt = .01;
//   switch(d)
//   {
//      case up:
//         bounds.min.y += amt;
//         done[up] = true;
//         checkAllDone();
//         break;
//      case down:
//         bounds.max.y -= amt;
//         done[down] = true;
//         checkAllDone();
//         break;
//      case left:
//         bounds.min.x += amt;
//         done[left] = true;
//         checkAllDone();
//         break;
//      case right:
//         bounds.max.x -= amt;
//         done[right] = true;
//         checkAllDone();
//         break;
//   }
//}
//
//void MeshBox::init()
//{
//   //boxCount = 0;
//   //doneCount = 0;
//}
//
//// See if the given box hits a wall
//bool MeshBox::checkWallCollision()
//{
//   for(S32 i = 0; i < gEditorUserInterface.mItems.size(); i++)
//   {
//      if(gEditorUserInterface.mItems[i].index == EditorUserInterface::ItemBarrierMaker)      // It's a wall
//      {
//         for(S32 j = 0; j < gEditorUserInterface.mItems[i].verts.size(); j++)
//         {
//            // Contains a vertex
//            Point v = gEditorUserInterface.mItems[i].verts[j];
//            if(bounds.contains(v))
//               return true;
//            // Contains an edge
//            if(j < gEditorUserInterface.mItems[i].verts.size() - 1 && bounds.intersects(gEditorUserInterface.mItems[i].verts[j], gEditorUserInterface.mItems[i].verts[j+1]))
//               return true;
//         }
//      }
//   }
//   return false;
//}
//
//void MeshBox::checkAllDone()
//{
//   if(done[up] && done[down] && done[left] && done[right] && !done[all])
//   {
//      done[all] = true;
//      //doneCount++;
//   }
//}

};
