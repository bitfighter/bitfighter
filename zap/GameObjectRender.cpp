//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GameObjectRender.h"

#include "BotNavMeshZone.h"      // For Border def
#include "CoreGame.h"            // For CORE_PANELS
#include "DisplayManager.h"
#include "EngineeredItem.h"      // For TURRET_OFFSET
#include "FontManager.h"
#include "projectile.h"
#include "soccerGame.h"
#include "Teleporter.h"          // For TELEPORTER_RADIUS
#include "UI.h"                  // For margins only
#include "version.h"
#include "VertexStylesEnum.h"
#include "WallItem.h"
#include "PolyWall.h"
#include "ship.h"

#include "Colors.h"

#include "stringUtils.h"
#include "RenderUtils.h"
#include "MathUtils.h"           // For converting radians to degrees, sq()
#include "GeomUtils.h"

#include "tnlRandom.h"

#ifdef SHOW_SERVER_SITUATION
#  include "GameManager.h"
#endif

#include <cmath>

namespace Zap
{


// Statics
F32 GameObjectRender::DEFAULT_LINE_WIDTH = RenderUtils::DEFAULT_LINE_WIDTH;

GameObjectRender::GameObjectRender()
{
   // Do nothing
}


GameObjectRender::~GameObjectRender()
{
   // Do nothing
}


// Health ranges from 0 to 1
// Center is the centerpoint of the health bar; normal is a normalized vector along the main axis of the bar
// length and width are in pixels, and are the dimensions of the health bar
void GameObjectRender::renderHealthBar(F32 health, const Point &center, const Point &dir, F32 length, F32 width, const Color &color, F32 alpha)
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

   RenderUtils::drawLines(&vertexArray, color, alpha);
}


void GameObjectRender::renderActiveModuleOverlays(F32 alpha, F32 radius, U32 sensorTime, bool shieldActive,
                                       bool sensorActive, bool repairActive, bool hasArmor)
{
   // Armor
   if(hasArmor)
   {
      nvgStrokeWidth(nvg, RenderUtils::LINE_WIDTH_3);
      RenderUtils::drawPolygon(5, 30, FloatHalfPi, Colors::yellow, alpha);
      nvgStrokeWidth(nvg, RenderUtils::DEFAULT_LINE_WIDTH);
   }

   // Shields
   if(shieldActive)
   {
      F32 shieldRadius = radius + 3;      // radius is the ship radius
      RenderUtils::drawCircle(shieldRadius, Colors::yellow, alpha);
   }

#ifdef SHOW_SERVER_SITUATION
   ServerGame *serverGame = GameManager::getServerGame();

   if(serverGame && static_cast<Ship *>(serverGame->getClientInfo(0)->getConnection()->getControlObject()) &&
      static_cast<Ship *>(serverGame->getClientInfo(0)->getConnection()->getControlObject())->isModulePrimaryActive(ModuleShield))
   {
      F32 shieldRadius = radius;

      drawCircle(shieldRadius, Colors::green, alpha);
   }
#endif


   // Sensor
   if(sensorActive)
   {
      F32 rad = (sensorTime & 0x1FF) * 0.002f;    // Radius changes over time
      RenderUtils::drawCircle(rad * Ship::CollisionRadius + 4, Colors::white, alpha);
   }

   // Repair
   if(repairActive)
   {
      nvgStrokeWidth(nvg, RenderUtils::LINE_WIDTH_3);

      RenderUtils::drawCircle(18, Colors::red, alpha);

      nvgStrokeWidth(nvg, RenderUtils::DEFAULT_LINE_WIDTH);
   }
}


void GameObjectRender::renderShipFlame(ShipFlame *flames, S32 flameCount, F32 thrust, F32 alpha, bool yThruster)
{
   for(S32 i = 0; i < flameCount; i++)
      for(S32 j = 0; j < flames[i].layerCount; j++)
      {
         ShipFlameLayer *flameLayer = &flames[i].layers[j];

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
         RenderUtils::drawLineStrip(vertices, 3, flameLayer->color, alpha);
      }
}


void GameObjectRender::renderShip(ShipShape::ShipShapeType shapeType, const Color &shipColor, F32 alpha,
                F32 thrusts[], F32 health, F32 radius, U32 sensorTime,
                bool shieldActive, bool sensorActive, bool repairActive, bool hasArmor)
{
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
   RenderUtils::drawLines(shipShapeInfo->flamePortPoints, shipShapeInfo->flamePortPointCount, Colors::gray50, alpha);

   // Inner hull with colored insides
   for(S32 i = 0; i < shipShapeInfo->innerHullPieceCount; i++)
      RenderUtils::drawLineStrip(shipShapeInfo->innerHullPieces[i].points, shipShapeInfo->innerHullPieces[i].pointCount, shipColor, alpha);

   // Render health bar
   renderHealthBar(health, Point(0,1.5), Point(0,1), 28, 4, shipColor, alpha);

   // Grey outer hull drawn last, on top
   RenderUtils::drawLineLoop(shipShapeInfo->outerHullPoints, shipShapeInfo->outerHullPointCount, Colors::gray70, alpha);

   // Now render any module states
   renderActiveModuleOverlays(alpha, radius, sensorTime, shieldActive, sensorActive, repairActive, hasArmor);
}


// Figure out the intensity of the thrusters based on the ship's actions and orientation
void calcThrustComponents(const Point &velocity, F32 angle, F32 deltaAngle, bool boostActive, F32 *thrusts)
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
void GameObjectRender::renderGamesPlayedMark(S32 x, S32 y, S32 height, U32 gamesPlayed)
{
   S32 sym = Platform::getRealMilliseconds() / 2000 % 9;

   //FontManager::pushFontContext(FPSContext);

//   mGL->glLineWidth(RenderUtils::LINE_WIDTH_1);
//
//
//   if(sym > 0)
//   {
//      S32 cenx = x - height/2 - 3;
//      S32 ceny = y - height/4 - 3;
//
//      drawHollowSquare(Point(cenx, ceny), height/2);
//
//      RenderUtils::drawStringc(Point(cenx, ceny + height/2 - 3), height - 4, itos(sym).c_str());
//   }


   //// Four square
   //S32 passedHeight = height;
   //height = (height + 3) & ~0x03;      // Round height up to next multiple of 4

   //// Adjust x and y to be center of rank icon
   //y -= height / 2 - (height - passedHeight) / 2 - 1;
   //x -= height / 2 + 4;    // 4 provides a margin

   //F32 rad = height / 4.0f;
   //S32 yoffset = 0;     // Tricky way to vertically align the squares when there are only 1 or 2

//   mGL->glLineWidth(RenderUtils::DEFAULT_LINE_WIDTH);

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
   //mGL->glPushMatrix();
   //// Changed position on me?
   //x = x - 8;
   //y = y - 6;
   //switch(sym)
   //{
   //   case 0:
   //      mGL->glColor(Colors::bronze);
   //      mGL->glTranslate(x, y + 1, 0);
   //      mGL->renderVertexArray(chevronPoints, 4, GLOPT::Lines);
   //      break;
   //   case 1:
   //      mGL->glColor(Colors::bronze);
   //      mGL->glTranslate(x, y - 2, 0);
   //      mGL->renderVertexArray(chevronPoints, 8, GLOPT::Lines);
   //      break;
   //   case 2:
   //      mGL->glColor(Colors::bronze);
   //      mGL->glTranslate(x, y - 5, 0);
   //      mGL->renderVertexArray(chevronPoints, 12, GLOPT::Lines);
   //      break;
   //   case 3:
   //      mGL->glColor(Colors::silver);
   //      mGL->glTranslate(x, y, 0);
   //      mGL->renderVertexArray(barPoints, 2, GLOPT::Lines);
   //      break;
   //   case 4:
   //      mGL->glColor(Colors::silver);
   //      mGL->glTranslate(x, y - 3, 0);
   //      mGL->renderVertexArray(barPoints, 4, GLOPT::Lines);
   //      break;
   //   case 5:
   //      mGL->glColor(Colors::silver);
   //      mGL->glTranslate(x, y - 6, 0);
   //      mGL->renderVertexArray(barPoints, 6, GLOPT::Lines);
   //      break;
   //   case 6:
   //      mGL->glLineWidth(RenderUtils::LINE_WIDTH_1);
   //      mGL->glColor(Colors::gold);
   //      drawStar(Point(x,y), 5, height / 2.0f, rad);
   //      mGL->glLineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
   //      break;
   //   case 7:
   //      mGL->glLineWidth(RenderUtils::LINE_WIDTH_1);
   //      mGL->glColor(Colors::gold);
   //      drawFilledStar(Point(x,y), 5, height / 2.0f, rad);
   //      mGL->glLineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
   //      break;
   //}
   //mGL->glPopMatrix();


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

   nvgSave(nvg);
   nvgTranslate(nvg, x - 10, y - 6);
   
   RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_1);

   RenderUtils::drawLineLoop(rectPoints,      4, Colors::gray20);
   RenderUtils::drawLineLoop(&rectPoints[8],  4, Colors::gray20);
   RenderUtils::drawLineLoop(&rectPoints[16], 4, Colors::gray20);
   RenderUtils::drawLineLoop(&rectPoints[24], 4, Colors::gray20);

   switch(sym)
   {
      case 0:
         //RenderUtils::drawFilledLineLoop(triPoints, 3, Colors::red);
         RenderUtils::drawLineLoop(rectPoints, 4, Colors::green80);
         break;
      case 1:
         RenderUtils::drawFilledLineLoop(rectPoints, 4, Colors::green80);
         RenderUtils::drawLineLoop(rectPoints, 4, Colors::green80);
         break;
      case 2:
         RenderUtils::drawFilledLineLoop(rectPoints, 4, Colors::green80);
         RenderUtils::drawLineLoop(rectPoints, 4, Colors::green80);
         //RenderUtils::drawFilledLineLoop(&triPoints[6], 3, Colors::red);
         RenderUtils::drawLineLoop(&rectPoints[8], 4, Colors::green80);
         break;
      case 3:
         RenderUtils::drawFilledLineLoop(rectPoints,     4, Colors::green80);
         RenderUtils::drawLineLoop(rectPoints,     4, Colors::green80);
         RenderUtils::drawFilledLineLoop(&rectPoints[8], 4, Colors::green80);
         RenderUtils::drawLineLoop(&rectPoints[8], 4, Colors::green80);
         break;
      case 4:
         RenderUtils::drawFilledLineLoop(rectPoints,      4, Colors::green80);
         RenderUtils::drawLineLoop(rectPoints,      4, Colors::green80);
         RenderUtils::drawFilledLineLoop(&rectPoints[8],  4, Colors::green80);
         RenderUtils::drawLineLoop(&rectPoints[8],  4, Colors::green80);
         //RenderUtils::drawFilledLineLoop(&triPoints[12], 3, Colors::red);
         RenderUtils::drawLineLoop(&rectPoints[16], 4, Colors::green80);
         break;
      case 5:
         RenderUtils::drawFilledLineLoop(rectPoints,      4, Colors::green80);
         RenderUtils::drawLineLoop(rectPoints,      4, Colors::green80);
         RenderUtils::drawFilledLineLoop(&rectPoints[8],  4, Colors::green80);
         RenderUtils::drawLineLoop(&rectPoints[8],  4, Colors::green80);
         RenderUtils::drawFilledLineLoop(&rectPoints[16], 4, Colors::green80);
         RenderUtils::drawLineLoop(&rectPoints[16], 4, Colors::green80);
         break;
      case 6:
         RenderUtils::drawFilledLineLoop(rectPoints,      4, Colors::green80);
         RenderUtils::drawLineLoop(rectPoints,      4, Colors::green80);
         RenderUtils::drawFilledLineLoop(&rectPoints[8],  4, Colors::green80);
         RenderUtils::drawLineLoop(&rectPoints[8],  4, Colors::green80);
         RenderUtils::drawFilledLineLoop(&rectPoints[16], 4, Colors::green80);
         RenderUtils::drawLineLoop(&rectPoints[16], 4, Colors::green80);
         //glColor(Colors::red);
         //RenderUtils::drawLineLoop(&triPoints[18], 3, GLOPT::TriangleFan);
         RenderUtils::drawLineLoop(&rectPoints[24], 4, Colors::green80);
         break;
      case 7:
         RenderUtils::drawFilledLineLoop(rectPoints,      4, Colors::green80);
         RenderUtils::drawLineLoop(rectPoints,      4, Colors::green80);
         RenderUtils::drawFilledLineLoop(&rectPoints[8],  4, Colors::green80);
         RenderUtils::drawLineLoop(&rectPoints[8],  4, Colors::green80);
         RenderUtils::drawFilledLineLoop(&rectPoints[16], 4, Colors::green80);
         RenderUtils::drawLineLoop(&rectPoints[16], 4, Colors::green80);
         RenderUtils::drawFilledLineLoop(&rectPoints[24], 4, Colors::green80);
         RenderUtils::drawLineLoop(&rectPoints[24], 4, Colors::green80);
         break;
      default:
         TNLAssert(false, "Unknown case!");
   }
   nvgRestore(nvg);
   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameObjectRender::renderShipName(const string &shipName, bool isAuthenticated, bool isBusy, bool drawRepairIcon,
                                      U32 killStreak, U32 gamesPlayed, F32 nameScale, F32 alpha)
{
   string renderName = isBusy ? "<<" + shipName + ">>" : shipName;

   F32 textSize = 14 * nameScale;

   nvgStrokeWidth(nvg, RenderUtils::LINE_WIDTH_1);

   // Set name color based on killStreak length
   const Color &color = (killStreak >= UserInterface::StreakingThreshold) ? Colors::streakPlayerNameColor : Colors::idlePlayerNameColor;

   FontManager::setFontColor(color, alpha);

   F32 ypos = textSize + 30;
   S32 len = RenderUtils::drawStringc(0.0f, ypos, textSize, renderName.c_str());

   // Underline name if player is authenticated
   if(isAuthenticated)
   {
      RenderUtils::drawHorizLine(-len * 0.5f, len * 0.5f, ypos + 3, color, alpha);     // 3 provides a little gap beneath the text
      ypos += 2;
   }

   // Indicate player has repair if drawRepairIcon is true -- icon goes to the left of the player name
   if(drawRepairIcon)
      renderRepairItem(Point(-len / 2.0f - 12, ypos - textSize / 2 + 1), true, NULL, 0.5);

   nvgStrokeWidth(nvg, RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameObjectRender::renderShip(S32 layerIndex, const Point &renderPos, const Point &actualPos, const Point &vel,
                                  F32 angle, F32 deltaAngle, ShipShape::ShipShapeType shape, const Color &color, F32 alpha, 
                                  U32 renderTime, const string &shipName, F32 nameScale, F32 warpInScale, bool isLocalShip, bool isBusy, 
                                  bool isAuthenticated, bool showCoordinates, F32 health, F32 radius, S32 team, 
                                  bool drawRepairIcon, bool boostActive, bool shieldActive, bool repairActive, bool sensorActive, 
                                  bool hasArmor, bool engineeringTeleport, U32 killStreak, U32 gamesPlayed)
{
   nvgSave(nvg);
   nvgTranslate(nvg, renderPos.x, renderPos.y);

   // Draw the ship name, if there is one, before the mGL->glRotate below, but only on layer 1...
   // Don't label the local ship.
   if(!isLocalShip && layerIndex == 1 && shipName != "")  
   {
      renderShipName(shipName, isAuthenticated, isBusy, drawRepairIcon, killStreak, gamesPlayed, nameScale, alpha);

      // Show if the player is engineering a teleport
      if(engineeringTeleport)
         renderTeleporterOutline(Point(cos(angle), sin(angle)) * (Ship::CollisionRadius + Teleporter::TELEPORTER_RADIUS),
                                (F32)Teleporter::TELEPORTER_RADIUS, Colors::richGreen);
   }

   if(showCoordinates && layerIndex == 1)
      renderShipCoords(actualPos, isLocalShip, alpha);

   F32 rotAmount = 0;      
   if(warpInScale < 0.8f)
      rotAmount = (0.8f - warpInScale) * 1.5 * FloatTau;

   // An angle of 0 means the ship is heading down the +X axis since we draw the ship 
   // pointing up the Y axis, we should rotate by the ship's angle, - 90 degrees
   nvgRotate(nvg, angle - FloatHalfPi + rotAmount);
   nvgScale(nvg, warpInScale, warpInScale);

   // NOTE: Get rid of this if we stop sending location of cloaked ship to clients.  Also, we can stop
   //       coming here altogether when layerIndex is not 1, and strip out a bunch of ifs above.
   if(layerIndex == -1)    
   {
      // Draw the outline of the ship in solid black -- this will block out any stars and give
      // a tantalizing hint of motion when the ship is cloaked.  Could also try some sort of star-twinkling or
      // scrambling thing here as well...
//      mGL->disable(GLOPT::Blend);

      // FIXME these coords look wrong and I have no idea what this is suppposed
      // to look like
      F32 vertices[] = { 20,-15,  0,25,  20,-15 };
      RenderUtils::drawFilledLineLoop(vertices, ARRAYSIZE(vertices) / 2, Colors::black);

//      mGL->enable(GLOPT::Blend);
   }

   else     // LayerIndex == 1
   {
      // Calculate the various thrust components for rendering purposes; fills thrusts
      static F32 thrusts[4];
      calcThrustComponents(vel, angle, deltaAngle, boostActive, thrusts);  

      renderShip(shape, color, alpha, thrusts, health, radius, renderTime,
                 shieldActive, sensorActive, repairActive, hasArmor);
   }

   nvgRestore(nvg);
}


void GameObjectRender::renderSpawnShield(const Point &pos, U32 shieldTime, U32 renderTime)
{
   static const U32 BlinkStartTime = 1500;
   static const U32 BlinkCycleDuration = 300;
   static const U32 BlinkDuration = BlinkCycleDuration / 2;       // Time shield is yellow or green during

   const Color &color = (shieldTime > BlinkStartTime || shieldTime % BlinkCycleDuration > BlinkDuration) ? Colors::green65 : Colors::yellow40;

   // This rather gross looking variable helps manage problems with the resolution of F32s when getRealMilliseconds() returns a large value
   const S32 BiggishNumber = 21988;
   F32 offset = F32(renderTime % BiggishNumber) * FloatTau / BiggishNumber;
   RenderUtils::drawDashedHollowCircle(pos, Ship::CollisionRadius + 5, Ship::CollisionRadius + 10, 8, FloatTau / 24.0f, offset, color);
}


// Render repair rays to all the repairing objects
void GameObjectRender::renderShipRepairRays(const Point &pos, const Ship *ship, Vector<SafePtr<BfObject> > &repairTargets, F32 alpha)
{
   RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_3);

   for(S32 i = 0; i < repairTargets.size(); i++)
   {
      if(repairTargets[i] && repairTargets[i].getPointer() != ship)
      {
         Vector<Point> targetRepairLocations = repairTargets[i]->getRepairLocations(pos);

         Vector<Point> vertexArray(2 * targetRepairLocations.size());
         for(S32 j = 0; j < targetRepairLocations.size(); j++)
         {
            vertexArray.push_back(pos);
            vertexArray.push_back(targetRepairLocations[j]);
         }

         RenderUtils::drawLines(&vertexArray, Colors::red, alpha);
      }
   }
   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameObjectRender::renderShipCoords(const Point &coords, bool localShip, F32 alpha)
{
   string str = string("@") + itos((S32) coords.x) + "," + itos((S32) coords.y);
   const U32 textSize = 18;

   nvgStrokeWidth(nvg, RenderUtils::LINE_WIDTH_1);

   FontManager::setFontColor(Colors::white,  0.5f * alpha);
   RenderUtils::drawStringc(0, 30 + (localShip ? 0 : textSize + 3) + textSize, textSize, str.c_str() );

   nvgStrokeWidth(nvg, RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameObjectRender::drawFourArrows(const Point &pos)
{
   static const F32 pointList[] = {
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

   nvgSave(nvg);
      nvgTranslate(nvg, pos.x, pos.y);
      RenderUtils::drawLines(pointList, ARRAYSIZE(pointList) / 2, Colors::white);
   nvgRestore(nvg);
}


static const U32 MaxTeleporterParticles = 100;


Color GameObjectRender::getTrackerColor(const Tracker &t, Color *liveColors, Color *deadColors, S32 i, S32 trackerCount)
{
   if(i < trackerCount)
      return liveColors[t.ci];
   else
   {
      Color c;
      c.interp(0.75f * F32(MaxTeleporterParticles - i) / F32(MaxTeleporterParticles - trackerCount), Colors::black, deadColors[t.ci]);
      return c;
   }
}


// TODO: Document me better!  Especially the nerdy math stuff
void GameObjectRender::renderTeleporter(const Point &pos, U32 type, bool spiralInwards, U32 time, F32 zoomFraction, F32 radiusFraction, F32 radius, F32 alpha,
                      const Vector<Point> *dests, S32 trackerCount)
{
   const S32 NumColors = 6;

   // Our Tracker array.  This is global so each teleporter uses the same values
   static Tracker particles[MaxTeleporterParticles];

   // Different teleport color styles
   static float colors[][NumColors][3] = {
      {  // 0 -> Our standard blue-styled teleporter                                               
         { 0.0f, 0.25f, 0.8f },
         { 0.0f, 0.5f,  1.0f },
         { 0.0f, 0.0f,  1.0f },
         { 0.0f, 1.0f,  1.0f },
         { 0.0f, 0.5f,  0.5f },
         { 0.0f, 0.0f,  1.0f },
      },
      {  // 1 -> Unused red/blue/purpley style
         { 1.0f, 0.0f, 0.5f },
         { 1.0f, 0.0f, 1.0f },
         { 0.0f, 0.0f, 1.0f },
         { 0.5f, 0.0f, 1.0f },
         { 0.0f, 0.0f, 0.5f },
         { 1.0f, 0.0f, 0.0f },
      },
      {  // 2 -> Our green engineered teleporter
         { 0.0f, 0.8f, 0.25f },
         { 0.5f, 1.0f, 0.0f },
         { 0.0f, 1.0f, 0.0f },
         { 1.0f, 1.0f, 0.0f },
         { 0.5f, 0.5f, 0.0f },
         { 0.0f, 1.0f, 0.0f },
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
      for(U32 i = 0; i < MaxTeleporterParticles; i++)
      {
         Tracker &t = particles[i];

         t.thetaI = Random::readF() * FloatTau;
         t.thetaP = Random::readF() * 2 + 0.5f;
         t.dP = Random::readF() * 5 + 2.5f;
         t.dI = Random::readF() * t.dP;
         t.ci = Random::readI(0, NumColors - 1);
      }
   }

   // Now the drawing!
   mGL->pushMatrix();

   // This piece draws the destination lines in the commander's map
   // It knows it's in the commander's map if the zoomFraction is greater than zero
   if(zoomFraction > 0)
   {
      const F32 width = 6.0;
      alpha = zoomFraction;

//      glEnable(GL_POLYGON_SMOOTH);
      mGL->setDefaultBlendFunction();
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

         F32 vertices[] = {
               pos.x + asina * width, pos.y + acosa * width,
               midx + asina * width, midy + acosa * width,
               midx - asina * width, midy - acosa * width,
               pos.x - asina * width, pos.y - acosa * width
         };
         mGL->renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GLOPT::TriangleFan, Colors::white, .25f * alpha);

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
         mGL->renderColorVertexArray(vertices2, colors, ARRAYSIZE(vertices2) / 2, GLOPT::TriangleFan);
      }

      //   glDisable(GL_POLYGON_SMOOTH);
   }


   mGL->glTranslate(pos);

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
   for(U32 i = 0; i < MaxTeleporterParticles; i++)
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
      Color currentColor = getTrackerColor(t, liveColors, deadColors, i, trackerCount);

      teleporterColorArray[0] = currentColor.r;
      teleporterColorArray[1] = currentColor.g;
      teleporterColorArray[2] = currentColor.b;
      teleporterColorArray[3] = alpha * alphaMod;
      teleporterColorArray[4] = currentColor.r;
      teleporterColorArray[5] = currentColor.g;
      teleporterColorArray[6] = currentColor.b;
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
         currentColor = getTrackerColor(t, liveColors, deadColors, i, trackerCount);

         teleporterColorArray[8*j]     = currentColor.r;
         teleporterColorArray[(8*j)+1] = currentColor.g;
         teleporterColorArray[(8*j)+2] = currentColor.b;
         teleporterColorArray[(8*j)+3] = alpha * alphaMod * (1 - frac);
         teleporterColorArray[(8*j)+4] = currentColor.r;
         teleporterColorArray[(8*j)+5] = currentColor.g;
         teleporterColorArray[(8*j)+6] = currentColor.b;
         teleporterColorArray[(8*j)+7] = alpha * alphaMod * (1 - frac);
      }

      mGL->renderColorVertexArray(teleporterVertexArray, teleporterColorArray, arrayCount, GLOPT::TriangleStrip);
   }

   mGL->popMatrix();
}


void GameObjectRender::renderTeleporterOutline(const Point &center, F32 radius, const Color &color)
{
   RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_3);
   RenderUtils::drawPolygon(center, 12, radius, 0, color);
   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}


// Render vertices of polyline; only used in the editor
void GameObjectRender::renderPolyLineVertices(const BfObject *obj, bool snapping, F32 currentScale)
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


void GameObjectRender::renderSpyBugVisibleRange(const Point &pos, const Color &color, F32 currentScale)
{
   GameObjectRender::renderFilledPolygon(pos, 6, SpyBug::SPY_BUG_RADIUS * currentScale, color * 0.45f);
}


// Use transparency to highlight areas with overlapping turret coverage
void GameObjectRender::renderTurretFiringRange(const Point &pos, const Color &color, F32 currentScale)
{
   F32 range = Turret::TurretPerceptionDistance * currentScale;
   RenderUtils::drawFilledRect(pos.x - range, pos.y - range, range, range, color, 0.25f);
}


// Renders turret!  --> note that anchor and normal can't be const &Points because of the point math
void GameObjectRender::renderTurret(const Color &color, Point anchor, Point normal, bool enabled, F32 health, F32 barrelAngle, S32 healRate)
{
   const F32 FrontRadius = 15;
   const F32 BaseWidth = 36;
   const F32 HealthBarLength = 28;

   // aimCenter is the point at which the turret would intersect the white base, if it extended that far.
   // It is kind of the "center of rotation" of the turret.
   // Turret is drawn centered on this point, therefore many offsets will be negative.
   Point aimCenter = anchor + normal * Turret::TURRET_OFFSET;

   // Turret is made up of the base, the front, two horizontal lines framing 
   // the health bar, an optional healing indicator, and finally the barrel
   static Vector<Point> basePoints, frontPoints, healthBarFrame, healIndicatorPoints, barrel;

   // Lazily initialize our point vectors
   if(frontPoints.size() == 0)
   {
      generatePointsInARectangle(BaseWidth,        0, -Turret::TURRET_OFFSET,     basePoints);
      generatePointsInARectangle(HealthBarLength, -3, -Turret::TURRET_OFFSET + 3, healthBarFrame);

      generatePointsInASemiCircle(16, FrontRadius,          frontPoints);
      generatePointsInASemiCircle(12, FrontRadius * 0.667f, healIndicatorPoints);

      // Remove first and last points from healing indicator to make it look cooler
      healIndicatorPoints.erase(healIndicatorPoints.size() - 1);     // Last
      healIndicatorPoints.erase(0);                                  // First

      barrel.resize(2);    // Set the dimensions of the vector that we'll need
   }

   nvgSave(nvg);
      nvgTranslate(nvg, aimCenter.x, aimCenter.y);
      nvgRotate(nvg, normal.ATAN2() - FloatHalfPi);

      RenderUtils::drawLineStrip(&frontPoints, color);
      RenderUtils::drawLines(&healthBarFrame, color);

      // Render symbol if it is a regenerating turret
      if(healRate > 0)
         RenderUtils::drawLineStrip(&healIndicatorPoints, color);

      // Draw base after potentially overlapping curvy bits
      RenderUtils::drawLineLoop(&basePoints, enabled ? Colors::white : Colors::gray60);

      renderHealthBar(health, Point(0, -Turret::TURRET_OFFSET / 2), Point(1, 0), HealthBarLength, 5, color);

   nvgRestore(nvg);

   // Now render the turret barrel
   Point aimDelta(cos(barrelAngle), sin(barrelAngle));

   barrel[0].set(aimCenter + aimDelta * FrontRadius);
   barrel[1].set(aimCenter + aimDelta * FrontRadius * 2);

   RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_3);
   RenderUtils::drawLines(&barrel, color);
   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameObjectRender::renderMortar(const Color &color, Point anchor, Point normal, bool enabled, F32 health, S32 healRate)
{
//   const F32 FrontRadius = 15;
   const F32 BaseWidth = 36;
   const F32 HealthBarLength = 28;

   // aimCenter is the point at which the turret would intersect the white base, if it extended that far.
   // It is kind of the "center of rotation" of the turret.
   // Turret is drawn centered on this point, therefore many offsets will be negative.
   Point aimCenter = anchor + normal * Turret::TURRET_OFFSET;

   // Turret is made up of the base, the front, two horizontal lines framing 
   // the health bar, an optional healing indicator, and finally the barrel
   static Vector<Point> basePoints, frontPoints, healthBarFrame, healIndicatorPoints, mortarPoints;

   // Lazily initialize our point vectors
   if(frontPoints.size() == 0)
   {
      generatePointsInARectangle(BaseWidth,        0, -Turret::TURRET_OFFSET,     basePoints);
      generatePointsInARectangle(HealthBarLength, -3, -Turret::TURRET_OFFSET + 3, healthBarFrame);

      generatePointsInARectangle(8, 16, 0, mortarPoints);

      //// Remove first and last points from healing indicator to make it look cooler
      //healIndicatorPoints.erase(healIndicatorPoints.size() - 1);     // Last
      //healIndicatorPoints.erase(0);                                  // First
   }

   nvgSave(nvg);
      F32 rotateAmount = normal.ATAN2() - FloatPi;
      nvgTranslate(nvg, aimCenter.x, aimCenter.y);
      nvgScale(nvg, rotateAmount, rotateAmount);

      RenderUtils::drawLineStrip(&frontPoints, color);
      RenderUtils::drawLines(&healthBarFrame,  color);

      // Render symbol if it is a regenerating turret
      if(healRate > 0)
         RenderUtils::drawLineStrip(&healIndicatorPoints, color);

      // Draw base after potentially overlapping curvy bits
      RenderUtils::drawLineLoop(&basePoints, enabled ? Colors::white : Colors::gray60);

      renderHealthBar(health, Point(0, -Turret::TURRET_OFFSET / 2), Point(1, 0), HealthBarLength, 5, color);

      RenderUtils::drawLineStrip(&mortarPoints, color);
   nvgRestore(nvg);
}


void GameObjectRender::renderFlag(const Color &flagColor, const Color &mastColor, F32 alpha)
{
   // First, the flag itself
   static const F32 flagPoints[] = { -15,-15, 15,-5,  15,-5, -15,5,  -15,-10, 10,-5,  10,-5, -15,0 };
   RenderUtils::drawLines(flagPoints, ARRAYSIZE(flagPoints) / 2, flagColor, alpha);

   // Now the flag's mast
   RenderUtils::drawVertLine(-15, -15, 15, mastColor, alpha);
}


// Draw flag with default mast color
void GameObjectRender::renderFlag(const Color &flagColor, F32 alpha)
{
   renderFlag(flagColor, Colors::white, alpha);
}


void GameObjectRender::doRenderFlag(F32 x, F32 y, F32 scale, const Color &flagColor, const Color &mastColor, F32 alpha)
{
   nvgSave(nvg);
   nvgTranslate(nvg, x, y);
   nvgScale(nvg, scale, scale);

   renderFlag(flagColor, mastColor, alpha);

   nvgRestore(nvg);
}


void GameObjectRender::doRenderFlag(F32 x, F32 y, F32 scale, const Color &flagColor, F32 alpha)
{
   nvgSave(nvg);
   nvgTranslate(nvg, x, y);
   nvgScale(nvg, scale, scale);

   renderFlag(flagColor, alpha);

   nvgRestore(nvg);
}


void GameObjectRender::renderFlag(const Point &pos, const Color &flagColor, const Color &mastColor, F32 alpha)
{
   doRenderFlag(pos.x, pos.y, 1.0, flagColor, mastColor, alpha);
}


void GameObjectRender::renderFlag(const Point &pos, const Color &flagColor, F32 alpha)
{
   doRenderFlag(pos.x, pos.y, 1.0, flagColor, alpha);
}


void GameObjectRender::renderFlag(const Point &pos, F32 scale, const Color &flagColor)
{
   doRenderFlag(pos.x, pos.y, scale, flagColor, 1);
}


void GameObjectRender::renderFlag(F32 x, F32 y, const Color &flagColor)
{
   doRenderFlag(x, y, 1.0, flagColor, 1);
}


void GameObjectRender::renderFlag(const Color &flagColor)
{
   renderFlag(0, 0, flagColor);
}


void GameObjectRender::renderSmallFlag(const Point &pos, const Color &c, F32 parentAlpha)
{
   F32 alpha = 0.75f * parentAlpha;
   
   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);
   nvgScale(nvg, 0.2f, 0.2f);

   static F32 vertices[] = {
         -15, -15,
          15,  -5,
          15,  -5,
         -15,   5,
   };

   RenderUtils::drawLines(vertices, ARRAYSIZE(vertices) / 2, c, alpha);

   // Now the flag's mast
   RenderUtils::drawVertLine(-15, -15, 15, Colors::white, alpha);

   nvgRestore(nvg);
}


void GameObjectRender::renderFlagSpawn(const Point &pos, F32 currentScale, const Color &color)
{
   static const Point p(-4, 0);

   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);
   nvgScale(nvg, 0.4f / currentScale, 0.4f / currentScale);

   renderFlag(color);
   RenderUtils::drawCircle(p, 26, Colors::white);

   nvgRestore(nvg);
}


F32 GameObjectRender::renderCenteredString(const Point &pos, F32 size, const Color &color, const char *string)
{
   return renderCenteredString(pos, S32(size + 0.5f), color, 1.0f, string);
}


F32 GameObjectRender::renderCenteredString(const Point &pos, S32 size, const Color &color, F32 alpha, const char *string)
{
   F32 width = RenderUtils::getStringWidth((F32)size, string);
   RenderUtils::drawStringAndGetWidth_fixed(floor(pos.x - width * 0.5), floor(pos.y - size * 0.5), size, color, alpha, string);

   return width;
}


void GameObjectRender::renderPolygonLabel(const Point &centroid, F32 angle, S32 size, const Color &color, F32 alpha, const char *text, F32 scaleFact)
{
   nvgSave(nvg);
      nvgScale(nvg, scaleFact, scaleFact);
      nvgTranslate(nvg, centroid.x, centroid.y);
      nvgRotate(nvg, angle);
      renderCenteredString(Point(0, size), size, color, alpha, text);
   nvgRestore(nvg);
}


void GameObjectRender::renderPolygonOutline(const Vector<Point> *outlinePoints, const Color &outlineColor, F32 alpha, F32 lineThickness)
{
   RenderUtils::lineWidth(lineThickness);
   RenderUtils::drawLineLoop(outlinePoints, outlineColor, alpha);
   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameObjectRender::renderPolygonFill(const Vector<Point> *triangulatedFillPoints, const Color &fillColor, F32 alpha)
{
   RenderUtils::drawFilledLineLoop(triangulatedFillPoints, fillColor, alpha);
}


void GameObjectRender::renderPolygon(const Vector<Point> *fillPoints, const Vector<Point> *outlinePoints,
                   const Color &fillColor, const Color &outlineColor, F32 alpha)
{
   renderPolygonFill(fillPoints, fillColor, alpha);
   renderPolygonOutline(outlinePoints, outlineColor, alpha);
}


void GameObjectRender::renderZone(const Color &outlineColor, const Vector<Point> *outline, const Vector<Point> *fill)
{
   Color fillColor = outlineColor;     // Copies color so we can modify it
   fillColor *= 0.5;

   renderPolygon(fill, outline, fillColor, outlineColor);
}


void GameObjectRender::renderLoadoutZone(const Color &color, const Vector<Point> *outline, const Vector<Point> *fill,
                                         const Point &centroid, F32 angle, F32 scaleFact)
{
   renderZone(color, outline, fill);
   renderLoadoutZoneIcon(centroid, 20, color, angle);
}


// In normal usage, outerRadius is 20, which is the default if no radius is specified
void GameObjectRender::renderLoadoutZoneIcon(const Point &center, S32 outerRadius, const Color &color, F32 angleRadians)
{
   // Pos, teeth, outer rad, inner rad, ang span of outer teeth, ang span of inner teeth, circle rad
   drawGear(center, 7, (F32)outerRadius, 0.75f * outerRadius, 20.0f, 18.0f, 0.4f * outerRadius, color, angleRadians);
}


void GameObjectRender::renderGoalZoneIcon(const Point &center, S32 radius, const Color &color)
{
   static const F32 flagPoints[] = { -6, 10,  -6,-10,  12, -3.333f,  -6, 3.333f, };

   nvgSave(nvg);
      nvgTranslate(nvg, center.x, center.y);
      nvgScale(nvg, radius * 0.041667f, radius * 0.041667f); // 1 / 24 since we drew it to in-game radius of 24 (a ship's radius)

      RenderUtils::drawPolygon(4, (F32)radius, 0.0f, color);
      RenderUtils::drawLineStrip(flagPoints, ARRAYSIZE(flagPoints) / 2, color);
   nvgRestore(nvg);
}


void GameObjectRender::renderNavMeshZone(const Vector<Point> *outline, const Vector<Point> *fill, const Point &centroid,
                       S32 zoneId)
{
   renderPolygon(fill, outline, Colors::green50, Colors::green, 0.4f);

   TNLAssert(zoneId <= U16_MAX, "Too many digits!");

   char buf[U16_MAX_DIGITS + 1];  // Can't have more than 65535 zones, +1 for the trailing \0
   dSprintf(buf, 24, "%d", zoneId);

   renderPolygonLabel(centroid, 0, 25, Colors::green, 0.4f, buf);
}


// Only used in-game
void GameObjectRender::renderNavMeshBorders(const Vector<NeighboringZone> &borders)
{
   RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_4);

   // Go through each border and render
   for(S32 i = 0; i < borders.size(); i++)
   {
      F32 vertices[] = {
            borders[i].borderStart.x, borders[i].borderStart.y,
            borders[i].borderEnd.x,   borders[i].borderEnd.y,
      };
      RenderUtils::drawLines(vertices, 2, Colors::cyan);
   }

   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
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
void GameObjectRender::renderGoalZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill)
{
   const Color &fillColor    = getGoalZoneFillColor(c, false, 0);
   const Color &outlineColor = getGoalZoneOutlineColor(c, false);

   renderPolygon(fill, outline, fillColor, outlineColor);
}


// Goal zone flashes after capture, but glows after touchdown...
void GameObjectRender::renderGoalZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill, Point centroid, F32 labelAngle,
                    bool isFlashing, F32 glowFraction, S32 score, F32 flashCounter, GoalZoneFlashStyle flashStyle)
{
   Color outlineColor, fillColor;
   F32 baseAlpha = 0.5f;

   if(flashStyle == GoalZoneFlashOriginal)
   {
//      fillColor    = getGoalZoneFillColor(c, isFlashing, glowFraction);
//      outlineColor = getGoalZoneOutlineColor(c, isFlashing);

      // TODO: reconcile why using the above commented out code doesn't work
      F32 alpha = isFlashing ? 0.75f : baseAlpha;
      fillColor    = Color(Colors::yellow * (glowFraction * glowFraction) + Color(c) * alpha * (1 - glowFraction * glowFraction));
      outlineColor = Color(Colors::yellow * (glowFraction * glowFraction) + Color(c) *         (1 - glowFraction * glowFraction));
   }
   else if(flashStyle == GoalZoneFlashExperimental) // Some new flashing effect (sam's idea)
   {
      F32 glowRate = baseAlpha - fabs(flashCounter - baseAlpha); 

      Color newColor(c);
      if(isFlashing)
         newColor = newColor + glowRate * (1 - glowRate);
      else
         newColor = newColor * (1 - glowRate);

      fillColor    = getGoalZoneFillColor(newColor, false, glowFraction);
      outlineColor = getGoalZoneOutlineColor(newColor, false);
   }
   else
   {
      fillColor = c * baseAlpha;
      outlineColor = c;
   }

   renderPolygon(fill, outline, fillColor, outlineColor);
   renderGoalZoneIcon(centroid, 24, outlineColor);
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


void GameObjectRender::renderNexusIcon(const Point &center, S32 radius, F32 angleRadians, const Color &color)
{
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

   nvgSave(nvg);
   nvgTranslate(nvg, center.x, center.y);
   nvgScale(nvg, radius * 0.05f, radius * 0.05f);
   nvgRotate(nvg, angleRadians);

   // Draw our center spokes
   RenderUtils::drawLines(spokes, ARRAYSIZE(spokes) / 2, color);

   // Draw design
   RenderUtils::drawArc(arcPoint1, arcRadius, 0.583f * FloatTau, 1.25f  * FloatTau, color);
   RenderUtils::drawArc(arcPoint2, arcRadius, 0.917f * FloatTau, 1.583f * FloatTau, color);
   RenderUtils::drawArc(arcPoint3, arcRadius, 0.25f  * FloatTau, 0.917f * FloatTau, color);

   nvgRestore(nvg);
}


void GameObjectRender::renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, bool open, F32 glowFraction)
{
   const Color baseColor = getNexusBaseColor(open, glowFraction);

   const Color fillColor = Color(baseColor * (glowFraction * glowFraction  + (1 - glowFraction * glowFraction) * 0.5f));
   const Color outlineColor = fillColor * 0.7f;

   renderPolygon(fill, outline, fillColor, outlineColor);
}


void GameObjectRender::renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, Point centroid,
                 F32 labelAngle, bool open, F32 glowFraction)
{
   renderNexus(outline, fill, open, glowFraction);

   renderNexusIcon(centroid, 20, labelAngle, getNexusBaseColor(open, glowFraction));
}


void GameObjectRender::renderSlipZoneIcon(const Point &center, S32 radius, F32 angleRadians)
{
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

   nvgSave(nvg);
      nvgTranslate(nvg, center.x, center.y);
      nvgScale(nvg, radius * 0.05f, radius * 0.05f);
      RenderUtils::drawLines(lines, ARRAYSIZE(lines) / 2, Colors::cyan);
   nvgRestore(nvg);
}


void GameObjectRender::renderSlipZone(const Vector<Point> *bounds, const Vector<Point> *boundsFill, const Point &centroid)
{
   Color color(0, 0.5, 0);  // Go for a pale green, for now...

   RenderUtils::drawFilledLineLoop(boundsFill, color * 0.5);
   RenderUtils::drawLineLoop(bounds, color * 0.7);

   renderSlipZoneIcon(centroid, 20);
}


void GameObjectRender::renderProjectile(const Point &pos, U32 type, U32 time)
{
   ProjectileInfo *pi = GameWeapon::projectileInfo + type;

   S32 bultype = 1;

   if(bultype == 1)    // Default stars
   { 
      nvgSave(nvg);
         nvgTranslate(nvg, pos.x, pos.y);
         nvgScale(nvg, pi->scaleFactor, pi->scaleFactor);

         nvgSave(nvg);
            nvgRotate(nvg, degreesToRadians(F32(time % 720)) * 0.5f);

            static S16 projectilePoints1[] = { -2,2,  0,6,  2,2,  6,0,  2,-2,  0,-6,  -2,-2,  -6,0 };
            RenderUtils::drawLineLoop(projectilePoints1, ARRAYSIZE(projectilePoints1) / 2, pi->projColors[0]);

         nvgRestore(nvg);

//         F32 rotateAmount = 180 - F32(time % 360);
         F32 rotateAmount = FloatPi - degreesToRadians(F32(time % 360));
         nvgRotate(nvg, rotateAmount);

         static S16 projectilePoints2[] = { -2,2,  0,8,  2,2,  8,0,  2,-2,  0,-8, -2,-2,  -8,0 };
         RenderUtils::drawLineLoop(projectilePoints2, ARRAYSIZE(projectilePoints2) / 2, pi->projColors[1]);

      nvgRestore(nvg);
   }
   else if (bultype == 2) { // Tiny squares rotating quickly, good machine gun

      nvgSave(nvg);
         nvgTranslate(nvg, pos.x, pos.y);
         nvgScale(nvg, pi->scaleFactor, pi->scaleFactor);
         nvgRotate(nvg, degreesToRadians(F32(time % 720)));

      static S16 projectilePoints3[] = { -2,2,  2,2,  2,-2,  -2,-2 };
      RenderUtils::drawLineLoop(projectilePoints3, ARRAYSIZE(projectilePoints3) / 2, pi->projColors[1]);

      nvgRestore(nvg);

   } else if (bultype == 3) {  // Rosette of circles  MAKES SCREEN GO BEZERK!!

      const F32 innerR = 6;
      const F32 outerR = 3;
      const int dist = 10;

#define dr(x) degreesToRadians(x)

      nvgRotate(nvg, dr( fmod(F32(time) * .15f, 720.f) ));

      Point p(0,0);
      RenderUtils::drawCircle(p, innerR, pi->projColors[1]);
      p.set(0,-dist);

      RenderUtils::drawCircle(p, outerR, pi->projColors[1]);
      p.set(0,-dist);
      RenderUtils::drawCircle(p, outerR, pi->projColors[1]);
      p.set(cos(dr(30)), -sin(dr(30)));
      RenderUtils::drawCircle(p*dist, outerR, pi->projColors[1]);
      p.set(cos(dr(30)), sin(dr(30)));
      RenderUtils::drawCircle(p * dist, outerR, pi->projColors[1]);
      p.set(0, dist);
      RenderUtils::drawCircle(p, outerR, pi->projColors[1]);
      p.set(-cos(dr(30)), sin(dr(30)));
      RenderUtils::drawCircle(p*dist, outerR, pi->projColors[1]);
      p.set(-cos(dr(30)), -sin(dr(30)));
      RenderUtils::drawCircle(p*dist, outerR, pi->projColors[1]);
   }
}


void GameObjectRender::renderSeeker(const Point &pos, F32 angleRadians, F32 speed, U32 timeRemaining)
{
   nvgSave(nvg);
      nvgTranslate(nvg, pos.x, pos.y);
      nvgRotate(nvg, angleRadians);

      // The flames first!
      F32 speedRatio = speed / WeaponInfo::getWeaponInfo(WeaponSeeker).projVelocity + (S32(timeRemaining) % 200)/ 400.0f;  
      F32 innerFlame[] = {
            -8, -1,
            -8 - (4 * speedRatio), 0,
            -8, 1,
      };
      RenderUtils::drawLineStrip(innerFlame, 3, Colors::yellow, 0.5f);
      F32 outerFlame[] = {
            -8, -3,
            -8 - (8 * speedRatio), 0,
            -8, 3,
      };
      RenderUtils::drawLineStrip(outerFlame, 3, Colors::orange50, 0.6f);

      // The body of the seeker
      F32 vertices[] = {
            -8, -4,
            -8,  4,
             8,  0
      };
      RenderUtils::drawLineLoop(vertices, 3, Colors::redderMagenta, 1.0f);
   nvgRestore(nvg);
}



void GameObjectRender::renderMine(const Point &pos, bool armed, bool visible)
{
   F32 mod;
   F32 vis;   

   if(visible)    // Friendly mine
   {
      RenderUtils::drawCircle(pos, (F32)Mine::SensorRadius, Colors::gray50);
      mod = 0.8f;
      vis = 1.0f;
   }
   else           // Invisible enemy mine
   {
      RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_1);
      mod = 0.80f;
      vis = 0.18f;
   }

   Color gray(mod);
   RenderUtils::drawCircle(pos, 10, gray, vis);

   if(armed)
   {
      Color red(mod, 0, 0);
      RenderUtils::drawCircle(pos, 6, red, vis);
   }
   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}

#ifndef min
#  define min(a,b) ((a) <= (b) ? (a) : (b))
#  define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


// lifeLeft is a number between 0 and 1.  Burst explodes when lifeLeft == 0.
void GameObjectRender::renderGrenade(const Point &pos, F32 lifeLeft)
{
   RenderUtils::drawCircle(pos, 10, Colors::white);

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

   Color color(1, min(1.25f - lifeLeft, 1), 0);

   if(innerVis)
      RenderUtils::drawFilledCircle(pos, 6, color);
   else
      RenderUtils::drawCircle(pos, 6, color);
}


void GameObjectRender::renderFilledPolygon(const Point &pos, S32 points, S32 radius, const Color &fillColor, const Color &outlineColor)
{
   Vector<Point> pts(points);

   calcPolygonVerts(pos, points, (F32)radius, 0, pts);

   renderPolygon(&pts, &pts, fillColor, outlineColor);
}


void GameObjectRender::renderFilledPolygon(const Point &pos, S32 points, S32 radius, const Color &fillColor)
{
   Vector<Point> pts(points);
   calcPolygonVerts(pos, points, (F32)radius, 0, pts);

   renderPolygonFill(&pts, fillColor);
}

void GameObjectRender::renderSpyBug(const Point &pos, const Color &teamColor, bool visible)
{
   if(visible)
   {
      const Color &color = Colors::gray50;

      renderFilledPolygon(pos, 6, 15, teamColor * 0.45f, color);
      RenderUtils::drawString_fixed(pos.x - 3, pos.y + 5, 10, color, "S");
   }
   else
   {
      RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_1);
      RenderUtils::drawPolygon(pos, 6, 5, 0, Colors::gray25);
      RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
  }

}


void GameObjectRender::renderRepairItem(const Point &pos)
{
   renderRepairItem(pos, false, 0, 1);
}


void GameObjectRender::renderRepairItem(const Point &pos, bool forEditor, const Color *overrideColor, F32 alpha)
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

   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);

   RenderUtils::drawHollowSquare(Point(0, 0), size, overrideColor == NULL ? Colors::white : *overrideColor, alpha);

   // The red cross
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

   RenderUtils::drawLineLoop(vertices, ARRAYSIZE(vertices) / 2, overrideColor == NULL ? Colors::red : *overrideColor, alpha);

   nvgRestore(nvg);
}


void GameObjectRender::renderEnergySymbol(const Point &pos, F32 scaleFactor)
{
   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);
   nvgScale(nvg, scaleFactor, scaleFactor);
   renderEnergySymbol();
   nvgRestore(nvg);
}


// Render the actual lightning bolt glyph at 0,0
void GameObjectRender::renderEnergySymbol()
{
   // Yellow lightning bolt
   static const F32 energySymbolPoints[] = { 20,-20,  3,-2,  12,5,  -20,20,  -2,3,  -12,-5 };
   RenderUtils::drawLineLoop(energySymbolPoints, ARRAYSIZE(energySymbolPoints) / 2, Colors::orange67);
}


void GameObjectRender::renderEnergyItem(const Point &pos, bool forEditor)
{
   F32 scaleFactor = forEditor ? .45f : 1;    // Resize for editor

   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);

      F32 size = 18;
      RenderUtils::drawHollowSquare(Point(0,0), size, Colors::white);
      nvgStrokeWidth(nvg, RenderUtils::DEFAULT_LINE_WIDTH);

      // Scale down the symbol a little so it fits in the box
      nvgScale(nvg, scaleFactor * 0.7f, scaleFactor * 0.7f);
      renderEnergySymbol();

   nvgRestore(nvg);
}


void GameObjectRender::renderEnergyItem(const Point &pos)
{
   renderEnergyItem(pos, false);
}


// Use faster method with no offset
void GameObjectRender::renderWallFill(const Vector<Point> *points, const Color &fillColor, bool polyWall)
{
   // Polywall geometry are triangles, barriers are just the outline
//   mGL->renderPointVector(points, polyWall ? GLOPT::Triangles : GLOPT::TriangleFan, fillColor);

   // FIXME NANOVG does its own triangulation, but it may not be cached.  This
   // may be a loss in performance for rendering polywalls.  This is drawing the
   // list of triangles as a line loop so there may be rendering issues
   RenderUtils::drawFilledLineLoop(points, fillColor, 1.0f);
}


// Use slower method if each point needs to be offset
void GameObjectRender::renderWallFill(const Vector<Point> *points, const Color &fillColor, const Point &offset, bool polyWall)
{
   // Polywall geometry are triangles, barriers are just the outline
//   mGL->renderPointVector(points, offset, polyWall ? GLOPT::Triangles : GLOPT::TriangleFan, fillColor);

   nvgSave(nvg);
   nvgTranslate(nvg, offset.x, offset.y);
   // FIXME NANOVG does its own triangulation, but it may not be cached.  This
   // may be a loss in performance for rendering polywalls.  This is drawing the
   // list of triangles as a line loop so there may be rendering issues
   RenderUtils::drawFilledLineLoop(points, fillColor, 1.0f);
   nvgRestore(nvg);
}


// Used in both editor and game
void GameObjectRender::renderWallEdges(const Vector<Point> &edges, const Color &outlineColor, F32 alpha)
{
   RenderUtils::drawLines(&edges, outlineColor, alpha);
}


// Used in editor only
void GameObjectRender::renderWallEdges(const Vector<Point> &edges, const Point &offset, const Color &outlineColor, F32 alpha)
{
   nvgSave(nvg);
   nvgTranslate(nvg, offset.x, offset.y);
   RenderUtils::drawLines(&edges, outlineColor, alpha);
   nvgRestore(nvg);
}


void GameObjectRender::renderSpeedZone(const Vector<Point> &points)
{
   S32 pointSize = points.size() / 2;

   RenderUtils::drawLineLoop(&points[0].x, pointSize, Colors::red);
   RenderUtils::drawLineLoop(&points[pointSize].x, pointSize, Colors::red);
}


void GameObjectRender::renderTestItem(const Point &pos, S32 size, F32 alpha)
{
   Vector<Point> pts;
   calcPolygonVerts(pos, TestItem::TEST_ITEM_SIDES, (F32)size, 0, pts);
   renderTestItem(pts, alpha);
}


void GameObjectRender::renderTestItem(const Vector<Point> &points, F32 alpha)
{
   RenderUtils::drawLineLoop((F32 *)&points[0], points.size(), Colors::yellow, alpha);
}


void GameObjectRender::renderAsteroid(const Point &pos, S32 design, F32 scaleFact, const Color *color, F32 alpha)
{
   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);

   F32 vertexArray[2 * ASTEROID_POINTS];
   for(S32 i = 0; i < ASTEROID_POINTS; i++)
   {
      vertexArray[2*i]     = AsteroidCoords[design][i][0] * scaleFact;
      vertexArray[(2*i)+1] = AsteroidCoords[design][i][1] * scaleFact;
   }
   RenderUtils::drawLineLoop(vertexArray, ASTEROID_POINTS, color ? *color : Color(.7), alpha);

   nvgRestore(nvg);
}


void GameObjectRender::renderAsteroid(const Point &pos, S32 design, F32 scaleFact)
{
   renderAsteroid(pos, design, scaleFact, NULL);
}


void GameObjectRender::renderAsteroidSpawn(const Point &pos, S32 time)
{
   static const S32 period = 4096;  // Power of 2 please
   static const F32 invPeriod = 1 / F32(period);

   F32 alpha = max(0.0f, 1.0f - time * invPeriod);

   renderAsteroid(pos, 2, 0.1f, &Colors::green, alpha);
}


void GameObjectRender::renderAsteroidSpawnEditor(const Point &pos, F32 scale)
{
   scale *= 0.8f;
   static const Point p(0,0);

   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);
   nvgScale(nvg, scale, scale);

   renderAsteroid(p, 2, 0.1f);
   RenderUtils::drawCircle(13, Colors::white);

   nvgRestore(nvg);
}

//
//void GameObjectRender::renderResourceItem(const Point &pos, F32 scaleFactor, const Color *color, F32 alpha)
//{
//   mGL->glPushMatrix();
//      mGL->glTranslate(pos);
//      mGL->glScale(scaleFactor);
//
//      static F32 resourcePoints[] = { -8,8,  0,20,  8,8,  20,0,  8,-8,  0,-20,  -8,-8,  -20,0 };
//      mGL->renderVertexArray(resourcePoints, ARRAYSIZE(resourcePoints) / 2, GLOPT::LineLoop, color == NULL ? Colors::white : *color, alpha);
//   mGL->glPopMatrix();
//}


void GameObjectRender::renderResourceItem(const Vector<Point> &points, F32 alpha)
{
   RenderUtils::drawLineLoop((F32 *)&points[0], points.size(), Colors::white, alpha);
}


void GameObjectRender::renderSoccerBall(const Point &pos, F32 size)
{
   RenderUtils::drawCircle(pos, size, Colors::white);
}


// Unscaled, the lock is 3.4 px tall, 3 px wide.  If these change, TimeLeftRenderer will need to change as well.
void GameObjectRender::renderLock(const Color &color)
{
   static F32 vertices[] = {
         0, 2,
         0, 4,
         3, 4,
         3, 2
   };

   RenderUtils::drawLineLoop(vertices, ARRAYSIZE(vertices) / 2, color);

   static F32 vertices2[] = {
         2.6f, 2,
         2.6f, 1.3f,
         2.4f, 0.9f,
         1.9f, 0.6f,
         1.1f, 0.6f,
         0.6f, 0.9f,
         0.4f, 1.3f,
         0.4f, 2
   };

   RenderUtils::drawLineStrip(vertices2, ARRAYSIZE(vertices2) / 2, color);
}


void GameObjectRender::renderCore(const Point &pos, const Color &coreColor, U32 time,
                const PanelGeom *panelGeom, const F32 panelHealth[], F32 panelStartingHealth)
{
   // Draw outer polygon and inner circle
   Color baseColor = Colors::gray80;

   Point dir;   // Reusable container
   
   for(S32 i = 0; i < CORE_PANELS; i++)
   {
      dir = (panelGeom->repair[i] - pos);
      dir.normalize();

      renderHealthBar(panelHealth[i] / panelStartingHealth, panelGeom->repair[i], dir, 30, 7, coreColor);

      F32 vertices[] = {
            panelGeom->getStart(i).x, panelGeom->getStart(i).y,
            panelGeom->getEnd(i).x, panelGeom->getEnd(i).y
      };
      RenderUtils::drawLines(vertices, 2, (panelHealth[i] == 0) ? coreColor * 0.2f : baseColor);

      // Draw health stakes
      if(panelHealth[i] > 0)
      {
//         F32 vertices2[] = {
//               panelGeom->repair[i].x, panelGeom->repair[i].y,
//               pos.x, pos.y
//         };
//
//         static F32 colors[] = {
//               Colors::gray20.r, Colors::gray20.g, Colors::gray20.b, 1,
//               Colors::black .r, Colors::black .g, Colors::black .b, 1,
//         };
//
//         mGL->renderColorVertexArray(vertices2, colors, 2, GLOPT::Lines);
         RenderUtils::drawLineGradient(panelGeom->repair[i].x, panelGeom->repair[i].y, pos.x, pos.y,
               Colors::gray20, 1.0f, Colors::black, 1.0f);
      }
   }

   F32 atomSize = 40;
   F32 angle = CoreItem::getCoreAngle(time);

   // Draw atom graphic
//   F32 t = FloatTau + (F32(time & 1023) / 1024.f * FloatTau);
//   for(F32 rotate = 0; rotate < FloatTau - 0.01f; rotate += FloatTau / 5)  //  0.01f part avoids rounding error
//   {
//      // 32 vertices and colors
//      F32 vertexArray[64];
//      F32 colorArray[128];
//      U32 count = 0;
//      for(F32 theta = 0; theta < FloatTau; theta += RenderUtils::CIRCLE_SIDE_THETA)
//      {
//         F32 x = cos(theta + rotate * 2 + t) * atomSize * 0.5f;
//         F32 y = sin(theta + rotate * 2 + t) * atomSize;
//
//         vertexArray[(2*count)]   = pos.x + cos(rotate + angle) * x + sin(rotate + angle) * y;
//         vertexArray[(2*count)+1] = pos.y + sin(rotate + angle) * x - cos(rotate + angle) * y;
//         colorArray[(4*count)]    = coreColor.r;
//         colorArray[(4*count)+1]  = coreColor.g;
//         colorArray[(4*count)+2]  = coreColor.b;
//         colorArray[(4*count)+3]  = theta / FloatTau;
//         count++;
//      }
//      mGL->renderColorVertexArray(vertexArray, colorArray, ARRAYSIZE(vertexArray)/2, GLOPT::LineLoop);
//   }
   // FIXME NANOVG draw new atom graphic with NanoVG

   RenderUtils::drawCircle(pos, atomSize + 2, baseColor);
}


// Here we render a simpler, non-animated Core to reduce distraction
void GameObjectRender::renderCoreSimple(const Point &pos, const Color &coreColor, S32 width)
{
   // Here we render a simpler, non-animated Core to reduce distraction in the editor
   RenderUtils::drawPolygon(pos, 10, (F32)width / 2, 0, Colors::white);
   RenderUtils::drawCircle(pos, width / 5.0f + width * .01f, coreColor);    // + .01 to match full item rendering
}


void GameObjectRender::renderSoccerBall(const Point &pos)
{
   renderSoccerBall(pos, SoccerBallItem::SOCCER_BALL_RADIUS);
}


void GameObjectRender::renderTextItem(const Point &pos, const Point &dir, F32 size, const string &text, const Color &color)
{
   // Handle special case
   if(text == "Bitfighter")
   {
      renderBitfighterLogo(pos, dir, size);
      return;
   }

   // Note that the LineEditor creates strings containing "\\n" when the user enters "\n".  We need to 
   // swap these out with a true newline char for the splitMultiLineString function to work.
   Vector<string> lines;
   splitMultiLineString(replaceString(text ,"\\n", "\n"), lines);     // Split with '\n'

   F32 angle = pos.angleTo(dir);    // In radians, I daresay!
   F32 ypos = pos.y;
   F32 xpos = pos.x;

   F32 lineHeight = size * 1.25f;
   F32 xspace = -lineHeight * sin(angle);
   F32 yspace =  lineHeight * cos(angle);

   FontManager::setFontColor(color);
   for(S32 i = 0; i < lines.size(); i++)
   {
      RenderUtils::drawAngleString(xpos, ypos, size, angle, lines[i].c_str());
      xpos += xspace;
      ypos += yspace;
   }
}


void GameObjectRender::renderBitfighterLogo(const Point &pos, const Point &dir, F32 size)
{
   // All factors in here determined experimentally, seem to work at a variety of sizes and approximate the width and height
   // of ordinary text in cases tested.  What should happen is the Bitfighter logo should, as closely as possible, match the 
   // size and extent of the text "Bitfighter".
   F32 scaleFactor = size / 129.0f;

   nvgSave(nvg);
      nvgTranslate(nvg, pos.x, pos.y);
      nvgScale(nvg, scaleFactor, scaleFactor);
      nvgRotate(nvg, pos.angleTo(dir));
      nvgTranslate(nvg, -119, -45);

      renderBitfighterLogo(0, 1, Colors::green);
   nvgRestore(nvg);
}


// Only used by instructions... in-game uses the other signature
void GameObjectRender::renderForceFieldProjector(const Point &pos, const Point &normal, const Color &color, bool enabled, S32 healRate)
{
   Vector<Point> geom = ForceFieldProjector::getForceFieldProjectorGeometry(pos, normal);
   renderForceFieldProjector(&geom, pos, color, enabled, healRate);
}


void GameObjectRender::renderForceFieldProjector(const Vector<Point> *geom, const Point &pos, const Color &color, bool enabled, S32 healRate)
{
   F32 ForceFieldBrightnessProjector = 0.50;

   Color c(color);      // Create locally modifiable copy

   c = c * (1 - ForceFieldBrightnessProjector) + ForceFieldBrightnessProjector;

   if(!enabled)
      c *= 0.6f;

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

      nvgSave(nvg);
         nvgTranslate(nvg, centerPoint.x, centerPoint.y);
         nvgRotate(nvg, angle);
         RenderUtils::drawLineStrip(symbol, ARRAYSIZE(symbol) / 2, c);
      nvgRestore(nvg);
   }

   RenderUtils::drawLineLoop(geom, c);
}


void GameObjectRender::renderForceField(Point start, Point end, const Color &color, bool fieldUp, F32 scaleFact)
{
   Vector<Point> geom = ForceField::computeGeom(start, end, scaleFact);

   F32 ForceFieldBrightness = 0.25;

   Color c(color);
   c = c * (1 - ForceFieldBrightness) + ForceFieldBrightness;
   if(!fieldUp)
      c *= 0.5;

   RenderUtils::drawLineLoop(&geom, c);
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


void GameObjectRender::renderStaticBitfighterLogo()
{
   renderBitfighterLogo(73, 1, Colors::green);
   FontManager::pushFontContext(ReleaseVersionContext);
   RenderUtils::drawCenteredString_fixed(130, 10, Colors::green, ZAP_GAME_RELEASE_LONGSTRING);  // The compiler combines both strings
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
void GameObjectRender::renderBitfighterLogo(U32 mask, const Color &color)
{
   S16 *posPtr = &gLogoPoints[0].x;

   if(mask & 1 << 0)
      RenderUtils::drawLineLoop(posPtr, LetterLoc1, color);
   posPtr += 2*LetterLoc1;
   if(mask & 1 << 1)
      RenderUtils::drawLineLoop(posPtr, LetterLoc2, color);
   posPtr += 2*LetterLoc2;
   if(mask & 1 << 2)
      RenderUtils::drawLineLoop(posPtr, LetterLoc3, color);
   posPtr += 2*LetterLoc3;
   if(mask & 1 << 3)
      RenderUtils::drawLineLoop(posPtr, LetterLoc4, color);
   posPtr += 2*LetterLoc4;
   if(mask & 1 << 4)
      RenderUtils::drawLineLoop(posPtr, LetterLoc5, color);
   posPtr += 2*LetterLoc5;
   if(mask & 1 << 5)
      RenderUtils::drawLineLoop(posPtr, LetterLoc6, color);
   posPtr += 2*LetterLoc6;
   if(mask & 1 << 6)
      RenderUtils::drawLineLoop(posPtr, LetterLoc7, color);
   posPtr += 2*LetterLoc7;
   if(mask & 1 << 7)
      RenderUtils::drawLineLoop(posPtr, LetterLoc8, color);
   posPtr += 2*LetterLoc8;

   if(mask & 1 << 8)
      RenderUtils::drawLineLoop(posPtr, LetterLoc9, color);
   posPtr += 2*LetterLoc9;
   if(mask & 1 << 8)
      RenderUtils::drawLineLoop(posPtr, LetterLoc10, color);
   posPtr += 2*LetterLoc10;
   if(mask & 1 << 8)
      RenderUtils::drawLineLoop(posPtr, LetterLoc11, color);
   posPtr += 2*LetterLoc11;

   if(mask & 1 << 9)
      RenderUtils::drawLineLoop(posPtr, LetterLoc12, color);
   posPtr += 2*LetterLoc12;
   if(mask & 1 << 9)
      RenderUtils::drawLineLoop(posPtr, LetterLoc13, color);
}


// Draw logo centered on screen horzontally, and on yPos vertically, scaled and rotated according to parameters
void GameObjectRender::renderBitfighterLogo(S32 yPos, F32 scale, const Color &color, U32 mask)
{
   const F32 fact = 0.15f * scale;   // Scaling factor to make the coordinates below fit nicely on the screen (derived by trial and error)
   
   // 3609 is the diff btwn the min and max x coords below, 594 is same for y
   nvgSave(nvg);
   nvgTranslate(nvg, (DisplayManager::getScreenInfo()->getGameCanvasWidth() - 3609 * fact) / 2, yPos - 594 * fact / 2);
   nvgScale(nvg, fact, fact);
   nvgStrokeWidth(nvg, RenderUtils::DEFAULT_LINE_WIDTH / fact);  // Offset scaling for the stroke width

   renderBitfighterLogo(mask, color);

   nvgRestore(nvg);
}


void GameObjectRender::renderBitfighterLogo(const Point &pos, F32 size, const Color &color, U32 letterMask)
{
   const F32 sizeToLogoRatio = 0.0013f;  // Shot in the dark!
   const F32 fact = sizeToLogoRatio * size;

   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);
   nvgScale(nvg, fact, fact);
   nvgStrokeWidth(nvg, RenderUtils::DEFAULT_LINE_WIDTH / fact);  // Offset scaling for the stroke width

   renderBitfighterLogo(letterMask, color);

   nvgRestore(nvg);
}


// Red vertices in walls, and magenta snapping vertices
void GameObjectRender::renderSmallSolidVertex(F32 currentScale, const Point &pos, bool snapping)
{
   F32 size = MIN(MAX(currentScale, 1), 2);              // currentScale, but limited to range 1-2
   RenderUtils::drawFilledSquare(pos, size / currentScale, snapping ? Colors::magenta : Colors::red);
}


void GameObjectRender::renderVertex(char style, const Point &v, S32 number)
{
   renderVertex(style, v, number, EditorObject::VERTEX_SIZE, 1, 1);
}


void GameObjectRender::renderVertex(char style, const Point &v, S32 number, F32 scale)
{
   renderVertex(style, v, number, EditorObject::VERTEX_SIZE, scale, 1);
}


void GameObjectRender::renderVertex(char style, const Point &v, S32 number, F32 scale, F32 alpha)
{
   renderVertex(style, v, number, EditorObject::VERTEX_SIZE, scale, alpha);
}


static const Color &getVertexColor(char style)
{
   if(style == HighlightedVertex)
      return Colors::EDITOR_HIGHLIGHT_COLOR;
   if(style == SelectedVertex)
      return Colors::EDITOR_SELECT_COLOR;
   if(style == SnappingVertex)
      return Colors::EDITOR_SNAP_VERTEX_COLOR;
   if(style == SelectedItemVertex)
      return Colors::EDITOR_SELECTED_ITEM_VERTEX_COLOR;
   
   TNLAssert(false, "Unhandled enum value!");
   return Colors::white;
}


void GameObjectRender::renderVertex(char style, const Point &v, S32 number, S32 size, F32 scale, F32 alpha)
{
   bool hollow = style == HighlightedVertex || style == SelectedVertex || style == SelectedItemVertex || style == SnappingVertex;

   // Fill the box with a dark gray to make the number easier to read
   if(hollow && number != NO_NUMBER)
      RenderUtils::drawFilledSquare(v, size / scale, Colors::gray25);
      
   RenderUtils::drawSquare(v, (F32)size / scale, getVertexColor(style), alpha, !hollow);

   if(number == NO_NUMBER)     
      return;

   // Draw vertex numbers
   F32 fontsize = 6 / scale;
   RenderUtils::drawStringfc(v.x, v.y + 3 / scale, fontsize, Colors::white, alpha, "%d", number);
}


void GameObjectRender::renderSquareItem(const Point &pos, const Color &c, F32 alpha, const Color &letterColor, char letter)
{
   RenderUtils::drawFilledSquare(pos, 8, c, alpha);  // Draw filled box in which we'll put our letter
   RenderUtils::drawLetter(letter, pos, letterColor, alpha);
}


void GameObjectRender::drawDivetedTriangle(F32 height, F32 len)
{
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

   nvgSave(nvg);
      nvgTranslate(nvg, 200, 200);
      nvgRotate(nvg, GLfloat(Platform::getRealMilliseconds() / 10 % 360));
      nvgScale(nvg, 6, 6);
      renderPolygonOutline(&pts, Colors::red);
   nvgRestore(nvg);
}


void GameObjectRender::drawGear(const Point &center, S32 teeth, F32 radius1, F32 radius2, F32 ang1, F32 ang2, F32 innerCircleRadius, const Color &color, F32 angleRadians)
{
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

   nvgSave(nvg);
      nvgTranslate(nvg, center.x, center.y);
      nvgRotate(nvg, angleRadians);

      renderPolygonOutline(&pts, color);

      RenderUtils::drawCircle(innerCircleRadius, color);

   nvgRestore(nvg);
}


void GameObjectRender::render25FlagsBadge(F32 x, F32 y, F32 rad)
{
   nvgSave(nvg);
   nvgTranslate(nvg, x, y);
   nvgScale(nvg, .05f * rad, .05f * rad);

   RenderUtils::drawEllipse(Point(-16, 15), 6, 2, Colors::gray40);

   renderFlag(-.10f * rad, -.10f * rad, Colors::red50);

   nvgRestore(nvg);

   F32 ts = rad - 3;
   F32 width = RenderUtils::getStringWidth(ts, "25");
   F32 tx = x + .30f * rad;
   F32 ty = y + rad - .40f * rad;

   RenderUtils::drawFilledRect((tx - width / 2.0 - 1.0), (ty - (ts + 2.0) / 2.0),
                               width + 1.5, ts + 2.0, Colors::yellow);

   renderCenteredString(Point(tx, ty), ts, Colors::gray20, "25");
}


void GameObjectRender::renderDeveloperBadge(F32 x, F32 y, F32 rad)
{
   F32 r3  = rad * 0.333f;
   F32 r23 = rad * 0.666f;

   // Render cells
   RenderUtils::drawSquare(Point(x,       y - r23), r3, Colors::green80, true);
   RenderUtils::drawSquare(Point(x + r23, y      ), r3, Colors::green80, true);
   RenderUtils::drawSquare(Point(x - r23, y + r23), r3, Colors::green80, true);
   RenderUtils::drawSquare(Point(x,       y + r23), r3, Colors::green80, true);
   RenderUtils::drawSquare(Point(x + r23, y + r23), r3, Colors::green80, true);

   // Render grid atop our cells
   RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_1);
   
   RenderUtils::drawHorizLine(x - rad, x + rad, y - rad, Colors::gray20);
   RenderUtils::drawHorizLine(x - rad, x + rad, y - r3,  Colors::gray20);
   RenderUtils::drawHorizLine(x - rad, x + rad, y + r3,  Colors::gray20);
   RenderUtils::drawHorizLine(x - rad, x + rad, y + rad, Colors::gray20);

   RenderUtils::drawVertLine(x - rad, y - rad, y + rad, Colors::gray20);
   RenderUtils::drawVertLine(x - r3,  y - rad, y + rad, Colors::gray20);
   RenderUtils::drawVertLine(x + r3,  y - rad, y + rad, Colors::gray20);
   RenderUtils::drawVertLine(x + rad, y - rad, y + rad, Colors::gray20);

   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameObjectRender::renderBBBBadge(F32 x, F32 y, F32 rad, const Color &color)
{
   renderBitfighterLogo(Point(x - (rad * 0.5f), y - (rad * 0.666f)), 2 * rad, color, 256);  // Draw the 'B' only
}


void GameObjectRender::renderLevelDesignWinnerBadge(F32 x, F32 y, F32 rad)
{
   F32 rm2 = rad - 2;

   Vector<Point> edges;
   edges.push_back(Point(x - rm2, y - rm2));
   edges.push_back(Point(x - rm2, y + rm2));
   edges.push_back(Point(x + rm2, y + rm2));
   edges.push_back(Point(x + rm2, y - rm2));

   renderWallFill(&edges, Colors::wallFillColor, false);
   renderPolygonOutline(&edges, Colors::blue);
   renderCenteredString(Point(x, y + rad), rad, Colors::white, "1");
}


void GameObjectRender::renderZoneControllerBadge(F32 x, F32 y, F32 rad)
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

   renderPolygon(&points,  &points,  Colors::gray40, Colors::gray80);
   renderPolygon(&points2, &points2, Colors::blue40, Colors::blue80);

   const F32 size = rad * 0.9f;
   renderCenteredString(Point(x, y + size), size, Colors::white, "ZC");
}


void GameObjectRender::renderRagingRabidRabbitBadge(F32 x, F32 y, F32 rad)
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

   nvgSave(nvg);
      nvgTranslate(nvg, x, y);
      nvgScale(nvg, .1f * rad, .1f * rad);
      RenderUtils::drawLineLoop(rabbit, ARRAYSIZE(rabbit) / 2, Colors::gray50);
      RenderUtils::drawLineLoop(eye,    ARRAYSIZE(eye)    / 2, Colors::red);
   nvgRestore(nvg);
}


void GameObjectRender::renderLastSecondWinBadge(F32 x, F32 y, F32 rad)
{
   nvgSave(nvg);
      nvgTranslate(nvg, x, y);
      nvgScale(nvg, .05f * rad, .05f * rad);
      renderFlag(-.10f * rad, -.10f * rad, Colors::blue);
   nvgRestore(nvg);

   F32 tx = x + .40f * rad;
   F32 ty = y + 1.40f * rad;

   renderCenteredString(Point(tx, ty), rad, Colors::white, ":01");
}


void GameObjectRender::renderHatTrickBadge(F32 x, F32 y, F32 rad)
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


   // Waldo?  I FOUND HIM
   nvgSave(nvg);
      nvgTranslate(nvg, x, y);
      nvgScale(nvg, .1f * rad, .1f * rad);

      // GLOPT::TriangleFan, then GLOPT::LineLoop for anti-aliasing
      RenderUtils::drawFilledLineLoop(brim, ARRAYSIZE(brim) / 2, Colors::red);
      RenderUtils::drawLineLoop(brim, ARRAYSIZE(brim) / 2, Colors::red);

      RenderUtils::drawFilledLineLoop(outline, ARRAYSIZE(outline) / 2, Colors::white);
      RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_1);
      RenderUtils::drawLineLoop(outline, ARRAYSIZE(outline) / 2, Colors::white);
      RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);

      RenderUtils::drawLineStrip(stripe, ARRAYSIZE(stripe) / 2, Colors::red);
      RenderUtils::drawLines(pompom, ARRAYSIZE(pompom) / 2, Colors::red);
   nvgRestore(nvg);
}


void GameObjectRender::renderBadge(F32 x, F32 y, F32 rad, MeritBadges badge)
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


void GameObjectRender::renderInfiniteRayFromPoint(const Point &point, F32 angleInRadians, const Color &color)
{
   const F32 len = 1000000;      // Not quite infinite...

   RenderUtils::drawLine(point.x, point.y,
         point.x + len * cos(angleInRadians), point.y - len * sin(angleInRadians),
         color);
}


void GameObjectRender::renderGridLines(const Point &offset, F32 gridScale, F32 grayVal, bool fadeLines)
{
   // Use F32 to avoid GameObjectRender::cumulative rounding errors
   F32 xStart = fmod(offset.x, gridScale);
   F32 yStart = fmod(offset.y, gridScale);

   const Color gray = Color(grayVal);

   while(yStart < DisplayManager::getScreenInfo()->getGameCanvasHeight())
   {
      RenderUtils::drawHorizLine((S32)yStart, gray);
      yStart += gridScale;
   }

   while(xStart < DisplayManager::getScreenInfo()->getGameCanvasWidth())
   {
      RenderUtils::drawVertLine((S32)xStart, gray);
      xStart += gridScale;
   }
}


// Render background snap grid for the editor
void GameObjectRender::renderGrid(F32 currentScale, const Point &offset, const Point &origin, F32 gridSize, bool fadeLines, bool showMinorGridLines)
{
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
   Color gray = Color(0.7f * snapFadeFact);
   RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_3);

   RenderUtils::drawHorizLine(origin.y, gray);
   RenderUtils::drawVertLine (origin.x, gray);

   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameObjectRender::renderStars(const Point *stars, const Color *colors, S32 numStars, F32 alpha, const Point &cameraPos, const Point &visibleExtent)
{
   if(alpha <= 0.5f)
      return;

   static const F32 starChunkSize = 1024;        // Smaller numbers = more dense stars
   static const F32 starDist = 3500;             // Bigger value = slower moving stars

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

   // Render some stars
   const F32 *vertexPointer = &stars[0].x;

   // Do some calculations for the parallax
   S32 xDist = (S32) (cameraPos.x / starDist);
   S32 yDist = (S32) (cameraPos.y / starDist);

   S32 fx1 = -1 - xDist;
   S32 fx2 =  1 - xDist;
   S32 fy1 = -1 - yDist;
   S32 fy2 =  1 - yDist;

   // The (F32(xPage + 1.f) == xPage) part becomes true, which could cause endless loop problem freezing game when way too far off the center.
   for(F32 xPage = upperLeft.x + fx1; xPage < lowerRight.x + fx2 && !(F32(xPage + 1.f) == xPage); xPage++)
      for(F32 yPage = upperLeft.y + fy1; yPage < lowerRight.y + fy2 && !(F32(yPage + 1.f) == yPage); yPage++)
      {
         RenderUtils::drawPoints(vertexPointer, numStars, Colors::gray60, alpha,
               RenderUtils::LINE_WIDTH_1, starChunkSize,
               xPage + (cameraPos.x / starDist), yPage + (cameraPos.y / starDist), 0.0f);
      }
}


void GameObjectRender::renderWalls(const Vector<DatabaseObject *> *walls,
                 const Vector<DatabaseObject *> *polyWalls,
                 const Vector<Point> &wallEdgePoints,
                 const Vector<Point> &selectedWallEdgePointsWholeWalls,
                 const Vector<Point> &selectedWallEdgePointsDraggedVertices,
                 const Color &outlineColor,
                 const Color &fillColor, 
                 F32 currentScale, 
                 bool dragMode, 
                 bool drawSelected,
                 const Point &selectedItemOffset, 
                 bool previewMode, 
                 bool showSnapVertices, 
                 F32 alpha)
{
   S32 wallCount     = walls->size();
   S32 polyWallCount = polyWalls->size();

   S32 count = wallCount + polyWallCount;

   if(drawSelected)     
   {
      for(S32 i = 0; i < count; i++)
      {
         // TODO figure out how to remove dynamic_cast
         BfObject *obj = static_cast<BfObject *>((i < wallCount) ? walls->get(i) : polyWalls->get(i - wallCount));
         BarrierX *barrier = dynamic_cast<BarrierX *>(obj);

         for(S32 j = 0; j < barrier->getSegmentCount(); j++)
         {
            const WallSegment *wallSegment = barrier->getSegment(j);

            if(obj->isSelected())  
               wallSegment->renderFill(selectedItemOffset, fillColor * alpha, true);
         }
      }

      // Render wall outlines for walls being dragged in their entirety
      renderWallEdges(selectedWallEdgePointsWholeWalls, selectedItemOffset, outlineColor);

      // Render wall outlines for walls/polywalls with vertices being dragged
      renderWallEdges(selectedWallEdgePointsDraggedVertices, Point(0,0), outlineColor);
   }
   else  // Render selected/moving walls last so they appear on top; this is pass 2, 
   {
      // hack for now
      Color color;
      if(alpha < 1)
         color = Colors::gray67;
      else
         color = fillColor * alpha;

      // This is what keeps wall rendering from rendering over the shadow walls when dragging a wall around
      bool moved = (selectedItemOffset.x != 0 || selectedItemOffset.y != 0);

      for(S32 i = 0; i < count; i++)
      {
         BarrierX *barrier;
         BfObject *obj;

         if(i < wallCount)
         {
            barrier = static_cast<BarrierX *>(static_cast<WallItem *>(walls->get(i)));
            obj = static_cast<BfObject *>(walls->get(i));
         }
         else
         {
            barrier = static_cast<BarrierX *>(static_cast<PolyWall *>(polyWalls->get(i - wallCount)));
            obj = static_cast<BfObject *>(polyWalls->get(i - wallCount));
         }

         for(S32 j = 0; j < barrier->getSegmentCount(); j++)
         {
            if(moved && obj->isSelected())
               continue;
            
            // RenderFill ignores offset for unselected walls
            barrier->getSegment(j)->renderFill(selectedItemOffset, color, false);
         }
      }

      renderWallEdges(wallEdgePoints, outlineColor);                 // Render wall outlines with unselected walls
   }

   if(showSnapVertices)
   {
      RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_1);

      for(S32 i = 0; i < wallEdgePoints.size(); i++)
         renderSmallSolidVertex(currentScale, wallEdgePoints[i], dragMode);

      RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
   }
}


// Render the gray shadows of walls that are being manipulated.  Ignore any passed objects that are
// not walls or polywalls.  These shadows are rendered before the wall being dragged.
void GameObjectRender::renderShadowWalls(const Vector<BfObject *> &objects)
{
   for(S32 i = 0; i < objects.size(); i++)
   {
      if(objects[i]->getObjectTypeNumber() == PolyWallTypeNumber)
         renderPolygonFill(objects[i]->getFill(), Colors::EDITOR_SHADOW_WALL_COLOR);

      else if(objects[i]->getObjectTypeNumber() == WallItemTypeNumber)
      {
         WallItem *barrier = static_cast<WallItem *>(objects[i]);

         for(S32 j = 0; j < barrier->getSegmentCount(); j++)
            barrier->getSegment(j)->renderFill(Point(0, 0), Colors::EDITOR_SHADOW_WALL_COLOR, false);
      }
   }
}


void GameObjectRender::renderConstrainedDraggingLines(const Point &origin)
{
   const S32 rays = 360 / 15;      // increments of 15 degrees
   Point offset;    // Reusable container
   const F32 radius = 40;

   for(S32 i = 0; i < rays; i++)
   {
      F32 ang = degreesToRadians(i * 15);
      offset.set(cos(ang) * radius, -sin(ang) * radius);

      GameObjectRender::renderInfiniteRayFromPoint(origin + offset, ang, Colors::gray40);

      RenderUtils::drawLineGradient(origin.x + offset.x, origin.y + offset.y,
                         origin.x,            origin.y, Colors::gray40, 1.0f, Colors::gray40, 0.0f);
   }
}


void GameObjectRender::renderWallSpine(const WallItem *wallItem, const Vector<Point> *outline, const Color &color,
                                       F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices)
{
   RenderUtils::drawLineStrip(outline, color);

   if(renderVertices)
      renderPolyLineVertices(wallItem, snappingToWallCornersEnabled, currentScale);

}


void GameObjectRender::drawObjectiveArrow(const Point &nearestPoint, F32 zoomFraction, const Color &outlineColor,
                                          S32 canvasWidth, S32 canvasHeight, F32 alphaMod, F32 highlightAlpha)
{
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

   const Color fillColor(outlineColor * 0.7f);    // Create local copy

   static Point vertices[3];     // Reusable array

   vertices[0].set(rp.x, rp.y);
   vertices[1].set(p2.x, p2.y);
   vertices[2].set(p3.x, p3.y);

   // This loops twice: once to render the objective arrow, once to render the outline
   RenderUtils::drawLineLoop((F32 *)(vertices), ARRAYSIZE(vertices), outlineColor, alpha);
   RenderUtils::drawFilledLineLoop((F32 *)(vertices), ARRAYSIZE(vertices), fillColor, alpha);

   // Highlight the objective arrow, if need be.  This will be called rarely, so efficiency here is 
   // not as important as above
   if(highlightAlpha > 0)
   {
      Vector<Point> inPoly(vertices, ARRAYSIZE(vertices));
      Vector<Point> outPoly;
      offsetPolygon(&inPoly, outPoly, HIGHLIGHTED_OBJECT_BUFFER_WIDTH);

      RenderUtils::drawLineLoop((F32 *)(outPoly.address()), outPoly.size(), Colors::HelpItemRenderColor, highlightAlpha * alpha);
   }

   //   Point cen = rp - arrowDir * 12;

   // Try labeling the objective arrows... kind of lame.
   //drawStringf(cen.x - UserInterface::getStringWidthf(10,"%2.1f", dist/100) / 2, cen.y - 5, 10, "%2.1f", dist/100);

   // Add an icon to the objective arrow...  kind of lame.
   //renderSmallFlag(cen, c, alpha);
}


void GameObjectRender::renderScoreboardOrnamentTeamFlags(S32 xpos, S32 ypos, const Color &color, bool teamHasFlag)
{
   nvgSave(nvg);
      nvgTranslate(nvg, F32(xpos), F32(ypos + 15));
      nvgScale(nvg, .75, .75);
      renderFlag(color);
   nvgRestore(nvg);

   // Add an indicator for the team that has the flag
   if(teamHasFlag)
      RenderUtils::drawString_fixed(xpos - 23, ypos + 25, 18, Colors::magenta, "*");      // These numbers are empirical alignment factors
}


void GameObjectRender::renderSpawn(const Point &pos, F32 scale, const Color &color)
{   
   nvgSave(nvg);
   nvgTranslate(nvg, pos.x, pos.y);
   nvgScale(nvg, scale, scale);

   renderSquareItem(Point(0,0), color, 1, Colors::white, 'S');

   nvgRestore(nvg);
}


// For debugging bots -- renders the path the bot will take from its current location to its destination.
// Note that the bot may cut corners as it goes.
void GameObjectRender::renderFlightPlan(const Point &from, const Point &to, const Vector<Point> &flightPlan)
{
   // Render from ship to start of flight plan
   Vector<Point> line(2);
   line.push_back(from);
   line.push_back(to);
   RenderUtils::drawLineStrip(&line, Colors::red);

   // Render the flight plan itself
   RenderUtils::drawLineStrip(&flightPlan, Colors::red);
}


// Used by SimpleLineItem to draw the chunky arrow that represents the item in the editor
void GameObjectRender::renderHeavysetArrow(const Point &pos, const Point &dest, const Color &color, bool isSelected, bool isLitUp)
{
   for(S32 i = 1; i >= 0; i--)
   {
      // Draw heavy colored line with colored core
      RenderUtils::lineWidth(i ? RenderUtils::LINE_WIDTH_4 : RenderUtils::DEFAULT_LINE_WIDTH);
      F32 alpha = i ? .35f : 1;

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
      RenderUtils::drawLines(vertices, 4, color, alpha);

      // Draw highlighted core on 2nd pass if item is selected, but not while it's being edited
      Color coreColor = color;
      if(i == 0 && (isSelected || isLitUp))
      {
         coreColor = isSelected ? Colors::EDITOR_SELECT_COLOR : Colors::EDITOR_HIGHLIGHT_COLOR;
         alpha = 1;
      }

      F32 vertices2[] = {
            pos.x,  pos.y,
            dest.x, dest.y
      };

      RenderUtils::drawLines(vertices2, 2, coreColor, alpha);
   }
}


void GameObjectRender::renderTeleporterEditorObject(const Point &pos, S32 radius, const Color &color)
{
   RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_3);
   RenderUtils::drawPolygon(pos, 12, (F32)radius, 0, color);
   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameObjectRender::renderUpArrow(const Point &center, S32 size, const Color &color)
{
   F32 offset = (F32)size / 2;

   F32 top = center.y - offset;
   F32 bot = center.y + offset;
   F32 capHeight = size * 0.39f;    // An artist need provide no explanation

   F32 vertices[] = { center.x, top,     center.x,             bot,
                      center.x, top,     center.x - capHeight, top + capHeight,
                      center.x, top,     center.x + capHeight, top + capHeight
                    };

   RenderUtils::drawLines(vertices, ARRAYSIZE(vertices) / 2, color);
}


void GameObjectRender::renderDownArrow(const Point &center, S32 size, const Color &color)
{
   F32 offset = (F32)size / 2;
   F32 top = center.y - offset;
   F32 bot = center.y + offset;
   F32 capHeight = size * 0.39f;    // An artist need provide no explanation

   F32 vertices[] = { center.x, top,     center.x,             bot,
                      center.x, bot,     center.x - capHeight, bot - capHeight,
                      center.x, bot,     center.x + capHeight, bot - capHeight
                    };

   RenderUtils::drawLines(vertices, ARRAYSIZE(vertices) / 2, color);
}


void GameObjectRender::renderLeftArrow(const Point &center, S32 size, const Color &color)
{
   F32 offset = (F32)size / 2;
   F32 left = center.x - offset;
   F32 right = center.x + offset;
   F32 capHeight = size * 0.39f;    // An artist need provide no explanation

   F32 vertices[] = { left, center.y,     right,            center.y,
                      left, center.y,     left + capHeight, center.y - capHeight,
                      left, center.y,     left + capHeight, center.y + capHeight
                    };

   RenderUtils::drawLines(vertices, ARRAYSIZE(vertices) / 2, color);
}


void GameObjectRender::renderRightArrow(const Point &center, S32 size, const Color &color)
{
   F32 offset = (F32)size / 2;
   F32 left = center.x - offset;
   F32 right = center.x + offset;
   F32 capHeight = size * 0.39f;    // An artist need provide no explanation

   F32 vertices[] = { left,  center.y,     right,             center.y,
                      right, center.y,     right - capHeight, center.y - capHeight,
                      right, center.y,     right - capHeight, center.y + capHeight
                    };

   RenderUtils::drawLines(vertices, ARRAYSIZE(vertices) / 2, color);
}


void GameObjectRender::renderNumberInBox(const Point pos, S32 number, F32 scale)
{
   string numberStr = itos(number);

   F32 height = 13.0f;
   F32 halfHeight = height / 2;
   F32 padding = 4.0f;

   F32 len = RenderUtils::getStringWidth(height, numberStr);

   RenderUtils::drawFilledRect(pos.x - len / 2 - padding, pos.y + halfHeight + padding,
                               len + 2*padding, height + 1.5*padding, // 0.5 compensates for weird font spacing
                               Colors::white, 0.75f); 

   RenderUtils::drawCenteredString_fixed(pos.x, pos.y + halfHeight, height, HelpContext, Colors::black, numberStr.c_str());
}


}

