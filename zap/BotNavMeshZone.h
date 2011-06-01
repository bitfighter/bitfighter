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

#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "UI.h"
#include "gameObjectRender.h"
#include "polygon.h"

#include "tnlNetBase.h"
#include "../glut/glutInclude.h"


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

class NeighboringZone : public Border
{
public:
   NeighboringZone() { zoneID = 0; distTo = 0; }     // Quickie constructor
   U16 zoneID;

   Point borderCenter;     // Simply a point half way between borderStart and borderEnd
   Point center;           // Center of zone
   F32 distTo;
};


class ZoneBorder : public Border
{
public:
   S32 mOwner1, mOwner2;      // IDs of border zones
};

////////////////////////////////////////
////////////////////////////////////////

class BotNavMeshZone : public GameObject
{
private:   
   typedef GameObject Parent;
   U16 mZoneId;            // Unique ID for each zone

public:
   bool flag;              // Flag used to mark zones during construction process, serves no purpose once zones have been generated

   static const S32 BufferRadius = Ship::CollisionRadius;  // Radius to buffer objects when creating the holes for zones
   static const S32 LEVEL_ZONE_BUFFER = 30;                // Extra padding around the game extents to allow outsize zones to be created

   BotNavMeshZone();       // Constructor
   ~BotNavMeshZone();      // Destructor

   void render(S32 layerIndex);

   S32 getRenderSortValue();


   // Create objects from parameters stored in level file
   bool processArguments(S32 argc, const char **argv);

   GridDatabase *getGridDatabase();
   void addToGame(Game *theGame);
   void onAddedToGame(Game *theGame);
   Point getCenter();      // Return center of zone

   // More precise boundary for precise collision detection
   bool getCollisionPoly(Vector<Point> &polyPoints);

   // Only gets run on the server, never on client
   bool collide(GameObject *hitObject);

   // These methods will be empty later...
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   U16 getZoneId() { return mZoneId; }
   void setZoneId(U16 zoneId) { mZoneId = zoneId; }

   Vector<NeighboringZone> mNeighbors;       // List of other zones this zone touches, only populated on server
   Vector<Border> mNeighborRenderPoints;     // Only populated on client
   S32 getNeighborIndex(S32 zone);           // Returns index of neighboring zone, or -1 if zone is not a neighbor

   static U16 findZoneContaining(const Point &p);    // Returns ID of zone containing specified point

   static bool buildBotMeshZones(Game *game);
   static void buildBotNavMeshZoneConnections(GridDatabase *zoneDb);
   static bool buildBotNavMeshZoneConnectionsRecastStyle(GridDatabase *zoneDb, rcPolyMesh &mesh, const Vector<S32> &polyToZoneMap);
   static void linkTeleportersBotNavMeshZoneConnections(Game *game);

   TNL_DECLARE_CLASS(BotNavMeshZone);
};


////////////////////////////////////////
////////////////////////////////////////

class AStar
{
private:
   static F32 heuristic(const Vector<BotNavMeshZone *> &zones, S32 fromZone, S32 toZone);
   static Point findGateway(const Vector<BotNavMeshZone *> &zones, S32 zone1, S32 zone2);

public:
   static Vector<Point> findPath (const Vector<BotNavMeshZone *> &zones, S32 startZone, S32 targetZone, const Point &target);
   
};


};


#endif


