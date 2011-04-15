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

#include "robotController.h"
#include "ship.h"
#include "robot.h"
#include "gameType.h"
#include "BotNavMeshZone.h"

namespace Zap{


extern bool calcInterceptCourse(GameObject *target, Point aimPos, F32 aimRadius, S32 aimTeam, F32 aimVel, F32 aimLife, bool ignoreFriendly, F32 &interceptAngle);


// Another helper function: returns id of closest zone to a given point
U16 findClosestZone(Game *game, const Point &point)
{
   U16 closestZone = U16_MAX;

   // First, do a quick search for zone based on the buffer; should be 99% of the cases

   // Search radius is just slightly larger than twice the zone buffers added to objects like barriers
   S32 searchRadius = 2 * BotNavMeshZone::BufferRadius + 1;

   Vector<DatabaseObject*> objects;
   Rect rect = Rect(point.x + searchRadius, point.y + searchRadius, point.x - searchRadius, point.y - searchRadius);

   game->mDatabaseForBotZones.findObjects(BotNavMeshZoneType, objects, rect);

   for(S32 i = 0; i < objects.size(); i++)
   {
      BotNavMeshZone *zone = dynamic_cast<BotNavMeshZone *>(objects[i]);
      Point center = zone->getCenter();

      if(game->getGridDatabase()->pointCanSeePoint(center, point))  // This is an expensive test
      {
         closestZone = zone->getZoneId();
         break;
      }
   }

   // Target must be outside extents of the map, find nearest zone if a straight line was drawn
   if (closestZone == U16_MAX)
   {
      Point extentsCenter = gServerGame->getWorldExtents().getCenter();

      F32 collisionTimeIgnore;
      Point surfaceNormalIgnore;

      DatabaseObject* object = game->mDatabaseForBotZones.findObjectLOS(BotNavMeshZoneType,
            MoveObject::ActualState, point, extentsCenter, collisionTimeIgnore, surfaceNormalIgnore);

      BotNavMeshZone *zone = dynamic_cast<BotNavMeshZone *>(object);

      if (zone != NULL)
         closestZone = zone->getZoneId();
   }

   return closestZone;
}



bool shipCanSeePoint(Ship *ship, Point point)
{
   // Need to check the two edge points perpendicular to the direction of looking to ensure we have an unobstructed
   // flight lane to point.  Radius of the robot is mRadius.  This keeps the ship from getting hung up on
   // obstacles that appear visible from the center of the ship, but are actually blocked.

   Point difference = point - ship->getActualPos();

   Point crossVector(difference.y, -difference.x);  // Create a point whose vector from 0,0 is perpenticular to the original vector
   crossVector.normalize(ship->getRadius());                  // reduce point so the vector has length of ship radius

   // edge points of ship
   Point shipEdge1 = ship->getActualPos() + crossVector;
   Point shipEdge2 = ship->getActualPos() - crossVector;

   // edge points of point
   Point pointEdge1 = point + crossVector;
   Point pointEdge2 = point - crossVector;

   return(
      ship->getGame()->getGridDatabase()->pointCanSeePoint(shipEdge1, pointEdge1) &&
      ship->getGame()->getGridDatabase()->pointCanSeePoint(shipEdge2, pointEdge2) );
}



// Get next waypoint to head toward when traveling from current location to x,y
// Note that this function will be called frequently by various robots, so any
// optimizations will be helpful.
bool getWaypoint(Point &target, Ship *ship, Vector<Point> *flightPlan = NULL, U16 *flightPlanTo1 = NULL)
{

   // If we can see the target, go there directly
   if(shipCanSeePoint(ship, target))
   {
		if(flightPlan)
			flightPlan->clear();
      return true;
   }

   // TODO: cache destination point; if it hasn't moved, then skip ahead.

   U16 targetZone = BotNavMeshZone::findZoneContaining(target);       // Where we're going  ===> returns zone id

   if(targetZone == U16_MAX)       // Our target is off the map.  See if it's visible from any of our zones, and, if so, go there
   {
      targetZone = findClosestZone(ship->getGame(), target);

      if(targetZone == U16_MAX)
         return false;
   }

	U16 flightPlanTo = flightPlanTo1 == NULL ? U16_MAX : *flightPlanTo1;

   // Make sure target is still in the same zone it was in when we created our flightplan.
   // If we're not, our flightplan is invalid, and we need to skip forward and build a fresh one.
   if(flightPlan->size() > 0 && targetZone == flightPlanTo)
   {
      // In case our target has moved, replace final point of our flightplan with the current target location
      (*flightPlan)[0] = target;

      // First, let's scan through our pre-calculated waypoints and see if we can see any of them.
      // If so, we'll just head there with no further rigamarole.  Remember that our flightplan is
      // arranged so the closest points are at the end of the list, and the target is at index 0.
      Point dest;
      bool found = false;
      bool first = true;

      while(flightPlan->size() > 0)
      {
         Point last = flightPlan->last();

         // We'll assume that if we could see the point on the previous turn, we can
         // still see it, even though in some cases, the turning of the ship around a
         // protruding corner may make it technically not visible.  This will prevent
         // rapidfire recalcuation of the path when it's not really necessary.

         // removed if(first) ... Problems with Robot get stuck after pushed from burst or mines.
         // To save calculations, might want to avoid (thisRobot->canSeePoint(last))
         if(shipCanSeePoint(ship, last))
         {
            dest = last;
            found = true;
            first = false;
            flightPlan->pop_back();   // Discard now possibly superfluous waypoint
         }
         else
            break;
      }

      // If we found one, that means we found a visible waypoint, and we can head there...
      if(found)
      {
         flightPlan->push_back(dest);    // Put dest back at the end of the flightplan
			target = dest;
         return true;
      }
   }

   // We need to calculate a new flightplan
	if(flightPlan)
		flightPlan->clear();

   U16 currentZone = BotNavMeshZone::findZoneContaining(ship->getActualPos());     // Where we are

   if(currentZone == U16_MAX)      // We don't really know where we are... bad news!  Let's find closest visible zone and go that way.
   {
      currentZone = findClosestZone(ship->getGame(), ship->getActualPos());
   }
   if(currentZone == U16_MAX)      // That didn't go so well...
      return false;

   // We're in, or on the cusp of, the zone containing our target.  We're close!!
   if(currentZone == targetZone)
   {
      if(shipCanSeePoint(ship, target))           // Possible, if we're just on a boundary, and a protrusion's blocking a ship edge
      {
         target = gBotNavMeshZones[targetZone]->getCenter();
			if(flightPlan)
				flightPlan->push_back(target);
      }

		if(flightPlan)
			flightPlan->push_back(target);
      return true;
   }

   // If we're still here, then we need to find a new path.  Either our original path was invalid for some reason,
   // or the path we had no longer applied to our current location
	if(flightPlanTo1)
		(*flightPlanTo1) = targetZone;

   // check cache for path first
   pair<S32,S32> pathIndex = pair<S32,S32>(currentZone, targetZone);

	Vector<Point> foundPath;

   if (ship->getGame()->getGameType()->cachedBotFlightPlans.find(pathIndex) == ship->getGame()->getGameType()->cachedBotFlightPlans.end())
   {
      // not found so calculate flight plan
      foundPath = AStar::findPath(currentZone, targetZone, target);

      // now add to cache
      gServerGame->getGameType()->cachedBotFlightPlans[pathIndex] = foundPath;
		if(flightPlan)
			(*flightPlan) = foundPath;
   }
   else
   {
      foundPath = gServerGame->getGameType()->cachedBotFlightPlans[pathIndex];
   }

	if(flightPlan)
		(*flightPlan) = foundPath;

   if(foundPath.size() > 0)
	{
      target = foundPath.last();
		return true;
	}
   else
      return false;    // Out of options, end of the road
}


void findNearestEnemyShip(Ship *ship, Ship **shipInVisibleArea, Ship **shipAttackable = NULL)
{
   Ship *enemyship = NULL;
	GameType *gametype = ship->getGame()->getGameType();
   F32 minDistance = F32_MAX;
   F32 minDistance2 = F32_MAX;
   Vector<DatabaseObject *> objects;
   Point pos = ship->getActualPos();
   Rect queryRect(pos, pos);
   queryRect.expand(gServerGame->computePlayerVisArea(ship));
   gServerGame->getGridDatabase()->findObjects(ShipType,objects,queryRect);
   for(S32 i=0; i<objects.size(); i++)
   {
      Ship *foundship = dynamic_cast<Ship *>(objects[i]);
      if(foundship != NULL)
      if(!gametype->isTeamGame() || foundship->getTeam() != ship->getTeam())
      {
         F32 newDist = pos.distanceTo(foundship->getActualPos());
         if(newDist < minDistance)
         {
				if(newDist < minDistance2)
				{
               minDistance2 = newDist;
					(*shipInVisibleArea) = foundship;
				}
            if(shipAttackable)
				{
					if(gametype->getGridDatabase()->pointCanSeePoint(pos, foundship->getActualPos()))
					{
					   minDistance = newDist;
					   (*shipAttackable) = foundship;
					}
				}
				else
					minDistance = minDistance2;
         }
      }
   }

}
bool attackShip(Ship *ship, Ship *enemyship)
{
   WeaponInfo weap = gWeapons[ship->getSelectedWeapon()];    // Robot's active weapon
   F32 interceptAngle;
   if(calcInterceptCourse(enemyship, ship->getActualPos(), ship->getRadius(), ship->getTeam(), weap.projVelocity, weap.projLiveTime, false, interceptAngle))
	{
		Move move = ship->getCurrentMove();
      move.angle = interceptAngle;
		move.fire = true;
		ship->setCurrentMove(move);
		return true;
	}
	return false;
}


void setMoveThrust(Move &move, F32 vel, F32 ang){
   move.up = sin(ang) <= 0 ? -vel * sin(ang) : 0;
   move.down = sin(ang) > 0 ? vel * sin(ang) : 0;
   move.right = cos(ang) >= 0 ? vel * cos(ang) : 0;
   move.left = cos(ang) < 0 ? -vel * cos(ang) : 0;
}


RobotController::RobotController()
{
}

// Currently does not go anywhere, all it does is fire at enemies.
void RobotController::run(Ship *ship)
{
   Move move = ship->getCurrentMove();
   move.fire = false;
	move.left = 0;
	move.up = 0;
	move.right = 0;
	move.down = 0;
   ship->setCurrentMove(move);

	if(ship->getGame() == NULL)
		return;
	GameType *gametype = ship->getGame()->getGameType();
	if(gametype == NULL)
		return;

   Ship *enemyship = NULL;
   Ship *enemyship2 = NULL;
	findNearestEnemyShip(ship, &enemyship2, &enemyship);
   if(enemyship != NULL)
		attackShip(ship, enemyship);
   if(enemyship2 != NULL)
		mPrevTarget = enemyship2->getActualPos();

	Point gotoPoint = mPrevTarget;
	if(getWaypoint(gotoPoint, ship, &mFlightPlan, &mFlightPlanTo))
	{
		move = ship->getCurrentMove();
		setMoveThrust(move, 1, ship->getActualPos().angleTo(gotoPoint));
		ship->setCurrentMove(move);
	}
}

}