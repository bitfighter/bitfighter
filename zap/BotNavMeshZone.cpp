//----------------------------------------------------------------------------------
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

#include "BotNavMeshZone.h"
#include "SweptEllipsoid.h"
#include "robot.h"
#include "UIMenus.h"
#include "UIGame.h"           // for access to mGameUserInterface.mDebugShowMeshZones
#include "gameObjectRender.h"
#include "teleporter.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(BotNavMeshZone);


Vector<SafePtr<BotNavMeshZone> > gBotNavMeshZones;     // List of all our zones

// Constructor
BotNavMeshZone::BotNavMeshZone()
{
   mGame = NULL;
   mObjectTypeMask = BotNavMeshZoneType | CommandMapVisType;
   //  The Zones is now rendered without the network interface, if client is hosting.
   //mNetFlags.set(Ghostable);    // For now, so we can see them on the client to help with debugging... when too many zones causes huge lag
   mZoneID = gBotNavMeshZones.size();

   gBotNavMeshZones.push_back(this);
}


// Destructor
BotNavMeshZone::~BotNavMeshZone()
{
   for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
   if(gBotNavMeshZones[i] == this)
   {
      gBotNavMeshZones.erase_fast(i);
      break;
   }
   if(mGame)
   {
      removeFromDatabase();
      mGame = NULL;
   }
}


// Return the center of this zone
Point BotNavMeshZone::getCenter()
{
   return getExtent().getCenter();     // Good enough for government work
}


void BotNavMeshZone::render(S32 layerIndex)    
{
   if(!gClientGame->mGameUserInterface->mDebugShowMeshZones)
      return;

   if(mPolyFill.size() == 0)    //Need to process PolyFill here, rendering server objects into client.
         Triangulate::Process(mPolyBounds, mPolyFill);

   if(layerIndex == 0)
      renderNavMeshZone(mPolyBounds, mPolyFill, mCentroid, mZoneID, mConvex);
   else if(layerIndex == 1)
      renderNavMeshBorders(mNeighbors);
}


// Use this to help keep track of which robots are where
// Only gets run on the server, never on client, obviously, because that's where the bots are!!!
bool BotNavMeshZone::collide(GameObject *hitObject)
{
   // This does not get run anymore, it is in a seperate database.
   if(hitObject->getObjectTypeMask() & RobotType)     // Only care about robots...
   {
      Robot *r = (Robot *) hitObject;
      r->setCurrentZone(mZoneID);
   }
   return false;
}


S32 BotNavMeshZone::getRenderSortValue()
{
   return -2;
}

GridDatabase * BotNavMeshZone::getGridDatabase()
{
   return &mGame->mDatabaseForBotZones;
}


extern bool isConvex(const Vector<Point> &verts);

// Create objects from parameters stored in level file
bool BotNavMeshZone::processArguments(S32 argc, const char **argv)
{
   if(argc < 6)
      return false;

   processPolyBounds(argc, argv, 0, mGame->getGridSize());
   computeExtent();  // Not needed?
   mConvex = isConvex(mPolyBounds);

   return true;
}


void BotNavMeshZone::addToGame(Game *game)
{
   // Don't add to game
   mGame = game;
   addToDatabase();
}


void BotNavMeshZone::onAddedToGame(Game *theGame)
{
   //if(!isGhost())     // For now, so we can see them on the client to help with debugging
   //   if(gBotNavMeshZones.size() < 50) // avoid start-up very long black screen.    // Causes items to stream in 
   //   setScopeAlways();

   // Don't need to increment our objectloaded counter, as this object resides only on the server
   TNLAssert(false, "Should not be added to game");
}


// Bounding box for quick collision-possibility elimination
void BotNavMeshZone::computeExtent()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);

   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);

   setExtent(extent);
}


// More precise boundary for precise collision detection
bool BotNavMeshZone::getCollisionPoly(Vector<Point> &polyPoints)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      polyPoints.push_back(mPolyBounds[i]);

   return true;
}


// These methods will be empty later...
 U32 BotNavMeshZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   stream->writeInt(mZoneID, 16);

   Polygon::packUpdate(connection, stream);

   stream->writeInt(mNeighbors.size(), 8);

   for(S32 i = 0; i < mNeighbors.size(); i++)
   {
      stream->write(mNeighbors[i].borderStart.x);
      stream->write(mNeighbors[i].borderStart.y);

      stream->write(mNeighbors[i].borderEnd.x);
      stream->write(mNeighbors[i].borderEnd.y);
   }

   return 0;
}


void BotNavMeshZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   mZoneID = stream->readInt(16);

   if(Polygon::unpackUpdate(connection, stream))
   {
      computeExtent();
      mConvex = isConvex(mPolyBounds);
   }

   U32 size = stream->readInt(8);
   Point p1, p2;

   for(U32 i = 0; i < size; i++)
   {
      NeighboringZone n;

      stream->read(&p1.x);
      stream->read(&p1.y);
   
      stream->read(&p2.x);
      stream->read(&p2.y);

      n.borderStart = p1;
      n.borderEnd = p2;
      mNeighbors.push_back(n);
     // mNeighborRenderPoints.push_back(Border(p1, p2));
   }
}


S32 findZoneContaining(const Point &p)
{
   Vector<DatabaseObject *> fillVector;
   gServerGame->mDatabaseForBotZones.findObjects(BotNavMeshZoneType, fillVector, Rect(p - Point(0.1f,0.1f),p + Point(0.1f,0.1f)));  // slightly extend Rect, it can be on the edge of zone.

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      // First a quick, crude elimination check then more comprehensive one
      // Since our zones are convex, we can use the faster method!  Yay!
      // Actually, we can't, as it is not reliable... reverting to more comprehensive (and working) version.
      BotNavMeshZone *zone = dynamic_cast<BotNavMeshZone *>(fillVector[i]);
      TNLAssert(zone, "NULL zone in findZoneContaining");
      if( zone->getExtent().contains(p) 
                        && (PolygonContains2(zone->mPolyBounds.address(), zone->mPolyBounds.size(), p)) )
         return zone->mZoneID;
   }
   return -1;
}


extern bool segmentsCoincident(Point p1, Point p2, Point p3, Point p4);

void testBotNavMeshZoneConnections()
{
   return;

   //for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
   //   for(S32 j = 0; j < gBotNavMeshZones.size(); j++)
   //   {
   //      Vector<S32> path = AStar::findPath(i, j);
   //      for(S32 k = 0; k < path.size(); k++)
   //         logprintf("Path %d from %d to %d: %d", k, i, j, path[k]);
   //      logprintf("");
   //   }
}


// Returns index of neighboring zone, or -1 if zone is not a neighbor
S32 BotNavMeshZone::getNeighborIndex(S32 zoneID)
{
   for(S32 i = 0; i < mNeighbors.size(); i++)
      if(mNeighbors[i].zoneID == zoneID)
         return i;

   return -1;
}


static const S32 MAX_ZONES = 10000;     // Don't make this go above S16 max - 1 (32,766), AStar::findPath is limited.
const F32 MinZoneSize = 32;

static void makeBotMeshZone(F32 x1, F32 y1, F32 x2, F32 y2)
{
	if(gBotNavMeshZones.size() >= MAX_ZONES)      // Don't add too many zones...
      return;   

	GridDatabase *gb = gServerGame->getGridDatabase();

	bool canseeX = gb->pointCanSeePoint(Point(x1, y1), Point(x2, y1)) && gb->pointCanSeePoint(Point(x1, y2), Point(x2, y2));
	bool canseeY = gb->pointCanSeePoint(Point(x1, y1), Point(x1, y2)) && gb->pointCanSeePoint(Point(x2, y1), Point(x2, y2));
	bool canseeD = gb->pointCanSeePoint(Point(x1, y1), Point(x2, y2)) && gb->pointCanSeePoint(Point(x1, y2), Point(x2, y1));

	if(canseeX && canseeY && canseeD)
	{
		BotNavMeshZone *botzone = new BotNavMeshZone();

		botzone->mPolyBounds.push_back(Point(x1, y1));
		botzone->mPolyBounds.push_back(Point(x2, y1));
		botzone->mPolyBounds.push_back(Point(x2, y2));
		botzone->mPolyBounds.push_back(Point(x1, y2));
		botzone->mCentroid = Point((x1 + x2) / 2, (y1 + y2) / 2);
		botzone->mConvex = true;             // avoid random red and green on /dzones, was uninitalized.
		botzone->addToGame(gServerGame);
		botzone->computeExtent();
	}
	else
	{
		if((canseeX && canseeY) || canseeD) 
      { 
         canseeX = false; 
         canseeY = false; 
      };

		if(!canseeX && x2 - x1 >= MinZoneSize && (canseeY || x2 - x1 > y2 - y1))
		{
			F32 x3 = (x1 + x2) / 2;
			makeBotMeshZone(x1, y1, x3, y2);
			makeBotMeshZone(x3, y1, x2, y2);
		}
		else if(!canseeY && y2 - y1 >= MinZoneSize)
		{
			F32 y3 = (y1 + y2)/2;
			makeBotMeshZone(x1, y1, x2, y3);
			makeBotMeshZone(x1, y3, x2, y2);
		}
	}
}


// In future, this function will store generated zones in a cache... but not yet; now it's just buildBotMeshZones!
void BotNavMeshZone::buildOrLoadBotMeshZones()
{
	Rect bounds = gServerGame->computeWorldObjectExtents();
	makeBotMeshZone(bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y);
}


Vector<DatabaseObject *> zones;
extern Rect gServerWorldBounds;

// Returns index of zone containing specified point
static BotNavMeshZone *findZoneContainingPoint(const Point &point)
{
   Rect rect(point, 0.01f);
   zones.clear();
   gServerGame->mDatabaseForBotZones.findObjects(BotNavMeshZoneType, zones, rect); 

   // If there is more than one possible match, pick the first arbitrarily (could happen if dest is right on a zone border)
   for(S32 i = 0; i < zones.size(); i++)
   {
      BotNavMeshZone *zone = dynamic_cast<BotNavMeshZone *>(zones[i]);     
      if(zone)
         return zone;
   }

   return NULL;
}


// Only runs on server
void BotNavMeshZone::buildBotNavMeshZoneConnections()    
{
   if(gBotNavMeshZones.size() == 0)
      return;

   // We'll reuse these objects throughout the following block, saving the cost of creating and destructing them
   Point bordStart, bordEnd, bordCen;
   Rect rect;
   NeighboringZone neighbor;
   Vector<DatabaseObject *> objects;

   // Figure out which zones are adjacent to which, and find the "gateway" between them
   for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
   {
      for(S32 j = i; j < gBotNavMeshZones.size(); j++)
      {
         if(i == j)
            continue;      // Don't check self...

         // Do zones i and j touch?  First a quick and dirty bounds check:
         if(!gBotNavMeshZones[i]->getExtent().intersectsOrBorders(gBotNavMeshZones[j]->getExtent()))
            continue;

         if(zonesTouch(gBotNavMeshZones[i]->mPolyBounds, gBotNavMeshZones[j]->mPolyBounds, 1.0, bordStart, bordEnd))
         {
            rect.set(bordStart, bordEnd);
            bordCen.set(rect.getCenter());

            // Zone j is a neighbor of i
            neighbor.zoneID = j;
            neighbor.borderStart.set(bordStart);
            neighbor.borderEnd.set(bordEnd);
            neighbor.borderCenter.set(bordCen);

            neighbor.distTo = gBotNavMeshZones[i]->getExtent().getCenter().distanceTo(bordCen);     // Whew!
            neighbor.center.set(gBotNavMeshZones[j]->getCenter());
            gBotNavMeshZones[i]->mNeighbors.push_back(neighbor);

            // Zone i is a neighbor of j
            neighbor.zoneID = i;
            neighbor.borderStart.set(bordStart);
            neighbor.borderEnd.set(bordEnd);
            neighbor.borderCenter.set(bordCen);

            neighbor.distTo = gBotNavMeshZones[j]->getExtent().getCenter().distanceTo(bordCen);     
            neighbor.center.set(gBotNavMeshZones[i]->getCenter());
            gBotNavMeshZones[j]->mNeighbors.push_back(neighbor);
         }
      }
   }
		
   // Now create paths representing the teleporters
   Vector<DatabaseObject *> teleporters, dests;

	gServerGame->getGridDatabase()->findObjects(TeleportType, teleporters, gServerWorldBounds);

	for(S32 i = 0; i < teleporters.size(); i++)
	{
		Teleporter *teleporter = dynamic_cast<Teleporter *>(teleporters[i]);

      if(!teleporter)
         continue;

      BotNavMeshZone *origZone = findZoneContainingPoint(teleporter->getActualPos());

		for(S32 j = 0; j < teleporter->mDest.size(); j++)     // Review each teleporter destination
		{
         BotNavMeshZone *destZone = findZoneContainingPoint(teleporter->mDest[j]);

			if(origZone != destZone)      // Ignore teleporters that begin and end in the same zone
			{
				// Teleporter is one way path
				neighbor.zoneID = destZone->mZoneID;
				neighbor.borderStart.set(teleporter->getActualPos());
				neighbor.borderEnd.set(teleporter->mDest[j]);
				neighbor.borderCenter.set(teleporter->getActualPos());

            // Teleport instantly, at no cost -- except this is wrong... if teleporter has multiple dests, actual cost could be quite high.
            // This should be the average of the costs of traveling from each dest zone to the target zone
				neighbor.distTo = 0;                                    
				neighbor.center.set(teleporter->getActualPos());

				origZone->mNeighbors.push_back(neighbor);
			}
		}
   }
}


// Rough guess as to distance from fromZone to toZone
F32 AStar::heuristic(S32 fromZone, S32 toZone)
{
   return gBotNavMeshZones[fromZone]->getCenter().distanceTo( gBotNavMeshZones[toZone]->getCenter() );
}


// Returns a path, including the startZone and targetZone 
Vector<Point> AStar::findPath(S32 startZone, S32 targetZone, const Point &target)
{
   // Because of these variables...
   static U16 onClosedList = 0;
   static U16 onOpenList;

   // ...these arrays can be reused without further initialization
   static U16 whichList[MAX_ZONES];  // Record whether a zone is on the open or closed list
   static S16 openList[MAX_ZONES + 1]; 
   static S16 openZone[MAX_ZONES]; 
   static S16 parentZones[MAX_ZONES]; 

   static F32 Fcost[MAX_ZONES];	
   static F32 Gcost[MAX_ZONES]; 	
   static F32 Hcost[MAX_ZONES];	

	S16 numberOfOpenListItems = 0;
	bool foundPath;

	S32 newOpenListItemID = 0;         // Used for creating new IDs for zones to make heap work

   Vector<Point> path;


   // This block here lets us repeatedly reuse the whichList array without resetting it or recreating it
   // which, for larger numbers of zones should be a real time saver.  It's not clear if it is particularly
   // more efficient for the zone counts we typically see in Bitfighter levels.

   if(onClosedList > U16_MAX - 3 ) // Reset whichList when we've run out of headroom
	{
		for(S32 i = 0; i < MAX_ZONES; i++) 
		   whichList[i] = 0;
		onClosedList = 0;	
	}
	onClosedList = onClosedList + 2; // Changing the values of onOpenList and onClosed list is faster than redimming whichList() array
	onOpenList = onClosedList - 1;

	Gcost[startZone] = 0;         // That's the cost of going from the startZone to the startZone!
   Fcost[0] = Hcost[0] = heuristic(startZone, targetZone);

	numberOfOpenListItems = 1;    // Start with one open item: the startZone

	openList[1] = 0;              // Start with 1 item in the open list (must be index 1), which is maintained as a binary heap
	openZone[0] = startZone;

   // Loop until a path is found or deemed nonexistent.
	while(true)
	{
	   if(numberOfOpenListItems == 0)      // List is empty, we're done
	   {
		   foundPath = false; 
         break;
	   }  
      else
	   {
         // The open list is not empty, so take the first cell off of the list.
         //	Since the list is a binary heap, this will be the lowest F cost cell on the open list.
	      S32 parentZone = openZone[openList[1]];

         if(parentZone == targetZone)
         {
            foundPath = true; 
            break;
         }

         whichList[parentZone] = onClosedList;   // add the item to the closed list
         numberOfOpenListItems--;	

         //	Open List = Binary Heap: Delete this item from the open list, which
         // is maintained as a binary heap. For more information on binary heaps, see:
         //	http://www.policyalmanac.org/games/binaryHeaps.htm
      		
         //	Delete the top item in binary heap and reorder the heap, with the lowest F cost item rising to the top.
	      openList[1] = openList[numberOfOpenListItems + 1];   // Move the last item in the heap up to slot #1
	      S16 v = 1; 

         //	Loop until the new item in slot #1 sinks to its proper spot in the heap.
	      while(true) // ***
	      {
	         S16 u = v;		
	         if (2 * u + 1 < numberOfOpenListItems) // if both children exist
	         {
	 	         //Check if the F cost of the parent is greater than each child.
		         //Select the lowest of the two children.
		         if(Fcost[openList[u]] >= Fcost[openList[2*u]]) 
			         v = 2 * u;
		         if(Fcost[openList[v]] >= Fcost[openList[2*u+1]]) 
			         v = 2 * u + 1;		
	         }
	         else if (2 * u < numberOfOpenListItems) // if only child (#1) exists
	         {
 	            // Check if the F cost of the parent is greater than child #1	
		         if(Fcost[openList[u]] >= Fcost[openList[2*u]]) 
			         v = 2 * u;
	         }

	         if(u != v) // If parent's F is > one of its children, swap them...
	         {
               S16 temp = openList[u];
               openList[u] = openList[v];
               openList[v] = temp;			
	         }
	         else
		         break; // ...otherwise, exit loop
	      } // ***

         // Check the adjacent zones. (Its "children" -- these path children
         //	are similar, conceptually, to the binary heap children mentioned
         //	above, but don't confuse them. They are different. 
         // Add these adjacent child squares to the open list
         //	for later consideration if appropriate.

         Vector<NeighboringZone> neighboringZones = gBotNavMeshZones[parentZone]->mNeighbors;

         for(S32 a = 0; a < neighboringZones.size(); a++)
         {
            NeighboringZone zone = neighboringZones[a];
            S32 zoneID = zone.zoneID;

            //	Check if zone is already on the closed list (items on the closed list have
            //	already been considered and can now be ignored).			
            if(whichList[zoneID] == onClosedList) 
               continue;

            //	Add zone to the open list if it's not already on it
            TNLAssert(newOpenListItemID < MAX_ZONES, "Too many nav zones... try increasing MAX_ZONES!");
            if(whichList[zoneID] != onOpenList && newOpenListItemID < MAX_ZONES) 
            {	
               // Create a new open list item in the binary heap
               newOpenListItemID = newOpenListItemID + 1;   // Give each new item a unique id
               S32 m = numberOfOpenListItems + 1;
               openList[m] = newOpenListItemID;             // Place the new open list item (actually, its ID#) at the bottom of the heap
               openZone[newOpenListItemID] = zoneID;        // Record zone as a newly opened

               Hcost[openList[m]] = heuristic(zoneID, targetZone);
               Gcost[zoneID] = Gcost[parentZone] + zone.distTo;
               Fcost[openList[m]] = Gcost[zoneID] + Hcost[openList[m]];
               parentZones[zoneID] = parentZone; 

               // Move the new open list item to the proper place in the binary heap.
               // Starting at the bottom, successively compare to parent items,
               // swapping as needed until the item finds its place in the heap
               // or bubbles all the way to the top (if it has the lowest F cost).
               while(m > 1 && Fcost[openList[m]] <= Fcost[openList[m/2]]) 
               {
                  S16 temp = openList[m/2];
                  openList[m/2] = openList[m];
                  openList[m] = temp;
                  m = m/2;
               }

               // Finally, put zone on the open list
               whichList[zoneID] = onOpenList;
               numberOfOpenListItems++;
            }

            // If zone is already on the open list, check to see if this 
            //	path to that cell from the starting location is a better one. 
            //	If so, change the parent of the cell and its G and F costs.

            else // zone was already on the open list
            {
               // Figure out the G cost of this possible new path
               S32 tempGcost = (S32)(Gcost[parentZone] + zone.distTo);
         		
               // If this path is shorter (G cost is lower) then change
               // the parent cell, G cost and F cost. 		
               if (tempGcost < Gcost[zoneID])
               {
                  parentZones[zoneID] = parentZone; // Change the square's parent
                  Gcost[zoneID] = tempGcost;        // and its G cost			

                  // Because changing the G cost also changes the F cost, if
                  // the item is on the open list we need to change the item's
                  // recorded F cost and its position on the open list to make
                  // sure that we maintain a properly ordered open list.

                  for(S32 i = 1; i <= numberOfOpenListItems; i++) // Look for the item in the heap
                  {
                     if(openZone[openList[i]] == zoneID) 
                     {
	                     Fcost[openList[i]] = Gcost[zoneID] + Hcost[openList[i]]; // Change the F cost
            				
	                     // See if changing the F score bubbles the item up from it's current location in the heap
	                     S32 m = i;
	                     while(m > 1 && Fcost[openList[m]] < Fcost[openList[m/2]]) 
	                     {
		                     S16 temp = openList[m/2];
		                     openList[m/2] = openList[m];
		                     openList[m] = temp;
		                     m = m/2;
	                     }
	                
                        break; 
                     } 
                  }
               }

            } // else if whichList(zoneID) = onOpenList	
         } // for loop looping through neighboring zones list
	   }  

	   // If target is added to open list then path has been found.
	   if(whichList[targetZone] == onOpenList)
	   {
		   foundPath = true; 
         break;
	   }
	}

   // Save the path if it exists.
	if(foundPath)
	{
      // Working backwards from the target to the starting location by checking
      //	each cell's parent, figure out the length of the path.
      // Fortunately, we want our list to have the closest zone last (see getWaypoint),
      // so it all works out nicely.
      // We'll store both the zone center and the gateway to the neighboring zone.  This
      // will help keep the robot from getting hung up on blocked but technically visible
      // paths, such as when we are trying to fly around a protruding wall stub.

      path.push_back(target);                                     // First point is the actual target itself
      path.push_back(gBotNavMeshZones[targetZone]->getCenter());  // Second is the center of the target's zone
      
      S32 zone = targetZone;

	   while(zone != startZone)
	   {
		   path.push_back(findGateway(parentZones[zone], zone));   // don't switch findGateway arguments, some path is one way (teleporters).

		   zone = parentZones[zone];		// Find the parent of the current cell	

         path.push_back(gBotNavMeshZones[zone]->getCenter());
	   }
      path.push_back(gBotNavMeshZones[startZone]->getCenter());

	   return path;
	}

   // else there is no path to the selected target
   TNLAssert(path.size() == 0, "Expected empty path!");
	return path;
}

// Return a point representing gateway between zones
Point AStar::findGateway(S32 zone1, S32 zone2)
{
   S32 neighborIndex = gBotNavMeshZones[zone1]->getNeighborIndex(zone2);
   TNLAssert(neighborIndex >= 0, "Invalid neighbor index!!");

   return gBotNavMeshZones[zone1]->mNeighbors[neighborIndex].borderCenter;
}


};


