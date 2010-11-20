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

#include "teleporter.h"
#include "../glut/glutInclude.h"

using namespace TNL;
#include "ship.h"
#include "sparkManager.h"
#include "gameLoader.h"
#include "sfx.h"
#include "gameObjectRender.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Teleporter);

static Vector<DatabaseObject *> foundObjects;

// Constructor --> need to set the pos and dest via methods like processArguments to make sure
// that we get the multiple destination aspect of teleporters right
Teleporter::Teleporter()
{
   mNetFlags.set(Ghostable);
   mPos = Point(F32_MAX, F32_MAX);
   timeout = 0;

   mObjectTypeMask |= CommandMapVisType | TeleportType;
   mTime = 0;
}

void Teleporter::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();    // Always in scope!
   getGame()->mObjectsLoaded++;
}

bool Teleporter::processArguments(S32 argc, const char **argv)
{
   if(argc != 4)
      return false;

   Point pos;
   pos.read(argv);
   pos *= getGame()->getGridSize();

   Point dest;
   dest.read(argv + 2);
   dest *= getGame()->getGridSize();

   // See if we already have any teleports with this pos... if so, this is a "multi-dest" teleporter
   bool found = false;

   foundObjects.clear();
   findObjects(TeleportType, foundObjects, gServerGame->computeWorldObjectExtents());

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Teleporter *tel = dynamic_cast<Teleporter *>(foundObjects[i]);
      if(tel->mPos.distanceTo(pos) < 1)     // i.e these are really close!  Must be the same!
      {
         tel->mDest.push_back(dest);
         found = true;
         break;      // There will be only one!
      }
   }

   if(!found)     // New teleporter origin
   {
      mPos = pos;
      mDest.push_back(dest);
      Rect r(pos, pos);
      r.expand(Point(TELEPORTER_RADIUS, TELEPORTER_RADIUS));
      setExtent(r);
   }
   else  
   {
      // Since this is really part of a different teleporter, delete this one
      removeFromDatabase();
      this->destroySelf();
   }

   return true;
}


U32 Teleporter::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   bool isInitial = (updateMask & BIT(3));

   if(stream->writeFlag(updateMask & InitMask))
   {
      stream->write(mPos.x);
      stream->write(mPos.y);
      stream->write(mDest.size());

      for(S32 i = 0; i < mDest.size(); i++)
      {
         stream->write(mDest[i].x);
         stream->write(mDest[i].y);
      }
   }

   if(stream->writeFlag((updateMask & TeleportMask) && !isInitial))    // Basically, this gets triggered if a ship passes through
      stream->write(mLastDest);     // Where ship is going

   return 0;
}

void Teleporter::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   S32 count;
   if(stream->readFlag())
   {
      stream->read(&mPos.x);
      stream->read(&mPos.y);
      stream->read(&count);
      mDest.clear();
      for(S32 i = 0; i < count; i++)
      {
         Point dest;
         stream->read(&dest.x);
         stream->read(&dest.y);
         mDest.push_back(dest);
      }
      Rect r(mPos, mPos);
      r.expand(Point(TELEPORTER_RADIUS, TELEPORTER_RADIUS));
      setExtent(r);
   }
   if(stream->readFlag() && isGhost())
   {
      S32 dest;
      stream->read(&dest);

      FXManager::emitTeleportInEffect(mDest[dest], 0);
      SFXObject::play(SFXTeleportIn, mDest[dest], Point());

      SFXObject::play(SFXTeleportOut, mPos, Point());
      timeout = TeleporterDelay;
   }
}


void Teleporter::idle(GameObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;
   mTime += deltaT;

   // Deal with our timeout...  could rewrite with a timer!
   if(timeout > deltaT)
   {
      timeout -= deltaT;
      return;
   }
   else
      timeout = 0;

   if(path != GameObject::ServerIdleMainLoop)
      return;

   // Check for players within range. If so, send them to dest
   Rect queryRect(mPos, mPos);
   queryRect.expand(Point(TELEPORTER_RADIUS, TELEPORTER_RADIUS));

   foundObjects.clear();
   findObjects(ShipType | RobotType, foundObjects, queryRect);

   // First see if we're triggered...
   bool isTriggered = false;

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Ship *s = dynamic_cast<Ship *>(foundObjects[i]);
      if((mPos - s->getActualPos()).len() < TeleporterTriggerRadius)
      {
         isTriggered = true;
         timeout = TeleporterDelay;    // Temporarily disable teleporter
         // break; <=== maybe, need to test
      }
   }

   if(!isTriggered)
      return;
   
   // We've triggered the teleporter.  Relocate ship.
   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Ship *s = dynamic_cast<Ship *>(foundObjects[i]);
      if((mPos - s->getRenderPos()).len() < TELEPORTER_RADIUS + s->getRadius())
      {
         mLastDest = TNL::Random::readI(0, mDest.size() - 1);
         Point newPos = s->getActualPos() - mPos + mDest[mLastDest];    
         s->setActualPos(newPos, true);
         setMaskBits(TeleportMask);
      }
   }
}

inline Point polarToRect(Point p)
{
   F32 &r  = p.x;
   F32 &th = p.y;

   return Point(cos(th) * r, sin(th) * r);
}

void Teleporter::render()
{
   F32 r;
   if(timeout > TeleporterExpandTime)
      r = (timeout - TeleporterExpandTime) / F32(TeleporterDelay - TeleporterExpandTime);
   else
      r = F32(TeleporterExpandTime - timeout) / F32(TeleporterExpandTime);

   renderTeleporter(mPos, 0, true, mTime, r, TELEPORTER_RADIUS, 1.0, mDest, false);
}

// Lua methods

const char Teleporter::className[] = "Teleporter";      // Class name as it appears to Lua scripts

// Lua constructor
Teleporter::Teleporter(lua_State *L)
{
   // Do nothing
}


// Define the methods we will expose to Lua
Lunar<Teleporter>::RegType Teleporter::methods[] =
{
   // Standard gameItem methods
   method(Teleporter, getClassID),
   method(Teleporter, getLoc),
   method(Teleporter, getRad),
   method(Teleporter, getVel),
   method(Teleporter, getTeamIndx),

   {0,0}    // End method list
};

};

