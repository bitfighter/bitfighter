//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _WALL_EDGE_MANAGER_H_
#define _WALL_EDGE_MANAGER_H_

#include "gridDB.h"
#include "Point.h"

#include "tnlVector.h"
#include "tnlNetObject.h"

namespace Zap
{

class GameSettings;
class WallEdge;
class WallSegment;
class DatabaseObject;
class BfObject;
class EngineeredItem;


class WallEdgeManager
{
private:
   bool mBatchUpdatingGeom;     

   GridDatabase mWallEdgeDatabase;

   void rebuildEdgesWithClipper(const Vector<WallSegment const *> &wallSegments, Vector<Point> &wallEdges);

public:
   WallEdgeManager();            // Constructor
   virtual ~WallEdgeManager();   // Destructor

   const GridDatabase *getWallEdgeDatabase() const;

   // Suspend certain geometry operations for greater efficiency
   void beginBatchGeomUpdate();                                     
   void endBatchGeomUpdate(GridDatabase *gameObjectDatabase, 
                           const Vector<WallSegment const *> &wallSegments, 
                           Vector<Point> &wallEdgePoints,
                           bool modifiedWalls);

   //void onWallGeomChanged(GridDatabase *editorDatabase, BfObject *wall, bool selected, S32 serialNumber);

   //void finishedChangingWalls(GridDatabase *editorDatabase,  S32 changedWallSerialNumber);
   void finishedChangingWalls(GridDatabase *gameObjectDatabase, 
                                            const Vector<WallSegment const *> &wallSegments, 
                                            Vector<Point> &wallEdgePoints);

   void buildAllWallSegmentEdgesAndPoints(GridDatabase *database, const Vector<Zap::DatabaseObject *> &walls);

   void clear();                                // Delete everything from everywhere!

   void updateAllMountedItems(const GridDatabase *gameObjectDatabase);

   //void rebuildEdges(GridDatabase *database);
   void rebuildEdges(const Vector<WallSegment const *> &wallSegments, Vector<Point> &wallEdgePoints);
   static void buildWallSegmentEdgesAndPoints(DatabaseObject *object);


   // Populate wallEdges
   static void clipAllWallEdges(const Vector<WallSegment const *> &wallSegments, Vector<Point> &wallEdges);
};


};

#endif

