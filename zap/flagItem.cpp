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

#include "flagItem.h"
#include "gameType.h"
#include "ship.h"
#include "UIMenus.h"
#include "../glut/glutInclude.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(FlagItem);

FlagItem::FlagItem(Point pos) : Item(pos, false, 20)
{
   mTeam = 0;
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= FlagType | CommandMapVisType;
}


void FlagItem::onAddedToGame(Game *theGame)
{
   theGame->getGameType()->addFlag(this);
   getGame()->mObjectsLoaded++;
}


void FlagItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 3)
      return;

   mTeam = atoi(argv[0]);
   Parent::processArguments(argc-1, argv+1);
   initialPos = mMoveState[ActualState].pos;

   // Now add the flag starting point to the list of flag spawn points
   if(mTeam < 0)
      gServerGame->getGameType()->mFlagSpawnPoints.push_back(initialPos);
   else
      gServerGame->getGameType()->mTeams[mTeam].flagSpawnPoints.push_back(initialPos);
}

U32 FlagItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
      stream->writeInt(mTeam + 1, 4);
   return Parent::packUpdate(connection, updateMask, stream);
}

void FlagItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
      mTeam = stream->readInt(4) - 1;
   Parent::unpackUpdate(connection, stream);
}

bool FlagItem::isAtHome()
{
   return mMoveState[ActualState].pos == initialPos;
}

void FlagItem::sendHome()
{
   // Now that we have flag spawn points, we'll simply redefine "initial pos" as a random selection of the flag spawn points
   // Everything else should remain as it was
   if(mTeam < 0)
   {
      S32 spawnIndex = TNL::Random::readI() % getGame()->getGameType()->mFlagSpawnPoints.size();
      initialPos = getGame()->getGameType()->mFlagSpawnPoints[spawnIndex];
   }
   else
   {
      S32 spawnIndex = TNL::Random::readI() % getGame()->getGameType()->mTeams[mTeam].flagSpawnPoints.size();
      initialPos = getGame()->getGameType()->mTeams[mTeam].flagSpawnPoints[spawnIndex];
   }

   mMoveState[ActualState].pos = mMoveState[RenderState].pos = initialPos;
   mMoveState[ActualState].vel = Point(0,0);
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

   c = gt->getTeamColor(getTeam());

   renderFlag(pos + offset, c);
}

bool FlagItem::collide(GameObject *hitObject)
{
   if(mIsMounted)
      return false;
   if(hitObject->getObjectTypeMask() & (BarrierType | ForceFieldType))
      return true;
   if(!(hitObject->getObjectTypeMask() & ShipType))
      return false;

   // We've hit a ship
   Ship *ship = dynamic_cast<Ship *>(hitObject);
   if(isGhost() || (ship->hasExploded))
      return false;

   GameType *gt = getGame()->getGameType();
   if(gt)
      gt->shipTouchFlag(ship, this);
   return false;
}

void FlagItem::onMountDestroyed()
{
   GameType *gt = getGame()->getGameType();
   if(!gt)
      return;

   if(!mMount.isValid())
      return;

   gt->flagDropped(mMount, this);
   dismount();
}

};

