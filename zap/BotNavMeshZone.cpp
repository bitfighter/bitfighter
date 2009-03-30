//----------------------------------------------------------------------------------
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

#include "BotNavMeshZone.h"
#include "SweptEllipsoid.h"
#include "Robot.h"
#include "UIMenus.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(BotNavMeshZone);

extern S32 gMaxPolygonPoints;


Vector<SafePtr<BotNavMeshZone> > gBotNavMeshZones;     // List of all our zones


// Constructor
BotNavMeshZone::BotNavMeshZone()
{
   mObjectTypeMask = BotNavMeshZoneType | CommandMapVisType;
   mNetFlags.set(Ghostable);    // For now, so we can see them on the client to help with debugging
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

void BotNavMeshZone::render()     // For now... in future will be invisible!
{
   glEnable(GL_BLEND);
      F32 alpha = 0.5;
   glColor3f(.2, .2, 0);

   // Render loadout zone trinagle geometry
   for(S32 i = 0; i < mPolyFill.size(); i+=3)
   {
      glBegin(GL_POLYGON);
      for(S32 j = i; j < i + 3; j++)
         glVertex2f(mPolyFill[j].x, mPolyFill[j].y);
      glEnd();
   }

      glColor3f(.7, .7, 0);
   glBegin(GL_LINE_LOOP);
      for(S32 i = 0; i < mPolyBounds.size(); i++)
         glVertex2f(mPolyBounds[i].x, mPolyBounds[i].y);
   glEnd();

   Rect extents = this->getExtent();
   Point center = extents.getCenter();

   glPushMatrix();
   glTranslatef(center.x, center.y, 0);
      glColor3f(1,1,0);
      char buf[24];
      dSprintf(buf, 24, "ZONE %d (tchs %d)", mZoneID, mNeighbors.size() );
      renderCenteredString(Point(0,0), 25, buf);
   glPopMatrix();

   glColor3f(1,0,0);
   for(S32 i = 0; i < mNeighbors.size(); i++)
   {
      glBegin(GL_LINES);
      glVertex2f(mNeighbors[i].borderStart.x, mNeighbors[i].borderStart.y);
      glVertex2f(mNeighbors[i].borderEnd.x, mNeighbors[i].borderEnd.y);
      glEnd();
   }

   glDisable(GL_BLEND);
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

// Create objects from parameters stored in level file
bool BotNavMeshZone::processArguments(S32 argc, const char **argv)
{
   if(argc < 6)
      return false;

   processPolyBounds(argc, argv, 0, mPolyBounds);
   computeExtent();  // Not needed?
   return true;
}


void BotNavMeshZone::onAddedToGame(Game *theGame)
{
   if(!isGhost())     // For now, so we can see them on the client to help with debugging
      setScopeAlways();

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
bool BotNavMeshZone::getCollisionPoly(U32 state, Vector<Point> &polyPoints)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      polyPoints.push_back(mPolyBounds[i]);
   return true;
}


   // These methods will be empty later...
 U32 BotNavMeshZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   stream->writeInt(mZoneID, 16);
   stream->writeEnum(mPolyBounds.size(), gMaxPolygonPoints);
   for(S32 i = 0; i < mPolyBounds.size(); i++)
   {
      stream->write(mPolyBounds[i].x);
      stream->write(mPolyBounds[i].y);
   }

   stream->writeEnum(mNeighbors.size(), 15);
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
   U32 size = stream->readEnum(gMaxPolygonPoints);
   for(U32 i = 0; i < size; i++)
   {
      Point p;
      stream->read(&p.x);
      stream->read(&p.y);
      mPolyBounds.push_back(p);
   }
   if(size)
   {
      computeExtent();
      Triangulate::Process(mPolyBounds, mPolyFill);
   }

   size = stream->readEnum(15);
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
   }

}

extern bool PolygonContains(const Point *inVertices, int inNumVertices, const Point &inPoint);

S32 findZoneContaining(Point p)
{
   for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
   {
      TNLAssert( gBotNavMeshZones[i]->mZoneID == i,"LLLLL"); 
      // First a quick, crude elimination check then more comprehensive one
      // Since our zones are convex, we can use the faster method!  Yay!
      // Actually, we can't, as it is not reliable... reverting to more comprehensive (and working) version.
      if( gBotNavMeshZones[i]->getExtent().contains(p) 
                        && (PolygonContains2(gBotNavMeshZones[i]->mPolyBounds.address(), gBotNavMeshZones[i]->mPolyBounds.size(), p)) )
         return i;
   }
   return -1;
}


extern bool segmentsCoincident(Point p1, Point p2, Point p3, Point p4);

void testBotNavMeshZoneConnections()
{
   return;

   for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
      for(S32 j = 0; j < gBotNavMeshZones.size(); j++)
      {
         Vector<S32> path = AStar::findPath(i, j);
         for(S32 k = 0; k < path.size(); k++)
            logprintf("Path %d from %d to %d: %d", k, i, j, path[k]);
         logprintf("");
      }
}


// Returns index of neighboring zone, or -1 if zone is not a neighbor
S32 BotNavMeshZone::getNeighborIndex(S32 zoneID)
{
   for(S32 i = 0; i < mNeighbors.size(); i++)
      if(mNeighbors[i].zoneID == zoneID)
         return i;

   return -1;
}


void BotNavMeshZone::buildBotNavMeshZoneConnections()
{
   for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
   {
      for(S32 j = i; j < gBotNavMeshZones.size(); j++)
      {
         if(i == j)
            continue;      // Don't check self...

         // Do zones i and j touch?  First a quick and dirty bounds check:
         if(!gBotNavMeshZones[i]->getExtent().intersectsOrBorders(gBotNavMeshZones[j]->getExtent()))
            continue;

         // Check for unlikely but fatal situation: Not enough vertices
          if(gBotNavMeshZones[i]->mPolyBounds.size() < 3 || gBotNavMeshZones[j]->mPolyBounds.size() < 3)
          {
             logprintf("ERROR!! Too few pts!");
            continue;
          }

         Point pi1, pi2, pj1, pj2;
         bool found = false;

         // Now, do we actually touch?  Let's look, segment by segment
         for(S32 ii = 0; !found && ii < gBotNavMeshZones[i]->mPolyBounds.size(); ii++)
         {
            pi1 = gBotNavMeshZones[i]->mPolyBounds[ii];
            if(ii == gBotNavMeshZones[i]->mPolyBounds.size() - 1)
               pi2 = gBotNavMeshZones[i]->mPolyBounds[0];
            else
               pi2 = gBotNavMeshZones[i]->mPolyBounds[ii + 1];

            for(S32 jj = 0; !found && jj < gBotNavMeshZones[j]->mPolyBounds.size(); jj++)
            {
               pj1 = gBotNavMeshZones[j]->mPolyBounds[jj];
               if(jj == gBotNavMeshZones[j]->mPolyBounds.size() - 1)
                  pj2 = gBotNavMeshZones[j]->mPolyBounds[0];
               else
                  pj2 = gBotNavMeshZones[j]->mPolyBounds[jj + 1];

               Point bordStart, bordEnd;

               if(segsOverlap(pi1, pi2, pj1, pj2, bordStart, bordEnd))
               {
                  Point bordCen = Rect(bordStart, bordEnd).getCenter();

                  NeighboringZone n1;
                  n1.zoneID = j;
                  n1.borderStart = bordStart;
                  n1.borderEnd = bordEnd;
                  n1.borderCenter = bordCen;

                  n1.distTo = gBotNavMeshZones[i]->getExtent().getCenter().distanceTo(bordCen);     // Whew!
                  n1.center = gBotNavMeshZones[j]->getCenter();
                  gBotNavMeshZones[i]->mNeighbors.push_back(n1);

                  NeighboringZone n2;
                  n2.zoneID = i;
                  n2.borderStart = bordStart;
                  n2.borderEnd = bordEnd;
                  n2.borderCenter = bordCen;

                  n2.distTo = gBotNavMeshZones[j]->getExtent().getCenter().distanceTo(bordCen);     
                  n2.center = gBotNavMeshZones[i]->getCenter();
                  gBotNavMeshZones[j]->mNeighbors.push_back(n2);

                  found = true;
               }
            }
         }
      }
   }
}


// Rough guess as to distance from fromZone to toZone
F32 AStar::heuristic(S32 fromZone, S32 toZone)
{
   return gBotNavMeshZones[fromZone]->getCenter().distanceTo( gBotNavMeshZones[toZone]->getCenter() ) * 1.2;
}

const S32 gMaxNavMeshZones = 2000;     // Don't make this go above S16 max

// Returns a path, including the startZone and targetZone 
Vector<S32> AStar::findPath(S32 startZone, S32 targetZone)
{   
   // Because of these variables...
   static U16 onClosedList = 0;
   static U16 onOpenList;

   // ...these arrays can be reused without further initialization
   static U16 whichList[gMaxNavMeshZones];  // Record whether a zone is on the open or closed list
   static S16 openList[gMaxNavMeshZones + 1]; 
   static S16 openZone[gMaxNavMeshZones]; 
   static S16 parentZones[gMaxNavMeshZones]; 

   static F32 Fcost[gMaxNavMeshZones];	
   static F32 Gcost[gMaxNavMeshZones]; 	
   static F32 Hcost[gMaxNavMeshZones];	

	S16 numberOfOpenListItems = 0;
	bool foundPath;

	S32 newOpenListItemID = 0;         // Used for creating new IDs for zones to make heap work

   Vector<S32> pathZones;

   // This block here lets us repeatedly reuse the whichList array without resetting it or recreating it
   // which, for larger numbers of zones should be a real time saver.  It's not clear if it is particularly
   // more efficient for the zone counts we typically see in Bitfighter levels.

   if(onClosedList > U16_MAX - 3 ) // Reset whichList when we've run out of headroom
	{
		for(S32 i = 0; i < gMaxNavMeshZones; i++) 
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
            if(whichList[zoneID] != onOpenList) 
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
               S32 tempGcost = Gcost[parentZone] + zone.distTo;
         		
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

	   S32 path = targetZone; 
      pathZones.push_back(path);

	   while(path != startZone)
	   {
		   // Find the parent of the current cell	
		   path = parentZones[path];		
         pathZones.push_back(path);
	   }

	   return pathZones;
	}

   // else there is no path to the selected target.
   pathZones.push_back(-1);
	return pathZones;
}



};

