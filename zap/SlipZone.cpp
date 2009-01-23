//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

// Modeled on LoadoutZone.cpp

#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "UI.h"
#include "gameObjectRender.h"
#include "../glut/glutInclude.h"

namespace Zap
{

extern S32 gMaxPolygonPoints;

class SlipZone : public GameObject
{
   typedef GameObject Parent;
   Vector<Point> mPolyBounds;

public:
   SlipZone()     // Constructor
   {
      mTeam = 0;
      mNetFlags.set(Ghostable);
      mObjectTypeMask = SlipZoneType | CommandMapVisType;
   }

   void render()
   {
      renderSlipZone(mPolyBounds, getExtent());
   }

   S32 getRenderSortValue()
   {
      return -1;
   }

   void processArguments(S32 argc, const char **argv)
   {
      if(argc < 6)
         return;

      processPolyBounds(argc, argv, 0, mPolyBounds);
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
   }

   void onAddedToGame(Game *theGame)
   {
      if(!isGhost())
         setScopeAlways();
   }

   void computeExtent()
   {
      Rect extent(mPolyBounds[0], mPolyBounds[0]);
      for(S32 i = 1; i < mPolyBounds.size(); i++)
         extent.unionPoint(mPolyBounds[i]);
      setExtent(extent);
   }

   bool getCollisionPoly(Vector<Point> &polyPoints)
   {
      for(S32 i = 0; i < mPolyBounds.size(); i++)
         polyPoints.push_back(mPolyBounds[i]);
      return true;
   }

   bool collide(GameObject *hitObject) ////////////////////////////////////////////////////////////////////
   {
      if(!isGhost() && (hitObject->getObjectTypeMask() & ShipType))
         //getGame()->getGameType()->updateShipLoadout(hitObject);
         logprintf("IN A SLIP ZONE!!");
      return false;
   }

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
   {
      stream->writeEnum(mPolyBounds.size(), gMaxPolygonPoints);
      for(S32 i = 0; i < mPolyBounds.size(); i++)
      {
         stream->write(mPolyBounds[i].x);
         stream->write(mPolyBounds[i].y);
      }
      return 0;
   }

   void unpackUpdate(GhostConnection *connection, BitStream *stream)
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
         computeExtent();
   }

   TNL_DECLARE_CLASS(SlipZone);
};

TNL_IMPLEMENT_NETOBJECT(SlipZone);

};