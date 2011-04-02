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

#include "polygon.h"
#include "GeomUtils.h"      // For polygon triangulation

namespace Zap
{

extern S32 gMaxPolygonPoints;

void Polyline::packUpdate(GhostConnection *connection, BitStream *stream)
{
   // - 1 because writeEnum ranges from 0 to n-1; mPolyBounds.size() ranges from 1 to n
   stream->writeEnum(mPolyBounds.size() - 1, gMaxPolygonPoints);  
   for(S32 i = 0; i < mPolyBounds.size(); i++)
   {
      stream->write(mPolyBounds[i].x);
      stream->write(mPolyBounds[i].y);
   }
}


// Read a series of points from a command line, and add them to a Vector of points
void Polyline::processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize, bool allowFirstAndLastPointToBeEqual)
{
   Point p, lastP;
   
   for(S32 i = firstCoord; i < argc; i += 2)
   {
      // Put a cap on the number of vertices in a polygon
      if(mPolyBounds.size() >= gMaxPolygonPoints)      // || argc == i + 1 might be needed...
         break;

      p.set( (F32) atof(argv[i]) * gridSize, (F32) atof(argv[i+1]) * gridSize );

   if(i == firstCoord || p != lastP)
         mPolyBounds.push_back(p);

      lastP.set(p);
   }

   // Check if last point was same as first; if so, scrap it
   if(!allowFirstAndLastPointToBeEqual && mPolyBounds.first() == mPolyBounds.last())
      mPolyBounds.erase(mPolyBounds.size() - 1);
}



U32 Polyline::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   U32 size = stream->readEnum(gMaxPolygonPoints) + 1;

   for(U32 i = 0; i < size; i++)
   {
      Point p;
      stream->read(&p.x);
      stream->read(&p.y);
      mPolyBounds.push_back(p);
   }

   return size;
}


Rect Polyline::computePolyExtents()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);

   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);

   return extent;
}


////////////////////////////////////////
////////////////////////////////////////

void Polygon::processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
{
   Parent::processPolyBounds(argc, argv, firstCoord, gridSize, false);

   mCentroid = findCentroid(mPolyBounds);
}


U32 Polygon::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   U32 size = Parent::unpackUpdate(connection, stream);

   if(size)
   {
      Triangulate::Process(mPolyBounds, mPolyFill);
      //TNLAssert(mPolyFill.size() > 0, "Bogus polygon geometry detected!"); // should be checked in a different place...

      mCentroid = findCentroid(mPolyBounds);
      mLabelAngle = angleOfLongestSide(mPolyBounds);
   }

   return size;
}


};

