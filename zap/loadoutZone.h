//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LOADOUTZONE_H_
#define _LOADOUTZONE_H_

#include "Zone.h"

namespace Zap
{

class LoadoutZone : public GameZone
{
   typedef GameZone Parent;

public:
   explicit LoadoutZone(lua_State *L = NULL);   // Combined Lua / C++ constructor
   virtual ~LoadoutZone();             // Destructor

   LoadoutZone *clone() const;

   void render();
   bool processArguments(S32 argc, const char **argv, Level *level);
   void onAddedToGame(Game *theGame);

   const Vector<Point> *getCollisionPoly() const;     // More precise boundary for precise collision detection
   bool collide(BfObject *hitObject);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   /////
   // Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   bool hasTeam();      
   bool canBeHostile(); 
   bool canBeNeutral(); 

   string toLevelCode() const;

   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false);
   void renderDock();

   TNL_DECLARE_CLASS(LoadoutZone);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(LoadoutZone);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};

};


#endif
