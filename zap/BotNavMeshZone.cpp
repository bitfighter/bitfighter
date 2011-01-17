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
#include "UIGame.h"           // for access to gGameUserInterface.mDebugShowMeshZones
#include "gameObjectRender.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(BotNavMeshZone);


Vector<SafePtr<BotNavMeshZone> > gBotNavMeshZones;     // List of all our zones

// Constructor
BotNavMeshZone::BotNavMeshZone()
{
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
      gBotNavMeshZones.erase(i);
      break;
   }
}


// Return the center of this zone
Point BotNavMeshZone::getCenter()
{
   return getExtent().getCenter();     // Good enough for government work
}


void BotNavMeshZone::render(S32 layerIndex)    
{
   if(!gGameUserInterface.mDebugShowMeshZones)
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


extern bool isConvex(const Vector<Point> &verts);

// Create objects from parameters stored in level file
bool BotNavMeshZone::processArguments(S32 argc, const char **argv)
{
   if(argc < 6)
      return false;

   processPolyBounds(argc, argv, 0, getGame()->getGridSize());
   computeExtent();  // Not needed?
   mConvex = isConvex(mPolyBounds);

   return true;
}


void BotNavMeshZone::onAddedToGame(Game *theGame)
{
   //if(!isGhost())     // For now, so we can see them on the client to help with debugging
   //   if(gBotNavMeshZones.size() < 50) // avoid start-up very long black screen.    // Causes items to stream in 
   //   setScopeAlways();

   // Don't need to increment our objectloaded counter, as this object resides only on the server

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
   for(U32 i = 0; i < size; i++)
   {
      NeighboringZone n;

      Point p1;
      stream->read(&p1.x);
      stream->read(&p1.y);
   
      Point p2;
      stream->read(&p2.x);
      stream->read(&p2.y);

      n.borderStart = p1;
      n.borderEnd = p2;
      mNeighbors.push_back(n);
     // mNeighborRenderPoints.push_back(Border(p1, p2));

   }
}



S32 findZoneContaining(const Vector<SafePtr<BotNavMeshZone> > &zones, const Point &p)
{
   for(S32 i = 0; i < zones.size(); i++)
   {
      // First a quick, crude elimination check then more comprehensive one
      // Since our zones are convex, we can use the faster method!  Yay!
      // Actually, we can't, as it is not reliable... reverting to more comprehensive (and working) version.
      if( zones[i]->getExtent().contains(p) 
                        && (PolygonContains2(zones[i]->mPolyBounds.address(), zones[i]->mPolyBounds.size(), p)) )
         return i;
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


//F32 gridsize1;
static const S32 MAX_ZONES = 10000;     // Don't make this go above S16 max - 1 (32,766)
S32 makeZonesCount = 0;  // -1 wnen new level loads and need to auto generate on findPath

void makeBotMeshZone2(F32 x1,F32 y1,F32 x2,F32 y2)
{
	if(makeZonesCount >= MAX_ZONES) return;   // don't add too many zones...
	GridDatabase *gb = gServerGame->getGridDatabase();
	bool canseeX = gb->pointCanSeePoint(Point(x1,y1),Point(x2,y1)) && gb->pointCanSeePoint(Point(x1,y2),Point(x2,y2));
	bool canseeY = gb->pointCanSeePoint(Point(x1,y1),Point(x1,y2)) && gb->pointCanSeePoint(Point(x2,y1),Point(x2,y2));
	if(canseeX && canseeY
		&& gb->pointCanSeePoint(Point(x1,y1),Point(x2,y2))
		&& gb->pointCanSeePoint(Point(x1,y2),Point(x2,y1))
		)
	{
		BotNavMeshZone *botzone = new BotNavMeshZone();
		botzone->addToGame(gServerGame);
		botzone->mPolyBounds.push_back(Point(x1,y1));
		botzone->mPolyBounds.push_back(Point(x2,y1));
		botzone->mPolyBounds.push_back(Point(x2,y2));
		botzone->mPolyBounds.push_back(Point(x1,y2));
		botzone->mCentroid = Point((x1+x2)/2,(y1+y2)/2);
		botzone->computeExtent();
		makeZonesCount++;
	}
	else
	{
		if(canseeX && canseeY) { canseeX = false; canseeY = false; };
		if(x2-x1 >= 35 && (x2-x1 > y2-y1 || canseeY) && !canseeX)
		{
			F32 x3 = (x1+x2)/2;
			makeBotMeshZone2(x1,y1,x3,y2);
			makeBotMeshZone2(x3,y1,x2,y2);
		}
		else if(y2-y1 >= 35 && !canseeY)
		{
			F32 y3 = (y1+y2)/2;
			makeBotMeshZone2(x1,y1,x2,y3);
			makeBotMeshZone2(x1,y3,x2,y2);
		}
	}
}

//extern Rect gServerWorldBounds;  // in main.cpp

void makeBotMeshZone()
{
	makeZonesCount = 0;
	Rect rect = gServerGame->computeWorldObjectExtents();
	makeBotMeshZone2(rect.min.x,rect.min.y,rect.max.x,rect.max.y);
	//gServerGame->computeWorldObjectExtents();
}


// Only runs on server
void BotNavMeshZone::buildBotNavMeshZoneConnections()    
{
   if(gBotNavMeshZones.size() == 0)
   {
      makeZonesCount = -1;
      return;
   }

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

         Point bordStart, bordEnd;

         if(zonesTouch(gBotNavMeshZones[i]->mPolyBounds, gBotNavMeshZones[j]->mPolyBounds, 1.0, bordStart, bordEnd))
         {
            Point bordCen = Rect(bordStart, bordEnd).getCenter();

            NeighboringZone n1;
            n1.zoneID = j;
            n1.borderStart.set(bordStart);
            n1.borderEnd.set(bordEnd);
            n1.borderCenter.set(bordCen);

            n1.distTo = gBotNavMeshZones[i]->getExtent().getCenter().distanceTo(bordCen);     // Whew!
            n1.center.set(gBotNavMeshZones[j]->getCenter());
            gBotNavMeshZones[i]->mNeighbors.push_back(n1);

            NeighboringZone n2;
            n2.zoneID = i;
            n2.borderStart.set(bordStart);
            n2.borderEnd.set(bordEnd);
            n2.borderCenter.set(bordCen);

            n2.distTo = gBotNavMeshZones[j]->getExtent().getCenter().distanceTo(bordCen);     
            n2.center.set(gBotNavMeshZones[i]->getCenter());
            gBotNavMeshZones[j]->mNeighbors.push_back(n2);
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
		   path.push_back(findGateway(zone, parentZones[zone]));

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


