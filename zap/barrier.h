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

#ifndef _BARRIER_H_
#define _BARRIER_H_

#include "gameObject.h"
#include "textItem.h"      // for LineItem def  TODO: get rid of this

#include "Point.h"
#include "tnlVector.h"
#include "tnlNetObject.h"

namespace Zap
{

/// The Barrier class represents rectangular barriers that player controlled
/// ships cannot pass through... i.e. walls  Barrier objects, once created, never
/// change state, simplifying the pack/unpack update methods.  Barriers are
/// constructed as an expanded line segment.
class Barrier : public GameObject
{
   typedef GameObject Parent;

public:
   Vector<Point> mPoints; ///< The points of the barrier --> if only two, first will be start, second end of an old-school segment

   bool mSolid;
   // By precomputing and storing, we should ease the rendering cost
   Vector<Point> mRenderFillGeometry; ///< Actual geometry used for rendering fill.
   Vector<Point> mRenderOutlineGeometry; ///< Actual geometry used for rendering outline.

   F32 mWidth;

   static const S32 MIN_BARRIER_WIDTH = 1;         // Clipper doesn't much like 0 width walls
   static const S32 MAX_BARRIER_WIDTH = 2500;      // Geowar has walls at least 350 units wide, so going lower will break at least one level

   static const S32 DEFAULT_BARRIER_WIDTH = 50; ///< The default width of the barrier in game units

   static Vector<Point> mRenderLineSegments;    ///< The clipped line segments representing this barrier.
   Vector<Point> mBotZoneBufferLineSegments;    ///< The line segments representing a buffered barrier.

   /// Barrier constructor
   Barrier(const Vector<Point> &points = Vector<Point>(), F32 width = DEFAULT_BARRIER_WIDTH, bool solid = false);

   /// Adds the server object to the net interface's scope always list
   void onAddedToGame(Game *theGame);

   /// Renders barrier fill
   void render(S32 layer);                // Renders barrier fill barrier-by-barrier
   static void renderEdges(S32 layer);    // Renders all edges in one pass

   /// Returns a sorting key for the object.  Barriers should be drawn first so as to appear behind other objects.
   S32 getRenderSortValue() { return 0; }

   /// returns the collision polygon of this barrier, which is the boundary extruded from the start,end line segment.
   bool getCollisionPoly(Vector<Point> &polyPoints) const;

   /// collide always returns true for Barrier objects.
   bool collide(GameObject *otherObject) { return true; }

   // Takes a list of vertices and converts them into a list of lines representing the edges of an object
   static void resetEdges(const Vector<Point> &corners, Vector<Point> &edges);

   // Simply takes a segment and "puffs it out" to a rectangle of a specified width, filling cornerPoints.  Does not modify endpoints.
   static void expandCenterlineToOutline(const Point &start, const Point &end, F32 width, Vector<Point> &cornerPoints);

   // Takes a segment and "puffs it out" to a polygon for bot zone generation.
   // This polygon is the width of the barrier plus the ship's collision radius added to the outside
   Vector<Point> getBufferForBotZone();

   // Combines multiple barriers into a single complex polygon
   static bool unionBarriers(const Vector<DatabaseObject *> &barriers, Vector<Vector<Point> > &solution);

   /// Clips the current set of render lines against the polygon passed as polyPoints, modifies lineSegmentPoints.
   static void clipRenderLinesToPoly(const Vector<DatabaseObject *> &barrierList, Vector<Point> &lineSegmentPoints);

   static void constructBarrierEndPoints(const Vector<Point> *vec, F32 width, Vector<Point> &barrierEnds);

   // Clean up edge geometry and get barriers ready for proper rendering
   static void prepareRenderingGeometry(Game *game);


   TNL_DECLARE_CLASS(Barrier);
};


////////////////////////////////////////
////////////////////////////////////////

// TODO: Don't need this to be a GameObject  -- perhaps merge with barrier above?
class WallItem : public LineItem
{
public:
   WallItem();    // Constructor
   WallItem *clone() const;
   
   virtual void onGeomChanged();
   void processEndPoints();      

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString() { return "Walls define the general form of your level."; }  
   const char *getPrettyNamePlural() { return "Walls"; }
   const char *getOnDockName() { return "Wall"; }
   const char *getOnScreenName() { return "Wall"; }
   bool hasTeam() { return false; }
   bool canBeHostile() { return false; }
   bool canBeNeutral() { return false; }

   const Color *getEditorRenderColor() const { return &Colors::gray50; }      // Unselected wall spine color

   void scale(const Point &center, F32 scale);

   string toString(F32 gridSize) const;

   void setWidth(S32 width);
};


////////////////////////////////////////
////////////////////////////////////////

class PolyWall : public EditorPolygon     // Don't need GameObject component of this...
{
   typedef EditorPolygon Parent;

private:
   //typedef EditorObject Parent;
   //void computeExtent();

public:
   PolyWall();      // Constructor
   PolyWall *clone() const;
  
   bool processArguments(S32 argc, const char **argv, Game *game);

   void processEndPoints();

   void render();
   void renderFill();
   void renderDock();
   void renderEditor(F32 currentScale);

   S32 getRenderSortValue() { return -1; }

   virtual void onGeomChanged();
   virtual void onItemDragging() { /* Do nothing */ }

   //bool getCollisionPoly(Vector<Point> &polyPoints);

   /////
   // Editor methods
   const char *getEditorHelpString() { return "Polygonal wall item let you be creative with your wall design."; }
   const char *getPrettyNamePlural() { return "PolyWalls"; }
   const char *getOnDockName() { return "PolyWall"; }
   const char *getOnScreenName() { return "PolyWall"; }
   string toString(F32 gridSize) const;

   /////
   // Lua interface  ==>  don't need these!!

   PolyWall(lua_State *L) { /* Do nothing */ };    //  Lua constructor
   GameObject *getGameObject() { return this; }          // Return the underlying GameObject

   static const char className[];                        // Class name as it appears to Lua scripts
   static Lunar<PolyWall>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, PolyWallTypeNumber); }
   TNL_DECLARE_CLASS(PolyWall);

private:
   void push(lua_State *L) { Lunar<PolyWall>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

class WallSegment : public DatabaseObject
{
private:
   GridDatabase *mDatabase;    
   GridDatabase *getGridDatabase() { return mDatabase; }      
   S32 mOwner;

  void init(S32 owner);
  bool invalid;              // A flag for marking segments in need of processing

public:
   WallSegment(GridDatabase *gridDatabase, const Point &start, const Point &end, F32 width, S32 owner = -1);    // Normal wall segment
   WallSegment(GridDatabase *gridDatabase, const Vector<Point> &points, S32 owner = -1);                        // PolyWall 
   virtual ~WallSegment();

   // TODO: Make these private
   Vector<Point> edges;    
   Vector<Point> corners;
   Vector<Point> triangulatedFillPoints;


   S32 getOwner() { return mOwner; }
   void invalidate() { invalid = true; }

   void resetEdges();         // Compute basic edges from corner points
   void computeBoundingBox(); // Computes bounding box based on the corners, updates database
   
   void renderFill(bool renderLight, bool showingReferenceShip);

   ////////////////////
   //  DatabaseObject methods

   // Note that the poly returned here is different than what you might expect -- it is composed of the edges,
   // not the corners, and is thus in A-B, C-D, E-F format rather than the more typical A-B-C-D format returned
   // by getCollisionPoly() elsewhere in the game.  Therefore, it needs to be handled differently.
   bool getCollisionPoly(Vector<Point> &polyPoints) const { polyPoints = edges; return true; }  
   bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) const { return false; }
   bool getCollisionRect(U32 stateIndex, Rect &rect) const { return false; }
};


////////////////////////////////////////
////////////////////////////////////////

// Width of line representing centerline of barriers
#define WALL_SPINE_WIDTH gLineWidth3


class WallEdge : public DatabaseObject
{
private:
   Point mStart, mEnd;

public:
   WallEdge(const Point &start, const Point &end, GridDatabase *database);
   virtual ~WallEdge();

   Point *getStart() { return &mStart; }
   Point *getEnd() { return &mEnd; }

   // Note that the poly returned here is different than what you might expect -- it is composed of the edges,
   // not the corners, and is thus in A-B, C-D, E-F format rather than the more typical A-B-C-D format returned
   // by getCollisionPoly() elsewhere in the game.  Therefore, it needs to be handled differently.
   bool getCollisionPoly(Vector<Point> &polyPoints) const { polyPoints.resize(2); polyPoints[0] = mStart; polyPoints[1] = mEnd; return true; }  
   bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) const { return false; }
   bool getCollisionRect(U32 stateIndex, Rect &rect) const { return false; }
};


////////////////////////////////////////
////////////////////////////////////////

class EditorObject;

class WallSegmentManager
{
private:
   GridDatabase *mWallSegmentDatabase;
   GridDatabase *mWallEdgeDatabase;

public:
   WallSegmentManager();   // Constructor
   ~WallSegmentManager();  // Destructor

   GridDatabase *getGridDatabase() { return mWallSegmentDatabase; } 

   GridDatabase *getWallSegmentDatabase() { return mWallSegmentDatabase; } 
   GridDatabase *getWallEdgeDatabase() { return mWallEdgeDatabase; }

   Vector<WallSegment *> mWallSegments;      
   Vector<WallEdge *> mWallEdges;               // For mounting forcefields/turrets
   Vector<Point> mWallEdgePoints;               // For rendering

   void buildAllWallSegmentEdgesAndPoints(GridDatabase *gameDatabase);
   void deleteSegments(S32 owner);              // Delete all segments owned by specified WorldItem
   void deleteAllSegments();

   // Recalucate edge geometry for all walls when item has changed
   void computeWallSegmentIntersections(GridDatabase *gameDatabase, EditorObject *item); 

   // Takes a wall, finds all intersecting segments, and marks them invalid
   void invalidateIntersectingSegments(GridDatabase *gameDatabase, EditorObject *item);

   void buildWallSegmentEdgesAndPoints(GridDatabase *gameDatabase, DatabaseObject *object);
   void buildWallSegmentEdgesAndPoints(GridDatabase *gameDatabase, DatabaseObject *object, const Vector<DatabaseObject *> &engrObjects);
   void recomputeAllWallGeometry(GridDatabase *gameDatabase);

   // Populate wallEdges
   void clipAllWallEdges(const Vector<WallSegment *> &wallSegments, Vector<Point> &wallEdges);
 
   ////////////////
   // Render functions
   void renderWalls(bool isBeingDragged, bool showingReferenceShip, bool showSnapVertices, F32 alpha);
};


};

#endif

