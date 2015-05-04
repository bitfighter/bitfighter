//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAMEOBJECTRENDER_H_
#define _GAMEOBJECTRENDER_H_

#include "tnl.h"

#include "RenderManager.h"
#include "Geometry.h"

#include "Color.h"
#include "Rect.h"
#include "SharedConstants.h"     // For MeritBadges enum
#include "ShipShape.h"

#include "BfObject.h"            // Need to use BfObject with SafePtr.  Whether this is really needed is a different question.

#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{

static const S32 NO_NUMBER = -1;

class Ship;
class WallItem;
class NeighboringZone;
struct PanelGeom;


class GameObjectRender: RenderManager
{
public:
   GameObjectRender();
   virtual ~GameObjectRender();

   // Some things we use internally, but also need from UIEditorInstructions for consistency
   static const Color BORDER_FILL_COLOR;
   static const F32 BORDER_FILL_ALPHA;
   static const F32 BORDER_WIDTH;
   static F32 DEFAULT_LINE_WIDTH;

   //////////
   // Primitives
   static void renderCentroidMark(const Point &pos, F32 radius);

   static void renderUpArrow(const Point &center, S32 size);
   static void renderDownArrow(const Point &center, S32 size);
   static void renderLeftArrow(const Point &center, S32 size);
   static void renderRightArrow(const Point &center, S32 size);

   static void renderNumberInBox(const Point pos, S32 number, F32 scale);

   static void renderHexScale(const Point &center, F32 radius);

   static void renderSmallSolidVertex(F32 currentScale, const Point &pos, bool snapping);

   static void renderVertex(char style, const Point &v, S32 number);
   static void renderVertex(char style, const Point &v, S32 number,           F32 scale);
   static void renderVertex(char style, const Point &v, S32 number,           F32 scale, F32 alpha);
   static void renderVertex(char style, const Point &v, S32 number, S32 size, F32 scale, F32 alpha);

   static void renderSquareItem(const Point &pos, const Color &c, F32 alpha, const Color &letterColor, char letter);

   static void drawDivetedTriangle(F32 height, F32 len);
   static void drawGear(const Point &center, S32 teeth, F32 r1, F32 r2, F32 ang1, F32 ang2, F32 innerCircleRadius, F32 angleRadians = 0.0f);


   //////////
   // Some things for rendering on screen display
   static F32 renderCenteredString(const Point &pos, S32 size, const char *string);
   static F32 renderCenteredString(const Point &pos, F32 size, const char *string);

   static void renderHealthBar(F32 health, const Point &center, const Point &dir, F32 length, F32 width);
   static void renderActiveModuleOverlays(F32 alpha, F32 radius, U32 sensorTime, bool shieldActive,
         bool sensorActive, bool repairActive, bool hasArmor);
   static void renderShipFlame(ShipFlame *flames, S32 flameCount, F32 thrust, F32 alpha, bool yThruster);
   static void renderShipName(const string &shipName, bool isAuthenticated, bool isBusy,
         U32 killStreak, U32 gamesPlayed, F32 nameScale, F32 alpha);

   // Renders the core ship, good for instructions and such
   static void renderShip(ShipShape::ShipShapeType shapeType, const Color &shipColor, F32 alpha, F32 thrusts[],
                          F32 health, F32 radius, U32 sensorTime,
                          bool shieldActive, bool sensorActive, bool repairActive, bool hasArmor);

   // Renders the ship and all the fixins
   static void renderShip(S32 layerIndex, const Point &renderPos, const Point &actualPos, const Point &vel,
                          F32 angle, F32 deltaAngle, ShipShape::ShipShapeType shape, const Color &color, F32 alpha,
                          U32 renderTime, const string &shipName, F32 nameScale, F32 warpInScale, bool isLocalShip, bool isBusy,
                          bool isAuthenticated, bool showCoordinates, F32 health, F32 radius, S32 team,
                          bool boostActive, bool shieldActive, bool repairActive, bool sensorActive,
                          bool hasArmor, bool engineeringTeleport, U32 killStreak, U32 gamesPlayed);

   static void renderSpawnShield(const Point &pos, U32 shieldTime, U32 renderTime);

   // Render repair rays to all the repairing objects
   static void renderShipRepairRays(const Point &pos, const Ship *ship, Vector<SafePtr<BfObject> > &repairTargets, F32 alpha);

   static void renderShipCoords(const Point &coords, bool localShip, F32 alpha);

   static void drawFourArrows(const Point &pos);

   static void renderTeleporter(const Point &pos, U32 type, bool spiralInwards, U32 time, F32 zoomFraction, F32 radiusFraction, F32 radius, F32 alpha,
                                const Vector<Point> *dests, U32 trackerCount = 100);
   static void renderTeleporterOutline(const Point &center, F32 radius, const Color &color);
   static void renderSpyBugVisibleRange(const Point &pos, const Color &color, F32 currentScale = 1);
   static void renderTurretFiringRange(const Point &pos, const Color &color, F32 currentScale);
   static void renderTurret(const Color &c, Point anchor, Point normal, bool enabled, F32 health, F32 barrelAngle, S32 healRate = 0);
   static void renderMortar(const Color &c, Point anchor, Point normal, bool enabled, F32 health, S32 healRate = 0);

   static void renderFlag(const Color &flagColor, const Color &mastColor, F32 alpha);
   static void renderFlag(const Color &flagColor, F32 alpha);
   static void renderFlag(const Point &pos, const Color &flagColor, F32 alpha = 1);
   static void renderFlag(const Point &pos, F32 scale, const Color &flagColor);
   static void renderFlag(F32 x, F32 y, const Color &flagColor);
   static void renderFlag(const Color &flagColor);
   static void renderFlag(const Point &pos, const Color &flagColor, const Color &mastColor, F32 alpha);
   static void doRenderFlag(F32 x, F32 y, F32 scale, const Color *flagColor, const Color *mastColor, F32 alpha);
   static void doRenderFlag(F32 x, F32 y, F32 scale, const Color &flagColor, const Color &mastColor, F32 alpha);
   static void doRenderFlag(F32 x, F32 y, F32 scale, const Color &flagColor, F32 alpha);


   //static void renderFlag(Point pos, Color c, F32 timerFraction);
   static void renderSmallFlag(const Point &pos, const Color &c, F32 parentAlpha);
   static void renderFlagSpawn(const Point &pos, F32 currentScale, const Color &color);

   static void renderZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill);

   static void renderLoadoutZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill,
                                 const Point &centroid, F32 angle, F32 scaleFact = 1);

   static void renderLoadoutZoneIcon(const Point &center, S32 outerRadius, F32 angleRadians = 0.0f);

   static void renderNavMeshZone(const Vector<Point> *outline, const Vector<Point> *fill,
                                 const Point &centroid, S32 zoneId);

   static void renderNavMeshBorders(const Vector<NeighboringZone> &borders);

   static void renderStars(const Point *stars, const Color *colors, S32 numStars, F32 alphaFrac, const Point &cameraPos, const Point &visibleExtent);

   static void drawObjectiveArrow(const Point &nearestPoint, F32 zoomFraction, const Color &outlineColor,
                                  S32 canvasWidth, S32 canvasHeight, F32 alphaMod, F32 highlightAlpha);

   static void renderScoreboardOrnamentTeamFlags(S32 xpos, S32 ypos, const Color &color, bool teamHasFlag);

   static void renderPolygonOutline(const Vector<Point> *outline);
   static void renderPolygonOutline(const Vector<Point> *outlinePoints, const Color &outlineColor, F32 alpha = 1, F32 lineThickness = DEFAULT_LINE_WIDTH);
   static void renderPolygonFill(const Vector<Point> *fillPoints, const Color &fillColor, F32 alpha = 1);
   static void renderPolygon(const Vector<Point> *fillPoints, const Vector<Point> *outlinePoints,
                             const Color &fillColor, const Color &outlineColor, F32 alpha = 1);

   static void renderGoalZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill);     // No label version
   static void renderGoalZone(const Color &c, const Vector<Point> *outline, const Vector<Point> *fill, Point centroid, F32 labelAngle,
                              bool isFlashing, F32 glowFraction, S32 score, F32 flashCounter, GoalZoneFlashStyle flashStyle);
   static void renderGoalZoneIcon(const Point &center, S32 radius);


   static void renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, Point centroid, F32 labelAngle,
                           bool open, F32 glowFraction);

   static void renderNexus(const Vector<Point> *outline, const Vector<Point> *fill, bool open, F32 glowFraction);
   static void renderNexusIcon(const Point &center, S32 radius, F32 angleRadians = 0.0f);


   static void renderSlipZone(const Vector<Point> *bounds, const Vector<Point> *boundsFill, const Point &centroid);
   static void renderSlipZoneIcon(const Point &center, S32 radius, F32 angleRadians = 0.0f);

   static void renderPolygonLabel(const Point &centroid, F32 angle, F32 size, const char *text, F32 scaleFact = 1);

   static void renderProjectile(const Point &pos, U32 type, U32 time);
   static void renderSeeker(const Point &pos, F32 angleRadians, F32 speed, U32 timeRemaining);

   static void renderMine(const Point &pos, bool armed, bool visible);
   static void renderGrenade(const Point &pos, F32 lifeLeft);
   static void renderSpyBug(const Point &pos, const Color &teamColor, bool visible, bool drawOutline);

   static void renderRepairItem(const Point &pos);
   static void renderRepairItem(const Point &pos, bool forEditor, const Color *overrideColor, F32 alpha);

   static void renderEnergyItem(const Point &pos);

   static void renderWallFill(const Vector<Point> *points, const Color &fillColor, bool polyWall);
   static void renderWallFill(const Vector<Point> *points, const Color &fillColor, const Point &offset, bool polyWall);

   static void renderEnergyItem(const Point &pos, bool forEditor);
   static void renderEnergySymbol();                                   // Render lightning bolt symbol
   static void renderEnergySymbol(const Point &pos, F32 scaleFactor);  // Another signature

   // Wall rendering
   static void renderWallEdges(const Vector<Point> &edges, const Color &outlineColor, F32 alpha = 1.0);
   static void renderWallEdges(const Vector<Point> &edges, const Point &offset, const Color &outlineColor, F32 alpha = 1.0);

   //static void renderSpeedZone(Point pos, Point normal, U32 time);
   static void renderSpeedZone(const Vector<Point> &pts);

   static void renderTestItem(const Point &pos, S32 size, F32 alpha = 1);
   //static void renderTestItem(const Point &pos, S32 size, F32 alpha = 1);
   static void renderTestItem(const Vector<Point> &points, F32 alpha = 1);

   static void renderAsteroid(const Point &pos, S32 design, F32 scaleFact);
   static void renderAsteroid(const Point &pos, S32 design, F32 scaleFact, const Color *color, F32 alpha = 1);

   static void renderAsteroidSpawn(const Point &pos, S32 time);
   static void renderAsteroidSpawnEditor(const Point &pos, F32 scale = 1.0);

   static void renderResourceItem(const Vector<Point> &points, F32 alpha = 1);
   //static void renderResourceItem(const Point &pos, F32 scaleFactor, const Color *color, F32 alpha);

   static void renderCore(const Point &pos, const Color &coreColor, U32 time,
                   const PanelGeom *panelGeom, const F32 panelHealth[10], F32 panelStartingHealth);

   static void renderCoreSimple(const Point &pos, const Color &coreColor, S32 width);

   static void renderSoccerBall(const Point &pos, F32 size);
   static void renderSoccerBall(const Point &pos);
   static void renderLock();

   static void renderTextItem(const Point &pos, const Point &dir, F32 size, const string &text, const Color &color);

   // Editor support items
   static void renderPolyLineVertices(const BfObject *obj, bool snapping, F32 currentScale);
   static void renderGrid(F32 currentScale, const Point &offset, const Point &origin, F32 gridSize, bool fadeLines, bool showMinorGridLines);

   static void renderForceFieldProjector(const Point &pos, const Point &normal, const Color &teamColor, bool enabled, S32 healRate);
   static void renderForceFieldProjector(const Vector<Point> *geom, const Point &pos, const Color &teamColor, bool enabled, S32 healRate = 0);
   static void renderForceField(Point start, Point end, const Color &c, bool fieldUp, F32 scale = 1);

   static void renderBitfighterLogo(U32 mask);
   static void renderBitfighterLogo(S32 yPos, F32 scale, U32 mask = 1023);
   static void renderBitfighterLogo(const Point &pos, F32 size, U32 letterMask = 1023);
   static void renderBitfighterLogo(const Point &pos, const Point &dir, F32 size);

   static void renderStaticBitfighterLogo();

   static void renderBadge(F32 x, F32 y, F32 rad, MeritBadges badge);

   static void renderWalls(const Vector<DatabaseObject *> *walls,
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
                           F32 alpha);

   static void renderShadowWalls(const Vector<BfObject *> &objects);

   static void renderWallSpine(const WallItem *wallItem, const Vector<Point> *outline, const Color &color,
                               F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false);

   static void renderWallSpine(const WallItem *wallItem, const Vector<Point> *outline,
                               F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false);

   static void renderSpawn(const Point &pos, F32 scale, const Color &color);
   static void renderFlightPlan(const Point &from, const Point &to, const Vector<Point> &flightPlan);
   static void renderHeavysetArrow(const Point &pos, const Point &dest, const Color &color, bool isSelected, bool isLitUp);
   static void renderTeleporterEditorObject(const Point &pos, S32 radius, const Color &color);

   static void renderGamesPlayedMark(S32 x, S32 y, S32 height, U32 gamesPlayed);

private:
   // Individual badges
   static void render25FlagsBadge(F32 x, F32 y, F32 rad);
   static void renderDeveloperBadge(F32 x, F32 y, F32 rad);
   static void renderBBBBadge(F32 x, F32 y, F32 rad, const Color &color);
   static void renderLevelDesignWinnerBadge(F32 x, F32 y, F32 rad);
   static void renderZoneControllerBadge(F32 x, F32 y, F32 rad);
   static void renderRagingRabidRabbitBadge(F32 x, F32 y, F32 rad);
   static void renderLastSecondWinBadge(F32 x, F32 y, F32 rad);
   static void renderHatTrickBadge(F32 x, F32 y, F32 rad);

   static void renderGridLines(const Point &offset, F32 gridScale, F32 grayVal, bool fadeLines);
};


};

#endif
