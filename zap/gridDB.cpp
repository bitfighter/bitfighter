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
#include "moveObject.h"    // For def of ActualState
#include "WallSegmentManager.h"

namespace Zap
{

U32 GridDatabase::mQueryId = 0;
ClassChunker<GridDatabase::BucketEntry> *GridDatabase::mChunker = NULL;
U32 GridDatabase::mCountGridDatabase = 0;

static U32 getNextId() 
{
   static U32 nextId = 0;
   return nextId++;
}

// Constructor
GridDatabase::GridDatabase(bool createWallSegmentManager)
{
   if(mChunker == NULL)
      mChunker = new ClassChunker<BucketEntry>();        // Static shared by all databases, reference counted and deleted in destructor

   mCountGridDatabase++;

   for(U32 i = 0; i < BucketRowCount; i++)
      for(U32 j = 0; j < BucketRowCount; j++)
         mBuckets[i][j] = NULL;

   if(createWallSegmentManager)
      mWallSegmentManager = new WallSegmentManager();    // Gets deleted in destructor
   else
      mWallSegmentManager = NULL;

   mDatabaseId = getNextId();
}


// Copy contents of source into this
// GridDatabase::GridDatabase(const GridDatabase &source)
// {
//    mAllObjects.reserve(source.mAllObjects.size());
// 
//    for(S32 i = 0; i < source.mAllObjects.size(); i++)
//       addToDatabase(source.mAllObjects[i]->clone(), source.mAllObjects[i]->getExtent());
// }


// Destructor
GridDatabase::~GridDatabase()       
{
   removeEverythingFromDatabase();

   TNLAssert(mChunker != NULL || mCountGridDatabase != 0, "Running GridDatabase destructor without initalizing?");

   if(mWallSegmentManager)
      delete mWallSegmentManager;

   mCountGridDatabase--;

   if(mCountGridDatabase == 0)
      delete mChunker;
}


// This sort will put points on top of lines on top of polygons...  as they should be
// We'll also put walls on the bottom, as this seems to work best in practice
S32 QSORT_CALLBACK geometricSort(DatabaseObject * &a, DatabaseObject * &b)
{
   if(isWallType(a->getObjectTypeNumber()))
      return 1;
   if(isWallType(b->getObjectTypeNumber()))
      return -1;

   return( b->getGeomType() - a->getGeomType() );
}


static void sortObjects(Vector<DatabaseObject *> &objects)
{
   if(objects.size() >= 2)       // No point sorting unless there are two or more objects!

      // Cannot use Vector.sort() here because I couldn't figure out how to cast shared_ptr as pointer (*)
      //sort(objects.getStlVector().begin(), objects.getStlVector().begin() + objects.size(), geometricSort);
      qsort(&objects[0], objects.size(), sizeof(BfObject *), (qsort_compare_func) geometricSort);
}


// Fill this database with objects from existing database
void GridDatabase::copyObjects(const GridDatabase *source)
{
   // Preallocate some memory to make copying a little more efficient
   mAllObjects.reserve(source->mAllObjects.size());
   mGoalZones.reserve (source->mGoalZones.size());

   for(S32 i = 0; i < source->mAllObjects.size(); i++)
      addToDatabase(source->mAllObjects[i]->clone(), source->mAllObjects[i]->getExtent());

   sortObjects(mAllObjects);
}


void GridDatabase::addToDatabase(DatabaseObject *theObject, const Rect &extents)
{
   TNLAssert(theObject->mDatabase != this,  "Already added to database, trying to add to same database again!");
   TNLAssert(!theObject->mDatabase,         "Already added to database, trying to add to different database!");
   TNLAssert(theObject->mExtent == extents, "Extents not equal!");

   if(theObject->mDatabase)      // Should never happen
      return;

   theObject->mDatabase = this;

   static IntRect bins;
   fillBins(extents, bins);

   for(S32 x = bins.minx; bins.maxx - x >= 0; x++)
      for(S32 y = bins.miny; bins.maxy - y >= 0; y++)
      {
         BucketEntry *be = mChunker->alloc();
         be->theObject = theObject;
         be->nextInBucket = mBuckets[x & BucketMask][y & BucketMask];
         mBuckets[x & BucketMask][y & BucketMask] = be;
      }

   // Add the object to our non-spatial "database" as well
   mAllObjects.push_back(theObject);

   if(theObject->getObjectTypeNumber() == GoalZoneTypeNumber)
      mGoalZones.push_back(theObject);

   //sortObjects(mAllObjects);  // problem: Barriers in-game don't have mGeometry (it is NULL)
}


// Bulk add items to database
void GridDatabase::addToDatabase(const Vector<DatabaseObject *> &objects)
{
   for(S32 i = 0; i < objects.size(); i++)
      addToDatabase(objects[i], objects[i]->getExtent());
}


void GridDatabase::removeEverythingFromDatabase()
{
   for(S32 x = 0; x < BucketRowCount; x++)
   {
      for(S32 y = 0; y < BucketRowCount; y++)
      {
         for(BucketEntry *walk = mBuckets[x & BucketMask][y & BucketMask]; walk; )
         {
            BucketEntry *rem = walk;
            walk->theObject->mDatabase = NULL;  // make sure object don't point to this database anymore
            walk = rem->nextInBucket;
            mChunker->free(rem);
         }
         mBuckets[x & BucketMask][y & BucketMask] = NULL;
      }
   }

   mGoalZones.clear();
   mAllObjects.deleteAndClear();
   
   if(mWallSegmentManager)
      mWallSegmentManager->clear();
}


void GridDatabase::removeFromDatabase(DatabaseObject *object, bool deleteObject)
{
   TNLAssert(object->mDatabase == this || object->mDatabase == NULL, "Trying to remove Object from wrong database");
   if(object->mDatabase != this)
      return;

   const Rect &extents = object->mExtent;
   object->mDatabase = NULL;

   static IntRect bins;
   fillBins(extents, bins);

   for(S32 x = bins.minx; bins.maxx - x >= 0; x++)
   {
      for(S32 y = bins.miny; bins.maxy - y >= 0; y++)
      {
         for(BucketEntry **walk = &mBuckets[x & BucketMask][y & BucketMask]; *walk; walk = &((*walk)->nextInBucket))
         {
            if((*walk)->theObject == object)
            {
               BucketEntry *rem = *walk;
               *walk = rem->nextInBucket;
               mChunker->free(rem);
               break;
            }
         }
      }
   }

   // Find and delete object from our non-spatial databases
   for(S32 i = 0; i < mAllObjects.size(); i++)
      if(mAllObjects[i] == object)
      {
         if(deleteObject)
            delete mAllObjects[i];      

         mAllObjects.erase(i);            // mAllObjects is sorted, so we can't use erase_fast
         break;
      }


   if(object->getObjectTypeNumber() == GoalZoneTypeNumber)
      for(S32 i = 0; i < mGoalZones.size(); i++)
         if(mGoalZones[i] == object)
         {
            mGoalZones.erase_fast(i);     // mGoalZones is not sorted, so erase_fast is just fine
            break;
         }
}


void GridDatabase::findObjects(Vector<DatabaseObject *> &fillVector)
{
   fillVector.resize(mAllObjects.size());

   for(S32 i = 0; i < mAllObjects.size(); i++)
      fillVector[i] = mAllObjects[i];
}


// Faster than above, but results can't be modified
const Vector<DatabaseObject *> *GridDatabase::findObjects_fast() const
{
   return &mAllObjects;
}


// Faster than above, but results can't be modified, and only works with GoalZones at the moment
const Vector<DatabaseObject *> *GridDatabase::findObjects_fast(U8 typeNumber) const
{
   TNLAssert(typeNumber == GoalZoneTypeNumber, "Function only supports GoalZones at the moment!");
   return &mGoalZones;
}


void GridDatabase::findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect *extents, const IntRect *bins)
{
   static Vector<U8> types;
   types.resize(1);

   types[0] = typeNumber;

   findObjects(types, fillVector, extents, bins);
}


void GridDatabase::findObjects(Vector<U8> typeNumbers, Vector<DatabaseObject *> &fillVector, const Rect *extents, const IntRect *bins)
{
   mQueryId++;    // Used to prevent the same item from being found in multiple buckets

   for(S32 x = bins->minx; bins->maxx - x >= 0; x++)
      for(S32 y = bins->miny; bins->maxy - y >= 0; y++)
         for(BucketEntry *walk = mBuckets[x & BucketMask][y & BucketMask]; walk; walk = walk->nextInBucket)
         {
            DatabaseObject *theObject = walk->theObject;

            if(theObject->mLastQueryId != mQueryId &&                         // Object hasn't been queried; and
               testTypes(typeNumbers, theObject->getObjectTypeNumber()) &&    // is of the right type; and
               (!extents || theObject->mExtent.intersects(*extents)) )        // overlaps our extents (if passed)
            {
               walk->theObject->mLastQueryId = mQueryId;    // Flag the object so we know we've already visited it
               fillVector.push_back(walk->theObject);       // And save it as a found item
            }
         }
}


// Find all objects in database of type typeNumber
void GridDatabase::findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector)
{
   if(typeNumber == GoalZoneTypeNumber)
   {
      for(S32 i = 0; i < mAllObjects.size(); i++)
         fillVector.push_back(mAllObjects[i]);
      return;
   }

   for(S32 i = 0; i < mAllObjects.size(); i++)
      if(mAllObjects[i]->getObjectTypeNumber() == typeNumber)
         fillVector.push_back(mAllObjects[i]);
}


// Translates extents into bins to search
void GridDatabase::fillBins(const Rect &extents, IntRect &bins)
{
   bins.minx = S32(extents.min.x) >> BucketWidthBitShift;
   bins.miny = S32(extents.min.y) >> BucketWidthBitShift;
   bins.maxx = S32(extents.max.x) >> BucketWidthBitShift;
   bins.maxy = S32(extents.max.y) >> BucketWidthBitShift;

   if(U32(bins.maxx - bins.minx) >= BucketRowCount)
      bins.maxx = bins.minx + BucketRowCount - 1;

   if(U32(bins.maxy - bins.miny) >= BucketRowCount)
      bins.maxy = bins.miny + BucketRowCount - 1;
}


// Find all objects in &extents that are of type typeNumber
void GridDatabase::findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect &extents)
{
   static IntRect bins;
   fillBins(extents, bins);

   findObjects(typeNumber, fillVector, &extents, &bins);
}


void GridDatabase::findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector, const Rect *extents, const IntRect *bins, bool sameQuery)
{
   TNLAssert(this, "findObjects 'this' is NULL");
   if(!sameQuery)
      mQueryId++;    // Used to prevent the same item from being found in multiple buckets

   for(S32 x = bins->minx; bins->maxx - x >= 0; x++)
      for(S32 y = bins->miny; bins->maxy - y >= 0; y++)
         for(BucketEntry *walk = mBuckets[x & BucketMask][y & BucketMask]; walk; walk = walk->nextInBucket)
         {
            DatabaseObject *theObject = walk->theObject;

            if(theObject->mLastQueryId != mQueryId &&                      // Object hasn't been queried; and
               (testFunc(theObject->getObjectTypeNumber())) &&             // is of the right type; and
               (!extents || theObject->mExtent.intersects(*extents)) )     // overlaps our extents (if passed)
            {
               walk->theObject->mLastQueryId = mQueryId;    // Flag the object so we know we've already visited it
               fillVector.push_back(walk->theObject);       // And save it as a found item
            }
         }
}


// Find all objects in database using derived type test function
void GridDatabase::findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector)
{
   for(S32 i = 0; i < mAllObjects.size(); i++)
      if(testFunc(mAllObjects[i]->getObjectTypeNumber()))
         fillVector.push_back(mAllObjects[i]);
}


// Find all objects in database using derived type test function
void GridDatabase::findObjects(const Vector<U8> &types, Vector<DatabaseObject *> &fillVector, const Rect &extents)
{
   static IntRect bins;
   fillBins(extents, bins);

   findObjects(types, fillVector, &extents, &bins);
}


// Find all objects in database using derived type test function
void GridDatabase::findObjects(const Vector<U8> &types, Vector<DatabaseObject *> &fillVector)
{
   for(S32 i = 0; i < mAllObjects.size(); i++)
      if(testTypes(types, mAllObjects[i]->getObjectTypeNumber()))
         fillVector.push_back(mAllObjects[i]);
}


bool GridDatabase::testTypes(const Vector<U8> &types, U8 objectType) const
{
   for(S32 i = 0; i < types.size(); i++)
      if(types[i] == objectType)
         return true;

   return false;
}


// Find all objects in &extents derived type test function
void GridDatabase::findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector, const Rect &extents, bool sameQuery)
{
   static IntRect bins;
   fillBins(extents, bins);

   findObjects(testFunc, fillVector, &extents, &bins, sameQuery);
}


void GridDatabase::dumpObjects()
{
   for(S32 x = 0; x < BucketRowCount; x++)
      for(S32 y = 0; y < BucketRowCount; y++)
         for(BucketEntry *walk = mBuckets[x & BucketMask][y & BucketMask]; walk; walk = walk->nextInBucket)
         {
            DatabaseObject *theObject = walk->theObject;
            logprintf("Found object in (%d,%d) with extents %s", x, y, theObject->getExtent().toString().c_str());
            logprintf("Obj coords: %s", static_cast<BfObject *>(theObject)->getPos().toString().c_str());
         }
}


// Get the extents of every object in the database
Rect GridDatabase::getExtents()
{
   if(mAllObjects.size() == 0)     // No objects ==> no extents!
      return Rect();

   Rect rect;

   // Think we can delete from HERE...   inserted this comment 27-Jan-2012  #########################################

   // All this rigamarole is to make world extent correct for levels that do not overlap (0,0)
   // The problem is that the GameType is treated as an object, and has the extent (0,0), and
   // a mask of UnknownType.  Fortunately, the GameType tends to be first, so what we do is skip
   // all objects until we find an UnknownType object, then start creating our extent from there.
   // We have to assign theRect to an extent object initially to avoid getting the default coords
   // of (0,0) that are assigned by the constructor.


   S32 first = -1;

   // Look for first non-UnknownType object
   for(S32 i = 0; i < mAllObjects.size() && first == -1; i++)
      if(mAllObjects[i]->getObjectTypeNumber() != UnknownTypeNumber)
      {
         rect = mAllObjects[i]->getExtent();
         first = i;
      }

   TNLAssert(first == 0, "I think this should never happen -- how would an object with UnknownTypeNumber get in the database?? \
                          if it does, please document it and remove this assert, along withthe rect = line below -Wat");

   if(first == -1)      // No suitable objects found, return empty extents
      return Rect();

   // ...to HERE

   rect = mAllObjects[0]->getExtent();

   // Now start unioning the extents of remaining objects.  Should be all of them.
   for(S32 i = /*first + */1; i < mAllObjects.size(); i++)
      rect.unionRect(mAllObjects[i]->getExtent());

   return rect;
}


WallSegmentManager *GridDatabase::getWallSegmentManager() const
{
   return mWallSegmentManager;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
DatabaseObject::DatabaseObject() 
{
   initialize();
}


// Copy constructor
DatabaseObject::DatabaseObject(const DatabaseObject &t) : Parent(t)
{  
   initialize();
   mObjectTypeNumber = t.mObjectTypeNumber; 
   mExtent = t.mExtent;
}


// Destructor
DatabaseObject::~DatabaseObject()
{
   TNLAssert(!mDatabase, "Must remove from database when deleting this object");
   // Do nothing
}


// Code that needs to run for both constructor and copy constructor
void DatabaseObject::initialize() 
{
   mLastQueryId = 0; 
   mExtent = Rect(); 
   mDatabase = NULL;
}


// Find objects along a ray, returning first discovered object, along with time of
// that collision and a Point representing the normal angle at intersection point
//             (at least I think that's what's going on here - CE)
DatabaseObject *GridDatabase::findObjectLOS(U8 typeNumber, U32 stateIndex,
                                            const Point &rayStart, const Point &rayEnd, 
                                            float &collisionTime, Point &surfaceNormal)
{
   return findObjectLOS(typeNumber, stateIndex, true, rayStart, rayEnd, collisionTime, surfaceNormal);
}

// Format is a passthrough to polygonLineIntersect().  Will be true for most items, false for walls in editor.
DatabaseObject *GridDatabase::findObjectLOS(U8 typeNumber, U32 stateIndex, bool format,
                                            const Point &rayStart, const Point &rayEnd,
                                            float &collisionTime, Point &surfaceNormal)
{
   Rect queryRect(rayStart, rayEnd);

   static Vector<DatabaseObject *> fillVector;  // Use local here, Most of code expects a global FillVector left unchanged
   fillVector.clear();

   findObjects(typeNumber, fillVector, queryRect);

   Point collisionPoint;

   collisionTime = 1;
   DatabaseObject *retObject = NULL;

   Point center;
   Rect rect;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      if(!fillVector[i]->isCollisionEnabled())     // Skip collision-disabled objects
         continue;

      static Vector<Point> poly;
      poly.clear();

      F32 radius, ct;

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
         if(circleIntersectsSegment(center, radius, rayStart, rayEnd, ct) && ct < collisionTime)
         {
            collisionTime = ct;
            surfaceNormal = (rayStart + (rayEnd - rayStart) * ct) - center;
            retObject = fillVector[i];
         }
      }
   }

   if(retObject)
      surfaceNormal.normalize();

   return retObject;
}


DatabaseObject *GridDatabase::findObjectLOS(TestFunc testFunc, U32 stateIndex, bool format,
                                            const Point &rayStart, const Point &rayEnd, 
                                            float &collisionTime, Point &surfaceNormal)
{
   Rect queryRect(rayStart, rayEnd);

   static Vector<DatabaseObject *> fillVector;  // Use local here, most callers expect our global fillVector to be left unchanged
   fillVector.clear();

   findObjects(testFunc, fillVector, queryRect);

   Point collisionPoint;

   collisionTime = 1;
   DatabaseObject *retObject = NULL;

   Point center;
   Rect rect;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      if(!fillVector[i]->isCollisionEnabled())     // Skip collision-disabled objects
         continue;

      static Vector<Point> poly;
      poly.clear();

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


DatabaseObject *GridDatabase::findObjectLOS(TestFunc testFunc, U32 stateIndex,
                                            const Point &rayStart, const Point &rayEnd,
                                            float &collisionTime, Point &surfaceNormal)
{
   return findObjectLOS(testFunc, stateIndex, true, rayStart, rayEnd, collisionTime, surfaceNormal);
}


bool GridDatabase::pointCanSeePoint(const Point &point1, const Point &point2)
{
   F32 time;
   Point coll;

   return( findObjectLOS((TestFunc)isWallType, ActualState, true, point1, point2, time, coll) == NULL );
}


S32 GridDatabase::getObjectCount()
{
   return mAllObjects.size();
}


// Return count of objects of specified type.  Only supports GoalZones at the moment.
S32 GridDatabase::getObjectCount(U8 typeNumber)
{
   TNLAssert(typeNumber == GoalZoneTypeNumber, "Function only supports GoalZones at the moment!");
   return mGoalZones.size();
}


bool GridDatabase::hasObjectOfType(U8 typeNumber)
{
   if(typeNumber == GoalZoneTypeNumber)
      return mGoalZones.size() > 0;

   for(S32 i = 0; i < mAllObjects.size(); i++)
      if(mAllObjects[i]->getObjectTypeNumber() == typeNumber)
         return true;

   return false;
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


void DatabaseObject::addToDatabase(GridDatabase *database, const Rect &extent)
{
   if(mDatabase)
      return;

   mExtent = extent;
   addToDatabase(database);
}


void DatabaseObject::addToDatabase(GridDatabase *database)
{
   if(isDatabasable())
      database->addToDatabase(this, mExtent);
}


bool DatabaseObject::isInDatabase()
{
   return mDatabase != NULL;
}


bool DatabaseObject::isDeleted() 
{
   return mObjectTypeNumber == DeletedTypeNumber;
}


void DatabaseObject::removeFromDatabase(bool deleteObject)
{
   if(!mDatabase)
      return;

   getDatabase()->removeFromDatabase(this, deleteObject);
}


bool DatabaseObject::isDatabasable()
{
   return true;
}


bool DatabaseObject::getCollisionPoly(Vector<Point> &polyPoints) const
{
   return false;
}


bool DatabaseObject::getCollisionCircle(U32 stateIndex, Point &point, F32 &radius) const
{
   return false;
}


bool DatabaseObject::isCollisionEnabled()
{
   return true;
}


U8 DatabaseObject::getObjectTypeNumber()
{
   return mObjectTypeNumber;
}


GridDatabase *DatabaseObject::getDatabase()
{
   return mDatabase;
}


Rect DatabaseObject::getExtent() const
{
   return mExtent;
}


//extern string itos(int);

// Update object's extents in the database -- will not add object to database if it's not already in it
void DatabaseObject::setExtent(const Rect &extents)
{
   // The following is some debugging code for seeing the ridiculous number of duplicate calls we make to this function
   // This duplication is probably a sign that there is a problem in the model.  Deal with it another day.
   //static string last = "", lastx = "";
   //logprintf("Updating %p extent to %s", this, extents.toString().c_str());
   //lastx = itos((int)(this)) + " " + extents.toString();
   //if(lastx == last) { logprintf("SAME======================="); }
   //last = lastx;

   GridDatabase *gridDB = getDatabase();

   if(gridDB)
   {
      // Remove from the extents database for current extents...
      //gridDB->removeFromDatabase(this, mExtent);    // old extent
      // ...and re-add for the new extent
      //gridDB->addToDatabase(this, extents);


      S32 minxold, minyold, maxxold, maxyold;
      S32 minx, miny, maxx, maxy;

      minxold = S32(mExtent.min.x) >> GridDatabase::BucketWidthBitShift;
      minyold = S32(mExtent.min.y) >> GridDatabase::BucketWidthBitShift;
      maxxold = S32(mExtent.max.x) >> GridDatabase::BucketWidthBitShift;
      maxyold = S32(mExtent.max.y) >> GridDatabase::BucketWidthBitShift;

      minx    = S32(extents.min.x) >> GridDatabase::BucketWidthBitShift;
      miny    = S32(extents.min.y) >> GridDatabase::BucketWidthBitShift;
      maxx    = S32(extents.max.x) >> GridDatabase::BucketWidthBitShift;
      maxy    = S32(extents.max.y) >> GridDatabase::BucketWidthBitShift;

      // Don't do anything if the buckets haven't changed...
      if((minxold - minx) | (minyold - miny) | (maxxold - maxx) | (maxyold - maxy))
      {
         // They are different... remove and readd to database, but don't touch gridDB->mAllObjects
         if(U32(maxx - minx) >= gridDB->BucketRowCount)        maxx    = minx    + gridDB->BucketRowCount - 1;
         if(U32(maxy - miny) >= gridDB->BucketRowCount)        maxy    = miny    + gridDB->BucketRowCount - 1;
         if(U32(maxxold >= minxold) + gridDB->BucketRowCount)  maxxold = minxold + gridDB->BucketRowCount - 1;
         if(U32(maxyold >= minyold) + gridDB->BucketRowCount)  maxyold = minyold + gridDB->BucketRowCount - 1;


         // Don't use x <= maxx, it will endless loop if maxx = S32_MAX and x overflows
         // Instead, use maxx - x >= 0, it will better handle overflows and avoid endless loop (MIN_S32 - MAX_S32 = +1)

         // Remove from the extents database for current extents...
         for(S32 x = minxold; maxxold - x >= 0; x++)
            for(S32 y = minyold; maxyold - y >= 0; y++)
               for(GridDatabase::BucketEntry **walk = &gridDB->mBuckets[x & gridDB->BucketMask][y & gridDB->BucketMask]; 
                                   *walk; walk = &((*walk)->nextInBucket))
                  if((*walk)->theObject == this)
                  {
                     GridDatabase::BucketEntry *rem = *walk;
                     *walk = rem->nextInBucket;
                     gridDB->mChunker->free(rem);
                     break;
                  }
         // ...and re-add for the new extent
         for(S32 x = minx; maxx - x >= 0; x++)
            for(S32 y = miny; maxy - y >= 0; y++)
            {
               GridDatabase::BucketEntry *be = gridDB->mChunker->alloc();
               be->theObject = this;
               be->nextInBucket = gridDB->mBuckets[x & gridDB->BucketMask][y & gridDB->BucketMask];
               gridDB->mBuckets[x & gridDB->BucketMask][y & gridDB->BucketMask] = be;
            }
      }

   }

   mExtent.set(extents);
}


DatabaseObject *DatabaseObject::clone() const
{
   TNLAssert(false, "Clone method not implemented!");
   return NULL;
}


};

////////////////////////////////////////
////////////////////////////////////////

// Reusable container for searching gridDatabases
// Has to be outside of Zap namespace seems to help with debugging showing what's inside fillVector  (debugger forgets to add Zap::)
Vector<Zap::DatabaseObject *> fillVector;
Vector<Zap::DatabaseObject *> fillVector2;


