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
 
#include "EnergyGaugeRenderer.h"
#include "ScreenInfo.h"
#include "UI.h"
#include "ship.h"

#include "Colors.h"
#include "gameObjectRender.h"
#include "OpenglUtils.h"


namespace Zap {   namespace UI {


void EnergyGaugeRenderer::render(S32 energy)
{
   // Coorinates of upper left corner of main guage bar
   const F32 xul = F32(                                    GaugeLeftMargin);
   const F32 yul = F32(gScreenInfo.getGameCanvasHeight() - GaugeBottomMargin - GaugeHeight);

   F32 full = F32(energy) / F32(Ship::EnergyMax) * GuageWidth;

   // Main bar outline
   F32 vertices[] = {
         xul,        yul,
         xul,        yul + GaugeHeight,
         xul + full, yul + GaugeHeight,
         xul + full, yul,
   };

   // For readability
   const Color blue = Colors::blue;
   const Color cyan = Colors::cyan;

   // Create blue-cyan fade
   static const F32 colors[] = {
         blue.r, blue.g, blue.b, 1,   // Fade from
         blue.r, blue.g, blue.b, 1,
         cyan.r, cyan.g, cyan.b, 1,   // Fade to
         cyan.r, cyan.g, cyan.b, 1,
   };
   renderColorVertexArray(vertices, colors, ARRAYSIZE(vertices) / 2, GL_TRIANGLE_FAN);

   // Guage outline
   glColor(Colors::white);
   drawVertLine(xul,              yul, yul + GaugeHeight);
   drawVertLine(xul + GuageWidth, yul, yul + GaugeHeight);

   // Show safety line
   S32 cutoffx = Ship::EnergyCooldownThreshold * GuageWidth / Ship::EnergyMax;

   glColor(Colors::yellow);
   drawVertLine(xul + cutoffx, yul - SafetyLineExtend - 1, yul + GaugeHeight + SafetyLineExtend);

#ifdef SHOW_SERVER_SITUATION
   if((gServerGame && gServerGame->getClientInfo(0)->getConnection()->getControlObject()))
   {
      S32 actDiff = static_cast<Ship *>(gServerGame->getClientInfo(0)->getConnection()->getControlObject())->getEnergy();
      S32 p = F32(actDiff) / Ship::EnergyMax * GuageWidth;
      glColor(Colors::magenta);
      drawVertLine(xul + p, yul - SafetyLineExtend - 1, yul + GaugeHeight + SafetyLineExtend);
   }
#endif
}


} }      // Nested namespaces
