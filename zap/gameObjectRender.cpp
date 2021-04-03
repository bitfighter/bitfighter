//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gameObjectRender.h"

#include "UI.h"                  // For margins only
#include "projectile.h"
#include "speedZone.h"
#include "soccerGame.h"
#include "CoreGame.h"            // For CORE_PANELS
#include "EngineeredItem.h"      // For TURRET_OFFSET
#include "BotNavMeshZone.h"      // For Border def
#include "Teleporter.h"          // For TELEPORTER_RADIUS
#include "version.h"
#include "config.h"              // Only for testing burst graphics below
#include "DisplayManager.h"
#include "game.h"
#include "VertexStylesEnum.h"
#include "FontManager.h"
#include "Asteroid.h"

#include "Colors.h"

#include "stringUtils.h"
#include "Renderer.h"
#include "RenderUtils.h"
#include "MathUtils.h"           // For converting radians to degrees, sq()
#include "GeomUtils.h"

#include "tnlRandom.h"

#ifdef SHOW_SERVER_SITUATION
#  include "GameManager.h"
#endif


namespace Zap
{

static const S32 NUM_CIRCLE_SIDES = 32;
static const F32 INV_NUM_CIRCLE_SIDES = 1 / F32(NUM_CIRCLE_SIDES);
static const F32 CIRCLE_SIDE_THETA = Float2Pi * INV_NUM_CIRCLE_SIDES;

extern F32 gLineWidth1;
extern F32 gLineWidth3;


void drawHorizLine(S32 x1, S32 x2, S32 y)
{
   drawHorizLine((F32)x1, (F32)x2, (F32)y);
}


void drawVertLine(S32 x, S32 y1, S32 y2)
{
   drawVertLine((F32)x, (F32)y1, (F32)y2);
}


void drawHorizLine(F32 x1, F32 x2, F32 y)
{
   F32 vertices[] = { x1, y,   x2, y };
   Renderer::get().renderVertexArray(vertices, 2, RenderType::Lines);
}


void drawVertLine(F32 x, F32 y1, F32 y2)
{
   F32 vertices[] = { x, y1,   x, y2 };
   Renderer::get().renderVertexArray(vertices, 2, RenderType::Lines);
}


void drawDashedLine(const Point &start, const Point &end, F32 period, F32 dutycycle, F32 fractionalOffset)
{
   Renderer& renderer = Renderer::get();

   // Quick check to see if we're just drawing a straight line
   if(dutycycle >= 1)
   {
      F32 vertices[] = { start.x, start.y, end.x, end.y };
      renderer.renderVertexArray(vertices, 2, RenderType::Lines);

      // No more to do
      return;
   }

   // Initialize point array, remember to clear at the end
   static Vector<Point> vertexArray(102);  // 51 dashes should be enough yeah?

   Point periodSeg = (end-start);
   // Save length first
   F32 lineLenSq = periodSeg.lenSquared();
   F32 lineLen = sqrt(lineLenSq);

   // Normalize to the period and scale by duty cycle
   periodSeg.normalize(period);
   Point dutySeg = periodSeg * dutycycle;

   // Draw up to the last duty cycle worth
   U32 numPeriods = U32(lineLen / period); // Integer truncation OK

   // Start point is one period segment before so we have a nice smooth transition
   Point currentDashStart = start - (periodSeg * (1-fractionalOffset));
   Point currentDashEnd = currentDashStart + dutySeg;

   Point firstDashStart = start;

   // If the end point is before the start, then shift segment forward
   if((currentDashEnd - end).lenSquared() > lineLenSq)
   {
      currentDashStart += periodSeg;
      currentDashEnd   += periodSeg;
      if(numPeriods != 0) // Don't wrap around
         numPeriods--;

      firstDashStart = currentDashStart;
   }


   // Draw first chunk
   vertexArray.push_back(firstDashStart);
   vertexArray.push_back(currentDashEnd);

   // Update to the next dash
   currentDashStart += periodSeg;
   currentDashEnd   += periodSeg;


   // Draw middles
   for(U32 i = 0; i < numPeriods; i++)
   {
      vertexArray.push_back(currentDashStart);
      vertexArray.push_back(currentDashEnd);

      // Update to the next dash
      currentDashStart += periodSeg;
      currentDashEnd   += periodSeg;
   }

   // Check to see if we drew over the line, if so replace the end
   if((start-vertexArray.last()).lenSquared() > lineLenSq)
   {
      vertexArray.pop_back();
      vertexArray.push_back(end);

      // And we're done!
   }

   // Otherwise check to see if we need another segment due to varying FF
   // lengths
   else if((currentDashStart - start).lenSquared() < lineLenSq)
   {
      // Draw last
      Point lastDashEnd = currentDashEnd;

      if((currentDashEnd-start).lenSquared() > lineLenSq)
         lastDashEnd = end;

      vertexArray.push_back(currentDashStart);
      vertexArray.push_back(lastDashEnd);   // Finish up just to the end
   }

   // Render!
   renderer.renderPointVector(&vertexArray, RenderType::Lines);

   // Clean up
   vertexArray.clear();
}

// Draw arc centered on pos, with given radius, from startAngle to endAngle.  0 is East, increasing CW
void drawArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle)
{
   // With theta delta of 0.2, that means maximum 32 points + 1 at the end
   static const S32 MAX_POINTS = NUM_CIRCLE_SIDES + 1;
   static F32 arcVertexArray[MAX_POINTS * 2];      // 2 components per point

   U32 count = 0;
   for(F32 theta = startAngle; theta < endAngle; theta += CIRCLE_SIDE_THETA)
   {
      arcVertexArray[2*count]       = pos.x + cos(theta) * radius;
      arcVertexArray[(2*count) + 1] = pos.y + sin(theta) * radius;
      count++;
   }

   // Make sure arc makes it all the way to endAngle...  rounding errors look terrible!
   arcVertexArray[2*count]       = pos.x + cos(endAngle) * radius;
   arcVertexArray[(2*count) + 1] = pos.y + sin(endAngle) * radius;
   count++;

   Renderer::get().renderVertexArray(arcVertexArray, count, RenderType::LineStrip);
}


void drawDashedArc(const Point &center, F32 radius, F32 arcTheta, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle)
{
   F32 interimAngle = arcTheta / dashCount;

   for(S32 i = 0; i < dashCount; i++)
      drawArc(center, radius, interimAngle * i + offsetAngle, (interimAngle * (i + 1)) - dashSpaceCentralAngle + offsetAngle);
}


void drawDashedCircle(const Point &center, F32 radius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle)
{
   drawDashedArc(center, radius, FloatTau, dashCount, dashSpaceCentralAngle, offsetAngle);
}


void drawAngledRay(const Point &center, F32 innerRadius, F32 outerRadius, F32 angle)
{
   F32 vertices[] = {
         center.x + cos(angle) * innerRadius, center.y + sin(angle) * innerRadius,
         center.x + cos(angle) * outerRadius, center.y + sin(angle) * outerRadius,
   };

   Renderer::get().renderVertexArray(vertices, 2, RenderType::LineStrip);
}


void drawAngledRayCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 rayCount, F32 offsetAngle)
{
   drawAngledRayArc(center, innerRadius, outerRadius, FloatTau, rayCount, offsetAngle);
}


void drawAngledRayArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, S32 rayCount, F32 offsetAngle)
{
   F32 interimAngle = centralAngle / rayCount;

   for(S32 i = 0; i < rayCount; i++)
      drawAngledRay(center, innerRadius, outerRadius, interimAngle * i + offsetAngle);
}


void drawDashedHollowCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle)
{
   // Draw the dashed circles
   drawDashedCircle(center, innerRadius, dashCount, dashSpaceCentralAngle, offsetAngle);
   drawDashedCircle(center, outerRadius, dashCount, dashSpaceCentralAngle, offsetAngle);

   // Now connect them
   drawAngledRayCircle(center, innerRadius,  outerRadius, dashCount, offsetAngle);
   drawAngledRayCircle(center, innerRadius,  outerRadius, dashCount, offsetAngle - dashSpaceCentralAngle);
}


void drawHollowArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, F32 offsetAngle)
{
   drawAngledRay(center, innerRadius, outerRadius, offsetAngle);
   drawAngledRay(center, innerRadius, outerRadius, offsetAngle + centralAngle);

   drawArc(center, innerRadius, offsetAngle, offsetAngle + centralAngle);
   drawArc(center, outerRadius, offsetAngle, offsetAngle + centralAngle);
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
   F32 width2  = width  / 2;
   F32 height2 = height / 2;

   F32 vertices[] = {
         pos.x - width2 + rad, pos.y - height2,
         pos.x + width2 - rad, pos.y - height2,

         pos.x + width2, pos.y - height2 + rad,
         pos.x + width2, pos.y + height2 - rad,

         pos.x + width2 - rad, pos.y + height2,
         pos.x - width2 + rad, pos.y + height2,

         pos.x - width2, pos.y + height2 - rad,
         pos.x - width2, pos.y - height2 + rad
   };

   Renderer::get().renderVertexArray(vertices, 8, RenderType::Lines);

   // Now add some quarter-rounds in the corners, start in UL, proceed CW
   p.set(pos.x - width2 + rad, pos.y - height2 + rad);
   drawArc(p, rad, -FloatPi, -FloatHalfPi);

   p.set(pos.x + width2 - rad, pos.y - height2 + rad);
   drawArc(p, rad, -FloatHalfPi, 0);

   p.set(pos.x + width2 - rad, pos.y + height2 - rad);
   drawArc(p, rad, 0, FloatHalfPi);

   p.set(pos.x - width2 + rad, pos.y + height2 - rad);
   drawArc(p, rad, FloatHalfPi, FloatPi);
}


void drawFilledArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle)
{
   // With theta delta of 0.2, that means maximum 32 points + 2 at the end
   const S32 MAX_POINTS = 32 + 2;
   static F32 filledArcVertexArray[MAX_POINTS * 2];      // 2 components per point

   U32 count = 0;

   for(F32 theta = startAngle; theta < endAngle; theta += CIRCLE_SIDE_THETA)
   {
      filledArcVertexArray[2*count]       = pos.x + cos(theta) * radius;
      filledArcVertexArray[(2*count) + 1] = pos.y + sin(theta) * radius;
      count++;
   }

   // Make sure arc makes it all the way to endAngle...  rounding errors look terrible!
   filledArcVertexArray[2*count]       = pos.x + cos(endAngle) * radius;
   filledArcVertexArray[(2*count) + 1] = pos.y + sin(endAngle) * radius;
   count++;

   filledArcVertexArray[2*count]       = pos.x;
   filledArcVertexArray[(2*count) + 1] = pos.y;
   count++;

   Renderer::get().renderVertexArray(filledArcVertexArray, count, RenderType::TriangleFan);
}


void drawFilledRoundedRect(const Point &pos, S32 width, S32 height, const Color &fillColor, const Color &outlineColor, S32 radius, F32 alpha)
{
   drawFilledRoundedRect(pos, (F32)width, (F32)height, fillColor, outlineColor, (F32)radius, alpha);
}


void drawFilledRoundedRect(const Point &pos, F32 width, F32 height, const Color &fillColor, const Color &outlineColor, F32 radius, F32 alpha)
{
   Renderer& r = Renderer::get();

   r.setColor(fillColor, alpha);

   drawFilledArc(Point(pos.x - width / 2 + radius, pos.y - height / 2 + radius), radius,      FloatPi, FloatPi + FloatHalfPi);
   drawFilledArc(Point(pos.x + width / 2 - radius, pos.y - height / 2 + radius), radius, -FloatHalfPi, 0);
   drawFilledArc(Point(pos.x + width / 2 - radius, pos.y + height / 2 - radius), radius,            0, FloatHalfPi);
   drawFilledArc(Point(pos.x - width / 2 + radius, pos.y + height / 2 - radius), radius,  FloatHalfPi, FloatPi);

   drawRect(pos.x - width / 2, pos.y - height / 2 + radius, 
            pos.x + width / 2, pos.y + height / 2 - radius, RenderType::TriangleFan);

   drawRect(pos.x - width / 2 + radius, pos.y - height / 2, 
            pos.x + width / 2 - radius, pos.y - height / 2 + radius, RenderType::TriangleFan);

   drawRect(pos.x - width / 2 + radius, pos.y + height / 2, 
            pos.x + width / 2 - radius, pos.y + height / 2 - radius, RenderType::TriangleFan);

   r.setColor(outlineColor, alpha);
   drawRoundedRect(pos, width, height, radius);
}


// Actually draw the ellipse
void drawFilledEllipseUtil(const Point &pos, F32 width, F32 height, F32 angle, RenderType mode)
{
   F32 sinbeta = sin(angle);
   F32 cosbeta = cos(angle);

   // 32 vertices to fake our ellipse
   F32 vertexArray[64];
   U32 count = 0;
   for(F32 theta = 0; theta < FloatTau; theta += CIRCLE_SIDE_THETA)
   {
      F32 sinalpha = sin(theta);
      F32 cosalpha = cos(theta);

      vertexArray[2*count]     = pos.x + (width * cosalpha * cosbeta - height * sinalpha * sinbeta);
      vertexArray[(2*count)+1] = pos.y + (width * cosalpha * sinbeta + height * sinalpha * cosbeta);
      count++;
   }

   Renderer::get().renderVertexArray(vertexArray, ARRAYSIZE(vertexArray) / 2, mode);
}


// Draw an n-sided polygon
void drawPolygon(const Point &pos, S32 sides, F32 radius, F32 angle)
{
   static const S32 MaxSides = 32;

   TNLAssert(sides <= MaxSides, "Too many sides!");

   // There is no polygon greater than 12 (I think) so I choose 32 sides to be safe
   static F32 polygonVertexArray[MaxSides * 2];  // 2 data points per vertex (x,y)

   F32 theta = 0;
   F32 dTheta = FloatTau / sides;
   for(S32 i = 0; i < sides; i++)
   {
      polygonVertexArray[2*i]       = pos.x + cos(theta + angle) * radius;
      polygonVertexArray[(2*i) + 1] = pos.y + sin(theta + angle) * radius;
      theta += dTheta;
   }

   Renderer::get().renderVertexArray(polygonVertexArray, sides, RenderType::LineLoop);
}


// Draw an ellipse at pos, with axes width and height, canted at angle
void drawEllipse(const Point &pos, F32 width, F32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, width, height, angle, RenderType::LineLoop);
}


// Draw an ellipse at pos, with axes width and height, canted at angle
void drawEllipse(const Point &pos, S32 width, S32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, (F32)width, (F32)height, angle, RenderType::LineLoop);
}


// Well...  draws a filled ellipse, much as you'd expect
void drawFilledEllipse(const Point &pos, F32 width, F32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, width, height, angle, RenderType::TriangleFan);
}


void drawFilledEllipse(const Point &pos, S32 width, S32 height, F32 angle)
{
   drawFilledEllipseUtil(pos, (F32)width, (F32)height, angle, RenderType::TriangleFan);
}


void drawFilledCircle(const Point &pos, F32 radius, const Color *color)
{
   if(color)
      Renderer::get().setColor(*color);

   drawFilledSector(pos, radius, 0, FloatTau);
}


void drawFilledSector(const Point &pos, F32 radius, F32 start, F32 end)
{
   // With theta delta of 0.2, that means maximum 32 points
   static const S32 MAX_POINTS = 32;
   static F32 filledSectorVertexArray[MAX_POINTS * 2];      // 2 components per point

   U32 count = 0;

   for(F32 theta = start; theta < end; theta += CIRCLE_SIDE_THETA)
   {
      filledSectorVertexArray[2*count]       = pos.x + cos(theta) * radius;
      filledSectorVertexArray[(2*count) + 1] = pos.y + sin(theta) * radius;
      count++;
   }

   Renderer::get().renderVertexArray(filledSectorVertexArray, count, RenderType::TriangleFan);
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

   Vector<Point> vertexArray(2 * hatchCount);
   for(F32 i = 0; i < hatchCount; i++)
   {
      dirx = dir;                                  // Reset to original value
      segMid = base + dirx * (i + 0.5f) / F32(HATCH_COUNT) * length;      // Adding 0.5 causes hatches to be centered properly
      vertexArray.push_back(segMid - cross * F32(width) * 0.5);
      vertexArray.push_back(segMid + cross * F32(width) * 0.5);
   }

   Renderer::get().renderPointVector(&vertexArray, RenderType::Lines);
}


static void renderActiveModuleOverlays(F32 alpha, F32 radius, U32 sensorTime, bool shieldActive, 
                                       bool sensorActive, bool repairActive, bool hasArmor)
{
   Renderer& r = Renderer::get();

   // Armor
   if(hasArmor)
   {
      r.setLineWidth(gLineWidth3);
      r.setColor(Colors::yellow, alpha);

      drawPolygon(Point(0,0), 5, 30, FloatHalfPi);

      r.setLineWidth(gDefaultLineWidth);
   }

   // Shields
   if(shieldActive)
   {
      F32 shieldRadius = radius + 3;      // radius is the ship radius

      r.setColor(Colors::yellow, alpha);
      drawCircle(0, 0, shieldRadius);
   }

#ifdef SHOW_SERVER_SITUATION
   ServerGame *serverGame = GameManager::getServerGame();

   if(serverGame && static_cast<Ship *>(serverGame->getClientInfo(0)->getConnection()->getControlObject()) &&
      static_cast<Ship *>(serverGame->getClientInfo(0)->getConnection()->getControlObject())->isModulePrimaryActive(ModuleShield))
   {
      F32 shieldRadius = radius;

      r.setColor(Colors::green, alpha);
      drawCircle(0, 0, shieldRadius);
   }
#endif


   // Sensor
//   if(sensorActive)
//   {
//      glColor(Colors::white, alpha);
//      F32 radius = (sensorTime & 0x1FF) * 0.002f;    // Radius changes over time
//      drawCircle(0, 0, radius * Ship::CollisionRadius + 4);
//   }

   // Repair
   if(repairActive)
   {
      r.setLineWidth(gLineWidth3);
      r.setColor(Colors::red, alpha);
      drawCircle(0, 0, 18);
      r.setLineWidth(gDefaultLineWidth);
   }
}


static void renderShipFlame(ShipFlame *flames, S32 flameCount, F32 thrust, F32 alpha, bool yThruster)
{
   Renderer& r = Renderer::get();

   for(S32 i = 0; i < flameCount; i++)
      for(S32 j = 0; j < flames[i].layerCount; j++)
      {
         ShipFlameLayer *flameLayer = &flames[i].layers[j];
         r.setColor(flameLayer->color, alpha);

         F32 yThrusterX;
         F32 yThrusterY;
         if(yThruster)
         {
            yThrusterX = flameLayer->points[2];
            yThrusterY = flameLayer->points[3] + (flameLayer->multiplier * thrust);
         }
         else
         {
            yThrusterX = flameLayer->points[2] + (flameLayer->multiplier * thrust);
            yThrusterY = flameLayer->points[3];
         }

         F32 vertices[] = {
               flameLayer->points[0], flameLayer->points[1],
               yThrusterX, yThrusterY,
               flameLayer->points[4], flameLayer->points[5]
         };
         r.renderVertexArray(vertices, 3, RenderType::LineStrip);
      }
}


void renderShip(ShipShape::ShipShapeType shapeType, const Color *shipColor, const Color &hbc, F32 alpha, F32 thrusts[], F32 health, F32 radius, U32 sensorTime,
                bool shieldActive, bool sensorActive, bool repairActive, bool hasArmor)
{
   Renderer& r = Renderer::get();
   ShipShapeInfo *shipShapeInfo = &ShipShape::shipShapeInfos[shapeType];

   // First render the thruster flames
   if(thrusts[0] > 0) // forward thrust
      renderShipFlame(shipShapeInfo->forwardFlames, shipShapeInfo->forwardFlameCount, thrusts[0], alpha, true);

   if(thrusts[1] > 0) // back thrust
      renderShipFlame(shipShapeInfo->reverseFlames, shipShapeInfo->reverseFlameCount, thrusts[1], alpha, true);

   // Right/left rotational thrusters - only one or the other
   if(thrusts[2] > 0)
      renderShipFlame(shipShapeInfo->portFlames, shipShapeInfo->portFlameCount, thrusts[2], alpha, false);

   else if(thrusts[3] > 0)
      renderShipFlame(shipShapeInfo->starboardFlames, shipShapeInfo->starboardFlameCount, thrusts[3], alpha, false);

   // Flame ports...
   r.setColor(Colors::gray50, alpha);
   r.renderVertexArray(shipShapeInfo->flamePortPoints, shipShapeInfo->flamePortPointCount, RenderType::Lines);

   // Inner hull with colored insides
   r.setColor(*shipColor, alpha);
   for(S32 i = 0; i < shipShapeInfo->innerHullPieceCount; i++)
      r.renderVertexArray(shipShapeInfo->innerHullPieces[i].points, shipShapeInfo->innerHullPieces[i].pointCount, RenderType::LineStrip);

   // Render health bar
   r.setColor(hbc, alpha);
   renderHealthBar(health, Point(0,1.5), Point(0,1), 28, 4);

   // Grey outer hull drawn last, on top
   r.setColor(Colors::gray70, alpha);
   r.renderVertexArray(shipShapeInfo->outerHullPoints, shipShapeInfo->outerHullPointCount, RenderType::LineLoop);

   // Now render any module states
   renderActiveModuleOverlays(alpha, radius, sensorTime, shieldActive, sensorActive, repairActive, hasArmor);
}


// Figure out the intensity of the thrusters based on the ship's actions and orientation
static void calcThrustComponents(const Point &velocity, F32 angle, F32 deltaAngle, bool boostActive, F32 *thrusts)
{
   Point vel = velocity;         // Make a working copy that we can modify

   F32 len = vel.len();

   // Reset thrusts
   for(U32 i = 0; i < 4; i++)
      thrusts[i] = 0;            

   if(len > 0)
   {
      if(len > 1)
         vel *= 1 / len;

      Point shipDirs[4];
      shipDirs[0].set(cos(angle), sin(angle) );
      shipDirs[1].set(-shipDirs[0]);
      shipDirs[2].set( shipDirs[0].y, -shipDirs[0].x);
      shipDirs[3].set(-shipDirs[0].y,  shipDirs[0].x);

      for(U32 i = 0; i < 4; i++)
         thrusts[i] = shipDirs[i].dot(vel);
   }


   if(deltaAngle > 0.001)
      thrusts[3] += 0.25;
   else if(deltaAngle < -0.001)
      thrusts[2] += 0.25;
   
   if(boostActive)
      for(U32 i = 0; i < 4; i++)
         thrusts[i] *= 1.3f;
}


// Passed position is lower left corner of player name
//static
void renderGamesPlayedMark(S32 x, S32 y, S32 height, U32 gamesPlayed)
{
   S32 sym = Platform::getRealMilliseconds() / 2000 % 9;

   //FontManager::pushFontContext(FPSContext);

//   glLineWidth(gLineWidth1);
//
//
//   if(sym > 0)
//   {
//      S32 cenx = x - height/2 - 3;
//      S32 ceny = y - height/4 - 3;
//
//      drawHollowSquare(Point(cenx, ceny), height/2);
//
//      drawStringc(Point(cenx, ceny + height/2 - 3), height - 4, itos(sym).c_str());
//   }


   //// Four square
   //S32 passedHeight = height;
   //height = (height + 3) & ~0x03;      // Round height up to next multiple of 4

   //// Adjust x and y to be center of rank icon
   //y -= height / 2 - (height - passedHeight) / 2 - 1;
   //x -= height / 2 + 4;    // 4 provides a margin

   //F32 rad = height / 4.0f;
   //S32 yoffset = 0;     // Tricky way to vertically align the squares when there are only 1 or 2

//   glLineWidth(gDefaultLineWidth);

   //FontManager::popFontContext();


   //// Chevrons
   //const F32 chevronPoints[] = {
   //     -6,  0,
   //     0, -3,
   //     0, -3,
   //     6,  0,
   //     -6,  6,
   //     0,  3,
   //     0,  3,
   //     6,  6,
   //     -6,  12,
   //     0,  9,
   //     0,  9,
   //     6,  12,
   //};
   //
   //// Bars
   //const F32 barPoints[] = {
   //     -6,  0,
   //     6, 0,
   //     -6,  6,
   //     6, 6,
   //     -6,  12,
   //     6, 12,
   //};
   //
   //
   //
   //glPushMatrix();
   //// Changed position on me?
   //x = x - 8;
   //y = y - 6;
   //switch(sym)
   //{
   //   case 0:
   //      glColor(Colors::bronze);
   //      glTranslate(x, y + 1, 0);
   //      renderVertexArray(chevronPoints, 4, GL_LINES);
   //      break;
   //   case 1:
   //      glColor(Colors::bronze);
   //      glTranslate(x, y - 2, 0);
   //      renderVertexArray(chevronPoints, 8, GL_LINES);
   //      break;
   //   case 2:
   //      glColor(Colors::bronze);
   //      glTranslate(x, y - 5, 0);
   //      renderVertexArray(chevronPoints, 12, GL_LINES);
   //      break;
   //   case 3:
   //      glColor(Colors::silver);
   //      glTranslate(x, y, 0);
   //      renderVertexArray(barPoints, 2, GL_LINES);
   //      break;
   //   case 4:
   //      glColor(Colors::silver);
   //      glTranslate(x, y - 3, 0);
   //      renderVertexArray(barPoints, 4, GL_LINES);
   //      break;
   //   case 5:
   //      glColor(Colors::silver);
   //      glTranslate(x, y - 6, 0);
   //      renderVertexArray(barPoints, 6, GL_LINES);
   //      break;
   //   case 6:
   //      glLineWidth(gLineWidth1);
   //      glColor(Colors::gold);
   //      drawStar(Point(x,y), 5, height / 2.0f, rad);
   //      glLineWidth(gDefaultLineWidth);
   //      break;
   //   case 7:
   //      glLineWidth(gLineWidth1);
   //      glColor(Colors::gold);
   //      drawFilledStar(Point(x,y), 5, height / 2.0f, rad);
   //      glLineWidth(gDefaultLineWidth);
   //      break;
   //}
   //glPopMatrix();


   static const F32 rectPoints[] = {
         -7, -7,
         -1, -7,
         -1, -1,
         -7, -1,

         1, -7,
         7, -7,
         7, -1,
         1, -1,

         -7,  7,
         -1,  7,
         -1,  1,
         -7,  1,

         1, 7,
         7, 7,
         7, 1,
         1, 1,
   };

//   static const F32 triPoints[] = {
//         -6, -6,
//         -1, -1,
//         -6, -1,
//
//         1, -6,
//         6, -1,
//         1, -1,
//
//         -6,  6,
//         -1,  6,
//         -6,  1,
//
//         1, 6,
//         6, 6,
//         1, 1,
//   };

   Renderer& r = Renderer::get();

   r.pushMatrix();

   r.translate(x - 10, y - 6, 0);
   
   r.setLineWidth(gLineWidth1);

   r.setColor(Colors::gray20);
   r.renderVertexArray(rectPoints, 4, RenderType::LineLoop);
   r.renderVertexArray(&rectPoints[8], 4, RenderType::LineLoop);
   r.renderVertexArray(&rectPoints[16], 4, RenderType::LineLoop);
   r.renderVertexArray(&rectPoints[24], 4, RenderType::LineLoop);

   switch(sym)
   {
      case 0:
         //glColor(Colors::red);
         //renderVertexArray(triPoints, 3, GL_TRIANGLE_FAN);
         r.setColor(Colors::green80);
         r.renderVertexArray(rectPoints, 4, RenderType::LineLoop);
         break;
      case 1:
         r.setColor(Colors::green80);
         r.renderVertexArray(rectPoints, 4, RenderType::TriangleFan);
         r.renderVertexArray(rectPoints, 4, RenderType::LineLoop);
         break;
      case 2:
         r.setColor(Colors::green80);
         r.renderVertexArray(rectPoints, 4, RenderType::TriangleFan);
         r.renderVertexArray(rectPoints, 4, RenderType::LineLoop);
         //glColor(Colors::red);
         //renderVertexArray(&triPoints[6], 3, GL_TRIANGLE_FAN);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::LineLoop);
         break;
      case 3:
         r.setColor(Colors::green80);
         r.renderVertexArray(rectPoints, 4, RenderType::TriangleFan);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::TriangleFan);
         r.renderVertexArray(rectPoints, 4, RenderType::LineLoop);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::LineLoop);
         break;
      case 4:
         r.setColor(Colors::green80);
         r.renderVertexArray(rectPoints, 4, RenderType::TriangleFan);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::TriangleFan);
         r.renderVertexArray(rectPoints, 4, RenderType::LineLoop);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::LineLoop);
         //glColor(Colors::red);
         //renderVertexArray(&triPoints[12], 3, GL_TRIANGLE_FAN);
         r.renderVertexArray(&rectPoints[16], 4, RenderType::LineLoop);
         break;
      case 5:
         r.setColor(Colors::green80);
         r.renderVertexArray(rectPoints, 4, RenderType::TriangleFan);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::TriangleFan);
         r.renderVertexArray(&rectPoints[16], 4, RenderType::TriangleFan);
         r.renderVertexArray(rectPoints, 4, RenderType::LineLoop);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::LineLoop);
         r.renderVertexArray(&rectPoints[16], 4, RenderType::LineLoop);
         break;
      case 6:
         r.setColor(Colors::green80);
         r.renderVertexArray(rectPoints, 4, RenderType::TriangleFan);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::TriangleFan);
         r.renderVertexArray(&rectPoints[16], 4, RenderType::TriangleFan);
         r.renderVertexArray(rectPoints, 4, RenderType::LineLoop);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::LineLoop);
         r.renderVertexArray(&rectPoints[16], 4, RenderType::LineLoop);
         //glColor(Colors::red);
         //renderVertexArray(&triPoints[18], 3, GL_TRIANGLE_FAN);
         r.renderVertexArray(&rectPoints[24], 4, RenderType::LineLoop);
         break;
      case 7:
         r.setColor(Colors::green80);
         r.renderVertexArray(rectPoints, 4, RenderType::TriangleFan);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::TriangleFan);
         r.renderVertexArray(&rectPoints[16], 4, RenderType::TriangleFan);
         r.renderVertexArray(&rectPoints[24], 4, RenderType::TriangleFan);
         r.renderVertexArray(rectPoints, 4, RenderType::LineLoop);
         r.renderVertexArray(&rectPoints[8], 4, RenderType::LineLoop);
         r.renderVertexArray(&rectPoints[16], 4, RenderType::LineLoop);
         r.renderVertexArray(&rectPoints[24], 4, RenderType::LineLoop);
         break;
   }
   r.popMatrix();
   r.setLineWidth(gDefaultLineWidth);
}


static void renderShipName(const string &shipName, bool isAuthenticated, bool isBusy, U32 killStreak, U32 gamesPlayed, F32 alpha)
{
   Renderer& r = Renderer::get();
   string renderName = isBusy ? "<<" + shipName + ">>" : shipName;

   F32 textAlpha = alpha;
   S32 textSize = 14;

   r.setLineWidth(gLineWidth1);


   // Set name color based on killStreak length
   if(killStreak >= UserInterface::StreakingThreshold)      
      r.setColor(Colors::streakPlayerNameColor, textAlpha);    
   else                                      
      r.setColor(Colors::idlePlayerNameColor, textAlpha);         // <=== Probably wrong, not sure how to fix...  


   S32 ypos = 30 + textSize;

   FontManager::pushFontContext(OldSkoolContext);
   S32 len = drawStringc(0, ypos, textSize, renderName.c_str());
   FontManager::popFontContext();

//   renderGamesPlayedMark(-len / 2, ypos, textSize, gamesPlayed);

   // Underline name if player is authenticated
   if(isAuthenticated)
      drawHorizLine(-len/2, len/2, 33 + textSize);

   r.setLineWidth(gDefaultLineWidth);
}


void renderShip(S32 layerIndex, const Point &renderPos, const Point &actualPos, const Point &vel, 
                F32 angle, F32 deltaAngle, ShipShape::ShipShapeType shape, const Color *color, const Color &hbc, F32 alpha, 
                U32 renderTime, const string &shipName, F32 warpInScale, bool isLocalShip, bool isBusy, 
                bool isAuthenticated, bool showCoordinates, F32 health, F32 radius, S32 team, 
                bool boostActive, bool shieldActive, bool repairActive, bool sensorActive, 
                bool hasArmor, bool engineeringTeleport, U32 killStreak, U32 gamesPlayed)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
   r.translate(renderPos);

   // Draw the ship name, if there is one, before the glRotatef below, but only on layer 1...
   // Don't label the local ship.
   if(!isLocalShip && layerIndex == 1 && shipName != "")  
   {
      renderShipName(shipName, isAuthenticated, isBusy, killStreak, gamesPlayed, alpha);

      // Show if the player is engineering a teleport
      if(engineeringTeleport)
         renderTeleporterOutline(Point(cos(angle), sin(angle)) * (Ship::CollisionRadius + Teleporter::TELEPORTER_RADIUS),
               (F32)Teleporter::TELEPORTER_RADIUS, Colors::richGreen);
   }

   if(showCoordinates && layerIndex == 1)
      renderShipCoords(actualPos, isLocalShip, alpha);

   F32 rotAmount = 0;      
   if(warpInScale < 0.8f)
      rotAmount = (0.8f - warpInScale) * 540;

   // An angle of 0 means the ship is heading down the +X axis since we draw the ship 
   // pointing up the Y axis, we should rotate by the ship's angle, - 90 degrees
   r.rotate(radiansToDegrees(angle) - 90 + rotAmount, 0, 0, 1.0);
   r.scale(warpInScale);

   // NOTE: Get rid of this if we stop sending location of cloaked ship to clients.  Also, we can stop
   //       coming here altogether when layerIndex is not 1, and strip out a bunch of ifs above.
   if(layerIndex == -1)    
   {
      // Draw the outline of the ship in solid black -- this will block out any stars and give
      // a tantalizing hint of motion when the ship is cloaked.  Could also try some sort of star-twinkling or
      // scrambling thing here as well...
      r.setColor(Colors::black);

      glDisable(GL_BLEND);

      F32 vertices[] = { 20,-15,  0,25,  20,-15 };
      r.renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, RenderType::TriangleFan);

      glEnable(GL_BLEND);
   }

   else     // LayerIndex == 1
   {
      // Calculate the various thrust components for rendering purposes; fills thrusts
      static F32 thrusts[4];
      calcThrustComponents(vel, angle, deltaAngle, boostActive, thrusts);  

      renderShip(shape, color, hbc, alpha, thrusts, health, radius, renderTime,
                 shieldActive, sensorActive, repairActive, hasArmor);
   }

   r.popMatrix();
}


void renderSpawnShield(const Point &pos, U32 shieldTime, U32 renderTime)
{
   Renderer& r = Renderer::get();

   static const U32 BlinkStartTime = 1500;
   static const U32 BlinkCycleDuration = 300;
   static const U32 BlinkDuration = BlinkCycleDuration / 2;       // Time shield is yellow or green during

   if(shieldTime > BlinkStartTime || shieldTime % BlinkCycleDuration > BlinkDuration)
      r.setColor(Colors::green65);  
   else
      r.setColor(Colors::yellow40);

   // This rather gross looking variable helps manage problems with the resolution of F32s when getRealMilliseconds() returns a large value
   const S32 BiggishNumber = 21988;
   F32 offset = F32(renderTime % BiggishNumber) * FloatTau / BiggishNumber;
   drawDashedHollowCircle(pos, Ship::CollisionRadius + 5, Ship::CollisionRadius + 10, 8, FloatTau / 24.0f, offset);
}


// Render repair rays to all the repairing objects
void renderShipRepairRays(const Point &pos, const Ship *ship, Vector<SafePtr<BfObject> > &repairTargets, F32 alpha)
{
   Renderer& r = Renderer::get();

   r.setLineWidth(gLineWidth3);
   r.setColor(Colors::red, alpha);

   for(S32 i = 0; i < repairTargets.size(); i++)
   {
      if(repairTargets[i] && repairTargets[i].getPointer() != ship)
      {
         Vector<Point> targetRepairLocations = repairTargets[i]->getRepairLocations(pos);

         Vector<Point> vertexArray(2 * targetRepairLocations.size());
         for(S32 i = 0; i < targetRepairLocations.size(); i++)
         {
            vertexArray.push_back(pos);
            vertexArray.push_back(targetRepairLocations[i]);
         }

         r.renderPointVector(&vertexArray, RenderType::Lines);
      }
   }
   r.setLineWidth(gDefaultLineWidth);
}


void renderShipCoords(const Point &coords, bool localShip, F32 alpha)
{
   Renderer& r = Renderer::get();

   string str = string("@") + itos((S32) coords.x) + "," + itos((S32) coords.y);
   const U32 textSize = 18;

   r.setLineWidth(gLineWidth1);
   r.setColor(Colors::white, 0.5f * alpha);

   FontManager::pushFontContext(OldSkoolContext);
   drawStringc(0, 30 + (localShip ? 0 : textSize + 3) + textSize, textSize, str.c_str() );
   FontManager::popFontContext();

   r.setLineWidth(gDefaultLineWidth);
}


void drawFourArrows(const Point &pos)
{
   Renderer& r = Renderer::get();

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

   r.pushMatrix();
      r.translate(pos);
      r.renderVertexArray(pointList, ARRAYSIZE(pointList) / 2, RenderType::Lines);
   r.popMatrix();

}


// TODO: Document me better!  Especially the nerdy math stuff
void renderTeleporter(const Point &pos, U32 type, bool spiralInwards, U32 time, F32 zoomFraction, F32 radiusFraction, F32 radius, F32 alpha,
                      const Vector<Point> *dests, U32 trackerCount)
{
   Renderer& renderer = Renderer::get();

   const S32 NumColors = 6;
   const U32 MaxParticles = 100;

   // Object to hold data on each swirling particle+trail
   struct Tracker
   {
      F32 thetaI;
      F32 thetaP;
      F32 dI;
      F32 dP;
      U32 ci;
   };

   // Our Tracker array.  This is global so each teleporter uses the same values
   static Tracker particles[MaxParticles];

   // Different teleport color styles
   static float colors[][NumColors][3] = {
      {  // 0 -> Our standard blue-styled teleporter                                               
         { 0, 0.25, 0.8f },
         { 0, 0.5, 1 },
         { 0, 0, 1 },
         { 0, 1, 1 },
         { 0, 0.5, 0.5 },
         { 0, 0, 1 },
      },
      {  // 1 -> Unused red/blue/purpley style
         { 1, 0, 0.5 },
         { 1, 0, 1 },
         { 0, 0, 1 },
         { 0.5, 0, 1 },
         { 0, 0, 0.5 },
         { 1, 0, 0 },
      },
      {  // 2 -> Our green engineered teleporter
         { 0, 0.8f, 0.25f },
         { 0.5, 1.0, 0 },
         { 0, 1, 0 },
         { 1, 1, 0 },
         { 0.5, 0.5, 0 },
         { 0, 1, 0 },
      },
      {  // 3 -> "Dead" tracker gray
         { 0.20f, 0.20f, 0.20f },
         { 0.50f, 0.50f, 0.50f },
         { 0.75f, 0.75f, 0.75f },
         { 0.30f, 0.30f, 0.30f },
         { 0.50f, 0.50f, 0.50f },
         { 0.15f, 0.15f, 0.15f },
      }
   };

   // Loads some random values for each Tracker, only once.  These values determine the
   // variation in Tracker arc steepness, etc.
   static bool trackerInit = false;

   if(!trackerInit)
   {
      trackerInit = true;
      for(U32 i = 0; i < MaxParticles; i++)
      {
         Tracker &t = particles[i];

         t.thetaI = TNL::Random::readF() * FloatTau;
         t.thetaP = TNL::Random::readF() * 2 + 0.5f;
         t.dP = TNL::Random::readF() * 5 + 2.5f;
         t.dI = TNL::Random::readF() * t.dP;
         t.ci = TNL::Random::readI(0, NumColors - 1);
      }
   }

   // Now the drawing!
   renderer.pushMatrix();

   // This piece draws the destination lines in the commander's map
   // It knows it's in the commander's map if the zoomFraction is greater than zero
   if(zoomFraction > 0)
   {
      const F32 width = 6.0;
      const F32 alpha = zoomFraction;

      renderer.setColor(Colors::white, .25f * alpha );

//      glEnable(GL_POLYGON_SMOOTH);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//      glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

      // Draw a different line for each destination
      for(S32 i = 0; i < dests->size(); i++)
      {
         F32 ang = pos.angleTo(dests->get(i));
         F32 sina = sin(ang);
         F32 cosa = cos(ang);
         F32 asina = (sina * cosa < 0) ? abs(sina) : -abs(sina);
         F32 acosa = abs(cosa);

         F32 dist = pos.distanceTo(dests->get(i));

         F32 midx = pos.x + .75f * cosa * dist;
         F32 midy = pos.y + .75f * sina * dist;

         renderer.setColor(Colors::white, .25f * alpha);
         F32 vertices[] = {
               pos.x + asina * width, pos.y + acosa * width,
               midx + asina * width, midy + acosa * width,
               midx - asina * width, midy - acosa * width,
               pos.x - asina * width, pos.y - acosa * width
         };
         renderer.renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, RenderType::TriangleFan);

         // Render blended part
         F32 x = dests->get(i).x;
         F32 y = dests->get(i).y;
         F32 vertices2[] = {
               midx + asina * width, midy + acosa * width,
               x + asina * width, y + acosa * width,
               x - asina * width, y - acosa * width,
               midx - asina * width, midy - acosa * width
         };
         F32 colors[] = {
               1, 1, 1, .25f * alpha,
               1, 1, 1, 0,
               1, 1, 1, 0,
               1, 1, 1, .25f * alpha
         };
         renderer.renderColored(vertices2, colors, ARRAYSIZE(vertices2) / 2, RenderType::TriangleFan);
      }

      //   glDisable(GL_POLYGON_SMOOTH);
   }


   renderer.translate(pos);

   // This adjusts the starting color of each particle to white depending on the radius
   // of the teleporter.  This makes the whole teleport look white when you move a
   // ship into it and it shrinks; then it expands and slowly fades back the colors
   Color liveColors[NumColors];
   for(S32 i = 0; i < NumColors; i++)
   {
      Color c(colors[type][i][0], colors[type][i][1], colors[type][i][2]);
      liveColors[i].interp(radiusFraction, c, Colors::white);
   }

   Color deadColors[NumColors];
   for(S32 i = 0; i < NumColors; i++)
   {
      Color c(colors[3][i][0], colors[3][i][1], colors[3][i][2]);
      deadColors[i].interp(radiusFraction, c, Colors::white);
   }


   F32 arcTime = 0.5f + (1 - radiusFraction) * 0.5f;

   // Invert arcTime if we want to spiral outwards
   if(!spiralInwards)
      arcTime = -arcTime;

   // Width of the particle 'head'
   F32 beamWidth = 4;

   // Use F64 because if the value is too high, we lose resolution when we convert it to an F32 and
   // its animation gets "chunky"
   F64 time_f64 = (F64)time * 0.001;

   // Draw the Trackers
   for(U32 i = 0; i < MaxParticles; i++)
   {
      // Do some math first
      Tracker &t = particles[i];
      F32 d = F32((t.dP - fmod(t.dI + time_f64, (F64)t.dP)) / t.dP);
      F32 alphaMod = 1;
      if(d > 0.9)
         alphaMod = (1 - d) * 10;

      F32 theta = F32(fmod(t.thetaI + time_f64 * t.thetaP, DoubleTau));
      F32 startRadius = radiusFraction * radius * d;

      Point start(cos(theta), sin(theta));
      Point normal(-start.y, start.x);

      theta -= arcTime * t.thetaP * (alphaMod + 0.05f);
      d += arcTime / t.dP;
      if(d < 0)
         d = 0;
      Point end(cos(theta), sin(theta));

      F32 endRadius = radiusFraction * radius * d;

      F32 arcLength = (end * endRadius - start * startRadius).len();
      U32 vertexCount = (U32)(floor(arcLength / 10)) + 2;
      U32 arrayCount = 2 * (vertexCount + 1);

      // Static arrays to hold rendering data.  arrayCount is usually 8 - 20, so I chose
      // a buffer of 32 just in case
      static const S32 MAX_POINTS = 32;
      static F32 teleporterVertexArray[MAX_POINTS * 2];  // 2 coordinate components per item
      static F32 teleporterColorArray[MAX_POINTS * 4];  // 4 color components per item

      // Fill starting vertices
      Point p1 = start * (startRadius + beamWidth * 0.3f) + normal * 2;
      Point p2 = start * (startRadius - beamWidth * 0.3f) + normal * 2;

      teleporterVertexArray[0] = p1.x;
      teleporterVertexArray[1] = p1.y;
      teleporterVertexArray[2] = p2.x;
      teleporterVertexArray[3] = p2.y;

      // Fill starting colors
      Color *currentColor = NULL;  // dummy default
      if(i < trackerCount)
         currentColor = &liveColors[t.ci];
      else
      {
         Color c;
         c.interp(0.75f * F32(MaxParticles - i) / F32(MaxParticles - trackerCount), Colors::black, deadColors[t.ci]);
         currentColor = &c;
      }

      teleporterColorArray[0] = currentColor->r;
      teleporterColorArray[1] = currentColor->g;
      teleporterColorArray[2] = currentColor->b;
      teleporterColorArray[3] = alpha * alphaMod;
      teleporterColorArray[4] = currentColor->r;
      teleporterColorArray[5] = currentColor->g;
      teleporterColorArray[6] = currentColor->b;
      teleporterColorArray[7] = alpha * alphaMod;

      for(U32 j = 0; j <= vertexCount; j++)
      {
         F32 frac = j / F32(vertexCount);
         F32 width = beamWidth * (1 - frac) * 0.5f;
         Point p = start * (1 - frac) + end * frac;
         p.normalize();
         F32 rad = startRadius * (1 - frac) + endRadius * frac;

         // Fill vertices
         p1 = p * (rad + width);
         p2 = p * (rad - width);

         teleporterVertexArray[4*j]     = p1.x;
         teleporterVertexArray[(4*j)+1] = p1.y;
         teleporterVertexArray[(4*j)+2] = p2.x;
         teleporterVertexArray[(4*j)+3] = p2.y;

         // Fill colors
         if(i < trackerCount)
            currentColor = &liveColors[t.ci];
         else
         {
            Color c;
            c.interp(0.75f * F32(MaxParticles - i) / F32(MaxParticles - trackerCount), Colors::black, deadColors[t.ci]);
            currentColor = &c;
         }

         teleporterColorArray[8*j]     = currentColor->r;
         teleporterColorArray[(8*j)+1] = currentColor->g;
         teleporterColorArray[(8*j)+2] = currentColor->b;
         teleporterColorArray[(8*j)+3] = alpha * alphaMod * (1 - frac);
         teleporterColorArray[(8*j)+4] = currentColor->r;
         teleporterColorArray[(8*j)+5] = currentColor->g;
         teleporterColorArray[(8*j)+6] = currentColor->b;
         teleporterColorArray[(8*j)+7] = alpha * alphaMod * (1 - frac);
      }

      renderer.renderColored(teleporterVertexArray, teleporterColorArray, arrayCount, RenderType::TriangleStrip);
   }

   renderer.popMatrix();
}


void renderTeleporterOutline(const Point &center, F32 radius, const Color &color)
{
   Renderer& r = Renderer::get();

   r.setColor(color);
   r.setLineWidth(gLineWidth3);
   drawPolygon(center, 12, radius, 0);
   r.setLineWidth(gDefaultLineWidth);
}


// Render vertices of polyline; only used in the editor
void renderPolyLineVertices(BfObject *obj, bool snapping, F32 currentScale)
{
   // Draw the vertices of the wall or the polygon area
   S32 verts = obj->getVertCount();

   for(S32 j = 0; j < verts; j++)
   {
      if(obj->vertSelected(j))
         renderVertex(SelectedVertex,     obj->getVert(j), j, currentScale, 1);   // Hollow yellow boxes with number

      else if(obj->isLitUp() && obj->isVertexLitUp(j))
         renderVertex(HighlightedVertex,  obj->getVert(j), j, currentScale, 1);   // Hollow yellow boxes with number

      else if(obj->isSelected() || obj->isLitUp() || obj->anyVertsSelected())
         renderVertex(SelectedItemVertex, obj->getVert(j), j, currentScale, 1);   // Hollow red boxes with number

      else
         renderSmallSolidVertex(currentScale, obj->getVert(j), snapping);          // Tiny red or magenta dot
   }
}


void renderSpyBugVisibleRange(const Point &pos, const Color &color, F32 currentScale)
{
   renderFilledPolygon(pos, 6, SpyBug::SPY_BUG_RADIUS * currentScale, color * 0.45f);
}


void renderTurretFiringRange(const Point &pos, const Color &color, F32 currentScale)
{
   Renderer::get().setColor(color, 0.25f);    // Use transparency to highlight areas with more turret coverage

   F32 range = Turret::TurretPerceptionDistance * currentScale;

   drawRect(pos.x - range, pos.y - range, pos.x + range, pos.y + range, RenderType::TriangleFan);
}


void renderTurretIcon(const Point &pos, F32 scale, const Color *color)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
      r.translate(pos.x, pos.y, 0);
      r.scale(scale, scale, 1);    // Make item draw at constant size, regardless of zoom
      renderSquareItem(Point(0,0), color, 1, &Colors::white, 'T');
   r.popMatrix();
}


// Renders turret!  --> note that anchor and normal can't be const &Points because of the point math
void renderTurret(const Color &c, const Color &hbc, Point anchor, Point normal, bool enabled, F32 health, F32 barrelAngle, S32 healRate)
{
   Renderer& r = Renderer::get();
   static const F32 frontRadius = 15.f;

   Point cross(normal.y, -normal.x);
   Point aimCenter = anchor + normal * Turret::TURRET_OFFSET;

   // Render half-circle front
   Vector<Point> vertexArray;
   for(S32 x = -10; x <= 10; x++)
   {
      F32 theta = x * FloatHalfPi * 0.1f;
      Point pos = normal * cos(theta) + cross * sin(theta);
      vertexArray.push_back(aimCenter + pos * frontRadius);
   }

   r.setColor(c);

   r.renderPointVector(&vertexArray, RenderType::LineStrip);

   // Render symbol if it is a regenerating turret
   if(healRate > 0)
   {
      Vector<Point> pointArray;
      for(S32 x = -4; x <= 4; x++)
      {
         F32 theta = x * FloatHalfPi * 0.2f;
         Point pos = normal * cos(theta) + cross * sin(theta);
         pointArray.push_back(aimCenter + pos * frontRadius * 0.667f);
      }
      r.renderPointVector(&pointArray, RenderType::LineStrip);
   }

   // Render gun
   r.setLineWidth(gLineWidth3);

   Point aimDelta(cos(barrelAngle), sin(barrelAngle));
   Point aim1(aimCenter + aimDelta * frontRadius);
   Point aim2(aimCenter + aimDelta * frontRadius * 2);
   F32 vertices[] = {
         aim1.x, aim1.y,
         aim2.x, aim2.y
   };
   r.renderVertexArray(vertices, 2, RenderType::Lines);

   r.setLineWidth(gDefaultLineWidth);

   if(enabled)
      r.setColor(Colors::white);
   else
      r.setColor(0.6f);

   // Render base?
   Point corner1(anchor + cross * 18);
   Point corner2(anchor + cross * 18 + normal * Turret::TURRET_OFFSET);
   Point corner3(anchor - cross * 18 + normal * Turret::TURRET_OFFSET);
   Point corner4(anchor - cross * 18);
   F32 vertices2[] = {
         corner1.x, corner1.y,
         corner2.x, corner2.y,
         corner3.x, corner3.y,
         corner4.x, corner4.y
   };
   r.renderVertexArray(vertices2, 4, RenderType::LineLoop);

   // Render health bar
   r.setColor(hbc);

   renderHealthBar(health, anchor + normal * 7.5, cross, 28, 5);

   r.setColor(c);

   // Render something...
   Point lsegStart = anchor - cross * 14 + normal * 3;
   Point lsegEnd = anchor + cross * 14 + normal * 3;
   Point n = normal * (Turret::TURRET_OFFSET - 6);

   Point seg2start(lsegStart + n);
   Point seg2end(lsegEnd + n);
   F32 vertices3[] = {
         lsegStart.x, lsegStart.y,
         lsegEnd.x, lsegEnd.y,
         seg2start.x, seg2start.y,
         seg2end.x, seg2end.y
   };
   r.renderVertexArray(vertices3, 4, RenderType::Lines);
}


static void drawFlag(const Color *flagColor, const Color *mastColor, F32 alpha)
{
   Renderer& r = Renderer::get();
   r.setColor(*flagColor, alpha);

   // First, the flag itself
   static F32 flagPoints[] = { -15,-15, 15,-5,  15,-5, -15,5,  -15,-10, 10,-5,  10,-5, -15,0 };
   r.renderVertexArray(flagPoints, ARRAYSIZE(flagPoints) / 2, RenderType::Lines);

   // Now the flag's mast
   r.setColor(mastColor != NULL ? *mastColor : Colors::white, alpha);

   drawVertLine(-15, -15, 15);
}


void doRenderFlag(F32 x, F32 y, F32 scale, const Color *flagColor, const Color *mastColor, F32 alpha)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
      r.translate(x, y, 0);
      r.scale(scale);
      drawFlag(flagColor, mastColor, alpha);
   r.popMatrix();
}


void renderFlag(const Point &pos, const Color *flagColor, const Color *mastColor, F32 alpha)
{
   doRenderFlag(pos.x, pos.y, 1.0, flagColor, mastColor, alpha);
}


void renderFlag(const Point &pos, const Color *flagColor)
{
   doRenderFlag(pos.x, pos.y, 1.0, flagColor, NULL, 1);
}


void renderFlag(const Point &pos, F32 scale, const Color *flagColor)
{
   doRenderFlag(pos.x, pos.y, scale, flagColor, NULL, 1);
}


void renderFlag(F32 x, F32 y, const Color *flagColor)
{
   doRenderFlag(x, y, 1.0, flagColor, NULL, 1);
}


void renderFlag(const Color *flagColor)
{
   renderFlag(0, 0, flagColor);
}


void renderSmallFlag(const Point &pos, const Color &c, F32 parentAlpha)
{
   Renderer& r = Renderer::get();
   F32 alpha = 0.75f * parentAlpha;
   
   r.pushMatrix();
      r.translate(pos);
      r.scale(0.2f);

      F32 vertices[] = {
            -15, -15,
            15, -5,
            15, -5,
            -15, 5,
            -15, -15,
            -15, 15
      };
      F32 colors[] = {
            c.r, c.g, c.b, alpha,
            c.r, c.g, c.b, alpha,
            c.r, c.g, c.b, alpha,
            c.r, c.g, c.b, alpha,
            1, 1, 1, alpha,
            1, 1, 1, alpha
      };
      r.renderColored(vertices, colors, ARRAYSIZE(vertices) / 2, RenderType::Lines);
   r.popMatrix();
}


void renderFlagSpawn(const Point &pos, F32 currentScale, const Color *color)
{
   static const Point p(-4, 0);
   Renderer& r = Renderer::get();

   r.pushMatrix();
      r.translate(pos.x + 1, pos.y, 0);
      r.scale(0.4f / currentScale, 0.4f / currentScale, 1);
      renderFlag(color);

      drawCircle(p, 26, &Colors::white);
   r.popMatrix();
}


F32 renderCenteredString(const Point &pos, S32 size, const char *string)
{
   F32 width = getStringWidth((F32)size, string);
   drawStringAndGetWidth((S32)floor(pos.x - width * 0.5), (S32)floor(pos.y - size * 0.5), size, string);

   return width;
}


F32 renderCenteredString(const Point &pos, F32 size, const char *string)
{
   return renderCenteredString(pos, S32(size + 0.5f), string);
}


void renderPolygonLabel(const Point &centroid, F32 angle, F32 size, const char *text, F32 scaleFact)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
      r.scale(scaleFact);
      r.translate(centroid);
      r.rotate(angle * RADIANS_TO_DEGREES, 0, 0, 1);
      renderCenteredString(Point(0,0), size, text);
   r.popMatrix();
}


// Renders fill in the form of a series of points representing triangles
void renderTriangulatedPolygonFill(const Vector<Point> *fill)
{
   Renderer::get().renderPointVector(fill, RenderType::Triangles);
}


void renderPolygonOutline(const Vector<Point> *outline)
{
   Renderer::get().renderPointVector(outline, RenderType::LineLoop);
}


void renderPolygonOutline(const Vector<Point> *outlinePoints, const Color *outlineColor, F32 alpha, F32 lineThickness)
{
   Renderer& r = Renderer::get();

   r.setColor(*outlineColor, alpha);
   r.setLineWidth(lineThickness);

   renderPolygonOutline(outlinePoints);

   r.setLineWidth(gDefaultLineWidth);
}


void renderPolygonFill(const Vector<Point> *triangulatedFillPoints, const Color *fillColor, F32 alpha)      
{
   if(fillColor)
      Renderer::get().setColor(*fillColor, alpha);

   renderTriangulatedPolygonFill(triangulatedFillPoints);
}


void renderPolygon(const Vector<Point> *fillPoints, const Vector<Point> *outlinePoints, 
                   const Color *fillColor, const Color *outlineColor, F32 alpha)
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


void drawFilledStar(const Point &pos, S32 points, F32 radius, F32 innerRadius)
{
   Renderer& renderer = Renderer::get();

   F32 ang = FloatTau / F32(points * 2);
   F32 a = ang / 2;
   F32 r = innerRadius;
   bool inout = false;

   static Point p;
   static Point first;

   static Vector<Point> pts;
   static Vector<Point> core;
   static Vector<Point> outline;

   pts.clear();
   core.clear();
   outline.clear();

   for(S32 i = 0; i < points * 2; i++)
   {
      p.set(r * cos(a) + pos.x, r * sin(a) + pos.y);

      outline.push_back(p);

      if(i == 0)
      {
         first = p;
         core.push_back(p);
      }
      else if(i % 2 == 0)
      {
         pts.push_back(p);
         core.push_back(p);
      }

      pts.push_back(p);

      a += ang;
      inout = !inout;
      r = inout ? radius : innerRadius;
   }

   pts.push_back(first);

   renderer.renderPointVector(&pts, RenderType::Triangles);       // Points
   renderer.renderPointVector(&core, RenderType::TriangleFan);        // Inner pentagon
   renderer.renderPointVector(&outline, RenderType::LineLoop);   // Outline to make things look smoother, at least when star is small
}


void renderZone(const Color *outlineColor, const Vector<Point> *outline, const Vector<Point> *fill)
{
   Color fillColor = *outlineColor;
   fillColor *= 0.5;

   renderPolygon(fill, outline, &fillColor, outlineColor);
}


void renderLoadoutZone(const Color *color, const Vector<Point> *outline, const Vector<Point> *fill, 
                       const Point &centroid, F32 angle, F32 scaleFact)
{
   renderZone(color, outline, fill);
   renderLoadoutZoneIcon(centroid, 20, angle);
}


// In normal usage, outerRadius is 20, which is the default if no radius is specified
void renderLoadoutZoneIcon(const Point &center, S32 outerRadius, F32 angleRadians)
{
   // Pos, teeth, outer rad, inner rad, ang span of outer teeth, ang span of inner teeth, circle rad
   drawGear(center, 7, (F32)outerRadius, 0.75f * outerRadius, 20.0f, 18.0f, 0.4f * outerRadius, angleRadians);
}


void renderGoalZoneIcon(const Point &center, S32 radius, F32 angleRadians)
{
   Renderer& r = Renderer::get();
   static const F32 flagPoints[] = { -6, 10,  -6,-10,  12, -3.333f,  -6, 3.333f, };

   drawPolygon(center, 4, (F32)radius, 0.0f);

   r.pushMatrix();
      glTranslatef(center.x, center.y, 0);
      glRotatef(angleRadians * RADIANS_TO_DEGREES, 0, 0, 1);
      r.scale(radius * 0.041667f);  // 1 / 24 since we drew it to in-game radius of 24 (a ship's radius)
      r.renderVertexArray(flagPoints, ARRAYSIZE(flagPoints) / 2, RenderType::LineStrip);
   r.popMatrix();
}


void renderNavMeshZone(const Vector<Point> *outline, const Vector<Point> *fill, const Point &centroid,
                       S32 zoneId)
{
   renderPolygon(fill, outline, &Colors::green50, &Colors::green, 0.4f);

   char buf[6];  // Can't have more than 65535 zones
   dSprintf(buf, 24, "%d", zoneId);

   renderPolygonLabel(centroid, 0, 25, buf);
}


// Only used in-game
void renderNavMeshBorders(const Vector<NeighboringZone> &borders)
{
   Renderer& r = Renderer::get();

   r.setLineWidth(4.);
   r.setColor(Colors::cyan);

   // Go through each border and render
   for(S32 i = 0; i < borders.size(); i++)
   {
      F32 vertices[] = {
            borders[i].borderStart.x, borders[i].borderStart.y,
            borders[i].borderEnd.x,   borders[i].borderEnd.y,
      };
      r.renderVertexArray(vertices, 2, RenderType::Lines);
   }

   r.setLineWidth(gDefaultLineWidth);
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
                    bool isFlashing, F32 glowFraction, S32 score, F32 flashCounter)
{
   Color fillColor, outlineColor;

//      fillColor    = getGoalZoneFillColor(c, isFlashing, glowFraction);
//      outlineColor = getGoalZoneOutlineColor(c, isFlashing);

   // TODO: reconcile why using the above commentted out code doesn't work
   F32 alpha = isFlashing ? 0.75f : 0.5f;
   fillColor    = Color(Color(1,1,0) * (glowFraction * glowFraction) + Color(c) * alpha * (1 - glowFraction * glowFraction));
   outlineColor = Color(Color(1,1,0) * (glowFraction * glowFraction) + Color(c) *         (1 - glowFraction * glowFraction));

   renderPolygon(fill, outline, &fillColor, &outlineColor);
   renderGoalZoneIcon(centroid, 24);
}


static Color getNexusBaseColor(bool open, F32 glowFraction)
{
   Color color;

   if(open)
      color.interp(glowFraction, Colors::yellow, Colors::NexusOpenColor);
   else
      color = Colors::NexusClosedColor;

   return color;
}


void renderNexusIcon(const Point &center, S32 radius, F32 angleRadians)
{
   Renderer& r = Renderer::get();
   static const F32 root3div2 = 0.866f;  // sqrt(3) / 2

   static const F32 arcRadius = 14.f;
   static const F32 vertRadius = 6.f;
   static const F32 xRadius = 6.f;

   static const F32 spokes[] = {
         0, 0, 0, -xRadius,
         0, 0, xRadius * root3div2, xRadius * 0.5f,
         0, 0, -xRadius * root3div2, xRadius * 0.5f,
   };

   static const Point arcPoint1 = Point(0, -vertRadius);
   static const Point arcPoint2 = Point(vertRadius * root3div2, vertRadius * 0.5f);
   static const Point arcPoint3 = Point(-vertRadius * root3div2, vertRadius * 0.5f);

   r.pushMatrix();
      r.translate(center);
      r.scale(radius * 0.05f);  // 1/20.  Default radius is 20 in-game
      r.rotate(angleRadians * RADIANS_TO_DEGREES, 0, 0, 1);

      // Draw our center spokes
      r.renderVertexArray(spokes, ARRAYSIZE(spokes) / 2, RenderType::Lines);

      // Draw design
      drawArc(arcPoint1, arcRadius, 0.583f * FloatTau, FloatTau * 1.25f);
      drawArc(arcPoint2, arcRadius, 0.917f * FloatTau, 1.583f * FloatTau);
      drawArc(arcPoint3, arcRadius, 0.25f * FloatTau, 0.917f * FloatTau);
   r.popMatrix();
}


void renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, bool open, F32 glowFraction)
{
   Color baseColor = getNexusBaseColor(open, glowFraction);

   Color fillColor = Color(baseColor * (glowFraction * glowFraction  + (1 - glowFraction * glowFraction) * 0.5f));
   Color outlineColor = fillColor * 0.7f;

   renderPolygon(fill, outline, &fillColor, &outlineColor);
}


void renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, Point centroid, F32 labelAngle, bool open,
                 F32 glowFraction)
{
   renderNexus(outline, fill, open, glowFraction);

   Renderer::get().setColor(getNexusBaseColor(open, glowFraction));

   renderNexusIcon(centroid, 20, labelAngle);
}


void renderSlipZoneIcon(const Point &center, S32 radius, F32 angleRadians)
{
   Renderer& r = Renderer::get();

   static const F32 lines[] = {
         // Lines
         -16,  7,  -6,  7,
         -20,  2, -10,  2,
         -16, -3,  -6, -3,

         // Ship
         15.35f,   9.61f, 10.86f, -12.29f,
         10.86f, -12.29f, -3.97f,   4.44f,
         -3.97f,   4.44f, 15.35f,   9.61f,
   };


   r.pushMatrix();
      r.translate(center);
      r.scale(radius * 0.05f);  // 1/20.  Default radius is 20 in-game

      r.setColor(Colors::cyan);
      r.renderVertexArray(lines, ARRAYSIZE(lines) / 2, RenderType::Lines);
   r.popMatrix();
}


void renderSlipZone(const Vector<Point> *bounds, const Vector<Point> *boundsFill, const Point &centroid)     
{
   Renderer& r = Renderer::get();
   Color theColor (0, 0.5, 0);  // Go for a pale green, for now...

   r.setColor(theColor * 0.5);
   r.renderPointVector(boundsFill, RenderType::Triangles);

   r.setColor(theColor * 0.7f);
   r.renderPointVector(bounds, RenderType::LineLoop);

   renderSlipZoneIcon(centroid, 20);
}


void renderProjectile(const Point &pos, U32 style, U32 time)
{
   Renderer& r = Renderer::get();
   ProjectileInfo *pi = GameWeapon::projectileInfo + style;

   S32 bultype = 1;

   if(bultype == 1)    // Default stars
   { 
      r.setColor(pi->projColors[0]);
      r.pushMatrix();
         r.translate(pos);
         r.scale(pi->scaleFactor);

         r.pushMatrix();
            r.rotate((time % 720) * 0.5f, 0, 0, 1);

            static S16 projectilePoints1[] = { -2,2,  0,6,  2,2,  6,0,  2,-2,  0,-6,  -2,-2,  -6,0 };
            r.renderVertexArray(projectilePoints1, ARRAYSIZE(projectilePoints1) / 2, RenderType::LineLoop);

         r.popMatrix();

         r.rotate(180 - F32(time % 360), 0, 0, 1);
         r.setColor(pi->projColors[1]);

         static S16 projectilePoints2[] = { -2,2,  0,8,  2,2,  8,0,  2,-2,  0,-8, -2,-2,  -8,0 };
         r.renderVertexArray(projectilePoints2, ARRAYSIZE(projectilePoints2) / 2, RenderType::LineLoop);

      r.popMatrix();

   } else if (bultype == 2) { // Tiny squares rotating quickly, good machine gun

      r.setColor(pi->projColors[0]);
      r.pushMatrix();
      r.translate(pos);
      r.scale(pi->scaleFactor);

      r.pushMatrix();

         r.rotate(F32(time % 720), 0, 0, 1);
         r.setColor(pi->projColors[1]);

         static S16 projectilePoints3[] = { -2,2,  2,2,  2,-2,  -2,-2 };
         r.renderVertexArray(projectilePoints3, ARRAYSIZE(projectilePoints3) / 2, RenderType::LineLoop);
      r.popMatrix();

      r.popMatrix();

   } else if (bultype == 3) { // Rosette of circles

      const int innerR = 6;
      const int outerR = 3;
      const int dist = 10;

#define dr(x) degreesToRadians(x)
      r.pushMatrix();
         r.translate(pos);
         r.scale(pi->scaleFactor);

         r.pushMatrix();
            r.rotate( fmod(F32(time) * .15f, 720.f), 0, 0, 1);
            r.setColor(pi->projColors[1]);

            Point p(0,0);
            drawCircle(p, innerR);
            p.set(0,-dist);

            drawCircle(p, outerR);
            p.set(0,-dist);
            drawCircle(p, outerR);
            p.set(cos(dr(30)), -sin(dr(30)));
            drawCircle(p*dist, outerR);
            p.set(cos(dr(30)), sin(dr(30)));
            drawCircle(p * dist, outerR);
            p.set(0, dist);
            drawCircle(p, outerR);
            p.set(-cos(dr(30)), sin(dr(30)));
            drawCircle(p*dist, outerR);
            p.set(-cos(dr(30)), -sin(dr(30)));
            drawCircle(p*dist, outerR);
         r.popMatrix();
      r.popMatrix();
   }
}


void renderProjectileRailgun(const Point &pos, const Point &velocity, U32 time)
{
   Renderer& r = Renderer::get();
   ProjectileInfo *pi = GameWeapon::projectileInfo + ProjectileStyleRailgun;

   r.setColor(pi->projColors[0]);
   r.pushMatrix();
      r.translate(pos);
      r.scale(pi->scaleFactor);
      r.pushMatrix();
         F32 angle = velocity.ATAN2() * 360.f * FloatInverse2Pi;
         r.rotate(angle, 0, 0, 1);

         // Outer polygon
         r.setColor(pi->projColors[0]);
         static S16 outer[] = { -1,1,  2,1,  4,0,  2,-1,  -1,-1 };
         r.renderVertexArray(outer, ARRAYSIZE(outer) / 2, RenderType::LineLoop);

         // Stripe
         r.setColor(pi->projColors[1]);
         static F32 inner[] = { 0,0,  2.5,0 };
         r.renderVertexArray(inner, ARRAYSIZE(inner) / 2, RenderType::LineStrip);
      r.popMatrix();
   r.popMatrix();
}


void renderSeeker(const Point &pos, U32 style, F32 angleRadians, F32 speed, U32 timeRemaining)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
      r.translate(pos);
      r.rotate(angleRadians * 360.f / FloatTau, 0, 0, 1.0);

      // The flames first!
      F32 updateTime = (timeRemaining % 256)/ 512.0f; // every ~ 1/4 second increases from 0 to 0.5
      F32 speedRatio = speed / WeaponInfo::getWeaponInfo(WeaponSeeker).projVelocity + updateTime;
      r.setColor(Colors::yellow, 0.5f);
      F32 innerFlame[] = {
            -8, -1,
            -8 - (4 * speedRatio), 0,
            -8, 1,
      };
      r.renderVertexArray(innerFlame, 3, RenderType::LineStrip);
      r.setColor(Colors::orange50, 0.6f);
      F32 outerFlame[] = {
            -8, -3,
            -8 - (8 * speedRatio), 0,
            -8, 3,
      };
      r.renderVertexArray(outerFlame, 3, RenderType::LineStrip);

      // The body of the seeker
      Color bodyColor = Color(1, 0, 0.35f);
      if(style == SeekerStyleTurret)
         bodyColor = Color(updateTime, 1, updateTime); // Different colors of green

      r.setColor(bodyColor, 1);  // A redder magenta
      F32 vertices[] = {
            -8, -4,
            -8,  4,
             8,  0
      };
      r.renderVertexArray(vertices, 3, RenderType::LineLoop);
   r.popMatrix();
}



void renderMine(const Point &pos, bool armed, bool visible)
{
   Renderer& r = Renderer::get();

   F32 mod;
   F32 vis;   

   if(visible)    // Friendly mine
   {
      r.setColor(Colors::gray50);
      drawCircle(pos, Mine::SensorRadius);
      mod = 0.8f;
      vis = 1.0f;
   }
   else           // Invisible enemy mine
   {
      r.setLineWidth(gLineWidth1);
      mod = 0.80f;
      vis = 0.18f;
   }

   r.setColor(mod, mod, mod, vis);
   drawCircle(pos, 10);

   if(armed)
   {
      r.setColor(mod, 0, 0, vis);
      drawCircle(pos, 6);
   }
   r.setLineWidth(gDefaultLineWidth);
}

#ifndef min
#  define min(a,b) ((a) <= (b) ? (a) : (b))
#  define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


// lifeLeft is a number between 0 and 1.  Burst explodes when lifeLeft == 0.
void renderGrenade(const Point &pos, U32 style, F32 lifeLeft)
{
   Renderer& r = Renderer::get();

   r.setColor(Colors::white);
   drawCircle(pos, 10);

   bool innerVis = true;

   // When in doubt, write it out!
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

   if(style == 1)
      r.setColor(0, 1, min(1.f - lifeLeft, 0.5));
   else
      r.setColor(1, min(1.25f - lifeLeft, 1), 0);

   if(innerVis)
      drawFilledCircle(pos, 6);
   else
      drawCircle(pos, 6);
}

void renderFilledPolygon(const Point &pos, S32 points, S32 radius, const Color &fillColor, const Color &outlineColor)
{
   Vector<Point> pts(points);
   Vector<Point> fill;
   calcPolygonVerts(pos, points, radius, 0, pts);

   Triangulate::Process(pts, fill);

   renderPolygon(&fill, &pts, &fillColor, &outlineColor);
}


void renderFilledPolygon(const Point &pos, S32 points, S32 radius, const Color &fillColor)
{
   Vector<Point> pts(points);
   Vector<Point> fill;
   calcPolygonVerts(pos, points, radius, 0, pts);

   Triangulate::Process(pts, fill);

   renderPolygonFill(&fill, &fillColor);
}


void renderSpyBug(const Point &pos, const Color &teamColor, bool visible)
{
   Renderer& r = Renderer::get();

   if(visible)
   {
      renderFilledPolygon(pos, 6, 15, teamColor * 0.45f, Colors::gray50);

      FontManager::pushFontContext(OldSkoolContext);
      drawString(pos.x - 3, pos.y - 5, 10, "S");
      FontManager::popFontContext();
   }
   else
   {
      r.setLineWidth(gLineWidth1);
      r.setColor(0.25f);
      drawPolygon(pos, 6, 5, 0);
   }

   r.setLineWidth(gDefaultLineWidth);
}


void renderRepairItem(const Point &pos)
{
   renderRepairItem(pos, false, 0, 1);
}


void renderRepairItem(const Point &pos, bool forEditor, const Color *overrideColor, F32 alpha)
{
   Renderer& r = Renderer::get();

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

   r.pushMatrix();
   r.translate(pos);

   r.setColor(overrideColor == NULL ? Colors::white : *overrideColor, alpha);
   drawHollowSquare(Point(0,0), size);

   r.setColor(overrideColor == NULL ? Colors::red : *overrideColor, alpha);
   F32 vertices[] = {
          crossWidth,  crossWidth,
          crossLen,    crossWidth,
          crossLen,   -crossWidth,
          crossWidth, -crossWidth,
          crossWidth, -crossLen,
         -crossWidth, -crossLen,
         -crossWidth, -crossWidth,
         -crossLen,   -crossWidth,
         -crossLen,    crossWidth,
         -crossWidth,  crossWidth,
         -crossWidth,  crossLen,
          crossWidth,  crossLen
   };
   r.renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, RenderType::LineLoop);

   r.popMatrix();
}


void renderEnergySymbol(const Point &pos, F32 scaleFactor)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
      r.translate(pos);
      r.scale(scaleFactor);
      renderEnergySymbol();
   r.popMatrix();
}


// Render the actual lightning bolt glyph at 0,0
void renderEnergySymbol()
{
   Renderer& r = Renderer::get();

   // Yellow lightning bolt
   r.setColor(Colors::orange67, 1);

   static S16 energySymbolPoints[] = { 20,-20,  3,-2,  12,5,  -20,20,  -2,3,  -12,-5 };
   r.renderVertexArray(energySymbolPoints, ARRAYSIZE(energySymbolPoints) / 2, RenderType::LineLoop);
}


void renderEnergyItem(const Point &pos, bool forEditor)
{
   Renderer& r = Renderer::get();
   F32 scaleFactor = 1;    // Resize for editor
   F32 size = 18;

   if(forEditor)
   {
      scaleFactor = 0.45;
      size = 8;
   }

   r.pushMatrix();
      r.translate(pos);

      drawHollowSquare(Point(0,0), size, &Colors::white);
      r.setLineWidth(gDefaultLineWidth);

      // Scale down the symbol a little so it fits in the box
      r.scale(scaleFactor * 0.7f);
      renderEnergySymbol();

   r.popMatrix();
}


void renderEnergyItem(const Point &pos)
{
   renderEnergyItem(pos, false);
}


// Use faster method with no offset
void renderWallFill(const Vector<Point> *points, const Color &fillColor, bool polyWall)
{
   Renderer& r = Renderer::get();
   r.setColor(fillColor);
   r.renderPointVector(points, polyWall ? RenderType::Triangles : RenderType::TriangleFan);
}


// Use slower method if each point needs to be offset
void renderWallFill(const Vector<Point> *points, const Color &fillColor, const Point &offset, bool polyWall)
{
   Renderer& r = Renderer::get();
   r.setColor(fillColor);
   r.renderPointVector(points, offset, polyWall ? RenderType::Triangles : RenderType::TriangleFan);
}


// Used in both editor and game
void renderWallEdges(const Vector<Point> &edges, const Color &outlineColor, F32 alpha)
{
   Renderer& r = Renderer::get();
   r.setColor(outlineColor, alpha);
   r.renderPointVector(&edges, RenderType::Lines);
}


// Used in editor only
void renderWallEdges(const Vector<Point> &edges, const Point &offset, const Color &outlineColor, F32 alpha)
{
   Renderer& r = Renderer::get();
   r.setColor(outlineColor, alpha);
   r.renderPointVector(&edges, offset, RenderType::Lines);
}


void renderSpeedZone(const Vector<Point> &points, U32 time)
{
   Renderer& r = Renderer::get();
   r.setColor(Colors::red);

   for(S32 j = 0; j < 2; j++)
   {
      S32 start = j * points.size() / 2;    // GoFast comes in two equal shapes

      glEnableClientState(GL_VERTEX_ARRAY);

      glVertexPointer(2, GL_FLOAT, sizeof(Point), points.address());    
      glDrawArrays(GL_LINE_LOOP, start, points.size() / 2);

      glDisableClientState(GL_VERTEX_ARRAY);
   }
}


void renderTestItem(const Point &pos, S32 size, F32 alpha)
{
   Vector<Point> pts;
   calcPolygonVerts(pos, TestItem::TEST_ITEM_SIDES, (F32)size, 0, pts);
   renderTestItem(pts, alpha);
}


void renderTestItem(const Vector<Point> &points, F32 alpha)
{
   Renderer& r = Renderer::get();
   r.setColor(Colors::yellow, alpha);
   r.renderVertexArray((F32 *)&points[0], points.size(), RenderType::LineLoop);
}


void renderAsteroid(const Point &pos, S32 design, F32 scaleFact, const Color *color, F32 alpha)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
   r.translate(pos);
   r.scale(scaleFact * ASTEROID_SCALING_FACTOR);

   if(color == NULL)
      r.setColor(Color(.7), alpha);  // Default gray
   else
      r.setColor(*color, alpha);     // Team color

   const F32 *vertexArray = AsteroidCoords[design];
   r.renderVertexArray(vertexArray, ASTEROID_POINTS, RenderType::LineLoop);

   r.popMatrix();
}


void renderAsteroidForTeam(const Point &pos, S32 design, F32 scaleFact, const Color *color, F32 alpha)
{
   Renderer& r = Renderer::get();

   // Render internal colored part, scaled a little smaller
   r.setLineWidth(gLineWidth4);
   renderAsteroid(pos, design, 0.95 * scaleFact, color, alpha);
   r.setLineWidth(gDefaultLineWidth);

   // Render standard outline
   renderAsteroid(pos, design, scaleFact, NULL, alpha);
}


void renderAsteroidSpawn(const Point &pos, S32 time, const Color* color)
{
   static const S32 period = 4096;  // Power of 2 please
   static const F32 invPeriod = 1 / F32(period);

   F32 alpha = max(0.0f, 0.8f - time * invPeriod);

   renderAsteroid(pos, 2, 9.0f, color, alpha);
}


void renderAsteroidSpawnEditor(const Point &pos, const Color* color, F32 scale)
{
   Renderer& r = Renderer::get();
   scale *= 0.8f;
   static const Point p(0,0);

   r.pushMatrix();
      r.translate(pos.x, pos.y, 0);
      r.scale(scale, scale, 1);
      renderAsteroid(p, 2, 9.f, color, 0.8);

      drawCircle(p, 13, &Colors::white);
   r.popMatrix();
}


void renderResourceItem(const Vector<Point> &points, F32 alpha)
{
   Renderer& r = Renderer::get();
   r.setColor(Colors::white, alpha);
   r.renderVertexArray((F32 *)&points[0], points.size(), RenderType::LineLoop);
}


void renderSoccerBall(const Point &pos, F32 size)
{
   Renderer::get().setColor(Colors::white);
   drawCircle(pos, size);
}


void renderCore(const Point &pos, const Color *coreColor, const Color &hbc, U32 time, 
                PanelGeom *panelGeom, F32 panelHealth[], F32 panelStartingHealth)
{
   Renderer& r = Renderer::get();

   // Draw outer polygon and inner circle
   Color baseColor = Colors::gray80;

   Point dir;   // Reusable container
   
   for(S32 i = 0; i < CORE_PANELS; i++)
   {
      dir = (panelGeom->repair[i] - pos);
      dir.normalize();
      Point cross(dir.y, -dir.x);

      r.setColor(hbc);
      renderHealthBar(panelHealth[i] / panelStartingHealth, panelGeom->repair[i], dir, 30, 7);

      if(panelHealth[i] == 0)          // Panel is dead
      {
         Color c = *coreColor;
         r.setColor(c * 0.2f);
      }
      else
         r.setColor(baseColor);

      F32 vertices[] = {
            panelGeom->getStart(i).x, panelGeom->getStart(i).y,
            panelGeom->getEnd(i).x, panelGeom->getEnd(i).y
      };
      r.renderVertexArray(vertices, 2, RenderType::Lines);

      // Draw health stakes
      if(panelHealth[i] > 0)
      {
         F32 vertices2[] = {
               panelGeom->repair[i].x, panelGeom->repair[i].y,
               pos.x, pos.y
         };

         F32 colors[] = {
               0.2f, 0.2f, 0.2f, 1,    // Colors::gray20
               0,    0,    0,    1,    // Colors::black
         };

         r.renderColored(vertices2, colors, 2, RenderType::Lines);
      }
   }

   F32 atomSize = 40;
   F32 angle = CoreItem::getCoreAngle(time);

   // Draw atom graphic
   F32 t = FloatTau + (F32(time & 1023) / 1024.f * FloatTau);
   for(F32 rotate = 0; rotate < FloatTau - 0.01f; rotate += FloatTau / 5)  //  0.01f part avoids rounding error
   {
      // 32 vertices and colors
      F32 vertexArray[64];
      F32 colorArray[128];
      U32 count = 0;
      for(F32 theta = 0; theta < FloatTau; theta += CIRCLE_SIDE_THETA)
      {
         F32 x = cos(theta + rotate * 2 + t) * atomSize * 0.5f;
         F32 y = sin(theta + rotate * 2 + t) * atomSize;

         vertexArray[2*count]     = pos.x + cos(rotate + angle) * x + sin(rotate + angle) * y;
         vertexArray[(2*count)+1] = pos.y + sin(rotate + angle) * x - cos(rotate + angle) * y;
         colorArray[4*count]      = coreColor->r;
         colorArray[(4*count)+1]  = coreColor->g;
         colorArray[(4*count)+2]  = coreColor->b;
         colorArray[(4*count)+3]  = theta / FloatTau;
         count++;
      }
      r.renderColored(vertexArray, colorArray, ARRAYSIZE(vertexArray)/2, RenderType::LineLoop);
   }

   r.setColor(baseColor);
   drawCircle(pos, atomSize + 2);
}


// Here we render a simpler, non-animated Core to reduce distraction
void renderCoreSimple(const Point &pos, const Color *coreColor, S32 width)
{
   Renderer& r = Renderer::get();

   // Here we render a simpler, non-animated Core to reduce distraction in the editor
   r.setColor(Colors::white);
   drawPolygon(pos, 10, (F32)width / 2, 0);

   r.setColor(*coreColor);
   drawCircle(pos, width / 5.0f);
}


void renderSoccerBall(const Point &pos)
{
   renderSoccerBall(pos, (F32)SoccerBallItem::Radius);
}


void renderTextItem(const Point &pos, const Point &dir, F32 size, const string &text, const Color *color)
{
   Renderer& r = Renderer::get();

   if(text == "Bitfighter")
   {
      r.setColor(Colors::green);
      // All factors in here determined experimentally, seem to work at a variety of sizes and approximate the width and height
      // of ordinary text in cases tested.  What should happen is the Bitfighter logo should, as closely as possible, match the 
      // size and extent of the text "Bitfighter".
      F32 scaleFactor = size / 129.0f;
      r.pushMatrix();
      r.translate(pos);
         r.scale(scaleFactor);
         glRotatef(pos.angleTo(dir) * RADIANS_TO_DEGREES, 0, 0, 1);
         glTranslatef(-119, -45, 0);      // Determined experimentally

         renderBitfighterLogo(0, 1);
      r.popMatrix();

      return;
   }

   r.setColor(*color);
   FontManager::pushFontContext(OldSkoolContext);
   drawAngleString(pos.x, pos.y, size, pos.angleTo(dir), text.c_str());
   FontManager::popFontContext();
}


// Only used by instructions... in-game uses the other signature
void renderForceFieldProjector(const Point &pos, const Point &normal, const Color *color, bool enabled, S32 healRate)
{
   Vector<Point> geom = ForceFieldProjector::getForceFieldProjectorGeometry(pos, normal);
   renderForceFieldProjector(&geom, pos, color, enabled, 1.0, healRate);
}


void renderForceFieldProjector(const Vector<Point> *geom, const Point &pos, const Color *color, bool enabled, F32 health, S32 healRate)
{
   Renderer& r = Renderer::get();
   F32 ForceFieldBrightnessProjector = 0.50;

   Color c(color);      // Create locally modifiable copy

   c = c * (1 - ForceFieldBrightnessProjector) + ForceFieldBrightnessProjector;

   if (enabled)
      r.setColor(c, 0.2f + (0.9f * health)); //adjust alpha a little so it doesn't get much darker when its enabled than disabled
   else
      r.setColor((c * 0.6f));

   // Draw a symbol in the project to show it is a regenerative projector
   if(healRate > 0)
   {
      // Point 0 is the where the forcefield comes out
      // Use a point partly along the line from the position to FF (not midpoint, doesn't look so good)
      Point centerPoint = ((geom->get(0) - pos) * 0.333f) + pos;
      F32 angle = pos.angleTo(geom->get(0));

      static const F32 symbol[] = {
            -2,  5,
             4,  0,
            -2, -5,
      };

      r.pushMatrix();
         r.translate(centerPoint);
         glRotatef(angle * RADIANS_TO_DEGREES, 0, 0, 1);
         r.renderVertexArray(symbol, ARRAYSIZE(symbol) / 2, RenderType::LineStrip);
      r.popMatrix();
   }
   
   r.renderPointVector(geom, RenderType::LineLoop);
}


void renderForceField(const Point &start, const Point &end, const Color *color, bool fieldUp, F32 health, U32 time)
{
   Renderer& r = Renderer::get();
   Vector<Point> geom = ForceField::computeGeom(start, end);

   const F32 ForceFieldBrightness = 0.25;

   Color c(color);
   c = c * (1 - ForceFieldBrightness) + ForceFieldBrightness;

   if(!fieldUp)
      c = c * 0.5;

   // Set color of field
   r.setColor(c);

   // This is a long, thin rectangle
   r.renderPointVector(&geom, RenderType::LineLoop);


   // Add health bar on top

   // Chosen to be the size of a ship
   const F32 healthBarPeriod = 50;

   // Scale animation speed based on health
   F32 timeScale = 4 * (1-health);

   U32 timePeriod = 4096;  // power of 2 in milliseconds
   U32 animationTime = U32(timeScale * time) & (timePeriod-1);
   F32 normAnimationTime = animationTime / F32(timePeriod);

   drawDashedLine(start, end, healthBarPeriod, health, normAnimationTime);  // Health is the duty cycle
}


struct pixLoc
{
   S16 x;
   S16 y;
};

const S32 LetterLoc1 = 25;   // I (1st)
const S32 LetterLoc2 = 25;   // I (2nd)
const S32 LetterLoc3 = 71;   // G
const S32 LetterLoc4 = 40;   // T (1st)
const S32 LetterLoc5 = 62;   // H
const S32 LetterLoc6 = 39;   // T (2nd)
const S32 LetterLoc7 = 54;   // F
const S32 LetterLoc8 = 60;   // E
const S32 LetterLoc9 = 30;   // B outline
const S32 LetterLoc10 = 21;  // B top hole
const S32 LetterLoc11 = 21;  // B bottom hole
const S32 LetterLoc12 = 46;  // R outline
const S32 LetterLoc13 = 22;  // R hole
const S32 LogoLetterCount = LetterLoc1 + LetterLoc2 + LetterLoc3 + LetterLoc4 + LetterLoc5 + LetterLoc6 + LetterLoc7 +
                            LetterLoc8 + LetterLoc9 + LetterLoc10 + LetterLoc11 + LetterLoc12 + LetterLoc13;

S16 gLogoPoints[LogoLetterCount][2] =
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
   Renderer::get().setColor(Colors::green);
   renderBitfighterLogo(73, 1);
   FontManager::pushFontContext(ReleaseVersionContext);
   drawCenteredString(120, 10, ZAP_GAME_RELEASE_LONGSTRING);  // The compiler combines both strings
   FontManager::popFontContext();
}


// Render logo at 0,0
//
// mask
//  1   = I (1st)
//  2   = I (2nd)
//  4   = G
//  8   = T (1st)
//  16  = H
//  32  = T (2nd)
//  64  = F
//  128 = E
//  256 = B
//  512 = R
void renderBitfighterLogo(U32 mask)
{
   Renderer& r = Renderer::get();

   S16* points = &gLogoPoints[0][0];
   U32 pos = 0;

   if (mask & 1 << 0)
      r.renderVertexArray(points, LetterLoc1, RenderType::LineLoop, 0, 0);
   pos += LetterLoc1;
   if (mask & 1 << 1)
      r.renderVertexArray(points, LetterLoc2, RenderType::LineLoop, pos, 0);
   pos += LetterLoc2;
   if (mask & 1 << 2)
      r.renderVertexArray(points, LetterLoc3, RenderType::LineLoop, pos, 0);
   pos += LetterLoc3;
   if (mask & 1 << 3)
      r.renderVertexArray(points, LetterLoc4, RenderType::LineLoop, pos, 0);
   pos += LetterLoc4;
   if (mask & 1 << 4)
      r.renderVertexArray(points, LetterLoc5, RenderType::LineLoop, pos, 0);
   pos += LetterLoc5;
   if (mask & 1 << 5)
      r.renderVertexArray(points, LetterLoc6, RenderType::LineLoop, pos, 0);
   pos += LetterLoc6;
   if (mask & 1 << 6)
      r.renderVertexArray(points, LetterLoc7, RenderType::LineLoop, pos, 0);
   pos += LetterLoc7;
   if (mask & 1 << 7)
      r.renderVertexArray(points, LetterLoc8, RenderType::LineLoop, pos, 0);
   pos += LetterLoc8;

   if (mask & 1 << 8)
      r.renderVertexArray(points, LetterLoc9, RenderType::LineLoop, pos, 0);
   pos += LetterLoc9;
   if (mask & 1 << 8)
      r.renderVertexArray(points, LetterLoc10, RenderType::LineLoop, pos, 0);
   pos += LetterLoc10;
   if (mask & 1 << 8)
      r.renderVertexArray(points, LetterLoc11, RenderType::LineLoop, pos, 0);
   pos += LetterLoc11;

   if (mask & 1 << 9)
      r.renderVertexArray(points, LetterLoc12, RenderType::LineLoop, pos, 0);
   pos += LetterLoc12;
   if (mask & 1 << 9)
      r.renderVertexArray(points, LetterLoc13, RenderType::LineLoop, pos, 0);
}


// Draw logo centered on screen horzontally, and on yPos vertically, scaled and rotated according to parameters
void renderBitfighterLogo(S32 yPos, F32 scale, U32 mask)
{
   Renderer& r = Renderer::get();
   const F32 fact = 0.15f * scale;   // Scaling factor to make the coordinates below fit nicely on the screen (derived by trial and error)
   
   // 3609 is the diff btwn the min and max x coords below, 594 is same for y

   r.pushMatrix();
      r.translate((DisplayManager::getScreenInfo()->getGameCanvasWidth() - 3609 * fact) / 2, yPos - 594 * fact / 2, 0);
      r.scale(fact);                   // Scale it down...
      renderBitfighterLogo(mask);
   r.popMatrix();
}


void renderBitfighterLogo(const Point &pos, F32 size, U32 letterMask)
{
   Renderer& r = Renderer::get();
   const F32 sizeToLogoRatio = 0.0013f;  // Shot in the dark!

   r.pushMatrix();
      r.translate(pos);
      r.scale(sizeToLogoRatio * size);
      renderBitfighterLogo(letterMask);
   r.popMatrix();
}


// Pos is the square's center
void drawSquare(const Point &pos, F32 radius, bool filled)
{
   drawRect(pos.x - radius, pos.y - radius, pos.x + radius, pos.y + radius, filled ? RenderType::TriangleFan : RenderType::LineLoop);
}


void drawSquare(const Point &pos, S32 radius, bool filled)
{
    drawSquare(pos, F32(radius), filled);
}


void drawHollowSquare(const Point &pos, F32 radius, const Color *color)
{
   if(color)
      Renderer::get().setColor(*color);

   drawSquare(pos, radius, false);
}


void drawFilledSquare(const Point &pos, F32 radius, const Color *color)
{
   if(color)
      Renderer::get().setColor(*color);

    drawSquare(pos, radius, true);
}


void drawFilledSquare(const Point &pos, S32 radius, const Color *color)
{
   if(color)
      Renderer::get().setColor(*color);

   drawSquare(pos, F32(radius), true);
}


// Red vertices in walls, and magenta snapping vertices
void renderSmallSolidVertex(F32 currentScale, const Point &pos, bool snapping)
{
   F32 size = MIN(MAX(currentScale, 1), 2);              // currentScale, but limited to range 1-2
   Renderer::get().setColor(snapping ? Colors::magenta : Colors::red);

   drawFilledSquare(pos, (F32)size / currentScale);
}


void renderVertex(char style, const Point &v, S32 number)
{
   renderVertex(style, v, number, EditorObject::VERTEX_SIZE, 1, 1);
}


void renderVertex(char style, const Point &v, S32 number, F32 scale)
{
   renderVertex(style, v, number, EditorObject::VERTEX_SIZE, scale, 1);
}


void renderVertex(char style, const Point &v, S32 number, F32 scale, F32 alpha)
{
   renderVertex(style, v, number, EditorObject::VERTEX_SIZE, scale, alpha);
}


void renderVertex(char style, const Point &v, S32 number, S32 size, F32 scale, F32 alpha)
{
   Renderer& r = Renderer::get();
   bool hollow = style == HighlightedVertex || style == SelectedVertex || style == SelectedItemVertex || style == SnappingVertex;

   // Fill the box with a dark gray to make the number easier to read
   if(hollow && number != -1)
   {
      r.setColor(.25f);
      drawFilledSquare(v, size / scale);
   }
      
   if(style == HighlightedVertex)
      r.setColor(Colors::EDITOR_HIGHLIGHT_COLOR, alpha);
   else if(style == SelectedVertex)
      r.setColor(Colors::EDITOR_SELECT_COLOR, alpha);
   else if(style == SnappingVertex)
      r.setColor(Colors::magenta, alpha);
   else     // SelectedItemVertex
      r.setColor(Colors::red, alpha);     

   drawSquare(v, (F32)size / scale, !hollow);

   if(number != NO_NUMBER)     // Draw vertex numbers
   {
      r.setColor(Colors::white, alpha);
      F32 fontsize = 6 / scale;
      FontManager::pushFontContext(OldSkoolContext);
      drawStringf(v.x - getStringWidthf(fontsize, "%d", number) / 2, v.y - 3 / scale, fontsize, "%d", number);
      FontManager::popFontContext();
   }
}


void renderLine(const Vector<Point> *points, const Color *color)
{
   Renderer& r = Renderer::get();

   if(color)
      r.setColor(*color);

   r.renderPointVector(points, RenderType::LineStrip);
}


void drawFadingHorizontalLine(S32 x1, S32 x2, S32 yPos, const Color &color)
{
   F32 vertices[] = {
         (F32)x1, (F32)yPos,
         (F32)x2, (F32)yPos
   };

   F32 colors[] = {
         color.r, color.g, color.b, 1,
         color.r, color.g, color.b, 0,
   };

   Renderer::get().renderColored(vertices, colors, ARRAYSIZE(vertices) / 2, RenderType::Lines);
}


void drawLetter(char letter, const Point &pos, const Color &color, F32 alpha)
{
   // Mark the item with a letter, unless we're showing the reference ship
   S32 vertOffset = 8;
   if (isAlpha(letter))    // Better position lowercase letters
      vertOffset = 10;

   Renderer::get().setColor(color, alpha);

   FontManager::pushFontContext(OldSkoolContext);
   F32 xpos = pos.x - getStringWidthf(15, "%c", letter) / 2;
   drawStringf(xpos, pos.y - vertOffset, 15, "%c", letter);
   FontManager::popFontContext();
}


void renderSquareItem(const Point &pos, const Color *c, F32 alpha, const Color *letterColor, char letter)
{
   Renderer::get().setColor(*c, alpha);
   drawFilledSquare(pos, 8);  // Draw filled box in which we'll put our letter
   drawLetter(letter, pos, *letterColor, alpha);
}


// Faster circle algorithm adapted from:  http://slabode.exofire.net/circle_draw.shtml
void drawCircle(F32 x, F32 y, F32 radius, const Color *color, F32 alpha)
{
   Renderer& r = Renderer::get();

   if(color)
      r.setColor(*color, alpha);

   F32 theta = Float2Pi * INV_NUM_CIRCLE_SIDES; // 1/32

   // Precalculate the sine and cosine
   F32 cosTheta = cosf(theta);
   F32 sinTheta = sinf(theta);

   F32 curX = radius;  // We start at angle = 0
   F32 curY = 0;
   F32 prevX;

   // 32 vertices is almost a circle..  right?
   F32 vertexArray[2 * NUM_CIRCLE_SIDES];

   // This is a repeated rotation
   for(S32 i = 0; i < NUM_CIRCLE_SIDES; i++)
   {
      vertexArray[(2*i)]     = curX + x;
      vertexArray[(2*i) + 1] = curY + y;

      // Apply the rotation matrix
      prevX = curX;
      curX = (cosTheta * curX)  - (sinTheta * curY);
      curY = (sinTheta * prevX) + (cosTheta * curY);
   }

   r.renderVertexArray(vertexArray, 32, RenderType::LineLoop);
}


void drawCircle(const Point &pos, S32 radius, const Color *color, F32 alpha)
{
   drawCircle(pos.x, pos.y, (F32)radius, color, alpha);
}


void drawCircle(const Point &pos, F32 radius, const Color *color, F32 alpha)
{
   drawCircle(pos.x, pos.y, radius, color, alpha);
}


void drawDivetedTriangle(F32 height, F32 len) 
{
   Renderer& r = Renderer::get();
   static const F32 t30 = tan(degreesToRadians(30.0f));
   static const F32 t60 = tan(degreesToRadians(60.0f));
   static const F32 c30 = cos(degreesToRadians(30.0f));
   static const F32 c60 = cos(degreesToRadians(60.0f));
   static const F32 s60 = sin(degreesToRadians(60.0f));

   const F32 h3 = height / 3;
   const F32 ht30 = height * t30;
   const F32 lc30 = len * c30;

   Vector<Point> pts;
   pts.push_back(Point(0, 2 * h3));   // A
   pts.push_back(Point(-.5 * len, 2 * h3 - lc30));   // B
   pts.push_back(Point(-.5 * len + 2 * (ht30 - len) * c60, 2 * h3 - lc30 - 2 * (ht30 - len) * s60));   // C
   pts.push_back(Point(-ht30 + .5 * len, -h3 + lc30));   // D
   pts.push_back(Point(-ht30, -h3));   // E
   pts.push_back(Point(-ht30 + len, -h3));   // F
   pts.push_back(Point(0, -h3 + (ht30 - len) * t60));   // G
   pts.push_back(Point(ht30 - len, -h3));   // H
   pts.push_back(Point(ht30, -h3));   // I
   pts.push_back(Point(ht30 - .5 * len, -h3 + lc30));   // J
   pts.push_back(Point(.5 * len - 2 * (ht30 - len) * c60, 2 * h3 - lc30 - 2 * (ht30 - len) * s60));   // K
   pts.push_back(Point(.5 * len, 2 * h3 - lc30));   // L

   
   r.pushMatrix();
      r.translate(200, 200, 0);
      r.rotate(GLfloat(Platform::getRealMilliseconds() / 10 % 360), 0, 0, 1);
      r.scale(6);

      renderPolygonOutline(&pts, &Colors::red);

   r.popMatrix();
}


void drawGear(const Point &center, S32 teeth, F32 radius1, F32 radius2, F32 ang1, F32 ang2, F32 innerCircleRadius, F32 angleRadians)
{
   Renderer& r = Renderer::get();
   static Vector<Point> pts;
   pts.clear();

   F32 ang1rad = degreesToRadians(ang1);
   F32 ang2rad = degreesToRadians(ang2);

   F32 a = Float2Pi / teeth;
   F32 theta  = -ang1rad / 2;    // Start a little rotated to get an outer tooth facing up!

   for(S32 i = 0; i < teeth; i++)
   {
      pts.push_back(Point(-radius1 * sin(theta), radius1 * cos(theta)));
      theta += ang1rad;

      pts.push_back(Point(-radius1 * sin(theta), radius1 * cos(theta)));
      theta += a / 2 - (ang1rad / 2 + ang2rad / 2);

      pts.push_back(Point(-radius2 * sin(theta), radius2 * cos(theta)));
      theta += ang2rad;

      pts.push_back(Point(-radius2 * sin(theta), radius2 * cos(theta)));
      theta += a / 2 - (ang1rad / 2 + ang2rad / 2);
   }

   r.pushMatrix();
      r.translate(center.x, center.y, 0);
      r.rotate(angleRadians * RADIANS_TO_DEGREES, 0, 0, 1);

      renderPolygonOutline(&pts);

      drawCircle(0, 0, innerCircleRadius);

   r.popMatrix();
}


void render25FlagsBadge(F32 x, F32 y, F32 rad)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
      r.translate(x, y, 0);
      r.scale(.05f * rad);
      r.setColor(Colors::gray40);
      drawEllipse(Point(-16, 15), 6, 2, 0);

      renderFlag(-.10f * rad, -.10f * rad, &Colors::red50);
   r.popMatrix();

   F32 ts = rad - 3;
   F32 width = getStringWidth(ts, "25");
   F32 tx = x + .30f * rad;
   F32 ty = y + rad - .40f * rad;

   r.setColor(Colors::yellow);
   drawFilledRect(F32(tx - width / 2.0 - 1.0), F32(ty - (ts + 2.0) / 2.0), 
                  F32(tx + width / 2.0 + 0.5), F32(ty + (ts + 2.0) / 2.0));

   r.setColor(Colors::gray20);
   renderCenteredString(Point(tx, ty), ts, "25");
}


void renderDeveloperBadge(F32 x, F32 y, F32 rad)
{
   Renderer& r = Renderer::get();
   F32 r3  = rad * 0.333f;
   F32 r23 = rad * 0.666f;

   // Render cells
   r.setColor(Colors::green80);

   drawSquare(Point(x,       y - r23), r3, true);
   drawSquare(Point(x + r23, y      ), r3, true);
   drawSquare(Point(x - r23, y + r23), r3, true);
   drawSquare(Point(x,       y + r23), r3, true);
   drawSquare(Point(x + r23, y + r23), r3, true);

   // Render grid atop our cells
   r.setColor(Colors::gray20);

   r.setLineWidth(1);
   
   drawHorizLine(x - rad, x + rad, y - rad);
   drawHorizLine(x - rad, x + rad, y - r3);
   drawHorizLine(x - rad, x + rad, y + r3);
   drawHorizLine(x - rad, x + rad, y + rad);

   drawVertLine(x - rad, y - rad, y + rad);
   drawVertLine(x - r3,  y - rad, y + rad);
   drawVertLine(x + r3,  y - rad, y + rad);
   drawVertLine(x + rad, y - rad, y + rad);

   r.setLineWidth(gDefaultLineWidth);
}


void renderBBBBadge(F32 x, F32 y, F32 rad, const Color &color)
{
   Renderer::get().setColor(color, 1.0);
   renderBitfighterLogo(Point(x - (rad * 0.5f), y - (rad * 0.666f)), 2 * rad, 256);  // Draw the 'B' only
}


void renderLevelDesignWinnerBadge(F32 x, F32 y, F32 rad)
{
   F32 rm2 = rad - 2;

   Vector<Point> edges;
   edges.push_back(Point(x - rm2, y - rm2));
   edges.push_back(Point(x - rm2, y + rm2));
   edges.push_back(Point(x + rm2, y + rm2));
   edges.push_back(Point(x + rm2, y - rm2));

   renderWallFill(&edges, Colors::wallFillColor, false);
   renderPolygonOutline(&edges, &Colors::blue);
   Renderer::get().setColor(Colors::white);
   renderCenteredString(Point(x, y), rad, "1");
}


void renderZoneControllerBadge(F32 x, F32 y, F32 rad)
{
   F32 rm2 = rad - 2;

   Vector<Point> points;
   points.push_back(Point(x - rm2, y + rm2));
   points.push_back(Point(x + rm2, y + rm2));
   points.push_back(Point(x + rm2, y - rm2));

   Vector<Point> points2;
   points2.push_back(Point(x - rm2, y - rm2));
   points2.push_back(Point(x - rm2, y + rm2));
   points2.push_back(Point(x + rm2, y - rm2));

   renderPolygon(&points, &points, &Colors::gray40, &Colors::gray80, 1.0);
   renderPolygon(&points2, &points2, &Colors::blue40, &Colors::blue80, 1.0);

   Renderer::get().setColor(Colors::white);
   renderCenteredString(Point(x, y), rad * 0.9f, "ZC");
}


void renderRagingRabidRabbitBadge(F32 x, F32 y, F32 rad)
{
   static const F32 rabbit[] = {
         -3.70f, -6.46f,
         -3.71f, -7.78f,
         -3.13f, -8.28f,
         -2.18f, -8.37f,
          0.10f, -5.04f,
          1.44f, -1.37f,
          3.77f, -1.08f,
          8.37f,  3.77f,
          8.15f,  4.91f,
          5.58f,  7.23f,
          4.48f,  7.72f,
          5.0f,    9.0f,
         -4.0f,    8.0f,
         -3.68f,  3.93f,
         -4.14f,  0.36f,
         -7.13f, -3.14f,
         -7.90f, -7.62f,
         -7.63f, -7.95f,
         -6.76f, -8.32f,
         -4.50f, -7.19f,
   };

   static const F32 eye[] = {
         1.18f, 0.68f,
         3.18f, 1.68f,
         3.18f, 2.68f,
         2.00f, 2.88f,
         0.78f, 1.20f,
   };

   Renderer& r = Renderer::get();

   r.pushMatrix();
      r.translate(x, y, 0);
      r.scale(.1f * rad);

      r.setColor(Colors::gray50);
      r.renderVertexArray(rabbit, ARRAYSIZE(rabbit) / 2, RenderType::LineLoop);

      r.setColor(Colors::red);
      r.renderVertexArray(eye, ARRAYSIZE(eye) / 2, RenderType::LineLoop);
   r.popMatrix();
}


void renderLastSecondWinBadge(F32 x, F32 y, F32 rad)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
      r.translate(x, y, 0);
      r.scale(.05f * rad);

      renderFlag(-.10f * rad, -.10f * rad, &Colors::blue);
   r.popMatrix();

   F32 tx = x + .40f * rad;
   F32 ty = y + .40f * rad;


   r.setColor(Colors::white);
   renderCenteredString(Point(tx, ty), rad, ":01");
}


void renderHatTrickBadge(F32 x, F32 y, F32 rad)
{
   static const F32 outline[] = {
         7.13f,  3.96f,
         5.84f,  4.35f,
         1.86f,  4.86f,
        -4.59f,  4.58f,
        -6.81f,  3.85f,
        -6.64f, -0.04f,
        -6.05f, -2.69f,
        -4.2f,  -4.7f,
        -1.14f, -4.69f,
         2.77f, -2.61f,
         5.66f,  0.50f,
   };

   static const F32 brim[] = {
         -8.27f, 6.44f,
         -7.12f, 7.38f,
         -3.07f, 8.06f,
          2.98f, 8.17f,
          8.03f, 7.10f,
          8.87f, 5.87f,
          8.53f, 3.57f,
          5.84f, 4.35f,
          1.86f, 4.86f,
         -4.59f, 4.58f,
         -8.41f, 3.35f,
   };

   static const F32 pompom[] = {
         -4.60f, -9.16f,
         -5.89f, -4.33f,

         -7.66f, -6.10f,
         -2.83f, -7.39f,

         -7.01f, -8.51f,
         -3.48f, -4.98f,
   };

   static const F32 stripe[] = {
          -6.62f,  0.54f,
          -3.45f,  1.96f,
           2.11f,  1.84f,
           5.66f,  0.50f,
   };

   Renderer& r = Renderer::get();

   // Waldo?
   r.pushMatrix();
      r.translate(x, y, 0);
      r.scale(.1f * rad);

      // GL_TRIANGLE_FAN, then GL_LINE_LOOP for anti-aliasing
      r.setColor(Colors::red);
      r.renderVertexArray(brim, ARRAYSIZE(brim) / 2, RenderType::TriangleFan);
      r.renderVertexArray(brim, ARRAYSIZE(brim) / 2, RenderType::LineLoop);

      r.setColor(Colors::white);
      r.renderVertexArray(outline, ARRAYSIZE(outline) / 2, RenderType::TriangleFan);
      r.setLineWidth(gLineWidth1);
      r.renderVertexArray(outline, ARRAYSIZE(outline) / 2, RenderType::LineLoop);
      r.setLineWidth(gDefaultLineWidth);

      r.setColor(Colors::red);
      r.renderVertexArray(stripe, ARRAYSIZE(stripe) / 2, RenderType::LineStrip);

      r.setColor(Colors::red);
      r.renderVertexArray(pompom, ARRAYSIZE(pompom) / 2, RenderType::Lines);
   r.popMatrix();
}


void renderBadge(F32 x, F32 y, F32 rad, MeritBadges badge)
{
   switch(S32(badge))
   {
      case DEVELOPER_BADGE:
         renderDeveloperBadge(x, y, rad);
         break;
      case BADGE_TWENTY_FIVE_FLAGS:
         render25FlagsBadge(x, y, rad);
         break;
      case BADGE_BBB_GOLD:
         renderBBBBadge(x, y, rad, Colors::gold);
         break;
      case BADGE_BBB_SILVER:
         renderBBBBadge(x, y, rad, Colors::silver);
         break;
      case BADGE_BBB_BRONZE:
         renderBBBBadge(x, y, rad, Colors::bronze);
         break;
      case BADGE_BBB_PARTICIPATION:
         renderBBBBadge(x, y, rad, Colors::green);
         break;
      case BADGE_LEVEL_DESIGN_WINNER:
         renderLevelDesignWinnerBadge(x, y, rad);
         break;
      case BADGE_ZONE_CONTROLLER:
         renderZoneControllerBadge(x, y, rad);
         break;
      case BADGE_RAGING_RABID_RABBIT:
         renderRagingRabidRabbitBadge(x, y, rad);
         break;
      case BADGE_HAT_TRICK:
         renderHatTrickBadge(x, y, rad);
         break;
      case BADGE_LAST_SECOND_WIN:
         renderLastSecondWinBadge(x, y, rad);
         break;
      default:
         TNLAssert(false, "Unknown Badge!");
         break;
   }
}


static void renderGridLines(const Point &offset, F32 gridScale, F32 grayVal, bool fadeLines)
{
   // Use F32 to avoid cumulative rounding errors
   F32 xStart = fmod(offset.x, gridScale);
   F32 yStart = fmod(offset.y, gridScale);

   Renderer::get().setColor(grayVal);

   while(yStart < DisplayManager::getScreenInfo()->getGameCanvasHeight())
   {
      drawHorizLine((F32)0, (F32)DisplayManager::getScreenInfo()->getGameCanvasWidth(), (F32)yStart);
      yStart += gridScale;
   }
   while(xStart < DisplayManager::getScreenInfo()->getGameCanvasWidth())
   {
      drawVertLine((F32)xStart, (F32)0, (F32)DisplayManager::getScreenInfo()->getGameCanvasHeight());
      xStart += gridScale;
   }
}


// Render background snap grid for the editor
void renderGrid(F32 currentScale, const Point &offset, const Point &origin, F32 gridSize, bool fadeLines, bool showMinorGridLines)
{
   Renderer& r = Renderer::get();
   F32 snapFadeFact = fadeLines ? 1 : 0.5f;

   // Gridlines
   // Render 2 layers:
   //  1. layer 1 - minor gridlines (only if set)
   //  2. layer 0 - major gridlines

   // First minor lines
   if(showMinorGridLines)
   {
      F32 gridScale = currentScale * gridSize * 0.1f;
      F32 grayVal = snapFadeFact * 0.2f;

      renderGridLines(offset, gridScale, grayVal, fadeLines);
   }


   // Now major lines
   F32 gridScale = currentScale * gridSize;
   F32 grayVal = snapFadeFact * 0.4f;

   // If we're zoomed out enough, don't draw so many lines
   if(!showMinorGridLines)
   {
      while(gridScale < 0.05f * gridSize)
         gridScale *= 20;
   }

   renderGridLines(offset, gridScale, grayVal, fadeLines);


   // Draw axes
   r.setColor(0.7f * snapFadeFact);
   r.setLineWidth(gLineWidth3);

   drawHorizLine((F32)0, (F32)DisplayManager::getScreenInfo()->getGameCanvasWidth(), (F32)origin.y);
   drawVertLine((F32)origin.x, (F32)0, (F32)DisplayManager::getScreenInfo()->getGameCanvasHeight());

   r.setLineWidth(gDefaultLineWidth);
}


void renderStars(const Point *stars, const Color *colors, S32 numStars, F32 alpha, Point cameraPos, Point visibleExtent)
{
   Renderer& r = Renderer::get();
   if (alpha <= 0.5f)
      return;

   static const F32 starChunkSize = 1024;        // Smaller numbers = more dense stars
   static const F32 starDist = 3500;             // Bigger value = slower moving stars
   const F32* starVerts = reinterpret_cast<const F32*>(stars); // Sad

   Point upperLeft  = cameraPos - visibleExtent * 0.5f;  // UL corner of screen in "world" coords
   Point lowerRight = cameraPos + visibleExtent * 0.5f;  // LR corner of screen in "world" coords

   // When zooming out to commander's view, visibleExtent will grow larger and larger.  At some point, drawing all the stars
   // needed to fill the zoomed out screen becomes overwhelming, and bogs the computer down.  So we really need to set some
   // rational limit on where we stop showing stars during the zoom process (recall that stars are hidden when we are done zooming,
   // so this effect should be transparent to the user except at the most extreme of scales, and then, the alternative is slowing 
   // the computer greatly).  Note that 10000 is probably irrationally high.
   if(visibleExtent.x > 10000 || visibleExtent.y > 10000) 
      return;

   upperLeft  *= 1 / starChunkSize;
   lowerRight *= 1 / starChunkSize;

   upperLeft.x = floor(upperLeft.x);      // Round to ints, slightly enlarging the corners to ensure
   upperLeft.y = floor(upperLeft.y);      // the entire screen is "covered" by the bounding box

   lowerRight.x = floor(lowerRight.x) + 0.5f;
   lowerRight.y = floor(lowerRight.y) + 0.5f;

   // Do some calculations for the parallax
   S32 xDist = (S32) (cameraPos.x / starDist);
   S32 yDist = (S32) (cameraPos.y / starDist);

   S32 fx1 = -1 - xDist;
   S32 fx2 =  1 - xDist;
   S32 fy1 = -1 - yDist;
   S32 fy2 =  1 - yDist;

   r.setPointSize(gLineWidth1);
   r.setColor(Colors::gray60, alpha);
   glEnable(GL_BLEND);

   // The (F32(xPage + 1.f) == xPage) part becomes true, which could cause endless loop problem freezing game when way too far off the center.
   for(F32 xPage = upperLeft.x + fx1; xPage < lowerRight.x + fx2 && !(F32(xPage + 1.f) == xPage); xPage++)
      for(F32 yPage = upperLeft.y + fy1; yPage < lowerRight.y + fy2 && !(F32(yPage + 1.f) == yPage); yPage++)
      {
         r.pushMatrix();
            r.scale(starChunkSize);   // Creates points with coords btwn 0 and starChunkSize
            r.translate(xPage + (cameraPos.x / starDist), yPage + (cameraPos.y / starDist), 0);
            r.renderVertexArray(starVerts, numStars, RenderType::Points);
         r.popMatrix();
      }

   glDisableClientState(GL_VERTEX_ARRAY);
}


void renderWalls(const GridDatabase *wallSegmentDatabase, const Vector<Point> &wallEdgePoints, 
                 const Vector<Point> &selectedWallEdgePoints, const Color &outlineColor, 
                 const Color &fillColor, F32 currentScale, bool dragMode, bool drawSelected,
                 const Point &selectedItemOffset, bool previewMode, bool showSnapVertices, F32 alpha)
{
   bool moved = (selectedItemOffset.x != 0 || selectedItemOffset.y != 0);
   S32 count = wallSegmentDatabase->getObjectCount();

   if(!drawSelected)    // Essentially pass 1, drawn earlier in the process
   {
      // Render walls that have been moved first (i.e. render their shadows)
      if(moved)
      {
         for(S32 i = 0; i < count; i++)
         {
            WallSegment *wallSegment = static_cast<WallSegment *>(wallSegmentDatabase->getObjectByIndex(i));
            if(wallSegment->isSelected())     
               wallSegment->renderFill(Point(0,0), Color(.1));
         }
      }

      // hack for now
      Color color;
      if(alpha < 1)
         color = Colors::gray67;
      else
         color = fillColor * alpha;

      for(S32 i = 0; i < count; i++)
      {
         WallSegment *wallSegment = static_cast<WallSegment *>(wallSegmentDatabase->getObjectByIndex(i));
         if(!moved || !wallSegment->isSelected())         
            wallSegment->renderFill(selectedItemOffset, color);      // RenderFill ignores offset for unselected walls
      }

      renderWallEdges(wallEdgePoints, outlineColor);                 // Render wall outlines with unselected walls
   }
   else  // Render selected/moving walls last so they appear on top; this is pass 2, 
   {
      for(S32 i = 0; i < count; i++)
      {
         WallSegment *wallSegment = static_cast<WallSegment *>(wallSegmentDatabase->getObjectByIndex(i));
         if(wallSegment->isSelected())  
            wallSegment->renderFill(selectedItemOffset, fillColor * alpha);
      }

      // Render wall outlines for selected walls only
      renderWallEdges(selectedWallEdgePoints, selectedItemOffset, outlineColor);
   }

   if(showSnapVertices)
   {
      Renderer& r = Renderer::get();
      r.setLineWidth(gLineWidth1);

      //glColor(Colors::magenta);
      for(S32 i = 0; i < wallEdgePoints.size(); i++)
         renderSmallSolidVertex(currentScale, wallEdgePoints[i], dragMode);

      r.setLineWidth(gDefaultLineWidth);
   }
}


void renderWallOutline(WallItem *wallItem, const Vector<Point> *outline, const Color *color, 
                       F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices)
{
   Renderer& r = Renderer::get();
   if(color)
      r.setColor(*color);

   r.renderPointVector(outline, RenderType::LineStrip);

   if(renderVertices)
      renderPolyLineVertices(wallItem, snappingToWallCornersEnabled, currentScale);
}


void drawObjectiveArrow(const Point &nearestPoint, F32 zoomFraction, const Color *outlineColor, 
                        S32 canvasWidth, S32 canvasHeight, F32 alphaMod, F32 highlightAlpha)
{
   Renderer& renderer = Renderer::get();
   Point center(canvasWidth / 2, canvasHeight / 2);
   Point arrowDir = nearestPoint - center;

   F32 er = arrowDir.x * arrowDir.x / (350 * 350) + arrowDir.y * arrowDir.y / (250 * 250);
   if(er < 1)
      return;

   Point rp = nearestPoint;
   Point np = nearestPoint;

   er = sqrt(er);
   rp.x = arrowDir.x / er;
   rp.y = arrowDir.y / er;
   rp += center;

   F32 dist = (np - rp).len();

   arrowDir.normalize();
   Point crossVec(arrowDir.y, -arrowDir.x);

   // Fade the arrows as we transition to/from commander's map
   F32 alpha = (1 - zoomFraction) * 0.6f * alphaMod;
   if(alpha == 0)
      return;

   // Make indicator fade as we approach the target
   if(dist < 50)
      alpha *= dist * 0.02f;

   // Scale arrow accorging to distance from objective --> doesn't look very nice
   //F32 scale = max(1 - (min(max(dist,100),1000) - 100) / 900, .5);
   F32 scale = 1.0;

   Point p2 = rp - arrowDir * 23 * scale + crossVec * 8 * scale;
   Point p3 = rp - arrowDir * 23 * scale - crossVec * 8 * scale;

   Color fillColor = *outlineColor;    // Create local copy
   fillColor *= .7f;

   static Point vertices[3];     // Reusable array

   vertices[0].set(rp.x, rp.y);
   vertices[1].set(p2.x, p2.y);
   vertices[2].set(p3.x, p3.y);

   // This loops twice: once to render the objective arrow, once to render the outline
   for(S32 i = 0; i < 2; i++)
   {
      renderer.setColor(i == 1 ? fillColor : *outlineColor, alpha);
      renderer.renderVertexArray((F32 *)(vertices), ARRAYSIZE(vertices), i == 1 ? RenderType::TriangleFan : RenderType::LineLoop);
   }

   // Highlight the objective arrow, if need be.  This will be called rarely, so efficiency here is 
   // not as important as above
   if(highlightAlpha > 0)
   {
      Vector<Point> inPoly(vertices, ARRAYSIZE(vertices));
      Vector<Point> outPoly;
      offsetPolygon(&inPoly, outPoly, HIGHLIGHTED_OBJECT_BUFFER_WIDTH);

      renderer.setColor(Colors::HelpItemRenderColor, highlightAlpha * alpha);
      renderer.renderVertexArray((F32 *)(outPoly.address()), outPoly.size(), RenderType::LineLoop);
   }

   //   Point cen = rp - arrowDir * 12;

   // Try labelling the objective arrows... kind of lame.
   //drawStringf(cen.x - UserInterface::getStringWidthf(10,"%2.1f", dist/100) / 2, cen.y - 5, 10, "%2.1f", dist/100);

   // Add an icon to the objective arrow...  kind of lame.
   //renderSmallFlag(cen, c, alpha);
}


void renderScoreboardOrnamentTeamFlags(S32 xpos, S32 ypos, const Color *color, bool teamHasFlag)
{
   Renderer& r = Renderer::get();

   r.pushMatrix();
   r.translate(F32(xpos), F32(ypos + 15), 0);
   r.scale(.75);
   renderFlag(color);
   r.popMatrix();

   // Add an indicator for the team that has the flag
   if(teamHasFlag)
   {
      r.setColor(Colors::magenta);
      drawString(xpos - 23, ypos + 7, 18, "*");      // These numbers are empirical alignment factors
   }
}


void renderSpawn(const Point &pos, F32 scale, const Color *color)
{   
   Renderer& r = Renderer::get();

   r.pushMatrix();
      glTranslatef(pos.x, pos.y, 0);
      glScalef(scale, scale, 1);    // Make item draw at constant size, regardless of zoom
      renderSquareItem(Point(0,0), color, 1, &Colors::white, 'S');
   r.popMatrix();
}


// For debugging bots -- renders the path the bot will take from its current location to its destination.
// Note that the bot may cut corners as it goes.
void renderFlightPlan(const Point &from, const Point &to, const Vector<Point> &flightPlan)
{
   Renderer& r = Renderer::get();
   r.setColor(Colors::red);

   // Render from ship to start of flight plan
   Vector<Point> line(2);
   line.push_back(from);
   line.push_back(to);
   r.renderPointVector(&line, RenderType::LineStrip);

   // Render the flight plan itself
   r.renderPointVector(&flightPlan, RenderType::LineStrip);
}


// Used by SimpleLineItem to draw the chunky arrow that represents the item in the editor
void renderHeavysetArrow(const Point &pos, const Point &dest, const Color &color, bool isSelected, bool isLitUp)
{
   Renderer& r = Renderer::get();

   for(S32 i = 1; i >= 0; i--)
   {
      // Draw heavy colored line with colored core
      r.setLineWidth(i ? gLineWidth4 : gDefaultLineWidth);                
      r.setColor(color, i ? .35f : 1);         // Get color from child class

      F32 ang = pos.angleTo(dest);
      const F32 al = 15;                // Length of arrow-head, in editor units (15 pixels)
      const F32 angoff = .5;            // Pitch of arrow-head prongs

      // Draw arrowhead
      F32 vertices[] = {
            dest.x, dest.y,
            dest.x - cos(ang + angoff) * al, dest.y - sin(ang + angoff) * al,
            dest.x, dest.y,
            dest.x - cos(ang - angoff) * al, dest.y - sin(ang - angoff) * al
      };
      r.renderVertexArray(vertices, 4, RenderType::Lines);

      // Draw highlighted core on 2nd pass if item is selected, but not while it's being edited
      if(!i && (isSelected || isLitUp))
         r.setColor(isSelected ? Colors::EDITOR_SELECT_COLOR : Colors::EDITOR_HIGHLIGHT_COLOR);

      F32 vertices2[] = {
            pos.x, pos.y,
            dest.x, dest.y
      };

      r.renderVertexArray(vertices2, 2, RenderType::Lines);
   }
}


void renderTeleporterEditorObject(const Point &pos, S32 radius, const Color &color)
{
   Renderer& r = Renderer::get();
   r.setColor(color);

   r.setLineWidth(gLineWidth3);
   drawPolygon(pos, 12, (F32)radius, 0);
   r.setLineWidth(gDefaultLineWidth);
}


}

