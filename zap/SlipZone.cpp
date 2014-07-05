//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#include "SlipZone.h"

#include "game.h"
#include "Level.h"
#include "gameObjectRender.h"
#include "LuaBase.h"

#include "stringUtils.h"

namespace Zap
{

using namespace LuaArgs;


// Constructor
SlipZone::SlipZone(lua_State *L)
{
   setTeam(0);
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = SlipZoneTypeNumber;
   slipAmount = 0.1f;

   if(L)   // Coming from Lua -- grab params from L
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, END }}, 1 };
      S32 profile = checkArgList(L, constructorArgList, "SlipZone", "constructor");

      if(profile == 1)         // Geom
         setGeom(L,  1);
   }

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}

// Destructor
SlipZone::~SlipZone()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


SlipZone *SlipZone::clone() const
{
   return new SlipZone(*this);
}


void SlipZone::render() const
{
   renderSlipZone(getOutline(), getFill(), getCentroid());
}


void SlipZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices) const
{
   render();
   PolygonObject::renderEditor(currentScale, snappingToWallCornersEnabled);
}


S32 SlipZone::getRenderSortValue()
{
   return -1;
}


bool SlipZone::processArguments(S32 argc2, const char **argv2, Level *level)
{
   // Need to handle or ignore arguments that starts with letters,
   // so a possible future version can add parameters without compatibility problem.
   S32 argc = 0;
   const char *argv[Geometry::MAX_POLY_POINTS * 2 + 1];
   for(S32 i = 0; i < argc2; i++)  // the idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char c = argv2[i][0];
      //switch(c)
      //{
      //case 'A': Something = atof(&argv2[i][1]); break;  // using second char to handle number
      //}
      if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
      {
			if(argc < Geometry::MAX_POLY_POINTS * 2 + 1)
         {  
            argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   if(argc < 6)
      return false;

   if(argc & 1)   // Odd number of arg count (7, 9, 11) to allow optional slipAmount arg
   {
      slipAmount = (F32)atof(argv[0]);
      readGeom(argc, argv, 1, level->getLegacyGridSize());
   }
   else           // Even number of arg count (6, 8, 10)
      readGeom(argc, argv, 0, level->getLegacyGridSize());

   updateExtentInDatabase();

   return true;
}


const char *SlipZone::getEditorHelpString() const
{
   return "Areas of higher than normal inertia.";
}


const char *SlipZone::getPrettyNamePlural() const
{
   return "Inertia zones";
}


const char *SlipZone::getOnDockName() const
{
   return "Inertia";
}


const char *SlipZone::getOnScreenName() const
{
   return "Inertia";
}


string SlipZone::toLevelCode() const
{
   return string(appendId(getClassName())) + " " + ftos(slipAmount, 3) + " " + geomToLevelCode();
}


void SlipZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();
}


const Vector<Point> *SlipZone::getCollisionPoly() const
{
   return getOutline();
}


bool SlipZone::collide(BfObject *hitObject) 
{
   if(!isGhost() && isShipType(hitObject->getObjectTypeNumber()))
   {
      //logprintf("IN A SLIP ZONE!!");
   }
   return false;
}


U32 SlipZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   Parent::packUpdate(connection, updateMask, stream);
   stream->write(slipAmount);
   return 0;
}


void SlipZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);
   stream->read(&slipAmount);
}


TNL_IMPLEMENT_NETOBJECT(SlipZone);


/////
// Lua interface
/**
 * @luaclass SlipZone
 *
 * @brief Experimental zone that increases a ship's inertia
 *
 * @descr SlipZone is weird, don't use it
 *
 * @note This is experimental and may be removed from the game at any time
 */
//               Fn name         Param profiles     Profile count
#define LUA_METHODS(CLASS, METHOD) \
/*
   METHOD(CLASS, getSlipFactor,       ARRAYDEF({{ END }}), 1 ) \
*/

GENERATE_LUA_METHODS_TABLE(SlipZone, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(SlipZone, LUA_METHODS);

#undef LUA_METHODS


const char *SlipZone::luaClassName = "SlipZone";
REGISTER_LUA_SUBCLASS(SlipZone, Zone);


};
