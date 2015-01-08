//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "HealthGaugeRenderer.h"
#include "DisplayManager.h"
#include "UI.h"
#include "ship.h"

#include "Colors.h"
#include "gameObjectRender.h"
#include "OpenglUtils.h"


namespace Zap {   namespace UI {


void HealthGaugeRenderer::render(F32 health)
{
   // Coorinates of upper left corner of main guage bar
   const F32 xul = F32(                                    GaugeLeftMargin);
   const F32 yul = F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - GaugeBottomMargin - GaugeHeight);

   F32 full = health * GuageWidth;

   // Main bar outline
   F32 vertices[] = {
         xul,        yul,
         xul,        yul + GaugeHeight,
         xul + full, yul + GaugeHeight,
         xul + full, yul,
   };

   // For readability
   const Color red = Colors::red;
   const Color other = Colors::paleRed;

   // Create fade
   static const F32 colors[] = {
         red.r, red.g, red.b, 1,   // Fade from
         red.r, red.g, red.b, 1,
         other.r, other.g, other.b, 1,   // Fade to
         other.r, other.g, other.b, 1,
   };
   renderColorVertexArray(vertices, colors, ARRAYSIZE(vertices) / 2, GL_TRIANGLE_FAN);

   // Guage outline
   glColor(Colors::white);
   drawVertLine(xul,              yul, yul + GaugeHeight);
   drawVertLine(xul + GuageWidth, yul, yul + GaugeHeight);
}


} }      // Nested namespaces
