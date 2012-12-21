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

#ifndef _GRIDDB_H_
#define _GRIDDB_H_

#include "GeomObject.h"    // Base class

#include "tnlTypes.h"
#include "tnlDataChunker.h"
#include "tnlVector.h"

#include "Rect.h"
#include "GeomUtils.h"


using namespace TNL;

namespace Zap
{

typedef bool (*TestFunc)(U8);

// Interface for dealing with objects that can be in our spatial database.
class  GridDatabase;
class EditorObjectDatabase;

class DatabaseObject : public GeomObject
{

   typedef GeomObject Parent;

friend class GridDatabase;
friend class EditorObjectDatabase;

private:
   U32 mLastQueryId;
   Rect mExtent;
   GridDatabase *mDatabase;

protected:
   U8 mObjectTypeNumber;

public:
   DatabaseObject();                            // Constructor
   DatabaseObject(const DatabaseObject &t);     // Copy constructor
   virtual ~DatabaseObject();                   // Destructor

   void initialize();

   GridDatabase *getDatabase();           // Returns the database in which this object is stored, NULL if not in any database

   Rect getExtent() const;
   void setExtent(const Rect &extentRect);

   virtual const Vector<Point> *getCollisionPoly() const;
   virtual bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) const;

   virtual bool isCollisionEnabled();

   bool isInDatabase();
   bool isDeleted();

   void addToDatabase(GridDatabase *database);
   void addToDatabase(GridDatabase *database, const Rect &extent);

   void removeFromDatabase(bool deleteObject);

   U8 getObjectTypeNumber();

   virtual bool isDatabasable();    // Can this item actually be inserted into a database?

   virtual DatabaseObject *clone() const;
};


////////////////////////////////////////
////////////////////////////////////////

class WallSegmentManager;
class GoalZone;

class GridDatabase
{
private:
   U32 mDatabaseId;
   static U32 mQueryId;
   static U32 mCountGridDatabase;

   WallSegmentManager *mWallSegmentManager;    

   void findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect *extents, const IntRect *bins);
   void findObjects(Vector<U8> typeNumbers, Vector<DatabaseObject *> &fillVector, const Rect *extents, const IntRect *bins);
   void findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector, const Rect *extents, const IntRect *bins, bool sameQuery = false);

   void fillBins(const Rect &extents, IntRect &bins);          // Helper function -- translates extents into bins to search

   Vector<DatabaseObject *> mAllObjects;
   Vector<DatabaseObject *> mGoalZones;

public:
   enum {
      BucketRowCount = 16,    // Number of buckets per grid row, and number of rows; should be power of 2
      BucketMask = BucketRowCount - 1,
   };

   struct BucketEntry
   {
      DatabaseObject *theObject;
      BucketEntry *nextInBucket;
   };

   static ClassChunker<BucketEntry> *mChunker;

   BucketEntry *mBuckets[BucketRowCount][BucketRowCount];

   GridDatabase(bool createWallSegmentManager = true);   // Constructor
   // GridDatabase::GridDatabase(const GridDatabase &source);
   virtual ~GridDatabase();                              // Destructor


   static const S32 BucketWidthBitShift = 8;    // Width/height of each bucket in pixels, in a form of 2 ^ n, 8 is 256 pixels

   DatabaseObject *findObjectLOS(U8 typeNumber, U32 stateIndex, bool format, const Point &rayStart, const Point &rayEnd,
                                 float &collisionTime, Point &surfaceNormal);
   DatabaseObject *findObjectLOS(U8 typeNumber, U32 stateIndex, const Point &rayStart, const Point &rayEnd,
                                 float &collisionTime, Point &surfaceNormal);
   DatabaseObject *findObjectLOS(TestFunc testFunc, U32 stateIndex, bool format, const Point &rayStart, const Point &rayEnd,
                                 float &collisionTime, Point &surfaceNormal);
   DatabaseObject *findObjectLOS(TestFunc testFunc, U32 stateIndex, const Point &rayStart, const Point &rayEnd,
                                 float &collisionTime, Point &surfaceNormal);
   bool pointCanSeePoint(const Point &point1, const Point &point2);

   void findObjects(Vector<DatabaseObject *> &fillVector);     // Returns all objects in the database
   const Vector<DatabaseObject *> *findObjects_fast() const;   // Faster than above, but results can't be modified
   const Vector<DatabaseObject *> *findObjects_fast(U8 typeNumber) const;   // Currently only works with goalZones, may be expanded in the future

   void findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector);
   void findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect &extents);

   void findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector);
   void findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector, const Rect &extents, bool sameQuery = false);

   void findObjects(const Vector<U8> &types, Vector<DatabaseObject *> &fillVector);
   void findObjects(const Vector<U8> &types, Vector<DatabaseObject *> &fillVector, const Rect &extents);

   void copyObjects(const GridDatabase *source);


   bool testTypes(const Vector<U8> &types, U8 objectType) const;


   void dumpObjects();     // For debugging purposes

   
   Rect getExtents();      // Get the combined extents of every object in the database

   WallSegmentManager *getWallSegmentManager() const;      

   void addToDatabase(DatabaseObject *theObject, const Rect &extents);
   void addToDatabase(const Vector<DatabaseObject *> &objects);


   virtual void removeFromDatabase(DatabaseObject *theObject, bool deleteObject);
   virtual void removeEverythingFromDatabase();

   S32 getObjectCount();                          // Return the number of objects currently in the database
   S32 getObjectCount(U8 typeNumber);             // Return the number of objects currently in the database of specified type
   bool hasObjectOfType(U8 typeNumber);
   DatabaseObject *getObjectByIndex(S32 index);   // Kind of hacky, kind of useful
};

};


// Reusable container for searching gridDatabases
// putting it outside of Zap namespace seems to help with debugging showing whats inside fillVector  (debugger forgets to add Zap::)
extern Vector<Zap::DatabaseObject *> fillVector;
extern Vector<Zap::DatabaseObject *> fillVector2;

#endif
