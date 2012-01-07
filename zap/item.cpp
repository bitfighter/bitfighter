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

   // TODO: We need to reconcile these two ways of storing an item's location
   setActualPos(pos);      // Needed by game
   setVert(pos, 0);        // Needed by editor

   return true;
}


string Item::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + geomToString(gridSize);
}


U32 Item::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & InitialMask))
   {
      // Send id in inital packet
      stream->writeRangedU32(mItemId, 0, U16_MAX);
      ((GameConnection *) connection)->writeCompressedPoint(getActualPos(), stream);
   }

   return retMask;
}


void Item::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())     // InitialMask
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


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(CoreItem);
class LuaCore;

// Constructor
CoreItem::CoreItem() : Parent(Point(0,0), F32(CoreStartWidth))
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = CoreTypeNumber;
   mHitPoints = CoreStartingHitPoints;     // Hits to kill
   hasExploded = false;

   mKillString = "crashed into a core";    // TODO: Really needed?
}


CoreItem *CoreItem::clone() const
{
   return new CoreItem(*this);
}


void CoreItem::renderItem(const Point &pos)
{
   if(!hasExploded)
      renderCore(pos, calcCoreWidth() / 2, getTeamColor(mTeam));
}


void CoreItem::renderDock()
{
   renderCore(getVert(0), 5, &Colors::white);
}


const char *CoreItem::getEditorHelpString()
{
   return "Core.  Destroy to score.";
}


const char *CoreItem::getPrettyNamePlural()
{
   return "Cores";
}


const char *CoreItem::getOnDockName()
{
   return "Core";
}


const char *CoreItem::getOnScreenName()
{
   return "Core";
}


F32 CoreItem::getEditorRadius(F32 currentScale)
{
   return getRadius() * currentScale;
}


bool CoreItem::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
   return false;
}


bool CoreItem::getCollisionPoly(Vector<Point> &polyPoints) const
{
   Rect rect = Rect(getActualPos(), calcCoreWidth());
   rect.toPoly(polyPoints);
   return true;
}


void CoreItem::damageObject(DamageInfo *theInfo)
{
   if(hasExploded)  
      return; 
   
   mHitPoints--;
   if(mHitPoints == 0)    // Kill small items
   {
      hasExploded = true;
      deleteObject(500);
      setMaskBits(ExplodedMask);    // Fix asteroids delay destroy after hit again...
      return;
   }

   setMaskBits(ItemChangedMask);    // So our clients will get new size
   setRadius(calcCoreWidth());
}


void CoreItem::setRadius(F32 radius) 
{ 
   Parent::setRadius(radius * getGame()->getGridSize());
}


U32 CoreItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & ItemChangedMask))
      stream->writeInt(mHitPoints, 8);

   stream->writeFlag(hasExploded);

   if(stream->writeFlag(updateMask & InitialMask))
      writeThisTeam(stream);

   return retMask;
}


void CoreItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      mHitPoints = stream->readInt(8);
      setRadius(calcCoreWidth());
   }

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      onItemExploded(getActualPos());
   }

   if(stream->readFlag())
      readThisTeam(stream);
}


bool CoreItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 3)         // CoreItem <team> <x> <y>
      return false;

   mTeam = atoi(argv[0]);

   if(!Parent::processArguments(argc-1, argv+1, game))
      return false;

   return true;
}


string CoreItem::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + itos(mTeam) + " " + geomToString(gridSize);
}


F32 CoreItem::calcCoreWidth() const
{
   return
         F32(CoreStartWidth - CoreMinWidth) * F32(mHitPoints) / F32(CoreStartingHitPoints) + CoreMinWidth;
}


bool CoreItem::collide(GameObject *otherObject)
{
   return true;
}


// Client only
void CoreItem::onItemExploded(Point pos)
{
   SoundSystem::playSoundEffect(SFXAsteroidExplode, pos, Point());
   // FXManager::emitBurst(pos, Point(.1, .1), Colors::white, Colors::white, 10);
}


const char CoreItem::className[] = "CoreItem";      // Class name as it appears to Lua scripts

// Lua constructor
CoreItem::CoreItem(lua_State *L)
{
   // Do we want to construct these from Lua?  If so, do that here!
}


// Define the methods we will expose to Lua
Lunar<CoreItem>::RegType CoreItem::methods[] =
{
   // Standard gameItem methods
   method(CoreItem, getClassID),
   method(CoreItem, getLoc),
   method(CoreItem, getRad),
   method(CoreItem, getVel),
   method(CoreItem, getTeamIndx),

   // Class specific methods
   method(CoreItem, getHitPoints),

   {0,0}    // End method list
};


S32 CoreItem::getClassID(lua_State *L)
{
   return returnInt(L, CoreTypeNumber);
}


S32 CoreItem::getHitPoints(lua_State *L)
{
   return returnInt(L, mHitPoints);
}


void CoreItem::push(lua_State *L)
{
   Lunar<CoreItem>::push(L, this);
}


};


