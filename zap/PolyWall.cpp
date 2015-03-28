//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "PolyWall.h"

#include "GameObjectRender.h"
#include "GameSettings.h"
#include "Level.h"
#include "LuaBase.h"
#include "WallItem.h"

#include "Colors.h"
#include "GeomUtils.h"

#include "tnlLog.h"


using namespace TNL;

namespace Zap
{

using namespace LuaArgs;

TNL_IMPLEMENT_NETOBJECT(PolyWall);

/**
 * @luafunc PolyWall::PolyWall()
 * @luafunc PolyWall::PolyWall(polyGeom)
 */
// Combined Lua/C++ constructor
PolyWall::PolyWall(lua_State *L)
{
   initialize();

   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, END }}, 2 };

      S32 profile = checkArgList(L, constructorArgList, "PolyWall", "constructor");

      if(profile == 1)
         lua_setGeom(L);
   }
}


PolyWall::PolyWall(const Vector<Point> &verts)
{
   TNLAssert(verts.size() >= 3, "Not enough vertices for a polywall!");

   initialize();

   if(isWoundClockwise(verts))
   {
      // All walls must be CCW to clip correctly -- we need to reverse them before setting our geometry
      S32 vsize = verts.size();

      Vector<Point> ccwVerts(vsize);      // Reserve some space to avoid resizing cost while we're adding our points
      for(S32 i = 0; i < vsize; i++)
         ccwVerts.push_back(verts[vsize - i - 1]);

      GeomObject::setGeom(ccwVerts);
   }
   else
   {
      // Verts are already wound the right way
      GeomObject::setGeom(verts);
   }

   updateExtentInDatabase();
}


PolyWall::~PolyWall()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


void PolyWall::initialize()
{
   mObjectTypeNumber = PolyWallTypeNumber;
   mAlreadyAdded = false;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


PolyWall *PolyWall::clone() const
{
   PolyWall *polyWall = new PolyWall(*this);

   polyWall->cloneSegments(this);

   return polyWall;
}


S32 PolyWall::getRenderSortValue()
{
   return -1;
}


void PolyWall::renderDock(const Color &color) const
{
   static const Color wallOutlineColor(GameSettings::get()->getWallOutlineColor());

   GameObjectRender::renderPolygonFill(getFill(), Colors::EDITOR_WALL_FILL_COLOR);
   GameObjectRender::renderPolygonOutline(getOutline(), wallOutlineColor);
}


void PolyWall::render() const
{
   GameObjectRender::renderWallFill(getFill(), GameSettings::get()->getWallFillColor(), true);
   //renderZone(color, outline, fill);
   //renderLoadoutZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}



bool PolyWall::processArguments(S32 argc, const char **argv, Level *level)
{
   if(argc < 7)            // Need "Polywall" keyword, and at least 3 points
      return false;

   S32 offset = 0;

   if(stricmp(argv[0], "BarrierMakerS") == 0)
      offset = 1;

   readGeom(argc, argv, offset, level->getLegacyGridSize());
   updateExtentInDatabase();

   if(getVertCount() < 3)     // Need at least 3 vertices for a polywall!
      return false;

   return true;
}


string PolyWall::toLevelCode() const
{
   return string(appendId(getClassName())) + " " + geomToLevelCode();
}


const char *PolyWall::getOnScreenName()     const { return "PolyWall";  }
const char *PolyWall::getOnDockName()       const { return "PolyWall";  }
const char *PolyWall::getPrettyNamePlural() const { return "PolyWalls"; }
const char *PolyWall::getEditorHelpString() const { return "Polygonal wall item lets you be creative with your wall design."; }


void PolyWall::onGeomChanged()
{
   Vector<WallSegment *> segments;
   segments.push_back(new WallSegment(*getOutline(), this));

   setSegments(segments);

   Parent::onGeomChanged();
}


void PolyWall::onItemDragging()
{
   // Do nothing -- this is here to override PolygonObject::onItemDragging(), 
   //               onGeomChanged() should only be called after move is complete
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


const Vector<Point> *PolyWall::getCollisionPoly() const
{
   return getOutline();
}


// Size of object in editor 
F32 PolyWall::getEditorRadius(F32 currentScale) const
{
   return (F32)EditorObject::VERTEX_SIZE;   // Keep vertex hit targets the same regardless of scale
}


bool PolyWall::collide(BfObject *otherObject)
{
   return true;
}


/////
// Lua interface

/**
 * @luaclass PolyWall
 * 
 * @brief Polygonal wall item.
 * 
 * @descr A PolyWall is a wall consisting of a filled polygonal shape. 
 * 
 * @geom PolyWall geometry is a typical polygon.
 */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_FUNARGS_TABLE(PolyWall, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(PolyWall, LUA_METHODS);

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
         THROW_LUA_EXCEPTION(L, msg);
      }
   }
}


// Lua method overrides.  Because walls are... special.

S32 PolyWall::lua_setPos(lua_State *L)
{
   checkIfHasBeenAddedToTheGame(L);
   return Parent::lua_setPos(L);
}


S32 PolyWall::lua_setGeom(lua_State *L)
{
   checkIfHasBeenAddedToTheGame(L);
   return Parent::lua_setGeom(L);
}



};
