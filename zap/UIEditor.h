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

class TeamEditor;

enum VertexRenderStyles
{
   SnappingVertex,                  // Vertex that indicates snapping point
   HighlightedVertex,               // Highlighted vertex
   SelectedItemVertex,              // Non-highlighted vertex of a selected item
   UnselectedItemVertex,            // Non-highlighted vertex of a non-selected item
};


// From http://stackoverflow.com/questions/134569/c-exception-throwing-stdstring
struct SaveException : public std::exception
{
   string msg;

   SaveException(string str) : msg(str) { /* do nothing */ }    // Constructor
   ~SaveException() throw() { /* do nothing */ }                // Destructor, needed to avoid "looser throw specifier" errors with gcc
   const char* what() const throw() { return msg.c_str(); }
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

enum GameItems    // Remember to keep these properly aligned with gGameItemRecs[]
{
   ItemSpawn,
   ItemSpeedZone,
   ItemSoccerBall,
   ItemFlag,
   ItemFlagSpawn,
   ItemBarrierMaker,
   ItemLineItem,
   ItemTeleporter,
   ItemRepair,
   ItemEnergy,
   ItemBouncyBall,
   ItemAsteroid,
   ItemAsteroidSpawn,
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

enum GeomType {
   geomPoint,           // Single point feature (like a flag)
   geomSimpleLine,      // Two point line (like a teleport)
   geomLine,            // Many point line (like a wall)
   geomPoly,            // Polygon feature (like a loadout zone)
   geomNone,            // Other/unknown (not used, just here for completeness)
};

class WorldItem
{

public:
   WorldItem() { /* do nothing */ }                                                        // Generic constructor
   WorldItem(GameItems itemType, Point pos, S32 team, F32 width, F32 height, U32 id = 0);  // Primary constructor
   WorldItem(const WorldItem &worldItem);                                                  // Copy constructor

   GameItems index;
   S32 team;
   F32 width;
   U32 id;              // Item's unique id... 0 if there is none
   Vector<Point> verts;
   bool selected;
   bool litUp;
   Vector<bool> vertSelected;
   LineEditor lineEditor; // For items that have an aux text field
   U32 textSize;          // For items that have an aux text field
   S32 repopDelay;        // For repair items, also used for engineered objects heal rate
   S32 speed;             // Speed for speedzone items
   bool boolattr;         // Additional optional boolean attribute for some items (only speedzone so far...)

   bool hasWidth();
   GeomType geomType();
};


class ItemSelection
{
public:
   ItemSelection() { /* do nothing */ }      // Generic constructor
   ItemSelection(WorldItem &item);           // Primary constructor

private:
   bool selected;
   Vector<bool> vertSelected;

public:
   void restore(WorldItem &item);
};


class Selection
{
public:
   Selection() { /* do nothing */ }          // Generic constructor
   Selection(Vector<WorldItem> &items);      // Primary constructor

private:
   Vector<ItemSelection> selection;

public:
   void restore(Vector<WorldItem> &items);
};

//class EditorCommand
//{
//public:
//    virtual bool Execute();
//    virtual bool Undo();
//    virtual ~EditorCommand() { }
//};
//
//class PlaceItemCommand : public EditorCommand
//{
//   PlaceItemCommand(WorldItem item, Point pos) : mItem(item), mPos(pos) { }
//
//   WorldItem mItem;
//   Point mPos;
//
//   bool Execute();
//   bool Undo();
//};



class EditorUserInterface : public UserInterface, public LevelLoader
{
public:
   EditorUserInterface();  // Constructor

   enum SpecialAttribute   // Some items have special attributes.  These are the ones
   {                       // we can edit in the editor
      Text = 0,
      RepopDelay,
      GoFastSpeed,
      GoFastSnap,
      None                 // Must be last
   };

private:
   string mSaveMsg;
   Color mSaveMsgColor;

   string mWarnMsg1;
   string mWarnMsg2;
   Color mWarnMsgColor;

   S32 mCurrentTeam;

   bool snapDisabled;

   enum ShowMode
   {
      ShowAllObjects,
      ShowWallsOnly,
      ShowAllButNavZones,
      ShowModesCount
   };

   ShowMode mShowMode;
   bool mHasBotNavZones;

   enum {
      saveMsgDisplayTime = 4000,
      warnMsgDisplayTime = 4000,
   };

   Timer mSaveMsgTimer;
   Timer mWarnMsgTimer;

   Vector<Vector<WorldItem> > mUndoItems;    // Undo/redo history  [[note that g++ requires space btwn >>]]
   Vector<WorldItem> mMostRecentState;       // Copy of most recent state, to facilitate dragging
   Vector<WorldItem> mUnmovedItems;          // Copy of items where they were before they moved... different than mMostRecentState when dragging from dock

   Vector<WorldItem> mLevelGenItems;         // Items added by a levelgen script

   U32 mFirstUndoIndex;
   U32 mLastUndoIndex;
   U32 mLastRedoIndex;
   bool mRedoingAnUndo;

   static const U32 UNDO_STATES = 128;
   void saveUndoState(Vector<WorldItem> items, bool cameFromRedo = false);    // Save current state into undo history buffer
   bool undoAvailable();               // Is an undo state available?
   void undo(bool addToRedoStack);     // Restore mItems to latest undo state
   void redo();                        // Redo latest undo

   void autoSave();                    // Hope for the best, prepare for the worst

   Vector<WorldItem> mDockItems;       // Items sitting in the dock
   Vector<WorldItem> mClipboard;       // Items on clipboard

   void saveSelection();               // Save selection mask
   void restoreSelection();            // Restore selection mask
   Selection mSelectedSet;             // Place to store selection mask

   S32 itemToLightUp;
   S32 vertexToLightUp;

   string mEditFileName;               // Manipulate with get/setLevelFileName

   bool mDraggingObjects;

   S32 mEditingSpecialAttrItem;        // Index of item we're editing special attributes on
   SpecialAttribute mSpecialAttribute; // Type of special attribute we're editing

   void doneEditingSpecialItem(bool save);    // Gets run when user exits special-item editing mode
   U32 getNextAttr(S32 item);                 // Assist on finding the next attribute this item is capable of editing,
                                              // for cycling through the various editable attributes
   WorldItem mNewItem;
   F32 mCurrentScale;
   Point mCurrentOffset;
   Point mMousePos;
   Point mMouseDownPos;

   void renderGenericItem(Point pos, Color c, F32 alpha);
   F32 renderTextItem(WorldItem &item, F32 alpha);          // Returns size of text
   void setTranslationAndScale(Point pos);

   bool mCreatingPoly;
   bool mCreatingPolyline;
   bool mDragSelecting;
   bool mShowingReferenceShip;
   bool editingIDMode;

   S32 mDraggingDockItem;
   Vector<string> mLevelErrorMsgs;

   bool mUp, mDown, mLeft, mRight, mIn, mOut;

   void clearSelection();        // Mark all objects and vertices as unselected
   void unselectItem(S32 i);     // Mark item and vertices as unselected

   void centerView();            // Center display on all objects
   void splitBarrier();          // Split wall on selected vertex/vertices
   void joinBarrier();           // Join barrier bits together into one (if ends are coincident)

   S32 countSelectedItems();
   S32 countSelectedVerts();
   bool anyItemsSelected();           // Are any items selected?
   bool anyItemsOrVertsSelected();    // Are any items/vertices selected?

   void findHitVertex(Point canvasPos, S32 &hitItem, S32 &hitVertex);
   void findHitItemAndEdge();         // Sets mItemHit and mEdgeHit
   S32 findHitItemOnDock(Point canvasPos);

   Point findClosestPointInSelection();   //<=====  TODO: Delete

   void findSnapVertex();
   S32 mSnapVertex_i;
   S32 mSnapVertex_j;

   S32 mEdgeHit, mItemHit;

   void computeSelectionMinMax(Point &min, Point &max);
   bool mouseOnDock();                // Return whether mouse is currently over the dock

   void processLevelLoadLine(U32 argc, U32 id, const char **argv);

   void insertNewItem(GameItems itemType);                                                    // Insert a new object into the game

   Vector<StringTableEntry> mgLevelList;
   bool mWasTesting;

   bool mUnselectVertexAfterDrag;      // <===== TODO: Delete
   S32 mUnselectVertexAfterDrag_i;
   S32 mUnselectVertexAfterDrag_j;

   bool anySelected();

public:
   void setLevelFileName(string name);
   void setLevelGenScriptName(string name);

   string getLevelFileName();
   void loadLevel();
   bool mNeedToSave;          // Have we modified the level such that we need to save?
   U32 mAllUndoneUndoLevel;   // What undo level reflects everything back just the

   char mGameType[gameTypeLen];
   Vector<S32> mGameTypeArgs;

   Color getTeamColor(S32 team);     // Return a color based on team index (needed by editor instructions)
   bool isFlagGame(char *mGameType);
   bool isTeamFlagGame(char *mGameType);

   void clearUndoHistory();      // Wipe undo/redo history

   Vector<TeamEditor> mTeams;          // Team list: needs to be public so we can edit from UITeamDefMenu
   Vector<TeamEditor> mOldTeams;       // Team list from before we run team editor, so we can see if anything has changed

   Vector<WorldItem> mItems;     // Item list: needs to be public so we can check team affiliation from UITeamDefMenu

   void render();
   void renderItem(WorldItem &item, S32 index, bool isBeingEdited, bool isDockItem, bool isScriptItem);
   void renderLinePolyVertices(WorldItem &item, S32 index, F32 alpha);

   void renderPolyline(GameItems itemType, Vector<Point> verts, bool selected, S32 team, F32 width, F32 alpha = 1.0, bool convert = true);   // Render walls & lineItems
   void renderPoly(Vector<Point> verts, bool isDockItem);
   static void renderVertex(VertexRenderStyles style, Point v, S32 number, F32 alpha = 1, S32 size = 5);


   // Handle input
   void onKeyDown(KeyCode keyCode, char ascii);    // Handles all keyboard inputs, mouse clicks, and button presses
   void onKeyUp(KeyCode keyCode);
   void onMouseMoved(S32 x, S32 y);
   void onMouseDragged(S32 x, S32 y);

   void onActivate();
   void onReactivate();

   void populateDock();                         // Load up dock with game-specific items to drag and drop

   F32 mGridSize;

   string mScriptLine;                          // Script and args, if any


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

   static S32 getDefaultRepopDelay(GameItems itemType);

   bool saveLevel(bool showFailMessages, bool showSuccessMessages, bool autosave = false);
   void testLevel();
   void setSaveMessage(string msg, bool savedOK);
   void setWarnMessage(string msg1, string msg2);

   Point convertCanvasToLevelCoord(Point p) { return (p - mCurrentOffset) * (1 / mCurrentScale); }
   Point convertLevelToCanvasCoord(Point p) { return p * mCurrentScale + mCurrentOffset; }
   Point snapToLevelGrid(Point const &p, bool snapWhileOnDock = false);

   void runScript();    // Run associated levelgen script


   //MeshBox testBox[5][5];  //TODO: del

};


class EditorMenuUserInterface : public MenuUserInterface
{
public:
   EditorMenuUserInterface();    // Constructor

private:
   typedef MenuUserInterface Parent;

public:
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


