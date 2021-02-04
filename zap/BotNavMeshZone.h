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
   U16 mZoneId;                              // Unique ID for each zone
   bool mWalkable;                           // Flag for if this zone can currently be traversed

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

   U16 getZoneId();
   bool getWalkable();
   void setWalkable(bool canWalk);

   Vector<NeighboringZone> mNeighbors;       // List of other zones this zone touches, only populated on server
   Vector<Border> mNeighborRenderPoints;     // Only populated on client
   S32 getNeighborIndex(S32 zone);           // Returns index of neighboring zone, or -1 if zone is not a neighbor

   static bool buildBotMeshZones(GridDatabase *botZoneDatabase, GridDatabase *gameObjDatabase, Vector<BotNavMeshZone *> *allZones,
                                 const Rect *worldExtents, bool triangulateZones);

   static bool buildConnectionsRecastStyle(const Vector<BotNavMeshZone *> *allZones,
                                                         rcPolyMesh &mesh, const Vector<S32> &polyToZoneMap);
   static void buildConnections(const Vector<BotNavMeshZone *> *allZones);

   static void buildConnectionsSpecial(const Vector<BotNavMeshZone *> *allZones,
         const Vector<Vector<Point> > &coreZones, const Vector<Vector<Point> > &speedZoneZones);
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


