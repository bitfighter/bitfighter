//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _BARRIER_H_
#define _BARRIER_H_

#include "BfObject.h"
#include "polygon.h"       // For PolygonObject def
#include "LineItem.h"   

#include "Point.h"
#include "tnlVector.h"
#include "tnlNetObject.h"

#include "gtest/gtest.h"


namespace Zap
{

/// The Barrier class represents rectangular barriers that player controlled
/// ships cannot pass through... i.e. walls  Barrier objects, once created, never
/// change state, simplifying the pack/unpack update methods.  Barriers are
/// constructed as an expanded line segment.
class Barrier : public BfObject
{
   typedef BfObject Parent;

private:
   Vector<Point> mPoints;  // The points of the barrier --> if only two, first will be start, second end of an old-school segment

   bool mIsPolywall;       // True if this represents a polywall

   // By precomputing and storing, we should ease the rendering cost
   Vector<Point> mRenderFillGeometry;        // Actual geometry used for rendering fill
   const Vector<Point> *mRenderOutlineGeometry;     // Actual geometry used for rendering outline

   F32 mWidth;

public:
   // Constructor
   Barrier(const Vector<Point> &points = Vector<Point>(), F32 width = DEFAULT_BARRIER_WIDTH, bool solid = false);
   virtual ~Barrier();


   static const S32 MIN_BARRIER_WIDTH = 1;         // Clipper doesn't much like 0 width walls
   static const S32 MAX_BARRIER_WIDTH = 2500;      // Geowar has walls 350+ units wide, so going lower will break at least one level

   static const S32 DEFAULT_BARRIER_WIDTH = 50;    // The default width of the barrier in game units

   static Vector<Point> mRenderLineSegments;       // The clipped line segments representing this barrier
   Vector<Point> mBotZoneBufferLineSegments;       // The line segments representing a buffered barrier

   static void constructWalls(Game *game, const Vector<Point> &verts, bool isPolywall, F32 width);

   void renderLayer(S32 layerIndex);                                          // Renders barrier fill barrier-by-barrier
   static void renderEdges(const GameSettings *settings, S32 layerIndex);     // Renders all edges in one pass

   // Returns a sorting key for the object.  Barriers should be drawn first so as to appear behind other objects.
   S32 getRenderSortValue();

   // Returns the collision polygon of this barrier, which is the boundary extruded from the start,end line segment
   const Vector<Point> *getCollisionPoly() const;

   // Collide always returns true for Barrier objects
   bool collide(BfObject *otherObject);

   void getBufferForBotZone(F32 bufferRadius, Vector<Point> &points) const;

   // Clips the current set of render lines against the polygon passed as polyPoints, modifies lineSegmentPoints
   static void clipRenderLinesToPoly(const Vector<DatabaseObject *> &barrierList, Vector<Point> &lineSegmentPoints);

   // Combine multiple barriers into a single complex polygon
   static bool unionBarriers(const Vector<DatabaseObject *> &barriers, Vector<Vector<Point> > &solution);

   static void prepareRenderingGeometry(Game *game);
   static void clearRenderItems();

   // Test access
   FRIEND_TEST(IntegrationTest, LevelReadingAndItemPropagation);
};


////////////////////////////////////////
////////////////////////////////////////

class WallItem;
class PolyWall;

// This is just a small container for the bits of a wall that we transfer from the client to the server
// I feel as if this should be a parent class for both WallItem and PolyWall, but I can't quite seem to
// get it to work...

//struct WallRec
//{
//   Vector<Point> verts;
//   F32 width;
//   bool solid;
//
//   void constructVertices(const GeomObject *source, bool dedupe);    // Helper for constructors
//
//public:
//   WallRec(F32 width, bool solid, const Vector<Point> &verts); // Constructor -- used on Client after receiving wall points
//   explicit WallRec(const WallItem *wallItem);                 // Constructor -- used on Server
//   explicit WallRec(const PolyWall *polyWall);                 // Constructor -- used on Server
//
//   void constructWalls(Game *theGame) const;
//};
// 

////////////////////////////////////////
////////////////////////////////////////

class WallItem : public CentroidObject
{
   typedef CentroidObject Parent;

private:
   S32 mWidth;
   bool mAlreadyAdded;
   void checkIfHasBeenAddedToTheGame(lua_State *L);

public:
   explicit WallItem(lua_State *L = NULL);   // Combined Lua/C++ constructor
   virtual ~WallItem();                      // Destructor
   WallItem *clone() const;

   bool processArguments(S32 argc, const char **argv, Level *level);
   string toLevelCode() const;

   Vector<Point> extendedEndPoints;
   virtual Rect calcExtents();

   virtual void onGeomChanged();
   virtual void onItemDragging();
   virtual void onAddedToGame(Game *game);

   void processEndPoints();  

   void changeWidth(S32 amt);

   void render();
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false);

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();          // Vertices should not be labeled
   const char *getInstructionMsg(S32 attributeCount);
   void fillAttributesVectors(Vector<string> &keys, Vector<string> &values); 

   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();
   F32 getEditorRadius(F32 currentScale);        // Basically, the size of our hit target for vertices

   const Color &getEditorRenderColor() const;    // Unselected wall spine color

   void scale(const Point &center, F32 scale);

   void addToGame(Game *game, GridDatabase *database);

   S32 getWidth() const;
   void setWidth(S32 width);

   void setSelected(bool selected);

   static const S32 VERTEX_SIZE = 5;

   ///// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(WallItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   // Get/set wall's thickness
   S32 lua_getWidth(lua_State *L);
   S32 lua_setWidth(lua_State *L);

   // Override standard methods basically to add checks to keep us from modifying a wall already in the game
   S32 lua_setPos(lua_State *L);
   S32 lua_setGeom(lua_State *L);
};


////////////////////////////////////////
////////////////////////////////////////

class PolyWall : public PolygonObject
{
   typedef PolygonObject Parent;

private:
   bool mAlreadyAdded;
   void checkIfHasBeenAddedToTheGame(lua_State *L);

public:
   explicit PolyWall(lua_State *L = NULL);      // Combined Lua/C++ constructor
   virtual ~PolyWall();                         // Destructor

   PolyWall *clone() const;

   bool processArguments(S32 argc, const char **argv, Level *level);

   void renderDock();

   S32 getRenderSortValue();

   void setSelected(bool selected);

   virtual void onGeomChanged();
   virtual void onItemDragging();
   virtual void onAddedToGame(Game *game);


   /////
   // Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   string toLevelCode() const;

   F32 getEditorRadius(F32 currentScale);

   TNL_DECLARE_CLASS(PolyWall);


   ///// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(PolyWall);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   // Override standard methods basically to add checks to keep us from modifying a wall already in the game
   S32 lua_setPos(lua_State *L);
   S32 lua_setGeom(lua_State *L);
};


////////////////////////////////////////
////////////////////////////////////////

class WallSegment : public DatabaseObject
{
private:
   S32 mOwner;
   bool mSelected;

  void init(GridDatabase *database, S32 owner);
  bool invalid;              // A flag for marking segments in need of processing

   Vector<Point> mEdges;    
   Vector<Point> mCorners;
   Vector<Point> mTriangulatedFillPoints;

public:
   WallSegment(GridDatabase *gridDatabase, const Point &start, const Point &end, F32 width, S32 owner = -1);    // Normal wall segment
   WallSegment(GridDatabase *gridDatabase, const Vector<Point> &points, S32 owner = -1);                        // PolyWall 
   virtual ~WallSegment();

   S32 getOwner();
   void invalidate();

   bool isSelected();
   void setSelected(bool selected);

   void resetEdges();         // Compute basic edges from corner points
   void computeBoundingBox(); // Computes bounding box based on the corners, updates database
   
   void renderFill(const Point &offset, const Color &color);

   const Vector<Point> *getCorners();
   const Vector<Point> *getEdges();
   const Vector<Point> *getTriangulatedFillPoints();

   ////////////////////
   // DatabaseObject methods

   // Note that the poly returned here is different than what you might expect -- it is composed of the edges,
   // not the corners, and is thus in A-B, C-D, E-F format rather than the more typical A-B-C-D format returned
   // by getCollisionPoly() elsewhere in the game.  Therefore, it needs to be handled differently.
   const Vector<Point> *getCollisionPoly() const;
   bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) const;
};


////////////////////////////////////////
////////////////////////////////////////


class WallEdge : public DatabaseObject
{
private:
   Point mStart, mEnd;
   Vector<Point> mPoints;

public:
   WallEdge(const Point &start, const Point &end);
   virtual ~WallEdge();

   Point *getStart();
   Point *getEnd();

   // Note that the poly returned here is different than what you might expect -- it is composed of the edges,
   // not the corners, and is thus in A-B, C-D, E-F format rather than the more typical A-B-C-D format returned
   // by getCollisionPoly() elsewhere in the game.  Therefore, it needs to be handled differently.
   const Vector<Point> *getCollisionPoly() const;
   bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) const;
};


};

#endif

