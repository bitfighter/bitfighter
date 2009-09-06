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

#ifndef _UIEDITOR_H_
#define _UIEDITOR_H_

#include "UIMenus.h"
#include "gameLoader.h"
#include "timer.h"
#include "point.h"
#include "tnlNetStringTable.h"
#include <string>

using namespace std;

namespace Zap
{

#define nameLen 256
#define gameTypeLen 256

enum VertexRenderStyles
{
   HighlightedVertex,               // Highlighted vertex
   SelectedItemVertex,              // Non-highlighted vertex of a selected item
   UnselectedItemVertex,            // Non-highlighted vertex of a non-selected item
};


//class MeshBox
//{
//public:
//
//   enum Direction {
//      all,
//      up,
//      down,
//      left,
//      right,
//      directions
//   };
//
//   MeshBox() { };
//   MeshBox(Point center, F32 size);    // Constructor
//   Rect bounds;
//   void grow(Direction dir);
//   void revert(Direction dir);
//   bool done[directions];
//   void checkAllDone();
//   static S32 doneCount;
//   static S32 boxCount;
//   static void init();
//   void render();
//   bool checkWallCollision();
//};

class EditorUserInterface : public UserInterface, public LevelLoader
{
public:
   enum GameItems    // Remember to keep these properly aligned with gGameItemRecs[]
   {
      ItemSpawn,
      ItemSpeedZone,
      ItemSoccerBall,
      ItemFlag,
      ItemFlagSpawn,
      ItemBarrierMaker,
      ItemTeleporter,
      ItemRepair,
      ItemBouncyBall,
      ItemAsteroid,
      ItemMine,
      ItemSpyBug,
      ItemResource,
      ItemLoadoutZone,
      ItemNexus,
      ItemSlipZone,
      ItemTurret,
      ItemForceField,
      ItemGoalZone,
      ItemTextItem,
      ItemNavMeshZone,
   };

   struct WorldItem
   {
      GameItems index;
      S32 team;
      F32 width;
      Vector<Point> verts;
      bool selected;
      Vector<bool> vertSelected;
      string text;         // For items that have an aux text field
      U32 textSize;        // For items that have an aux text field
      S32 repopDelay;      // For repair items, also used for engineered objects heal rate
      S32 speed;           // Speed for speedzone items
      bool boolattr;       // Additional optional boolean attribute for some items (only speedzone so far...)

      WorldItem()    // Constructor
      {
         repopDelay = -1;
         speed = -1;
         boolattr = false;
      }
   };

private:
   string mSaveMsg;
   Color mSaveMsgColor;

   string mWarnMsg1;
   string mWarnMsg2;
   Color mWarnMsgColor;

   bool snapDisabled;
   bool showAllObjects;
   enum {
      saveMsgDisplayTime = 4000,
      warnMsgDisplayTime = 4000,
   };

   Timer mSaveMsgTimer;
   Timer mWarnMsgTimer;

   Vector<Vector<WorldItem> > mUndoItems;    // Undo history
   Vector<Vector<WorldItem> > mRedoItems;    // Redo history
   Vector<WorldItem> mMostRecentState;       // Copy of most recent state, to facilitate dragging
   Vector<WorldItem> mUnmovedItems;          // Copy of items where they were before they moved... different than mMostRecentState when dragging from dock

   void saveUndoState(Vector<WorldItem> items);    // Save current state into undo history buffer

   //Vector<Vector<const char *> > mUnknownItems;    // Items from level file we can't parse
   Vector<WorldItem> mDockItems;                   // Items sitting in the dock
   Vector<WorldItem> mClipboard;                   // Items on clipboard

   void saveSelection();               // Save selection mask
   void restoreSelection();            // Restore selection mask
   Vector<WorldItem> mSelectedSet;     // Place to store selection mask

   S32 itemToLightUp;
   S32 vertexToLightUp;

   string mEditFileName;               // Manipulate with get/setLevelFileName

   bool mDraggingObjects;
   S32 mEditingTextItem;               // Index of item we're editing text of
   WorldItem mNewItem;
   F32 mCurrentScale;
   Point mCurrentOffset;
   Point mMousePos;
   Point mMouseDownPos;

   void renderGenericItem(Point pos, Color c);

   bool mCreatingPoly;
   bool mDragSelecting;
   bool mShowingReferenceShip;
   S32 mDraggingDockItem;
   bool itemCameFromDock;
   Vector<string> mLevelErrorMsgs;

   bool mUp, mDown, mLeft, mRight, mIn, mOut;

   void clearSelection();        // Mark all objects and vertices as unselected
   void clearDockSelection();    // Mark all objects on dock as unselected

   void centerView();            // Center display on all objects
   void splitBarrier();          // Split wall on selected vertex/vertices
   void joinBarrier();           // Join barrier bits together into one (if ends are coincident)

   S32 countSelectedItems();
   S32 countSelectedVerts();
   void findHitVertex(Point canvasPos, S32 &hitItem, S32 &hitVertex);
   void findHitItemAndEdge(Point canvasPos, S32 &hitItem, S32 &hitEdge);
   void findHitItemOnDock(Point canvasPos, S32 &hitItem);

   void computeSelectionMinMax(Point &min, Point &max);
   bool mouseOnDock();           // Return whether mouse is currently over the dock

   void processLevelLoadLine(int argc, const char **argv);

   void insertNewItem(GameItems itemType);                                                    // Insert a new object into the game
   WorldItem constructItem(GameItems itemType, Point pos, S32 team, F32 width, F32 height);   // Construct a new object

   Color getTeamColor(S32 team);    // Return a color based on team index

   string mgLevelDir;
   Vector<StringTableEntry> mgLevelList;
   bool mWasTesting;

public:
   EditorUserInterface();           // Constructor

   struct Team    // Pretty basic: correlates a name and a color
   {
      char name[nameLen];
      Color color;
   };

   void setLevelFileName(string name);
   string getLevelFileName();
   void loadLevel();
   bool mNeedToSave;          // Have we modified the level such that we need to save?
   S32 mAllUndoneUndoLevel;   // What undo level reflects everything back just the

   char mGameType[gameTypeLen];
   Vector<S32> mGameTypeArgs;

   bool isFlagGame(char *mGameType);
   bool isTeamFlagGame(char *mGameType);

   void clearUndoHistory();      // Wipe undo/redo history

   Vector<Team> mTeams;          // Team list: needs to be public so we can edit from UITeamDefMenu
   Vector<Team> mOldTeams;       // Team list from before we run team editor, so we can see if anything has changed

   Vector<WorldItem> mItems;     // Item list: needs to be public so we can check team affiliation from UITeamDefMenu

   void render();
   void renderItem(WorldItem &i, S32 itemID, bool isDockItem);
   void renderLinePolyVertices(WorldItem item, S32 itemID, bool isDockItem);

   void renderBarrier(Vector<Point> verts, bool selected, F32 width, bool isDockItem);
   void renderPoly(Vector<Point> verts, bool isDockItem);
   static void renderVertex(VertexRenderStyles style, Point v, S32 number, S32 size = 5);


   // Handle input
   void onKeyDown(KeyCode keyCode, char ascii);    // Handles all keyboard inputs, mouse clicks, and button presses
   void onKeyUp(KeyCode keyCode);
   void onMouseMoved(S32 x, S32 y);
   void onMouseDragged(S32 x, S32 y);

   void onActivate();
   void onReactivate();

   void populateDock();                         // Load up dock with game-specific items to drag and drop

   F32 mGridSize;

   void idle(U32 timeDelta);
   void deleteSelection(bool objectsOnly);      // Delete selected items (true = items only, false = items & vertices)
   void copySelection();                        // Copy selection to clipboard
   void pasteSelection();                       // Paste selection from clipboard
   void setCurrentTeam(S32 currentTeam);        // Set current team for selected items, also sets team for all dock items
   void flipSelectionVertical();                // Flip selection along vertical axis
   void flipSelectionHorizontal();              // Flip selection along horizontal axis
   void rotateSelection(F32 angle);             // Rotate selecton by angle

   void validateLevel();               // Check level for things that will make the game crash!
   void validateTeams();               // Check that each item has a valid team (and fix any errors found)
   void teamsHaveChanged();            // Another team validation routine, used when all items have valid teams, but the teams themselves change
   void makeSureThereIsAtLeastOneTeam();

   void incBarrierWidth(S32 amt);      // Increase selected wall thickness by amt
   void decBarrierWidth(S32 amt);      // Decrease selected wall thickness by amt

   bool saveLevel(bool showFailMessages, bool showSuccessMessages);
   void testLevel();
   void setSaveMessage(string msg, bool savedOK);
   void setWarnMessage(string msg1, string msg2);

   Point convertWindowToCanvasCoord(Point p) { return Point(p.x * canvasWidth / windowWidth, p.y * canvasHeight / windowHeight); }
   Point convertCanvasToLevelCoord(Point p) { return (p - mCurrentOffset) * (1 / mCurrentScale); }
   Point convertLevelToCanvasCoord(Point p) { return p * mCurrentScale + mCurrentOffset; }
   Point snapToLevelGrid(Point p);

   //MeshBox testBox[5][5];  //TODO: del

};

class EditorMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   EditorMenuUserInterface();    // Constructor
   void render();
   void onActivate();
   void setupMenus();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
};

extern EditorUserInterface gEditorUserInterface;
extern EditorMenuUserInterface gEditorMenuUserInterface;





};

#endif

