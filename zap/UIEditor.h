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
//#include "UIEditorMenus.h"       // needed for editorObject only?

#include "gameLoader.h"
#include "gameObject.h"          // For EditorObject definition
#include "gridDB.h"              // For DatabaseObject definition
#include "timer.h"
#include "Point.h"
#include "BotNavMeshZone.h"      // For Border def
#include "tnlNetStringTable.h"
#include "pointainer.h"
#include <string>
#include <vector>

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

enum ShowMode
{
   ShowAllButNavZones,
   ShowAllObjects,
   ShowWallsOnly,
   NavZoneMode,
   ShowModesCount
};


////////////////////////////////////////
////////////////////////////////////////

class WallSegment : public DatabaseObject
{
private:
   static GridDatabase *mGridDatabase;
   GridDatabase *getGridDatabase() { return mGridDatabase; }      

   void init(S32 owner);

public:
   WallSegment(const Point &start, const Point &end, F32 width, S32 owner = -1);    // Normal wall segment
   WallSegment(const Vector<Point> &points, S32 owner = -1);                        // PolyWall 
   ~WallSegment();

   Vector<Point> edges;    
   Vector<Point> corners;
   Vector<Point> triangulatedFillPoints;
   U32 mOwner;

   bool invalid;              // A flag for marking segments in need of processing

   void resetEdges();         // Compute basic edges from corner points
   void computeBoundingBox(); // Computes bounding box based on the corners, updates database
   
   void renderFill(bool renderLight);

   ////////////////////
   //  DatabaseObject methods
   static void setGridDatabase(GridDatabase *database) { mGridDatabase = database; }


   // Note that the poly returned here is different than what you might expect -- it is composed of the edges,
   // not the corners, and is thus in A-B, C-D, E-F format rather than the more typical A-B-C-D format returned
   // by getCollisionPoly() elsewhere in the game.  Therefore, it needs to be handled differently.
   bool getCollisionPoly(Vector<Point> &polyPoints) { polyPoints = edges; return true; }  
   bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) { return false; }
};

////////////////////////////////////////
////////////////////////////////////////

enum GeomType {           
   geomPoint,           // ype = BIT(Single point feature (like a flag)
   geomSimpleLine,      // = BIT(28),Two point line (like a teleport)
   geomLine,            // Many point line (like a wall)
   geomPoly,            // BIT(30),Polygon feature (like a loadout zone)
   geomNone,            // BIT(31),  Other/unknown (not used, just here for completeness)
};


// Width of line representing centerline of barriers
#define WALL_SPINE_WIDTH gLineWidth3



class WorldItem : public DatabaseObject
{  
private:
   Vector<Point> mVerts;


   void init(GameObjectType itemType, S32 xteam, F32 xwidth, U32 itemid, bool isDockItem);

   static GridDatabase *mGridDatabase;

   Game *mGame;

public:
   WorldItem(GameObjectType itemType = UnknownType, S32 itemId = 0);    // Only used when creating an item from a loaded level
   WorldItem(GameObjectType itemType, Point pos, S32 team, bool isDockItem, F32 width = 1, F32 height = 1, U32 id = 0);  // Primary constructor


   ////////////////////////////
   ////// TEMP THINGS
   Point getDest();      // only needed for teleporter, speedzone, textitem

   Game *getGame() { return mGame; }
   F32 getGridSize() { return mGame->getGridSize(); }

   bool flag;

   U32 id;                // Item's unique id... 0 if there is none
   U32 mId;               // TODO: rename... an autoincremented serial number

   S32 mScore;            // Score awarded for this item
   


   // Find mount point or turret or forcefield closest to pos
   //Point snapEngineeredObject(const Point &pos);  

   ////////////////////
   // Rendering methods
   
   void render(bool isBeingEdited, bool isScriptItem, bool showingReferenceShip, ShowMode showMode);
};

////////////////////////////////////////
////////////////////////////////////////


class WallEdge : public DatabaseObject
{
private:
   static GridDatabase *mGridDatabase;

   Point mStart, mEnd;

public:
   WallEdge(const Point &start = Point(), const Point &end = Point());
   ~WallEdge();

   Point *getStart() { return &mStart; }
   Point *getEnd() { return &mEnd; }

   GridDatabase *getGridDatabase() { return mGridDatabase; }      // TODO: make private


   // Note that the poly returned here is different than what you might expect -- it is composed of the edges,
   // not the corners, and is thus in A-B, C-D, E-F format rather than the more typical A-B-C-D format returned
   // by getCollisionPoly() elsewhere in the game.  Therefore, it needs to be handled differently.
   bool getCollisionPoly(Vector<Point> &polyPoints) { polyPoints.resize(2); polyPoints[0] = mStart; polyPoints[1] = mEnd; return true; }  
   bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) { return false; }

   static void setGridDatabase(GridDatabase *database) { mGridDatabase = database; }
};


////////////////////////////////////////
////////////////////////////////////////

class EditorObject;

class WallSegmentManager
{
private:
   static GridDatabase *mGridDatabase;
   GridDatabase *getGridDatabase() { return mGridDatabase; }      

public:
   WallSegmentManager()  { /* Do nothing */ }

   Vector<WallSegment *> mWallSegments;
   static Vector<WallEdge *> mWallEdges;        // For mounting forcefields/turrets
   static Vector<Point> mWallEdgePoints;        // For rendering

   void deleteSegments(U32 owner);              // Delete all segments owned by specified WorldItem
   void deleteAllSegments();

   // Recalucate edge geometry for all walls when item has changed
   void computeWallSegmentIntersections(EditorObject *item); 

   // Takes a wall, finds all intersecting segments, and marks them invalid
   void invalidateIntersectingSegments(EditorObject *item);

   void buildWallSegmentEdgesAndPoints(EditorObject *item);
   void recomputeAllWallGeometry();

   static void setGridDatabase(GridDatabase *database) { mGridDatabase = database; }

   // Populate wallEdges
   static void clipAllWallEdges(const Vector<WallSegment *> &wallSegments, Vector<Point> &wallEdges);
 
   ////////////////
   // Render functions
   void renderWalls(bool convert, F32 alpha);
};


////////////////////////////////////////
////////////////////////////////////////

class SelectionItem
{
private:
   bool mSelected;
   vector<bool> mVertSelected;

public:
   SelectionItem() { /* Do nothing */ }      // Generic constructor
   SelectionItem(EditorObject *item);        // Primary constructor

   void restore(EditorObject *item);
};

class EditorObject;

class Selection
{
public:
   Selection() { /* Do nothing */ }            // Generic constructor
   Selection(Vector<EditorObject *> &items);   // Primary constructor

private:
   Vector<SelectionItem> mSelection;

public:
   void restore(Vector<EditorObject *> &items);
};


////////////////////////////////////////
////////////////////////////////////////

class EditorAttributeMenuUI;

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
      NoAttribute                 // Must be last
   };

   static const S32 DOCK_LABEL_SIZE = 9;      // Size to label items on the dock
   static const Color DOCK_LABEL_COLOR;
   static const Color HIGHLIGHT_COLOR;

private:
   string mSaveMsg;
   Color mSaveMsgColor;

   string mWarnMsg1;
   string mWarnMsg2;
   Color mWarnMsgColor;

   S32 mCurrentTeam;

   bool mSnapDisabled;

   ShowMode mShowMode;
   bool mHasBotNavZones;

   GridDatabase mGridDatabase;

   enum {
      saveMsgDisplayTime = 4000,
      warnMsgDisplayTime = 4000,
   };

   Timer mSaveMsgTimer;
   Timer mWarnMsgTimer;

   Vector<Vector</*EditorObject **/string> > mUndoItems;  // Undo/redo history  [[note that g++ requires space btwn >>]]
   Point mMoveOrigin;                           // Point representing where items were moved "from" for figuring out how far they moved
   Vector<Point> mOriginalVertLocations;

   pointainer<vector<EditorObject *> > mLevelGenItems;       // Items added by a levelgen script

   U32 mFirstUndoIndex;
   U32 mLastUndoIndex;
   U32 mLastRedoIndex;
   bool mRedoingAnUndo;

   WallSegmentManager wallSegmentManager;

   static const U32 UNDO_STATES = 128;
   void deleteUndoState();             // Removes most recent undo state from stack
   bool undoAvailable();               // Is an undo state available?
   void undo(bool addToRedoStack);     // Restore mItems to latest undo state
   void redo();                        // Redo latest undo

   void autoSave();                    // Hope for the best, prepare for the worst

   Vector<EditorObject *> mClipboard;    // Items on clipboard

   bool mLastUndoStateWasBarrierWidthChange;

   EditorObject *mItemToLightUp;
 
   string mEditFileName;                      // Manipulate with get/setLevelFileName

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

   void findHitVertex(const Point &canvasPos, EditorObject *&hitObject, S32 &hitVertex);
   void findHitItemAndEdge();         // Sets mItemHit and mEdgeHit
   S32 findHitItemOnDock(Point canvasPos);

   void findSnapVertex();
   EditorObject *mSnapVertex_i;
   S32 mSnapVertex_j;

   S32 mEdgeHit;
   EditorObject *mItemHit;

   void computeSelectionMinMax(Point &min, Point &max);
   bool mouseOnDock();                // Return whether mouse is currently over the dock

   void processLevelLoadLine(U32 argc, U32 id, const char **argv) { /* TODO: Delete this! */};

   void insertNewItem(GameObjectType itemType);                      // Insert a new object into the game

   bool mWasTesting;

   void finishedDragging();

protected:
   void onActivate();
   void onReactivate();
   void onDeactivate();

public:
   ~EditorUserInterface();    // Destructor

   void setLevelFileName(string name);
   void setLevelGenScriptName(string name);

   string getLevelFileName();
   void loadLevel();
   bool mNeedToSave;          // Have we modified the level such that we need to save?
   U32 mAllUndoneUndoLevel;   // What undo level reflects everything back just the

   void saveUndoState();
   bool showingNavZones();    // Stupid helper function, will be deleted in future

   #define GAME_TYPE_LEN 256   // TODO: Define this in terms of something else...
   char mGameType[GAME_TYPE_LEN];

   Vector<string> mGameTypeArgs;

   bool isFlagGame(char *mGameType);
   bool isTeamFlagGame(char *mGameType);
   bool isShowingReferenceShip() { return mShowingReferenceShip; }

   F32 getCurrentScale() { return mCurrentScale; }
   Point getCurrentOffset() { return mCurrentOffset; }

   void doneEditingAttributes(EditorAttributeMenuUI *editor, EditorObject *object);   // Gets run when user exits attribute editor

   void clearUndoHistory();         // Wipe undo/redo history

   Vector<TeamEditor> mTeams;       // Team list: needs to be public so we can edit from UITeamDefMenu
   Vector<TeamEditor> mOldTeams;    // Team list from before we run team editor, so we can see what changed

   pointainer<vector<EditorObject *> > mItems;        // Item list: needs to be public so we can get team info while in UITeamDefMenu
   pointainer<vector<EditorObject *> > mDockItems;    // Items sitting in the dock

   GridDatabase *getGridDatabase() { return &mGridDatabase; }

   static void setTranslationAndScale(const Point &pos);

   EditorObject *getSnapItem() { return mSnapVertex_i; }
   S32 getSnapVertexIndex() { return mSnapVertex_j; }
   void rebuildEverything();        // Does lots of things in undo, redo, and add items from script
   void recomputeAllEngineeredItems();

   void onBeforeRunScriptFromConsole();
   void onAfterRunScriptFromConsole();

   void render();
   static void renderPolyline(const Vector<Point> verts);


   Color getTeamColor(S32 teamId);


   bool mDraggingObjects;     // Should be private

   // Render walls & lineItems
   WallSegmentManager *getWallSegmentManager() { return &wallSegmentManager; }

   // Handle input
   void onKeyDown(KeyCode keyCode, char ascii);             // Handle all keyboard inputs, mouse clicks, and button presses
   void textEntryKeyHandler(KeyCode keyCode, char ascii);   // Handle keyboard activity when we're editing an item's attributes
   void specialAttributeKeyHandler(KeyCode keyCode, char ascii);
   void startAttributeEditor();

   // This function is only called from the levelgens.  If we're running in the editor, I think we can safely ignore it.
   void setGameTime(F32 time) { /* Do nothing */ }

   void onKeyUp(KeyCode keyCode);
   void onMouseMoved(S32 x, S32 y);
   void onMouseMoved();
   void onMouseDragged(S32 x, S32 y);
   void startDraggingDockItem();
   bool mouseIgnore;


   void populateDock();                         // Load up dock with game-specific items to drag and drop

   string mScriptLine;                          // Script and args, if any
   void setHasNavMeshZones(bool hasZones) { mHasBotNavZones = hasZones; }

   void idle(U32 timeDelta);
   void deleteSelection(bool objectsOnly);      // Delete selected items (true = items only, false = items & vertices)
   void copySelection();                        // Copy selection to clipboard
   void pasteSelection();                       // Paste selection from clipboard
   void setCurrentTeam(S32 currentTeam);        // Set current team for selected items, also sets team for all dock items
   void flipSelectionVertical();                // Flip selection along vertical axis
   void flipSelectionHorizontal();              // Flip selection along horizontal axis
   void scaleSelection(F32 scale);              // Scale selection by scale
   void rotateSelection(F32 angle);             // Rotate selecton by angle

   void buildAllWallSegmentEdgesAndPoints();    // Populate wallSegments from our collection of worldItems

   void validateLevel();               // Check level for things that will make the game crash!
   void validateTeams();               // Check that each item has a valid team (and fix any errors found)
   void teamsHaveChanged();            // Another team validation routine, used when all items have valid teams, but the teams themselves change
   void makeSureThereIsAtLeastOneTeam();

   void incBarrierWidth(S32 amt);      // Increase selected wall thickness by amt
   void decBarrierWidth(S32 amt);      // Decrease selected wall thickness by amt

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

   static S32 checkEdgesForSnap(const Point &clickPoint, const Vector<Point> &points, bool abcFormat, F32 &minDist, Point &snapPoint);
   static S32 checkEdgesForSnap(const Point &clickPoint,  const Vector<WallEdge *> &edges, bool abcFormat, F32 &minDist, Point &snapPoint);

   S32 checkCornersForSnap(const Point &clickPoint, const Vector<Point> &points, F32 &minDist, Point &snapPoint);
   S32 checkCornersForSnap(const Point &clickPoint,  const Vector<WallEdge *> &edges, F32 &minDist, Point &snapPoint);


   
   void deleteBorderSegs(S32 zoneId);     // Clear any borders associated with the specified zone
   void rebuildBorderSegs(U32 zoneId);
   void rebuildAllBorderSegs();
   void checkZones(S32 i, S32 j);

   void deleteItem(S32 itemIndex);

   bool itemIsSelected(U32 id);           // See if item with specified id is selected

   void runScript(const string &scriptName, const Vector<string> &args);        
   void runLevelGenScript();              // Run associated levelgen script
   void copyScriptItemsToEditor();        // Insert these items into the editor as first class items that can be manipulated or saved
   void clearLevelGenItems();             // Clear any previously created levelgen items
};


class EditorMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
   void setupMenus();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();

protected:
   void onActivate();

public:
   EditorMenuUserInterface();    // Constructor
};

extern EditorUserInterface gEditorUserInterface;
extern EditorMenuUserInterface gEditorMenuUserInterface;

};

#endif


