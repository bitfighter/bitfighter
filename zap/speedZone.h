//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo relased for Torque Network Library by GarageGames.com
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

#ifndef _GO_FAST_H_
#define _GO_FAST_H_

#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "UI.h"
#include "gameObjectRender.h"
#include "ship.h"
#include "../glut/glutInclude.h"

namespace Zap
{

class SpeedZone : public GameObject
{
private:

   typedef GameObject Parent;
   Vector<Point> mPolyBounds;

   //struct Exclusion {
   //   Ship *ship;
   //   U32 time;
   //};

   // To keep a ship from triggering the SpeedZone multiple times in one go, we'll exclude any ships
   // that hit the zone from hitting it again within a brief time.
   //Vector<Exclusion> mExclusions;

   // Take our basic inputs, pos and dir, and expand them into a three element
   // vector (the three points of our triangle graphic), and compute its extent
   void preparePoints();

public:
   enum {
      halfWidth = 25,
      height = 64,
      defaultSnap = 0,     // 0 = false, 1 = true

      InitMask     = BIT(0),
      HitMask      = BIT(1),
   };

   static const U16 minSpeed = 500;       // How slow can you go?
   static const U16 maxSpeed = 5000;      // Max speed for the goFast
   static const U16 defaultSpeed = 2000;  // Default speed if none specified

   F32 mRotateSpeed;
   U32 mUnpackInit;  // Some form of counter, to know that it is a rotating speed zone.

   Point pos;
   Point dir;
   U16 mSpeed;             // Speed at which ship is propelled, defaults to defaultSpeed
   bool mSnapLocation;     // If true, ship will be snapped to center of speedzone before being ejected
   
   SpeedZone();   // Constructor
   static Vector<Point> generatePoints(Point pos, Point dir);
   void render();
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv);           // Create objects from parameters stored in level file
   void onAddedToGame(Game *theGame);
   void computeExtent();                                         // Bounding box for quick collision-possibility elimination

   bool getCollisionPoly(Vector<Point> &polyPoints);  // More precise boundary for precise collision detection
   bool collide(GameObject *hitObject);
   void idle(GameObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(SpeedZone);
};


};

#endif


