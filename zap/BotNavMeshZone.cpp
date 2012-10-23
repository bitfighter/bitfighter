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
#include "gameObjectRender.h"
#include "teleporter.h"
#include "barrier.h"             // For Barrier methods in generating zones
#include "EngineeredItem.h"   // For Turret and ForceFieldProjector methods in generating zones
#include "../recast/Recast.h"    // For zone generation
#include "../recast/RecastAlloc.h"
#include "ServerGame.h"

#ifndef ZAP_DEDICATED
#  include "UIMenus.h"
#endif

#include <vector>
#include <math.h>


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

// Declare our statics
static const S32 MAX_ZONES = 10000;     // Don't make this go above S16 max - 1 (32,766), AStar::findPath is limited
Vector<BotNavMeshZone *> BotNavMeshZone::mAllZones;

static GridDatabase mBotZoneDatabase;


//TNL_IMPLEMENT_NETOBJECT(BotNavMeshZone);


// Constructor
BotNavMeshZone::BotNavMeshZone(S32 id)
{
   mObjectTypeNumber = BotNavMeshZoneTypeNumber;
   setNewGeometry(geomPolygon);

   mZoneId = id;
}


// Destructor
BotNavMeshZone::~BotNavMeshZone()
{
   removeFromDatabase();
}


// Return the center of this zone
Point BotNavMeshZone::getCenter()
{
   return getExtent().getCenter();     // Good enough for government work
}


void BotNavMeshZone::render(S32 layerIndex)    
{
#ifndef ZAP_DEDICATED
   if(layerIndex == 0)
      renderNavMeshZone(getOutline(), getFill(), getCentroid(), mZoneId, true);

   else if(layerIndex == 1)
      renderNavMeshBorders(mNeighbors);
#endif
}


// Use this to help keep track of which robots are where
// Only gets run on the server, never on client, obviously, because that's where the bots are!!!
//bool BotNavMeshZone::collide(BfObject *hitObject)
//{
//   // This does not get run anymore, it is in a seperate database.
//   if(hitObject->getObjectTypeNumber() == RobotShipTypeNumber)     // Only care about robots...
//   {
//      Robot *r = (Robot *) hitObject;
//      r->setCurrentZone(mZoneId);
//   }
//   return false;
//}


GridDatabase *BotNavMeshZone::getBotZoneDatabase()
{
   return &mBotZoneDatabase;
}


void BotNavMeshZone::addToZoneDatabase()
{
   setExtent(calcExtents());
   DatabaseObject::addToDatabase(&mBotZoneDatabase);
}


// More precise boundary for precise collision detection
bool BotNavMeshZone::getCollisionPoly(Vector<Point> &polyPoints) const
{
   polyPoints = *getOutline();
   return true;
}


// Returns ID of zone containing specified point
U16 BotNavMeshZone::findZoneContaining(GridDatabase *botZoneDatabase, const Point &p)
{
   fillVector.clear();
   botZoneDatabase->findObjects(BotNavMeshZoneTypeNumber, fillVector,
                              Rect(p - Point(0.1f,0.1f),p + Point(0.1f,0.1f)));  // Slightly extend Rect, it can be on the edge of zone

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      // First a quick, crude elimination check then more comprehensive one
      // Since our zones are convex, we can use the faster method!  Yay!
      // Actually, we can't, as it is not reliable... reverting to more comprehensive (and working) version.
      BotNavMeshZone *zone = static_cast<BotNavMeshZone *>(fillVector[i]);

      if( zone->getExtent().contains(p) 
                        && (PolygonContains2(zone->getOutline()->address(), zone->getOutline()->size(), p)) )
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
bool BotNavMeshZone::buildBotNavMeshZoneConnectionsRecastStyle(ServerGame *game, rcPolyMesh &mesh, const Vector<S32> &polyToZoneMap)    
{
   if(mAllZones.size() == 0)      // Nothing to do!
      return true;

   // We'll reuse these objects throughout the following block, saving the cost of creating and destructing them
   Point bordStart, bordEnd, bordCen;
   Rect rect;
   NeighboringZone neighbor;

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
         mAllZones[polyToZoneMap[e.poly[0]]]->mNeighbors.push_back(neighbor);  // (copies neighbor implicitly)

         neighbor.zoneID = polyToZoneMap[e.poly[0]];   
         mAllZones[polyToZoneMap[e.poly[1]]]->mNeighbors.push_back(neighbor);
      }
   }
   
   rcFree(firstEdge);
   rcFree(edges);
   
   return true;
}


static Vector<DatabaseObject *> zones;

// Returns index of zone containing specified point
static BotNavMeshZone *findZoneContainingPoint(GridDatabase &botZoneDatabase, const Point &point)
{
   Rect rect(point, 0.01f);
   zones.clear();
   botZoneDatabase.findObjects(BotNavMeshZoneTypeNumber, zones, rect);

   // If there is more than one possible match, pick the first arbitrarily (could happen if dest is right on a zone border)
   for(S32 i = 0; i < zones.size(); i++)
   {
      BotNavMeshZone *zone = static_cast<BotNavMeshZone *>(zones[i]);

      if(zone && PolygonContains2(zone->getOutline()->address(), zone->getOutline()->size(), point))
         return zone;   
   }

   if(zones.size() != 0)  // In case of point was close to polygon, but not inside the zone?
      return static_cast<BotNavMeshZone *>(zones[0]);

   return NULL;
}


#ifdef TNL_DEBUG
#  define LOG_TIMER
#endif

// Required for input to Triangle
static void buildHolesList(const Vector<DatabaseObject *> &barriers,
      const Vector<DatabaseObject *> &turrets,
      const Vector<DatabaseObject *> &forceFieldProjectors, Vector<F32> &holes)
{
   Point ctr; 

   // Build holes list for barriers
   for(S32 i = 0; i < barriers.size(); i++)
   {
      if(barriers[i]->getObjectTypeNumber() != BarrierTypeNumber)
         continue;

      Barrier *barrier = static_cast<Barrier *>(barriers[i]);

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

   // Build holes list for turrets
   for(S32 i = 0; i < turrets.size(); i++)
   {
      if(turrets[i]->getObjectTypeNumber() != TurretTypeNumber)
         continue;

      Turret *turret = static_cast<Turret *>(turrets[i]);

      ctr = turret->getExtent().getCenter();

      holes.push_back(ctr.x);
      holes.push_back(ctr.y);
   }

   // Build holes list for forcefields
   for(S32 i = 0; i < forceFieldProjectors.size(); i++)
   {
      if(forceFieldProjectors[i]->getObjectTypeNumber() != ForceFieldProjectorTypeNumber)
         continue;

      ForceFieldProjector *forceField = static_cast<ForceFieldProjector *>(forceFieldProjectors[i]);

      ctr = forceField->getExtent().getCenter();

      holes.push_back(ctr.x);
      holes.push_back(ctr.y);
   }
}


static bool mergeBotZoneBuffers(const Vector<DatabaseObject *> &barriers,
                                const Vector<DatabaseObject *> &turrets,
                                const Vector<DatabaseObject *> &forceFieldProjectors, 
                                      Vector<Vector<Point> > &solution)
{

   Vector<const Vector<Point> *> inputPolygons;

   // Add barriers
   for(S32 i = 0; i < barriers.size(); i++)
   {
      if(barriers[i]->getObjectTypeNumber() != BarrierTypeNumber)
         continue;

      Barrier *barrier = static_cast<Barrier *>(barriers[i]);

      inputPolygons.push_back(barrier->getBufferForBotZone());
   }

   // Add turrets
   for (S32 i = 0; i < turrets.size(); i++)
   {
      if(turrets[i]->getObjectTypeNumber() != TurretTypeNumber)
         continue;

      Turret* turret = static_cast<Turret *>(turrets[i]);

      inputPolygons.push_back(turret->getBufferForBotZone());
   }

   // Add forcefield projectors
   for (S32 i = 0; i < forceFieldProjectors.size(); i++)
   {
      if(forceFieldProjectors[i]->getObjectTypeNumber() != ForceFieldProjectorTypeNumber)
         continue;

      ForceFieldProjector* forceFieldProjector = static_cast<ForceFieldProjector *>(forceFieldProjectors[i]);

      inputPolygons.push_back(forceFieldProjector->getBufferForBotZone());
   }

   return mergePolys(inputPolygons, solution);
}


// Populate mAllZones -- we'll use this for efficiency, saving us the trouble of repeating this operation in multiple places.  We can retrieve
// them using BotNavMeshZone::getBotZones().
void BotNavMeshZone::populateZoneList()
{
   const Vector<DatabaseObject *> *objects = mBotZoneDatabase.findObjects_fast();

   mAllZones.resize(objects->size());

   for(S32 i = 0; i < objects->size(); i++)
      mAllZones[i] = static_cast<BotNavMeshZone *>(objects->get(i));
}


// Server only
// Use the Triangle library to create zones.  Aggregate triangles with Recast
bool BotNavMeshZone::buildBotMeshZones(ServerGame *game, bool triangulateZones)
{

#ifdef LOG_TIMER
   U32 starttime = Platform::getRealMilliseconds();
#endif

   mAllZones.deleteAndClear();

   Rect bounds = game->getWorldExtents();
   bounds.expandToInt(Point(LEVEL_ZONE_BUFFER, LEVEL_ZONE_BUFFER));      // Provide a little breathing room

   // Make sure level isn't too big for zone generation, which uses 16 bit ints
   if(bounds.getHeight() >= (F32)U16_MAX || bounds.getWidth() >= (F32)U16_MAX)
   {
      logprintf(LogConsumer::LogLevelError, "Level too big for zone generation! (max allowed dimension, after gridSize expansion, is %d)", U16_MAX);
      return false;
   }

   Vector<F32> holes;
   Vector<Vector<Point> > solution;

   Vector<DatabaseObject *> barrierList;
   game->getGameObjDatabase()->findObjects((TestFunc)isWallType, barrierList, bounds);

   Vector<DatabaseObject *> turretList;
   game->getGameObjDatabase()->findObjects(TurretTypeNumber, turretList, bounds);

   Vector<DatabaseObject *> forceFieldProjectorList;
   game->getGameObjDatabase()->findObjects(ForceFieldProjectorTypeNumber, forceFieldProjectorList, bounds);

   // Merge bot zone buffers from barriers, turrets, and forcefield projectors
   if(!mergeBotZoneBuffers(barrierList, turretList, forceFieldProjectorList, solution))
      return false;

   buildHolesList(barrierList, turretList, forceFieldProjectorList, holes);


#ifdef LOG_TIMER
   U32 done1 = Platform::getRealMilliseconds();
#endif

   // Tessellate!
   Triangulate::TriangleData triangleData;
   if(!Triangulate::processComplex(triangleData, bounds, solution, holes))
      return false;

#ifdef LOG_TIMER
   U32 done2 = Platform::getRealMilliseconds();
#endif

   bool recastPassed = false;
   rcPolyMesh mesh;

   if(bounds.getWidth() < U16_MAX && bounds.getHeight() < U16_MAX)
   {
      mesh.offsetX = -1 * (int)floor(bounds.min.x + 0.5f);
      mesh.offsetY = -1 * (int)floor(bounds.min.y + 0.5f);

      // This works because bounds is always passed by reference.  Is this really needed?
      bounds.offset(Point(mesh.offsetX, mesh.offsetY));

      // Merge!  into convex polygons
      recastPassed = Triangulate::mergeTriangles(triangleData, mesh);
   }

   populateZoneList();     // Populate mAllZones

   // So here we are.  If recastPassed, our triangles were successfully aggregated into zones, but will need further polishing below.  If it failed 
   // (which will happen rarely, if ever), the aggregation failed and our zones are just the unaggregated raw triangles that we created before 
   // attempting mergeTriangles.  

   if(recastPassed)
   {
      BotNavMeshZone *botzone = NULL;
      bool addedZones = false;
   
      const S32 bytesPerVertex = sizeof(U16);      // Recast coords are U16s
      Vector<S32> polyToZoneMap;
      polyToZoneMap.resize(mesh.npolys);

      // Visualize rcPolyMesh
      for(S32 i = 0; i < mesh.npolys; i++)
      {
         S32 j = 0;
         botzone = NULL;

         while(j < mesh.nvp)
         {
            if(mesh.polys[(i * mesh.nvp + j)] == U16_MAX)
               break;

            const U16 *vert = &mesh.verts[mesh.polys[(i * mesh.nvp + j)] * bytesPerVertex];

            if(vert[0] == U16_MAX)
               break;

            if(mBotZoneDatabase.getObjectCount() >= MAX_ZONES)      // Don't add too many zones...
               break;

            if(j == 0)     // Add new zone because... why?
            {
               botzone = new BotNavMeshZone(mBotZoneDatabase.getObjectCount());

               // Triangulation only needed for display on local client... it is expensive to compute for so many zones,
               // and there is really no point if they will never be viewed.  Once disabled, triangluation cannot be re-enabled
               // for this object.
               if(!triangulateZones)
                  botzone->disableTriangulation();

               polyToZoneMap[i] = botzone->getZoneId();
            }

            botzone->addVert(Point(vert[0] - mesh.offsetX, vert[1] - mesh.offsetY));
            j++;
         }
   
         if(botzone != NULL)
         {
            botzone->addToZoneDatabase();   
            addedZones = true;
         }
      }

#ifdef LOG_TIMER
      logprintf("Recast built %d zones!", mBotZoneDatabase.getObjectCount());
#endif              

      if(addedZones)
         populateZoneList();     // Repopulate mAllZones with the zones we modified above

      buildBotNavMeshZoneConnectionsRecastStyle(game, mesh, polyToZoneMap);
      linkTeleportersBotNavMeshZoneConnections(game);
   }

   // If recast failed, build zones from the underlying triangle geometry.  This bit could be made more efficient by using the adjacnecy
   // data from Triangle, but it should only run rarely, if ever.
   else  // recastPassed == false
   {
      TNLAssert(false, "Recast failed -- please report this level to the devs, and pick continue to build zones from triangle output");
      logprintf(LogConsumer::LogLevelError, "There were problems with bot nav zone creation -- please report this level to the devs!");

      // Visualize triangle output
      for(S32 i = 0; i < triangleData.triangleCount * 3; i+=3)
      {
         if(mBotZoneDatabase.getObjectCount() >= MAX_ZONES)      // Don't add too many zones...
            break;

         BotNavMeshZone *botzone = new BotNavMeshZone();
         // Triangulation only needed for display on local client... it is expensive to compute for so many zones,
         // and there is really no point if they will never be viewed.  Once disabled, triangluation cannot be re-enabled
         // for this object.
         if(!triangulateZones)
            botzone->disableTriangulation();

         botzone->addVert(Point(triangleData.pointList[triangleData.triangleList[i]*2],   triangleData.pointList[triangleData.triangleList[i]*2 + 1]));
         botzone->addVert(Point(triangleData.pointList[triangleData.triangleList[i+1]*2], triangleData.pointList[triangleData.triangleList[i+1]*2 + 1]));
         botzone->addVert(Point(triangleData.pointList[triangleData.triangleList[i+2]*2], triangleData.pointList[triangleData.triangleList[i+2]*2 + 1]));

         botzone->addToZoneDatabase();
       }

      BotNavMeshZone::buildBotNavMeshZoneConnections(game);
   }

#ifdef LOG_TIMER
   U32 done3 = Platform::getRealMilliseconds();

   logprintf("Timings: %d %d %d", done1-starttime, done2-done1, done3-done2);
#endif

   return true;
}


// static fn
const Vector<BotNavMeshZone *> *BotNavMeshZone::getBotZones()
{
   return &mAllZones;
}


// Only runs on server
// TODO can be combined with buildBotNavMeshZoneConnectionsRecastStyle() ?
void BotNavMeshZone::buildBotNavMeshZoneConnections(ServerGame *game)    
{
   if(mAllZones.size() == 0)      // Nothing to do!
      return;

   // We'll reuse these objects throughout the following block, saving the cost of creating and destructing them
   Point bordStart, bordEnd, bordCen;
   Rect rect;
   NeighboringZone neighbor;

   // Figure out which zones are adjacent to which, and find the "gateway" between them
   for(S32 i = 0; i < zones.size() - 1; i++)
   {
      for(S32 j = i + 1; j < zones.size(); j++)
      {
         // Do zones i and j touch?  First a quick and dirty bounds check:
         if(!zones[i]->getExtent().intersectsOrBorders(zones[j]->getExtent()))
            continue;

         if(zonesTouch(mAllZones[i]->getOutline(), mAllZones[j]->getOutline(), 1.0, bordStart, bordEnd))
         {
            rect.set(bordStart, bordEnd);
            bordCen.set(rect.getCenter());

            // Zone j is a neighbor of i
            neighbor.zoneID = j;
            neighbor.borderStart.set(bordStart);
            neighbor.borderEnd.set(bordEnd);
            neighbor.borderCenter.set(bordCen);

            neighbor.distTo = mAllZones[i]->getExtent().getCenter().distanceTo(bordCen);     // Whew!
            neighbor.center.set(mAllZones[j]->getCenter());
            mAllZones[i]->mNeighbors.push_back(neighbor);

            // Zone i is a neighbor of j
            neighbor.zoneID = i;
            neighbor.borderStart.set(bordStart);
            neighbor.borderEnd.set(bordEnd);
            neighbor.borderCenter.set(bordCen);

            neighbor.distTo = mAllZones[j]->getExtent().getCenter().distanceTo(bordCen);     
            neighbor.center.set(mAllZones[i]->getCenter());
            mAllZones[j]->mNeighbors.push_back(neighbor);
         }
      }
   }

   linkTeleportersBotNavMeshZoneConnections(game);
}


// Only runs on server
void BotNavMeshZone::linkTeleportersBotNavMeshZoneConnections(ServerGame *game)
{
   NeighboringZone neighbor;
   // Now create paths representing the teleporters
   Vector<DatabaseObject *> teleporters, dests;

   game->getGameObjDatabase()->findObjects(TeleporterTypeNumber, teleporters);

   for(S32 i = 0; i < teleporters.size(); i++)
   {
      Teleporter *teleporter = static_cast<Teleporter *>(teleporters[i]);

      BotNavMeshZone *origZone = findZoneContainingPoint(mBotZoneDatabase, teleporter->getPos());

      if(origZone != NULL)
         for(S32 j = 0; j < teleporter->getDestCount(); j++)     // Review each teleporter destination
         {
            BotNavMeshZone *destZone = findZoneContainingPoint(mBotZoneDatabase, teleporter->getDest(j));

            if(destZone != NULL && origZone != destZone)      // Ignore teleporters that begin and end in the same zone
            {
               // Teleporter is one way path
               neighbor.zoneID = destZone->mZoneId;
               neighbor.borderStart.set(teleporter->getPos());
               neighbor.borderEnd.set(teleporter->getDest(j));
               neighbor.borderCenter.set(teleporter->getPos());

               // Teleport instantly, at no cost -- except this is wrong... if teleporter has multiple dests, actual cost could be quite high.
               // This should be the average of the costs of traveling from each dest zone to the target zone
               neighbor.distTo = 0;                                    
               neighbor.center.set(teleporter->getPos());

               origZone->mNeighbors.push_back(neighbor);
            }
         }  // for loop iterating over teleporter dests
   } // for loop iterating over teleporters
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
NeighboringZone::NeighboringZone()
{
   zoneID = 0;
   distTo = 0;
}


////////////////////////////////////////
////////////////////////////////////////


// Rough guess as to distance from fromZone to toZone
F32 AStar::heuristic(const Vector<BotNavMeshZone *> *zones, S32 fromZone, S32 toZone)
{
   return zones->get(fromZone)->getCenter().distanceTo(zones->get(toZone)->getCenter());
}


// Returns a path, including the startZone and targetZone 
Vector<Point> AStar::findPath(const Vector<BotNavMeshZone *> *zones, S32 startZone, S32 targetZone, const Point &target)
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
   Fcost[0] = Hcost[0] = heuristic(zones, startZone, targetZone);

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
         //   Since the list is a binary heap, this will be the lowest F cost cell on the open list.
         S32 parentZone = openZone[openList[1]];

         if(parentZone == targetZone)
         {
            foundPath = true; 
            break;
         }

         whichList[parentZone] = onClosedList;   // add the item to the closed list
         numberOfOpenListItems--;   

         //   Open List = Binary Heap: Delete this item from the open list, which
         // is maintained as a binary heap. For more information on binary heaps, see:
         //   http://www.policyalmanac.org/games/binaryHeaps.htm
            
         //   Delete the top item in binary heap and reorder the heap, with the lowest F cost item rising to the top.
         openList[1] = openList[numberOfOpenListItems + 1];   // Move the last item in the heap up to slot #1
         S16 v = 1; 

         //   Loop until the new item in slot #1 sinks to its proper spot in the heap.
         while(true) // ***
         {
            S16 u = v;      
            if (2 * u + 1 < numberOfOpenListItems) // if both children exist
            {
               // Check if the F cost of the parent is greater than each child,
               // and select the lowest of the two children
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
         //   are similar, conceptually, to the binary heap children mentioned
         //   above, but don't confuse them. They are different.
         // Add these adjacent child squares to the open list
         //   for later consideration if appropriate.

         Vector<NeighboringZone> neighboringZones = zones->get(parentZone)->mNeighbors;

         for(S32 a = 0; a < neighboringZones.size(); a++)
         {
            NeighboringZone zone = neighboringZones[a];
            S32 zoneID = zone.zoneID;

            //   Check if zone is already on the closed list (items on the closed list have
            //   already been considered and can now be ignored).
            if(whichList[zoneID] == onClosedList) 
               continue;

            //   Add zone to the open list if it's not already on it
            TNLAssert(newOpenListItemID < MAX_ZONES, "Too many nav zones... try increasing MAX_ZONES!");
            if(whichList[zoneID] != onOpenList && newOpenListItemID < MAX_ZONES) 
            {   
               // Create a new open list item in the binary heap
               newOpenListItemID = newOpenListItemID + 1;   // Give each new item a unique id
               S32 m = numberOfOpenListItems + 1;
               openList[m] = newOpenListItemID;             // Place the new open list item (actually, its ID#) at the bottom of the heap
               openZone[newOpenListItemID] = zoneID;        // Record zone as a newly opened

               Hcost[openList[m]] = heuristic(zones, zoneID, targetZone);
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
            //   path to that cell from the starting location is a better one. 
            //   If so, change the parent of the cell and its G and F costs.

            else // zone was already on the open list
            {
               // Figure out the G cost of this possible new path
               S32 tempGcost = (S32)(Gcost[parentZone] + zone.distTo);
               
               // If this path is shorter (G cost is lower) then change
               // the parent cell, G cost and F cost.
               if(tempGcost < Gcost[zoneID])
               {
                  parentZones[zoneID] = parentZone; // Change the square's parent
                  Gcost[zoneID] = (F32)tempGcost;        // and its G cost         

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

   // Save the path if it exists
   if(!foundPath)
   {      
      TNLAssert(path.size() == 0, "Expected empty path!");
      return path;
   }

   // Working backwards from the target to the starting location by checking
   // each cell's parent, figure out the length of the path.
   //
   // Fortunately, we want our list to have the closest zone last (see getWaypoint),
   // so it all works out nicely.
   //
   // We'll store both the zone center and the gateway to the neighboring zone.  This
   // will help keep the robot from getting hung up on blocked but technically visible
   // paths, such as when we are trying to fly around a protruding wall stub.

   path.push_back(target);                               // First point is the actual target itself
   path.push_back(zones->get(targetZone)->getCenter());  // Second is the center of the target's zone
      
   S32 zone = targetZone;

   while(zone != startZone)
   {
      path.push_back(findGateway(zones, parentZones[zone], zone));   // Don't switch findGateway arguments, some path is one way (teleporters).
      zone = parentZones[zone];                                      // Find the parent of the current cell
      path.push_back(zones->get(zone)->getCenter());
   }

   path.push_back(zones->get(startZone)->getCenter());
   return path;
}


// Return a point representing gateway between zones
Point AStar::findGateway(const Vector<BotNavMeshZone *> *zones, S32 zone1, S32 zone2)
{
   S32 neighborIndex = zones->get(zone1)->getNeighborIndex(zone2);
   TNLAssert(neighborIndex >= 0, "Invalid neighbor index!!");

   return zones->get(zone1)->mNeighbors[neighborIndex].borderCenter;
}


};


