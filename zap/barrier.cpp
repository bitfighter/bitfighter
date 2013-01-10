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

#include "barrier.h"
#include "WallSegmentManager.h"
#include "BotNavMeshZone.h"
#include "gameObjectRender.h"
#include "gameType.h"               // For BarrierRec struct
#include "game.h"
#include "config.h"
#include "stringUtils.h"

#ifndef ZAP_DEDICATED 
#  include "OpenglUtils.h"
#endif

#include <cmath>


using namespace TNL;

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Barrier);

Vector<Point> Barrier::mRenderLineSegments;



// Constructor
WallRec::WallRec(F32 width, bool solid, const Vector<F32> &verts)
{
   this->width = width;
   this->solid = solid;

   this->verts.resize(verts.size());

   for(S32 i = 0; i < verts.size(); i++)
      this->verts[i] = verts[i];
}


// Constructor
WallRec::WallRec(const WallItem *wallItem)
{
   width = (F32)wallItem->getWidth();
   solid = false;

   for(S32 i = 0; i < wallItem->getVertCount(); i++)
   {
      verts.push_back(wallItem->getVert(i).x);
      verts.push_back(wallItem->getVert(i).y);
   }
}


// Constructor
WallRec::WallRec(const PolyWall *polyWall)
{
   width = 1;      // Doesn't really matter... will be ignored
   solid = true;

   for(S32 i = 0; i < polyWall->getVertCount(); i++)
   {
      verts.push_back(polyWall->getVert(i).x);
      verts.push_back(polyWall->getVert(i).y);
   }
}


// Runs on server or on client, never in editor
// Generates a list of barriers, which are then added to the game one-by-one
// Barriers will either be a simple 2-point segment, or a longer list of vertices defining a polygon
void WallRec::constructWalls(Game *game) const
{
   Vector<Point> vec = floatsToPoints(verts);

   if(vec.size() < 2)
      return;

   if(solid)   // This is a polywall
   {
      if(vec.first() == vec.last())      // Does our barrier form a closed loop?
         vec.erase(vec.size() - 1);      // If so, remove last vertex

      Barrier *b = new Barrier(vec, width, true);
      b->addToGame(game, game->getGameObjDatabase());
   }
   else        // This is a standard series of segments
   {
      // First, fill a vector with barrier segments
      Vector<Point> barrierEnds;
      constructBarrierEndPoints(&vec, width, barrierEnds);

      Vector<Point> pts;
      // Then add individual segments to the game
      for(S32 i = 0; i < barrierEnds.size(); i += 2)
      {
         pts.clear();
         pts.push_back(barrierEnds[i]);
         pts.push_back(barrierEnds[i+1]);

         Barrier *b = new Barrier(pts, width, false);    // false = not solid
         b->addToGame(game, game->getGameObjDatabase());
      }
   }
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor --> gets called from constructBarriers above
Barrier::Barrier(const Vector<Point> &points, F32 width, bool solid)
{
   mObjectTypeNumber = BarrierTypeNumber;
   mPoints = points;

   if(points.size() < 2)      // Invalid barrier!
   {
      //delete this;    // Sam: cannot "delete this" in constructor, as "new" still returns non-NULL address
      logprintf(LogConsumer::LogWarning, "Invalid barrier detected (has only one point).  Disregarding...");
      return;
   }

   Rect extent(points);

   F32 w = abs(width);

   mWidth = w;                      // Must be positive to avoid problem with bufferBarrierForBotZone
   w = w * 0.5f + 1;                // Divide by 2 to avoid double size extents, add 1 to avoid rounding errors

   if(points.size() == 2)           // It's a regular segment, need to make a little larger to accomodate width
      extent.expand(Point(w, w));


   setExtent(extent);

   mSolid = solid;

   if(mSolid)  // Polywall
   {
      if(isWoundClockwise(mPoints))         // All walls must be CCW to clip correctly
         mPoints.reverse();

      Triangulate::Process(mPoints, mRenderFillGeometry);

      if(mRenderFillGeometry.size() == 0)    // Geometry is bogus; perhaps duplicated points, or other badness
      {
         logprintf(LogConsumer::LogWarning, "Invalid barrier detected (polywall with invalid geometry).  Disregarding...");
         return;
      }

      setNewGeometry(geomPolygon);
   }
   else     // Normal wall
   {
      if(mPoints.size() == 2 && mPoints[0] == mPoints[1])      // Test for zero-length barriers
         mPoints[1] += Point(0, 0.5);                          // Add vertical vector of half a point so we can see outline in-game

      if(mPoints.size() == 2 && mWidth != 0)                   // It's a regular segment, so apply width
         expandCenterlineToOutline(mPoints[0], mPoints[1], mWidth, mRenderFillGeometry);     // Fills mRenderFillGeometry with 4 points

      setNewGeometry(geomPolyLine);
   }

   // Outline is the same for regular walls and polywalls
   mRenderOutlineGeometry = getCollisionPoly(); 

   GeomObject::setGeom(*mRenderOutlineGeometry);

   // Compute a special buffered wall that makes computing bot zones easier
   computeBufferForBotZone(mPoints, mWidth, mSolid, mBufferedObjectPointsForBotZone);   
}


// Processes mPoints and fills polyPoints 
const Vector<Point> *Barrier::getCollisionPoly() const
{
   if(mSolid)
      return &mPoints;
   else
      return &mRenderFillGeometry;
}


bool Barrier::collide(BfObject *otherObject)
{
   return true;
}


// Server only
const Vector<Point> *Barrier::getBufferForBotZone()
{
   return &mBufferedObjectPointsForBotZone;
}


// Server only
void Barrier::computeBufferForBotZone(const Vector<Point> &points, F32 width, bool isPolywall, Vector<Point> &bufferedPoints)  // static
{
   // Use a clipper library to buffer polywalls; should be counter-clockwise by here
   if(isPolywall)
      offsetPolygon(&points, bufferedPoints, (F32)BotNavMeshZone::BufferRadius);

   // If a barrier, do our own buffer
   // Puffs out segment to the specified width with a further buffer for bot zones, has an inset tangent corner cut
   else
   {
      const Point &start = points[0];
      const Point &end   = points[1];
      Point difference   = end - start;

      Point crossVector(difference.y, -difference.x);  // Create a point whose vector from 0,0 is perpenticular to the original vector
      crossVector.normalize((width * 0.5f) + BotNavMeshZone::BufferRadius);  // Reduce point so the vector has length of barrier width + ship radius

      Point parallelVector(difference.x, difference.y); // Create a vector parallel to original segment
      parallelVector.normalize((F32)BotNavMeshZone::BufferRadius);  // Reduce point so vector has length of ship radius

      // For octagonal zones
      //   create extra vectors that are offset full offset to create 'cut' corners
      //   (FloatSqrtHalf * BotNavMeshZone::BufferRadius)  creates a tangent to the radius of the buffer
      //   we then subtract a little from the tangent cut to shorten the buffer on the corners and allow zones to be created when barriers are close
      Point crossPartial = crossVector;
      crossPartial.normalize((FloatSqrtHalf * BotNavMeshZone::BufferRadius) + (width * 0.5f) - (0.3f * BotNavMeshZone::BufferRadius));

      Point parallelPartial = parallelVector;
      parallelPartial.normalize((FloatSqrtHalf * BotNavMeshZone::BufferRadius) - (0.3f * BotNavMeshZone::BufferRadius));

      // Now add/subtract perpendicular and parallel vectors to buffer the segments
      bufferedPoints.push_back((start - parallelVector)  + crossPartial);
      bufferedPoints.push_back((start - parallelPartial) + crossVector);
      bufferedPoints.push_back((end   + parallelPartial) + crossVector);
      bufferedPoints.push_back((end   + parallelVector)  + crossPartial);
      bufferedPoints.push_back((end   + parallelVector)  - crossPartial);
      bufferedPoints.push_back((end   + parallelPartial) - crossVector);
      bufferedPoints.push_back((start - parallelPartial) - crossVector);
      bufferedPoints.push_back((start - parallelVector)  - crossPartial);
   }
}


// Merges wall outlines together, client only
// This is used for barriers and polywalls
void Barrier::prepareRenderingGeometry(Game *game)    // static
{
   mRenderLineSegments.clear();

   Vector<DatabaseObject *> barrierList;

   game->getGameObjDatabase()->findObjects((TestFunc)isWallType, barrierList);

   clipRenderLinesToPoly(barrierList, mRenderLineSegments);
}


// Clears out overlapping barrier lines for better rendering appearance, modifies lineSegmentPoints.
// This is effectively called on every pair of potentially intersecting barriers, and lineSegmentPoints gets 
// refined as each additional intersecting barrier gets processed.
void Barrier::clipRenderLinesToPoly(const Vector<DatabaseObject *> &barrierList, Vector<Point> &lineSegmentPoints)  
{
   Vector<Vector<Point> > solution;

   unionBarriers(barrierList, solution);

   unpackPolygons(solution, lineSegmentPoints);
}


// Combines multiple barriers into a single complex polygon... fills solution
bool Barrier::unionBarriers(const Vector<DatabaseObject *> &barriers, Vector<Vector<Point> > &solution)
{
   Vector<const Vector<Point> *> inputPolygons;
   Vector<Point> points;

   for(S32 i = 0; i < barriers.size(); i++)
   {
      if(barriers[i]->getObjectTypeNumber() != BarrierTypeNumber)
         continue;

      inputPolygons.push_back(static_cast<Barrier *>(barriers[i])->getCollisionPoly());
   }

   return mergePolys(inputPolygons, solution);
}


// Render wall fill only for this wall; all edges rendered in a single pass later
void Barrier::render(S32 layerIndex)
{
#ifndef ZAP_DEDICATED
   if(layerIndex == 0)           // First pass: draw the fill
   {
      glColor(getGame()->getSettings()->getWallFillColor());
      renderWallFill(&mRenderFillGeometry, mSolid);
   }
#endif
}


// Render all edges for all barriers... faster to do it all at once than try to sort out whose edges are whose
void Barrier::renderEdges(S32 layerIndex, const Color &outlineColor)  // static
{
   if(layerIndex == 1)
      renderWallEdges(&mRenderLineSegments, outlineColor);
}


S32 Barrier::getRenderSortValue()
{
   return 0;
}


////////////////////////////////////////
////////////////////////////////////////

// WallItem is child of LineItem... the only thing LineItem brings to the party is width

// Combined C++/Lua constructor
WallItem::WallItem(lua_State *L)
{
   mObjectTypeNumber = WallItemTypeNumber;
   mWidth = Barrier::DEFAULT_BARRIER_WIDTH;
   mAlreadyAdded = false;

   setNewGeometry(geomPolyLine);

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
bool WallItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 6)         // "BarrierMaker" keyword, width, and two or more x,y pairs
      return false;

   setWidth(atoi(argv[1]));

   readGeom(argc, argv, 2, game->getGridSize());

   updateExtentInDatabase();

   return true;
}


string WallItem::toString(F32 gridSize) const
{
   return appendId("BarrierMaker") + " " + itos(getWidth()) + " " + geomToString(gridSize);
}


static void setWallSelected(GridDatabase *database, S32 serialNumber, bool selected)
{
   // Find the associated segment(s) and mark them as selected (or not)
   if(database)
      database->getWallSegmentManager()->setSelected(serialNumber, selected);
}


static F32 getWallEditorRadius(F32 currentScale)
{
   return (F32)WallItem::VERTEX_SIZE;   // Keep vertex hit targets the same regardless of scale
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
void WallItem::render()
{
   // Do nothing
}


void WallItem::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   if(!isSelected() && !isLitUp())
      glColor(getEditorRenderColor());

   renderLine(getOutline());
   renderPolyLineVertices(this, snappingToWallCornersEnabled, currentScale);
#endif
}



void WallItem::processEndPoints()
{
#ifndef ZAP_DEDICATED
   constructBarrierEndPoints(getOutline(), (F32)getWidth(), extendedEndPoints);     // Fills extendedEndPoints
#endif
}


Rect WallItem::calcExtents()
{
   // mExtent was already calculated when the wall was inserted into the segmentManager...
   // All we need to do here is override the default calcExtents, to avoid clobbering our already good mExtent.
   return getExtent();     
}


const char *WallItem::getOnScreenName()     { return "Wall";  }
const char *WallItem::getOnDockName()       { return "Wall";  }
const char *WallItem::getPrettyNamePlural() { return "Walls"; }
const char *WallItem::getEditorHelpString() { return "Walls define the general form of your level."; }

string WallItem::getAttributeString()     { return "Width: " + itos(getWidth()); }
const char *WallItem::getInstructionMsg() { return "[+] and [-] to change";      }


bool WallItem::hasTeam()      { return false; }
bool WallItem::canBeHostile() { return false; }
bool WallItem::canBeNeutral() { return false; }


const Color *WallItem::getEditorRenderColor()
{
   return &Colors::gray50;    // Color of wall spine in editor
}


// Size of object in editor 
F32 WallItem::getEditorRadius(F32 currentScale)
{
   return getWallEditorRadius(currentScale);
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
   setWallSelected(getDatabase(), getSerialNumber(), selected);
}


// Here to provide a valid signature in WallItem
void WallItem::addToGame(Game *game, GridDatabase *database)
{
   Parent::addToGame(game, database);

   // Convert the wallItem in to a wallRec, an abbreviated form of wall that represents both regular walls and polywalls, and 
   // is convenient to transmit to the clients
   WallRec wallRec(this);
   game->getGameType()->addWall(wallRec, game);

   onAddedToGame(game);
}


/////
// Lua interface
/**
  *  @luaclass WallItem
  *  @brief Traditional wall item.
  *  @descr A %WallItem is a traditional wall consisting of a series of straight-line segments.  WallItems have a width setting that 
  *         expands the walls outward on the edges, but not the ends.  The game may make slight adjustments to the interior vertices
  *         of a wall to improve visual appearance.  Collinear vertices may be deleted to simplify wall geometry.
  *
  *  @geom %WallItem geometry consists of two or more points forming a linear sequence, each consecutive pair defining a straight-line 
  *        segment.  %WallItem geometry can cross or form loops with no adverse consequences.
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
  *  @luafunc num WallItem::getWidth()
  *  @brief   Returns %WallItem's width setting.
  *  @descr   Walls have a default width of 50.
  *  @return  \e width: \e Int representing %WallItem's width.
  */
S32 WallItem::getWidth(lua_State *L)     
{ 
   return returnInt(L, getWidth()); 
}


/**
  *  @luafunc WallItem::setWidth(width)
  *  @brief   Sets %WallItem's width.
  *  @descr   Walls have a default width of 50.
  *  @param  \e width: \e Int representing %WallItem's width.
  */
S32 WallItem::setWidth(lua_State *L)     
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

S32 WallItem::setLoc(lua_State *L)
{
   checkIfHasBeenAddedToTheGame(L);
   return Parent::setLoc(L);
}


S32 WallItem::setGeom(lua_State *L)
{
   checkIfHasBeenAddedToTheGame(L);
   return Parent::setGeom(L);
}


////////////////////////////////////////
////////////////////////////////////////

extern Color EDITOR_WALL_FILL_COLOR;

TNL_IMPLEMENT_NETOBJECT(PolyWall);


// Combined Lua/C++ constructor
PolyWall::PolyWall(lua_State *L)
{
   mObjectTypeNumber = PolyWallTypeNumber;
   mAlreadyAdded = false;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


PolyWall::~PolyWall()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


PolyWall *PolyWall::clone() const
{
   PolyWall *polyWall = new PolyWall(*this);
   return polyWall;
}


S32 PolyWall::getRenderSortValue()
{
   return -1;
}


void PolyWall::renderDock()
{
   renderPolygonFill(getFill(), &EDITOR_WALL_FILL_COLOR);
   renderPolygonOutline(getOutline(), getGame()->getSettings()->getWallOutlineColor());
}


bool PolyWall::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 7)            // Need "Polywall" keyword, and at least 3 points
      return false;

   S32 offset = 0;

   if(!stricmp(argv[0], "BarrierMakerS"))
   {
      logprintf(LogConsumer::LogLevelError, "BarrierMakerS has been deprecated.  Please use PolyWall instead.");
      offset = 1;
   }

   readGeom(argc, argv, 1 + offset, game->getGridSize());

   return true;
}


string PolyWall::toString(F32 gridSize) const
{
   return string(appendId(getClassName())) + " " + geomToString(gridSize);
}


// Size of object in editor 
F32 PolyWall::getEditorRadius(F32 currentScale)
{
   return getWallEditorRadius(currentScale);
}


const char *PolyWall::getOnScreenName()     { return "PolyWall";  }
const char *PolyWall::getOnDockName()       { return "PolyWall";  }
const char *PolyWall::getPrettyNamePlural() { return "PolyWalls"; }
const char *PolyWall::getEditorHelpString() { return "Polygonal wall item lets you be creative with your wall design."; }


void PolyWall::setSelected(bool selected)
{
   Parent::setSelected(selected);

   setWallSelected(getDatabase(), getSerialNumber(), selected);
}


// Only called from editor
void PolyWall::onGeomChanged()
{
   GridDatabase *db = getDatabase();

   if(db)      // db might be NULL if PolyWall hasn't yet been added to the editor (e.g. if it's still a figment of Lua's fancy)
   {
      db->getWallSegmentManager()->onWallGeomChanged(db, this, isSelected(), getSerialNumber());
      Parent::onGeomChanged();
   }
}


void PolyWall::addToGame(Game *game, GridDatabase *database)
{
   Parent::addToGame(game, database);

   // Convert the wallItem in to a wallRec, an abbreviated form of wall that represents both regular walls and polywalls, and 
   // is convenient to transmit to the clients
   WallRec wallRec(this);
   game->getGameType()->addWall(wallRec, game);

   onAddedToGame(game);
}


void PolyWall::onItemDragging()
{
   // Do nothing -- this is here to override PolygonObject::onItemDragging(), onGeomChanged() should only be called after move is complete
}


// PolyWalls are not really added to the game in the sense of other objects; rather their geometry is used
// to create Barriers that are added directly.  Here we will mark the item as added (to catch errors in Lua
// scripts that attempt to modify an added item), but we have no need to pass the event handler up the stack
// to superclass event handlers.
void PolyWall::onAddedToGame(Game *game)
{
  Parent::onAddedToGame(game);
  mAlreadyAdded = true;
}


/////
// Lua interface

/**
  *  @luaclass PolyWall
  *  @brief Polygonal wall item.
  *  @descr A %PolyWall is a wall consisting of a filled polygonal shape.  
  *
  *  @geom %PolyWall geometry is a typical polygon.
  */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_FUNARGS_TABLE(PolyWall, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE_NEW(PolyWall, LUA_METHODS);

#undef LUA_METHODS


const char *PolyWall::luaClassName = "PolyWall";
REGISTER_LUA_SUBCLASS(PolyWall, BfObject);


void PolyWall::checkIfHasBeenAddedToTheGame(lua_State *L)
{
   if(mAlreadyAdded)
   {
      ScriptContext context = getScriptContext(L);

      if(context != PluginContext)     // Plugins can alter walls that are already in-game... levelgens cannot
      {
         const char *msg = "Can't modify a PolyWall that's already been added to a game!";
         logprintf(LogConsumer::LogError, msg);
         throw LuaException(msg);
      }
   }
}


// Lua method overrides.  Because walls are... special.

S32 PolyWall::setLoc(lua_State *L)
{
   checkIfHasBeenAddedToTheGame(L);
   return Parent::setLoc(L);
}


S32 PolyWall::setGeom(lua_State *L)
{
   checkIfHasBeenAddedToTheGame(L);
   return Parent::setGeom(L);
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

   // Add item to database, set its extents
   addToDatabase(database, Rect(mCorners));
   
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


void WallSegment::renderFill(const Point &offset)
{
#ifndef ZAP_DEDICATED
   if(mSelected)
      renderWallFill(&mTriangulatedFillPoints, offset, true);       // Use true because all segment fills are triangulated
   else
      renderWallFill(&mTriangulatedFillPoints, true);
#endif
}


bool WallSegment::isSelected()
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
   return &mEdges;
}


bool WallSegment::getCollisionCircle(U32 stateIndex, Point &point, float &radius) const
{
   return false;
}


};
