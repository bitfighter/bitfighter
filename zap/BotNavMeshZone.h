//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _BOT_NAV_MESH_ZONES_H_
#define _BOT_NAV_MESH_ZONES_H_

#include "gridDB.h"            // Parent
#include "../recast/Recast.h"  // for rcPolyMesh;

namespace Zap
{


////////////////////////////////////////
////////////////////////////////////////

class Border
{
public:
   Point borderStart;
   Point borderEnd;
};


////////////////////////////////////////
////////////////////////////////////////

class NeighboringZone : public Border
{
public:
   NeighboringZone();      // Constructor
   U16 zoneID;

   Point borderCenter;     // Simply a point half way between borderStart and borderEnd
   Point center;           // Center of zone
   F32 distTo;
};


class ServerGame;

////////////////////////////////////////
////////////////////////////////////////

class BotNavMeshZone : public DatabaseObject 
{
   typedef GeomObject Parent;

private:   
   U16 mZoneId;                                    // Unique ID for each zone

   static void populateZoneList(GridDatabase *mBotZoneDatabase, Vector<BotNavMeshZone *> *allZones);  // Populates allZones

public:
   explicit BotNavMeshZone(S32 id = -1);     // Constructor
   virtual ~BotNavMeshZone();                // Destructor
   
   static const S32 BufferRadius;            // Radius to buffer objects when creating the holes for zones
   static const S32 LevelZoneBuffer;         // Extra padding around the game extents to allow outsize zones to be created

   void renderLayer(S32 layerIndex);

   GridDatabase *getGameObjDatabase();
   void addToZoneDatabase(GridDatabase *botZoneDatabase);

   Point getCenter();      // Return center of zone

   // More precise boundary for precise collision detection
   const Vector<Point> *getCollisionPoly() const;

   // Only gets run on the server, never on client
   //bool collide(BfObject *hitObject);

   U16 getZoneId() { return mZoneId; }

   Vector<NeighboringZone> mNeighbors;       // List of other zones this zone touches, only populated on server
   Vector<Border> mNeighborRenderPoints;     // Only populated on client
   S32 getNeighborIndex(S32 zone);           // Returns index of neighboring zone, or -1 if zone is not a neighbor

   static bool buildBotMeshZones(GridDatabase *botZoneDatabase, Vector<BotNavMeshZone *> *allZones,
                                 const Rect *worldExtents, const Vector<DatabaseObject *> &barrierList,
                                 const Vector<DatabaseObject *> &turretList, const Vector<DatabaseObject *> &forceFieldProjectorList,
                                 const Vector<pair<Point, const Vector<Point> *> > &teleporterData, bool triangulateZones);

   static bool buildBotNavMeshZoneConnectionsRecastStyle(const Vector<BotNavMeshZone *> *allZones, 
                                                         rcPolyMesh &mesh, const Vector<S32> &polyToZoneMap);
   static void buildBotNavMeshZoneConnections(const Vector<BotNavMeshZone *> *allZones);
};


////////////////////////////////////////
////////////////////////////////////////

class AStar
{
private:
   static F32 heuristic(const Vector<BotNavMeshZone *> *zones, S32 fromZone, S32 toZone);
   static Point findGateway(const Vector<BotNavMeshZone *> *zones, S32 zone1, S32 zone2);

public:
   static Vector<Point> findPath (const Vector<BotNavMeshZone *> *zones, S32 startZone, S32 targetZone, const Point &target);
   
};


};


#endif


