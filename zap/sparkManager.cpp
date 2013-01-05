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
#include "Colors.h"
#include "config.h"
#include "ClientGame.h"    // For getCommanderZoomFraction()

#include "OpenglUtils.h"
#include "UI.h"

#include <math.h>

using namespace TNL;

namespace Zap
{


FXManager::FXManager()
{
   for(U32 i = 0; i < SparkTypeCount; i++)
   {
      firstFreeIndex[i] = 0;
      lastOverwrittenIndex[i] = 500;
   }
   teleporterEffects = NULL;
}


FXManager::~FXManager()
{
   // Do nothing
}


// Create a new spark.   ttl = Time To Live (milliseconds)
void FXManager::emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, SparkType sparkType)
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
      firstFreeIndex[sparkType] += slotsNeeded;    // Point sparks take 1 slot, line sparks need 2
   }

   s = gSparks[sparkType] + sparkIndex;            // Assign our spark to its slot

   s->pos = pos;
   s->vel = vel;
   s->color = color;

   // Use ttl if it was specified, otherwise pick something random
   s->ttl = ttl > 0 ? ttl : 15 * TNL::Random::readI(0, 1000);  // 0 - 15 seconds

   if(sparkType == SparkTypeLine)                  // Line sparks require two points; add the second here
   {
      s2 = gSparks[sparkType] + sparkIndex + 1;    // Since we know we had room for two, this one should be available
      Point len = vel;
      len.normalize(20);
      s2->pos = (pos - len);
      s2->vel = vel;
      s2->color = Color(color.r * 1, color.g * 0.25, color.b * 0.25);    // Give the trailing edge of this spark a fade effect
      s2->ttl = s->ttl;
   }
}


#define dr(x) (float) (x) * FloatTau / 360    // degreesToRadians()
#define rd(x) (float) (x) * 360 / FloatTau    // radiansToDegrees()


void FXManager::DebrisChunk::idle(U32 timeDelta)
{
   F32 dT = F32(timeDelta) * 0.001f;      // Convert timeDelta to seconds

   pos   += vel      * dT;
   angle += rotation * dT;
   ttl   -= timeDelta;
}


void FXManager::DebrisChunk::render()
{
   glPushMatrix();

   glTranslate(pos);
   glRotatef(rd(angle), 0, 0, 1);

   F32 alpha = 1;
   if(ttl < 250)
      alpha = ttl / 250.f;

   glColor(color, alpha);

   renderPointVector(&points, GL_LINE_LOOP);

   glPopMatrix();
}


void FXManager::TextEffect::idle(U32 timeDelta)
{
   F32 dTsecs = F32(timeDelta) * 0.001f;     // Convert timeDelta to seconds

   pos += vel * dTsecs;
   if(size < 10)
      size += growthRate * dTsecs;

   ttl -= timeDelta;
}


void FXManager::TextEffect::render()
{
   F32 alpha = 1;
   if(ttl < 300)
      alpha = F32(ttl) / 300.f;     // Fade as item nears the end of its life
   glColor(color, alpha);
   //glLineWidth(size);
   glPushMatrix();
   glTranslate(pos);
   glScale(size);
   drawStringc(0, 0, 12, text.c_str(), false);
   glPopMatrix();
   //glLineWidth(gDefaultLineWidth);
}


void FXManager::emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation)
{
   DebrisChunk debrisChunk;

   debrisChunk.points   = points;
   debrisChunk.color    = color;
   debrisChunk.pos      = pos;
   debrisChunk.vel      = vel;
   debrisChunk.ttl      = ttl;
   debrisChunk.angle    = angle;
   debrisChunk.rotation = rotation;

   mDebrisChunks.push_back(debrisChunk);
}


void FXManager::emitTextEffect(string text, Color color, Point pos)
{
   TextEffect textEffect;

   textEffect.text  = text;
   textEffect.color = color;
   textEffect.pos   = pos;

   textEffect.vel  = Point(0,-130);
   textEffect.size = 0;
   textEffect.growthRate = 20;
   textEffect.ttl = 1500;

   mTextEffects.push_back(textEffect);
}


struct FXManager::TeleporterEffect
{
   Point pos;
   S32 time;
   U32 type;
   TeleporterEffect *nextEffect;
};


void FXManager::emitTeleportInEffect(Point pos, U32 type)
{
   TeleporterEffect *e = new TeleporterEffect;

   e->pos = pos;
   e->time = 0;
   e->type = type;
   e->nextEffect = teleporterEffects;
   teleporterEffects = e;
}


void FXManager::idle(U32 timeDelta)
{
   F32 dTsecs = timeDelta * .001f;

   for(U32 j = 0; j < SparkTypeCount; j++)
      for(U32 i = 0; i < firstFreeIndex[j]; )
      {
         Spark *theSpark = gSparks[j] + i;
         if(theSpark->ttl < (S32)timeDelta)
         {                          // Spark is dead -- remove it
            firstFreeIndex[j]--;
            *theSpark = gSparks[j][firstFreeIndex[j]];
         }
         else
         {
            theSpark->ttl -= timeDelta;
            theSpark->pos += theSpark->vel * dTsecs;
            if ((SparkType) j == SparkTypePoint)
            {
               if(theSpark->ttl > 1000)
                  theSpark->alpha = 1;
               else
                  theSpark->alpha = F32(theSpark->ttl) / 1000.f;
            }
            else if ((SparkType) j == SparkTypeLine)
            {
               if(theSpark->ttl > 250)
                  theSpark->alpha = 1;
               else
                  theSpark->alpha = F32(theSpark->ttl) / 250.f;
            }

            i++;
         }
      }


   // Kill off any old debris chunks, advance the others
   for(S32 i = 0; i < mDebrisChunks.size(); i++)
   {
      if(mDebrisChunks[i].ttl < (S32)timeDelta)
      {
         mDebrisChunks.erase_fast(i);
         i--;
      }
      else
         mDebrisChunks[i].idle(timeDelta);
   }


   // Same for our TextEffects
   for(S32 i = 0; i < mTextEffects.size(); i++)
   {
      if(mTextEffects[i].ttl < (S32)timeDelta)
      {
         mTextEffects.erase_fast(i);
         i--;
      }
      else
         mTextEffects[i].idle(timeDelta);
   }


   for(TeleporterEffect **walk = &teleporterEffects; *walk; )
   {
      TeleporterEffect *temp = *walk;
      temp->time += timeDelta;
      if(temp->time > Teleporter::TeleportInExpandTime)
      {
         *walk = temp->nextEffect;
         delete temp;
      }
      else
         walk = &(temp->nextEffect);
   }
}


void FXManager::render(S32 renderPass)
{
   TNLAssert(dynamic_cast<ClientGame *>(this), "Not a ClientGame");
   ClientGame *clientGame = static_cast<ClientGame *>(this);
   F32 commanderZoomFraction = clientGame->getCommanderZoomFraction();

   // The teleporter effects should render under the ships and such
   if(renderPass == 0)
   {
      for(TeleporterEffect *walk = teleporterEffects; walk; walk = walk->nextEffect)
      {
         F32 radius = walk->time / F32(Teleporter::TeleportInExpandTime);
         F32 alpha = 1.0;

         if(radius > 0.5)
            alpha = (1 - radius) / 0.5f;

         Vector<Point> dummy;
         
         renderTeleporter(walk->pos, walk->type, false, Teleporter::TeleportInExpandTime - walk->time, commanderZoomFraction,
                          radius, Teleporter::TeleportInRadius, alpha, &dummy);
      }
   }

   else if(renderPass == 1)      // Time for sparks!!
   {
      for(S32 i = SparkTypeCount - 1; i >= 0; i --)     // Loop through our different spark types
      {
         glPointSize(gDefaultLineWidth);

         TNLAssert(glIsEnabled(GL_BLEND), "We expect blending to be on here!");

         glEnableClientState(GL_COLOR_ARRAY);
         glEnableClientState(GL_VERTEX_ARRAY);

         glVertexPointer(2, GL_FLOAT, sizeof(Spark), &gSparks[i][0].pos);     // Where to find the vertices -- see OpenGL docs
         glColorPointer (4, GL_FLOAT, sizeof(Spark), &gSparks[i][0].color);   // Where to find the colors -- see OpenGL docs

         if((SparkType) i == SparkTypePoint)
            glDrawArrays(GL_POINTS, 0, firstFreeIndex[i]);
         else if((SparkType) i == SparkTypeLine)
            glDrawArrays(GL_LINES, 0, firstFreeIndex[i]);

         glDisableClientState(GL_COLOR_ARRAY);
         glDisableClientState(GL_VERTEX_ARRAY);
      }

      for(S32 i = 0; i < mDebrisChunks.size(); i++)
         mDebrisChunks[i].render();

      for(S32 i = 0; i < mTextEffects.size(); i++)
         mTextEffects[i].render();
   }
}


// Create a circular pattern of long sparks, a-la bomb in Gridwars
void FXManager::emitBlast(const Point &pos, U32 size)
{
   const F32 speed = 800.0f;
   for(U32 i = 0; i < 360; i+=1)
   {
      Point dir = Point(cos(dr(i)), sin(dr(i)));
      // Emit a ring of bright orange sparks, as well as a whole host of yellow ones
      emitSpark(pos + dir * 50, dir * TNL::Random::readF() * 500, Colors::yellow, TNL::Random::readI(0, U32(1000.f * F32(1000.f / speed))), SparkTypePoint );
      emitSpark(pos + dir * 50, dir * speed, Color(1, .8, .45), U32(1000.f * F32(size - 50) / speed), SparkTypeLine);
   }
}


void FXManager::emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors)
{
   for(U32 i = 0; i < (250.0 * size); i++)
   {
      F32 th = TNL::Random::readF() * 2 * 3.14f;
      F32 f = (TNL::Random::readF() * 2 - 1) * 400 * size;
      U32 colorIndex = TNL::Random::readI() % numColors;

      emitSpark(pos, Point(cos(th)*f, sin(th)*f), colorArray[colorIndex], TNL::Random::readI(0, 1000) * size + 2000 * size);
   }
}


void FXManager::emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2)
{
   emitBurst(pos, scale, color1, color2, 250);
}


void FXManager::emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2, U32 sparkCount)
{
   F32 size = 1;

   for(U32 i = 0; i < sparkCount; i++)
   {

      F32 th = TNL::Random::readF() * 2 * FloatPi;                // angle
      F32 f = (TNL::Random::readF() * 0.1f + 0.9f) * 200 * size;

      Color color;
      color.interp(TNL::Random::readF(), color1, color2);         // Random blend of color1 and color2

      emitSpark(
            pos + Point(cos(th)*scale.x, sin(th)*scale.y),        // pos
            Point(cos(th)*scale.x*f, sin(th)*scale.y*f),          // vel
            color,                                                // color
            TNL::Random::readI(0, 1000) * scale.len() * 3 + S32(1000.f * scale.len())  // ttl
      );
   }
}


void FXManager::clearSparks()
{
   // Remove all sparks
   for(U32 j = 0; j < SparkTypeCount; j++)
      for(U32 i = 0; i < firstFreeIndex[j]; )
      {
         Spark *theSpark = gSparks[j] + i;
         firstFreeIndex[j]--;
         *theSpark = gSparks[j][firstFreeIndex[j]];
      }
}


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


void FXTrail::update(Point pos, TrailProfile profile)
{
   if(mNodes.size() < mLength)
   {
      TrailNode t;
      t.pos = pos;
      t.ttl = mDropFreq;
      t.profile = profile;

      mNodes.push_front(t);
   }
   else
   {
      mNodes[0].pos = pos;
      mNodes[0].profile = profile;
   }
}


void FXTrail::idle(U32 timeDelta)
{
   if(mNodes.size() == 0)
      return;

   mNodes.last().ttl -= timeDelta;
   if(mNodes.last().ttl < (S32)timeDelta)
      mNodes.pop_back();      // Delete last item
}


void FXTrail::render()
{
   // Largest node size found was 15; I chose buffer of 32
   static F32 fxTrailVertexArray[64];     // 2 coordinates per node
   static F32 fxTrailColorArray[128];     // 4 colors components per node

   for(S32 i = 0; i < mNodes.size(); i++)
   {
      F32 t = ((F32)i / (F32)mNodes.size());

      F32 r, g, b, a, rFade, gFade, bFade, aFade;

      switch(mNodes[i].profile) {
         case Ship:
            r = 1;      g = 1;      b = 1;      a = 0.7f;
            rFade = 2;  gFade = 2;  bFade = 0;  aFade = 0.7f;
            break;

         case CloakedShip:
            r = 0;      g = 0;      b = 0;      a = 0.0f;
            rFade = 0;  gFade = 0;  bFade = 0;  aFade = 0.0f;
            break;

         case TurboShip:
            r = 1;      g = 1;      b = 0;      a = 1;
            rFade = 1;  gFade = 1;  bFade = 0;  aFade = 1;
            break;

         case Seeker:
            r = 0.5f;       g = 0.5f;    b = 0.5f;   a = 0.4f;
            rFade = 0.5f;   gFade = 1;   bFade = 1;  aFade = 0.2f;
            break;

         default:
            TNLAssert(false, "No such profile!");
            break;
      }

      fxTrailColorArray[(4*i) + 0] = r - rFade * t;
      fxTrailColorArray[(4*i) + 1] = g - gFade * t;
      fxTrailColorArray[(4*i) + 2] = b - bFade * t;
      fxTrailColorArray[(4*i) + 3] = a - aFade * t;


      fxTrailVertexArray[(2*i) + 0] = mNodes[i].pos.x;
      fxTrailVertexArray[(2*i) + 1] = mNodes[i].pos.y;
   }

   renderColorVertexArray(fxTrailVertexArray, fxTrailColorArray, mNodes.size(), GL_LINE_STRIP);
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
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   FXTrail *w = mHead;
   while(w)
   {
      w->render();
      w = w->mNext;
   }
}

};


