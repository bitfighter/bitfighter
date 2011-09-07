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
Item::Item(const Point &pos, F32 radius, F32 mass)
{
   setActualPos(pos);
   mRadius = radius;
   mMass = mass;

   mItemId = sItemId;
   sItemId++;
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


// Provide generic item rendering; will be overridden
void Item::renderItem(const Point &pos)
{
#ifndef ZAP_DEDICATED
   glColor(Colors::cyan);
   drawSquare(pos, 10, true);
#endif
}


void Item::renderEditor(F32 currentScale)
{
   renderItem(getVert(0));                    
}


F32 Item::getEditorRadius(F32 currentScale)
{
   return (getRadius() + 2) * currentScale;
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Reactor);
class LuaReactor;

static const F32 REACTOR_MASS = F32_MAX;

// Constructor
Reactor::Reactor() : Parent(Point(0,0), (F32)REACTOR_RADIUS, REACTOR_MASS)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = ReactorTypeNumber;
   mHitPoints = 10;     // Hits to kill
   hasExploded = false;

   F32 vel = 0;

   mKillString = "crashed into an reactor";     // TODO: Really needed?
}


Reactor *Reactor::clone() const
{
   return new Reactor(*this);
}


void Reactor::renderItem(const Point &pos)
{
   if(!hasExploded)
      renderReactor(pos, getReactorRadius());
}


void Reactor::renderDock()
{
   renderReactor(getVert(0), 5);
}


F32 Reactor::getEditorRadius(F32 currentScale)
{
   return 75 * currentScale;
}


bool Reactor::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
   return false;
}


bool Reactor::getCollisionPoly(Vector<Point> &polyPoints) const
{
   return false;
}


bool Reactor::getCollisionRect(U32 state, Rect &rect) const
{
   rect = Rect(getActualPos(), F32(getReactorRadius()));
   return true;
}


void Reactor::damageObject(DamageInfo *theInfo)
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
   setRadius(getReactorRadius());
}


void Reactor::setRadius(F32 radius) 
{ 
   Parent::setRadius(radius * getGame()->getGridSize()); 
}


U32 Reactor::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & ItemChangedMask))
      stream->writeInt(mHitPoints, 8);

   stream->writeFlag(hasExploded);

   return retMask;
}


void Reactor::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      mHitPoints = stream->readInt(8);
      setRadius(getReactorRadius());

      //if(!mInitial)
      //   SoundSystem::playSoundEffect(SFXAsteroidExplode, mMoveState[RenderState].pos, Point());
   }

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      onItemExploded(getActualPos());
   }
}


F32 Reactor::getReactorRadius() const
{
   return F32(5 + 2 * mHitPoints);
}


bool Reactor::collide(GameObject *otherObject)
{
   return true;
}


// Client only
void Reactor::onItemExploded(Point pos)
{
   SoundSystem::playSoundEffect(SFXAsteroidExplode, pos, Point());
   // FXManager::emitBurst(pos, Point(.1, .1), Colors::white, Colors::white, 10);
}


const char Reactor::className[] = "Reactor";      // Class name as it appears to Lua scripts

// Lua constructor
Reactor::Reactor(lua_State *L)
{
   // Do we want to construct these from Lua?  If so, do that here!
}


// Define the methods we will expose to Lua
Lunar<Reactor>::RegType Reactor::methods[] =
{
   // Standard gameItem methods
   method(Reactor, getClassID),
   method(Reactor, getLoc),
   method(Reactor, getRad),
   method(Reactor, getVel),
   method(Reactor, getTeamIndx),

   // Class specific methods
   method(Reactor, getHitPoints),

   {0,0}    // End method list
};


S32 Reactor::getHitPoints(lua_State *L) { return returnInt(L, mHitPoints); }      



};


