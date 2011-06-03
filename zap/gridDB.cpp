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

#include "gridDB.h"
#include "gameObject.h"
#include "moveObject.h"    // For def of ActualState
#include "EditorObject.h"  // For def of EditorObject

namespace Zap
{

// Constructor
GridDatabase::GridDatabase(bool usingGameCoords)
{
   mUsingGameCoords = usingGameCoords;

   mQueryId = 0;
   BucketWidth = usingGameCoords ? 255 : 1;

   for(U32 i = 0; i < BucketRowCount; i++)
      for(U32 j = 0; j < BucketRowCount; j++)
         mBuckets[i][j] = 0;
}


void GridDatabase::addToDatabase(DatabaseObject *theObject, const Rect &extents)
{
   S32 minx, miny, maxx, maxy;  
   F32 widthDiv = 1 / F32(BucketWidth);

   minx = S32(extents.min.x * widthDiv);
   miny = S32(extents.min.y * widthDiv);
   maxx = S32(extents.max.x * widthDiv);
   maxy = S32(extents.max.y * widthDiv);

   if(maxx > minx + BucketRowCount)
      maxx = minx + BucketRowCount;
   if(maxy > miny + BucketRowCount)
      maxy = miny + BucketRowCount;

   for(S32 x = minx; x <= maxx; x++)
      for(S32 y = miny; y <= maxy; y++)
      {
         BucketEntry *be = mChunker.alloc();
         be->theObject = theObject;
         be->nextInBucket = mBuckets[x & BucketMask][y & BucketMask];
         mBuckets[x & BucketMask][y & BucketMask] = be;
      }

   // Add the object to our non-spatial "database" as well
   mAllObjects.push_back(theObject);
}


void GridDatabase::removeFromDatabase(DatabaseObject *theObject, const Rect &extents)
{
   S32 minx, miny, maxx, maxy;
   F32 widthDiv = 1 / F32(BucketWidth);

   minx = S32(extents.min.x * widthDiv);
   miny = S32(extents.min.y * widthDiv);
   maxx = S32(extents.max.x * widthDiv);
   maxy = S32(extents.max.y * widthDiv);

   if(maxx > minx + BucketRowCount)
      maxx = minx + BucketRowCount;
   if(maxy > miny + BucketRowCount)
      maxy = miny + BucketRowCount;


   for(S32 x = minx; x <= maxx; x++)
   {
      for(S32 y = miny; y <= maxy; y++)
      {
         for(BucketEntry **walk = &mBuckets[x & BucketMask][y & BucketMask]; *walk; walk = &((*walk)->nextInBucket))
         {
            if((*walk)->theObject == theObject)
            {
               BucketEntry *rem = *walk;
               *walk = rem->nextInBucket;
               mChunker.free(rem);
               break;
            }
         }
      }
   }

   // Remove the object to our non-spatial "database" as well
   for(S32 i = 0; i < mAllObjects.size(); i++)
      if(mAllObjects[i] == theObject)
      {
         mAllObjects.erase_fast(i);
         break;
      }
}


void GridDatabase::findObjects(U32 typeMask, Vector<DatabaseObject *> &fillVector, const Rect *extents, S32 minx, S32 miny, S32 maxx, S32 maxy, U8 typeNumber)
{
   mQueryId++;    // Used to prevent the same item from being found in multiple buckets

   for(S32 x = minx; x <= maxx; x++)
      for(S32 y = miny; y <= maxy; y++)
         for(BucketEntry *walk = mBuckets[x & BucketMask][y & BucketMask]; walk; walk = walk->nextInBucket)
         {
            DatabaseObject *theObject = walk->theObject;

            if(theObject->mLastQueryId != mQueryId &&                                                             // Object hasn't been queried; and
               ((theObject->getObjectTypeMask() & typeMask) || theObject->getObjectTypeNumber() == typeNumber) && // is of the right type; and
               (!extents || theObject->extent.intersects(*extents)) )                                             // overlaps our extents (if passed)
            {
               walk->theObject->mLastQueryId = mQueryId;    // Flag the object so we know we've already visited it
               fillVector.push_back(walk->theObject);       // And save it as a found item
            }
         }
}


void GridDatabase::findObjects(Vector<DatabaseObject *> &fillVector)
{
   for(S32 i = 0; i < mAllObjects.size(); i++)
      fillVector.push_back(mAllObjects[i]);
}


//const Vector<DatabaseObject *> *GridDatabase::getObjectList()
//{
//   return &mAllObjects;
//}


// Find all objects in database of type typeMask
void GridDatabase::findObjects(U32 typeMask, Vector<DatabaseObject *> &fillVector, U8 typeNumber)
{
   for(S32 i = 0; i < mAllObjects.size(); i++)
      if((mAllObjects[i]->getObjectTypeMask() & typeMask) || (mAllObjects[i]->getObjectTypeNumber() == typeNumber))
         fillVector.push_back(mAllObjects[i]);
}


// Find all objects in &extents that are of type typeMask
void GridDatabase::findObjects(U32 typeMask, Vector<DatabaseObject *> &fillVector, const Rect &extents, U8 typeNumber)
{
   S32 minx, miny, maxx, maxy;

   F32 widthDiv = 1 / F32(BucketWidth);

   minx = S32(extents.min.x * widthDiv);
   miny = S32(extents.min.y * widthDiv);
   maxx = S32(extents.max.x * widthDiv);
   maxy = S32(extents.max.y * widthDiv);

   if(maxx >= minx + BucketRowCount)
      maxx = minx + BucketRowCount - 1;
   if(maxy >= miny + BucketRowCount)
      maxy = miny + BucketRowCount - 1;

   findObjects(typeMask, fillVector, &extents, minx, miny, maxx, maxy, typeNumber);
}


// Find objects along a ray, returning first discovered object, along with time of
// that collision and a Point representing the normal angle at intersection point
//             (at least I think that's what's going on here - CE)
DatabaseObject *GridDatabase::findObjectLOS(U32 typeMask, U32 stateIndex, 
                                            const Point &rayStart, const Point &rayEnd, 
                                            float &collisionTime, Point &surfaceNormal, U8 typeNumber)
{
   return findObjectLOS(typeMask, stateIndex, true, rayStart, rayEnd, collisionTime, surfaceNormal, typeNumber);
}


// Format is a passthrough to polygonLineIntersect().  Will be true for most items, false for walls in editor.
DatabaseObject *GridDatabase::findObjectLOS(U32 typeMask, U32 stateIndex, bool format,
                                            const Point &rayStart, const Point &rayEnd, 
                                            float &collisionTime, Point &surfaceNormal, U8 typeNumber)
{
   Rect queryRect(rayStart, rayEnd);

   static Vector<DatabaseObject *> fillVector;

   fillVector.clear();

   findObjects(typeMask, fillVector, queryRect, typeNumber);

   Point collisionPoint;

   collisionTime = 100;
   DatabaseObject *retObject = NULL;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      if(!fillVector[i]->isCollisionEnabled())     // Skip collision-disabled objects
         continue;

      static Vector<Point> poly;
      poly.clear();
      Point center;
      F32 radius;
      float ct;

      if(fillVector[i]->getCollisionPoly(poly))
      {
         if(poly.size() == 0)    // This can happen in the editor when a wall segment is completely hidden by another
            continue;

         Point normal;
         if(polygonIntersectsSegmentDetailed(&poly[0], poly.size(), format, rayStart, rayEnd, ct, normal))
         {
            if(ct < collisionTime)
            {
               collisionTime = ct;
               retObject = fillVector[i];
               surfaceNormal = normal;
            }
         }
      }
      else if(fillVector[i]->getCollisionCircle(stateIndex, center, radius))
      {
         if(circleIntersectsSegment(center, radius, rayStart, rayEnd, ct))
         {
            if(ct < collisionTime)
            {
               collisionTime = ct;
               surfaceNormal = (rayStart + (rayEnd - rayStart) * ct) - center;
               retObject = fillVector[i];
            }
         }
      }
   }

   if(retObject)
      surfaceNormal.normalize();

   return retObject;
}


bool GridDatabase::pointCanSeePoint(const Point &point1, const Point &point2)
{
   F32 time;
   Point coll;

   return( findObjectLOS(BarrierType, MoveObject::ActualState, true, point1, point2, time, coll) == NULL );
}


// Kind of hacky, kind of useful.  Only used by BotZones, and ony works because all zones are added at one time, the list does not change,
// and the index of the bot zones is stored as an ID by the zone.  If we added and removed zones from our list, this would probably not
// be a reliable way to access a specific item.  We could probably phase this out by passing pointers to zones rather than indices.
DatabaseObject *GridDatabase::getObjectByIndex(S32 index) 
{  
   if(index < 0 || index >= mAllObjects.size())
      return NULL;
   else
      return mAllObjects[index]; 
} 



////////////////////////////////////////
////////////////////////////////////////

void DatabaseObject::addToDatabase()
{
   if(!mInDatabase)
   {
      mInDatabase = true;
      getGridDatabase()->addToDatabase(this, extent);
   }
}


void DatabaseObject::removeFromDatabase()
{
   if(mInDatabase)
   {
      mInDatabase = false;
      getGridDatabase()->removeFromDatabase(this, extent);
   }
}


// Update object's extents in the database -- will not add object to database if it's not already in it
void DatabaseObject::setExtent(const Rect &extents)
{
   GridDatabase *gridDB = getGridDatabase();

   if(mInDatabase && gridDB != NULL)
   {
      // Remove from the extents database for current extents...
      gridDB->removeFromDatabase(this, extent);    // old extent
      // ...and re-add for the new extent
      gridDB->addToDatabase(this, extents);
   }

   extent.set(extents);
}


////////////////////////////////////////
////////////////////////////////////////

class EditorObject;

// Constructor
EditorObjectDatabase::EditorObjectDatabase(bool usingGameCoords) : Parent(usingGameCoords)
{
   // Do nothing, just here to call Parent's constructor
}


void EditorObjectDatabase::addToDatabase(DatabaseObject *object, const Rect &extents)
{
   //EditorObject *eObj = (EditorObject *)(object);
   EditorObject *eObj = dynamic_cast<EditorObject *>(object);
   TNLAssert(eObj, "Bad cast!");

   Parent::addToDatabase(object, extents);

   mAllEditorObjects.push_back(eObj);
}


void EditorObjectDatabase::removeFromDatabase(EditorObject *object, const Rect &extents)
{
   Parent::removeFromDatabase((DatabaseObject *)object, extents);

   // Remove the object to our list as well
   for(S32 i = 0; i < mAllEditorObjects.size(); i++)
      if(mAllEditorObjects[i] == object)
      {
         mAllEditorObjects.erase_fast(i);
         break;
      }
}


// Provide a read-only shortcut to a pre-cast list of editor objects
const Vector<EditorObject *> *EditorObjectDatabase::getObjectList()
{
   return &mAllEditorObjects;
}


};


