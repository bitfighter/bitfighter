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

#include <map>

namespace Zap
{

//class BucketEntry;

U32 GridDatabase::mQueryId = 0;
ClassChunker<GridDatabase::BucketEntry> GridDatabase::mChunker;

// Constructor
GridDatabase::GridDatabase()
{
   mQueryId = 0;

   for(U32 i = 0; i < BucketRowCount; i++)
      for(U32 j = 0; j < BucketRowCount; j++)
         mBuckets[i][j] = NULL;

   printf("Creating database %p\n", this);
}



// Destructor
GridDatabase::~GridDatabase()       
{
   // Do nothing for the moment
   logprintf("destroying database %p", this);
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
   // Working backwards makes clear() go faster, and should have little effect on the case of removing an arbitrary object
   for(S32 i = mAllObjects.size() - 1; i >= 0 ; i--)
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
   fillVector.reserve(mAllObjects.size());
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


////////////////////////////////////////
////////////////////////////////////////

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
   if(mInDatabase || !getIsDatabasable())
      return;

   mInDatabase = true;
   getGridDatabase()->addToDatabase(this, extent);
}


void DatabaseObject::removeFromDatabase()
{
   if(!mInDatabase)
      return;

   mInDatabase = false;
   getGridDatabase()->removeFromDatabase(this, extent);
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

// This sort will put points on top of lines on top of polygons...  as they should be
// We'll also put walls on the bottom, as this seems to work best in practice
bool QSORT_CALLBACK geometricSort(EditorObject * &a, EditorObject * &b)
{
   if(a->getObjectTypeMask() & BarrierType)
      return true;
   if(b->getObjectTypeMask() & BarrierType)
      return false;

   return( a->getGeomType() > b->getGeomType() );
}


static void geomSort(Vector<EditorObject *> &objects)
{
   if(objects.size() >= 2)  // nothing to sort when there is one or zero objects
      // Cannot use Vector.sort() here because I couldn't figure out how to cast shared_ptr as pointer (*)
      //sort(objects.getStlVector().begin(), objects.getStlVector().begin() + objects.size(), geometricSort);
      qsort(&objects[0], objects.size(), sizeof(EditorObject *), (qsort_compare_func) geometricSort);
}


////////////////////////////////////////
////////////////////////////////////////

class EditorObject;

// Constructor
EditorObjectDatabase::EditorObjectDatabase() : Parent()
{
   // Do nothing, just here to call Parent's constructor
}

// Copy constructor
EditorObjectDatabase::EditorObjectDatabase(const EditorObjectDatabase &database)
{
   copy(database);
}


EditorObjectDatabase &EditorObjectDatabase::operator= (const EditorObjectDatabase &database)
{
   copy(database);
   return *this;
}


typedef map<DatabaseObject *, EditorObject *> dbMap;

static DatabaseObject *getObject(dbMap &dbObjectMap, DatabaseObject *theObject)
{
   // Check if this object has already been found
   dbMap::iterator iter = dbObjectMap.find(theObject);

   if(iter != dbObjectMap.end()) // Found an existing copy -- use that
      return iter->second;

   else                          // This is a new object, copy and add to our map
   {
      EditorObject *newObject = dynamic_cast<EditorObject *>(theObject)->newCopy();

      pair<dbMap::iterator, bool> retval;
      retval = dbObjectMap.insert(pair<DatabaseObject *, EditorObject *>(theObject, newObject));

      TNLAssert(retval.second, "Invalid attempt to insert object into map (duplicate insert?)");

      return newObject;
   }
}
 

// Copy contents of source into this
void EditorObjectDatabase::copy(const EditorObjectDatabase &source)
{
   dbMap dbObjectMap;

   for(U32 x = 0; x < BucketRowCount; x++)
      for(U32 y = 0; y < BucketRowCount; y++)
      {
         mBuckets[x & BucketMask][y & BucketMask] = NULL;

         for(BucketEntry *walk = source.mBuckets[x & BucketMask][y & BucketMask]; walk; walk = walk->nextInBucket)
         {
            BucketEntry *be = mChunker.alloc();                // Create a slot for our new object
            DatabaseObject *theObject = walk->theObject;

            DatabaseObject *object = getObject(dbObjectMap, theObject);    // Returns a pointer to a new or existing copy of theObject
            be->theObject = object;

            be->nextInBucket = mBuckets[x & BucketMask][y & BucketMask];
            mBuckets[x & BucketMask][y & BucketMask] = be;
         }
      }

   // Copy our non-spatial databases as well
   mAllEditorObjects.resize(source.mAllEditorObjects.size());
   mAllObjects.resize(source.mAllEditorObjects.size());

   for(S32 i = 0; i < source.mAllEditorObjects.size(); i++)
   {
      dbMap::iterator iter = dbObjectMap.find(source.mAllEditorObjects[i]);
      TNLAssert(iter != dbObjectMap.end(), "Could not find object in our database copy map!") ;

      mAllEditorObjects[i] = iter->second;
      mAllObjects[i] = iter->second;
   }
}


void EditorObjectDatabase::addToDatabase(DatabaseObject *object, const Rect &extents)
{
   //EditorObject *eObj = (EditorObject *)(object);
   EditorObject *eObj = dynamic_cast<EditorObject *>(object);
   TNLAssert(eObj, "Bad cast!");

   Parent::addToDatabase(object, extents);

   if(eObj)
      mAllEditorObjects.push_back(eObj);

   geomSort(mAllEditorObjects);
}


void EditorObjectDatabase::removeFromDatabase(DatabaseObject *object, const Rect &extents)
{
   Parent::removeFromDatabase((DatabaseObject *)object, extents);

   // Remove the object to our list as well
   for(S32 i = 0; i < mAllEditorObjects.size(); i++)
      if(mAllEditorObjects[i] == object)
      {
         mAllEditorObjects.erase(i);      // Use erase to maintain sorted order
         break;
      }
}


// Provide a read-only shortcut to a pre-cast list of editor objects
const Vector<EditorObject *> *EditorObjectDatabase::getObjectList()
{
   return &mAllEditorObjects;
}


};


