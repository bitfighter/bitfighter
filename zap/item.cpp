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

#include "item.h"
#include "ship.h"
#include "goalZone.h"
#include "gameType.h"
//#include "flagItem.h"
#include "game.h"
#include "gameConnection.h"

#include "gameObjectRender.h"
#include "stringUtils.h"
#include "gameObjectRender.h"

#ifndef ZAP_DEDICATED
#include "UI.h"
#include "OpenglUtils.h"
#endif

namespace Zap
{

bool Item::mInitial;

// Constructor
Item::Item(F32 radius) : Parent()
{
   mRadius = radius;
   setPos(Point(0,0));

   static U16 itemId = 1;
   mItemId = itemId++;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
Item::~Item()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


void Item::setPos(const Point &p)
{
   Parent::setPos(p);
   setExtent(Rect(p, mRadius));
}


bool Item::getCollisionCircle(U32 stateIndex, Point &point, F32 &radius) const
{
   point = getPos();
   radius = mRadius;
   return true;
}


// Server only  --> Assumes first two params are x and y location; subclasses may read additional params
bool Item::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 2)
      return false;

   Point pos;
   pos.read(argv);
   pos *= game->getGridSize();

   setPos(pos);      // Needed by game

   return true;
}


string Item::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + geomToString(gridSize);
}


U16 Item::getItemId()
{
   return mItemId;
}


// Client only
void Item::setItemId(U16 id)
{
   mItemId = id;
}


U32 Item::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   //U32 retMask = Parent::packUpdate(connection, updateMask, stream);  // Goes to empty function NetObject::packUpdate

   if(stream->writeFlag(updateMask & InitialMask))
      stream->writeRangedU32(mItemId, 0, U16_MAX);    // Send id in inital packet

   if(stream->writeFlag(updateMask & (InitialMask | GeomMask)))
      ((GameConnection *) connection)->writeCompressedPoint(getPos(), stream);

   return 0; //retMask;
}


void Item::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   //Parent::unpackUpdate(connection, stream);  // Goes to empty function NetObject::unpackUpdate

   mInitial = stream->readFlag();      // InitialMask
   if(mInitial)     
      mItemId = stream->readRangedU32(0, U16_MAX);

   if(stream->readFlag())              // GeomMask
   {
      Point pos;
      ((GameConnection *) connection)->readCompressedPoint(pos, stream);

      setPos(pos);      // Also sets object extent
   }
}


F32 Item::getRadius()
{
   return mRadius;
}


void Item::setRadius(F32 radius)
{
   mRadius = radius;
}


// Provide generic item rendering; will be overridden
void Item::renderItem(const Point &pos)
{
#ifndef ZAP_DEDICATED
   glColor(Colors::cyan);
   drawSquare(pos, 10, true);
#endif
}

void Item::render()
{
   renderItem(getPos());
}


void Item::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   renderItem(getPos());                    
}


F32 Item::getEditorRadius(F32 currentScale)
{
   return (getRadius() + 2) * currentScale;
}


Rect Item::calcExtents()
{
   return Rect(getPos(), mRadius);
}


/////
// Lua interface
/**
 *   @luaclass Item
 *   @brief    Parent class for most common game items
 */

// Standard methods available to all Items:
//               Fn name           Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getRad,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isOnShip,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getShip,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isInCaptureZone,  ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getCaptureZone,   ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(Item, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(Item, LUA_METHODS);

#undef LUA_METHODS


const char *Item::luaClassName = "Item";
REGISTER_LUA_SUBCLASS(Item, BfObject);


/**
 * @luafunc Item::getRad()
 * @brief   Returns radius of the %item.
 * @return  \e num representing the radius of the %item.
 */
S32 Item::getRad(lua_State *L) { return returnFloat(L, getRadius()); }

/**
 * @luafunc Item::isOnShip()
 * @brief   Returns true if item is mounted on a ship.
 * @descr   Currently, only \link FlagItem FlagItems\endlink and \link ResourceItem ResourceItems\endlink can be mounted on ships.
 * @return  \e bool True if the item is mounted on a ship, false otherwise.
 */
S32 Item::isOnShip (lua_State *L) { return returnBool(L, false); }

/**
 * @luafunc Item::getCaptureZone()
 * @brief   Returns capture zone holding the item.
 * @descr   Many games do not feature capture zones.  For those games, this function will always return nil.
 *          Currently only \link FlagItem FlagItems\endlink can be captured.
 * @return  \e Zone where the item has been captured.  Returns nil if the item is not in a capture zone.
 */
S32 Item::getCaptureZone (lua_State *L) { return returnNil(L); }

/**
 * @luafunc Item::getShip()
 * @brief   Returns the ship where the item is mounted.
 * @descr   Most objects cannot be mounted.  For those, this function will always return nil.
 * @return  \e Ship where the item is mounted.  Returns nil if the item is not mounted.
 */
S32 Item::getShip (lua_State *L) { return returnNil(L); }

/**
 * @luafunc Item::isInCaptureZone()
 * @brief  Returns whether or not the item is in a capture zone.
 * @return \e bool True if item is in a capture zone, false otherwise.
 */
S32 Item::isInCaptureZone(lua_State *L) { return returnBool(L, false); }


};


