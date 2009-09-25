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

#include "../tnl/tnl.h"

#include "point.h"
#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{
extern void glVertex(Point p);
extern void glColor(Color c, float alpha = 1);
extern void drawCircle(Point pos, F32 radius);
extern void drawFilledCircle(Point pos, F32 radius);
extern void drawFilledSector(Point pos, F32 radius, F32 start, F32 end);

extern void drawRoundedRect(Point pos, F32 width, F32 height, F32 radius);
extern void drawArc(Point pos, F32 radius, F32 startAngle, F32 endAngle);
extern void drawEllipse(Point pos, F32 width, F32 height, F32 angle);
extern void drawFilledEllipse(Point pos, F32 width, F32 height, F32 angle);
extern void drawPolygon(Point pos, S32 sides, F32 radius, F32 angle);


extern void renderCenteredString(Point pos, U32 size, const char *string);
extern void renderShip(Color c, F32 alpha, F32 thrusts[], F32 health, F32 radius, bool cloakActive, bool shieldActive);
extern void renderAimVector();
extern void renderTeleporter(Point pos, U32 type, bool in, S32 time, F32 radiusFraction, F32 radius, F32 alpha, Vector<Point> dests, bool showDestOverride);
extern void renderTurret(Color c, Point anchor, Point normal, bool enabled, F32 health, F32 barrelAngle, F32 aimOffset);

extern void renderFlag(Point pos, Color flagColor);
extern void renderFlag(Point pos, Color flagColor, Color mastColor);

//extern void renderFlag(Point pos, Color c, F32 timerFraction);
extern void renderSmallFlag(Point pos, Color c, F32 parentAlpha);


extern void renderLoadoutZone(Color c, Vector<Point> &outline, Vector<Point> &fill, Point centroid, F32 labelAngle);
extern void renderNexus(Vector<Point> &outline, Vector<Point> &fill, Point centroid, F32 labelAngle, bool open, F32 glowFraction);

extern void renderGoalZone(Vector<Point> &outline, Vector<Point> &fill, Point centroid, Color c, F32 labelAngle, bool isFlashing, F32 glowFraction);
extern void renderSlipZone(Vector<Point> &bounds, Rect extent);

extern void renderPolygonLabel(Point centroid, F32 angle, F32 size, const char *text);


extern void renderProjectile(Point pos, U32 type, U32 time);

extern void renderMine(Point pos, bool armed, bool visible);
extern void renderGrenade(Point pos, F32 vel);
extern void renderSpyBug(Point pos, bool visible);

extern void renderRepairItem(Point pos);
extern void renderRepairItem(Point pos, bool forEditor, Color overrideColor);
//extern void renderSpeedZone(Point pos, Point normal, U32 time);
void renderSpeedZone(Point pos, Point dir, U32 time);

void renderTestItem(Point pos);
void renderAsteroid(Point pos, S32 design, F32 radius);
void renderAsteroid(Point pos, S32 design, F32 radius, Color c);
void renderResourceItem(Point pos);
void renderSoccerBall(Point pos);
void renderTextItem(Point pos, Point dir, U32 size, S32 team, string text);

extern void renderForceFieldProjector(Point pos, Point normal);
extern void renderForceFieldProjector(Point pos, Point normal, Color teamColor, bool enabled);
extern void renderForceField(Point start, Point end, Color c, bool fieldUp);

extern void renderBitfighterLogo(S32 yPos, F32 scale, F32 angle, U32 mask = 1023);
extern void renderStaticBitfighterLogo();

};

#endif

