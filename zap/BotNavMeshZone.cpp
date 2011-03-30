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
#include "GeomUtils.h"
#include "robot.h"
#include "UIMenus.h"
#include "UIGame.h"           // for access to mGameUserInterface.mDebugShowMeshZones
#include "gameObjectRender.h"
#include "teleporter.h"
#include "barrier.h"             // For Barrier methods in generating zones
#include "../recast/Recast.h"    // For zone generation
#include "../recast/RecastAlloc.h"

#include <vector>


// triangulate wants quit on error
// will probably crash on error when this function exits
//extern "C" void triexit(int status)
//{
//   TNLAssert(false, "Triangulate error");
//   logprintf(LogConsumer::LogError, "While generating bot zones, Triangulate error");
//   // here may be a good place to print lots of data.
//   //throw;  // any better ways?
//}



namespace Zap
{

static const S32 MAX_ZONES = 10000;     // Don't make this go above S16 max - 1 (32,766), AStar::findPath is limited


TNL_IMPLEMENT_NETOBJECT(BotNavMeshZone);


Vector<SafePtr<BotNavMeshZone> > gBotNavMeshZones;     // List of all our zones

// Constructor
BotNavMeshZone::BotNavMeshZone()
{
   mGame = NULL;
   mObjectTypeMask = BotNavMeshZoneType | CommandMapVisType;
   //  The Zones is now rendered without the network interface, if client is hosting.
   //mNetFlags.set(Ghostable);    // For now, so we can see them on the client to help with debugging... when too many zones causes huge lag
   mZoneId = gBotNavMeshZones.size();

   gBotNavMeshZones.push_back(this);
}


// Destructor
BotNavMeshZone::~BotNavMeshZone()
{
   for(S32 i = gBotNavMeshZones.size() - 1; i >= 0; i--)  // For speed, check in reverse order. Game::cleanUp() clears on reverse order.
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

   // TODO: Move to constructor, only doing this when there is a local client?
   if(mPolyFill.size() == 0)    // Need to process PolyFill here, rendering server objects into client.  
      Triangulate::Process(mPolyBounds, mPolyFill);

   if(layerIndex == 0)
      renderNavMeshZone(mPolyBounds, mPolyFill, mCentroid, mZoneId, mConvex);

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
      r->setCurrentZone(mZoneId);
   }
   return false;
}


S32 BotNavMeshZone::getRenderSortValue()
{
   return -2;
}

GridDatabase *BotNavMeshZone::getGridDatabase()
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
   computeExtent();  // Computes extent so we can insert this into the BotNavMesh object database
   mConvex = isConvex(mPolyBounds);

   return true;
}


void BotNavMeshZone::addToGame(Game *game)
{
   // Ordinarily, we'd call GameObject::addToGame() here, but the BotNavMeshZones don't need to be added to the game
   // the way an ordinary game object would be.  So we won't.
   mGame = game;
   addToDatabase();
}


void BotNavMeshZone::onAddedToGame(Game *theGame)
{
   TNLAssert(false, "Should not be added to game");
}


// Bounding box for quick collision-possibility elimination
void BotNavMeshZone::computeExtent()
{
   Rect extent(mPolyBounds);
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
   stream->writeInt(mZoneId, 16);

   Polygon::packUpdate(connection, stream);

   stream->writeInt(mNeighbors.size(), 8);

   for(S32 i = 0; i < mNeighbors.size(); i++)
   {
      stream->write(mNeighbors[i].borderStart.x);
      stream->write(mNeighbors[i].borderStart.y);

      stream->write(mNeighbors[i].borderEnd.x);
      stream->write(mNeighbors[i].borderEnd.y);

      stream->write(mNeighbors[i].borderCenter.x);
      stream->write(mNeighbors[i].borderCenter.y);

      stream->write(mNeighbors[i].zoneID);
      stream->write(mNeighbors[i].distTo);
      stream->write(mNeighbors[i].center.x);
      stream->write(mNeighbors[i].center.y);
   }

   return 0;
}


void BotNavMeshZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   mZoneId = stream->readInt(16);

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
      stream->read(&n.borderCenter.x);
      stream->read(&n.borderCenter.y);
      stream->read(&n.zoneID);
      stream->read(&n.distTo);
      stream->read(&n.center.x);
      stream->read(&n.center.y);
      mNeighbors.push_back(n);
     // mNeighborRenderPoints.push_back(Border(p1, p2));
   }
}


// Returns ID of zone containing specified point
U16 BotNavMeshZone::findZoneContaining(const Point &p)
{
   Vector<DatabaseObject *> fillVector;
   gServerGame->mDatabaseForBotZones.findObjects(BotNavMeshZoneType, fillVector, 
                                                      Rect(p - Point(0.1f,0.1f),p + Point(0.1f,0.1f)));  // Slightly extend Rect, it can be on the edge of zone

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      // First a quick, crude elimination check then more comprehensive one
      // Since our zones are convex, we can use the faster method!  Yay!
      // Actually, we can't, as it is not reliable... reverting to more comprehensive (and working) version.
      BotNavMeshZone *zone = dynamic_cast<BotNavMeshZone *>(fillVector[i]);

      TNLAssert(zone, "NULL zone in findZoneContaining");
      if( zone->getExtent().contains(p) 
                        && (PolygonContains2(zone->mPolyBounds.address(), zone->mPolyBounds.size(), p)) )
         return zone->mZoneId;
   }

   return U16_MAX;
}


// Returns index of neighboring zone, or -1 if zone is not a neighbor
S32 BotNavMeshZone::getNeighborIndex(S32 zoneID)
{
   for(S32 i = 0; i < mNeighbors.size(); i++)
      if(mNeighbors[i].zoneID == zoneID)
         return i;

   return -1;
}


struct rcEdge
{
	unsigned short vert[2];    // from, to verts
	unsigned short poly[2];    // left, right poly
};

// Build connections between zones using the adjacency data created in recast
static bool buildBotNavMeshZoneConnectionsRecastStyle(rcPolyMesh &mesh, const Vector<S32> &polyToZoneMap)    
{
   if(gBotNavMeshZones.size() == 0)
      return true;

   // We'll reuse these objects throughout the following block, saving the cost of creating and destructing them
   Point bordStart, bordEnd, bordCen;
   Rect rect;
   NeighboringZone neighbor;
   Vector<DatabaseObject *> objects;


   /////////////////////////////
   // Based on recast's interpretation of code by Eric Lengyel:
	// http://www.terathon.com/code/edges.php
	
   int maxEdgeCount = mesh.npolys*mesh.nvp;
	unsigned short* firstEdge = (unsigned short*)rcAlloc(sizeof(unsigned short)*(mesh.nverts + maxEdgeCount), RC_ALLOC_TEMP);
	if (!firstEdge)      // Allocation went bad
		return false;
	unsigned short* nextEdge = firstEdge + mesh.nverts;
	int edgeCount = 0;
	
	rcEdge* edges = (rcEdge*)rcAlloc(sizeof(rcEdge)*maxEdgeCount, RC_ALLOC_TEMP);
	if (!edges)
	{
		rcFree(firstEdge);
		return false;
	}
	
	for(int i = 0; i < mesh.nverts; i++)
		firstEdge[i] = RC_MESH_NULL_IDX;
	
   // First process edges where 1st node < 2nd node
	for(int i = 0; i < mesh.npolys; ++i)
	{
      unsigned short* t = &mesh.polys[i*mesh.nvp];

      // Skip "missing" polygons
      if(*t == U16_MAX)
         continue;

		for (int j = 0; j < mesh.nvp; ++j)
		{
			unsigned short v0 = t[j];           // jth vert

         if(v0 == RC_MESH_NULL_IDX)
            break;

			unsigned short v1 = (j+1 >= mesh.nvp || t[j+1] == RC_MESH_NULL_IDX) ? t[0] : t[j+1];  // j+1th vert

			if (v0 < v1)      
			{
				rcEdge& edge = edges[edgeCount];       // edge connecting v0 and v1
				edge.vert[0] = v0;
				edge.vert[1] = v1;
				edge.poly[0] = (unsigned short)i;      // left poly
				edge.poly[1] = (unsigned short)i;      // right poly, will be recalced later -- the fact that both are the same is used as a marker

				nextEdge[edgeCount] = firstEdge[v0];        // Next edge on the previous vert now points to whatever was in firstEdge previously
				firstEdge[v0] = (unsigned short)edgeCount;  // First edge of this vert 

				edgeCount++;                           // edgeCount never resets -- each edge gets a unique id
			}
		}
	}
	
   // Now process edges where 2nd node is > 1st node
	for(int i = 0; i < mesh.npolys; ++i)
	{
		unsigned short* t = &mesh.polys[i*mesh.nvp];

      // Skip "missing" polygons
      if(*t == U16_MAX)
         continue;

		for(int j = 0; j < mesh.nvp; ++j)
		{
			unsigned short v0 = t[j];
         if(v0 == RC_MESH_NULL_IDX)
            break;

			unsigned short v1 = (j+1 >= mesh.nvp || t[j+1] == RC_MESH_NULL_IDX) ? t[0] : t[j+1];

			if(v0 > v1)      
			{
				for(unsigned short e = firstEdge[v1]; e != RC_MESH_NULL_IDX; e = nextEdge[e])
				{
					rcEdge& edge = edges[e];
					if(edge.vert[1] == v0 && edge.poly[0] == edge.poly[1])
               {
						edge.poly[1] = (unsigned short)i;
                  break;
               }
				}
			}
		}
	}

	// Now create our neighbor data
	for(int i = 0; i < edgeCount; i++)
	{
		const rcEdge& e = edges[i];

		if(e.poly[0] != e.poly[1])      // Should normally be the case
		{
         U16 *v;

         v = &mesh.verts[e.vert[0] * 2];
         neighbor.borderStart.set(v[0] - mesh.offsetX, v[1] - mesh.offsetY);

         v = &mesh.verts[e.vert[1] * 2];
         neighbor.borderEnd.set(v[0] - mesh.offsetX, v[1] - mesh.offsetY);

         neighbor.borderCenter.set((neighbor.borderStart + neighbor.borderEnd) * 0.5);

         neighbor.zoneID = polyToZoneMap[e.poly[1]];  
         gBotNavMeshZones[polyToZoneMap[e.poly[0]]]->mNeighbors.push_back(neighbor);  // (copies neighbor implicitly)

         neighbor.zoneID = polyToZoneMap[e.poly[0]];   
         gBotNavMeshZones[polyToZoneMap[e.poly[1]]]->mNeighbors.push_back(neighbor);
		}
	}
	
	rcFree(firstEdge);
	rcFree(edges);
	
	return true;
}


Vector<DatabaseObject *> zones;

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

      if(zone && PolygonContains2(zone->mPolyBounds.address(), zone->mPolyBounds.size(), point))
         return zone;   
   }

   if(zones.size() != 0)  // In case of point was close to polygon, but not inside the zone?
      return dynamic_cast<BotNavMeshZone *>(zones[0]);

   return NULL;
}

#ifdef TNL_DEBUG
#define LOG_TIMER
#endif

// Required for input to Triangle
static void buildHolesList(const Vector<DatabaseObject *> &barriers, Vector<F32> &holes)
{
   Point ctr; 

   for(S32 i = 0; i < barriers.size(); i++)
   {
      Barrier *barrier = dynamic_cast<Barrier *>(barriers[i]);

      if(!barrier)
         continue;

      // Triangle requires a point interior to each hole.  Finding one depends on what type of barrier we have:
      if(barrier->mSolid && barrier->mRenderFillGeometry.size() >= 3)     // Could be concave, centroid of first triangle of fill geom will be interior
      {
         ctr.set((barrier->mRenderFillGeometry[0].x + barrier->mRenderFillGeometry[1].x + barrier->mRenderFillGeometry[2].x) / 3, 
                  (barrier->mRenderFillGeometry[0].y + barrier->mRenderFillGeometry[1].y + barrier->mRenderFillGeometry[2].y) / 3);
      }
      else                    // Standard wall, convex poly, center will be an interior point
         ctr = barrier->getExtent().getCenter();

      holes.push_back(ctr.x);
      holes.push_back(ctr.y);
   }
}


// Server only
// Use the Triangle library to create zones.  Aggregate triangles with Recast
bool BotNavMeshZone::buildBotMeshZones(Game *game)
{

#ifdef LOG_TIMER
   U32 starttime = Platform::getRealMilliseconds();
#endif

   Rect bounds = game->getWorldExtents();
   bounds.expandToInt(Point(LEVEL_ZONE_BUFFER, LEVEL_ZONE_BUFFER));      // Provide a little breathing room

   Vector<F32> holes;

   Vector<DatabaseObject *> barrierList;
   game->getGridDatabase()->findObjects(BarrierType, barrierList, bounds);

   TPolyPolygon solution;

   if(!Barrier::unionBarriers(barrierList, true, solution))
      return false;

   buildHolesList(barrierList, holes);


#ifdef LOG_TIMER
   U32 done1 = Platform::getRealMilliseconds();
#endif

   // Tessellate!
   Triangulate::TriangleData triangleData;
   if(!Triangulate::processComplex(triangleData, bounds, solution, holes, Triangulate::cmTriangle))  // use Triangulate::cmP2t for poly2tri
      return false;

#ifdef LOG_TIMER
   U32 done2 = Platform::getRealMilliseconds();
#endif

   bool recastPassed = false;

   if(bounds.getWidth() < U16_MAX && bounds.getHeight() < U16_MAX)
   {
      //if(bounds.getWidth() >= U16_MAX || bounds.getHeight() >= U16_MAX)
      //{
      //   logprintf(LogConsumer::LogWarning, "Cannot create bot zones: level too big!");
      //   return false;
      //}

      rcPolyMesh mesh;
      mesh.offsetX = -1 * floor(bounds.min.x + 0.5);
      mesh.offsetY = -1 * bounds.min.y;

      // this works because bounds is always passed by reference.  Is this really needed?
      bounds.offset(Point(mesh.offsetX, mesh.offsetY));

      // Merge!  into convex polygons
      recastPassed = Triangulate::mergeTriangles(triangleData, mesh);
      if(recastPassed)
      {

         BotNavMeshZone *botzone = NULL;
   
         const S32 bytesPerVertex = sizeof(U16);      // Recast coords are U16s
         Vector<S32> polyToZoneMap(mesh.npolys);

         // Visualize rcPolyMesh
         for(S32 i = 0; i < mesh.npolys; i++)
         {
            S32 firstx = S32_MAX;
            S32 firsty = S32_MAX;
            S32 lastx = S32_MAX;
            S32 lasty = S32_MAX;

            S32 j = 0;
            botzone = NULL;

            while(j < mesh.nvp)
            {
               if(mesh.polys[(i * mesh.nvp + j)] == U16_MAX)
                  break;

               const U16 *vert = &mesh.verts[mesh.polys[(i * mesh.nvp + j)] * bytesPerVertex];

               if(vert[0] == U16_MAX)
                  break;

               if(gBotNavMeshZones.size() >= MAX_ZONES)      // Don't add too many zones...
                  break;

               if(j == 0)
               {
                  botzone = new BotNavMeshZone();
                  polyToZoneMap[i] = botzone->getZoneId();
               }

               botzone->mPolyBounds.push_back(Point(vert[0] - mesh.offsetX, vert[1] - mesh.offsetY));
               j++;
            }
   
            if(botzone != NULL)
            {
               botzone->mCentroid.set(findCentroid(botzone->mPolyBounds));

		         botzone->mConvex = true;             
		         botzone->addToGame(game);
		         botzone->computeExtent();   
            }
         }

#ifdef LOG_TIMER
         logprintf("Recast built %d zones!", gBotNavMeshZones.size() );
#endif               

         buildBotNavMeshZoneConnectionsRecastStyle(mesh, polyToZoneMap);
         BotNavMeshZone::linkTeleportersBotNavMeshZoneConnections(game);
		}
   }

   // If recast failed, build zones from the underlying triangle geometry.  This bit could be made more efficient by using the adjacnecy
   // data from Triangle, but it should only run rarely, if ever.
   if(!recastPassed)
   {
      // Visualize triangle output
      for(S32 i = 0; i < triangleData.triangleCount * 3; i+=3)
      {
         if(gBotNavMeshZones.size() >= MAX_ZONES)      // Don't add too many zones...
            break;
         BotNavMeshZone *botzone = new BotNavMeshZone();

         // removed F32(S32(...)) - why do rounding here? (sam)
		   botzone->mPolyBounds.push_back(Point(triangleData.pointList[triangleData.triangleList[i]*2],   triangleData.pointList[triangleData.triangleList[i]*2 + 1]));
		   botzone->mPolyBounds.push_back(Point(triangleData.pointList[triangleData.triangleList[i+1]*2], triangleData.pointList[triangleData.triangleList[i+1]*2 + 1]));
		   botzone->mPolyBounds.push_back(Point(triangleData.pointList[triangleData.triangleList[i+2]*2], triangleData.pointList[triangleData.triangleList[i+2]*2 + 1]));
         botzone->mCentroid.set(findCentroid(botzone->mPolyBounds));

		   botzone->mConvex = true;             // Avoid random red and green on /showzones
		   botzone->addToGame(game);
		   botzone->computeExtent();   
      }

      BotNavMeshZone::buildBotNavMeshZoneConnections();
   }

#ifdef LOG_TIMER
   U32 done3 = Platform::getRealMilliseconds();

   logprintf("Timings: %d %d %d", done1-starttime, done2-done1, done3-done2);
#endif

   return true;
}


// Only runs on server
// TODO can be combined with buildBotNavMeshZoneConnectionsRecastStyle() ?
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
   linkTeleportersBotNavMeshZoneConnections(gServerGame);
}

// Only runs on server
void BotNavMeshZone::linkTeleportersBotNavMeshZoneConnections(Game *game)
{
   NeighboringZone neighbor;
   // Now create paths representing the teleporters
   Vector<DatabaseObject *> teleporters, dests;

	game->getGridDatabase()->findObjects(TeleportType, teleporters, game->getWorldExtents());

	for(S32 i = 0; i < teleporters.size(); i++)
	{
		Teleporter *teleporter = dynamic_cast<Teleporter *>(teleporters[i]);

      if(!teleporter)
         continue;

      BotNavMeshZone *origZone = findZoneContainingPoint(teleporter->getActualPos());

      if(origZone != NULL)
		for(S32 j = 0; j < teleporter->mDest.size(); j++)     // Review each teleporter destination
		{
         BotNavMeshZone *destZone = findZoneContainingPoint(teleporter->mDest[j]);

			if(destZone != NULL && origZone != destZone)      // Ignore teleporters that begin and end in the same zone
			{
				// Teleporter is one way path
				neighbor.zoneID = destZone->mZoneId;
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
   static U16 whichList[MAX_ZONES];       // Record whether a zone is on the open or closed list
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


