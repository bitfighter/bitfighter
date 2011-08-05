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

using namespace TNL;
#include "ship.h"
#include "sparkManager.h"
#include "gameLoader.h"
#include "SoundSystem.h"
#include "gameObjectRender.h"
#include "Colors.h"
#include "game.h"

#include "SDL/SDL_opengl.h"
#include "UI.h"

#include <math.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Teleporter);

static Vector<DatabaseObject *> foundObjects;

// Constructor --> need to set the pos and dest via methods like processArguments to make sure
// that we get the multiple destination aspect of teleporters right
Teleporter::Teleporter()
{
   mObjectTypeMask = TeleportType | CommandMapVisType;
   mObjectTypeNumber = TeleportTypeNumber;
   mNetFlags.set(Ghostable);

   timeout = 0;
   mTime = 0;
   mTeleporterDelay = TeleporterDelay;
}


Teleporter *Teleporter::clone() const
{
   return new Teleporter(*this);
   //copyAttrs(clone.get());

   //return clone;
}


//void Teleporter::copyAttrs(Teleporter *target)
//{
//   SimpleLine::copyAttrs(target);
//}



void Teleporter::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();    // Always in scope!
}


bool Teleporter::processArguments(S32 argc2, const char **argv2, Game *game)
{
   S32 argc = 0;
   const char *argv[8];

   for(S32 i = 0; i < argc2; i++)      // The idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char firstChar = argv2[i][0];    // First character of arg

      if((firstChar >= 'a' && firstChar <= 'z') || (firstChar >= 'A' && firstChar <= 'Z'))  // starts with a letter
      {
         if(!strnicmp(argv2[i], "Delay=", 6))
            mTeleporterDelay = U32(atof(&argv2[i][6]) * 1000);
      }
      else
      {
         if(argc < 8)
         {  
            argv[argc] = argv2[i];
            argc++;
         }
      }
   }

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

   if(!dynamic_cast<ClientGame *>(game))              // Editor handles multi-dest teleporters as separate single dest items
   {
      foundObjects.clear();
      game->getGameObjDatabase()->findObjects(TeleportType, foundObjects, Rect(pos, 1));

      for(S32 i = 0; i < foundObjects.size(); i++)
      {
         Teleporter *tel = dynamic_cast<Teleporter *>(foundObjects[i]);
         if(tel->getVert(0).distSquared(pos) < 1)     // i.e These are really close!  Must be the same!
         {
            tel->mDests.push_back(dest);
            found = true;
            break;      // There will only be one!
         }
      }
   }

   if(!found)           // New teleporter origin
   {
      mDests.push_back(dest);
      setExtent(Rect(pos, (F32)TELEPORTER_RADIUS));
   }
   else  
      destroySelf();    // Since this is really part of a different teleporter, delete this one

   return true;
}


string Teleporter::toString(F32 gridSize) const
{
   string out = string(getClassName()) + " " + geomToString(gridSize);
   if(mTeleporterDelay != TeleporterDelay)
      out += " Delay=" + ftos(mTeleporterDelay / 1000.f, 3);
   return out;
}


U32 Teleporter::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   bool isInitial = (updateMask & BIT(3));

   if(stream->writeFlag(updateMask & InitMask))
   {
      getVert(0).write(stream);

      stream->writeInt(mDests.size(), 16);

      for(S32 i = 0; i < mDests.size(); i++)
         mDests[i].write(stream);

      if(stream->writeFlag(mTeleporterDelay != TeleporterDelay))
         stream->writeInt(mTeleporterDelay, 32);
   }

   if(stream->writeFlag((updateMask & TeleportMask) && !isInitial))    // Basically, this gets triggered if a ship passes through
      stream->write(mLastDest);     // Where ship is going

   return 0;
}

void Teleporter::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
   {
      U32 count;
      Point pos;
      pos.read(stream);
      setVert(pos, 0);

      count = stream->readInt(16);
      mDests.resize(count);

      for(U32 i = 0; i < count; i++)
         mDests[i].read(stream);
      
      setExtent(Rect(pos, (F32)TELEPORTER_RADIUS));

      if(stream->readFlag())
         mTeleporterDelay = stream->readInt(32);
   }

   if(stream->readFlag() && isGhost())
   {
      S32 dest;
      stream->read(&dest);

      FXManager::emitTeleportInEffect(mDests[dest], 0);
      SoundSystem::playSoundEffect(SFXTeleportIn, mDests[dest], Point());

      SoundSystem::playSoundEffect(SFXTeleportOut, getVert(0), Point());
      timeout = mTeleporterDelay;
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
   Rect queryRect(getVert(0), (F32)TELEPORTER_RADIUS);

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
         timeout = mTeleporterDelay;    // Temporarily disable teleporter
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
         if(s->getOwner())
            s->getOwner()->mStatistics.mTeleport++;
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
   if(timeout > TeleporterExpandTime - TeleporterDelay + mTeleporterDelay)
      r = (timeout - TeleporterExpandTime + TeleporterDelay - mTeleporterDelay) / F32(TeleporterDelay - TeleporterExpandTime);
   else if(timeout < TeleporterExpandTime)
      r = F32(TeleporterExpandTime - timeout) / F32(TeleporterExpandTime);
   else
      r = 0;

   if(r != 0)
      renderTeleporter(getVert(0), 0, true, mTime, r, (F32)TELEPORTER_RADIUS, 1.0, mDests, false);
}


void Teleporter::renderEditorItem()
{
   glColor(Colors::green);

   glLineWidth(gLineWidth3);
   drawPolygon(getVert(0), 12, (F32)TELEPORTER_RADIUS, 0);
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

