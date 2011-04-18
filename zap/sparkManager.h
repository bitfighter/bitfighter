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

#ifndef _FXMANAGER_H_
#define _FXMANAGER_H_

#include "gameObject.h"     
#include "Point.h"

namespace Zap
{

namespace FXManager
{
   enum SparkType
   {
      SparkTypePoint = 0,
      SparkTypeLine,
      SparkTypeCount
   };

   void init();
   void emitSpark(Point pos, Point vel, Color color, F32 ttl=0, SparkType=SparkTypePoint);
   void emitExplosion(Point pos, F32 size, Color *colorArray, U32 numColors);
   void emitBurst(Point pos, Point scale, Color color1, Color color2);
   void emitBurst(Point pos, Point scale, Color color1, Color color2, U32 count);
   void emitBlast(Point pos, U32 size);

   void emitTeleportInEffect(Point pos, U32 type);
   void tick( F32 dT);
   void render(S32 renderPass);
};

class FXTrail
{
private:
   struct TrailNode
   {
      Point pos;
      S32   ttl;
      bool  boosted;
      bool  invisible;
   };

   Vector<TrailNode> mNodes;

   U32 mDropFreq;
   S32 mLength;

   FXTrail *mNext;

   static FXTrail *mHead;
   void registerTrail();
   void unregisterTrail();

public:
   FXTrail(U32 dropFrequency = 32, U32 len = 15);
   ~FXTrail();

   /// Update the point this trail is attached to.
   void update(Point pos, bool boosted = false, bool invisible = false);

   void tick(U32 dT);

   void render();

   void reset();

   Point getLastPos();

   static void renderTrails();
};

};

#endif

