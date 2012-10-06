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
#   include "OpenglUtils.h"
#endif

#include <cmath>


using namespace TNL;

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Barrier);

Vector<Point> Barrier::mRenderLineSegments;

bool loadBarrierPoints(const WallRec *barrier, Vector<Point> &points)
{
   // Convert the list of floats into a list of points
   for(S32 i = 1; i < barrier->verts.size(); i += 2)
      points.push_back( Point(barrier->verts[i-1], barrier->verts[i]) );

   removeCollinearPoints(points, false);   // Remove collinear points to make rendering nicer and datasets smaller

   return (points.size() >= 2);
}


////////////////////////////////////////
////////////////////////////////////////


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
WallRec::WallRec(const WallItem &wallItem)
{
   width = (F32)wallItem.getWidth();
   solid = false;

   for(S32 i = 0; i < wallItem.getVertCount(); i++)
   {
      verts.push_back(wallItem.getVert(i).x);
      verts.push_back(wallItem.getVert(i).y);
   }
}


// Constructor
WallRec::WallRec(const PolyWall &polyWall)
{
   width = 1;      // Doesn't really matter... will be ignored
   solid = true;

   for(S32 i = 0; i < polyWall.getVertCount(); i++)
   {
      verts.push_back(polyWall.getVert(i).x);
      verts.push_back(polyWall.getVert(i).y);
   }
}


// Runs on server or on client, never in editor
void WallRec::constructWalls(Game *theGame) const
{
   Vector<Point> vec;

   if(!loadBarrierPoints(this, vec))
      return;

   if(solid)   // This is a polywall
   {
      if(vec.first() == vec.last())      // Does our barrier form a closed loop?
         vec.erase(vec.size() - 1);      // If so, remove last vertex

      Barrier *b = new Barrier(vec, width, true);
      b->addToGame(theGame, theGame->getGameObjDatabase());
   }
   else        // This is a standard series of segments
   {
      // First, fill a vector with barrier segments
      Vector<Point> barrierEnds;
      Barrier::constructBarrierEndPoints(&vec, width, barrierEnds);

      Vector<Point> pts;
      // Then add individual segments to the game
      for(S32 i = 0; i < barrierEnds.size(); i += 2)
      {
         pts.clear();
         pts.push_back(barrierEnds[i]);
         pts.push_back(barrierEnds[i+1]);

         Barrier *b = new Barrier(pts, width, false);    // false = not solid
         b->addToGame(theGame, theGame->getGameObjDatabase());
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

   if(width < 0)             // Force positive width
      width = -width;

   mWidth = width;           // must be positive to avoid problem with bufferBarrierForBotZone
   width = width * 0.5f + 1;  // divide by 2 to avoid double size extents, add 1 to avoid rounding errors.
   if(points.size() == 2)    // It's a regular segment, need to make a little larger to accomodate width
      extent.expand(Point(width, width));
   // use mWidth, not width, for anything below this.

   setExtent(extent);

    mSolid = solid;

   if(mSolid)
   {
      if (isWoundClockwise(mPoints))  // all walls must be CCW to clip correctly
         mPoints.reverse();

      Triangulate::Process(mPoints, mRenderFillGeometry);

      if(mRenderFillGeometry.size() == 0)      // Geometry is bogus; perhaps duplicated points, or other badness
      {
         //delete this;    // Sam: cannot "delete this" in constructor, as "new" still returns non-NULL address
         logprintf(LogConsumer::LogWarning, "Invalid barrier detected (polywall with invalid geometry).  Disregarding...");
         return;
      }
   }
   else
   {
      if (mPoints.size() == 2 && mPoints[0] == mPoints[1])   // Test for zero-length barriers
         mPoints[1] += Point(0,0.5);                         // Add vertical vector of half a point so we can see outline geo in-game

      if(mPoints.size() == 2 && mWidth != 0)   // It's a regular segment, so apply width
         expandCenterlineToOutline(mPoints[0], mPoints[1], mWidth, mRenderFillGeometry);     // Fills with 4 points
   }

   getCollisionPoly(mRenderOutlineGeometry);    // Outline is the same for both barrier geometries
   computeBufferForBotZone(mBufferedObjectPointsForBotZone);
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

      inputPolygons.push_back(static_cast<Barrier*>(barriers[i])->getCollisionPolyPtr());
   }

   return mergePolys(inputPolygons, solution);
}


// Processes mPoints and fills polyPoints 
bool Barrier::getCollisionPoly(Vector<Point> &polyPoints) const
{
   if(mSolid)
      polyPoints = mPoints;
   else
      polyPoints = mRenderFillGeometry;

   return true;
}


const Vector<Point> *Barrier::getCollisionPolyPtr() const
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


// Takes a list of vertices and converts them into a list of lines representing the edges of an object -- static method
void Barrier::resetEdges(const Vector<Point> &corners, Vector<Point> &edges)
{
   edges.clear();

   S32 last = corners.size() - 1;             
   for(S32 i = 0; i < corners.size(); i++)
   {
      edges.push_back(corners[last]);
      edges.push_back(corners[i]);
      last = i;
   }
}


// Given the points in vec, figure out where the ends of the walls should be (they'll need to be extended slighly in some cases
// for better rendering).  Set extendAmt to 0 to see why it's needed.
// Populates barrierEnds with the results.
void Barrier::constructBarrierEndPoints(const Vector<Point> *vec, F32 width, Vector<Point> &barrierEnds)    // static
{
   barrierEnds.clear();       // Local static vector

   if(vec->size() <= 1)       // Protect against bad data
      return;

   bool loop = (vec->first() == vec->last());      // Does our barrier form a closed loop?

   Vector<Point> edgeVector;
   for(S32 i = 0; i < vec->size() - 1; i++)
   {
      Point e = vec->get(i+1) - vec->get(i);
      e.normalize();
      edgeVector.push_back(e);
   }

   Point lastEdge = edgeVector[edgeVector.size() - 1];
   Vector<F32> extend;

   for(S32 i = 0; i < edgeVector.size(); i++)
   {
      Point curEdge = edgeVector[i];
      double cosTheta = curEdge.dot(lastEdge);

      // Do some bounds checking.  Crazy, I know, but trust me, it's worth it!
      if (cosTheta > 1.0)
         cosTheta = 1.0;
      else if(cosTheta < -1.0)  
         cosTheta = -1.0;

      cosTheta = abs(cosTheta);     // Seems to reduce "end gap" on acute junction angles
      
      F32 extendAmt = width * 0.5f * F32(tan( acos(cosTheta) / 2 ));
      if(extendAmt > 0.01f)
         extendAmt -= 0.01f;
      extend.push_back(extendAmt);
   
      lastEdge = curEdge;
   }

   F32 first = extend[0];
   extend.push_back(first);

   for(S32 i = 0; i < edgeVector.size(); i++)
   {
      F32 extendBack = extend[i];
      F32 extendForward = extend[i+1];
      if(i == 0 && !loop)
         extendBack = 0;
      if(i == edgeVector.size() - 1 && !loop)
         extendForward = 0;

      Point start = vec->get(i) - edgeVector[i] * extendBack;
      Point end = vec->get(i+1) + edgeVector[i] * extendForward;

      barrierEnds.push_back(start);
      barrierEnds.push_back(end);
   }
}


// Simply takes a segment and "puffs it out" to a rectangle of a specified width, filling cornerPoints.  Does not modify endpoints.
void Barrier::expandCenterlineToOutline(const Point &start, const Point &end, F32 width, Vector<Point> &cornerPoints)
{
   cornerPoints.clear();

   Point dir = end - start;
   Point crossVec(dir.y, -dir.x);
   crossVec.normalize(width * 0.5f);

   cornerPoints.push_back(start + crossVec);
   cornerPoints.push_back(end + crossVec);
   cornerPoints.push_back(end - crossVec);
   cornerPoints.push_back(start - crossVec);
}


// Server only
const Vector<Point> *Barrier::getBufferForBotZone()
{
   return &mBufferedObjectPointsForBotZone;
}


// Server only
void Barrier::computeBufferForBotZone(Vector<Point> &bufferedPoints)
{
   // Use a clipper library to buffer polywalls; should be counter-clockwise by here
   if(mSolid)
      offsetPolygon(&mPoints, bufferedPoints, (F32)BotNavMeshZone::BufferRadius);

   // If a barrier, do our own buffer
   // Puffs out segment to the specified width with a further buffer for bot zones, has an inset tangent corner cut
   else
   {
      Point& start = mPoints[0];
      Point& end = mPoints[1];
      Point difference = end - start;

      Point crossVector(difference.y, -difference.x);  // Create a point whose vector from 0,0 is perpenticular to the original vector
      crossVector.normalize((mWidth * 0.5f) + BotNavMeshZone::BufferRadius);  // reduce point so the vector has length of barrier width + ship radius

      Point parallelVector(difference.x, difference.y); // Create a vector parallel to original segment
      parallelVector.normalize((F32)BotNavMeshZone::BufferRadius);  // Reduce point so vector has length of ship radius

      // For octagonal zones
      //   create extra vectors that are offset full offset to create 'cut' corners
      //   (FloatSqrtHalf * BotNavMeshZone::BufferRadius)  creates a tangent to the radius of the buffer
      //   we then subtract a little from the tangent cut to shorten the buffer on the corners and allow zones to be created when barriers are close
      Point crossPartial = crossVector;
      crossPartial.normalize((FloatSqrtHalf * BotNavMeshZone::BufferRadius) + (mWidth * 0.5f) - (0.3f * BotNavMeshZone::BufferRadius));

      Point parallelPartial = parallelVector;
      parallelPartial.normalize((FloatSqrtHalf * BotNavMeshZone::BufferRadius) - (0.3f * BotNavMeshZone::BufferRadius));

      // Now add/subtract perpendicular and parallel vectors to buffer the segments
      bufferedPoints.push_back((start - parallelVector)  + crossPartial);
      bufferedPoints.push_back((start - parallelPartial)  + crossVector);
      bufferedPoints.push_back(end   + parallelPartial + crossVector);
      bufferedPoints.push_back(end   + parallelVector  + crossPartial);
      bufferedPoints.push_back(end   + parallelVector  - crossPartial);
      bufferedPoints.push_back(end   + parallelPartial - crossVector);
      bufferedPoints.push_back((start - parallelPartial) - crossVector);
      bufferedPoints.push_back((start - parallelVector)  - crossPartial);
   }
}


// Clears out overlapping barrier lines for better rendering appearance, modifies lineSegmentPoints.
// This is effectively called on every pair of potentially intersecting barriers, and lineSegmentPoints gets 
// refined as each additional intersecting barrier gets processed.
// static method
void Barrier::clipRenderLinesToPoly(const Vector<DatabaseObject *> &barrierList, Vector<Point> &lineSegmentPoints)
{
   Vector<Vector<Point> > solution;

   unionBarriers(barrierList, solution);

   unpackPolygons(solution, lineSegmentPoints);
}


// Merges wall outlines together, client only
// This is used for barriers and polywalls
void Barrier::prepareRenderingGeometry(Game *game)
{
   mRenderLineSegments.clear();

   Vector<DatabaseObject *> barrierList;

   game->getGameObjDatabase()->findObjects((TestFunc)isWallType, barrierList);

   clipRenderLinesToPoly(barrierList, mRenderLineSegments);
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


// static 
void Barrier::renderEdges(S32 layerIndex, const Color &outlineColor)
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

// Constructor
WallItem::WallItem()
{
   mObjectTypeNumber = WallItemTypeNumber;
   setWidth(Barrier::DEFAULT_BARRIER_WIDTH);
   mAddedToGame = false;

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


// Only called from editor
void WallItem::onGeomChanged()
{
   // Fill extendedEndPoints from the vertices of our wall's centerline, or from PolyWall edges
   processEndPoints();

   GridDatabase *db = getDatabase();

   db->getWallSegmentManager()->onWallGeomChanged(db, this, isSelected(), getSerialNumber());
   Parent::onGeomChanged();
}


// Only called in editor during preview mode -- basicaly prevents parent class from rendering spine of wall
void WallItem::render()
{
   // Do nothing
}


// Only called in editor?
void WallItem::processEndPoints()
{
#ifndef ZAP_DEDICATED
   Barrier::constructBarrierEndPoints(getOutline(), (F32)getWidth(), extendedEndPoints);     // Fills extendedEndPoints
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

string WallItem::getAttributeString() { return "Width: " + itos(getWidth()); }
const char *WallItem::getInstructionMsg() { return "[+] and [-] to change"; }


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


string WallItem::toString(F32 gridSize) const
{
   return "BarrierMaker " + itos(getWidth()) + " " + geomToString(gridSize);
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
   return Parent::getWidth();
}


void WallItem::setWidth(S32 width) 
{         
   Parent::setWidth(width, Barrier::MIN_BARRIER_WIDTH, Barrier::MAX_BARRIER_WIDTH);     // Why do we need Barrier:: prefix here???
}


void WallItem::setSelected(bool selected)
{
   Parent::setSelected(selected);
   
   // Find the associated segment(s) and mark them as selected (or not)
   setWallSelected(getDatabase(), getSerialNumber(), selected);
}


void WallItem::onItemDragging()
{
   // Do nothing -- this is here to override Parent::onItemDragging(), onGeomChanged() should only be called after move is complete
}


// Client (i.e. editor) only; walls processed in ServerGame::processPseudoItem() on server
bool WallItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 6)         // Need "BarrierMaker" keyword, width, and two or more x,y pairs
      return false;

   return processGeometry(argc, argv, game);
}


// Here to provide a valid signature in WallItem
void WallItem::addToGame(Game *game, GridDatabase *database)
{
   Parent::addToGame(game, database);
}


/////
// Lua interface

//               Fn name       Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getWidth,     ARRAYDEF({{      END }}), 1 ) \
   METHOD(CLASS, setWidth,     ARRAYDEF({{ INT, END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(WallItem, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(WallItem, LUA_METHODS);

#undef LUA_METHODS


const char *WallItem::luaClassName = "WallItem";
REGISTER_LUA_SUBCLASS(WallItem, BfObject);


S32 WallItem::getWidth(lua_State *L)     { return returnInt(L, getWidth()); }
S32 WallItem::setWidth(lua_State *L)     
{ 
   checkIfWallHasBeenAddedToTheGame();

   checkArgList(L, functionArgs, "WallItem", "setWidth");

   setWidth(getInt(L, 1));

   return 0; 
}


void WallItem::checkIfWallHasBeenAddedToTheGame()
{
   if(mAddedToGame)
   {
      const char *msg = "Can't modify a wall that's already been added to a game!";
      logprintf(LogConsumer::LogError, msg);
      throw LuaException(msg);
   }
}


// Lua method overrides.  Because walls are... special.

// Wall items are basically pseudo items that need to be added via a non-standard mechanism
S32 WallItem::addToGame(lua_State *L)
{
   WallRec wallRec(*this);
   Game *game = Game::getAddTarget();
   game->getGameType()->addWall(wallRec, game);
   mAddedToGame = true;    // To prevent further modification once this has been added to game

   return 0;
}


S32 WallItem::setLoc(lua_State *L)
{
   checkIfWallHasBeenAddedToTheGame();
   return Parent::setLoc(L);
}


S32 WallItem::setGeom(lua_State *L)
{
   checkIfWallHasBeenAddedToTheGame();
   return Parent::setGeom(L);
}


////////////////////////////////////////
////////////////////////////////////////

extern Color EDITOR_WALL_FILL_COLOR;

TNL_IMPLEMENT_NETOBJECT(PolyWall);

const char PolyWall::className[] = "PolyWall";      // Class name as it appears to Lua scripts


// Constructor
PolyWall::PolyWall() : Parent()
{
   mObjectTypeNumber = PolyWallTypeNumber;
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
   return string(getClassName()) + " " + geomToString(gridSize);
}


// Size of object in editor 
F32 PolyWall::getEditorRadius(F32 currentScale)
{
   return getWallEditorRadius(currentScale);
}


const char *PolyWall::getEditorHelpString()
{
   return "Polygonal wall item lets you be creative with your wall design.";
}


const char *PolyWall::getPrettyNamePlural()
{
   return "PolyWalls";
}


const char *PolyWall::getOnDockName()
{
   return "PolyWall";
}


const char *PolyWall::getOnScreenName()
{
   return "PolyWall";
}


void PolyWall::setSelected(bool selected)
{
   Parent::setSelected(selected);

   setWallSelected(getDatabase(), getSerialNumber(), selected);
}


// Only called from editor
void PolyWall::onGeomChanged()
{
   GridDatabase *db = getDatabase();

   db->getWallSegmentManager()->onWallGeomChanged(db, this, isSelected(), getSerialNumber());
   Parent::onGeomChanged();
}


void PolyWall::onItemDragging()
{
   // Do nothing -- this is here to override PolygonObject::onItemDragging(), onGeomChanged() should only be called after move is complete
}


/////
// Lua interface  ==>  don't need these!!

//  Lua constructor
PolyWall::PolyWall(lua_State *L)
{
   /* Do nothing */
}


S32 PolyWall::getClassID(lua_State *L)
{
   return returnInt(L, PolyWallTypeNumber);
}


void PolyWall::push(lua_State *L)
{
   Lunar<PolyWall>::push(L, this);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
WallEdge::WallEdge(const Point &start, const Point &end, GridDatabase *database) 
{ 
   mStart = start; 
   mEnd = end; 

   addToDatabase(database, Rect(start, end));

   // Set some things required by DatabaseObject
   mObjectTypeNumber = WallEdgeTypeNumber;
}


// Destructor
WallEdge::~WallEdge()
{
    // Make sure object is out of the database
   removeFromDatabase(); 
}


Point *WallEdge::getStart()
{
   return &mStart;
}


Point *WallEdge::getEnd()
{
   return &mEnd;
}


bool WallEdge::getCollisionPoly(Vector<Point> &polyPoints) const
{
   polyPoints.resize(2);
   polyPoints[0] = mStart;
   polyPoints[1] = mEnd;
   return true;
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
   Barrier::expandCenterlineToOutline(start, end, width, mCorners);  // ==> Fills mCorners 
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

   // Add item to database, set its extents.  Roughly equivalent to addToDatabase(database, Rect(corners))
   addToDatabase(database, Rect(mCorners));
   
   // Drawing filled wall requires that points be triangluated
   Triangulate::Process(mCorners, mTriangulatedFillPoints);    // ==> Fills mTriangulatedFillPoints

   mOwner = owner; 
   invalid = false; 
   mSelected = false;

   /////
   // Set some things required by DatabaseObject
   mObjectTypeNumber = WallSegmentTypeNumber;
}


// Destructor
WallSegment::~WallSegment()
{ 
   // Make sure object is out of the database
   if(getDatabase())
      getDatabase()->removeFromDatabase(this); 
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
   Barrier::resetEdges(mCorners, mEdges);
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


const Vector<Point> *WallSegment::getCorners()
{
   return &mCorners;
}


const Vector<Point> *WallSegment::getEdges()
{
   return &mEdges;
}


const Vector<Point> *WallSegment::getTriangulatedFillPoints()
{
   return &mTriangulatedFillPoints;
}


bool WallSegment::getCollisionPoly(Vector<Point> &polyPoints) const
{
   polyPoints = mEdges;
   return true;
}


bool WallSegment::getCollisionCircle(U32 stateIndex, Point &point, float &radius) const
{
   return false;
}


};
