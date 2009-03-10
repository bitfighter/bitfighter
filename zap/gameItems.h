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


#ifndef _GAME_ITEMS_H_
#define _GAME_ITEMS_H_

#include "item.h"
#include "ship.h"
#include "gameObjectRender.h"
#include "../glut/glutInclude.h"

namespace Zap
{

class RepairItem : public PickupItem
{
private:
   typedef PickupItem Parent;
   S32 mRepopDelay;

public:
   RepairItem(Point p = Point());   // Constructor
   void processArguments(S32 argc, const char **argv);
   bool pickup(Ship *theShip);
   void onClientPickup();
   U32 getRepopDelay();
   void renderItem(Point pos);

   TNL_DECLARE_CLASS(RepairItem);
};



static const S32 AsteroidDesigns = 2;// 5;
static const S32 AsteroidPoints = 12;
static const F32 mRenderSize[] = { 1, .5, .25, -1 };      // Must end in -1
static const S32 mSizeIndexLength = sizeof(mRenderSize) / sizeof(S32) - 1;
static const F32 AsteroidRadius = 89;

static const S32 AsteroidCoords[AsteroidDesigns][AsteroidPoints][2] =   // <== Wow!  A 3D array!
{
   { { 43, -80 }, { 84, -47 }, { 58, -5 }, { 81, 41 }, { 21, 79 }, { 0, 79 }, { -10, 79 }, { -47, 79 }, { -78, 49 }, { -78, -43 }, { -40, -80 }, { 0, -46 } },
   { { 83, 41 }, { 83, -18 }, { 42, -81 }, { 42, -83 }, { 2, -7 }, { -38, -81 }, { -79, -41 }, { -56, -10 }, { -79, 48 }, { -15, 80 }, { 43, 80 }, { 43, 17 } }
};


class Asteroid : public Item
{

typedef Item Parent;

private:
   S32 mSizeIndex;    
   bool hasExploded;
   S32 mDesign;

public:
   Asteroid();     // Constructor

   void renderItem(Point pos);
   bool getCollisionPoly(U32 state, Vector<Point> &polyPoints);
   bool getCollisionCircle(U32 state, Point center, F32 radius);
   bool collide(GameObject *otherObject);

   void damageObject(DamageInfo *theInfo);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void emitAsteroidExplosion(Point pos);

   TNL_DECLARE_CLASS(Asteroid);
};


class TestItem : public Item
{
public:
   TestItem();   // Constructor

   void renderItem(Point pos);
   void damageObject(DamageInfo *theInfo);
   bool getCollisionPoly(U32 state, Vector<Point> &polyPoints);

   TNL_DECLARE_CLASS(TestItem);
};


class ResourceItem : public Item
{
public:
   ResourceItem();      // Constructor

   void renderItem(Point pos);
   bool collide(GameObject *hitObject);
   void damageObject(DamageInfo *theInfo);
   
   TNL_DECLARE_CLASS(ResourceItem);
};

};

#endif
