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
#include "flagItem.h"
#include "game.h"
#include "gameConnection.h"

#include "gameObjectRender.h"
#include "stringUtils.h"
#include "gameObjectRender.h"

#ifndef ZAP_DEDICATED
#include "UI.h"
#include "SDL_opengl.h"
#endif

namespace Zap
{

bool Item::mInitial;
static U32 sItemId = 1;

// Constructor
Item::Item(const Point &pos, F32 radius)
{
   mRadius = radius;
   setPos(pos);

   mItemId = sItemId;
   sItemId++;

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


U32 Item::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   //U32 retMask = Parent::packUpdate(connection, updateMask, stream);  // Goes to empty function NetObject::packUpdate

   if(stream->writeFlag(updateMask & InitialMask))
   {
      // Send id in inital packet
      stream->writeRangedU32(mItemId, 0, U16_MAX);
      ((GameConnection *) connection)->writeCompressedPoint(getPos(), stream);
   }

   return 0; //retMask;
}


void Item::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   //Parent::unpackUpdate(connection, stream);  // Goes to empty function NetObject::unpackUpdate

   mInitial = stream->readFlag();
   if(mInitial)     // InitialMask
   {
      mItemId = stream->readRangedU32(0, U16_MAX);

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


void Item::renderEditor(F32 currentScale)
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


// LuaItem interface
S32 Item::getLoc(lua_State *L)
{
   return LuaObject::returnPoint(L, getPos());
}


S32 Item::getRad(lua_State *L)
{
   return LuaObject::returnFloat(L, getRadius());
}


S32 Item::getVel(lua_State *L)
{
   return LuaObject::returnPoint(L, Point(0,0));
}


S32 Item::getTeamIndx(lua_State *L)
{
   return returnInt(L, getTeam() + 1);
}


S32 Item::isInCaptureZone(lua_State *L)
{
   return returnBool(L, false);
}


S32 Item::isOnShip(lua_State *L)
{
   return returnBool(L, false);
}


S32 Item::getCaptureZone(lua_State *L)
{
   return returnNil(L);
}


S32 Item::getShip(lua_State *L)
{
   return returnNil(L);
}


// ==> This one is a good candidate for moving directly into LuaW's index table rather than having a C++ method to support it
static S32 doGetClassId(lua_State *L) 
{ 
   MoveItem *w = luaW_check<MoveItem>(L, 1); 
   if(w) 
      return w->getObjectTypeNumber(); 
      
   return LuaObject::returnNil(L); 
}


// Standard methods available to all Items
const luaL_reg Item::luaMethods[] =
{
   { "getClassID",      doGetClassId                          },
   { "getLoc",          luaW_doMethod<Item, &getLoc>          },
   { "getRad",          luaW_doMethod<Item, &getRad>          },
   { "getVel",          luaW_doMethod<Item, &getVel>          },
   { "getTeamIndx",     luaW_doMethod<Item, &getTeamIndx>     },
   { "isInCaptureZone", luaW_doMethod<Item, &isInCaptureZone> },

   { NULL, NULL }
};


const char *Item::luaClassName = "Item";

REGISTER_CLASS_XXX(Item);


// For getting the underlying object when all we have is a Lua pointer to it
GameObject *Item::getGameObject()
{
   return this;
}


};


