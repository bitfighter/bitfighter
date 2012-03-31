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

#include "gameObjectRender.h"

#include "tnlRandom.h"

#include "UI.h"
#include "projectile.h"
#include "speedZone.h"
#include "soccerGame.h"
#include "CoreGame.h"            // For CORE_PANELS
#include "EngineeredItem.h"      // For TURRET_OFFSET
#include "BotNavMeshZone.h"      // For Border def
#include "version.h"
#include "config.h"              // Only for testing burst graphics below
#include "ScreenInfo.h"
#include "game.h"

#include "UIEditor.h"            // For RenderingStyles enum

#include "SDL_opengl.h"

//#include "pictureloader.h"

#include <math.h>

namespace Zap
{

const Color *HIGHLIGHT_COLOR = &Colors::white;
const Color *SELECT_COLOR = &Colors::yellow;

const float gShapeLineWidth = 2.0f;

void glVertex(const Point &p)
{
   glVertex2f(p.x, p.y);
}


void glScale(F32 scaleFactor)
{
    glScalef(scaleFactor, scaleFactor, 1);
}


void glTranslate(const Point &pos)
{
   glTranslatef(pos.x, pos.y, 0);
}


void setDefaultBlendFunction()
{
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


// geomType should be GL_LINES or GL_POLYGON
void renderPointVector(const Vector<Point> *points, U32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, sizeof(Point), points->address());    
   glDrawArrays(geomType, 0, points->size());

   glDisableClientState(GL_VERTEX_ARRAY);

   // Equivalent to, but amost twice as fast as:

   //glBegin(geomType);
   //   for(S32 i = 0; i < points.size(); i++)
   //      glVertex2f(points[i].x, points[i].y);
   //glEnd();
}


void renderLine(const Vector<Point> *points)
{
   renderPointVector(points, GL_LINE_STRIP);
}


// Use slower method here because we need to visit each point to add offset
void renderPointVector(const Vector<Point> *points, const Point &offset, U32 geomType)
{
   glBegin(geomType);
      for(S32 i = 0; i < points->size(); i++)
         glVertex(points->get(i) + offset);
   glEnd();
}


static void renderVertexArray(const S16 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_SHORT, 0, verts);    
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}


static void renderVertexArray(const F32 verts[], S32 vertCount, S32 geomType)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_FLOAT, 0, verts);    
   glDrawArrays(geomType, 0, vertCount);

   glDisableClientState(GL_VERTEX_ARRAY);
}


void drawHorizLine(S32 x1, S32 x2, S32 y)
{
   glBegin(GL_LINES);
      glVertex2i(x1, y);
      glVertex2i(x2, y);
   glEnd();
}


void drawVertLine(S32 x, S32 y1, S32 y2)
{
   glBegin(GL_LINES);
      glVertex2i(x, y1);
      glVertex2i(x, y2);
   glEnd();
}


// Draw arc centered on pos, with given radius, from startAngle to endAngle.  0 is East, increasing CW
void drawArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle)
{
   glBegin(GL_LINE_STRIP);

      for(F32 theta = startAngle; theta < endAngle; theta += 0.2f)
         glVertex2f(pos.x + cos(theta) * radius, pos.y + sin(theta) * radius);

      // Make sure arc makes it all the way to endAngle...  rounding errors look terrible!
      glVertex2f(pos.x + cos(endAngle) * radius, pos.y + sin(endAngle) * radius);

   glEnd();
}


void drawDashedArc(const Point &center, F32 radius, S32 dashCount, F32 spaceAngle, F32 offset)
{
   F32 interimAngle = FloatTau / dashCount;  

   for(S32 i = 0; i < dashCount; i++)
      drawArc(center, radius, interimAngle * i + offset, (interimAngle * (i + 1)) - spaceAngle + offset);
}


void drawAngledRay(const Point &center, F32 innerRadius, F32 outerRadius, F32 angle)
{
   glBegin(GL_LINE_STRIP);

   glVertex2f(center.x + cos(angle) * innerRadius, center.y + sin(angle) * innerRadius);
   glVertex2f(center.x + cos(angle) * outerRadius, center.y + sin(angle) * outerRadius);

   glEnd();
}


void drawAngledRayCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 rayCount, F32 startAngle, F32 offset)
{
   F32 interimAngle = FloatTau / rayCount;

   for(S32 i = 0; i < rayCount; i++)
      drawAngledRay(center, innerRadius, outerRadius, interimAngle * i + startAngle + offset);
}


void drawDashedHollowArc(const Point &center, F32 innerRadius, F32 outerRadius, S32 dashCount, F32 spaceAngle, F32 offset)
{
   // Draw the dashed arcs
   drawDashedArc(center, innerRadius, dashCount, spaceAngle, offset);
   drawDashedArc(center, outerRadius, dashCount, spaceAngle, offset);

   // Now connect them
   drawAngledRayCircle(center, innerRadius,  outerRadius, dashCount, 0, offset);
   drawAngledRayCircle(center, innerRadius,  outerRadius, dashCount, 0 - spaceAngle, offset);
}


void drawRoundedRect(const Point &pos, S32 width, S32 height, S32 rad)
{
   drawRoundedRect(pos, (F32)width, (F32)height, (F32)rad);
}


// Draw rounded rectangle centered on pos
void drawRoundedRect(const Point &pos, F32 width, F32 height, F32 rad)
{
   Point p;

   // First the main body of the rect, start in UL, proceed CW
   glBegin(GL_LINES);
      glVertex2f(pos.x - width/2 + rad, pos.y - height/2);
      glVertex2f(pos.x + width/2 - rad, pos.y - height/2);

      glVertex2f(pos.x + width/2, pos.y - height/2 + rad);
      glVertex2f(pos.x + width/2, pos.y + height/2 - rad);

      glVertex2f(pos.x + width/2 - rad, pos.y + height/2);
      glVertex2f(pos.x - width/2 + rad, pos.y + height/2);

      glVertex2f(pos.x - width/2, pos.y + height/2 - rad);
      glVertex2f(pos.x - width/2, pos.y - height/2 + rad);
   glEnd();

   // Now add some quarter-rounds in the corners, start in UL, proceed CW
   p.set(pos.x - width/2 + rad, pos.y - height/2 + rad);
   drawArc(p, rad, -FloatPi, -FloatHalfPi);

   p.set(pos.x + width/2 - rad, pos.y - height/2 + rad);
   drawArc(p, rad, -FloatHalfPi, 0);

   p.set(pos.x + width/2 - rad, pos.y + height/2 - rad);
   drawArc(p, rad, 0, FloatHalfPi);

   p.set(pos.x - width/2 + rad, pos.y + height/2 - rad);
   drawArc(p, rad, FloatHalfPi, FloatPi);
}


void drawFilledArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle)
{
   glBegin(GL_POLYGON);

      for(F32 theta = startAngle; theta < endAngle; theta += 0.2f)
         glVertex2f(pos.x + cos(theta) * radius, pos.y + sin(theta) * radius);

      // Make sure arc makes it all the way to endAngle...  rounding errors look terrible!
      glVertex2f(pos.x + cos(endAngle) * radius, pos.y + sin(endAngle) * radius);

      glVertex2f(pos.x, pos.y);

   glEnd();
}


void drawFilledRoundedRect(const Point &pos, S32 width, S32 height, const Color &fillColor, const Color &outlineColor, S32 radius)
{
   drawFilledRoundedRect(pos, (F32)width, (F32)height, fillColor, outlineColor, (F32)radius);
}


void drawFilledRoundedRect(const Point &pos, F32 width, F32 height, const Color &fillColor, const Color &outlineColor, F32 radius)
{
   glColor(fillColor);

   drawFilledArc(Point(pos.x - width / 2 + radius, pos.y - height / 2 + radius), radius,      FloatPi, FloatPi + FloatHalfPi);
   drawFilledArc(Point(pos.x + width / 2 - radius, pos.y - height / 2 + radius), radius, -FloatHalfPi, 0);
   drawFilledArc(Point(pos.x + width / 2 - radius, pos.y + height / 2 - radius), radius,            0, FloatHalfPi);
   drawFilledArc(Point(pos.x - width / 2 + radius, pos.y + height / 2 - radius), radius,  FloatHalfPi, FloatPi);

   UserInterface::drawRect(pos.x - width / 2, pos.y - height / 2 + radius, 
                           pos.x + width / 2, pos.y + height / 2 - radius, GL_POLYGON);

   UserInterface::drawRect(pos.x - width / 2 + radius, pos.y - height / 2, 
                           pos.x + width / 2 - radius, pos.y + height / 2, GL_POLYGON);


   glColor(outlineColor);
   drawRoundedRect(pos, width, height, radius);
}


// Actually draw the ellipse
void drawFilledEllipseUtil(const Point &pos, F32 width, F32 height, F32 angle, U32 glStyle)
{
   F32 sinbeta = sin(angle);
   F32 cosbeta = cos(angle);

   glBegin(glStyle);
      for(F32 theta = 0; theta < FloatTau; theta += 0.2f)
      {
         F32 sinalpha = sin(theta);
         F32 cosalpha = cos(theta);

         glVertex2f(
            pos.x + (width * cosalpha * cosbeta - height * sinalpha * sinbeta),
            pos.y + (width * cosalpha * sinbeta + height * sinalpha * cosbeta)
         );
      }

   glEnd();
}


// Draw an n-sided polygon
void drawPolygon(const Point &pos, S32 sides, F32 radius, F32 angle)
{
   glBegin(GL_LINE_LOOP);
      for(F32 theta = 0; theta < FloatTau; theta += FloatTau / sides)
         glVertex2f(pos.x + cos(theta + angle) * radius, pos.y + sin(theta + angle) * radius);
   glEnd();
}


// Draw an ellipse at pos, with axes width and height, canted at angle
void drawEllipse(const Point &pos, F32 width, F32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, width, height, angle, GL_LINE_LOOP);
}


// Draw an ellipse at pos, with axes width and height, canted at angle
void drawEllipse(const Point &pos, S32 width, S32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, (F32)width, (F32)height, angle, GL_LINE_LOOP);
}


// Well...  draws a filled ellipse, much as you'd expect
void drawFilledEllipse(const Point &pos, F32 width, F32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, width, height, angle, GL_POLYGON);
}


void drawFilledEllipse(const Point &pos, S32 width, S32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, (F32)width, (F32)height, angle, GL_POLYGON);
}


void drawFilledCircle(const Point &pos, F32 radius)
{
   drawFilledSector(pos, radius, 0, FloatTau);
}


void drawFilledSector(const Point &pos, F32 radius, F32 start, F32 end)
{
   glBegin(GL_POLYGON);
      for(F32 theta = start; theta < end; theta += 0.2f)
         glVertex2f(pos.x + cos(theta) * radius, pos.y + sin(theta) * radius);
   glEnd();
}


void drawCentroidMark(const Point &pos, F32 radius)
{  
   drawPolygon(pos, 6, radius, 0);
}


// Health ranges from 0 to 1
// Center is the centerpoint of the health bar; normal is a normalized vector along the main axis of the bar
// length and width are in pixels, and are the dimensions of the health bar
void renderHealthBar(F32 health, const Point &center, const Point &dir, F32 length, F32 width)
{
   const F32 HATCH_COUNT = 14;                     // Number of lines to draw a full health
   U32 hatchCount = U32(HATCH_COUNT * health);     // Number of lines to draw at current health
   Point cross(dir.y, -dir.x);                     // Direction across the health bar, perpendicular to the main axis

   Point dirx = dir;                               // Needs modifiable copy for the Point math to work
   Point base = center - dirx * length * .5;       // Point at center of end of health bar

   Point segMid;                                   // Reusable container

   glBegin(GL_LINES);

   for(F32 i = 0; i < hatchCount; i++)
   {
      dirx = dir;                                  // Reset to original value
      segMid = base + dirx * (i + 0.5f) / F32(HATCH_COUNT) * length;      // Adding 0.5 causes hatches to be centered properly

      glVertex(segMid - cross * F32(width) * 0.5);  
      glVertex(segMid + cross * F32(width) * 0.5);   
   }

   glEnd();
}


void renderShip(const Color *shipColor, F32 alpha, F32 thrusts[], F32 health, F32 radius, U32 sensorTime,
                bool cloakActive, bool shieldActive, bool sensorActive, bool repairActive, bool hasArmor)
{
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   // First render the thrusters
   if(thrusts[0] > 0) // forward thrust
   {
      glColor(Colors::red, alpha);
      glBegin(GL_LINES);
         glVertex2f(-8, -15);
         glVertex2f(0, -15 - 20 * thrusts[0]);
         glVertex2f(0, -15 - 20 * thrusts[0]);
         glVertex2f(8, -15);
      glEnd();

      glColor(Colors::orange50, alpha);
      glBegin(GL_LINES);
         glVertex2f(-6, -15);
         glVertex2f(0, -15 - 15 * thrusts[0]);
         glVertex2f(0, -15 - 15 * thrusts[0]);
         glVertex2f(6, -15);
      glEnd();

      glColor(Colors::yellow, alpha);
      glBegin(GL_LINES);
         glVertex2f(-4, -15);
         glVertex2f(0, -15 - 8 * thrusts[0]);
         glVertex2f(0, -15 - 8 * thrusts[0]);
         glVertex2f(4, -15);
      glEnd();
   }
   if(thrusts[1] > 0) // back thrust
   {
      // two jets:
      // left and right side:
      // from 7.5, 10 -> 12.5, 10 and from -7.5, 10 to -12.5, 10
      glColor(Colors::orange50, alpha);
      glBegin(GL_LINES);
         glVertex2f(7.5, 10);
         glVertex2f(10, 10 + thrusts[1] * 15);
         glVertex2f(12.5, 10);
         glVertex2f(10, 10 + thrusts[1] * 15);
         glVertex2f(-7.5, 10);
         glVertex2f(-10, 10 + thrusts[1] * 15);
         glVertex2f(-12.5, 10);
         glVertex2f(-10, 10 + thrusts[1] * 15);
      glEnd();

      glColor(Colors::yellow, alpha);
      glBegin(GL_LINES);
         glVertex2f(9, 10);
         glVertex2f(10, 10 + thrusts[1] * 10);
         glVertex2f(11, 10);
         glVertex2f(10, 10 + thrusts[1] * 10);
         glVertex2f(-9, 10);
         glVertex2f(-10, 10 + thrusts[1] * 10);
         glVertex2f(-11, 10);
         glVertex2f(-10, 10 + thrusts[1] * 10);
      glEnd();

   }
   float xThrust = -12.5;
   if(thrusts[3] > 0)
   {
      xThrust = -xThrust;
      thrusts[2] = thrusts[3];
   }
   if(thrusts[2] > 0)
   {
      glColor(Colors::red, alpha);
      glBegin(GL_LINE_STRIP);
         glVertex2f(xThrust, 10);
         glVertex2f(xThrust + thrusts[2] * xThrust * 1.5f, 5);
         glVertex2f(xThrust, 0);
      glEnd();

      glColor(Colors::orange50, alpha);
      glBegin(GL_LINE_STRIP);
         glVertex2f(xThrust, 8);
         glVertex2f(xThrust + thrusts[2] * xThrust, 5);
         glVertex2f(xThrust, 2);
      glEnd();

      glColor(Colors::yellow, alpha);
      glBegin(GL_LINE_STRIP);
         glVertex2f(xThrust, 6);
         glVertex2f(xThrust + thrusts[2] * xThrust * 0.5f, 5);
         glVertex2f(xThrust, 4);
      glEnd();
   }

   // Then render the ship:
   // flameports...
   glColor(Colors::gray50, alpha);
   static F32 flamePortPoints[] = { -12.5, 0,   -12.5,10,    -12.5, 10,   -7.5, 10,    7.5, 10,   12.5, 10,   12.5, 10,   12.5, 0 };
   renderVertexArray(flamePortPoints, ARRAYSIZE(flamePortPoints) / 2, GL_LINES);

   // Colored insides
   glColor(shipColor, alpha);
   static F32 shipInnardPoints[] = { -12, -13,   0, 22,   12, -13 };
   renderVertexArray(shipInnardPoints, ARRAYSIZE(shipInnardPoints) / 2, GL_LINE_LOOP);

   renderHealthBar(health, Point(0,1.5), Point(0,1), 28, 4);

   // Grey outer hull
   glColor(Colors::gray70, alpha);
   static F32 shipHullPoints[] = { -20, -15,   0, 25,   20, -15 };
   renderVertexArray(shipHullPoints, ARRAYSIZE(shipHullPoints) / 2, GL_LINE_LOOP);

   // Armor
   if(hasArmor)
   {
      glLineWidth(gLineWidth3);
      glColor(Colors::yellow ,alpha);   

      drawPolygon(Point(0,0), 5, 30, FloatHalfPi);

      glLineWidth(gDefaultLineWidth);
   }

   // Shields
   if(shieldActive)
   {
      F32 shieldRadius = radius + 3;

      glColor(Colors::yellow, alpha);
      drawCircle(0, 0, shieldRadius);
   }
      
   // Sensor
   if(sensorActive)
   {
      glColor(Colors::white, alpha);
      F32 radius = (sensorTime & 0x1FF) * 0.002f;    // Radius changes over time    
      drawCircle(0, 0, radius * Ship::CollisionRadius + 4);
   }

   // Repair
   if(repairActive)
   {
      glLineWidth(gLineWidth3);
      glColor(Colors::red, alpha);
      drawCircle(0, 0, 18);
      glLineWidth(gDefaultLineWidth);
   }
}


// Render repair rays to all the repairing objects
void renderShipRepairRays(const Point &pos, const Ship *ship, Vector<SafePtr<GameObject> > &repairTargets, F32 alpha)
{
   glLineWidth(gLineWidth3);
   glColor(Colors::red, alpha);

   for(S32 i = 0; i < repairTargets.size(); i++)
   {
      if(repairTargets[i] && repairTargets[i].getPointer() != ship)
      {
         Vector<Point> targetRepairLocations = repairTargets[i]->getRepairLocations(pos);

         glBegin(GL_LINES);
            for(S32 i = 0; i < targetRepairLocations.size(); i++)
            {
               glVertex(pos);
               glVertex(targetRepairLocations[i]);
            }
         glEnd();
      }
   }
   glLineWidth(gDefaultLineWidth);
}


void renderShipCoords(const Point &coords, bool localShip, F32 alpha)
{
   string str = string("@") + itos((S32) coords.x) + "," + itos((S32) coords.y);
   const U32 textSize = 18;

   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");
      
   glLineWidth(gLineWidth1);
   glColor(Colors::white, 0.5f * alpha);

   UserInterface::drawStringc(0, 30 + (localShip ? 0 : textSize + 3) + textSize, textSize, str.c_str() );

   glLineWidth(gDefaultLineWidth);
}


void drawFourArrows(const Point &pos)
{
   const F32 pointList[] = {
        0,  15,   0, -15,
        0,  15,   5,  10,
        0,  15,  -5,  10,
        0, -15,   5, -10,
        0, -15,  -5, -10,
       15,   0, -15,   0,
       15,   0,  10,   5,
       15,   0,  10,  -5,
      -15,   0, -10,   5,
      -15,   0, -10,  -5,
   };

   glPushMatrix();
      glTranslate(pos);
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(2, GL_FLOAT, sizeof(pointList[0]) * 2, pointList);    
      glDrawArrays(GL_LINES, 0, sizeof(pointList) / (sizeof(pointList[0]) * 2));
      glDisableClientState(GL_VERTEX_ARRAY);
   glPopMatrix();

}

// This is a line extending from the ship to give joystick players some idea of where they're aiming
void renderAimVector()
{
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   glBegin(GL_LINES);
      glColor(Colors::green, 0);
      glVertex(0, 50);       // Gradient from here...

      glColor(Colors::green, 0.5);        // Reticle color
      glVertex(0, 150);      // ...to here

      glColor(Colors::green, 0.5);
      glVertex(0, 150);      // Solid from here on out
      glVertex(0, 1000);     // 1000 is pretty aribitrary!
   glEnd();
}

#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : -(x))
#endif

void renderTeleporter(const Point &pos, U32 type, bool in, S32 time, F32 zoomFraction, F32 radiusFraction, F32 radius, F32 alpha, 
                      const Vector<Point> &dests, bool showDestOverride)
{
   enum {
      NumColors = 6,
      NumTypes = 2,
      NumParticles = 100,
   };

   static bool trackerInit = false;

   struct Tracker
   {
      F32 thetaI;
      F32 thetaP;
      F32 dI;
      F32 dP;
      U32 ci;
   };
   static Tracker particles[NumParticles];

   static float colors[NumTypes][NumColors][3] = {
      {
         { 0, 0.25, 0.8f },
         { 0, 0.5, 1 },
         { 0, 0, 1 },
         { 0, 1, 1 },
         { 0, 0.5, 0.5 },
         { 0, 0, 1 },
      },
      {
         { 1, 0, 0.5 },
         { 1, 0, 1 },
         { 0, 0, 1 },
         { 0.5, 0, 1 },
         { 0, 0, 0.5 },
         { 1, 0, 0 },
      }
   };
   if(!trackerInit)
   {
      trackerInit = true;
      for(S32 i = 0; i < NumParticles; i++)
      {
         Tracker &t = particles[i];

         t.thetaI = TNL::Random::readF() * FloatTau;
         t.thetaP = TNL::Random::readF() * 2 + 0.5f;
         t.dP = TNL::Random::readF() * 5 + 2.5f;
         t.dI = TNL::Random::readF() * t.dP;
         t.ci = TNL::Random::readI(0, NumColors - 1);
      }
   }

   glPushMatrix();

   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   if(zoomFraction > 0 || showDestOverride)
   {
      const F32 wid = 6.0;
      const F32 alpha = showDestOverride ? 1.f : zoomFraction;

      // Show teleport destinations on commander's map only
      glColor(Colors::white, .25f * alpha );

      glEnable(GL_POLYGON_SMOOTH);
      setDefaultBlendFunction();
      glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

      for(S32 i = 0; i < dests.size(); i++)
      {
         F32 ang = pos.angleTo(dests[i]);
         F32 sina = sin(ang);
         F32 cosa = cos(ang);
         F32 asina = (sina * cosa < 0) ? ABS(sina) : -ABS(sina);
         F32 acosa = ABS(cosa);

         F32 dist = pos.distanceTo(dests[i]);

         F32 midx = pos.x + .75f * cosa * dist;
         F32 midy = pos.y + .75f * sina * dist;

         glBegin(GL_POLYGON);
            glColor(Colors::white, .25f * alpha);
            glVertex2f(pos.x + asina * wid, pos.y + acosa * wid);
            glVertex2f(midx + asina * wid, midy + acosa * wid);
            glVertex2f(midx - asina * wid, midy - acosa * wid);
            glVertex2f(pos.x - asina * wid, pos.y - acosa * wid);
         glEnd();

         glBegin(GL_POLYGON);
            glVertex2f(midx + asina * wid, midy + acosa * wid);
            glColor(Colors::white, 0);
            glVertex2f(dests[i].x + asina * wid, dests[i].y + acosa * wid);
            glVertex2f(dests[i].x - asina * wid, dests[i].y - acosa * wid);
            glColor(Colors::white, .25f * alpha);
            glVertex2f(midx - asina * wid, midy - acosa * wid);
         glEnd();
      }

      glEnd();
   }

   glDisable(GL_POLYGON_SMOOTH);

   glTranslate(pos);

   F32 arcTime = 0.5f + (1 - radiusFraction) * 0.5f;
   if(!in)
      arcTime = -arcTime;

   Color tpColors[NumColors];
   for(S32 i = 0; i < NumColors; i++)
   {
      Color c(colors[type][i][0], colors[type][i][1], colors[type][i][2]);
      tpColors[i].interp(radiusFraction, c, Colors::white);
   }
   F32 beamWidth = 4;

   for(S32 i = 0; i < NumParticles; i++)
   {
      Tracker &t = particles[i];
      //glColor3f(t.c.r, t.c.g, t.c.b);
      F32 d = (t.dP - fmod(t.dI + F32(time) * 0.001f, t.dP)) / t.dP;
      F32 alphaMod = 1;
      if(d > 0.9)
         alphaMod = (1 - d) * 10;

      F32 theta = fmod(t.thetaI + F32(time) * 0.001f * t.thetaP, FloatTau);
      F32 startRadius = radiusFraction * radius * d;

      Point start(cos(theta), sin(theta));
      Point n(-start.y, start.x);

      theta -= arcTime * t.thetaP * (alphaMod + 0.05f);
      d += arcTime / t.dP;
      if(d < 0)
         d = 0;
      Point end(cos(theta), sin(theta));

      F32 endRadius = radiusFraction * radius * d;

      glBegin(GL_TRIANGLE_STRIP);
         glColor(tpColors[t.ci], alpha * alphaMod);

         F32 arcLength = (end * endRadius - start * startRadius).len();
         U32 vertexCount = (U32)(floor(arcLength / 10)) + 2;

         glVertex(start * (startRadius + beamWidth * 0.3f) + n * 2);
         glVertex(start * (startRadius - beamWidth * 0.3f) + n * 2);

         for(U32 j = 0; j <= vertexCount; j++)
         {
            F32 frac = j / F32(vertexCount);
            F32 width = beamWidth * (1 - frac) * 0.5f;
            Point p = start * (1 - frac) + end * frac;
            p.normalize();
            F32 rad = startRadius * (1 - frac) + endRadius * frac;

            p.normalize();
            glColor(tpColors[t.ci], alpha * alphaMod * (1 - frac));
            glVertex(p * (rad + width));
            glVertex(p * (rad - width));
         }
      glEnd();
   }

   glPopMatrix();
}


void renderSpyBugVisibleRange(const Point &pos, const Color &color, F32 currentScale)
{
   Color col(color);        // Make a copy we can alter
   glColor(col * 0.45f);    // Slightly different color than that used for ships

   F32 range = gSpyBugRange * currentScale;

   UserInterface::drawRect(pos.x - range, pos.y - range, pos.x + range, pos.y + range, GL_POLYGON);
}


void renderTurretFiringRange(const Point &pos, const Color &color, F32 currentScale)
{
   glColor(color, 0.25f);    // Use transparency to highlight areas with more turret coverage

   F32 range = Turret::TurretPerceptionDistance * currentScale;

   UserInterface::drawRect(pos.x - range, pos.y - range, pos.x + range, pos.y + range, GL_POLYGON);
}


// Renders turret!  --> note that anchor and normal can't be const &Points because of the point math
void renderTurret(const Color &c, Point anchor, Point normal, bool enabled, F32 health, F32 barrelAngle)
{
   glColor(c);

   Point cross(normal.y, -normal.x);
   Point aimCenter = anchor + normal * Turret::TURRET_OFFSET;

   glBegin(GL_LINE_STRIP);
      for(S32 x = -10; x <= 10; x++)
      {
         F32 theta = x * FloatHalfPi * 0.1f;
         Point pos = normal * cos(theta) + cross * sin(theta);
         glVertex(aimCenter + pos * 15);
      }
   glEnd();

   glLineWidth(gLineWidth3);
   glBegin(GL_LINES);
      Point aimDelta(cos(barrelAngle), sin(barrelAngle));
      glVertex(aimCenter + aimDelta * 15);
      glVertex(aimCenter + aimDelta * 30);
   glEnd();
   glLineWidth(gDefaultLineWidth);

   if(enabled)
      glColor(Colors::white);
   else
      glColor(0.6f);

   glBegin(GL_LINE_LOOP);
      glVertex(anchor + cross * 18);
      glVertex(anchor + cross * 18 + normal * Turret::TURRET_OFFSET);
      glVertex(anchor - cross * 18 + normal * Turret::TURRET_OFFSET);
      glVertex(anchor - cross * 18);
   glEnd();

   // Render health bar
   glColor(c);

   renderHealthBar(health, anchor + normal * 7.5, cross, 28, 5);

   glBegin(GL_LINES);

      Point lsegStart = anchor - cross * 14 + normal * 3;
      Point lsegEnd = anchor + cross * 14 + normal * 3;
      Point n = normal * (Turret::TURRET_OFFSET - 6);

      glVertex(lsegStart);
      glVertex(lsegEnd);
      glVertex(lsegStart + n);
      glVertex(lsegEnd + n);
   glEnd();
}


static void drawFlag(const Color *flagColor, const Color *mastColor, F32 alpha)
{
   glColor(flagColor, alpha);

   // First, the flag itself
   static F32 flagPoints[] = { -15,-15, 15,-5,  15,-5, -15,5,  -15,-10, 10,-5,  10,-5, -15,0 };
   renderVertexArray(flagPoints, ARRAYSIZE(flagPoints) / 2, GL_LINES);


   // Now the flag's mast
   glColor(mastColor != NULL ? *mastColor : Colors::white, alpha);

   drawVertLine(-15, -15, 15);
}


void renderFlag(F32 x, F32 y, const Color *flagColor, const Color *mastColor, F32 alpha)
{
   glPushMatrix();
   glTranslatef(x, y, 0);

   drawFlag(flagColor, mastColor, alpha);

   glPopMatrix();
}


void renderFlag(const Point &pos, const Color *flagColor, const Color *mastColor, F32 alpha)
{
   renderFlag(pos.x, pos.y, flagColor, mastColor, alpha);
}


void renderFlag(const Point &pos, const Color *flagColor)
{
   renderFlag(pos.x, pos.y, flagColor, NULL, 1);
}


void renderFlag(F32 x, F32 y, const Color *flagColor)
{
   renderFlag(x, y, flagColor, NULL, 1);
}


// Not used
//void renderFlag(Point pos, Color c, F32 timerFraction)
//{
//   glPushMatrix();
//   glTranslatef(pos.x, pos.y, 0);
//
//   drawFlag(c, NULL);
//
//   drawCircle(Point(1,1), 5);
//
//   drawFilledSector(Point(1,1), 5, 0, timerFraction * Float2Pi);
//
//   glPopMatrix();
//}

void renderSmallFlag(const Point &pos, const Color &c, F32 parentAlpha)
{
   F32 alpha = 0.75f * parentAlpha;

   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");
   
   glPushMatrix();
      glTranslate(pos);
      glScale(0.2f);

      glColor(c, alpha);
      glBegin(GL_LINES);
         glVertex2f(-15, -15);
         glVertex2f(15, -5);

         glVertex2f(15, -5);
         glVertex2f(-15, 5);

         //glVertex2f(-15, -10);
         //glVertex2f(10, -5);

         //glVertex2f(10, -5);
         //glVertex2f(-15, 0);
         glColor(Colors::white, alpha);
         glVertex2f(-15, -15);
         glVertex2f(-15, 15);
      glEnd();
   glPopMatrix();
}


F32 renderCenteredString(const Point &pos, S32 size, const char *string)
{
   F32 width = UserInterface::getStringWidth((F32)size, string);
   UserInterface::drawString((S32)floor(pos.x - width * 0.5), (S32)floor(pos.y - size * 0.5), size, string);

   return width;
}


F32 renderCenteredString(const Point &pos, F32 size, const char *string)
{
   return renderCenteredString(pos, S32(size + 0.5f), string);
}


void renderPolygonLabel(const Point &centroid, F32 angle, F32 size, const char *text, F32 scaleFact)
{
   glPushMatrix();
      glScale(scaleFact);
      glTranslate(centroid);
      glRotatef(angle * 360 / FloatTau, 0, 0, 1);
      renderCenteredString(Point(0,0), size,  text);
   glPopMatrix();
}


// Renders fill in the form of a series of points representing triangles
void renderTriangulatedPolygonFill(const Vector<Point> *fill)
{
   renderPointVector(fill, GL_TRIANGLES);
}


void renderPolygonOutline(const Vector<Point> *outline)
{
   renderPointVector(outline, GL_LINE_LOOP);
}


void renderPolygonOutline(const Vector<Point> *outlinePoints, const Color *outlineColor, F32 alpha)
{
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   glColor(outlineColor, alpha);
   renderPolygonOutline(outlinePoints);
}


void renderPolygonFill(const Vector<Point> *triangulatedFillPoints, const Color *fillColor, F32 alpha)      
{
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   glColor(fillColor, alpha);
   renderTriangulatedPolygonFill(triangulatedFillPoints);
}


void renderPolygon(const Vector<Point> *fillPoints, const Vector<Point> *outlinePoints, 
                   const Color *fillColor, const Color *outlineColor, F32 alpha = 1)
{
   renderPolygonFill(fillPoints, fillColor, alpha);
   renderPolygonOutline(outlinePoints, outlineColor, alpha);
}


void drawStar(const Point &pos, S32 points, F32 radius, F32 innerRadius)
{
   F32 ang = FloatTau / F32(points * 2);
   F32 a = -ang / 2;
   F32 r = radius;
   bool inout = true;

   Point p;

   Vector<Point> pts;
   for(S32 i = 0; i < points * 2; i++)
   {
      p.set(r * cos(a), r * sin(a));
      pts.push_back(p + pos);

      a += ang;
      inout = !inout;
      r = inout ? radius : innerRadius;
   }

   renderPolygonOutline(&pts);
}


void renderLoadoutZone(const Color *outlineColor, const Vector<Point> *outline, const Vector<Point> *fill)
{
   Color fillColor = *outlineColor;
   fillColor *= 0.5;

   renderPolygon(fill, outline, &fillColor, outlineColor);
}


void renderLoadoutZone(const Color *color, const Vector<Point> *outline, const Vector<Point> *fill, 
                       const Point &centroid, F32 angle, F32 scaleFact)
{
   renderLoadoutZone(color, outline, fill);
   renderPolygonLabel(centroid, angle, 25, "LOADOUT ZONE", scaleFact);
}


void renderNavMeshZone(const Vector<Point> *outline, const Vector<Point> *fill, const Point &centroid, 
                       S32 zoneId, bool isConvex, bool isSelected)
{
   Color outlineColor = isConvex ? Colors::green : Colors::red;
   Color fillColor = outlineColor * .5;

   renderPolygon(fill, outline, &fillColor, &outlineColor, isSelected ? .65f : .4f);

   if(zoneId > 0)
   {
      char buf[24];
      dSprintf(buf, 24, "%d", zoneId );

      renderPolygonLabel(centroid, 0, 25, buf);
   }
   else if(zoneId == -2)
      drawCentroidMark(centroid, .05f);
}


void renderNavMeshBorder(const Border &border, F32 scaleFact, const Color &color, F32 fillAlpha, F32 width)
{
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   for(S32 j = 1; j >= 0; j--)
   {
      glColor(color, j ? fillAlpha : 1); 
      renderTwoPointPolygon(border.borderStart, border.borderEnd, width * scaleFact, j ? GL_POLYGON : GL_LINE_LOOP);
   }
}


void renderTwoPointPolygon(const Point &p1, const Point &p2, F32 width, S32 mode)
{
   F32 ang = p1.angleTo(p2);
   F32 cosa = cos(ang) * width;
   F32 sina = sin(ang) * width;

   glBegin(mode);
      glVertex2f(p1.x + sina, p1.y - cosa);
      glVertex2f(p2.x + sina, p2.y - cosa);
      glVertex2f(p2.x - sina, p2.y + cosa);
      glVertex2f(p1.x - sina, p1.y + cosa);
   glEnd();
}

const Color BORDER_FILL_COLOR(0,1,1);
const F32 BORDER_FILL_ALPHA = .25;
const F32 BORDER_WIDTH = 3;

// Only used in editor
void renderNavMeshBorders(const Vector<ZoneBorder> &borders, F32 scaleFact)
{
   for(S32 i = 0; i < borders.size(); i++)
      renderNavMeshBorder(borders[i], scaleFact, BORDER_FILL_COLOR, BORDER_FILL_ALPHA, BORDER_WIDTH);
}


// Only used in-game
void renderNavMeshBorders(const Vector<NeighboringZone> &borders, F32 scaleFact)
{
   for(S32 i = 0; i < borders.size(); i++)
      renderNavMeshBorder(borders[i], scaleFact, BORDER_FILL_COLOR, BORDER_FILL_ALPHA, BORDER_WIDTH);
}


static Color getGoalZoneOutlineColor(const Color &c, F32 glowFraction)
{
   return Color(Colors::yellow) * (glowFraction * glowFraction) + Color(c) * (1 - glowFraction * glowFraction);
}


// TODO: Consider replacing yellow with team color to indicate who scored!
static Color getGoalZoneFillColor(const Color &c, bool isFlashing, F32 glowFraction)
{
   F32 alpha = isFlashing ? 0.75f : 0.5f;

   return Color(Colors::yellow) * (glowFraction * glowFraction) + Color(c) * alpha * (1 - glowFraction * glowFraction);
}


// No label version
void renderGoalZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill)
{
   Color fillColor    = getGoalZoneFillColor(c, false, 0);
   Color outlineColor = getGoalZoneOutlineColor(c, false);

   renderPolygon(fill, outline, &fillColor, &outlineColor);
}


// Goal zone flashes after capture, but glows after touchdown...
void renderGoalZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill, Point centroid, F32 labelAngle,
                    bool isFlashing, F32 glowFraction, S32 score, F32 flashCounter, bool useOldStyle)
{
   Color fillColor, outlineColor;

   if(useOldStyle)
   {
//      fillColor    = getGoalZoneFillColor(c, isFlashing, glowFraction);
//      outlineColor = getGoalZoneOutlineColor(c, isFlashing);

      // TODO: reconcile why using the above commentted out code doesn't work
      F32 alpha = isFlashing ? 0.75f : 0.5f;
      fillColor    = Color(Color(1,1,0) * (glowFraction * glowFraction) + Color(c) * alpha * (1 - glowFraction * glowFraction));
      outlineColor = Color(Color(1,1,0) * (glowFraction * glowFraction) + Color(c) *         (1 - glowFraction * glowFraction));
   }
   else // Some new flashing effect (sam's idea)
   {
      F32 glowRate = 0.5f - fabs(flashCounter - 0.5f);  // will need flashCounter for this.

      Color newColor = c;
      if(isFlashing)
         newColor = newColor + glowRate * (1 - glowRate);
      else
         newColor = newColor * (1 - glowRate);

      fillColor    = getGoalZoneFillColor(newColor, false, glowFraction);
      outlineColor = getGoalZoneOutlineColor(newColor, false);
   }


   renderPolygon(fill, outline, &fillColor, &outlineColor);
   renderPolygonLabel(centroid, labelAngle, 25, "GOAL");
}


extern Color gNexusOpenColor;
extern Color gNexusClosedColor;

static Color getNexusBaseColor(bool open, F32 glowFraction)
{
   Color color;

   if(open)
      color.interp(glowFraction, Colors::yellow, gNexusOpenColor);
   else
      color = gNexusClosedColor;

   return color;
}


void renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, bool open, F32 glowFraction)
{
   Color baseColor = getNexusBaseColor(open, glowFraction);

   Color fillColor = Color(baseColor * (glowFraction * glowFraction  + (1 - glowFraction * glowFraction) * 0.5f));
   Color outlineColor = fillColor * 0.7f;

   renderPolygon(fill, outline, &fillColor, &outlineColor);
}


void renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, Point centroid, F32 labelAngle, bool open, 
                 F32 glowFraction, F32 scaleFact)
{
   renderNexus(outline, fill, open, glowFraction);

   glColor(getNexusBaseColor(open, glowFraction));
   renderPolygonLabel(centroid, labelAngle, 25, "NEXUS", scaleFact);
}


void renderSlipZone(const Vector<Point> *bounds, const Vector<Point> *boundsFill, const Point &centroid)     
{
   Color theColor (0, 0.5, 0);  // Go for a pale green, for now...

   glColor(theColor * 0.5);
   renderPointVector(boundsFill, GL_TRIANGLES);

   glColor(theColor * 0.7f);
   renderPointVector(bounds, GL_LINE_LOOP);

   glPushMatrix();
      glTranslate(centroid);

      //if(extents.x < extents.y)
      //   glRotatef(90, 0, 0, 1);

      glColor(Colors::cyan);
      renderCenteredString(Point(0,0), 25, "CAUTION - SLIPPERY!");
   glPopMatrix();
}


void renderProjectile(const Point &pos, U32 type, U32 time)
{
   ProjectileInfo *pi = GameWeapon::projectileInfo + type;

   S32 bultype = 1;

   if(bultype == 1)    // Default stars
   { 
      glColor(pi->projColors[0]);
      glPushMatrix();
         glTranslate(pos);
         glScale(pi->scaleFactor);

         glPushMatrix();
            glRotatef((time % 720) * 0.5f, 0, 0, 1);

            static S16 projectilePoints1[] = { -2,2,  0,6,  2,2,  6,0,  2,-2,  0,-6,  -2,-2,  -6,0 };
            renderVertexArray(projectilePoints1, ARRAYSIZE(projectilePoints1) / 2, GL_LINE_LOOP);

         glPopMatrix();

         glRotatef(180 - F32(time % 360), 0, 0, 1);
         glColor(pi->projColors[1]);

         static S16 projectilePoints2[] = { -2,2,  0,8,  2,2,  8,0,  2,-2,  0,-8, -2,-2,  -8,0 };
         renderVertexArray(projectilePoints2, ARRAYSIZE(projectilePoints2) / 2, GL_LINE_LOOP);

      glPopMatrix();

   } else if (bultype == 2) { // Tiny squares rotating quickly, good machine gun

      glColor(pi->projColors[0]);
      glPushMatrix();
      glTranslate(pos);
      glScale(pi->scaleFactor);

      glPushMatrix();

      glRotatef(F32(time % 720), 0, 0, 1);
      glColor(pi->projColors[1]);

      static S16 projectilePoints3[] = { -2,2,  2,2,  2,-2,  -2,-2 };
      renderVertexArray(projectilePoints3, ARRAYSIZE(projectilePoints3) / 2, GL_LINE_LOOP);

      glPopMatrix();

   } else if (bultype == 3) {  // Rosette of circles  MAKES SCREEN GO BEZERK!!

      const int innerR = 6;
      const int outerR = 3;
      const int dist = 10;

#define dr(x) (float) x * FloatTau / 360     // degreesToRadians()

      glRotatef( fmod(F32(time) * .15f, 720.f), 0, 0, 1);
      glColor(pi->projColors[1]);

      Point p(0,0);
      drawCircle(p, (F32)innerR);
      p.set(0,-dist);

      drawCircle(p, (F32)outerR);
      p.set(0,-dist);
      drawCircle(p, (F32)outerR);
      p.set(cos(dr(30)), -sin(dr(30)));
      drawCircle(p*dist, (F32)outerR);
      p.set(cos(dr(30)), sin(dr(30)));
      drawCircle(p * dist, (F32)outerR);
      p.set(0, dist);
      drawCircle(p, (F32)outerR);
      p.set(-cos(dr(30)), sin(dr(30)));
      drawCircle(p*dist, (F32)outerR);
      p.set(-cos(dr(30)), -sin(dr(30)));
      drawCircle(p*dist, (F32)outerR);
   }
}


void renderMine(const Point &pos, bool armed, bool visible)
{
   F32 mod = 0.8f;
   F32 vis = .25;
   if(visible)
   {
      glColor(Colors::gray50);
      drawCircle(pos, Mine::SensorRadius);
      mod = 0.8f;
      vis = 1.0;
   }
   else
      glLineWidth(gLineWidth1);

   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   glColor4f(mod, mod, mod, vis);
   drawCircle(pos, 10);

   if(armed)
   {
      glColor4f(mod, 0, 0, vis);
      drawCircle(pos, 6);
   }
   glLineWidth(gDefaultLineWidth);
}

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


// lifeLeft is a number between 0 and 1.  Burst explodes when lifeLeft == 0.

// burstGraphicsMode is between 1 and 5
// 1 = flash & color change
// 2 = flash only
// 3 = color change only
// 4 = normal rendering, filled circle
// 5 = normal rendering
void renderGrenade(const Point &pos, F32 lifeLeft, S32 style)
{
   glColor(Colors::white);
   drawCircle(pos, 10);

   bool innerVis = true;

   if(lifeLeft > .85)
      innerVis = false;
   else if(lifeLeft > .7)
      innerVis = true;
   else if(lifeLeft > .55)
      innerVis = false;
   else if(lifeLeft > .4)
      innerVis = true;
   else if(lifeLeft > .3)
      innerVis = false;
   else if(lifeLeft > .2)
      innerVis = true;
   else if(lifeLeft > .15)
      innerVis = false;
   else if(lifeLeft > .1)
      innerVis = true;
   else if(lifeLeft > .05)
      innerVis = false;

   if(style == 1 || style == 3)
      glColor3f(1, min(1.25f - lifeLeft, 1), 0);
   else
      glColor(Colors::red);

   // if(innerVis)
   if((innerVis && (style == 1 || style == 2)) || style == 3 || style == 4 )
      drawFilledCircle(pos, 6);
   else
      drawCircle(pos, 6);

/*
   const S32 r1 = 6;
   const S32 r2 = 7;
   const S32 r3 = 8;
   const S32 r4 = 12;
   const S32 wedges = 12;
   const F32 wedgeAngle = 360 / 12 * Float2Pi / 360;     // 22.5 = 360 / wedges


   F32 alpha = 1 - (vel / 500) * (vel / 500);      // TODO: Make 500 come from constant in gameWeapons.cpp

   // Color inner circle with a color that gets darker as grenade slows

   glColor4f(1,.5,0,alpha);
   drawFilledCircle(pos, r3);
   glDisable(GL_BLEND);

   bool inOut = true;

      glColor3f(1,1,0);
      glLineWidth(gLineWidth1);
      F32 off = 0;
      for(S32 j = 0; j < 2; j++)
      {
         if(j)
         {
            off =  vel/50;
            glColor3f(1,.5,0);

         }
      // Draw each of the 16 wedge pieces
      for(S32 i = 0; i < wedges; i++)
      {
         F32 a1 = i * wedgeAngle - vel / 100 + off;
         F32 a2 = a1 + wedgeAngle;
         S32 ra, rb;

         if(inOut)
         {
            ra = r4 + 2*(1-j);
            rb = r3;
         }
         else
         {
            ra = r3;
            rb = r4 + 2*(1-j);
         }

         glBegin(GL_LINES);
            glVertex2f(ra * cos(a1) + pos.x, ra * sin(a1) + pos.y);
            glVertex2f(rb * cos(a1) + pos.x, rb * sin(a1) + pos.y);

            glVertex2f(rb * cos(a1) + pos.x, rb * sin(a1) + pos.y);
            glVertex2f(rb * cos(a2) + pos.x, rb * sin(a2) + pos.y);
         glEnd();

         inOut = !inOut;
      }
      }
      glLineWidth(gDefaultLineWidth);
      */
}


void renderSpyBug(const Point &pos, const Color &teamColor, bool visible, bool drawOutline)
{
   F32 mod = 0.25;

   if(visible)
   {
      glColor(teamColor);
      drawFilledCircle(pos, 15);
      if(drawOutline)
      {
         glColor(Colors::gray50);
         drawCircle(pos, 15);
      }

      mod = 1.0;
      UserInterface::drawString(pos.x - 3, pos.y - 5, 10, "S");
   }
   else
   {
      glLineWidth(gLineWidth1);
      glColor(mod);
      drawCircle(pos, 5);
   }

   glLineWidth(gDefaultLineWidth);
}


void renderRepairItem(const Point &pos)
{
   renderRepairItem(pos, false, 0, 1);
}


void renderRepairItem(const Point &pos, bool forEditor, const Color *overrideColor, F32 alpha)
{
   F32 crossWidth;
   F32 crossLen;
   F32 size;

   if(forEditor)  // Rendering icon for editor
   {
      crossWidth = 2;
      crossLen = 6;
      size = 8;
   }
   else           // Normal in-game rendering
   {
      crossWidth = 4;
      crossLen = 14;
      size = 18;
   }

   glPushMatrix();
   glTranslate(pos);

   glColor(overrideColor == NULL ? Colors::white : *overrideColor, alpha);
   drawSquare(Point(0,0), size, false);

   glColor(overrideColor == NULL ? Colors::red : *overrideColor, alpha);
   glBegin(GL_LINE_LOOP);
      glVertex2f(crossWidth, crossWidth);
      glVertex2f(crossLen, crossWidth);
      glVertex2f(crossLen, -crossWidth);
      glVertex2f(crossWidth, -crossWidth);
      glVertex2f(crossWidth, -crossLen);
      glVertex2f(-crossWidth, -crossLen);
      glVertex2f(-crossWidth, -crossWidth);
      glVertex2f(-crossLen, -crossWidth);
      glVertex2f(-crossLen, crossWidth);
      glVertex2f(-crossWidth, crossWidth);
      glVertex2f(-crossWidth, crossLen);
      glVertex2f(crossWidth, crossLen);
   glEnd();

   glPopMatrix();
}


void renderEnergyGuage(S32 energy, S32 maxEnergy, S32 cooldownThreshold)
{
   const S32 GAUGE_WIDTH = 200;
   const S32 GUAGE_HEIGHT = 20;

   // For readability
   const S32 hMargin = UserInterface::horizMargin;
   const S32 vMargin = UserInterface::vertMargin;

   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   F32 full = F32(energy) / F32(maxEnergy) * GAUGE_WIDTH;

   // Guage fill
   glBegin(GL_POLYGON);
      glColor(Colors::blue);
      glVertex2i(hMargin, canvasHeight - vMargin - GUAGE_HEIGHT);
      glVertex2i(hMargin, canvasHeight - vMargin);

      glColor(Colors::cyan);
      glVertex2f(hMargin + full, (F32)canvasHeight - vMargin);
      glVertex2f(hMargin + full, (F32)canvasHeight - vMargin - GUAGE_HEIGHT);
   glEnd();

   // Guage outline
   glColor(Colors::white);
   drawVertLine(hMargin, canvasHeight - vMargin - GUAGE_HEIGHT, canvasHeight - vMargin);
   drawVertLine(hMargin + GAUGE_WIDTH, canvasHeight - vMargin - GUAGE_HEIGHT, canvasHeight - vMargin);

   // Show safety line
   S32 cutoffx = cooldownThreshold * GAUGE_WIDTH / maxEnergy;

   glColor(Colors::yellow);
   drawVertLine(hMargin + cutoffx, canvasHeight - vMargin - 23, canvasHeight - vMargin + 4);
}


// Render the actual lightning bolt
void renderEnergySymbol(const Color *overrideColor, F32 alpha)
{
   // Yellow lightning bolt
   glColor(overrideColor == NULL ? Colors::orange67 : *overrideColor, alpha);

   static S16 energySymbolPoints[] = { 20,-20,  3,-2,  12,5,  -20,20,  -2,3,  -12,-5 };
   renderVertexArray(energySymbolPoints, ARRAYSIZE(energySymbolPoints) / 2, GL_LINE_LOOP);
}


void renderEnergySymbol(const Point &pos, F32 scaleFactor)
{
   glPushMatrix();
      glTranslate(pos);
      glScale(scaleFactor);
      renderEnergySymbol(0, 1);
   glPopMatrix();
}


void renderEnergyItem(const Point &pos, bool forEditor, const Color *overrideColor, F32 alpha)
{
   F32 scaleFactor = forEditor ? .45f : 1;    // Resize for editor

   glPushMatrix();
      glTranslate(pos);

      // Scale down the symbol a little so it fits in the box
      glScale(scaleFactor * .7f);
      renderEnergySymbol(overrideColor, alpha);

      // Scale back up to where we were
      glScale(1 / .7f);

      S32 size = 18;
      glColor(Colors::white);
      drawSquare(Point(0,0), size, false);
      glLineWidth(gDefaultLineWidth);

   glPopMatrix();
}


void renderEnergyItem(const Point &pos)
{
   renderEnergyItem(pos, false, NULL, 1);
}


// Use faster method with no offset
void renderWallFill(const Vector<Point> *points, bool polyWall)
{
   renderPointVector(points, polyWall ? GL_TRIANGLES : GL_POLYGON);
}


// Use slower method if each point needs to be offset
void renderWallFill(const Vector<Point> *points, const Point &offset, bool polyWall)
{
   renderPointVector(points, offset, polyWall ? GL_TRIANGLES : GL_POLYGON);
}


// Used in both editor and game
void renderWallEdges(const Vector<Point> *edges, const Color &outlineColor, F32 alpha)
{
   glColor(outlineColor, alpha);
   renderPointVector(edges, GL_LINES);
}


// Used in editor only
void renderWallEdges(const Vector<Point> *edges, const Point &offset, const Color &outlineColor, F32 alpha)
{
   glColor(outlineColor, alpha);
   renderPointVector(edges, offset, GL_LINES);
}


void renderSpeedZone(const Vector<Point> &points, U32 time)
{
   glColor(Colors::red);

   for(S32 j = 0; j < 2; j++)
   {
      S32 start = j * points.size() / 2;    // GoFast comes in two equal shapes

      glEnableClientState(GL_VERTEX_ARRAY);

      glVertexPointer(2, GL_FLOAT, sizeof(Point), points.address());    
      glDrawArrays(GL_LINE_LOOP, start, points.size() / 2);

      glDisableClientState(GL_VERTEX_ARRAY);
   }
}


void renderTestItem(const Point &pos, F32 alpha)
{
   renderTestItem(pos, 60, alpha);
}


void renderTestItem(const Point &pos, S32 size, F32 alpha)
{
   glColor(Colors::yellow, alpha);
   drawPolygon(pos, 7, (F32)size, 0);
}


void renderWorm(const Point &pos)
{
   glPushMatrix();
   glTranslate(pos);

   F32 size = (F32)Worm::WORM_RADIUS * .5f;

   glColor(Color(1, .1, .1));
      glBegin(GL_LINE_LOOP);
         glVertex2f(0,    -size);
         glVertex2f(size,  0    );
         glVertex2f(0,     size );
         glVertex2f(-size, 0   );
      glEnd();
   glPopMatrix();
}


void renderAsteroid(const Point &pos, S32 design, F32 scaleFact, const Color *color, F32 alpha)
{
   glPushMatrix();
   glTranslate(pos);

   glColor(color ? *color : Color(.7), alpha);
      // Design 1
      glBegin(GL_LINE_LOOP);
         for(S32 i = 0; i < AsteroidPoints; i++)
         {
            F32 x = AsteroidCoords[design][i][0] * scaleFact;
            F32 y = AsteroidCoords[design][i][1] * scaleFact;
            glVertex2f(x, y);
         }
      glEnd();
   glPopMatrix();
}


void renderAsteroid(const Point &pos, S32 design, F32 scaleFact)
{
   renderAsteroid(pos, design, scaleFact, NULL);
}


void renderResourceItem(const Point &pos, F32 scaleFactor, const Color *color, F32 alpha)
{
   glPushMatrix();
      glTranslate(pos);
      glScale(scaleFactor);
      glColor(color == NULL ? Colors::white : *color, alpha);

      static F32 resourcePoints[] = { -8,8,  0,20,  8,8,  20,0,  8,-8,  0,-20,  -8,-8,  -20,0 };
      renderVertexArray(resourcePoints, ARRAYSIZE(resourcePoints) / 2, GL_LINE_LOOP);
   glPopMatrix();
}


void renderResourceItem(const Point &pos, F32 alpha)
{
   renderResourceItem(pos, 1, 0, alpha);
}


void renderSoccerBall(const Point &pos, F32 size)
{
   glColor(Colors::white);
   drawCircle(pos, size);
}


void renderCore(const Point &pos, F32 size, const Color *coreColor, U32 time, 
                PanelGeom *panelGeom, F32 panelHealth[], F32 panelStartingHealth)
{
   TNLAssert(glIsEnabled(GL_BLEND), "Expect blending to be on here!");

   if(size != 100)
   {
      glPushMatrix();
      glTranslate(pos);
      glScale(size / 100);
   }


   // Draw outer polygon and inner circle
   Color baseColor = Colors::gray80;

   Point dir;   // Reusable container
   
   for(S32 i = 0; i < CORE_PANELS; i++)
   {
      dir = (panelGeom->repair[i] - pos);
      dir.normalize();
      Point cross(dir.y, -dir.x);

      glColor(coreColor);
      renderHealthBar(panelHealth[i] / panelStartingHealth, panelGeom->repair[i], dir, 30 * size / 100, 7 * size / 100);

      if(panelHealth[i] == 0)          // Panel is dead
      {
         Color c = coreColor;
         glColor(c * .2f);
      }      

      glBegin(GL_LINES);
         glVertex(panelGeom->getStart(i));
         glVertex(panelGeom->getEnd(i));
      glEnd();

      // Draw health stakes
      if(panelHealth[i] > 0)
      {
         glBegin(GL_LINES);
            //if(panelHealth[i] == panelStartingHealth)
            //   glColor(coreColor);
            //else
               glColor(Colors::gray20);

            glVertex(panelGeom->repair[i]);
            glColor(Colors::black);
            glVertex(pos);
         glEnd();
      }
   }

   F32 atomSize = size * 0.40f;
   F32 angle = CoreItem::getCoreAngle(time);

   // Draw atom graphic
   F32 t = FloatTau - (F32(time & 1023) / 1024.f * FloatTau);  // Reverse because time is counting down
   for(F32 rotate = 0; rotate < FloatTau - 0.01f; rotate += FloatTau / 5)  //  0.01f part avoids rounding error
   {
      glBegin(GL_LINE_LOOP);
      for(F32 theta = 0; theta < FloatTau; theta += 0.2f)
      {
         F32 x = cos(theta + rotate * 2 + t) * atomSize * 0.5f;
         F32 y = sin(theta + rotate * 2 + t) * atomSize;
         glColor(coreColor, theta / FloatTau);
         glVertex2f(pos.x + cos(rotate + angle) * x + sin(rotate + angle) * y, pos.y + sin(rotate + angle) * x - cos(rotate + angle) * y);
      }
      glEnd();
   }

   glColor(baseColor);
   drawCircle(pos, atomSize + 2);

   if(size != 100)
      glPopMatrix();
}


// Here we render a simpler, non-animated Core to reduce distraction
void renderCoreSimple(const Point &pos, const Color *coreColor, S32 width)
{
   // Here we render a simpler, non-animated Core to reduce distraction in the editor
   glColor(Colors::white);
   drawPolygon(pos, 10, (F32)width / 2, 0);

   glColor(coreColor);
   drawCircle(pos, (F32)width / 5);
}


void renderSoccerBall(const Point &pos)
{
   renderSoccerBall(pos, (F32)SoccerBallItem::SOCCER_BALL_RADIUS);
}


void renderTextItem(const Point &pos, const Point &dir, F32 size, const string &text, const Color *color)
{
   if(text == "Bitfighter")
   {
      glColor(Colors::green);
      // All factors in here determined experimentally, seem to work at a variety of sizes and approximate the width and height
      // of ordinary text in cases tested.  What should happen is the Bitfighter logo should, as closely as possible, match the 
      // size and extent of the text "Bitfighter".
      F32 scaleFactor = size / 129.0f;
      glPushMatrix();
      glTranslate(pos);
         glScale(scaleFactor);
         glRotatef(pos.angleTo(dir) * radiansToDegreesConversion, 0, 0, 1);
         glTranslatef(-119, -45, 0);      // Determined experimentally

         renderBitfighterLogo(0, 1);
      glPopMatrix();

      return;
   }

   glColor(color);      
   UserInterface::drawAngleString(pos.x, pos.y, size, pos.angleTo(dir), text.c_str());
}


void renderForceFieldProjector(Point pos, Point normal, const Color *color, bool enabled)
{
   Vector<Point> geom;
   ForceFieldProjector::getForceFieldProjectorGeometry(pos, normal, geom);      // fills geom from pos and normal

   renderForceFieldProjector(&geom, color, enabled);
}


void renderForceFieldProjector(const Vector<Point> *geom, const Color *color, bool enabled)
{
   F32 ForceFieldBrightnessProjector = 0.50;

   Color c(color);      // Create locally modifiable copy

   c = c * (1 - ForceFieldBrightnessProjector) + ForceFieldBrightnessProjector;

   glColor(enabled ? c : (c * 0.6f));

   renderPointVector(geom, GL_LINE_LOOP);
}


void renderForceField(Point start, Point end, const Color *color, bool fieldUp, F32 scaleFact)
{
   Vector<Point> geom;
   ForceField::getGeom(start, end, geom, scaleFact);

   F32 ForceFieldBrightness = 0.25;

   Color c(color);
   c = c * (1 - ForceFieldBrightness) + ForceFieldBrightness;

   glColor(fieldUp ? c : c * 0.5);

   renderPointVector(&geom, GL_LINE_LOOP);
}


struct pixLoc
{
   S16 x;
   S16 y;
};

const S32 LetterLoc1 = 25;   // I
const S32 LetterLoc2 = 25;   // I
const S32 LetterLoc3 = 71;   // G
const S32 LetterLoc4 = 40;
const S32 LetterLoc5 = 62;
const S32 LetterLoc6 = 39;
const S32 LetterLoc7 = 54;
const S32 LetterLoc8 = 60;
const S32 LetterLoc9 = 30;
const S32 LetterLoc10 = 21;
const S32 LetterLoc11 = 21;
const S32 LetterLoc12 = 46;
const S32 LetterLoc13 = 22;

pixLoc gLogoPoints[LetterLoc1 + LetterLoc2 + LetterLoc3 + LetterLoc4 + LetterLoc5 + LetterLoc6 +
                   LetterLoc7 + LetterLoc8 + LetterLoc9 + LetterLoc10 + LetterLoc11 + LetterLoc12 + LetterLoc13] =
{
{ 498,265 }, { 498,31  }, { 500,21  }, { 506,11  }, { 516,3   }, { 526,0   }, { 536,3   }, { 546,11  }, { 551,21  },
{ 554,31  }, { 554,554 }, { 551,569 }, { 546,577 }, { 539,582 }, { 457,582 }, { 442,580 }, { 434,575 }, { 429,567 },
{ 429,333 }, { 437,321 }, { 450,313 }, { 473,305 }, { 478,303 }, { 483,300 }, { 495,285 },

{ 1445,265 }, { 1445,31  }, { 1448,21  }, { 1453,11  }, { 1463,3   }, { 1473,0   }, { 1483,3   }, { 1494,11  }, { 1499,21  },
{ 1501,31  }, { 1501,554 }, { 1499,569 }, { 1494,577 }, { 1486,582 }, { 1405,582 }, { 1389,580 }, { 1382,575 }, { 1377,567 },
{ 1377,333 }, { 1384,321 }, { 1397,313 }, { 1420,305 }, { 1425,303 }, { 1430,300 }, { 1443,285 },

{ 1562,13  }, { 1575,3   }, { 1585,0   }, { 1905,0   }, { 1920,8   }, { 1930,21  }, { 1933,39  }, { 1933,148 }, { 1928,155 },
{ 1923,161 }, { 1915,163 }, { 1837,163 }, { 1826,158 }, { 1824,153 }, { 1824,145 }, { 1821,138 }, { 1821,82  }, { 1814,69  },
{ 1801,61  }, { 1783,59  }, { 1717,59  }, { 1702,61  }, { 1689,69  }, { 1682,77  }, { 1679,84  }, { 1676,105 }, { 1676,501 },
{ 1682,514 }, { 1687,519 }, { 1697,524 }, { 1720,529 }, { 1847,529 }, { 1857,526 }, { 1872,519 }, { 1882,503 }, { 1885,483 },
{ 1885,359 }, { 1882,343 }, { 1875,331 }, { 1862,321 }, { 1842,315 }, { 1806,315 }, { 1793,313 }, { 1783,308 }, { 1776,298 },
{ 1773,288 }, { 1776,277 }, { 1783,267 }, { 1793,262 }, { 1806,260 }, { 1908,260 }, { 1923,262 }, { 1936,270 }, { 1941,282 },
{ 1943,303 }, { 1943,554 }, { 1941,562 }, { 1938,567 }, { 1933,575 }, { 1925,580 }, { 1915,582 }, { 1905,587 }, { 1585,587 },
{ 1575,585 }, { 1567,582 }, { 1562,577 }, { 1557,569 }, { 1555,562 }, { 1552,557 }, { 1552,49  }, { 1555,28  },

{ 594,28  }, { 597,16  }, { 605,6   }, { 617,0   }, { 630,-2  }, { 953,-2  }, { 965,0   }, { 975,6   }, { 983,16  },
{ 986,26  }, { 983,39  }, { 978,46  }, { 968,54  }, { 955,56  }, { 879,56  }, { 866,64  }, { 859,77  }, { 856,92  },
{ 856,552 }, { 854,572 }, { 848,580 }, { 843,585 }, { 833,587 }, { 757,587 }, { 749,585 }, { 744,585 }, { 739,580 },
{ 734,572 }, { 734,564 }, { 732,552 }, { 732,97  }, { 729,79  }, { 721,67  }, { 709,59  }, { 688,56  }, { 630,56  },
{ 617,54  }, { 605,49  }, { 597,39  }, { 594,28  },

{ 2012,-2  }, { 2022,0   }, { 2032,6   }, { 2037,16  }, { 2040,26  }, { 2040,227 }, { 2042,237 }, { 2047,247 }, { 2060,257 },
{ 2075,260 }, { 2218,260 }, { 2228,257 }, { 2235,254 }, { 2243,249 }, { 2251,239 }, { 2253,227 }, { 2253,31  }, { 2256,21  },
{ 2261,11  }, { 2271,3   }, { 2284,0   }, { 2294,3   }, { 2304,11  }, { 2311,21  }, { 2314,34  }, { 2314,216 }, { 2317,239 },
{ 2324,254 }, { 2334,260 }, { 2347,262 }, { 2362,270 }, { 2370,277 }, { 2375,288 }, { 2378,303 }, { 2378,567 }, { 2375,572 },
{ 2367,577 }, { 2357,582 }, { 2271,582 }, { 2266,580 }, { 2258,575 }, { 2256,567 }, { 2253,554 }, { 2253,343 }, { 2248,333 },
{ 2235,321 }, { 2220,318 }, { 2131,318 }, { 2113,326 }, { 2103,338 }, { 2098,354 }, { 2098,567 }, { 2093,577 }, { 2083,582 },
{ 1991,582 }, { 1984,577 }, { 1981,569 }, { 1981,559 }, { 1981,26  }, { 1984,16  }, { 1989,6   }, { 1999,0   },

{ 2416,28  }, { 2418,16  }, { 2426,6   }, { 2438,0   }, { 2451,-2  }, { 2774,-2  }, { 2786,0   }, { 2797,6   }, { 2804,16  },
{ 2807,26  }, { 2804,39  }, { 2799,46  }, { 2789,54  }, { 2776,56  }, { 2700,56  }, { 2687,64  }, { 2680,77  }, { 2677,92  },
{ 2677,552 }, { 2675,572 }, { 2670,580 }, { 2665,585 }, { 2654,587 }, { 2578,587 }, { 2571,585 }, { 2565,585 }, { 2560,580 },
{ 2555,572 }, { 2555,564 }, { 2553,552 }, { 2553,97  }, { 2550,79  }, { 2543,67  }, { 2530,59  }, { 2510,56  }, { 2451,56  },
{ 2438,54  }, { 2426,49  }, { 2418,39  },

{ 1034,3   }, { 1049,-2  }, { 1057,-5  }, { 1306,-5  }, { 1318,-2  }, { 1326,3   }, { 1334,13  }, { 1336,26  }, { 1334,36  },
{ 1326,46  }, { 1318,51  }, { 1306,54  }, { 1181,54  }, { 1166,61  }, { 1151,77  }, { 1148,94  }, { 1148,227 }, { 1151,234 },
{ 1156,242 }, { 1171,254 }, { 1194,257 }, { 1290,257 }, { 1306,260 }, { 1318,265 }, { 1329,272 }, { 1331,285 }, { 1329,300 },
{ 1321,308 }, { 1306,313 }, { 1290,315 }, { 1191,315 }, { 1174,318 }, { 1158,328 }, { 1148,341 }, { 1146,359 }, { 1146,557 },
{ 1143,567 }, { 1138,577 }, { 1128,582 }, { 1118,585 }, { 1105,582 }, { 1097,577 }, { 1090,567 }, { 1087,557 }, { 1087,300 },
{ 1082,285 }, { 1072,275 }, { 1054,267 }, { 1036,260 }, { 1029,252 }, { 1024,237 }, { 1021,232 }, { 1021,31  }, { 1026,13  },

{ 2858,6   }, { 2873,-2  }, { 2880,-5  }, { 3122,-5  }, { 3134,-2  }, { 3147,3   }, { 3155,13  }, { 3157,26  }, { 3155,39  },
{ 3147,46  }, { 3137,51  }, { 3122,54  }, { 2959,54  }, { 2934,59  }, { 2924,64  }, { 2911,79  }, { 2906,94  }, { 2906,206 },
{ 2908,227 }, { 2916,244 }, { 2924,249 }, { 2934,254 }, { 2946,257 }, { 3117,257 }, { 3132,260 }, { 3147,265 }, { 3155,272 },
{ 3157,288 }, { 3155,300 }, { 3145,310 }, { 3132,315 }, { 3015,315 }, { 3000,318 }, { 2985,326 }, { 2974,341 }, { 2972,359 },
{ 2972,483 }, { 2974,501 }, { 2980,514 }, { 2987,519 }, { 2997,524 }, { 3018,526 }, { 3122,526 }, { 3137,529 }, { 3147,534 },
{ 3155,542 }, { 3157,554 }, { 3155,569 }, { 3147,577 }, { 3134,580 }, { 3122,582 }, { 2891,582 }, { 2878,580 }, { 2868,580 },
{ 2860,572 }, { 2853,567 }, { 2850,559 }, { 2847,552 }, { 2847,36  }, { 2850,18  },

{ 343,244  }, { 351,249  }, { 366,254  }, { 379,260  }, { 384,262  }, { 391,275  }, { 394,290  }, { 394,549  }, { 391,557  },
{ 389,564  }, { 381,572  }, { 368,580  }, { 351,582  }, { 43,582   }, { 25,580   }, { 18,577   }, { 10,572   }, { 3,559    },
{ 0,542    }, { 0,31     }, { 3,13     }, { 10,3     }, { 23,-5    }, { 41,-7    }, { 307,-7   }, { 323,3    }, { 328,13   },
{ 330,28   }, { 330,219  }, { 333,234  },

{ 262,64  }, { 252,56  }, { 229,51  }, { 160,51  }, { 152,54  }, { 140,61  }, { 130,74  }, { 127,92  }, { 127,206 },
{ 130,224 }, { 132,232 }, { 137,239 }, { 150,249 }, { 168,252 }, { 229,252 }, { 249,247 }, { 262,237 }, { 269,221 },
{ 272,204 }, { 272,92  }, { 269,77  },

{ 320,321 }, { 305,313 }, { 285,310 }, { 155,310 }, { 147,313 }, { 140,321 }, { 130,333 }, { 127,351 }, { 127,478 },
{ 130,498 }, { 132,506 }, { 137,514 }, { 155,524 }, { 175,526 }, { 302,524 }, { 313,521 }, { 320,514 }, { 333,498 },
{ 335,475 }, { 335,356 }, { 330,336 },

{ 3218,0   }, { 3231,-5  }, { 3244,-7  }, { 3518,-7  }, { 3526,-5  }, { 3533,0   }, { 3541,11  }, { 3541,206 }, { 3543,216 },
{ 3543,224 }, { 3548,239 }, { 3559,247 }, { 3576,254 }, { 3594,260 }, { 3602,265 }, { 3607,277 }, { 3609,290 }, { 3609,564 },
{ 3602,575 }, { 3592,580 }, { 3584,582 }, { 3515,582 }, { 3503,580 }, { 3490,572 }, { 3488,569 }, { 3485,559 }, { 3482,539 },
{ 3482,341 }, { 3475,328 }, { 3462,315 }, { 3447,313 }, { 3355,313 }, { 3343,321 }, { 3333,336 }, { 3330,354 }, { 3330,562 },
{ 3325,569 }, { 3312,580 }, { 3294,582 }, { 3249,582 }, { 3223,580 }, { 3216,575 }, { 3208,567 }, { 3206,552 }, { 3206,26  },
{ 3208,11  },

{ 3294,49  }, { 3287,51  }, { 3277,56  }, { 3267,69  }, { 3261,87  }, { 3261,211 }, { 3264,232 }, { 3272,244 }, { 3289,254 },
{ 3312,257 }, { 3444,257 }, { 3454,254 }, { 3465,249 }, { 3472,242 }, { 3480,227 }, { 3482,206 }, { 3482,89  }, { 3480,69  },
{ 3475,61  }, { 3467,54  }, { 3457,51  }, { 3447,49  },
};

void renderStaticBitfighterLogo()
{
   glColor4f(0, 1, 0, 1);
   renderBitfighterLogo(73, 1);
   UserInterface::drawCenteredStringf(120, 10, "Release %s", ZAP_GAME_RELEASE);
}


// Render logo at 0,0
void renderBitfighterLogo(U32 mask)
{
   glEnableClientState(GL_VERTEX_ARRAY);

   glVertexPointer(2, GL_SHORT, sizeof(pixLoc), &gLogoPoints[0]);

   U32 pos = 0;

   if(mask & 1 << 0)
      glDrawArrays(GL_LINE_LOOP,   0, LetterLoc1);
   pos += LetterLoc1;
   if(mask & 1 << 1)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc2);
   pos += LetterLoc2;
   if(mask & 1 << 2)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc3);
   pos += LetterLoc3;
   if(mask & 1 << 3)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc4);
   pos += LetterLoc4;
   if(mask & 1 << 4)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc5);
   pos += LetterLoc5;
   if(mask & 1 << 5)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc6);
   pos += LetterLoc6;
   if(mask & 1 << 6)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc7);
   pos += LetterLoc7;
   if(mask & 1 << 7)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc8);
   pos += LetterLoc8;
   if(mask & 1 << 8)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc9);
   pos += LetterLoc9;
   if(mask & 1 << 9)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc10);
   pos += LetterLoc10;
   if(mask & 1 << 2)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc11);
   pos += LetterLoc11;
   if(mask & 1 << 1)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc12);
   pos += LetterLoc12;
   if(mask & 1 << 1)
      glDrawArrays(GL_LINE_LOOP, pos, LetterLoc13);

   glDisableClientState(GL_VERTEX_ARRAY);
}


// Draw logo centered on screen horzontally, and on yPos vertically, scaled and rotated according to parameters
void renderBitfighterLogo(S32 yPos, F32 scale, U32 mask)
{
   const F32 fact = 0.15f * scale;   // Scaling factor to make the coordinates below fit nicely on the screen (derived by trial and error)
   
   // 3609 is the diff btwn the min and max x coords below, 594 is same for y

   glPushMatrix();
      glTranslatef((gScreenInfo.getGameCanvasWidth() - 3609 * fact) / 2, yPos - 594 * fact / 2, 0);
      glScale(fact);                   // Scale it down...
      renderBitfighterLogo(mask);
   glPopMatrix();
}


void glColor(const Color &c, float alpha)
{
    glColor4f(c.r, c.g, c.b, alpha);
}


void glColor(const Color *c, float alpha)
{
    glColor4f(c->r, c->g, c->b, alpha);
}


void drawSquare(const Point &pos, F32 size, bool filled)
{
   UserInterface::drawRect(pos.x - size, pos.y - size, pos.x + size, pos.y + size, filled ? GL_POLYGON : GL_LINE_LOOP);
}


void drawSquare(const Point &pos, S32 size, bool filled)
{
    drawSquare(pos, F32(size), filled);
}


void drawFilledSquare(const Point &pos, F32 size)
{
    drawSquare(pos, size, true);
}


void drawFilledSquare(const Point &pos, S32 size)
{
    drawSquare(pos, F32(size), true);
}


// Red vertices in walls, and magenta snapping vertices
void renderSmallSolidVertex(F32 currentScale, const Point &pos, bool snapping)
{
   F32 size = MIN(MAX(currentScale, 1), 2);
   glColor(snapping ? Colors::magenta : Colors::red);

   drawFilledSquare(pos, (F32)size / currentScale);
}


void renderVertex(char style, const Point &v, S32 number)
{
   renderVertex(style, v, number, WallItem::VERTEX_SIZE, 1, 1);
}


void renderVertex(char style, const Point &v, S32 number, F32 scale)
{
   renderVertex(style, v, number, WallItem::VERTEX_SIZE, scale, 1);
}


void renderVertex(char style, const Point &v, S32 number, F32 scale, F32 alpha)
{
   renderVertex(style, v, number, WallItem::VERTEX_SIZE, scale, alpha);
}


void renderVertex(char style, const Point &v, S32 number, S32 size, F32 scale, F32 alpha)
{
   bool hollow = style == HighlightedVertex || style == SelectedVertex || style == SelectedItemVertex || style == SnappingVertex;

   // Fill the box with a dark gray to make the number easier to read
   if(hollow && number != -1)
   {
      glColor(.25);
      drawFilledSquare(v, size / scale);
   }
      
   if(style == HighlightedVertex)
      glColor(*HIGHLIGHT_COLOR, alpha);
   else if(style == SelectedVertex)
      glColor(*SELECT_COLOR, alpha);
   else if(style == SnappingVertex)
      glColor(Colors::magenta, alpha);
   else     // SelectedItemVertex
      glColor(Colors::red, alpha);     

   drawSquare(v, (F32)size / scale, !hollow);

   if(number != NO_NUMBER)     // Draw vertex numbers
   {
      glColor(Colors::white, alpha);
      F32 fontsize = 6 / scale;
      UserInterface::drawStringf(v.x - UserInterface::getStringWidthf(fontsize, "%d", number) / 2, v.y - 3 / scale, fontsize, "%d", number);
   }
}



static void drawLetter(char letter, const Point &pos, const Color *color, F32 alpha)
{
   // Mark the item with a letter, unless we're showing the reference ship
   S32 vertOffset = 8;
   if (letter >= 'a' && letter <= 'z')    // Better position lowercase letters
      vertOffset = 10;

   glColor(color, alpha);
   F32 xpos = pos.x - UserInterface::getStringWidthf(15, "%c", letter) / 2;

   UserInterface::drawStringf(xpos, pos.y - vertOffset, 15, "%c", letter);
}


void renderSquareItem(const Point &pos, const Color *c, F32 alpha, const Color *letterColor, char letter)
{
   glColor(c, alpha);
   drawFilledSquare(pos, 8);  // Draw filled box in which we'll put our letter
   drawLetter(letter, pos, letterColor, alpha);
}


void drawCircle(F32 x, F32 y, F32 radius)
{
    glBegin(GL_LINE_LOOP);
    
    for(F32 theta = 0; theta < FloatTau; theta += 0.2f)
        glVertex2f(x + cos(theta) * radius, y + sin(theta) * radius);
    
    glEnd();
}


void drawCircle(const Point &pos, F32 radius)
{
   drawCircle(pos.x, pos.y, radius);
}


void render25FlagsBadge(F32 x, F32 y, F32 rad)
{
   glPushMatrix();
      glTranslate(x, y, 0);
      glScale(.50);
      glColor(Colors::gray40);
      drawEllipse(Point(-16, 15), 6, 2, 0);

      renderFlag(-.10f * rad, -.10f * rad, &Colors::red50);
   glPopMatrix();

   glColor(Colors::red);
   F32 ts = rad - 3;
   F32 width = UserInterface::getStringWidth(ts, "25");
   F32 tx = x + .30f * rad;
   F32 ty = y + rad - .40f * rad;

   glColor(Colors::yellow);
   UserInterface::drawFilledRect(F32(tx - width / 2.0 - 1.0), F32(ty - (ts + 2.0) / 2.0), 
                                 F32(tx + width / 2.0 + 0.5), F32(ty + (ts + 2.0) / 2.0));
   glColor(Colors::gray20);
   renderCenteredString(Point(tx, ty), ts, "25");
}


void renderDeveloperBadge(F32 x, F32 y, F32 rad)
{
   F32 rm2 = rad - 2;
   F32 rm26 = rm2 * .666f;

   glColor(Colors::green80);
   glPointSize(rad * 0.4f);
   glBegin(GL_POINTS);
      glVertex2f(x, y - rm26);
      glVertex2f(x + rm26, y);
      glVertex2f(x -rm26, y + rm26);
      glVertex2f(x, y + rm26);
      glVertex2f(x + rm26, y + rm26);
   glEnd();
}


void renderBadge(F32 x, F32 y, F32 rad, MeritBadges badge)
{
   switch(S32(badge))
   {
      case DEVELOPER_BADGE:
         renderDeveloperBadge(x, y, rad);
         return;
      case BADGE_TWENTY_FIVE_FLAGS:
         render25FlagsBadge(x, y, rad);
         return;
      default:
         TNLAssert(false, "Unknown Badge!");
   }
}


};


