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
#include "SDL/SDL_opengl.h"
#endif

namespace Zap
{

bool Item::mInitial;
static U32 sItemId = 1;

// Constructor
Item::Item(const Point &pos, F32 radius)
{
   setActualPos(pos);
   mRadius = radius;

   mItemId = sItemId;
   sItemId++;
}


Point Item::getActualPos() const
{
   return getVert(0);
}

void Item::setActualPos(const Point &p)
{
   setVert(p, 0);
   setExtent(Rect(p, mRadius));
}

bool Item::getCollisionCircle(U32 stateIndex, Point &point, F32 &radius) const
{
   point = getVert(0);
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

   // TODO? We need to reconcile these two ways of storing an item's location
   setActualPos(pos);      // Needed by game
   setVert(pos, 0);        // Needed by editor... But setActualPos(pos) already does setVert(pos, 0)

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
      ((GameConnection *) connection)->writeCompressedPoint(getActualPos(), stream);
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

      setActualPos(pos);      // Also sets object extent
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
   renderItem(getActualPos());
}


void Item::renderEditor(F32 currentScale)
{
   renderItem(getVert(0));                    
}


F32 Item::getEditorRadius(F32 currentScale)
{
   return (getRadius() + 2) * currentScale;
}

Rect Item::calcExtents()
{
   return Rect(getVert(0), mRadius);
}

// LuaItem interface
S32 Item::getLoc(lua_State *L)
{
   return LuaObject::returnPoint(L, getActualPos());
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
   return TEAM_NEUTRAL + 1;
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


GameObject *Item::getGameObject()
{
   return this;
}


};


