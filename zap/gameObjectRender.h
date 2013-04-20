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

#ifndef _GAMEOBJECTRENDER_H_
#define _GAMEOBJECTRENDER_H_

#ifndef ZAP_DEDICATED

#include "tnl.h"

#include "Geometry.h"

#include "Rect.h"
#include "Color.h"
#include "SharedConstants.h"     // For MeritBadges enum
#include "ShipShape.h"

#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{

extern const Color *HIGHLIGHT_COLOR;
extern const Color *SELECT_COLOR;
extern const Color *PLAIN_COLOR;

static const S32 NO_NUMBER = -1;

class Ship;
class WallItem;

//////////
// Primitives
extern void drawFilledCircle(const Point &pos, F32 radius, const Color *color = NULL);
extern void drawFilledSector(const Point &pos, F32 radius, F32 start, F32 end);
extern void drawCentroidMark(const Point &pos, F32 radius);
extern void renderTwoPointPolygon(const Point &p1, const Point &p2, F32 width, S32 mode);

extern void drawRoundedRect(const Point &pos, F32 width, F32 height, F32 radius);
extern void drawRoundedRect(const Point &pos, S32 width, S32 height, S32 radius);


extern void drawFilledRoundedRect(const Point &pos, S32 width, S32 height, const Color &fillColor, const Color &outlineColor, S32 radius);
extern void drawFilledRoundedRect(const Point &pos, F32 width, F32 height, const Color &fillColor, const Color &outlineColor, F32 radius);

extern void drawArc(const Point &pos, F32 radius, F32 startAngle, F32 endAngle);

extern void drawEllipse(const Point &pos, F32 width, F32 height, F32 angle);
extern void drawEllipse(const Point &pos, S32 width, S32 height, F32 angle);

extern void drawFilledEllipse(const Point &pos, F32 width, F32 height, F32 angle);
extern void drawFilledEllipse(const Point &pos, S32 width, S32 height, F32 angle);

extern void drawPolygon(const Point &pos, S32 sides, F32 radius, F32 angle);

extern void drawStar(const Point &pos, S32 points, F32 radius, F32 innerRadius);


extern void drawAngledRay(const Point &center, F32 innerRadius, F32 outerRadius, F32 angle);
extern void drawAngledRayCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 rayCount, F32 offsetAngle);
extern void drawAngledRayArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, S32 rayCount, F32 offsetAngle);
extern void drawDashedArc(const Point &center, F32 radius, F32 centralAngle, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle);
extern void drawDashedHollowCircle(const Point &center, F32 innerRadius, F32 outerRadius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle);
extern void drawDashedCircle(const Point &center, F32 radius, S32 dashCount, F32 dashSpaceCentralAngle, F32 offsetAngle);
extern void drawHollowArc(const Point &center, F32 innerRadius, F32 outerRadius, F32 centralAngle, F32 offsetAngle);

extern void drawSquare(const Point &pos, F32 radius, bool filled = false);
extern void drawSquare(const Point &pos, S32 radius, bool filled = false);
extern void drawHollowSquare(const Point &pos, F32 radius, const Color *color = NULL);
extern void drawFilledSquare(const Point &pos, F32 radius, const Color *color = NULL);
extern void drawFilledSquare(const Point &pos, S32 radius, const Color *color = NULL);

extern void renderSmallSolidVertex(F32 currentScale, const Point &pos, bool snapping);

extern void renderVertex(char style, const Point &v, S32 number);
extern void renderVertex(char style, const Point &v, S32 number,           F32 scale);
extern void renderVertex(char style, const Point &v, S32 number,           F32 scale, F32 alpha);
extern void renderVertex(char style, const Point &v, S32 number, S32 size, F32 scale, F32 alpha);

void renderLine(const Vector<Point> *points, const Color *color);

extern void drawHorizLine(S32 x1, S32 x2, S32 y);
extern void drawVertLine (S32 x,  S32 y1, S32 y2);
extern void drawHorizLine(F32 x1, F32 x2, F32 y);
extern void drawVertLine (F32 x,  F32 y1, F32 y2);

extern void renderSquareItem(const Point &pos, const Color *c, F32 alpha, const Color *letterColor, char letter);

extern void drawCircle(const Point &pos, F32 radius, const Color *color = NULL, F32 alpha = 1.0);
extern void drawCircle(F32 x, F32 y,     F32 radius, const Color *color = NULL, F32 alpha = 1.0);

extern void drawDivetedTriangle(F32 height, F32 len);
extern void drawGear(const Point &pos, S32 teeth, F32 r1, F32 r2, F32 ang1, F32 ang2, F32 innerCircleRadius);


//////////
// Some things for rendering on screen display
void renderEnergyGuage(S32 energy);

extern F32 renderCenteredString(const Point &pos, S32 size, const char *string);
extern F32 renderCenteredString(const Point &pos, F32 size, const char *string);

extern void renderShip(ShipShape::ShipShapeType shapeType, const Color *shipColor, F32 alpha, F32 thrusts[], F32 health, F32 radius, U32 sensorTime,
                       bool cloakActive, bool shieldActive, bool sensorActive, bool repairActive, bool hasArmor);

void renderShipRepairRays(const Point &pos, const Ship *ship, Vector<SafePtr<BfObject> > &repairTargets, F32 alpha);   // Render repair rays to all the repairing objects

extern void renderShipCoords(const Point &coords, bool localShip, F32 alpha);

extern void renderAimVector();
extern void drawFourArrows(const Point &pos);

extern void renderTeleporter(const Point &pos, U32 type, bool spiralInwards, S32 time, F32 zoomFraction, F32 radiusFraction, F32 radius, F32 alpha, 
                             const Vector<Point> *dests, U32 trackerCount = 100);
extern void renderTeleporterOutline(const Point &center, F32 radius, const Color &color);
extern void renderSpyBugVisibleRange(const Point &pos, const Color &color, F32 currentScale = 1);
extern void renderTurretFiringRange(const Point &pos, const Color &color, F32 currentScale);
extern void renderTurret(const Color &c, Point anchor, Point normal, bool enabled, F32 health, F32 barrelAngle, S32 healRate = 0);

extern void renderFlag(const Point &pos, const Color *flagColor);
extern void renderFlag(const Point &pos, F32 scale, const Color *flagColor);
extern void renderFlag(F32 x, F32 y, const Color *flagColor);
extern void renderFlag(const Color *flagColor);
extern void renderFlag(const Point &pos, const Color *flagColor, const Color *mastColor, F32 alpha);
extern void doRenderFlag(F32 x, F32 y, F32 scale, const Color *flagColor, const Color *mastColor, F32 alpha);


//extern void renderFlag(Point pos, Color c, F32 timerFraction);
extern void renderSmallFlag(const Point &pos, const Color &c, F32 parentAlpha);
extern void renderFlagSpawn(const Point &pos, F32 currentScale, const Color *color);

extern void renderZone(const Color *c, const Vector<Point> *outline, const Vector<Point> *fill);   

extern void renderLoadoutZone(const Color *c, const Vector<Point> *outline, const Vector<Point> *fill, 
                              const Point &centroid, F32 angle, F32 scaleFact = 1);

extern void renderLoadoutZoneIcon(const Point &pos);

extern void renderNavMeshZone(const Vector<Point> *outline, const Vector<Point> *fill,
                              const Point &centroid, S32 zoneId, bool isConvex, bool isSelected = false);

class Border;
extern void renderNavMeshBorder(const Border &border, F32 scaleFact, const Color &color, F32 fillAlpha, F32 width);

class ZoneBorder;
extern void renderNavMeshBorders(const Vector<ZoneBorder> &borders, F32 scaleFact = 1);

class NeighboringZone;
extern void renderNavMeshBorders(const Vector<NeighboringZone> &borders, F32 scaleFact = 1);

extern void drawStars(Point *stars, S32 numStars, F32 alphaFrac, bool starsInDistance, Point cameraPos, Point visibleExtent);


// Some things we use internally, but also need from UIEditorInstructions for consistency
extern const Color BORDER_FILL_COLOR;
extern const F32 BORDER_FILL_ALPHA;
extern const F32 BORDER_WIDTH;
extern float gDefaultLineWidth;

extern void renderPolygonOutline(const Vector<Point> *outline);
extern void renderPolygonOutline(const Vector<Point> *outlinePoints, const Color *outlineColor, F32 alpha = 1, F32 lineThickness = gDefaultLineWidth);
extern void renderPolygonFill(const Vector<Point> *fillPoints, const Color *fillColor, F32 alpha = 1);
extern void renderPolygon(const Vector<Point> *fillPoints, const Vector<Point> *outlinePoints,
                          const Color *fillColor, const Color *outlineColor, F32 alpha = 1);

extern void renderGoalZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill);     // No label version
extern void renderGoalZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill, Point centroid, F32 labelAngle,
                           bool isFlashing, F32 glowFraction, S32 score, F32 flashCounter, bool useOldStyle);

extern void renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, Point centroid, F32 labelAngle, 
                        bool open, F32 glowFraction, F32 scaleFact = 1);

extern void renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, bool open, F32 glowFraction);



extern void renderSlipZone(const Vector<Point> *bounds, const Vector<Point> *boundsFill, const Point &centroid);
extern void renderPolygonLabel(const Point &centroid, F32 angle, F32 size, const char *text, F32 scaleFact = 1);

extern void renderProjectile(const Point &pos, U32 type, U32 time);
extern void renderSeeker(const Point &pos, F32 angleRadians, F32 speed, U32 timeRemaining);

extern void renderMine(const Point &pos, bool armed, bool visible);
extern void renderGrenade(const Point &pos, F32 lifeLeft);
extern void renderSpyBug(const Point &pos, const Color &teamColor, bool visible, bool drawOutline);

extern void renderRepairItem(const Point &pos);
extern void renderRepairItem(const Point &pos, bool forEditor, const Color *overrideColor, F32 alpha);

extern void renderEnergyItem(const Point &pos); 

extern void renderWallFill(const Vector<Point> *points, const Color &fillColor, bool polyWall);
extern void renderWallFill(const Vector<Point> *points, const Color &fillColor, const Point &offset, bool polyWall);

extern void renderEnergyItem(const Point &pos, bool forEditor);
extern void renderEnergySymbol();                                   // Render lightning bolt symbol
extern void renderEnergySymbol(const Point &pos, F32 scaleFactor);  // Another signature

// Wall rendering
void renderWallEdges(const Vector<Point> &edges, const Color &outlineColor, F32 alpha = 1.0);
void renderWallEdges(const Vector<Point> &edges, const Point &offset, const Color &outlineColor, F32 alpha = 1.0);

//extern void renderSpeedZone(Point pos, Point normal, U32 time);
void renderSpeedZone(const Vector<Point> &pts, U32 time);

void renderTestItem(const Point &pos, F32 alpha = 1);
void renderTestItem(const Point &pos, S32 size, F32 alpha = 1);

void renderWorm(const Point &pos);

void renderAsteroid(const Point &pos, S32 design, F32 scaleFact);
void renderAsteroid(const Point &pos, S32 design, F32 scaleFact, const Color *color, F32 alpha = 1);

void renderAsteroidSpawn(const Point &pos, S32 time);
void renderAsteroidSpawnEditor(const Point &pos, F32 scale = 1.0);

void renderResourceItem(const Point &pos, F32 alpha = 1);
void renderResourceItem(const Point &pos, F32 scaleFactor, const Color *color, F32 alpha);

struct PanelGeom;
void renderCore(const Point &pos, const Color *coreColor, U32 time, 
                PanelGeom *panelGeom, F32 panelHealth[10], F32 panelStartingHealth);

void renderCoreSimple(const Point &pos, const Color *coreColor, S32 width);

void renderSoccerBall(const Point &pos, F32 size);
void renderSoccerBall(const Point &pos);

void renderTextItem(const Point &pos, const Point &dir, F32 size, const string &text, const Color *color);

// Editor support items
extern void renderPolyLineVertices(BfObject *obj, bool snapping, F32 currentScale);
extern void renderGrid(F32 curentScale, const Point &offset, const Point &origin, F32 gridSize, bool fadeLines, bool showMinorGridLines);

extern void renderForceFieldProjector(Point pos, Point normal);
extern void renderForceFieldProjector(Point pos, Point normal, const Color *teamColor, bool enabled);
extern void renderForceFieldProjector(const Vector<Point> *geom, const Color *teamColor, bool enabled, S32 healRate = 0);
extern void renderForceField(Point start, Point end, const Color *c, bool fieldUp, F32 scale = 1);

extern void renderBitfighterLogo(S32 yPos, F32 scale, U32 mask = 1023);
extern void renderBitfighterLogo(const Point &pos, F32 size, U32 letterMask = 1023);
extern void renderStaticBitfighterLogo();

extern void renderBadge(F32 x, F32 y, F32 rad, MeritBadges badge);

extern void renderWalls(const GridDatabase *wallSegmentDatabase, const Vector<Point> &wallEdgePoints, 
                        const Vector<Point> &selectedWallEdgePoints, const Color &outlineColor, 
                        const Color &fillColor, F32 currentScale, bool dragMode, bool drawSelected,
                        const Point &selectedItemOffset, bool previewMode, bool showSnapVertices, F32 alpha);


extern void renderWallOutline(WallItem *wallItem, const Vector<Point> *outline, const Color *color, 
                              F32 currentScale, bool snappingToWallCornersEnabled);

extern void drawLetter(char letter, const Point &pos, const Color &color, F32 alpha);
extern void renderCircleSpawn(const Point &pos, F32 scale = 1.0);
extern void renderSpawn(const Point &pos, F32 scale, const Color *color);
extern void renderFlightPlan(const Point &from, const Point &to, const Vector<Point> &flightPlan);
extern void renderHeavysetArrow(const Point &pos, const Point &dest, const Color &color, bool isSelected, bool isLitUp);

};

#else

// for ZAP_DEDICATED, we will just define blank functions, and don't compile gameObjectRender.cpp

#define renderSoccerBall
#define renderNexus
#define renderTextItem
#define renderSpeedZone
#define renderSlipZone
#define renderProjectile
#define renderGrenade
#define renderMine
#define renderSpyBug
#define renderLoadoutZone
#define renderZone
#define renderGoalZone
#define renderForceFieldProjector
#define renderForceField
#define renderTurret
#define renderSquareItem
#define renderNavMeshZone
#define renderNavMeshBorders
#define renderRepairItem
#define renderEnergyItem
#define renderAsteroid
#define renderCore
#define renderWorm
#define renderTestItem
#define renderResourceItem
#define renderFlag
#define renderWallFill
#define renderWallEdges
#define renderPolygonFill
#define renderPolygonOutline
#define drawCircle
#define drawSquare


#endif
#endif
