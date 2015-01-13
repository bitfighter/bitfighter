//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _POLYWALL_H_
#define _POLYWALL_H_

#include "polygon.h"       // For PolygonObject def
#include "barrier.h"       // for BarrierX def


namespace Zap
{

class PolyWall : public PolygonObject, public BarrierX
{
   typedef PolygonObject Parent;

private:
   bool mAlreadyAdded;
   void checkIfHasBeenAddedToTheGame(lua_State *L);

public:
   explicit PolyWall(lua_State *L = NULL);            // Combined Lua/C++ constructor
   explicit PolyWall(const Vector<Point> &verts);     // Client-side constructor
   virtual ~PolyWall();                               // Destructor

   void initialize();

   PolyWall *clone() const;

   bool processArguments(S32 argc, const char **argv, Level *level);

   void render() const;
   void renderDock(const Color &color) const;

   S32 getRenderSortValue();

   virtual void onGeomChanged();
   virtual void onItemDragging();
   virtual void onAddedToGame(Game *game);

   /////
   // Editor methods
   const char *getEditorHelpString() const;
   const char *getPrettyNamePlural() const;
   const char *getOnDockName() const;
   const char *getOnScreenName() const;
   string toLevelCode() const;

   F32 getEditorRadius(F32 currentScale) const;

   // Returns the collision polygon of this barrier, which is the boundary extruded from the start,end line segment
   const Vector<Point> *getCollisionPoly() const;

   // Collide always returns true for Barrier objects
   bool collide(BfObject *otherObject);

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


};

#endif

