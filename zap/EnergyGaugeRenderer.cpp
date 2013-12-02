//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "EnergyGaugeRenderer.h"
#include "DisplayManager.h"
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
   const F32 yul = F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - GaugeBottomMargin - GaugeHeight);

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
