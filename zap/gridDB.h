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


#include "tnlTypes.h"
#include "tnlDataChunker.h"
#include "tnlVector.h"

#include "point.h"


using namespace TNL;

namespace Zap
{

// Interface for dealing with objects that can be in our spatial database.  Can be either GameObjects or
// items in te
class  GridDatabase;

class DatabaseObject
{
friend class GridDatabase;

private:
   U32 mLastQueryId;    
   Rect extent;     
   bool mInDatabase;

protected:
   U32 mObjectTypeMask;

public:
   DatabaseObject() { mLastQueryId = 0; extent = Rect(); mInDatabase = false; }    // Quickie constructor

   U32 getObjectTypeMask() { return mObjectTypeMask; }   
   void setObjectTypeMask(U32 objectTypeMask) { mObjectTypeMask = objectTypeMask; }

   Rect getExtent() { return extent; }
   void setExtent(const Rect &extentRect);

   virtual GridDatabase *getGridDatabase() = 0;
   virtual bool getCollisionPoly(Vector<Point> &polyPoints) = 0;
   virtual bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) = 0;
   virtual bool isCollisionEnabled() = 0;

   bool isInDatabase() { return mInDatabase; }

   void addToDatabase();
   void removeFromDatabase();
};

////////////////////////////////////////
////////////////////////////////////////

class GridDatabase
{
private:
   bool mUsingGameCoords;

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

   U32 mQueryId;
   BucketEntry *mBuckets[BucketRowCount][BucketRowCount];
   ClassChunker<BucketEntry> mChunker;

   GridDatabase(bool usingGameCoords = true);      // Constructor

   S32 BucketWidth;     // Width/height of each bucket in pixels

   DatabaseObject *findObjectLOS(U32 typeMask, U32 stateIndex, bool format, const Point &rayStart, const Point &rayEnd, 
                                 float &collisionTime, Point &surfaceNormal);
   DatabaseObject *findObjectLOS(U32 typeMask, U32 stateIndex, const Point &rayStart, const Point &rayEnd, 
                                 float &collisionTime, Point &surfaceNormal);
   bool pointCanSeePoint(const Point &point1, const Point &point2);

   void findObjects(U32 typeMask, Vector<DatabaseObject *> &fillVector, const Rect &extents);
   void findAllObjects(U32 typeMask, Vector<DatabaseObject *> &fillVector);

   void addToDatabase(DatabaseObject *theObject, const Rect &extents);
   void removeFromDatabase(DatabaseObject *theObject, const Rect &extents);

   bool isUsingGameCoords() { return mUsingGameCoords; }
};

};

#endif


