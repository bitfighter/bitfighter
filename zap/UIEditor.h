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
#include "gameObject.h"          // For EditorObject definition
#include "gridDB.h"              // For DatabaseObject definition
#include "timer.h"
#include "point.h"
#include "BotNavMeshZone.h"      // For Border definition
#include "tnlNetStringTable.h"
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
   bool isCollisionEnabled() { return true; }
};

////////////////////////////////////////
////////////////////////////////////////

enum GameItems    // Remember to keep these properly aligned with gGameItemRecs[]
{
   ItemSpawn,
   ItemSpeedZone,
   ItemSoccerBall,
   ItemFlag,
   ItemFlagSpawn,
   ItemBarrierMaker,
   ItemPolyWall,
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
   ItemInvalid
};


enum GeomType {
   geomPoint,           // Single point feature (like a flag)
   geomSimpleLine,      // Two point line (like a teleport)
   geomLine,            // Many point line (like a wall)
   geomPoly,            // Polygon feature (like a loadout zone)
   geomNone,            // Other/unknown (not used, just here for completeness)
};


extern bool isConvex(const Vector<Point> &verts);

// Width of line representing centerline of barriers
#define WALL_SPINE_WIDTH gLineWidth3


class WorldItem : public DatabaseObject, public EditorObject
{  
private:
   Vector<Point> mVerts;

   bool mDockItem;              // True if this item lives on the dock
   bool mSelected;
   bool mLitUp;
   S32 mVertexLitUp;

   vector<bool> mVertSelected;  // never use std::vector<bool> use deque instead
   bool mAnyVertsSelected;

   void init(GameItems itemType, S32 xteam, F32 xwidth, U32 itemid, bool isDockItem);

   static GridDatabase *mGridDatabase;

   Game *mGame;

public:
   WorldItem(GameItems itemType = ItemInvalid, S32 itemId = 0);    // Only used when creating an item from a loaded level
   WorldItem(GameItems itemType, Point pos, S32 team, bool isDockItem, F32 width = 1, F32 height = 1, U32 id = 0);  // Primary constructor

   void initializeGeom();     // Once we have our points, do some geom preprocessing


   ////////////////////////////
   ////// TEMP THINGS
   
   // only needed for teleporter, speedzone, textitem
   Point getDest();
   F32 renderTextItem(F32 alpha);          // Returns size of text
   void addToGame(Game *game) { mGame = game; /*addToDatabase();*/ }    // Could maintain mItems here...
   Game *getGame() { return mGame; }
   F32 getGridSize() { return mGame->getGridSize(); }

   bool flag;

   bool processArguments(S32 argc, const char **argv);
   S32 getDefaultRepopDelay(GameItems itemType);
   S32 getRadius(F32 scale);
   Point getSelectionOffset(F32 scale);      // For turrets, apparent selection center is not the same as the item's actual location

   GameItems index;
   S32 team;
   F32 width;
   U32 id;                // Item's unique id... 0 if there is none
   U32 mId;               // TODO: rename... an autoincremented serial number

   S32 mScore;            // Score awarded for this item
   
   bool isSelected() { return mSelected; }
   void setSelected(bool selected) { mSelected = selected; }

   bool isLitUp() { return mLitUp; }
   void setLitUp(bool litUp) { mLitUp = litUp; }

   bool isVertexLitUp(S32 vertexIndex) { return mVertexLitUp == vertexIndex; }
   bool setVertexLitUp(S32 vertexIndex) { mVertexLitUp = vertexIndex; }


   bool isOnDock() { return mDockItem; }

   LineEditor lineEditor; // For items that have an aux text field
   U32 textSize;          // For items that have an aux text field
   S32 repopDelay;        // For repair items, also used for engineered objects heal rate
   S32 speed;             // Speed for speedzone items
   bool boolattr;         // Additional optional boolean attribute for some items (only speedzone so far...)

   bool hasWidth();
   bool anyVertsSelected() { return mAnyVertsSelected; }
   bool renderFull(F32 scale);    // Should item be rendered it full glory?  Not fully used at the moment...

   // Following are used by all items; scale has wall specific code
   void rotateAboutPoint(const Point &center, F32 angle);      // Rotate item around specified point
   void scale(const Point &center, F32 scale);                 // Scale item centered on center


   // These methods are mostly for lines and polygons
   void selectVert(S32 vertIndex);
   void aselectVert(S32 vertIndex);
   void unselectVert(S32 vertIndex);
   void unselectVerts();
   bool vertSelected(S32 vertIndex);
   void addVert(Point vert);
   void addVertFront(Point vert);
   void deleteVert(S32 vertIndex);
   void insertVert(Point vertex, S32 vertIndex);
   void setVert(Point vertex, S32 vertIndex);
   void invalidate() { mVerts.clear(); index = ItemInvalid; }

   void onGeomChanging();   // Item geom is interactively changing
   void onItemDragging();   // Item is being dragged around the screen
   void onGeomChanged();    // Item changed geometry (or moved), do any internal updating that might be required
   void onAttrsChanging();
   void onAttrsChanged();   // Attrs changed

   void flipHorizontal(const Point &boundingBoxMin, const Point &boundingBoxMax);      // All items use this
   void flipVertical(const Point &boundingBoxMin, const Point &boundingBoxMax);        // All items use this

   Point normal;             // Point perpendicular to snap point, only for turrets and forcefields
   bool snapped;             // Is item sufficiently snapped?  only for turrets and forcefields

   Point forceFieldEnd;      // Point where forcefield terminates.  Only used for turrets.
   WallSegment *forceFieldMountSegment;   // Segment where forcefield is mounted
   WallSegment *forceFieldEndSegment;     // Segment where forcefield ends

   // The following are for polygonal items only
   Vector<Point> fillPoints;
   Point centroid;

   void processEndPoints();      // Wall only

   void decreaseWidth(S32 amt);  // Wall only
   void increaseWidth(S32 amt);  // Wall only


   // Find mount point or turret or forcefield closest to pos
   Point snapEngineeredObject(const Point &pos);  
   void findForceFieldEnd();                                      // Find end of forcefield

   virtual bool isConvex() { return Zap::isConvex(mVerts); }      // Only used for navmeshzones

   Vector<Point> &getVerts() { return mVerts; }
   Vector<Point> extendedEndPoints;                               // a-b b-c format

   S32 vertCount() { return mVerts.size(); }
   Point vert(S32 vertIndex) { return mVerts[vertIndex]; }

   GeomType geomType();

   ////////////////////
   // Rendering methods
   void renderPolyline();                       // Draws a line connecting points in mVerts

   void renderPolylineCenterline(F32 alpha);    // Draw barrier centerlines; wraps renderPolyline()
   void renderLinePolyVertices(F32 scale, F32 alpha = 1.0);
   
   void render(S32 index, bool isBeingEdited, bool isScriptItem, bool showingReferenceShip, ShowMode showMode);

   const char *getOriginBottomLabel();          // SimpleLine items only
   const char *getDestinationBottomLabel();

   ////////////////////
   //  DatabaseObject methods
   static void setGridDatabase(GridDatabase *database) { mGridDatabase = database; }
   GridDatabase *getGridDatabase() { return mGridDatabase; }

   bool getCollisionPoly(Vector<Point> &polyPoints) { return false; }
   bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) { return false; }
   bool isCollisionEnabled() { return true; }

   ////////////////////
   //  EditorItem methods
   void render() { /* Do nothing */ }
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
   bool isCollisionEnabled() { return true; }

   static void setGridDatabase(GridDatabase *database) { mGridDatabase = database; }
};


////////////////////////////////////////
////////////////////////////////////////


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
   void computeWallSegmentIntersections(WorldItem *item); 

   // Takes a wall, finds all intersecting segments, and marks them invalid
   void invalidateIntersectingSegments(WorldItem *item);

   void buildWallSegmentEdgesAndPoints(WorldItem *item);
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
   SelectionItem() { /* do nothing */ }      // Generic constructor
   SelectionItem(WorldItem &item);           // Primary constructor

   void restore(WorldItem &item);
};


class Selection
{
public:
   Selection() { /* do nothing */ }          // Generic constructor
   Selection(Vector<WorldItem> &items);      // Primary constructor

private:
   Vector<SelectionItem> mSelection;

public:
   void restore(Vector<WorldItem> &items);
};


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

   Vector<Vector<WorldItem> > mUndoItems;    // Undo/redo history  [[note that g++ requires space btwn >>]]
   Vector<WorldItem> mMostRecentState;       // Copy of most recent state, to facilitate dragging
   Point mMoveOrigin;                        // Point representing where items were moved "from" for figuring out how far they moved

   Vector<WorldItem> mLevelGenItems;         // Items added by a levelgen script

   U32 mFirstUndoIndex;
   U32 mLastUndoIndex;
   U32 mLastRedoIndex;
   bool mRedoingAnUndo;

   WallSegmentManager wallSegmentManager;

   static const U32 UNDO_STATES = 128;
   void saveUndoState(const Vector<WorldItem> &items, bool cameFromRedo = false);    // Save current state into undo history buffer
   void deleteUndoState();             // Removes most recent undo state from stack
   bool undoAvailable();               // Is an undo state available?
   void undo(bool addToRedoStack);     // Restore mItems to latest undo state
   void redo();                        // Redo latest undo

   void autoSave();                    // Hope for the best, prepare for the worst

   Vector<WorldItem> mDockItems;       // Items sitting in the dock
   Vector<WorldItem> mClipboard;       // Items on clipboard

   bool mLastUndoStateWasBarrierWidthChange;

   void saveSelection();               // Save selection mask
   void restoreSelection();            // Restore selection mask
   Selection mSelectedSet;             // Place to store selection mask

   S32 itemToLightUp;
   S32 vertexToLightUp;

   string mEditFileName;               // Manipulate with get/setLevelFileName

   S32 mEditingSpecialAttrItem;        // Index of item we're editing special attributes on
   SpecialAttribute mSpecialAttribute; // Type of special attribute we're editing

   void doneEditingSpecialItem(bool save);    // Gets run when user exits special-item editing mode
   U32 getNextAttr(S32 item);                 // Assist on finding the next attribute this item is capable of editing,
                                              // for cycling through the various editable attributes
   WorldItem mNewItem;
   F32 mCurrentScale;
   Point mCurrentOffset;         // Coords of UR corner of screen

   Point mMousePos;              // Where the mouse is at the moment
   Point mMouseDownPos;          // Where the mouse was pressed for a drag operation

   void renderGrid();                                       // Draw background snap grid
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
   void unselectItem(S32 i);     // Mark item and vertices as unselected

   void centerView();            // Center display on all objects
   void splitBarrier();          // Split wall on selected vertex/vertices
   void joinBarrier();           // Join barrier bits together into one (if ends are coincident)

   //S32 countSelectedVerts();
   bool anyItemsSelected();           // Are any items selected?
   bool anythingSelected();           // Are any items/vertices selected?

   void findHitVertex(Point canvasPos, S32 &hitItem, S32 &hitVertex);
   void findHitItemAndEdge();         // Sets mItemHit and mEdgeHit
   S32 findHitItemOnDock(Point canvasPos);

   void findSnapVertex();
   S32 mSnapVertex_i;
   S32 mSnapVertex_j;

   S32 mEdgeHit, mItemHit;

   void computeSelectionMinMax(Point &min, Point &max);
   bool mouseOnDock();                // Return whether mouse is currently over the dock

   void processLevelLoadLine(U32 argc, U32 id, const char **argv);

   void insertNewItem(GameItems itemType);                                                    // Insert a new object into the game

   bool mWasTesting;
   F32 mGridSize;

   void finishedDragging();

protected:
   void onActivate();
   void onReactivate();
   void onDeactivate();

public:
   ~EditorUserInterface();    // Destructor

   EditorGame *editorGame;

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

   S32 getEditingSpecialAttrItem() { return mEditingSpecialAttrItem; }
   bool isEditingSpecialAttribute(SpecialAttribute attribute) { return mSpecialAttribute == attribute; }

   void clearUndoHistory();         // Wipe undo/redo history

   Vector<TeamEditor> mTeams;       // Team list: needs to be public so we can edit from UITeamDefMenu
   Vector<TeamEditor> mOldTeams;    // Team list from before we run team editor, so we can see what changed

   Vector<WorldItem> mItems;        // Item list: needs to be public so we can get team info while in UITeamDefMenu

   GridDatabase *getGridDatabase() { return &mGridDatabase; }

   static void setTranslationAndScale(const Point &pos);

   S32 getSnapItemIndex() { return mSnapVertex_i; }
   S32 getSnapVertexIndex() { return mSnapVertex_j; }
   void rebuildEverything();        // Does lots of things in undo, redo, and add items from script
   void recomputeAllEngineeredItems();

   void onBeforeRunScriptFromConsole();
   void onAfterRunScriptFromConsole();

   void render();

   bool mDraggingObjects;     // Should be private

   // Render walls & lineItems
   WallSegmentManager *getWallSegmentManager() { return &wallSegmentManager; }

   // Handle input
   void onKeyDown(KeyCode keyCode, char ascii);             // Handle all keyboard inputs, mouse clicks, and button presses
   void textEntryKeyHandler(KeyCode keyCode, char ascii);   // Handle keyboard activity when we're editing an item's attributes
   void specialAttributeKeyHandler(KeyCode keyCode, char ascii);
   void itemPropertiesEnterKeyHandler();

   // This function is only called from the levelgens.  If we're running in the editor, I think we can safely ignore it.
   void setGameTime(F32 time) { /* Do nothing */ }

   void onKeyUp(KeyCode keyCode);
   void onMouseMoved(S32 x, S32 y);
   void onMouseMoved();
   void onMouseDragged(S32 x, S32 y);
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

   // For generating nav mesh zones automatically.  Hooray!
   void generateBotZones();
   void removeUnusedNavMeshZones(Vector<WorldItem> &zones);
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
   void render();
};

extern EditorUserInterface gEditorUserInterface;
extern EditorMenuUserInterface gEditorMenuUserInterface;

};

#endif


