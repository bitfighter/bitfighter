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

#include "Point.h"
#include "Color.h"
#include "SparkTypesEnum.h"

#include "tnlVector.h"

namespace Zap { namespace UI
{

class FxManager
{
   struct Spark
   {
      Point pos;
      Color color;
      F32 alpha;
      S32 ttl;     // Milliseconds
      Point vel;
   };

   struct DebrisChunk
   {
      Vector<Point> points;
      Color color;

      Point pos;
      Point vel;
      S32 ttl;     // Milliseconds
      F32 angle;
      F32 rotation;

      void idle(U32 timeDelta);
      void render();
   };

   Vector<DebrisChunk> mDebrisChunks;

   struct TextEffect
   {
      string text;
      Color color;
      Point pos;
      Point vel;
      F32 size;
      F32 growthRate;
      S32 ttl;    // Milliseconds
      
      void idle(U32 timeDelta);
      void render();
   };

   Vector<TextEffect> mTextEffects;

   struct TeleporterEffect;
   TeleporterEffect *teleporterEffects;

   static const U32 MAX_SPARKS = 8192;    // Make this an even number

   U32 firstFreeIndex[SparkTypeCount];            // Tracks next available slot when we have fewer than MAX_SPARKS 
   U32 lastOverwrittenIndex[SparkTypeCount];      // Keep track of which spark we last overwrote

   Spark mSparks[SparkTypeCount][MAX_SPARKS];     // Our sparks themselves... two types, each with room for MAX_SPARKS

public:
   FxManager();
   virtual ~FxManager();
   void emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl = 0, SparkType = SparkTypePoint);
   void emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors);
   void emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2);
   void emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2, U32 count);
   void emitBlast(const Point &pos, U32 size);
   void emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation);
   void emitTextEffect(const string &text, const Color &color, const Point &pos);
   void emitTeleportInEffect(const Point &pos, U32 type);

   void idle(U32 timeDelta);
   void render(S32 renderPass, F32 commanderZoomFraction);
   void clearSparks();
};

class FxTrail
{
private:
   struct TrailNode
   {
      Point pos;
      S32   ttl;    // Milliseconds
      TrailProfile profile;
   };


   Vector<TrailNode> mNodes;

   U32 mDropFreq;
   S32 mLength;

   FxTrail *mNext;

   static FxTrail *mHead;
   void registerTrail();
   void unregisterTrail();

public:
   FxTrail(U32 dropFrequency = 32, U32 len = 15);
   ~FxTrail();

   /// Update the point this trail is attached to.
   void update(Point pos, TrailProfile profile);

   void idle(U32 timeDelta);

   void render();

   void reset();

   Point getLastPos();

   static void renderTrails();
};

}  }     // Nested namespace

#endif

