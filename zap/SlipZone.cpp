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


#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "UI.h"
#include "gameObjectRender.h"
#include "../glut/glutInclude.h"
#include "polygon.h"
#include "ship.h"
#include "SlipZone.h"

namespace Zap
{

extern S32 gMaxPolygonPoints;

SlipZone::SlipZone()     // Constructor
{
   mTeam = 0;
   mNetFlags.set(Ghostable);
   mObjectTypeMask = SlipZoneType | CommandMapVisType;
	slipAmount = 0.1;
}


void SlipZone::render()
{
   renderSlipZone(mPolyBounds, mPolyFill, getExtent());
}


S32 SlipZone::getRenderSortValue()
{
   return -1;
}


bool SlipZone::processArguments(S32 argc2, const char **argv2)
{
   // Need to handle or ignore arguments that starts with letters,
   // so a possible future version can add parameters without compatibility problem.
   S32 argc = 0;
   const char *argv[65]; // 32 * 2 + 1 = 65
   for(S32 i=0; i<argc2; i++)  // the idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char c = argv2[i][0];
      //switch(c)
      //{
      //case 'A': Something = atof(&argv2[i][1]); break;  // using second char to handle number
      //}
      if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
      {
         if(argc < 65)
         {  argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   if(argc < 6)
      return false;

	if(argc & 1) // odd number of arg count (7,9,11) to allow optional slipAmount arg
	{
		slipAmount = atof(argv[0]);
      processPolyBounds(argc-1, argv+1, 0, getGame()->getGridSize());
	}else // even number of arg count (6,8,10)
	{
      processPolyBounds(argc, argv, 0, getGame()->getGridSize());
	}
   computeExtent();

   /*for(S32 i = 1; i < argc; i += 2)
   {
      // Put a cap on the number of vertices in a polygon
      if(mPolyBounds.verts.size() >= gMaxPolygonPoints)
         break;

      Point p;
      p.x = atof(argv[i]) * getGame()->getGridSize();
      p.y = atof(argv[i+1]) * getGame()->getGridSize();
      mPolyBounds.push_back(p);
   }*/

   return true;
}


void SlipZone::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();

   getGame()->mObjectsLoaded++;
}


void SlipZone::computeExtent()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);
   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);
   setExtent(extent);
}


bool SlipZone::getCollisionPoly(Vector<Point> &polyPoints)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      polyPoints.push_back(mPolyBounds[i]);
   return true;
}


bool SlipZone::collide(GameObject *hitObject) 
{
   if(!isGhost() && hitObject->getObjectTypeMask() & (ShipType))
   {
      //logprintf("IN A SLIP ZONE!!");
   }
   return false;
}


U32 SlipZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   Polygon::packUpdate(connection, stream);
   stream->writeFloat(slipAmount, 8);
   return 0;
}


void SlipZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(Polygon::unpackUpdate(connection, stream))
      computeExtent();

   slipAmount = stream->readFloat(8);
}

TNL_IMPLEMENT_NETOBJECT(SlipZone);

};
