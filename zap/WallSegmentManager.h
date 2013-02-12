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

#ifndef _WALL_SEGMENT_MANAGER_H_
#define _WALL_SEGMENT_MANAGER_H_

#include "BfObject.h"
#include "Point.h"
#include "tnlVector.h"
#include "tnlNetObject.h"

namespace Zap
{

class GameSettings;
class WallEdge;
class WallSegment;


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
   ~WallSegmentManager();  // Destructor

   GridDatabase *getWallSegmentDatabase();
   GridDatabase *getWallEdgeDatabase();

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
 
   ////////////////
   // Render functions
   void renderWalls(GameSettings *settings, F32 currentScale, bool dragMode, bool drawSelected, const Point &selectedItemOffset,
                    bool previewMode, bool showSnapVertices, F32 alpha);
};


};

#endif

