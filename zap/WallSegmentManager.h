//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _WALL_SEGMENT_MANAGER_H_
#define _WALL_SEGMENT_MANAGER_H_

#include "Point.h"

#include "tnlVector.h"
#include "tnlNetObject.h"

namespace Zap
{

class GameSettings;
class WallEdge;
class WallSegment;
class GridDatabase;
class DatabaseObject;
class BfObject;
class EngineeredItem;


class WallSegmentManager
{
private:
   GridDatabase *mWallSegmentDatabase;
   GridDatabase *mWallEdgeDatabase;

   bool mBatchUpdatingGeom;     

   void rebuildEdges();
   void buildWallSegmentEdgesAndPoints(GridDatabase *gameDatabase, 
                                       DatabaseObject *object);
public:

   WallSegmentManager();   // Constructor
   virtual ~WallSegmentManager();  // Destructor

   GridDatabase *getWallSegmentDatabase() const;
   GridDatabase *getWallEdgeDatabase() const;

   const Vector<Point> *getWallEdgePoints() const;
   const Vector<Point> *getSelectedWallEdgePoints() const;

   void beginBatchGeomUpdate();                                     // Suspend certain geometry operations so they can be batched when 
   void endBatchGeomUpdate(GridDatabase *db, bool modifiedWalls);   // this method is called

   void onWallGeomChanged(GridDatabase *editorDatabase, BfObject *wall, bool selected, S32 serialNumber);

   void finishedChangingWalls(GridDatabase *editorDatabase,  S32 changedWallSerialNumber);
   void finishedChangingWalls(GridDatabase *editorDatabase);

   Vector<Point> mWallEdgePoints;               // For rendering
   Vector<Point> mSelectedWallEdgePoints;       // Also for rendering

   void buildAllWallSegmentEdgesAndPoints(GridDatabase *database, const Vector<Zap::DatabaseObject *> &walls);

   void clear();                                // Delete everything from everywhere!

   void clearSelected();
   void setSelected(S32 owner, bool selected);
   void rebuildSelectionOutline();

   void deleteSegments(S32 owner);              // Delete all segments owned by specified WorldItem

   void updateAllMountedItems(GridDatabase *database);

   // Recalucate edge geometry for all walls when item has changed
   void computeWallSegmentIntersections(GridDatabase *gameDatabase, BfObject *item); 

   void recomputeAllWallGeometry(GridDatabase *database);
   void recomputeAllWallGeometry(GridDatabase *database, const Vector<Zap::DatabaseObject *> &walls);

   // Populate wallEdges
   static void clipAllWallEdges(const Vector<DatabaseObject *> *wallSegments, Vector<Point> &wallEdges);
};


};

#endif

