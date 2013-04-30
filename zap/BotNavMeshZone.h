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

#ifndef _BOTNAVMESHZONES_H_
#define _BOTNAVMESHZONES_H_

#include "BfObject.h"    // For base classes
#include "polygon.h"
#include "ship.h"

#include "tnlNetBase.h"


namespace Zap
{

class ServerGame;

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


////////////////////////////////////////
////////////////////////////////////////

class BotNavMeshZone : public DatabaseObject 
{
   typedef GeomObject Parent;

private:   
   U16 mZoneId;                                    // Unique ID for each zone

   static Vector<BotNavMeshZone *> mAllZones;   
   static void populateZoneList();                 // Populates mAllZones

public:
   explicit BotNavMeshZone(S32 id = -1);    // Constructor
   virtual ~BotNavMeshZone();               // Destructor
   
   static const S32 BufferRadius = Ship::CollisionRadius;  // Radius to buffer objects when creating the holes for zones
   static const S32 LEVEL_ZONE_BUFFER = 30;                // Extra padding around the game extents to allow outsize zones to be created

   void render(S32 layerIndex);

   GridDatabase *getGameObjDatabase();
   void addToZoneDatabase();

   static GridDatabase *getBotZoneDatabase();

   static void createBotZoneDatabase();
   static void deleteBotZoneDatabase();

   Point getCenter();      // Return center of zone

   // More precise boundary for precise collision detection
   const Vector<Point> *getCollisionPoly() const;

   // Only gets run on the server, never on client
   //bool collide(BfObject *hitObject);

   U16 getZoneId() { return mZoneId; }

   Vector<NeighboringZone> mNeighbors;       // List of other zones this zone touches, only populated on server
   Vector<Border> mNeighborRenderPoints;     // Only populated on client
   S32 getNeighborIndex(S32 zone);           // Returns index of neighboring zone, or -1 if zone is not a neighbor

   static U16 findZoneContaining(GridDatabase *botZoneDatabase, const Point &p);     // Returns ID of zone containing specified point

   static const Vector<BotNavMeshZone *> *getBotZones();                // Return cached list of all zones

   static bool buildBotMeshZones(ServerGame *game, bool triangulateZones);
   static void buildBotNavMeshZoneConnections(ServerGame *game);
   static bool buildBotNavMeshZoneConnectionsRecastStyle(ServerGame *game, rcPolyMesh &mesh, const Vector<S32> &polyToZoneMap);
   static void linkTeleportersBotNavMeshZoneConnections(ServerGame *game);

   //TNL_DECLARE_CLASS(BotNavMeshZone);
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


