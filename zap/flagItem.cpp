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

#include "flagItem.h"
#include "gameType.h"
#include "ship.h"
#include "UIMenus.h"
#include "../glut/glutInclude.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(FlagItem);

// C++ constructor
FlagItem::FlagItem(Point pos) : Item(pos, false, 20)
{
   initialize();
}

// Alternate constructor, currently used by HuntersFlag
FlagItem::FlagItem(Point pos, bool collidable, float radius, float mass) : Item(pos, collidable, radius, mass)
{
   initialize();
}


void FlagItem::initialize()
{
   mTeam = -1;
   mFlagCount = 1;
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= FlagType | CommandMapVisType;
   setZone(NULL);
}


const char FlagItem::className[] = "FlagItem";      // Class name as it appears to Lua scripts

// Define the methods we will expose to Lua
Lunar<FlagItem>::RegType FlagItem::methods[] =
{
   // Standard gameItem methods
   method(FlagItem, getClassID),
   method(FlagItem, getLoc),
   method(FlagItem, getRad),
   method(FlagItem, getVel),

   // Flag specific methods
   method(FlagItem, getTeamIndx),
   method(FlagItem, isInInitLoc),
   method(FlagItem, isInCaptureZone),
   method(FlagItem, isOnShip),

   {0,0}    // End method list
};



void FlagItem::onAddedToGame(Game *theGame)
{
   theGame->getGameType()->addFlag(this);    // Does nothing for Nexus game
   getGame()->mObjectsLoaded++;
}


bool FlagItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 3)         // FlagItem <team> <x> <y> {time}
      return false;

   mTeam = atoi(argv[0]);
   
   if(!Parent::processArguments(argc-1, argv+1))
      return false;

   S32 time = (argc >= 4) ? atoi(argv[4]) : 0;     // Flag spawn time is possible 4th argument.  This time only turns out to be important in Nexus games at the moment.

   mInitialPos = mMoveState[ActualState].pos;

   // Now add the flag starting point to the list of flag spawn points
   if(!gServerGame->getGameType()->isTeamFlagGame() || mTeam < 0)
      gServerGame->getGameType()->mFlagSpawnPoints.push_back(FlagSpawn(mInitialPos, time));
   else
      gServerGame->getGameType()->mTeams[mTeam].flagSpawnPoints.push_back(FlagSpawn(mInitialPos, time));

   return true;
}


U32 FlagItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
      stream->writeInt(mTeam + 2, 4);
   return Parent::packUpdate(connection, updateMask, stream);
}


void FlagItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
      mTeam = stream->readInt(4) - 2;
   Parent::unpackUpdate(connection, stream);
}


void FlagItem::idle(GameObject::IdleCallPath path)
{
   Parent::idle(path);

   if(isGhost()) 
      return;
   
   // Server only...
   U32 deltaT = mCurrentMove.time;
   mDroppedTimer.update(deltaT);
}


bool FlagItem::isAtHome()
{
   return mMoveState[ActualState].pos == mInitialPos;
}


void FlagItem::sendHome()
{
   // Now that we have flag spawn points, we'll simply redefine "initial pos" as a random selection of the flag spawn points
   // Everything else should remain as it was

   // First, make list of valid spawn points -- start with a list of all spawn points, then remove any occupied ones

   Vector<FlagSpawn> spawnPoints;
   GameType *gt = getGame()->getGameType();

   if(!gt->isTeamFlagGame() || mTeam < 0)     // Neutral or hostile flag
      spawnPoints = gt->mFlagSpawnPoints;
   else              // Team flag
      spawnPoints = gt->mTeams[mTeam].flagSpawnPoints;

   // Now remove the occupied spots from our list of potential spawns
   for(S32 i = 0; i < gt->mFlags.size(); i++)
   {
      FlagItem *flag = gt->mFlags[i];
      if(flag->isAtHome() && (flag->mTeam < 0 || flag->mTeam == mTeam))
      {
         // Need to remove this flag's spawnpoint from the list of potential spawns... it's occupied, after all...
         for(S32 j = 0; j < spawnPoints.size(); j++)
            if(spawnPoints[j].getPos() == flag->mInitialPos)
            {
               spawnPoints.erase(j);
               break;
            }
      }
   }

   S32 spawnIndex = TNL::Random::readI() % spawnPoints.size();
   mInitialPos = spawnPoints[spawnIndex].getPos();

   mMoveState[ActualState].pos = mMoveState[RenderState].pos = mInitialPos;
   mMoveState[ActualState].vel = mMoveState[RenderState].vel = Point(0,0);
   setMaskBits(PositionMask);
   updateExtent();
}


void FlagItem::renderItem(Point pos)
{
   Point offset;

   if(mIsMounted)
      offset.set(15, -15);

   Color c;
   GameType *gt = getGame()->getGameType();

   c = gt->getTeamColor(mTeam);

   renderFlag(pos + offset, c);
}


bool FlagItem::collide(GameObject *hitObject)
{
   if(mIsMounted || !mIsCollideable)
      return false;

   if(hitObject->getObjectTypeMask() & (BarrierType | ForceFieldType))
      return true;

   if(isGhost() || !(hitObject->getObjectTypeMask() & (ShipType | RobotType)))
      return false;

   // We've hit a ship or robot  (remember, robot is a subtype of ship, so this will work for both)
   Ship *ship = dynamic_cast<Ship *>(hitObject);
   if(!ship || (ship->hasExploded))
      return false;

   // Server only from here on out...

   if(mDroppedTimer.getCurrent())    // Dropped flag not ready to be picked up! 
      return false;

   GameType *gt = getGame()->getGameType();

   if(!gt)
      return false;
   
   gt->shipTouchFlag(ship, this);

   return false;
}


// Private helper function -- what happens when flag is dropped?
void FlagItem::flagDropped()
{
   GameType *gt = getGame()->getGameType();
   if(!gt || !mMount.isValid())
      return;

   gt->flagDropped(mMount, this);
   dismount();
}


void FlagItem::onMountDestroyed()
{
   flagDropped();
}


void FlagItem::onItemDropped(Ship *ship)
{
   flagDropped();
   mDroppedTimer.reset(dropDelay);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
FlagSpawn::FlagSpawn(Point pos, S32 delay)
{
   mPos = pos;
   timer = Timer(delay);
};

};
