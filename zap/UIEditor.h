//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIEDITOR_H_
#define _UIEDITOR_H_

#include "UI.h"                  // Parent
#include "UIMenus.h"             // Parent

#include "EditorPlugin.h"        // For plugin support
#include "EditorUndoManager.h"   // To, like, undo stuff

#include "teamInfo.h"            // For TeamManager def
#include "VertexStylesEnum.h"
#include "BfObject.h"            // For BfObject definition
#include "Timer.h"
#include "Point.h"
#include "Color.h"
#include "Level.h"

#include "tnlNetStringTable.h"

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

using namespace std;

namespace Zap
{

using namespace Editor;

class DatabaseObject;
class EditorAttributeMenuUI;
class EditorTeam;
class GameType;
class LuaLevelGenerator;
class PluginMenuUI;
class SimpleTextEntryMenuUI;

class FolderManager;


////////////////////////////////////////
////////////////////////////////////////

struct PluginInfo
{
   PluginInfo(string prettyName, string fileName, string description, string requestedBinding);

   bool bindingCollision;
   string prettyName;
   string fileName;
   string binding;
   string description;
   string requestedBinding;
};

////////////////////////////////////////
////////////////////////////////////////


class BarrierX;
class MasterServerConnection;

class EditorUserInterface : public UserInterface
{
   typedef UserInterface Parent;

public:
   // Some items have special attributes.  These are the ones we can edit in the editor.
   enum SpecialAttribute {  
      Text,
      RepopDelay,
      GoFastSpeed,
      GoFastSnap,
      NoAttribute    // Must be last
   };

private:
   string mInfoMsg;
   string mSaveMsg;
   Color mSaveMsgColor;

   string mWarnMsg1;
   string mWarnMsg2;
   Color mWarnMsgColor;

   S32 mCurrentTeam;

   enum SnapContext {
      FULL_SNAPPING,
      NO_GRID_SNAPPING,
      NO_SNAPPING
   };

   enum RenderModes {
      RENDER_UNSELECTED_NONWALLS,
      RENDER_SELECTED_NONWALLS,
      RENDER_UNSELECTED_WALLS,
      RENDER_SELECTED_WALLS
   };

   enum DockMode {
      DOCKMODE_ITEMS,
      DOCKMODE_PLUGINS
   };

   enum SimpleTextEntryType {
      SimpleTextEntryID,              // Entering an objectID
      SimpleTextEntryRotateOrigin,    // Entering an angle for rotating about the origin
      SimpleTextEntryRotateCentroid,  // Entering an angle for spinning
      SimpleTextEntryScale,           // Entering a scale
   };

   DockMode mDockMode;

   SnapContext mSnapContext;

   Timer mSaveMsgTimer;
   Timer mWarnMsgTimer;

   SymbolString mLingeringMessage;

   Point mMoveOrigin;                              // Point representing where items were moved "from" for figuring out how far they moved
   Point mSnapDelta;                               // For tracking how far from the snap point our cursor is
   Vector<Point> mMoveOrigins;

   boost::shared_ptr<Level> mLevel;
   Level mDockItems;                               // Use a level to manage items on the dock to make interactions more consistent

   void setLevel(const boost::shared_ptr<Level> &level);

   Vector<Vector<string> > mMessageBoxQueue;

   bool mDragCopying;
   bool mJustInsertedVertex;

   Vector<string> mRobotLines;         // A list of robot lines read from a level file when loading from the editor

   void clearSnapEnvironment();
   void rebuildWallGeometry(Level *level);

   EditorUndoManager mUndoManager;
   void undo(bool addToRedoStack);     // Restore mItems to latest undo state
   void redo();                        // Redo latest undo

   Vector<boost::shared_ptr<BfObject> > mClipboard;    // Items on clipboard

   string mEditFileName;            // Manipulate with get/setLevelFileName

   TeamManager mTeamManager;

   F32 mCurrentScale;
   Point mCurrentOffset;            // Coords of UL corner of screen

   Point mMousePos;                 // Where the mouse is at the moment
   Point mMouseDownPos;             // Where the mouse was pressed for a drag operation

   bool mAutoScrollWithMouse;       // Make use of scrolling using middle mouse position
   bool mAutoScrollWithMouseReady;
   Point mScrollWithMouseLocation;

   U32 mGridSize;                   // Our editor gridsize
   bool showMinorGridLines() const;

   // Helper drawing methods
   void renderTurretAndSpyBugRanges(GridDatabase *editorDb) const;   // Draw translucent turret & spybug ranges
   void renderObjectsUnderConstruction() const;                      // Render partially constructed walls and other items that aren't yet in a db
   void renderDock() const;
   void renderInfoPanel() const;
   void renderPanelInfoLine(S32 line, const char *format, ...) const;

   void renderItemInfoPanel() const;

   void renderReferenceShip() const;
   void renderDragSelectBox() const;      // Render box when selecting a group of items
   void renderDockItems() const;          // Render all items on the dock
   void renderDockPlugins() const;
   void renderSaveMessage() const;
   void renderWarnings() const;
   void renderLingeringMessage() const;

   bool mCreatingPoly;
   bool mCreatingPolyline;
   bool mDragSelecting;
   bool mAddingVertex;
   bool mPreviewMode;
   bool mNormalizedScreenshotMode;
   bool mVertexEditMode;
   bool mShowAllIds;

   bool mQuitLocked;
   string mQuitLockedMessage;

   boost::shared_ptr<EditorPlugin> mPluginRunner;

   Vector<string> mLevelErrorMsgs, mLevelWarnings;
   Vector<PluginInfo> mPluginInfos;

   bool mUp, mDown, mLeft, mRight, mIn, mOut;

   void selectAll(GridDatabase *database);          // Mark all objects and vertices in specified db as selected
   void clearSelection(GridDatabase *database);     // Mark all objects and vertices in specified db as unselected

   void centerView(bool isScreenshot = false);      // Center display on all objects
   void splitBarrier();          // Split wall on selected vertex/vertices
   void doSplit(BfObject *object, S32 vertex);
   void joinBarrier();           // Join barrier bits together into one (if ends are coincident)

   BfObject *doMergeLines(BfObject *firstItem, S32 firstItemIndex);   
   BfObject *doMergeLines(BfObject *firstItem, BfObject *mergingWith, S32 mergingWithIndex,
                          void (*mergeFunction)(BfObject *, BfObject *));


   BfObject *doMergePolygons(BfObject *firstItem, S32 firstItemIndex);
   
   bool anyItemsSelected(const GridDatabase *database) const;  // Are any items selected?
   bool anythingSelected() const;                              // Are any items/vertices selected?

   static void renderAttribText(S32 xpos, S32 ypos, S32 textsize, const Color &keyColor,
         const Color &valColor, const Vector<string> &keys, const Vector<string> &vals);
   static void setColor(bool isSelected, bool isLitUp, bool isScriptItem);
   static void renderDockItemLabel(const Point &pos, const char *label);
   static void renderDockItem(const BfObject *object, const Color &color, F32 currentScale, S32 snapVertexIndex);

public:
   S32 getItemSelectedCount();                     // How many are objects are selected?

private:
   // Sets mHitItem and mEdgeHit -- findHitItemAndEdge calls one or more of the associated helper functions below
   void findHitItemAndEdge();                         
   bool checkForVertexHit(BfObject *object);
   bool checkForEdgeHit(const Point &point, BfObject *object);        

   bool overlaps(const Point &point, BfObject *object);    // Returns true if point overlaps object

   void findHitItemOnDock();     // Sets mDockItemHit
   S32 findHitPlugin() const;

   void findSnapVertex();
   S32 mSnapVertexIndex;

   Point *mPointOfRotation;

   S32 mEdgeHit;
   S32 mHitVertex;

   bool canRotate() const;             // Returns true if we're able to rotate something
   void clearPointOfRotation();

   SafePtr<BfObject> mNewItem;
   SafePtr<BfObject> mSnapObject;
   SafePtr<BfObject> mHitItem;

   SafePtr<BfObject> mDraggingDockItem;
   SafePtr<BfObject> mDockItemHit;

   SafePtr<BfObject> mDelayedUnselectObject;
   S32 mDelayedUnselectVertex;

   Vector<BfObject *> mSelectedObjectsForDragging;

   S32 mDockPluginScrollOffset;
   U32 mDockWidth;
   bool mIgnoreMouseInput;

   bool mouseOnDock() const;          // Return whether mouse is currently over the dock

   void insertNewItem(U8 itemTypeNumber);    // Insert a new object into the specified database

   bool mWasTesting;

   void onFinishedDragging();    // Called when we're done dragging an object
   void onFinishedDragging_droppedItemOnDock();
   void onFinishedDragging_movingObject();

   void onSelectionChanged();    // Called when current selection has changed

   void onMouseClicked_left();
   void onMouseClicked_right();

   Point convertCanvasToLevelCoord(const Point &p) const;
   Point convertLevelToCanvasCoord(const Point &p, bool convert = true) const;

   string getInfoMsg() const;

   boost::scoped_ptr<SimpleTextEntryMenuUI> mSimpleTextEntryMenu;
   boost::scoped_ptr<PluginMenuUI> mPluginMenu;      
   map<string, Vector<string> > mPluginMenuValues;

   // These are for rendering
   Vector<Point> mWallEdgePoints;
   Vector<Point> mSelectedWallEdgePointsWholeWalls;
   Vector<Point> mSelectedWallEdgePointsDraggedVertices;

   void showCouldNotFindScriptMessage(const string &scriptName);
   void showPluginError(const string &msg);

   string mLingeringMessageQueue;   // Ok, not much of a queue, but we can only have one of these, so this is enough

   Level mLevelGenDatabase;         // Database for inserting objects when running a levelgen script in the editor

   void translateSelectedItems(const Point &offset, const Point &lastOffset);
   void snapSelectedEngineeredItems(const Point &cumulativeOffset);

   void render() const;
   void renderObjects(const GridDatabase *database, RenderModes renderMode, bool isLevelgenOverlay) const;
   void renderWallsAndPolywalls(const GridDatabase *database, const Point &offset, bool selected, bool isLevelGenDatabase) const;
   void renderObjectIds(GridDatabase *database) const;


   void autoSave();                    // Hope for the best, prepare for the worst
   bool doSaveLevel(const string &saveName, bool showFailMessages);

   void onActivateReactivate();

   void setCurrentOffset(const Point &center);

   void doneAddingObjects(const Vector<BfObject *> &bfObjects);
   void doneAddingObjects(BfObject *bfObject);

   void doneChangingGeoms(const Vector<BfObject *> &bfObjects);
   void doneChangingGeoms(BfObject *bfObject);

   void showLevelHasErrorMessage(bool gameTypeError);

protected:
   void onActivate();
   void onReactivate();

   void renderMasterStatus(const MasterServerConnection *connectionToMaster) const;

   bool usesEditorScreenMode() const;

public:
   explicit EditorUserInterface(ClientGame *game, UIManager *uiManager);   // Constructor
   virtual ~EditorUserInterface();                                         // Destructor

   Level *getLevel() const;           // Need external access to this in one static function

   void setLevelFileName(const string &name);
   //void setLevelGenScriptName(const string &name);

   string getLevelFileName() const;
   void cleanUp();
   void loadLevel();

   Vector<string> mGameTypeArgs;

   static const string UnnamedFile;

   bool saveLevel(bool showFailMessages, bool showSuccessMessages);   // Public because called from callbacks

   void lockQuit(const string &message);
   void unlockQuit();

   string getQuitLockedMessage();
   bool isQuitLocked();

   string getLevelText() const;
   const Vector<PluginInfo> *getPluginInfos() const;

   F32 getCurrentScale() const;
   Point getCurrentOffset() const;

   Vector<TeamInfo> mOldTeams;     // Team list from before we run team editor, so we can see what changed

   void rebuildEverything(Level *level);   // Does lots of things in undo, redo, and add items from script

   void onQuitted();       // Releases some memory when quitting the editor

   S32 getTeamCount() const;
   const AbstractTeam *getTeam(S32 teamId);

   //void addTeam(const TeamInfo &teamInfo);
   void addTeam(EditorTeam *team, S32 index);
   void removeTeam(S32 teamId);
   void clearTeams();

   bool getNeedToSave() const;

   void clearRobotLines();
   void addRobotLine(const string &robotLine);

   bool mDraggingObjects;     // Should be private
   void geomChanged(BfObject *obj);

   // Handle input
   bool onKeyDown(InputCode inputCode);                         // Handle all keyboard inputs, mouse clicks, and button presses
   bool handleKeyPress(InputCode inputCode, const string &inputString);
   
   void onTextInput(char ascii);                                // Handle all text input characters
   bool checkPluginKeyBindings(string inputString);             // Handle keys bound to plugins
   void specialAttributeKeyHandler(InputCode inputCode, char ascii);
   void startAttributeEditor();
   void doneEditingAttributes(EditorAttributeMenuUI *editor, BfObject *object);   // Gets run when user exits attribute editor

   void startSimpleTextEntryMenu(SimpleTextEntryType entryType);
   void doneWithSimpleTextEntryMenu(SimpleTextEntryMenuUI *menu, S32 entryType);

   void zoom(F32 zoomAmount);
   void setDisplayScale(F32 scale);
   void setDisplayCenter(const Point &center);
   void setDisplayExtents(const Rect &extents, F32 backoffFact = 1.0f);
   Rect getDisplayExtents() const;
   Point getDisplayCenter() const;

   F32 getGridSize() const;


   void onKeyUp(InputCode inputCode);
   void onMouseUp();

   void setMousePos();

   void onMouseMoved();
   void onMouseDragged();
   void onMouseDragged_startDragging();
   void onMouseDragged_copyAndDrag(const Vector<DatabaseObject *> *objList);
   void startDraggingDockItem();
   BfObject *copyDockItem(BfObject *source);

   void doneAddingObjects(S32 serialNumber);
   void doneAddingObjects(const Vector<S32> &serialNumbers);

   void doneChangingGeoms(S32 serialNumber);
   void doneChangingGeoms(const Vector<S32> &serialNumbers);


   void populateDock();                         // Load up dock with game-specific items to drag and drop
   void addDockObject(BfObject *object, F32 xPos, F32 yPos);

   string mScriptLine;                           // Script and args, if any

   void idle(U32 timeDelta);
   void deleteSelection(bool objectsOnly);       // Delete selected items (true = items only, false = items & vertices)
   void copySelection();                         // Copy selection to clipboard
   void pasteSelection();                        // Paste selection from clipboard
   void setCurrentTeam(S32 currentTeam);         // Set current team for selected items, also sets team for all dock items

   void flipSelectionHorizontal();               // Flip selection along horizontal axis
   void flipSelectionVertical();                 // Flip selection along vertical axis
   void flipSelection(F32 center, bool isHoriz); // Do the actual flipping for the above

   void scaleSelection(F32 scale);                    // Scale selection by scale
   void rotateSelection(F32 angle, bool useOrigin);   // Rotate selecton by angle
   void setSelectionId(S32 id);

   void rebuildSelectionOutline();

   void validateLevel();               // Check level for things that will make the game crash!
   void validateTeams();               // Check that each item has a valid team (and fix any errors found)
   void validateTeams(const Vector<DatabaseObject *> *dbObjects);

   void teamsHaveChanged();            // Another team validation routine, used when all items have valid teams, but the teams themselves change
   void makeSureThereIsAtLeastOneTeam();

   void changeBarrierWidth(S32 amt);   // Increase selected wall thickness by amt

   void testLevel();
   void testLevelStart();

   void setSaveMessage(const string &msg, bool savedOK);
   void clearSaveMessage();

   void setWarnMessage(const string &msg1, const string &msg2);
   void clearWarnMessage();

   void queueSetLingeringMessage(const string &msg);
   void setLingeringMessage(const string &msg);
   void clearLingeringMessage();

   void onDisplayModeChange();      // Called when we shift between windowed and fullscreen mode, after change is made

   // Snapping related functions:
   Point snapPoint(const Point &p, bool snapWhileOnDock = false) const;
   Point snapPointToLevelGrid(Point const &p) const;

   void markSelectedObjectsAsUnsnapped2(const Vector<DatabaseObject *> *objList);


   bool getSnapToWallCorners() const;     // Returns true if wall corners are active snap targets

   void onBeforeRunScriptFromConsole();
   void onAfterRunScriptFromConsole();

   S32 checkCornersForSnap(const Point &clickPoint,  const Vector<DatabaseObject *> *edges, F32 &minDist, Point &snapPoint) const;

   void deleteItem(S32 itemIndex, bool batchMode = false);

   // Helpers for doing batch deletes
   void doneDeleteingWalls(); 
   void doneDeletingObjects();

   // Run a script, and put resulting objects in database
   void runScript(Level *level, const FolderManager *folderManager, const string &scriptName, const Vector<string> &args);
   void runPlugin(const FolderManager *folderManager, const string &scriptName, const Vector<string> &args);  

   string getPluginSignature();                 // Try to create some sort of uniqeish signature for the plugin
   void onPluginExecuted(const Vector<string> &args);
   void runLevelGenScript();              // Run associated levelgen script
   void copyScriptItemsToEditor();        // Insert these items into the editor as first class items that can be manipulated or saved
   void clearLevelGenItems();             // Clear any previously created levelgen items

   void addToEditor(BfObject *obj);
   void showUploadErrorMessage(S32 errorCode, const string &errorBody);

   void createNormalizedScreenshot(ClientGame* game);

   void findPlugins();
   U32 findPluginDockWidth();

   ///// Testing
   friend class EditorTest;
   FRIEND_TEST(EditorTest, findSnapVertexTest);
   FRIEND_TEST(EditorTest, wallCentroidForRotationTest);
};


};

#endif
