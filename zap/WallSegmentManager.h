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


class WallSegmentManager
{
private:
   GridDatabase *mWallSegmentDatabase;
   GridDatabase *mWallEdgeDatabase;

   static bool mBatchUpdatingGeom;     

   void rebuildEdges();
   void buildWallSegmentEdgesAndPoints(GridDatabase *gameDatabase, DatabaseObject *object, const Vector<DatabaseObject *> &engrObjects);

public:
   WallSegmentManager();   // Constructor
   virtual ~WallSegmentManager();  // Destructor

   GridDatabase *getWallSegmentDatabase();
   GridDatabase *getWallEdgeDatabase();

   const Vector<Point> *getWallEdgePoints() const;
   const Vector<Point> *getSelectedWallEdgePoints() const;

   static void beginBatchGeomUpdate();                                     // Suspend certain geometry operations so they can be batched when 
   static void endBatchGeomUpdate(GridDatabase *db, bool modifiedWalls);   // this method is called

   void onWallGeomChanged(GridDatabase *editorDatabase, BfObject *wall, bool selected, S32 serialNumber);

   void finishedChangingWalls(GridDatabase *editorDatabase,  S32 changedWallSerialNumber);
   void finishedChangingWalls(GridDatabase *editorDatabase);

   Vector<Point> mWallEdgePoints;               // For rendering
   Vector<Point> mSelectedWallEdgePoints;       // Also for rendering

   void buildAllWallSegmentEdgesAndPoints(GridDatabase *gameDatabase);

   void clear();                                // Delete everything from everywhere!

   void clearSelected();
   void setSelected(S32 owner, bool selected);
   void rebuildSelectedOutline();

   void deleteSegments(S32 owner);              // Delete all segments owned by specified WorldItem

   void updateAllMountedItems(GridDatabase *database);


   // Takes a wall, finds all intersecting segments, and marks them invalid
   //void invalidateIntersectingSegments(GridDatabase *gameDatabase, BfObject *item);      // unused

   // Recalucate edge geometry for all walls when item has changed
   void computeWallSegmentIntersections(GridDatabase *gameDatabase, BfObject *item); 

   void recomputeAllWallGeometry(GridDatabase *gameDatabase);

   // Populate wallEdges
   void clipAllWallEdges(const Vector<DatabaseObject *> *wallSegments, Vector<Point> &wallEdges);
};


};

#endif

