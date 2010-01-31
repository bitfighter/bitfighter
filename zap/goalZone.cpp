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

#include "goalZone.h"
#include "gameType.h"
#include "gameObjectRender.h"


namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(GoalZone);

GoalZone::GoalZone()
{
   mTeam = -1;
   mNetFlags.set(Ghostable);
   mObjectTypeMask = CommandMapVisType | GoalZoneType;
   mFlashCount = 0;
   mHasFlag = false;
}


void GoalZone::render()
{
   GameType *gt = getGame()->getGameType();
   F32 glow = gt->mZoneGlowTimer.getFraction();

   // Check if to make sure that the zone matches the glow team if we're glowing
      if(gt->mGlowingZoneTeam >= 0 && gt->mGlowingZoneTeam != mTeam)
      glow = 0;

   renderGoalZone(mPolyBounds, mPolyFill, mCentroid, gt->getTeamColor(getTeam()), mLabelAngle, isFlashing(), glow);
}


S32 GoalZone::getRenderSortValue()
{
   return -1;     // Renders beneath everything else
}

extern S32 gMaxPolygonPoints;

bool GoalZone::processArguments(S32 argc, const char **argv)
{
   if(argc < 7)
      return false;

   mTeam = atoi(argv[0]);     // Team is first arg
   processPolyBounds(argc, argv, 1, getGame()->getGridSize());
   computeExtent();

   /*for(S32 i = 2; i < argc; i += 2)
   {
      // Put a cap on the number of vertices in a polygon
      if(mPolyBounds.verts.size() >= gMaxPolygonPoints)
         break;
      Point p;
      p.x = (F32) atof(argv[i-1]) * getGame()->getGridSize();
      p.y = (F32) atof(argv[i]) * getGame()->getGridSize();
      mPolyBounds.push_back(p);
   } */

   return true;
}

void GoalZone::setTeam(S32 team)
{
   mTeam = team;
   setMaskBits(TeamMask);
}

void GoalZone::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();

   theGame->getGameType()->addZone(this);

  theGame->mObjectsLoaded++;     // N.B.: For some reason this has no effect on the client
}

void GoalZone::computeExtent()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);
   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);
   setExtent(extent);
}

bool GoalZone::getCollisionPoly(Vector<Point> &polyPoints)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      polyPoints.push_back(mPolyBounds[i]);
   return true;
}

bool GoalZone::collide(GameObject *hitObject)
{
   if( !isGhost() && (hitObject->getObjectTypeMask() & (ShipType | RobotType)) )
   {
      Ship *s = dynamic_cast<Ship *>(hitObject); 
      getGame()->getGameType()->shipTouchZone(s, this);
   }

   return false;
}

U32 GoalZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
      Polyline::packUpdate(connection, stream);

   if(stream->writeFlag(updateMask & TeamMask))
      stream->write(mTeam);

   return 0;
}

void GoalZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag()) 
   {
      if(Polygon::unpackUpdate(connection, stream))
         computeExtent();
   }

   if(stream->readFlag())
   {
      stream->read(&mTeam);                      // Zone was captured by team mTeam
      if(!isInitialUpdate() && mTeam != -1)      // mTeam will be -1 on touchdown, and we don't want to flash then!
      {
         mFlashTimer.reset(FlashDelay);
         mFlashCount = FlashCount;
      }
   }
}


void GoalZone::idle(GameObject::IdleCallPath path)
{
   if(path != GameObject::ClientIdleMainRemote || mFlashCount == 0)
      return;

   if(mFlashTimer.update(mCurrentMove.time))
   {
      mFlashTimer.reset(FlashDelay);
      mFlashCount--;
   }
}


};


