//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "WallItem.h"

#include "barrier.h"
#include "Colors.h"
#include "gameObjectRender.h"
#include "GeomUtils.h"
#include "Level.h"
#include "stringUtils.h"
#include "WallSegmentManager.h"

#include "tnlLog.h"


using namespace TNL;

namespace Zap
{

using namespace LuaArgs;


// WallItem is child of LineItem... the only thing LineItem brings to the party is width

// Combined C++/Lua constructor
WallItem::WallItem(lua_State *L)
{
   mObjectTypeNumber = WallItemTypeNumber;
   mWidth = Barrier::DEFAULT_BARRIER_WIDTH;
   mAlreadyAdded = false;

   setNewGeometry(geomPolyLine);
   
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { LINE, INT, END }}, 2 };
      S32 profile = checkArgList(L, constructorArgList, "WallItem", "constructor");
      if(profile == 1)
      {
         setWidth(lua_tointeger(L, -1));     // Grab this before it gets popped
         lua_pop(L, 1);                      // Clean up stack for setGeom, which only expects points
         lua_setGeom(L);
      }
   }

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
WallItem::~WallItem()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


WallItem *WallItem::clone() const
{
   return new WallItem(*this);
}


// Client (i.e. editor) only; walls processed in ServerGame::processPseudoItem() on server
// BarrierMaker <width> <x> <y> <x> <y> ...
bool WallItem::processArguments(S32 argc, const char **argv, Level *level)
{
   if(argc < 6)         // "BarrierMaker" keyword, width, and two or more x,y pairs
      return false;

   setWidth(atoi(argv[1]));

   readGeom(argc, argv, 2, level->getLegacyGridSize());

   updateExtentInDatabase();

   if(getVertCount() < 2)
      return false;

   return true;
}


string WallItem::toLevelCode() const
{
   return appendId("BarrierMaker") + " " + itos(getWidth()) + " " + geomToLevelCode();
}


void WallItem::changeWidth(S32 amt)
{
   S32 width = mWidth;

   if(amt > 0)
      width += amt - (S32) width % amt;    // Handles rounding
   else
   {
      amt *= -1;
      width -= ((S32) width % amt) ? (S32) width % amt : amt;      // Dirty, ugly thing
   }

   setWidth(width);
   onGeomChanged();
}


void WallItem::onGeomChanged()
{
   // Fill extendedEndPoints from the vertices of our wall's centerline, or from PolyWall edges
   processEndPoints();

   GridDatabase *db = getDatabase();

   if(db)
      db->getWallSegmentManager()->onWallGeomChanged(db, this, isSelected(), getSerialNumber());

   Parent::onGeomChanged();
}


void WallItem::onItemDragging()
{
   // Do nothing -- this is here to override Parent::onItemDragging(), onGeomChanged() should only be called after move is complete
}

void WallItem::setTeam(S32 team)
{
   // Do nothing... walls have no teams!
}


// WallItems are not really added to the game in the sense of other objects; rather their geometry is used
// to create Barriers that are added directly.  Here we will mark the item as added (to catch errors in Lua
// scripts that attempt to modify an added item), but we have no need to pass the event handler up the stack
// to superclass event handlers.
void WallItem::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);
   mAlreadyAdded = true;
}


// Only called in editor during preview mode -- basicaly prevents parent class from rendering spine of wall
void WallItem::render() const
{
   // Do nothing
}


void WallItem::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices) const
{
#ifndef ZAP_DEDICATED
   if(isSelected() || isLitUp())
      renderWallOutline(this, getOutline(), currentScale, snappingToWallCornersEnabled, renderVertices);
   else
      renderWallOutline(this, getOutline(), getEditorRenderColor(), currentScale, snappingToWallCornersEnabled, renderVertices);
#endif
}


void WallItem::processEndPoints()
{
#ifndef ZAP_DEDICATED
   // Extend wall endpoints for nicer rendering
   constructBarrierEndPoints(getOutline(), (F32)getWidth(), extendedEndPoints);     // Fills extendedEndPoints
#endif
}


Rect WallItem::calcExtents()
{
   // mExtent was already calculated when the wall was inserted into the segmentManager...
   // All we need to do here is override the default calcExtents, to avoid clobbering our already good mExtent.
   return getExtent();     
}


const char *WallItem::getOnScreenName()     const { return "Wall";  }
const char *WallItem::getOnDockName()       const { return "Wall";  }
const char *WallItem::getPrettyNamePlural() const { return "Walls"; }
const char *WallItem::getEditorHelpString() const { return "Walls define the general form of your level."; }

const char *WallItem::getInstructionMsg(S32 attributeCount) const { return "[+] and [-] to change width"; }


void WallItem::fillAttributesVectors(Vector<string> &keys, Vector<string> &values)
{ 
   keys.push_back("Width");   values.push_back(itos(getWidth()));
}


bool WallItem::hasTeam()      { return false; }
bool WallItem::canBeHostile() { return false; }
bool WallItem::canBeNeutral() { return false; }


const Color &WallItem::getEditorRenderColor() const
{
   return Colors::gray50;    // Color of wall spine in editor
}


// Size of object in editor 
F32 WallItem::getEditorRadius(F32 currentScale) const
{
   return (F32)WallItem::VERTEX_SIZE;   // Keep vertex hit targets the same regardless of scale
}


void WallItem::scale(const Point &center, F32 scale)
{
   Parent::scale(center, scale);

   // Adjust the wall thickness
   // Scale might not be accurate due to conversion to S32
   setWidth(S32(getWidth() * scale));
}


// Needed to provide a valid signature
S32 WallItem::getWidth() const
{
   return mWidth;
}


void WallItem::setWidth(S32 width) 
{         
   if(width < Barrier::MIN_BARRIER_WIDTH)
      mWidth = Barrier::MIN_BARRIER_WIDTH;

   else if(width > Barrier::MAX_BARRIER_WIDTH)
      mWidth = Barrier::MAX_BARRIER_WIDTH; 

   else
      mWidth = width; 
}


void WallItem::setSelected(bool selected)
{
   Parent::setSelected(selected);
   
   // Find the associated segment(s) and mark them as selected (or not)
   Barrier::setWallSelected(getDatabase(), getSerialNumber(), selected);
}


// Here to provide a valid signature in WallItem
void WallItem::addToGame(Game *game, GridDatabase *database)
{
   Parent::addToGame(game, database);
}


/////
// Lua interface
/**
 * @luafunc WallItem::WallItem()
 * @luafunc WallItem::WallItem(geom geometry, int thickness)
 * @luaclass WallItem
 * 
 * @brief Traditional wall item.
 * 
 * @descr A WallItem is a traditional wall consisting of a series of
 * straight-line segments. WallItems have a width setting that expands the walls
 * outward on the edges, but not the ends. The game may make slight adjustments
 * to the interior vertices of a wall to improve visual appearance. Collinear
 * vertices may be deleted to simplify wall geometry.
 * 
 * @geom WallItem geometry consists of two or more points forming a linear
 * sequence, each consecutive pair defining a straight-line segment. WallItem
 * geometry can cross or form loops with no adverse consequences.
 */
//               Fn name       Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getWidth,     ARRAYDEF({{      END }}), 1 ) \
   METHOD(CLASS, setWidth,     ARRAYDEF({{ INT, END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(WallItem, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(WallItem, LUA_METHODS);

#undef LUA_METHODS


const char *WallItem::luaClassName = "WallItem";
REGISTER_LUA_SUBCLASS(WallItem, BfObject);

/**
 * @luafunc int WallItem::getWidth()
 * 
 * @brief Returns the WallItem's width setting.
 * 
 * @descr Walls have a default width of 50.
 * 
 * @return The WallItem's width.
 */
S32 WallItem::lua_getWidth(lua_State *L)     
{ 
   return returnInt(L, getWidth()); 
}


/**
 * @luafunc WallItem::setWidth(int width)
 * 
 * @brief Sets the WallItem's width.
 * 
 * @descr Walls have a default width of 50.
 * 
 * @param width The WallItem's new width.
 */
S32 WallItem::lua_setWidth(lua_State *L)     
{ 
   checkIfHasBeenAddedToTheGame(L);

   checkArgList(L, functionArgs, "WallItem", "setWidth");

   setWidth(getInt(L, 1));

   return 0; 
}


void WallItem::checkIfHasBeenAddedToTheGame(lua_State *L)
{
   if(mAlreadyAdded)
   {
      ScriptContext context = getScriptContext(L);

      if(context != PluginContext)     // Plugins can alter walls that are already in-game... levelgens cannot
      {
         const char *msg = "Can't modify a wall that's already been added to a game!";
         logprintf(LogConsumer::LogError, msg);
         throw LuaException(msg);
      }
   }
}


// Some Lua method overrides.  Because walls are... special.

S32 WallItem::lua_setPos(lua_State *L)
{
   checkIfHasBeenAddedToTheGame(L);
   return Parent::lua_setPos(L);
}


S32 WallItem::lua_setGeom(lua_State *L)
{
   checkIfHasBeenAddedToTheGame(L);
   return Parent::lua_setGeom(L);
}




////////////////////////////////////////
////////////////////////////////////////

// Constructor
WallEdge::WallEdge(const Point &start, const Point &end) 
{ 
   mStart = start; 
   mEnd   = end; 
   mPoints.reserve(2);
   mPoints.push_back(start);
   mPoints.push_back(end);

   setExtent(Rect(start, end));

   // Set some things required by DatabaseObject
   mObjectTypeNumber = WallEdgeTypeNumber;
}


// Destructor
WallEdge::~WallEdge()
{
    // Make sure object is out of the database
   removeFromDatabase(false); 
}


Point *WallEdge::getStart() { return &mStart; }
Point *WallEdge::getEnd()   { return &mEnd;   }


const Vector<Point> *WallEdge::getCollisionPoly() const
{
   return &mPoints;
}


bool WallEdge::getCollisionCircle(U32 stateIndex, Point &point, float &radius) const
{
   return false;
}


////////////////////////////////////////
////////////////////////////////////////

// Regular constructor
WallSegment::WallSegment(GridDatabase *gridDatabase, const Point &start, const Point &end, F32 width, S32 owner) 
{ 
   // Calculate segment corners by expanding the extended end points into a rectangle
   expandCenterlineToOutline(start, end, width, mCorners);  // ==> Fills mCorners 
   init(gridDatabase, owner);
}


// PolyWall constructor
WallSegment::WallSegment(GridDatabase *gridDatabase, const Vector<Point> &points, S32 owner)
{
   mCorners = points;

   if(isWoundClockwise(points))
      mCorners.reverse();

   init(gridDatabase, owner);
}


// Intialize, only called from constructors above
void WallSegment::init(GridDatabase *database, S32 owner)
{
   // Recompute the edges based on our new corner points
   resetEdges();   

   mObjectTypeNumber = WallSegmentTypeNumber;

   setExtent(Rect(mCorners));

   // Add item to database, set its extents
   addToDatabase(database);
   
   // Drawing filled wall requires that points be triangluated
   Triangulate::Process(mCorners, mTriangulatedFillPoints);    // ==> Fills mTriangulatedFillPoints

   mOwner = owner; 
   invalid = false; 
   mSelected = false;
}


// Destructor
WallSegment::~WallSegment()
{ 
   // Make sure object is out of the database -- but don't delete it since we're destructing
   if(getDatabase())
      getDatabase()->removeFromDatabase(this, false); 
}


// Returns serial number of owner
S32 WallSegment::getOwner()
{
   return mOwner;
}


void WallSegment::invalidate()
{
   invalid = true;
}
 

// Resets edges of a wall segment to their factory settings; i.e. 4 simple walls representing a simple outline
void WallSegment::resetEdges()
{
   cornersToEdges(mCorners, mEdges);
}


void WallSegment::renderFill(const Point &offset, const Color &color)
{
#ifndef ZAP_DEDICATED
   if(mSelected)
      renderWallFill(&mTriangulatedFillPoints, color, offset, true);       // Use true because all segment fills are triangulated
   else
      renderWallFill(&mTriangulatedFillPoints, color, true);
#endif
}


bool WallSegment::isSelected() const
{
   return mSelected;
}


void WallSegment::setSelected(bool selected)
{
   mSelected = selected;
}


const Vector<Point> *WallSegment::getCorners()                { return &mCorners; }
const Vector<Point> *WallSegment::getEdges()                  { return &mEdges; }
const Vector<Point> *WallSegment::getTriangulatedFillPoints() { return &mTriangulatedFillPoints; }


const Vector<Point> *WallSegment::getCollisionPoly() const
{
   return &mCorners;
}


bool WallSegment::getCollisionCircle(U32 stateIndex, Point &point, float &radius) const
{
   return false;
}


};
