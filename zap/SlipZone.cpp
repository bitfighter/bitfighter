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


#include "BfObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "stringUtils.h"
#include "gameObjectRender.h"
#include "polygon.h"
#include "ship.h"
#include "SlipZone.h"
#include "game.h"

namespace Zap
{

SlipZone::SlipZone()     // Constructor
{
   setTeam(0);
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = SlipZoneTypeNumber;
   slipAmount = 0.1f;
}


SlipZone *SlipZone::clone() const
{
   return new SlipZone(*this);
}


void SlipZone::render()
{
   renderSlipZone(getOutline(), getFill(), getCentroid());
}


void SlipZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   render();
   PolygonObject::renderEditor(currentScale, snappingToWallCornersEnabled);
}


S32 SlipZone::getRenderSortValue()
{
   return -1;
}


bool SlipZone::processArguments(S32 argc2, const char **argv2, Game *game)
{
   // Need to handle or ignore arguments that starts with letters,
   // so a possible future version can add parameters without compatibility problem.
   S32 argc = 0;
   const char *argv[Geometry::MAX_POLY_POINTS * 2 + 1];
   for(S32 i = 0; i < argc2; i++)  // the idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char c = argv2[i][0];
      //switch(c)
      //{
      //case 'A': Something = atof(&argv2[i][1]); break;  // using second char to handle number
      //}
      if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
      {
			if(argc < Geometry::MAX_POLY_POINTS * 2 + 1)
         {  argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   if(argc < 6)
      return false;

   if(argc & 1)   // Odd number of arg count (7,9,11) to allow optional slipAmount arg
   {
      slipAmount = (F32)atof(argv[0]);
      readGeom(argc, argv, 1, game->getGridSize());
   }
   else           // Even number of arg count (6,8,10)
      readGeom(argc, argv, 0, game->getGridSize());

   updateExtentInDatabase();

   return true;
}


const char *SlipZone::getEditorHelpString()
{
   return "Areas of higher than normal inertia.";
}


const char *SlipZone::getPrettyNamePlural()
{
   return "Inertia zones";
}


const char *SlipZone::getOnDockName()
{
   return "Inertia";
}


const char *SlipZone::getOnScreenName()
{
   return "Inertia";
}


string SlipZone::toLevelCode(F32 gridSize) const
{
   return string(appendId(getClassName())) + " " + ftos(slipAmount, 3) + " " + geomToLevelCode(gridSize);
}


void SlipZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();
}


const Vector<Point> *SlipZone::getCollisionPoly() const
{
   return getOutline();
}


bool SlipZone::collide(BfObject *hitObject) 
{
   if(!isGhost() && isShipType(hitObject->getObjectTypeNumber()))
   {
      //logprintf("IN A SLIP ZONE!!");
   }
   return false;
}


U32 SlipZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   packGeom(connection, stream);
   stream->write(slipAmount);
   return 0;
}


void SlipZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   unpackGeom(connection, stream);
   stream->read(&slipAmount);
}


TNL_IMPLEMENT_NETOBJECT(SlipZone);


};
