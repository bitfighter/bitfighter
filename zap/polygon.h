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

#ifndef _POLYGON_H_
#define _POLYGON_H_

#include "SweptEllipsoid.h"      // For polygon triangulation


namespace Zap
{

extern S32 gMaxPolygonPoints;

class Polygon
{
protected:

   Vector<Point> mPolyBounds;
   Vector<Point> mPolyFill;      // Triangles used for rendering polygon fill

   Point mCentroid;
   F32 mLabelAngle;



   void packPolygonUpdate(GhostConnection *connection, BitStream *stream)
   {
      stream->writeEnum(mPolyBounds.size(), gMaxPolygonPoints);
      for(S32 i = 0; i < mPolyBounds.size(); i++)
      {
         stream->write(mPolyBounds[i].x);
         stream->write(mPolyBounds[i].y);
      }
   }


   U32 unpackPolygonUpdate(GhostConnection *connection, BitStream *stream)
   {
      U32 size = stream->readEnum(gMaxPolygonPoints);
      for(U32 i = 0; i < size; i++)
      {
         Point p;
         stream->read(&p.x);
         stream->read(&p.y);
         mPolyBounds.push_back(p);
      }
      if(size)
      {
         Triangulate::Process(mPolyBounds, mPolyFill);
         mCentroid = centroid(mPolyBounds);
         mLabelAngle = angleOfLongestSide(mPolyBounds);
      }

      return size;
   }


   // Read a series of points from a command line, and add them to a Vector of points
   void static processPolyBounds(S32 argc, const char **argv, S32 firstCoord, Vector<Point> &polyBounds, F32 gridSize)
   {
      for(S32 i = firstCoord; i < argc; i += 2)
      {
         // Put a cap on the number of vertices in a polygon
         if(polyBounds.size() >= gMaxPolygonPoints)
            break;

         Point p;
         p.x = (F32) atof(argv[i]) * gridSize;
         p.y = (F32) atof(argv[i+1]) * gridSize;
         polyBounds.push_back(p);
      }
   }

};




};

#endif   // _POLYGON_H_