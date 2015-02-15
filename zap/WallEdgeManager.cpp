//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "WallEdgeManager.h"

#include "barrier.h"
#include "WallItem.h"

#include "EngineeredItem.h"      // For forcefieldprojector def ==> probably should not be here

#include "GeomUtils.h"

using namespace TNL;

namespace Zap
{


// Constructor
WallEdgeManager::WallEdgeManager()
{
   mBatchUpdatingGeom  = false;
}


// Destructor
WallEdgeManager::~WallEdgeManager()
{
   // Do nothing
}


void WallEdgeManager::beginBatchGeomUpdate()
{
   mBatchUpdatingGeom = true;
}


void WallEdgeManager::endBatchGeomUpdate(GridDatabase *gameObjectDatabase, 
                                         const Vector<WallSegment const *> &wallSegments, 
                                         Vector<Point> &wallEdgePoints,    // <== gets modified!
                                         bool modifiedWalls)
{
   if(modifiedWalls)
      finishedChangingWalls(gameObjectDatabase, wallSegments, wallEdgePoints);

   mBatchUpdatingGeom = false;
}


const GridDatabase *WallEdgeManager::getWallEdgeDatabase() const
{
   return &mWallEdgeDatabase;
}


// Delete all segments, then find all walls and build a new set of segments
void WallEdgeManager::rebuildEdges(const Vector<WallSegment const *> &wallSegments, Vector<Point> &wallEdgePoints)
{
   // Clear the edges
   mWallEdgeDatabase.removeEverythingFromDatabase();

   // Iterate over all our wall objects (WallItems and PolyWalls when run from the editor, Barriers when run from ServerGame::loadLevel)
   // This should already be done!
   //for(S32 i = 0; i < walls.size(); i++)
   //   buildWallSegmentEdgesAndPoints(walls[i]);

   rebuildEdgesWithClipper(wallSegments, wallEdgePoints);
}


// Take geometry from all wall segments, and run them through clipper to generate new edge geometry.  Then use the results to create
// a bunch of WallEdge objects, which will be stored in mWallEdgeDatabase for future reference.  The two key things to understand here
// are that 1) it's all-or-nothing: all edges need to be recomputed at once; there is no way to do a partial rebuild.  And 2) the edges
// cannot be associated with their source, so we'll need to rely on other tricks to find an associated wall when needed.
// Private method
void WallEdgeManager::rebuildEdgesWithClipper(const Vector<WallSegment const *> &wallSegments, Vector<Point> &wallEdgePoints)
{
   // Data flow in this method: wallSegments -> wallEdgePoints -> wallEdges

   wallEdgePoints.clear();

   // Run clipper --> fills wallEdgePoints from wallSegments
   clipAllWallEdges(wallSegments, wallEdgePoints);

   // Create a WallEdge object from the clipped wall geometry.  We'll add it to the WallEdgeDatabase, which will 
   // delete the object when it is ulitmately removed.
   mWallEdgeDatabase.removeEverythingFromDatabase();    // Remove the old edges

   for(S32 i = 0; i < wallEdgePoints.size(); i+=2)
   {
      WallEdge *newEdge = new WallEdge(wallEdgePoints[i], wallEdgePoints[i+1]);   // Create the edge object
      newEdge->addToDatabase(&mWallEdgeDatabase);                                 // And add it to the database
   }
}


//// Find the associated segment(s) and mark them as selected (or not)
//void WallEdgeManager::onWallGeomChanged(GridDatabase *gameObjectDatabase, BfObject *wall, bool selected, S32 serialNumber)
//{
//   if(!mBatchUpdatingGeom)
//      finishedChangingWalls(gameObjectDatabase, serialNumber);
//}


//// This variant only resnaps engineered items that were attached to a segment that moved
//void WallEdgeManager::finishedChangingWalls(GridDatabase *editorObjectDatabase, S32 changedWallSerialNumber)
//{
//   rebuildEdgesWithClipper();         // Rebuild all edges for all walls
//
//   // This block is a modified version of updateAllMountedItems that homes in on a particular segment
//   // First, find any items directly mounted on our wall, and update their location.  Because we don't know where the wall _was_, we 
//   // will need to search through all the engineered items, and query each to find which ones where attached to the wall that moved.
//   fillVector.clear();
//   editorObjectDatabase->findObjects((TestFunc)isEngineeredType, fillVector);
//
//   for(S32 i = 0; i < fillVector.size(); i++)
//   {
//      EngineeredItem *engrItem = static_cast<EngineeredItem *>(fillVector[i]);
//
//      // Remount any engr items that were either not attached to any wall, or were attached to any segments on the modified wall
//      if(engrItem->getMountSegment() == NULL || engrItem->getMountSegment()->getOwner() == changedWallSerialNumber)
//         engrItem->mountToWall(engrItem->getVert(0), editorObjectDatabase->getWallSegmentManager(), NULL);
//
//      // Calculate where all ffs land -- no telling if the segment we moved is or was interfering in its path
//      if(engrItem->getObjectTypeNumber() == ForceFieldProjectorTypeNumber)
//      {
//         ForceFieldProjector *ffp = static_cast<ForceFieldProjector *>(engrItem);
//         ffp->findForceFieldEnd();
//      }
//   }
//
//   rebuildSelectionOutline();
//}


void WallEdgeManager::finishedChangingWalls(GridDatabase *gameObjectDatabase, 
                                            const Vector<WallSegment const *> &wallSegments, 
                                            Vector<Point> &wallEdgePoints)
{
   rebuildEdgesWithClipper(wallSegments, wallEdgePoints);         // Rebuild all edges for all walls
   updateAllMountedItems(gameObjectDatabase);
}


//// These functions clear the WallSegment database, and refills it with the output of clipper
//void WallEdgeManager::rebuildEdges(GridDatabase *database)
//{
//   fillVector.clear();
//   database->findObjects((TestFunc)isWallType, fillVector);
//
//   recomputeAllWallGeometry(database, fillVector);
//}


// Given a wall, build all the segments and related geometry; also manage any affected mounted items
// Operates only on passed wall segment -- does not alter others
// Static method
void WallEdgeManager::buildWallSegmentEdgesAndPoints(DatabaseObject *wallDbObject)
{
   //// Find any engineered objects that terminate on this wall, and mark them for resnapping later
   //TNLAssert(dynamic_cast<BfObject *>(wallDbObject), "Can't cast to BfObject!");

   //BfObject *wall = static_cast<BfObject *>(wallDbObject);     // Wall we're deleting and rebuilding

   //// A list of engr objects mounted or terminating on the wall segment that we'll be deleting -- these will
   //// need to be remounted
   //Vector<EngineeredItem *> toBeRemounted;    

   //Rect allSegExtent;

 
   ////// The final thing we could have here is a barrier, which is either a polywall or a single segment of a normal wall 
   ////// that has been converted to a 4-point rectanglular polygon
   ////// TODO -- this is really test code; if it works, we need to clean it up and optimize it
   ////else
   ////{
   ////   TNLAssert(dynamic_cast<Barrier *>(wall), "Expected a Barrier!");

   ////   Barrier *barrier = static_cast<Barrier *>(wall);

   ////   Vector<Point> pts;

   ////   for(S32 i = 0; i < barrier->getVertCount(); i++)
   ////   {
   ////      pts.push_back(barrier->getVert(i));

   ////      if(i == 0)
   ////         allSegExtent.set(barrier->getVert(i), 0);
   ////      else
   ////         allSegExtent.unionRect(Rect(barrier->getVert(i), 0));
   ////   }

   ////   // Create the segment; the WallSegment constructor will add it to the specified database
   ////   WallSegment *newSegment = new WallSegment(mWallSegmentDatabase, pts, barrier->getSerialNumber());

   ////   wallDbObject->setExtent(allSegExtent);      // A wall's extent is the union of the extents of all its segments.  Makes sense, right?
   ////}

   //// Remount all turrets & forcefields mounted on or terminating on any of the wall segments we deleted and potentially recreated
   //for(S32 i = 0; i < toBeRemounted.size(); i++)  
   //   toBeRemounted[i]->mountToWall(toBeRemounted[i]->getVert(0), database->getWallSegmentManager(), NULL);
}


// Used above and from instructions -- static method
void WallEdgeManager::clipAllWallEdges(const Vector<WallSegment const *> &wallSegments, Vector<Point> &wallEdges)
{
   S32 count = wallSegments.size();

   if(count == 0)
   {
      wallEdges.clear();
      return;
   }

   Vector<const Vector<Point> *> inputPolygons;
   Vector<Vector<Point> > solution;

   for(S32 i = 0; i < count; i++)
      inputPolygons.push_back(wallSegments[i]->getCorners());

   mergePolys(inputPolygons, solution);      // Merged wall segments are placed in solution

   unpackPolygons(solution, wallEdges);
}


// Called by WallItems and PolyWalls when their geom changes
void WallEdgeManager::updateAllMountedItems(const GridDatabase *gameObjectDatabase)
{
   // First, find any items directly mounted on our wall, and update their location.  Because we don't know where the wall _was_, we 
   // will need to search through all the engineered items, and query each to find which ones where attached to the wall that moved.
   fillVector.clear();
   gameObjectDatabase->findObjects((TestFunc)isEngineeredType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      EngineeredItem *engrItem = static_cast<EngineeredItem *>(fillVector[i]);
      engrItem->mountToWall(engrItem->getVert(0), gameObjectDatabase, &mWallEdgeDatabase);
   }
}


void WallEdgeManager::clear()
{
   mWallEdgeDatabase.removeEverythingFromDatabase();
}


};
