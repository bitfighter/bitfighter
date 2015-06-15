//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "WallItem.h"

#include "barrier.h"
#include "Colors.h"
#include "GameObjectRender.h"
#include "GeomUtils.h"
#include "Level.h"
#include "stringUtils.h"

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

      LUA_REGISTER_WITH_TRACKER;
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
   WallItem *wallItem = new WallItem(*this);
   wallItem->cloneSegments(this);

   return wallItem;
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


bool WallItem::overlapsPoint(const Point &point) const
{
   return isPointOnWall(point);
}


// Override
bool WallItem::checkForCollision(const Point &rayStart, const Point &rayEnd, bool format, U32 stateIndex,
                                 F32 &collisionTime, Point &surfaceNormal) const
{
   F32 ct;
   Point norm;
   bool found = false;

   for(S32 i = 0; i < getSegmentCount(); i++)
   {
      if(getSegment(i)->checkForCollision(rayStart, rayEnd, format, stateIndex, ct, norm))
      {
         if(ct < collisionTime)
         {
            collisionTime = ct;
            surfaceNormal = norm;
            found = true;
         }
      }
   }

   return found;
}


void WallItem::onGeomChanged()
{
   // Fill extendedEndPoints from the vertices of our wall's centerline, or from PolyWall edges
   computeExtendedEndPoints();

   Vector<WallSegment *> segments;

   // Create a WallSegment for each sequential pair of vertices
   for(S32 i = 0; i < extendedEndPoints.size(); i += 2)
   {
      // Create the segment; the WallSegment constructor will add it to the specified database
      WallSegment *newSegment = new WallSegment(extendedEndPoints[i], extendedEndPoints[i+1], (F32)getWidth(), this);
      segments.push_back(newSegment);
   }

   setSegments(segments);

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
      GameObjectRender::renderWallSpine(this, getOutline(), currentScale, snappingToWallCornersEnabled, renderVertices);
   else
      GameObjectRender::renderWallSpine(this, getOutline(), getEditorRenderColor(), currentScale, snappingToWallCornersEnabled, renderVertices);
#endif
}


void WallItem::computeExtendedEndPoints()
{
#ifndef ZAP_DEDICATED
   // Extend wall endpoints for nicer rendering
   constructBarrierEndPoints(getOutline(), (F32)getWidth(), extendedEndPoints);     // Fills extendedEndPoints
#endif
}


// TODO: Should return const ref rather than copy
Rect WallItem::calcExtents() const
{
   return getSegmentExtent();
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
   return (F32)EditorObject::VERTEX_SIZE;   // Keep vertex hit targets the same regardless of scale
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
         THROW_LUA_EXCEPTION(L, msg);
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
   return &mPoints;     // Will contain 2 points
}


bool WallEdge::getCollisionCircle(U32 stateIndex, Point &point, float &radius) const
{
   return false;
}


////////////////////////////////////////
////////////////////////////////////////

// Regular constructor
WallSegment::WallSegment(const Point &start, const Point &end, F32 width, BarrierX *owner) 
{ 
   // Calculate segment corners by expanding the extended end points into a rectangle
   expandCenterlineToOutline(start, end, width, mCorners);  // ==> Fills mCorners 
   init(owner);
}


// PolyWall constructor
WallSegment::WallSegment(const Vector<Point> &points, BarrierX *owner)
{
   mCorners = points;

   if(isWoundClockwise(points))
      mCorners.reverse();

   init(owner);
}


WallSegment::WallSegment(const WallSegment *source, BarrierX *owner)
{
   mCorners = *source->getCorners();
   init(owner);
}


// Intialize, only called from constructors above
void WallSegment::init(BarrierX *owner)
{
   mOwner = owner;

   // Recompute the edges based on our new corner points
   resetEdges();   

   mObjectTypeNumber = WallSegmentTypeNumber;

   setExtent(Rect(mCorners));

   // Drawing filled wall requires that points be triangluated
   Triangulate::Process(mCorners, mTriangulatedFillPoints);    // ==> Fills mTriangulatedFillPoints

   invalid = false; 
}


// Destructor
WallSegment::~WallSegment()
{ 
   // Make sure object is out of the database -- but don't delete it since we're destructing
   if(getDatabase())
      getDatabase()->removeFromDatabase(this, false); 
}


void WallSegment::invalidate()
{
   invalid = true;
}
 

BarrierX *WallSegment::getOwner() const
{
   return mOwner;
}


// Resets edges of a wall segment to their factory settings; i.e. 4 simple walls representing a simple outline
void WallSegment::resetEdges()
{
   cornersToEdges(mCorners, mEdges);
}


void WallSegment::renderFill(const Point &offset, const Color &color, bool isSelected) const
{
#ifndef ZAP_DEDICATED
   if(isSelected)
      GameObjectRender::renderWallFill(&mTriangulatedFillPoints, color, offset, true);    // true ==> all segment fills are triangulated
   else
      GameObjectRender::renderWallFill(&mTriangulatedFillPoints, color, true);
#endif
}


const Vector<Point> *WallSegment::getCorners() const                { return &mCorners; }
const Vector<Point> *WallSegment::getEdges() const                  { return &mEdges; }
const Vector<Point> *WallSegment::getTriangulatedFillPoints() const { return &mTriangulatedFillPoints; }


const Vector<Point> *WallSegment::getCollisionPoly() const
{
   return &mCorners;
}


bool WallSegment::getCollisionCircle(U32 stateIndex, Point &point, float &radius) const
{
   return false;
}


};
