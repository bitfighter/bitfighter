//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _ZONE_H_
#define _ZONE_H_

#include "polygon.h"          // Parent class


namespace Zap
{

class Zone : public PolygonObject
{
   typedef PolygonObject Parent;

public:
   explicit Zone(lua_State *L = NULL);    // Combined Lua / C++ constructor
   virtual ~Zone();              // Destructor
   Zone *clone() const;

   virtual void render();
   S32 getRenderSortValue();
   virtual bool processArguments(S32 argc, const char **argv, Game *game);

   virtual const Vector<Point> *getCollisionPoly() const;     // More precise boundary for precise collision detection
   virtual bool collide(BfObject *hitObject);

   /////
   // Editor methods
   virtual const char *getEditorHelpString();
   virtual const char *getPrettyNamePlural();
   virtual const char *getOnDockName();
   virtual const char *getOnScreenName();

   bool hasTeam();      
   bool canBeHostile(); 
   bool canBeNeutral(); 

   virtual string toLevelCode() const;

   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   virtual void renderDock();

   TNL_DECLARE_CLASS(Zone);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(Zone);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   int lua_containsPoint(lua_State *L);
};


////////////////////////////////////////
////////////////////////////////////////

// Extends above with some methods related to client/server interaction; Zone itself is server-only
class GameZone : public Zone
{
   typedef Zone Parent;

public:
   GameZone();
   virtual ~GameZone();

   virtual U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);
};


};


#endif
