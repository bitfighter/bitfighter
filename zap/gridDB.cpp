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
#include "WallSegmentManager.h"

#include <map>

namespace Zap
{

U32 GridDatabase::mQueryId = 0;
ClassChunker<GridDatabase::BucketEntry> *GridDatabase::mChunker = NULL;
U32 GridDatabase::mCountGridDatabase = 0;

// Constructor
GridDatabase::GridDatabase(bool createWallSegmentManager)
{
   if(mChunker == NULL)
      mChunker = new ClassChunker<BucketEntry>();     // static shared by all databases, reference counted and deleted in destructor

   mCountGridDatabase++;

   mQueryId = 0;

   for(U32 i = 0; i < BucketRowCount; i++)
      for(U32 j = 0; j < BucketRowCount; j++)
         mBuckets[i][j] = NULL;

   if(createWallSegmentManager)
      mWallSegmentManager = new WallSegmentManager();    // gets deleted in destructor
   else
      mWallSegmentManager = NULL;
}


// Destructor
GridDatabase::~GridDatabase()       
{
   removeEverythingFromDatabase();

   TNLAssert(mChunker != NULL || mCountGridDatabase != 0, "running GridDatabase Destructor without initalizing?")

   if(mWallSegmentManager)
      delete mWallSegmentManager;

   mCountGridDatabase--;

   if(mCountGridDatabase == 0)
      delete mChunker;
}


void GridDatabase::addToDatabase(DatabaseObject *theObject, const Rect &extents)
{
   TNLAssert(theObject->mDatabase != this, "Already added to database, trying to add to same database again");
   TNLAssert(!theObject->mDatabase, "Already added to database, trying to add to different database");
   if(theObject->mDatabase)
      return;
	theObject->mDatabase = this;


   S32 minx, miny, maxx, maxy;

   minx = S32(extents.min.x) >> BucketWidthBitShift;
   miny = S32(extents.min.y) >> BucketWidthBitShift;
   maxx = S32(extents.max.x) >> BucketWidthBitShift;
   maxy = S32(extents.max.y) >> BucketWidthBitShift;

   if(U32(maxx - minx) >= BucketRowCount)
      maxx = minx + BucketRowCount - 1;
   if(U32(maxy - miny) >= BucketRowCount)
      maxy = miny + BucketRowCount - 1;

   for(S32 x = minx; maxx - x >= 0; x++)
      for(S32 y = miny; maxy - y >= 0; y++)
      {
         BucketEntry *be = mChunker->alloc();
         be->theObject = theObject;
         be->nextInBucket = mBuckets[x & BucketMask][y & BucketMask];
         mBuckets[x & BucketMask][y & BucketMask] = be;
      }

   // Add the object to our non-spatial "database" as well
   mAllObjects.push_back(theObject);
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
   mAllObjects.clear();
}

void GridDatabase::removeFromDatabase(DatabaseObject *theObject, const Rect &extents)
{
   TNLAssert(theObject->mDatabase == this || theObject->mDatabase == NULL, "Trying to remove Object from wrong database");
   if(theObject->mDatabase != this)
      return;
	theObject->mDatabase = NULL;

   S32 minx, miny, maxx, maxy;

   minx = S32(extents.min.x) >> BucketWidthBitShift;
   miny = S32(extents.min.y) >> BucketWidthBitShift;
   maxx = S32(extents.max.x) >> BucketWidthBitShift;
   maxy = S32(extents.max.y) >> BucketWidthBitShift;

   if(U32(maxx - minx) >= BucketRowCount)
      maxx = minx + BucketRowCount - 1;
   if(U32(maxy - miny) >= BucketRowCount)
      maxy = miny + BucketRowCount - 1;


   for(S32 x = minx; maxx - x >= 0; x++)
   {
      for(S32 y = miny; maxy - y >= 0; y++)
      {
         for(BucketEntry **walk = &mBuckets[x & BucketMask][y & BucketMask]; *walk; walk = &((*walk)->nextInBucket))
         {
            if((*walk)->theObject == theObject)
            {
               BucketEntry *rem = *walk;
               *walk = rem->nextInBucket;
               mChunker->free(rem);
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


void GridDatabase::findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect *extents, S32 minx, S32 miny, S32 maxx, S32 maxy)
{
   mQueryId++;    // Used to prevent the same item from being found in multiple buckets

   for(S32 x = minx; maxx - x >= 0; x++)
      for(S32 y = miny; maxy - y >= 0; y++)
         for(BucketEntry *walk = mBuckets[x & BucketMask][y & BucketMask]; walk; walk = walk->nextInBucket)
         {
            DatabaseObject *theObject = walk->theObject;

            if(theObject->mLastQueryId != mQueryId &&                      // Object hasn't been queried; and
               (theObject->getObjectTypeNumber() == typeNumber) &&         // is of the right type; and
               (!extents || theObject->mExtent.intersects(*extents)) )     // overlaps our extents (if passed)
            {
               walk->theObject->mLastQueryId = mQueryId;    // Flag the object so we know we've already visited it
               fillVector.push_back(walk->theObject);       // And save it as a found item
            }
         }
}


// Find all objects in database of type typeNumber
void GridDatabase::findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector)
{
   for(S32 i = 0; i < mAllObjects.size(); i++)
      if(mAllObjects[i]->getObjectTypeNumber() == typeNumber)
         fillVector.push_back(mAllObjects[i]);
}


// Find all objects in &extents that are of type typeNumber
void GridDatabase::findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect &extents)
{
   S32 minx, miny, maxx, maxy;

   minx = S32(extents.min.x) >> BucketWidthBitShift;
   miny = S32(extents.min.y) >> BucketWidthBitShift;
   maxx = S32(extents.max.x) >> BucketWidthBitShift;
   maxy = S32(extents.max.y) >> BucketWidthBitShift;

   if(U32(maxx - minx) >= BucketRowCount)
      maxx = minx + BucketRowCount - 1;
   if(U32(maxy - miny) >= BucketRowCount)
      maxy = miny + BucketRowCount - 1;

   findObjects(typeNumber, fillVector, &extents, minx, miny, maxx, maxy);
}


void GridDatabase::findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector, const Rect *extents, S32 minx, S32 miny, S32 maxx, S32 maxy)
{
   TNLAssert(this, "findObjects 'this' is NULL");
   mQueryId++;    // Used to prevent the same item from being found in multiple buckets

   for(S32 x = minx; maxx - x >= 0; x++)
      for(S32 y = miny; maxy - y >= 0; y++)
         for(BucketEntry *walk = mBuckets[x & BucketMask][y & BucketMask]; walk; walk = walk->nextInBucket)
         {
            DatabaseObject *theObject = walk->theObject;

            if(theObject->mLastQueryId != mQueryId &&                      // Object hasn't been queried; and
               (testFunc(theObject->getObjectTypeNumber())) &&         // is of the right type; and
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


// Find all objects in &extents derived type test function
void GridDatabase::findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector, const Rect &extents)
{
   S32 minx, miny, maxx, maxy;

   minx = S32(extents.min.x) >> BucketWidthBitShift;
   miny = S32(extents.min.y) >> BucketWidthBitShift;
   maxx = S32(extents.max.x) >> BucketWidthBitShift;
   maxy = S32(extents.max.y) >> BucketWidthBitShift;

   if(U32(maxx - minx) >= BucketRowCount)
      maxx = minx + BucketRowCount - 1;
   if(U32(maxy - miny) >= BucketRowCount)
      maxy = miny + BucketRowCount - 1;

   findObjects(testFunc, fillVector, &extents, minx, miny, maxx, maxy);
}


void GridDatabase::dumpObjects()
{
   for(S32 x = 0; x < BucketRowCount; x++)
      for(S32 y = 0; y < BucketRowCount; y++)
         for(BucketEntry *walk = mBuckets[x & BucketMask][y & BucketMask]; walk; walk = walk->nextInBucket)
         {
            DatabaseObject *theObject = walk->theObject;
            logprintf("Found object in (%d,%d) with extents %s", x,y,theObject->getExtent().toString().c_str());
            logprintf("Obj coords: %s", dynamic_cast<EditorObject *>(theObject)->getPos().toString().c_str());
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
DatabaseObject::DatabaseObject(const DatabaseObject &t) 
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

   collisionTime = 100;
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

   static Vector<DatabaseObject *> fillVector;  // Use local here, moust callers expect our global fillVector to be left unchanged
   fillVector.clear();

   findObjects(testFunc, fillVector, queryRect);

   Point collisionPoint;

   collisionTime = 100;
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

   return( findObjectLOS((TestFunc)isWallType, MoveObject::ActualState, true, point1, point2, time, coll) == NULL );
}


S32 GridDatabase::getObjectCount()
{
   return mAllObjects.size();
}


S32 GridDatabase::hasObjectOfType(U8 typeNumber)
{
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


////////////////////////////////////////
////////////////////////////////////////

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


void DatabaseObject::removeFromDatabase()
{
   if(!mDatabase)
      return;

   getDatabase()->removeFromDatabase(this, mExtent);
   mDatabase = NULL;
}


bool DatabaseObject::isDatabasable()
{
   return true;
}


bool DatabaseObject::getCollisionPoly(Vector<Point> &polyPoints) const
{
   return false;
}


bool DatabaseObject::getCollisionCircle(U32 stateIndex, Point &point, float &radius) const
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


void DatabaseObject::setObjectTypeNumber(U8 objectTypeNumber)
{
   mObjectTypeNumber = objectTypeNumber;
}


GridDatabase *DatabaseObject::getDatabase()
{
   return mDatabase;
}


void DatabaseObject::setDatabase(GridDatabase *database)
{
   mDatabase = database;
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
      minx = S32(extents.min.x) >> GridDatabase::BucketWidthBitShift;
      miny = S32(extents.min.y) >> GridDatabase::BucketWidthBitShift;
      maxx = S32(extents.max.x) >> GridDatabase::BucketWidthBitShift;
      maxy = S32(extents.max.y) >> GridDatabase::BucketWidthBitShift;

      // To save CPU, check if there is anything different
      if((minxold - minx) | (minyold - miny) | (maxxold - maxx) | (maxyold - maxy))
      {
         // it is different, remove and add to database, but don't touch gridDB->mAllObjects

         //printf("new  %i %i %i %i old %i %i %i %i\n", minx, miny, maxx, maxy, minxold, minyold, maxxold, maxyold);

         if(U32(maxx - minx) >= gridDB->BucketRowCount)
            maxx = minx + gridDB->BucketRowCount - 1;
         if(U32(maxy - miny) >= gridDB->BucketRowCount)
            maxy = miny + gridDB->BucketRowCount - 1;
         if(U32(maxxold >= minxold) + gridDB->BucketRowCount)
            maxxold = minxold + gridDB->BucketRowCount - 1;
         if(U32(maxyold >= minyold) + gridDB->BucketRowCount)
            maxyold = minyold + gridDB->BucketRowCount - 1;


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


////////////////////////////////////////
////////////////////////////////////////

// This sort will put points on top of lines on top of polygons...  as they should be
// We'll also put walls on the bottom, as this seems to work best in practice
S32 QSORT_CALLBACK geometricSort(EditorObject * &a, EditorObject * &b)
{
   if(isWallType(a->getObjectTypeNumber()))
      return 1;
   if(isWallType(b->getObjectTypeNumber()))
      return -1;

   return( b->getGeomType() - a->getGeomType() );
}


static void geomSort(Vector<EditorObject *> &objects)
{
   if(objects.size() >= 2)       // No point sorting unless there are two or more objects!

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
      EditorObject *newObject = dynamic_cast<EditorObject *>(theObject)->copy();

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
   //S32 ctr = 0;

   for(U32 x = 0; x < BucketRowCount; x++)
      for(U32 y = 0; y < BucketRowCount; y++)
      {
         mBuckets[x][y] = NULL;

         for(BucketEntry *walk = source.mBuckets[x][y]; walk; walk = walk->nextInBucket)
         {
            //ctr++;
            BucketEntry *be = mChunker->alloc();                // Create a slot for our new object
            DatabaseObject *theObject = walk->theObject;

            DatabaseObject *object = getObject(dbObjectMap, theObject);    // Returns a pointer to a new or existing copy of theObject
            object->setDatabase(this);
            be->theObject = object;

            be->nextInBucket = mBuckets[x][y];
            mBuckets[x][y] = be;
         }
      }

//logprintf("found %d refs!", ctr);   

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
   EditorObject *eObj = dynamic_cast<EditorObject *>(object);
   TNLAssert(eObj, "Bad cast!");

   Vector<EditorObject *> objects;
   objects.push_back(eObj);

   addToDatabase(objects);
}


// Add items in bulk to avoid resorting after each of a dozen or two objects are added
void EditorObjectDatabase::addToDatabase(const Vector<EditorObject *> &objects)
{
   for(S32 i = 0; i < objects.size(); i++)
   {
      Parent::addToDatabase(objects[i], objects[i]->getExtent());
      mAllEditorObjects.push_back(objects[i]);
   }

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


void EditorObjectDatabase::removeEverythingFromDatabase()
{
   Vector<EditorObject *> tempVector(mAllEditorObjects);  // To keep synchronization

   for(S32 i = 0; i < tempVector.size(); i++)
      removeFromDatabase(tempVector[i], tempVector[i]->getExtent());

   WallSegmentManager *wallSegmentManager = getWallSegmentManager();
   if(wallSegmentManager)
      wallSegmentManager->clear();
}


// Provide a read-only shortcut to a pre-cast list of editor objects
const Vector<EditorObject *> *EditorObjectDatabase::getObjectList()
{
   return &mAllEditorObjects;
}


};

// Reusable container for searching gridDatabases
// putting it outside of Zap namespace seems to help with debugging showing whats inside fillVector  (debugger forgets to add Zap::)
Vector<Zap::DatabaseObject *> fillVector;
Vector<Zap::DatabaseObject *> fillVector2;
