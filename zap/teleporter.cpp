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
#include "SoundSystem.h"
#include "gameObjectRender.h"
#include <math.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Teleporter);

static Vector<DatabaseObject *> foundObjects;

// Constructor --> need to set the pos and dest via methods like processArguments to make sure
// that we get the multiple destination aspect of teleporters right
Teleporter::Teleporter() : SimpleLine(TeleportType)
{
   mObjectTypeMask = TeleportType | CommandMapVisType;
   mObjectTypeNumber = TeleporterTypeNumber;
   mNetFlags.set(Ghostable);

   timeout = 0;
   mTime = 0;
}


void Teleporter::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();    // Always in scope!
}


bool Teleporter::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc != 4)
      return false;

   Point pos, dest;

   pos.read(argv);
   dest.read(argv + 2);

   pos *= game->getGridSize();
   dest *= game->getGridSize();

   setVert(pos, 0);
   setVert(dest, 1);

   // See if we already have any teleports with this pos... if so, this is a "multi-dest" teleporter
   bool found = false;

   foundObjects.clear();
   findObjects(TeleportType, foundObjects, Rect(pos, 2));      // 1 would probably work just as well here

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Teleporter *tel = dynamic_cast<Teleporter *>(foundObjects[i]);
      if(tel->getVert(0).distanceTo(pos) < 1)     // i.e These are really close!  Must be the same!  --> is always true?
      {
         tel->mDests.push_back(dest);
         found = true;
         break;      // There will only be one!
      }
   }

   if(!found)     // New teleporter origin
   {
      mDests.push_back(dest);
      setExtent(Rect(pos, TELEPORTER_RADIUS));
   }
   else  
      // Since this is really part of a different teleporter, delete this one
      destroySelf();

   return true;
}


string Teleporter::toString()
{
   F32 gs = getGame()->getGridSize();
   
   Point pos = getVert(0) / gs;
   Point dest = getVert(1) / gs;

   return string(getClassName()) + " " + pos.toString() + " " + dest.toString();
}


U32 Teleporter::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   bool isInitial = (updateMask & BIT(3));

   if(stream->writeFlag(updateMask & InitMask))
   {
      getVert(0).write(stream);

      stream->write(mDests.size());

      for(S32 i = 0; i < mDests.size(); i++)
         mDests[i].write(stream);
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
      Point pos;
      pos.read(stream);
      setVert(pos, 0);

      stream->read(&count);
      mDests.clear();

      Point p;    // Reusable container
      for(S32 i = 0; i < count; i++)
      {
         p.read(stream);
         mDests.push_back(p);
      }
      
      setExtent(Rect(pos, TELEPORTER_RADIUS));
   }

   if(stream->readFlag() && isGhost())
   {
      S32 dest;
      stream->read(&dest);

      FXManager::emitTeleportInEffect(mDests[dest], 0);
      SoundSystem::playSoundEffect(SFXTeleportIn, mDests[dest], Point());

      SoundSystem::playSoundEffect(SFXTeleportOut, getVert(0), Point());
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

   // Check for players within range.  If found, send them to dest.
   Rect queryRect(getVert(0), TELEPORTER_RADIUS);

   foundObjects.clear();
   findObjects(ShipType | RobotType, foundObjects, queryRect);

   // First see if we're triggered...
   bool isTriggered = false;
   Point pos = getVert(0);

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Ship *s = dynamic_cast<Ship *>(foundObjects[i]);
      if((pos - s->getActualPos()).len() < TeleporterTriggerRadius)
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
      if((pos - s->getRenderPos()).len() < TELEPORTER_RADIUS + s->getRadius())
      {
         mLastDest = TNL::Random::readI(0, mDests.size() - 1);
         Point newPos = s->getActualPos() - pos + mDests[mLastDest];    
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

   renderTeleporter(getVert(0), 0, true, mTime, r, TELEPORTER_RADIUS, 1.0, mDests, false);
}


void Teleporter::renderEditorItem()
{
   glColor(green);

   glLineWidth(gLineWidth3);
   drawPolygon(getVert(0), 12, Teleporter::TELEPORTER_RADIUS, 0);
   glLineWidth(gDefaultLineWidth);
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

