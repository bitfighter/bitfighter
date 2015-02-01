//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _WALLITEM_H_
#define _WALLITEM_H_

#include "barrier.h"

namespace Zap
{

class PolyWall;

class WallItem : public CentroidObject, public BarrierX
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
   virtual Rect calcExtents() const;

   virtual void onGeomChanged();
   virtual void onItemDragging();
   virtual void onAddedToGame(Game *game);

   void computeExtendedEndPoints();  

   void changeWidth(S32 amt);

   bool overlapsPoint(const Point &point) const;
   bool checkForCollision(const Point &rayStart, const Point &rayEnd, bool format, U32 stateIndex,
                          F32 &collisionTime, Point &surfaceNormal) const;

   void render() const;
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false) const;

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString() const;
   const char *getPrettyNamePlural() const;
   const char *getOnDockName() const;
   const char *getOnScreenName() const;            // Vertices should not be labeled
   const char *getInstructionMsg(S32 attributeCount) const;
   void fillAttributesVectors(Vector<string> &keys, Vector<string> &values); 

   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();
   void setTeam(S32 team);
   F32 getEditorRadius(F32 currentScale) const;    // Basically, the size of our hit target for vertices

   const Color &getEditorRenderColor() const;      // Unselected wall spine color

   void scale(const Point &center, F32 scale);

   void addToGame(Game *game, GridDatabase *database);

   S32 getWidth() const;
   void setWidth(S32 width);

   //const Vector<Point> *getCollisionPoly() const;

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

class WallSegment : public DatabaseObject
{
private:
  bool invalid;              // A flag for marking segments in need of processing

   Vector<Point> mEdges;    
   Vector<Point> mCorners;
   Vector<Point> mTriangulatedFillPoints;

   BarrierX *mOwner;

   void init(BarrierX *owner);

public:
   WallSegment(const Point &start, const Point &end, F32 width, BarrierX *owner);    // Normal wall segment
   WallSegment(const Vector<Point> &points, BarrierX *owner);                        // PolyWall 
   WallSegment(const WallSegment *source, BarrierX *owner);                          // Copy constructor
   virtual ~WallSegment();

   void invalidate();

   BarrierX *getOwner() const;

   void resetEdges();         // Compute basic edges from corner points
   void computeBoundingBox(); // Computes bounding box based on the corners, updates database
   
   void renderFill(const Point &offset, const Color &color, bool isSelected) const;

   const Vector<Point> *getCorners() const;
   const Vector<Point> *getEdges() const;
   const Vector<Point> *getTriangulatedFillPoints() const;

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

