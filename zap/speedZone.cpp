//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#include "speedZone.h"
#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "gameObjectRender.h"
#include "ship.h"
#include "UI.h"
#include "../glut/glutInclude.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(SpeedZone);

// Constructor
SpeedZone::SpeedZone()
{
   mNetFlags.set(Ghostable);
   mObjectTypeMask = CommandMapVisType;
   mSpeed = defaultSpeed;
}


// Take our basic inputs, pos and dir, and expand them into a three element
// vector (the three points of our triangle graphic), and compute its extent
void SpeedZone::preparePoints()
{
   mPolyBounds = generatePoints(pos, dir);
   computeExtent();
}

Vector<Point> SpeedZone::generatePoints(Point pos, Point dir)
{
   Vector<Point> points;

   // Yes, we already did this if we came from processArguments,
   // but not if we came from elsewhere, like UIInstructions
   Point offset(dir - pos);
   offset.normalize();
   dir = Point(pos + offset * SpeedZone::height);

   Point perpendic(pos.y - dir.y, dir.x - pos.x);
   perpendic.normalize();

   points.push_back(pos + perpendic * SpeedZone::halfWidth);
   points.push_back(dir);
   points.push_back(pos - perpendic * SpeedZone::halfWidth);

   return points;
}

void SpeedZone::render()
{
   renderSpeedZone(pos, dir, gClientGame->getCurrentTime());
}

// This object should be drawn above polygons
S32 SpeedZone::getRenderSortValue()
{
   return 0;
}


// Create objects from parameters stored in level file
void SpeedZone::processArguments(S32 argc, const char **argv)
{
   if(argc <= 4)
      return;

   pos.read(argv);
   pos *= getGame()->getGridSize();

   dir.read(argv + 2);
   dir *= getGame()->getGridSize();


   // Adjust the direction point so that it also represents the tip of the triangle
   Point offset(dir - pos);
   offset.normalize();
   dir = Point(pos + offset * height);

   preparePoints();

   if(argc == 5)
      mSpeed = max(minSpeed, min(maxSpeed, atoi(argv[4])));
}


void SpeedZone::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();

   getGame()->mObjectsLoaded++;
}

// Bounding box for quick collision-possibility elimination
void SpeedZone::computeExtent()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);
   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);

   setExtent(extent);
}

// More precise boundary for precise collision detection
bool SpeedZone::getCollisionPoly(U32 state, Vector<Point> &polyPoints)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      polyPoints.push_back(mPolyBounds[i]);
   return true;
}

// Handle collisions with a SpeedZone
bool SpeedZone::collide(GameObject *hitObject)
{
   if(!isGhost() && (hitObject->getObjectTypeMask() & ShipType))     // Only ships collide, and only happens on server
   {
      Ship *s = dynamic_cast<Ship *>(hitObject);

     // Make sure ship hasn't been excluded
      for(S32 i = 0; i < mExclusions.size(); i++)
         if (mExclusions[i].ship == s)
            return false;

      Point impulse = (dir - pos);
      impulse.normalize(mSpeed); 
      //impulse.normalize(impulseSpeed); 
      s->setActualVel(Point(0,0));
      s->setActualPos(pos);
      s->mImpulseVector = impulse * 1.5;

      // To ensure we don't give multiple impulses to the same ship, we'll exclude it from
      // further action for about 300ms.  That should do the trick.

      Exclusion exclusion;
      exclusion.ship = s;
      exclusion.time = gServerGame->getCurrentTime() + 300;

      mExclusions.push_back(exclusion);
   }

   return false;
}

// Runs only on server!
void SpeedZone::idle(GameObject::IdleCallPath path)
{
   // Check for old exclusions that no longer apply
   for(S32 i = 0; i < mExclusions.size(); i++)
      if(mExclusions[i].time < gServerGame->getCurrentTime())     // Exclusion has expired
         mExclusions.erase(i);
}

U32 SpeedZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   stream->write(pos.x);
   stream->write(pos.y);

   stream->write(dir.x);
   stream->write(dir.y);

   stream->writeInt(mSpeed, 16);

   return 0;
}

void SpeedZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   stream->read(&pos.x);
   stream->read(&pos.y);

   stream->read(&dir.x);
   stream->read(&dir.y);

   mSpeed = stream->readInt(16);

   preparePoints();
}



};

