//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "BotNavMeshZone.h"

#include "ship.h"                   // For Ship::CollisionRadius
#include "Teleporter.h"             // For Teleporter::TELEPORTER_RADIUS
#include "CoreGame.h"
#include "gameObjectRender.h"
#include "barrier.h"                // For Barrier methods in generating zones
#include "EngineeredItem.h"         // For Turret and ForceFieldProjector methods in generating zones
#include "speedZone.h"
#include "GeomUtils.h"
#include "MathUtils.h"

#include "tnlLog.h"

#include "../recast/RecastAlloc.h"
#include <clipper.hpp>

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
static const S32 MAX_ZONES = 10000;                              // Don't make this go above S16 max - 1 (32,766), AStar::findPath is limited
const S32 BotNavMeshZone::BufferRadius = Ship::CollisionRadius;  // Radius to buffer objects when creating the holes for zones

// Extra padding around the game extents to allow outsize zones to be created.
// Make sure we always have 50 for good measure
const S32 BotNavMeshZone::LevelZoneBuffer = MAX(BufferRadius * 2, 50);
const F32 BotNavMeshZone::CoreTraversalCost = 1000;

// Constructor
BotNavMeshZone::BotNavMeshZone(S32 id)
{
   mObjectTypeNumber = BotNavMeshZoneTypeNumber;
   setNewGeometry(geomPolygon);

   mZoneId = id;
   mWalkable = true;
}


// Destructor
BotNavMeshZone::~BotNavMeshZone()
{
   removeFromDatabase(false);
}


// Return the center of this zone
Point BotNavMeshZone::getCenter()
{
   return getExtent().getCenter();     // Good enough for government work
}


void BotNavMeshZone::renderLayer(S32 layerIndex)    
{
#ifndef ZAP_DEDICATED
   if(layerIndex == 0)
      renderNavMeshZone(getOutline(), getFill(), getCentroid(), mZoneId);

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



void BotNavMeshZone::addToZoneDatabase(GridDatabase *botZoneDatabase)
{
   setExtent(calcExtents());
   DatabaseObject::addToDatabase(botZoneDatabase);    // not a static, just looks like one from here!
}


// More precise boundary for precise collision detection
const Vector<Point> *BotNavMeshZone::getCollisionPoly() const
{
   return getOutline();
}


// Returns index of neighboring zone, or -1 if zone is not a neighbor
S32 BotNavMeshZone::getNeighborIndex(S32 zoneID)
{
   for(S32 i = 0; i < mNeighbors.size(); i++)
      if(mNeighbors[i].zoneID == zoneID)
         return i;

   return -1;
}

U16 BotNavMeshZone::getZoneId()
{
   return mZoneId;
}


bool BotNavMeshZone::getWalkable()
{
   return mWalkable;
}


void BotNavMeshZone::setWalkable(bool canWalk)
{
   mWalkable = canWalk;
}


struct rcEdge
{
   unsigned short vert[2];    // from, to verts
   unsigned short poly[2];    // left, right poly
};


// Build connections between zones using the adjacency data created in recast
//
// Adapted from RecastMesh::buildMeshAdjacency() to fill connection data into
// BotNavMeshZone instead of Recast's adjaceny data
bool BotNavMeshZone::buildConnectionsRecastStyle(const Vector<BotNavMeshZone *> *allZones,
      rcPolyMesh &mesh, const Vector<S32> &polyToZoneMap, S32 coreRecastPolyStartIdx,
      S32 szRecastPolyStartIdx)
{
   if(allZones->size() == 0)      // Nothing to do!
      return true;

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
      if(*t == RC_MESH_NULL_IDX)
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


   // We'll reuse these objects throughout the following block
   Point bordStart, bordEnd, bordCen;
   Rect rect;
   NeighboringZone neighbor;

   // Now create our neighbor data
   for(int i = 0; i < edgeCount; i++)
   {
      const rcEdge& e = edges[i];

      U16 polyId0 = e.poly[0];
      U16 polyId1 = e.poly[1];

      if(polyId0 != polyId1)      // Should normally be the case
      {
         U16 *v;

         v = &mesh.verts[e.vert[0] * 2];
         neighbor.borderStart.set(v[0] - mesh.offsetX, v[1] - mesh.offsetY);

         v = &mesh.verts[e.vert[1] * 2];
         neighbor.borderEnd.set(v[0] - mesh.offsetX, v[1] - mesh.offsetY);

         neighbor.borderCenter.set((neighbor.borderStart + neighbor.borderEnd) * 0.5);

         // Test if polygons are under a Core or SpeedZone using prior knowledge of mesh placement
         bool poly0isCore = (polyId0 > coreRecastPolyStartIdx && polyId0 < szRecastPolyStartIdx);
         bool poly1isCore = (polyId1 > coreRecastPolyStartIdx && polyId1 < szRecastPolyStartIdx);

         bool poly0isSz = (polyId0 > szRecastPolyStartIdx);
         bool poly1isSz = (polyId1 > szRecastPolyStartIdx);

         // Logic for poly0 to get poly1 as a neighbor
         if(!poly0isSz)  // Connections only go one direction for SpeedZone
         {
            neighbor.zoneID = polyToZoneMap[polyId1];

            // Going into a Core zone is high cost so bots try another way
            if(poly1isCore && !poly0isCore)
               neighbor.distTo = CoreTraversalCost;

            // Save poly1 as neighbor to poly0 (copies neighbor implicitly)
            allZones->get(polyToZoneMap[polyId0])->mNeighbors.push_back(neighbor);
         }

         // Now do the same for poly1 to get poly0 as a neighbor
         if(!poly1isSz)  // Connections only go one direction for SpeedZone
         {
            neighbor.zoneID = polyToZoneMap[polyId0];

            // Going into a Core zone is high cost so bots try another way
            if(poly0isCore && !poly1isCore)
               neighbor.distTo = CoreTraversalCost;

            allZones->get(polyToZoneMap[polyId1])->mNeighbors.push_back(neighbor);
         }
      }
   }
   
   rcFree(firstEdge);
   rcFree(edges);
   
   return true;
}


static Vector<DatabaseObject *> zones;

// Returns index of zone containing specified point
static BotNavMeshZone *findZoneTouchingCircle(const GridDatabase *botZoneDatabase, const Point &centerPoint, F32 radius)
{
   Rect rect(centerPoint, radius);
   zones.clear();
   botZoneDatabase->findObjects(BotNavMeshZoneTypeNumber, zones, rect);

   const Vector<Point> *poly;
   Point c;

   // Pick the first zone within our radius
   for(S32 i = 0; i < zones.size(); i++)
   {
      BotNavMeshZone *zone = static_cast<BotNavMeshZone *>(zones[i]);
      poly = zone->getOutline();

      if(zone && polygonCircleIntersect(poly->address(), poly->size(), centerPoint, radius * radius, c))
         return zone;   
   }

   return NULL;
}


#ifdef TNL_DEBUG
#  define LOG_TIMER
#endif


// Populate allZones -- we'll use this for efficiency, saving us the trouble of repeating this operation in multiple places.  
// We can retrieve them using BotNavMeshZone::getBotZones().
void BotNavMeshZone::populateZoneList(GridDatabase *botZoneDatabase, Vector<BotNavMeshZone *> *allZones)
{
   const Vector<DatabaseObject *> *objects = botZoneDatabase->findObjects_fast();

   allZones->resize(objects->size());

   for(S32 i = 0; i < objects->size(); i++)
      allZones->get(i) = static_cast<BotNavMeshZone *>(objects->get(i));
}


// Only runs on server
static void linkConnectionsTeleporters(const GridDatabase *botZoneDatabase,
      const Vector<pair<Point, const Vector<Point> *> > &teleporterData)
{
   NeighboringZone neighbor;
   // Now create paths representing the teleporters
   Point origin, dest;

   F32 triggerRadius = F32(Teleporter::TELEPORTER_RADIUS - Ship::CollisionRadius);

   for(S32 i = 0; i < teleporterData.size(); i++)
   {
      origin = teleporterData[i].first;
      BotNavMeshZone *origZone = findZoneTouchingCircle(botZoneDatabase, origin, triggerRadius);

      if(origZone != NULL)
      for(S32 j = 0; j < teleporterData[i].second->size(); j++)     // Review each teleporter destination
      {
         dest = teleporterData[i].second->get(j);
         BotNavMeshZone *destZone = findZoneTouchingCircle(botZoneDatabase, dest, triggerRadius);

         if(destZone != NULL && origZone != destZone)      // Ignore teleporters that begin and end in the same zone
         {
            // Teleporter is one way path
            neighbor.zoneID = destZone->getZoneId();
            neighbor.borderStart.set(origin);
            neighbor.borderEnd.set(dest);
            neighbor.borderCenter.set(origin);

            // Teleport instantly, at no cost -- except this is wrong... if teleporter has multiple dests, actual cost could be quite high.
            // This should be the average of the costs of traveling from each dest zone to the target zone
            neighbor.distTo = 0;
            neighbor.center.set(origin);

            origZone->mNeighbors.push_back(neighbor);
         }
      }  // for loop iterating over teleporter dests
   } // for loop iterating over teleporters
}


bool isSpeedZoneProblemObject(U8 x)
{
   return
         // BarrierType is the server-side degenerate object for BarrierMaker and PolyWall
         x == BarrierTypeNumber ||
         x == SpeedZoneTypeNumber || x == TeleporterTypeNumber;
}


static S32 QSORT_CALLBACK sortBarriersFirst(DatabaseObject **a, DatabaseObject **b)
{
   return ((*b)->getObjectTypeNumber() == BarrierTypeNumber ? 1 : 0) - ((*a)->getObjectTypeNumber() == BarrierTypeNumber ? 1 : 0);
}

// This is patterned after MoveObject::findFirstCollision(), but modified with
// different assumptions because we are not a moving ship
static BfObject* findFirstCollision(const GridDatabase *gameObjDatabase, F32 &collisionTime, Point &collisionPoint,
      const Point &source, const Point &delta)
{
   // Check for collisions against other objects
   Rect queryRect(source, source + delta);
   queryRect.expand(Point(Ship::CollisionRadius, Ship::CollisionRadius));

   fillVector.clear();

   gameObjDatabase->findObjects(isSpeedZoneProblemObject, fillVector, queryRect);

   fillVector.sort(sortBarriersFirst);

   BfObject *collisionObject = NULL;
   F32 closestCollisionFraction = F32_MAX;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      BfObject *foundObject = static_cast<BfObject *>(fillVector[i]);

      if(!foundObject->isCollisionEnabled())
         continue;

      const Vector<Point> *poly = foundObject->getCollisionPoly();

      if(!poly) // Teleporter
      {
         Point teleporterPos = foundObject->getVert(0);
         F32 thisDist = (teleporterPos - source).len();
         F32 origDist = delta.len();

         collisionTime   = thisDist / origDist;
         collisionPoint  = teleporterPos;
         collisionObject = foundObject;

         continue;
      }

      Point outPoint;
      F32 outFraction;

      bool doesIntersect = PolygonSweptCircleIntersect(&poly->first(), poly->size(), source,
            delta, Ship::CollisionRadius, outPoint, outFraction);

      if(!doesIntersect)
         continue;

      if(outPoint == source)
         continue;

      // Found a collision that's closer
      if(outFraction < closestCollisionFraction)
      {
         // New closest
         closestCollisionFraction = outFraction;

         // Update returned variables
         collisionTime   = outFraction;
         collisionPoint  = outPoint;
         collisionObject = foundObject;
      }

      // Implicit continue
   }

   return collisionObject;
}



static void linkConnectionsSpeedZones(const GridDatabase *gameObjDatabase,
      const GridDatabase *botZoneDatabase, Vector<BotNavMeshZone *> *allZones,
      const Vector<DatabaseObject *> &speedZoneList, const Vector<Vector<Point> > &speedZonePolygons,
      S32 szBotZoneStartId)
{
   NeighboringZone neighbor;
   Point origin;       // SpeedZone origin point
   Point source, dest; // Source and destination points for the current travelling vector

   // Go through each known SpeedZone
   for(S32 i = 0; i < speedZoneList.size(); i++)
   {
      SpeedZone *speedZone = static_cast<SpeedZone *>(speedZoneList[i]);
      const Vector<Point> &szBufferedPoly = speedZonePolygons[i];  // Aligned with speedZoneList

      Point vert0 = speedZone->getVert(0);
      Point dir = speedZone->getVert(1) - vert0;  // end - start

      // Determine the likely end point based on the initial speed of the SpeedZone
      // It has a 1.5 multiplier (see SpeedZone::collided()) for some reason
      U16 currentSpeed = speedZone->getSpeed() * SpeedZone::SpeedMultiplier;

      // Physics: v_i^2 / 2a
      F32 distanceEstimate = sq(currentSpeed) / (2 * Ship::Acceleration);
      Point szTip = speedZone->getOutline()->get(2);  // Tip point 2, from SpeedZone::generatePoints()
      Point szMiddle = (vert0 + szTip) * 0.5;

      if(distanceEstimate < Ship::CollisionRadius)  // Clamp to lower bound
         distanceEstimate = Ship::CollisionRadius;

      Point currentVec = dir;
      currentVec.normalize(distanceEstimate);   // Vector from the SZ

      // These position variables will be updated to reflect the currect trajectory
      // as the simulated ship bounces around
      source = szMiddle;           // Start from middle of zone as an average of any point of entry
      dest = source + currentVec;  // Will likely be overridden

      // This represents the remaining distance we have to travel based on the physics
      F32 currentVecDist = distanceEstimate;

      // Here we'll attempt to see if the speed zone runs us into another object,
      // even after bouncing off of walls multiple times
      bool objSearch = true;
      while(objSearch)
      {
         // Used for bouncing off Barriers
         F32 collisionTime;
         Point collisionPoint;

         // Search for a collision as though we were a ship
         DatabaseObject* hitObject = findFirstCollision(gameObjDatabase,
               collisionTime, collisionPoint, source, currentVec);

         if(hitObject)  // Some object in the way
         {
            U8 hitObjectType = hitObject->getObjectTypeNumber();

            if(hitObjectType == SpeedZoneTypeNumber)
            {
               // If speedzone, set end point to new speedzone
               SpeedZone* hitSpeedZone = static_cast<SpeedZone*>(hitObject);
               // Center of outline
               dest = (hitSpeedZone->getOutline()->get(2) + hitSpeedZone->getPos()) * 0.5;

               objSearch = false;
            }
            else if(hitObjectType == TeleporterTypeNumber)
            {
               // If teleporter, set end point to it's position
               Teleporter* teleporter = static_cast<Teleporter*>(hitObject);
               dest = teleporter->getPos();

               objSearch = false;
            }
            else if(hitObjectType == BarrierTypeNumber)  // Includes both BarrierMaker/Polywall
            {
               // Get better estimate of where we hit the wall because of ship's radius
               F32 distBeforeWall = collisionTime * currentVecDist;

               // If we're close enough set the end point near the wall
               if(currentVecDist - distBeforeWall < 150)  // Somewhat arbitrary
               {
                  F32 newDist = distBeforeWall - 2 * Ship::CollisionRadius;  // Dont get stuck off-botzone
                  currentVec.normalize(newDist);

                  // Re set destination
                  dest = source + currentVec;

                  objSearch = false;
               }
               // Much shorter, include a bounce
               else
               {
                  // Distance vector traveled
                  Point thisVec = currentVec;
                  thisVec.normalize(distBeforeWall);

                  // Advance ship to collision location
                  Point shipPos = source + thisVec;
                  // Recalc normal
                  Point normal = shipPos - collisionPoint;
                  normal.normalize();

                  // calc new v from d, apply elasticity, calc new d
                  F32 collisionSpeed = sqrt(2 * -Ship::Acceleration * distBeforeWall + sq(currentSpeed));
                  Point collisionVel = currentVec;
                  collisionVel.normalize(collisionSpeed);
                  // Taken from MoveObject::computeCollisionResponseBarrier()
                  Point reflectVel = collisionVel -
                        normal * MoveObject::CollisionElasticity * normal.dot(collisionVel);
                  F32 reflectSpeed = reflectVel.len();
                  F32 remainingDist = sq(reflectSpeed) / (2 * Ship::Acceleration);

                  Point newVec = reflectVel;
                  newVec.normalize(remainingDist);

                  // Update new reflected source and destination
                  source = source + thisVec;
                  dest = source + newVec;


                  // Update new current vector estimates
                  currentVec = newVec;
                  currentVecDist = remainingDist;
                  currentSpeed = reflectSpeed;

                  objSearch = true;
               }
            }
            else  // Some other weird hit object?
            {
               // Minimum point the SpeedZone will send you is directly in front of it;
               // this is our fallback end point
               Point dirMin = dir;
               dirMin.normalize(1.5*Ship::CollisionRadius);
               dest = szTip + dirMin;
            }
         }

         // Nothing in the way so use our last updated destination point
         else
             objSearch = false;
      }

      // Now go through all known bot zones under speedzones
      // allZones should be a list of BotNavMeshZone ordered by their zoneId
      // We know ahead of time what ID zones below SpeedZone start at
      for(S32 j = szBotZoneStartId; j < allZones->size(); j++)
      {
         BotNavMeshZone *origZone = allZones->get(j);
         origin = origZone->getCenter();

         // Test if this bot zone center is within the buffered polygon, if so, this
         // speedzone 'owns' this botzone
         if(polygonContainsPoint(&szBufferedPoly[0], szBufferedPoly.size(), origin))
         {
            BotNavMeshZone *destZone =
                  findZoneTouchingCircle(botZoneDatabase, dest, 1);  // Small radius needed only

            // What to do if calculated destination is not found
            if(destZone == NULL || origZone == destZone)
            {
               Point dirMin = dir;
               dirMin.normalize(1.5*Ship::CollisionRadius);
               dest = szTip + dirMin;

               // Redo search
               destZone = findZoneTouchingCircle(botZoneDatabase, dest, 1);
            }

            // Still no zone??
            if(destZone == NULL)
            {
               TNLAssert(destZone != NULL, "Missing SpeedZone destination connection");
               continue;
            }

            // SpeedZone is one way path
            neighbor.zoneID = destZone->getZoneId();
            neighbor.borderStart.set(origin);
            neighbor.borderEnd.set(dest);
            neighbor.borderCenter.set(origin);

            neighbor.distTo = 0;  // Not sure what this should be
            neighbor.center.set(origin);

            origZone->mNeighbors.push_back(neighbor);
         }
      }
   }
}


// Mesh a particular Clipper-sanitized set of polygons.
//
// The 'invertFill' flag instructs triangulation of the Clipper holes instead
// of the Clipper fill polygons
static bool meshArea(const PolyTree &polytree, const Rect &levelBounds,
      const Rect &triangulationBounds, bool invertFill, rcPolyMesh &outMesh)
{
   // Triangulate Clipper solution
   Vector<Point> resultTriangles;
   // This will downscale the Clipper output and use poly2tri to triangulate
   bool successTriangulate = Triangulate::processComplex(resultTriangles, triangulationBounds,
         polytree, !invertFill, invertFill);

   // Guarantee positive coordinates needed for recast
   outMesh.offsetX = -1 * (int)round(levelBounds.min.x);
   outMesh.offsetY = -1 * (int)round(levelBounds.min.y);

   // Merge triangles into convex polygons using Recast
   bool successMerge = Triangulate::mergeTriangles(resultTriangles, outMesh);

   return successTriangulate && successMerge;
}


// Server only
// Use the Triangle library to create zones.  Aggregate triangles with Recast
bool BotNavMeshZone::buildBotMeshZones(GridDatabase *botZoneDatabase, GridDatabase *gameObjDatabase, Vector<BotNavMeshZone *> *allZones,
                                       const Rect *worldExtents, bool triangulateZones)
{
   // Start by finding all objects that'll matter for meshing the level

   // Teleporters form extra connections
   fillVector.clear();
   gameObjDatabase->findObjects(TeleporterTypeNumber, fillVector);

   Vector<pair<Point, const Vector<Point> *> > teleporterData(fillVector.size());
   pair<Point, const Vector<Point> *> teldat;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      Teleporter *teleporter = static_cast<Teleporter *>(fillVector[i]);

      teldat.first  = teleporter->getPos();
      teldat.second = teleporter->getDestList();

      teleporterData.push_back(teldat);
   }

   // So do speedzones
   Vector<DatabaseObject *> speedZoneList;
   gameObjDatabase->findObjects(SpeedZoneTypeNumber, speedZoneList);

   // Cores can be destroyed and open up new paths - need special handling
   Vector<DatabaseObject *> coreList;
   gameObjDatabase->findObjects(CoreTypeNumber, coreList);


   // Get our parameters together
   Vector<DatabaseObject *> barrierList;
   gameObjDatabase->findObjects((TestFunc)isWallType, barrierList, *worldExtents);

   Vector<DatabaseObject *> turretList;
   gameObjDatabase->findObjects(TurretTypeNumber, turretList, *worldExtents);

   Vector<DatabaseObject *> forceFieldProjectorList;
   gameObjDatabase->findObjects(ForceFieldProjectorTypeNumber, forceFieldProjectorList, *worldExtents);



#ifdef LOG_TIMER
   U32 starttime = Platform::getRealMilliseconds();
#endif

   Rect bounds(worldExtents);      // Modifiable copy
   // allZones is a Vector cache of all zones held in memory by the server to
   // be used by Robots without having to call the grid database
   //
   // Clearing this here *also* clears the botZoneDatabase and is required before
   // repopulating it below
   allZones->deleteAndClear();

   bounds.expandToInt(Point(LevelZoneBuffer, LevelZoneBuffer));      // Provide a little breathing room

   // Make sure level isn't too big for zone generation, which uses 16 bit ints
   if(bounds.getHeight() >= (F32)U16_MAX || bounds.getWidth() >= (F32)U16_MAX)
   {
      logprintf(LogConsumer::LogLevelError, "Level too big for zone generation! (max allowed dimension is %d)", U16_MAX);
      return false;
   }


   // Merge bot zone buffers from barriers, turrets, and forcefield projectors
   // The Clipper library is the work horse here.  Its output is essential for the
   // triangulation.  The output contains the upscaled Clipper points (you will need to downscale)

   // Keep track of all the non-navigable polygons to merge
   Vector<Vector<Point> > blockingPolygons;

   // First merge all barrier polygons
   if(barrierList.size() > 0)
   {
      Vector<Vector<Point> > barrierInputPolygons;
      bool unionSucceeded = Barrier::unionBarriers(barrierList,  barrierInputPolygons);
      if(!unionSucceeded)
      {
         logprintf(LogConsumer::LogLevelError, "Barriers failed to merge for bot zones!", U16_MAX);
         return false;
      }

      // Now offset and fill inputPolygons directly
      offsetPolygons(barrierInputPolygons, blockingPolygons, BufferRadius);
   }

   // Add turrets
   for (S32 i = 0; i < turretList.size(); i++)
   {
      if(turretList[i]->getObjectTypeNumber() != TurretTypeNumber)
         continue;

      Turret *turret = static_cast<Turret *>(turretList[i]);

      blockingPolygons.push_back(Vector<Point>());
      turret->getBufferForBotZone(BufferRadius, blockingPolygons.last());
   }

   // Add forcefield projectors
   for (S32 i = 0; i < forceFieldProjectorList.size(); i++)
   {
      if(forceFieldProjectorList[i]->getObjectTypeNumber() != ForceFieldProjectorTypeNumber)
         continue;

      ForceFieldProjector *forceFieldProjector = static_cast<ForceFieldProjector *>(forceFieldProjectorList[i]);

      blockingPolygons.push_back(Vector<Point>());

      forceFieldProjector->getBufferForBotZone(BufferRadius, blockingPolygons.last());
   }


   // The next items are special items that need to be meshed, but must be
   // excluded from initial mesh for various reasons

   // Add Cores - they are destructible
   Vector<Vector<Point> > corePolygons;

   // Increase buffer radius a little to handle the spinning corners
   F32 coreBufferRadius = BufferRadius + 5;

   for (S32 i = 0; i < coreList.size(); i++)
   {
      CoreItem *core = static_cast<CoreItem *>(coreList[i]);

      corePolygons.push_back(Vector<Point>());
      core->getBufferForBotZone(coreBufferRadius, corePolygons.last());
   }

   // Add SpeedZones - they are one-way like areas
   Vector<Vector<Point> > speedZonePolygons;

   for (S32 i = 0; i < speedZoneList.size(); i++)
   {
      SpeedZone *speedZone = static_cast<SpeedZone *>(speedZoneList[i]);

      speedZonePolygons.push_back(Vector<Point>());
      speedZone->getBufferForBotZone(BufferRadius, speedZonePolygons.last());
   }

   bool hasCores = corePolygons.size() > 0;
   bool hasSpeedZones = speedZonePolygons.size() > 0;

   // Make copy and extend to include special areas, this will be used to
   // find all non-navigable areas in the level
   Vector<Vector<Point> > nonStandardPolygons = blockingPolygons;

   if(hasCores)
      for(S32 i = 0; i < corePolygons.size(); i++)
         nonStandardPolygons.push_back(corePolygons[i]);

   if(hasSpeedZones)
      for(S32 i = 0; i < speedZonePolygons.size(); i++)
         nonStandardPolygons.push_back(speedZonePolygons[i]);


   // This is some sort of degenerate empty level; manually inject a tiny zone hole.
   if(nonStandardPolygons.size() == 0)
   {
      // Just add a simple, small square
      Vector<Point> points(4);
      points.push_back(Point(0, 0));
      points.push_back(Point(3, 0));
      points.push_back(Point(3, 3));
      points.push_back(Point(0, 3));

      nonStandardPolygons.push_back(points);
   }


   // Run clipper to merge all the areas, this contains blocked areas and
   // special areas excluded from normal navigable zones
   //
   // These operations upscale the geometry points
   PolyTree levelPolyTree;
   bool clipSuccess = mergePolysToPolyTree(nonStandardPolygons, levelPolyTree);

   // Cores
   PolyTree corePolyTree;
   if(hasCores)
      clipSuccess = clipSuccess &&
      clipPolygonsAsTree(ClipperLib::ctDifference, corePolygons, blockingPolygons, corePolyTree);

   // SpeedZones
   PolyTree szPolyTree;

   if(hasSpeedZones)
      clipSuccess = clipSuccess &&
      clipPolygonsAsTree(ClipperLib::ctDifference, speedZonePolygons, blockingPolygons, szPolyTree);

   // Any failures
   if(!clipSuccess)
   {
      logprintf(LogConsumer::LogLevelError, "Clipper failed to generate input polygons for bot zones!");
      return false;
   }


#ifdef LOG_TIMER
   U32 done1 = Platform::getRealMilliseconds();  // Clipper done
#endif

   // Mesh the main level area (minus special areas)
   rcPolyMesh levelMesh;
   bool meshSuccess = meshArea(levelPolyTree, bounds, bounds, false, levelMesh);

   // Now create the zone polygons for the special areas
   rcPolyMesh coreMesh;
   if(hasCores)
      meshSuccess = meshSuccess && meshArea(corePolyTree, bounds, Rect(0,0,0,0), true, coreMesh);

   rcPolyMesh szMesh;
   if(hasSpeedZones)
      meshSuccess = meshSuccess && meshArea(szPolyTree, bounds, Rect(0,0,0,0), true, szMesh);

   // Any failures
   if(!meshSuccess)
   {
      logprintf(LogConsumer::LogLevelError, "Bot zone mesh failed to generate!");
      return false;
   }

   // Merge meshes
   rcPolyMesh mesh;
   // Reapply bounds
   mesh.offsetX = -1 * (int)round(bounds.min.x);
   mesh.offsetY = -1 * (int)round(bounds.min.y);

   // Build up references to send to the merge method
   const S32 meshCount = 3;
   rcPolyMesh *meshes[meshCount] = {};
   // Order matters here!
   meshes[0] = &levelMesh;
   meshes[1] = &coreMesh;
   meshes[2] = &szMesh;

   // Do the merge
   bool mergeSuccess = rcMergePolyMeshes(meshes, meshCount, mesh);

   if(!mergeSuccess)
   {
      logprintf(LogConsumer::LogLevelError, "Bot zone mesh failed to merge!");
      return false;
   }


   // Save what index the special zones start at in the Recast merged mesh.
   // This will be used later to modify zone connections
   S32  coreRecastPolyStartIdx = levelMesh.npolys;
   S32    szRecastPolyStartIdx = levelMesh.npolys + coreMesh.npolys;


#ifdef LOG_TIMER
   U32 done2 = Platform::getRealMilliseconds();  // poly2tri and Recast done
#endif

   // If Recast succeeded, our triangles were successfully aggregatedinto zones,
   // but will need further polishing.

   BotNavMeshZone *botzone = NULL;
   bool addedZones = false;

   const S32 bytesPerVertex = sizeof(U16);      // Recast coords are U16s

   // Instead of using polyMeshToPolygons() we choose to keep the Recast mesh to
   // build the zone connections in buildConnectionsRecastStyle()
   //
   // This polyToZoneMap is required for that
   Vector<S32> polyToZoneMap;
   polyToZoneMap.resize(mesh.npolys);

   // Generate BotNavMeshZone objects for each valid polygon
   for(S32 i = 0; i < mesh.npolys; i++)
   {
      S32 j = 0;
      botzone = NULL;

      while(j < mesh.nvp)
      {
         if(mesh.polys[(i * mesh.nvp + j)] == RC_MESH_NULL_IDX)
            break;

         const U16 *vert = &mesh.verts[mesh.polys[(i * mesh.nvp + j)] * bytesPerVertex];

         if(vert[0] == RC_MESH_NULL_IDX)
            break;

         if(botZoneDatabase->getObjectCount() >= MAX_ZONES)      // Don't add too many zones...
            break;

         if(j == 0)     // New poly, add new zone
         {
            // Create new zone, give it an ID that is +1 to the highest already in the database
            // This assumes zones are already added sequentially in this manner
            botzone = new BotNavMeshZone(botZoneDatabase->getObjectCount());

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
         botzone->addToZoneDatabase(botZoneDatabase);
         addedZones = true;
      }
   }

   // Repopulate allZones with the zones we modified above
   if(addedZones)
      populateZoneList(botZoneDatabase, allZones);


   // Build connections between zones
   // Build connections for standard zones
   buildConnectionsRecastStyle(allZones, mesh, polyToZoneMap,
         coreRecastPolyStartIdx, szRecastPolyStartIdx);

   // Teleporters require special connections
   linkConnectionsTeleporters(botZoneDatabase, teleporterData);

   // And SpeedZones, if they exist
   if(szRecastPolyStartIdx < polyToZoneMap.size())
   {
      S32 szBotZoneStartId = polyToZoneMap[szRecastPolyStartIdx];

      linkConnectionsSpeedZones(gameObjDatabase, botZoneDatabase, allZones,
            speedZoneList, speedZonePolygons, szBotZoneStartId);
   }

#ifdef LOG_TIMER
   U32 done3 = Platform::getRealMilliseconds();  // Done
   logprintf("Built %d zones!", botZoneDatabase->getObjectCount());
   logprintf("Timings: %d %d %d", done1-starttime, done2-done1, done3-done2);
#endif

   return true;
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


