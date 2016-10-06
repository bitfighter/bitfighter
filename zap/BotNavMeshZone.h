//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _BOT_NAV_MESH_ZONES_H_
#define _BOT_NAV_MESH_ZONES_H_

#include "gridDB.h"            // Parent
#include "../recast/Recast.h"  // for rcPolyMesh;

#include "Test.h"

struct sqlite3_stmt;

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
   // These are from Parent:
   // Point borderStart;
   // Point borderEnd;

   U16 zoneID;

   Point borderCenter;     // Simply a point half way between borderStart and borderEnd
   Point center;           // Center of zone
   F32 distTo;

   NeighboringZone();      // Constructor
   NeighboringZone(S32 zoneId, const Point &bordStart, const Point &bordEnd, const Point &selfCenter, const Point &otherCenter);

   static bool bindDataAndRun(sqlite3_stmt *pStmt, U64 sqliteLevelInfoId, S32 order, S32 originZoneId, S32 destZoneId,
                       const Point &borderStart, const Point &borderEnd);
};

class ServerGame;

////////////////////////////////////////
////////////////////////////////////////

class BotNavMeshZone : public DatabaseObject 
{
   typedef GeomObject Parent;

private:   
   U16 mZoneId;                              // Unique ID for each zone

   Vector<NeighboringZone> mNeighbors;       // List of other zones this zone touches, only populated on server
   Vector<Border> mNeighborRenderPoints;     // Only populated on client

   static void populateZoneList(GridDatabase *mBotZoneDatabase, Vector<BotNavMeshZone *> *allZones);  // Populates allZones
   static bool saveBotZonesToSqlite(const string &databaseName, const Vector<BotNavMeshZone *> &allZones, U64 sqliteLevelInfoId);
   static bool tryToLoadZonesFromSqlite(const string &databaseName, U64 sqliteLevelInfoId, Vector<BotNavMeshZone *> &allZones);
   static bool clearZonesFromDatabase(sqlite3 *sqliteDb, U64 sqliteLevelInfoId);

public:
   explicit BotNavMeshZone(S32 id = -1);     // Constructor
   virtual ~BotNavMeshZone();                // Destructor
   Vector<NeighboringZone> &getNeighbors();
   static const S32 BufferRadius;            // Radius to buffer objects when creating the holes for zones
   static const S32 LevelZoneBuffer;         // Extra padding around the game extents to allow outsize zones to be created

   void renderLayer(S32 layerIndex) const;

   //GridDatabase *getGameObjDatabase();
   void addToZoneDatabase(GridDatabase *botZoneDatabase);
   bool writeZoneToSqlite(sqlite3_stmt *zones_pStmt, sqlite3_stmt *neighbors_pStmt, U64 sqliteLevelInfoId);


   Point getCenter() const;      // Return center of zone

   // More precise boundary for precise collision detection
   const Vector<Point> *getCollisionPoly() const;
   void addNeighbor(const NeighboringZone neighbor);

   // Only gets run on the server, never on client
   //bool collide(BfObject *hitObject);

   U16 getZoneId() { return mZoneId; }

   S32 getNeighborIndex(S32 zone);           // Returns index of neighboring zone, or -1 if zone is not a neighbor

   static bool buildBotMeshZones(GridDatabase &botZoneDatabase, Vector<BotNavMeshZone *> &allZones,
                                 const Rect *worldExtents, const Vector<DatabaseObject *> &barrierList,
                                 const Vector<DatabaseObject *> &turretList, const Vector<DatabaseObject *> &forceFieldProjectorList,
                                 const Vector<pair<Point, const Vector<Point> *> > &teleporterData, bool triangulateZones, U64 sqliteLevelInfoId);

   static S32 calcLevelSize     (const Rect *worldExtents, const Vector<DatabaseObject *> &barrierList,
                                 const Vector<pair<Point, const Vector<Point> *> > &teleporterData);

   static bool buildBotNavMeshZoneConnectionsRecastStyle(const Vector<BotNavMeshZone *> &allZones, 
                                                         rcPolyMesh &mesh, const Vector<S32> &polyToZoneMap);
   static void buildBotNavMeshZoneConnections(const Vector<BotNavMeshZone *> &allZones);

   // Test access
   FRIEND_TEST(DatabaseTest, GeometryRoundTrip);
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


