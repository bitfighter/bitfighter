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
#include "EditorObject.h"

#include "gameLoader.h"
#include "gameObject.h"          // For EditorObject definition
#include "gridDB.h"              // For DatabaseObject definition
#include "timer.h"
#include "Point.h"
#include "BotNavMeshZone.h"      // For Border def
#include "tnlNetStringTable.h"
#include "barrier.h"             // For wall related defs (WallSegmentManager, etc.)
#include "Colors.h"

#include <string>

#include <boost/shared_ptr.hpp>

using namespace std;

namespace Zap
{

class TeamEditor;

enum VertexRenderStyles
{
   SnappingVertex,                  // Vertex that indicates snapping point
   HighlightedVertex,               // Highlighted vertex
   SelectedVertex,                  // Vertex itself is selected
   SelectedItemVertex,              // Non-highlighted vertex of a selected item
   UnselectedItemVertex,            // Non-highlighted vertex of a non-selected item
};


////////////////////////////////////////
////////////////////////////////////////

//class SelectionItem
//{
//private:
//   bool mSelected;
//   Vector<bool> mVertSelected;
//
//public:
//   SelectionItem() { /* Do nothing */ }      // Generic constructor
//   SelectionItem(EditorObject *item);        // Primary constructor
//
//   void restore(EditorObject *item);
//};
//
//
//////////////////////////////////////////
//////////////////////////////////////////
//
//class EditorObject;
//
//class Selection
//{
//public:
//   Selection() { /* Do nothing */ }            // Generic constructor
//   Selection(Vector<EditorObject *> &items);   // Primary constructor
//
//private:
//   Vector<SelectionItem> mSelection;
//
//public:
//   void restore(Vector<EditorObject *> &items);
//};
//
//
////////////////////////////////////////
////////////////////////////////////////

static const Color *HIGHLIGHT_COLOR = &Colors::white;
static const Color *SELECT_COLOR = &Colors::yellow;

////////////////////////////////////////
////////////////////////////////////////

class EditorAttributeMenuUI;

class EditorUserInterface : public UserInterface
{
   typedef UserInterface Parent;

public:
   EditorUserInterface(Game *game);  // Constructor

   enum SpecialAttribute   // Some items have special attributes.  These are the ones
   {                       // we can edit in the editor
      Text = 0,
      RepopDelay,
      GoFastSpeed,
      GoFastSnap,
      NoAttribute                 // Must be last
   };

private:
   string mSaveMsg;
   Color mSaveMsgColor;

   string mWarnMsg1;
   string mWarnMsg2;
   Color mWarnMsgColor;

   S32 mCurrentTeam;

   bool mSnapDisabled;

   ShowMode mShowMode;

   enum {
      saveMsgDisplayTime = 4000,
      warnMsgDisplayTime = 4000,
   };

   Timer mSaveMsgTimer;
   Timer mWarnMsgTimer;

   const Vector<EditorObject *> *getObjectList();     // Convenience method


   Vector<boost::shared_ptr<EditorObjectDatabase> > mUndoItems;  // Undo/redo history 
   Point mMoveOrigin;                           // Point representing where items were moved "from" for figuring out how far they moved
   Vector<Point> mOriginalVertLocations;

   Vector<boost::shared_ptr<EditorObject> > mLevelGenItems;       // Items added by a levelgen script
   Vector<boost::shared_ptr<EditorObject> > mDockItems;    // Items sitting in the dock

   U32 mFirstUndoIndex;
   U32 mLastUndoIndex;
   U32 mLastRedoIndex;
   bool mRedoingAnUndo;

   static const U32 UNDO_STATES = 128;
   void deleteUndoState();             // Removes most recent undo state from stack
   bool undoAvailable();               // Is an undo state available?
   void undo(bool addToRedoStack);     // Restore mItems to latest undo state
   void redo();                        // Redo latest undo

   void autoSave();                    // Hope for the best, prepare for the worst

   Vector<boost::shared_ptr<EditorObject> > mClipboard;    // Items on clipboard

   bool mLastUndoStateWasBarrierWidthChange;

   EditorObject *mItemToLightUp;
 
   string mEditFileName;            // Manipulate with get/setLevelFileName

   EditorObject *mNewItem;
   F32 mCurrentScale;
   Point mCurrentOffset;            // Coords of UR corner of screen

   Point mMousePos;                 // Where the mouse is at the moment
   Point mMouseDownPos;             // Where the mouse was pressed for a drag operation

   bool showMinorGridLines();
   void renderGrid();               // Draw background snap grid
   void renderDock(F32 width);
   void renderTextEntryOverlay();
   void renderReferenceShip();

   bool mCreatingPoly;
   bool mCreatingPolyline;
   bool mDragSelecting;
   bool mAddingVertex;
   bool mShowingReferenceShip;
   LineEditor mEntryBox;

   S32 mDraggingDockItem;
   Vector<string> mLevelErrorMsgs, mLevelWarnings;

   bool mUp, mDown, mLeft, mRight, mIn, mOut;

   void clearSelection();        // Mark all objects and vertices as unselected

   void centerView();            // Center display on all objects
   void splitBarrier();          // Split wall on selected vertex/vertices
   void joinBarrier();           // Join barrier bits together into one (if ends are coincident)

   //S32 countSelectedVerts();
   bool anyItemsSelected();           // Are any items selected?
   bool anythingSelected();           // Are any items/vertices selected?

   //void findHitVertex(EditorObject *&hitObject, S32 &hitVertex);
   void findHitItemAndEdge();         // Sets mItemHit and mEdgeHit
   S32 findHitItemOnDock(Point canvasPos);

   void findSnapVertex();
   EditorObject *mSnapObject;
   S32 mSnapVertexIndex;

   S32 mEdgeHit;
   S32 mVertexHit;
   EditorObject *mItemHit;

   void computeSelectionMinMax(Point &min, Point &max);
   bool mouseOnDock();                // Return whether mouse is currently over the dock
   bool mNeedToSave;                  // Have we modified the level such that we need to save?

   //void processLevelLoadLine(U32 argc, U32 id, const char **argv);

   void insertNewItem(GameObjectType itemType);                      // Insert a new object into the game

   bool mWasTesting;

   void onFinishedDragging();    // Called when we're done dragging an object

   void resnapAllEngineeredItems();

protected:
   void onActivate();
   void onReactivate();
   void onDeactivate();

public:
   ~EditorUserInterface();       // Destructor

   void clearDatabase(GridDatabase *database);

   void setLevelFileName(string name);
   void setLevelGenScriptName(string name);

   string getLevelFileName();
   void loadLevel();
   U32 mAllUndoneUndoLevel;   // What undo level reflects everything back just the

   void saveUndoState();
   void restoreItems(const Vector<string> &from);

   Vector<string> mGameTypeArgs;

   bool isShowingReferenceShip() { return mShowingReferenceShip; }

   F32 getCurrentScale() { return mCurrentScale; }
   Point getCurrentOffset() { return mCurrentOffset; }

   void doneEditingAttributes(EditorAttributeMenuUI *editor, EditorObject *object);   // Gets run when user exits attribute editor

   void clearUndoHistory();        // Wipe undo/redo history

   Vector<TeamInfo> mOldTeams;     // Team list from before we run team editor, so we can see what changed

   EditorObject *getSnapItem() { return mSnapObject; }
   S32 getSnapVertexIndex() { return mSnapVertexIndex; }
   void rebuildEverything();                  // Does lots of things in undo, redo, and add items from script

   void onBeforeRunScriptFromConsole();
   void onAfterRunScriptFromConsole();

   void render();
   void renderPolyline(const Vector<Point> *verts);

   void setLevelToCanvasCoordConversion();


   const Color *getTeamColor(S32 teamId) const;

   bool getNeedToSave() const { return mNeedToSave; }
   void setNeedToSave(bool needToSave) { mNeedToSave = needToSave; }

   bool mDraggingObjects;     // Should be private

   // Handle input
   void onKeyDown(KeyCode keyCode, char ascii);             // Handle all keyboard inputs, mouse clicks, and button presses
   void textEntryKeyHandler(KeyCode keyCode, char ascii);   // Handle keyboard activity when we're editing an item's attributes
   void specialAttributeKeyHandler(KeyCode keyCode, char ascii);
   void startAttributeEditor();

   void onKeyUp(KeyCode keyCode);
   void onMouseMoved(S32 x, S32 y);
   void onMouseMoved();
   void onMouseDragged();
   void startDraggingDockItem();
   EditorObject *copyDockItem(S32 index);
   bool mouseIgnore;

   void populateDock();                         // Load up dock with game-specific items to drag and drop
   void addToDock(EditorObject* object);
   void addDockObject(EditorObject *object, F32 xPos, F32 yPos);

   string mScriptLine;                          // Script and args, if any

   void idle(U32 timeDelta);
   void deleteSelection(bool objectsOnly);      // Delete selected items (true = items only, false = items & vertices)
   void copySelection();                        // Copy selection to clipboard
   void pasteSelection();                       // Paste selection from clipboard
   void setCurrentTeam(S32 currentTeam);        // Set current team for selected items, also sets team for all dock items
   void flipSelectionVertical();                // Flip selection along vertical axis
   void flipSelectionHorizontal();              // Flip selection along horizontal axis
   void scaleSelection(F32 scale);              // Scale selection by scale
   void rotateSelection(F32 angle);             // Rotate selecton by angle

   void validateLevel();               // Check level for things that will make the game crash!
   void validateTeams();               // Check that each item has a valid team (and fix any errors found)
   void validateTeams(const Vector<DatabaseObject *> &dbObjects);

   void teamsHaveChanged();            // Another team validation routine, used when all items have valid teams, but the teams themselves change
   void makeSureThereIsAtLeastOneTeam();

   void changeBarrierWidth(S32 amt);   // Increase selected wall thickness by amt

   bool saveLevel(bool showFailMessages, bool showSuccessMessages, bool autosave = false);
   void testLevel();
   void testLevelStart();
   void setSaveMessage(string msg, bool savedOK);
   void setWarnMessage(string msg1, string msg2);

   Point convertCanvasToLevelCoord(Point p) { return (p - mCurrentOffset) * (1 / mCurrentScale); }
   Point convertLevelToCanvasCoord(Point p, bool convert = true) { return convert ? p * mCurrentScale + mCurrentOffset : p; }

   void onPreDisplayModeChange();   // Called when we shift between windowed and fullscreen mode, before change is made
   void onDisplayModeChange();      // Called when we shift between windowed and fullscreen mode, after change is made

   // Snapping related functions:
   Point snapPoint(Point const &p, bool snapWhileOnDock = false);
   Point snapPointToLevelGrid(Point const &p);

   bool getSnapToWallCorners();     // Returns true if wall corners are active snap targets

   static void renderSnapTarget(const Point &target);


   static S32 checkEdgesForSnap(const Point &clickPoint, const Vector<Point> &points, bool abcFormat, F32 &minDist, Point &snapPoint);
   static S32 checkEdgesForSnap(const Point &clickPoint,  const Vector<WallEdge *> &edges, bool abcFormat, F32 &minDist, Point &snapPoint);

   S32 checkCornersForSnap(const Point &clickPoint,  const Vector<WallEdge *> &edges, F32 &minDist, Point &snapPoint);

   void deleteItem(S32 itemIndex);

   void runScript(const string &scriptName, const Vector<string> &args);        
   void runLevelGenScript();              // Run associated levelgen script
   void copyScriptItemsToEditor();        // Insert these items into the editor as first class items that can be manipulated or saved
   void clearLevelGenItems();             // Clear any previously created levelgen items
};


////////////////////////////////////////
////////////////////////////////////////

class EditorMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

private:
   void setupMenus();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();

protected:
   void onActivate();

public:
   EditorMenuUserInterface(Game *game);    // Constructor
};


};

#endif
