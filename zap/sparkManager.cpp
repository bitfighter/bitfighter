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

#include "sparkManager.h"
#include "teleporter.h"
#include "gameObjectRender.h"
#include "../glut/glutInclude.h"

#include <math.h>

using namespace TNL;

namespace Zap
{

extern IniSettings gIniSettings;

namespace FXManager
{



struct Spark
{
   Point pos;
   Color color;
   F32 alpha;
   F32 ttl;
   Point vel;
};


const U32 MAX_SPARKS = 8192;    // Make this an even number


U32 firstFreeIndex[SparkTypeCount];            // Tracks next available slot when we have fewer than MAX_SPARKS 
U32 lastOverwrittenIndex[SparkTypeCount];      // Keep track of which spark we last overwrote

Spark gSparks[SparkTypeCount][MAX_SPARKS];     // Our sparks themselves... two types, each with room for MAX_SPARKS


void init()
{
   for(U32 i = 0; i < SparkTypeCount; i++)
   {
      firstFreeIndex[i] = 0;
      lastOverwrittenIndex[i] = 500;
   }
}

// Create a new spark.   ttl = Time to Live (secs)
void emitSpark(Point pos, Point vel, Color color, F32 ttl, SparkType sparkType)
{
   Spark *s;
   Spark *s2;

   U32 sparkIndex;

   U8 slotsNeeded = (sparkType == SparkTypePoint ? 1 : 2);             // We need a U2 data type here!

   // Make sure we have room for an additional spark.  Regular sparks need one slot, line sparks need two.
   if(firstFreeIndex[sparkType] >= MAX_SPARKS - slotsNeeded)           // Spark list is full... need to overwrite an older spark
   {
      // Out of room for new sparks.  We'll jump elsewhere in our array and overwrite some older spark.
      // Overwrite every nth spark to avoid noticable artifacts by grabbing too many sparks from one place.
      // But make sure we grab a multiple of 2 to avoid wierdness with SparkTypeLine sparks, wich require proper byte alignment.
      // This doesn't matter for point sparks, but neither does it hurt.
      sparkIndex = (lastOverwrittenIndex[sparkType] + 100) % (MAX_SPARKS / 2 - 1) * 2;
      lastOverwrittenIndex[sparkType] = sparkIndex;
      TNLAssert(sparkIndex < MAX_SPARKS - slotsNeeded, "Spark error!");
   }
   else
   {
      sparkIndex = firstFreeIndex[sparkType];
      firstFreeIndex[sparkType] += slotsNeeded;     // Point sparks take 1 slot, line sparks need 2
   }
   
   s = gSparks[sparkType] + sparkIndex;   // Assign our spark to its slot

   s->pos = pos;
   s->vel = vel;
   s->color = color;

   // Use ttl if it was specified, otherwise pick something random
   s->ttl = ttl ? ttl : 15 * TNL::Random::readF() * TNL::Random::readF();

   if(sparkType == SparkTypeLine)   // Line sparks require two points; add the second here
   {
      s2 = gSparks[sparkType] + sparkIndex + 1;   // Since we know we had room for two, this one should be available
      Point len = vel;
      len.normalize(20);
      s2->pos = (pos - len);
      s2->vel = vel;
      s2->color = Color(color.r * 1, color.g * 0.25, color.b * 0.25);    // Give the trailing edge of this spark a fade effect
      s2->ttl = s->ttl;
   }
}

struct TeleporterEffect
{
   Point pos;
   S32 time;
   U32 type;
   TeleporterEffect *nextEffect;
};

TeleporterEffect *teleporterEffects = NULL;

void emitTeleportInEffect(Point pos, U32 type)
{
   TeleporterEffect *e = new TeleporterEffect;
   e->pos = pos;
   e->time = 0;
   e->type = type;
   e->nextEffect = teleporterEffects;
   teleporterEffects = e;
}

void tick( F32 dT )
{
   for(U32 j = 0; j < SparkTypeCount; j++)
      for(U32 i = 0; i < firstFreeIndex[j]; )
      {
         Spark *theSpark = gSparks[j] + i;
         if(theSpark->ttl <= dT)
         {                          // Spark is dead -- remove it
            firstFreeIndex[j]--;
            *theSpark = gSparks[j][firstFreeIndex[j]];
         }
         else
         {
            theSpark->ttl -= dT;
            theSpark->pos += theSpark->vel * dT;
            if ((SparkType) j == SparkTypePoint)
            {
               if(theSpark->ttl > 1)
                  theSpark->alpha = 1;
               else
                  theSpark->alpha = theSpark->ttl;
            }
            else if ((SparkType) j == SparkTypeLine)
            {
               if(theSpark->ttl > 0.25)
                  theSpark->alpha = 1;
               else
                  theSpark->alpha = theSpark->ttl / 0.25;
            }

            i++;
         }
      }

   for(TeleporterEffect **walk = &teleporterEffects; *walk; )
   {
      TeleporterEffect *temp = *walk;
      temp->time += (S32)(dT * 1000);
      if(temp->time > Teleporter::TeleportInExpandTime)
      {
         *walk = temp->nextEffect;
         delete temp;
      }
      else
         walk = &(temp->nextEffect);
   }
}

void render(S32 renderPass)
{
   // The teleporter effects should render under the ships and such
   if(renderPass == 0)
   {
      for(TeleporterEffect *walk = teleporterEffects; walk; walk = walk->nextEffect)
      {
         F32 radius = walk->time / F32(Teleporter::TeleportInExpandTime);
         F32 alpha = 1.0;
         if(radius > 0.5)
            alpha = (1 - radius) / 0.5;
         renderTeleporter(walk->pos, walk->type, false, Teleporter::TeleportInExpandTime - walk->time, radius, Teleporter::TeleportInRadius, alpha, Vector<Point>(), false);
      }
   }
   else if(renderPass == 1)      // Time for sparks!!
   {
      for (S32 i = SparkTypeCount - 1; i >= 0; i --)     // Loop through our different spark types
      {
         glPointSize( gDefaultLineWidth );
         glEnableBlend;

         glEnableClientState(GL_COLOR_ARRAY);
         glEnableClientState(GL_VERTEX_ARRAY);

         glVertexPointer(2, GL_FLOAT, sizeof(Spark), &gSparks[i][0].pos);     // Where to find the vertices -- see OpenGL docs
         glColorPointer(4, GL_FLOAT, sizeof(Spark), &gSparks[i][0].color);    // Where to find the colors -- see OpenGL docs

         if((SparkType) i == SparkTypePoint)
            glDrawArrays(GL_POINTS, 0, firstFreeIndex[i]);
         else if((SparkType) i == SparkTypeLine)
            glDrawArrays(GL_LINES, 0, firstFreeIndex[i]);

         glDisableClientState(GL_COLOR_ARRAY);
         glDisableClientState(GL_VERTEX_ARRAY);

         glDisableBlend;
      }
   }
}

#define dr(x) (float) x * Float2Pi / 360     // degreesToRadians()

// Create a circular pattern of long sparks, a-la bomb in Gridwars
void emitBlast(Point pos, U32 size)
{
   const F32 speed = 800.0f;
   for(U32 i = 0; i < 360; i+=1)
   {
      Point dir = Point(cos(dr(i)), sin(dr(i)));
      // Emit a ring of bright orange sparks, as well as a whole host of yellow ones
      emitSpark(pos + dir * 50, dir * TNL::Random::readF() * 500, Color(1, 1, 0), TNL::Random::readF() * 1000 / speed, SparkTypePoint );
      emitSpark(pos + dir * 50, dir * speed, Color(1, .8, .45), (F32) (size - 50) / speed, SparkTypeLine);
   }
}


void emitExplosion(Point pos, F32 size, Color *colorArray, U32 numColors)
{
   for(U32 i = 0; i < (250.0 * size); i++)
   {
      F32 th = TNL::Random::readF() * 2 * 3.14;
      F32 f = (TNL::Random::readF() * 2 - 1) * 400 * size;
      U32 colorIndex = TNL::Random::readI() % numColors;

      emitSpark(pos, Point(cos(th)*f, sin(th)*f), colorArray[colorIndex], TNL::Random::readF()*size + 2*size);
   }
}


void emitBurst(Point pos, Point scale, Color color1, Color color2)
{
   emitBurst(pos, scale, color1, color2, 250);
}

void emitBurst(Point pos, Point scale, Color color1, Color color2, U32 count)
{
   F32 size = 1;

   for(U32 i = 0; i < ((F32) count * size); i++)
   {

      F32 th = TNL::Random::readF() * 2 * FloatPi;
      F32 f = (TNL::Random::readF() * 0.1 + 0.9) * 200 * size;
      F32 t = TNL::Random::readF();

      Color r;

      r.interp(t, color1, color2);

      emitSpark(
         pos + Point(cos(th)*scale.x, sin(th)*scale.y),
         Point(cos(th)*scale.x*f, sin(th)*scale.y*f),
         r,
         TNL::Random::readF() * scale.len() * 3 + scale.len()
      );
   }
}

};

//-----------------------------------------------------------------------------

FXTrail::FXTrail(U32 dropFrequency, U32 len)
{
   mDropFreq = dropFrequency;
   mLength   = len;
   registerTrail();
}

FXTrail::~FXTrail()
{
   unregisterTrail();
}

void FXTrail::update(Point pos, bool boosted, bool invisible)
{
   if(mNodes.size() < mLength)
   {
      TrailNode t;
      t.pos = pos;
      t.ttl = mDropFreq;
      t.boosted = boosted;
      t.invisible = invisible;

      mNodes.push_front(t);
   }
   else
   {
      mNodes[0].pos = pos;
      if(invisible)
         mNodes[0].invisible = true;
      else if(boosted)
         mNodes[0].boosted = true;
   }
}

void FXTrail::tick(U32 dT)
{
   if(mNodes.size() == 0)
      return;

   mNodes.last().ttl -= dT;
   if(mNodes.last().ttl <= 0)
      mNodes.pop_back();      // Delete last item
}

void FXTrail::render()
{
   glBegin(GL_LINE_STRIP);

   for(S32 i=0; i<mNodes.size(); i++)
   {
      F32 t = ((F32)i/(F32)mNodes.size());

      if(mNodes[i].invisible)
         glColor4f(0.f,0.f,0.f,0.f);
      else if(mNodes[i].boosted)
         glColor4f(1.f - t, 1.f - t, 0.f, 1.f-t);
      else
         glColor4f(1.f - 2*t, 1.f - 2*t, 1.f, 0.7f-0.7*t);

      glVertex2f(mNodes[i].pos.x, mNodes[i].pos.y);
   }

   glEnd();
}

void FXTrail::reset()
{
   mNodes.clear();
}

Point FXTrail::getLastPos()
{
   if(mNodes.size())
   {
      return mNodes[0].pos;
   }
   else
      return Point(0,0);
}

FXTrail * FXTrail::mHead = NULL;

void FXTrail::registerTrail()
{
  FXTrail *n = mHead;
  mHead = this;
  mNext = n;
}

void FXTrail::unregisterTrail()
{
   // Find ourselves in the list (lame O(n) solution)
   FXTrail *w = mHead, *p = NULL;
   while(w)
   {
      if(w == this)
      {
         if(p)
         {
            p->mNext = w->mNext;
         }
         else
         {
            mHead = w->mNext;
         }
      }
      p = w;
      w = w->mNext;
   }
}

void FXTrail::renderTrails()
{
   glEnableBlend;

   FXTrail *w = mHead;
   while(w)
   {
      w->render();
      w = w->mNext;
   }

   glDisableBlend;
}

};


