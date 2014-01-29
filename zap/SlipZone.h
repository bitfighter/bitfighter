//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SLIP_ZONE_H_
#define _SLIP_ZONE_H_

#include "Zone.h"


namespace Zap
{

class SlipZone : public GameZone
{
   typedef GameZone Parent;

public:
	F32 slipAmount;   // 0.0 to 1.0 , lower = more slippy

	explicit SlipZone(lua_State *L = NULL);   // Combined Lua / C++ constructor
   virtual ~SlipZone();

   SlipZone *clone() const;

   bool processArguments(S32 argc, const char **argv, Game *game);

   void render();
   S32 getRenderSortValue();

   void onAddedToGame(Game *theGame);
   const Vector<Point> *getCollisionPoly() const;
   bool collide(BfObject *hitObject);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   /////
   // Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   string toLevelCode() const;

   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);


   TNL_DECLARE_CLASS(SlipZone);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(SlipZone);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};

};
#endif
