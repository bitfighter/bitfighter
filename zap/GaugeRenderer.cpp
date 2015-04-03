//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GaugeRenderer.h"
#include "DisplayManager.h"
#include "UI.h"
#include "ship.h"

#include "Colors.h"
#include "GameObjectRender.h"
#include "RenderUtils.h"

#ifdef SHOW_SERVER_SITUATION
#  include "GameManager.h"
#endif


namespace Zap {   namespace UI {

void GaugeRenderer::render(F32 ether, F32 maxEther, const F32 colors[], S32 bottomMargin, S32 height, F32 safetyThresh)
{
   // Coordinates of upper left corner of main guage bar
   const F32 xul = F32(GaugeRenderer::GaugeLeftMargin);
   const F32 yul = F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - bottomMargin - height);

   F32 full = ether / maxEther * GaugeRenderer::GaugeWidth;

   // Main bar outline
   F32 vertices[] = {
      xul,        yul,
      xul,        yul + height,
      xul + full, yul + height,
      xul + full, yul,
   };

   mGL->renderColorVertexArray(vertices, colors, ARRAYSIZE(vertices) / 2, GLOPT::TriangleFan);

   // Gauge outline
   mGL->glColor(Colors::white);
   RenderUtils::drawVertLine(xul, yul, yul + height);
   RenderUtils::drawVertLine(xul + GaugeWidth, yul, yul + height);

   // Show safety line... or not as the case may be
   if(safetyThresh >= 0)
   {
      F32 cutoffx = safetyThresh * GaugeWidth / maxEther;

      mGL->glColor(Colors::yellow);
      RenderUtils::drawVertLine(xul + cutoffx, yul - SafetyLineExtend - 1, yul + height + SafetyLineExtend);
   }
}


void EnergyGaugeRenderer::render(S32 energy)
{
   // Create fade
   static const F32 colors[] = {
      Colors::blue.r, Colors::blue.g, Colors::blue.b, 1,   // Fade from
      Colors::blue.r, Colors::blue.g, Colors::blue.b, 1,
      Colors::cyan.r, Colors::cyan.g, Colors::cyan.b, 1,   // Fade to
      Colors::cyan.r, Colors::cyan.g, Colors::cyan.b, 1,
   };

   GaugeRenderer::render(energy, Ship::EnergyMax, colors, GaugeBottomMargin, GaugeHeight, Ship::EnergyCooldownThreshold);

#ifdef SHOW_SERVER_SITUATION
   ServerGame *serverGame = GameManager::getServerGame();

   if((serverGame && serverGame->getClientInfo(0)->getConnection()->getControlObject()))
   {
      S32 actDiff = static_cast<Ship *>(serverGame->getClientInfo(0)->getConnection()->getControlObject())->getEnergy();
      S32 p = F32(actDiff) / Ship::EnergyMax * GaugeWidth;
      glColor(Colors::magenta);
      drawVertLine(xul + p, yul - SafetyLineExtend - 1, yul + GaugeHeight + SafetyLineExtend);

      //Or, perhaps, just this:
      //renderGauge(energy, Ship::EnergyMax, Colors::blue, Colors::cyan, GaugeBottomMargin, GaugeHeight);
   }
#endif
}


void HealthGaugeRenderer::render(F32 health)
{
   static const S32 GaugeHeight = 5;
   static const S32 GaugeBottomMargin = UserInterface::vertMargin;

   // Create fade
   static const F32 colors[] = {
      Colors::red.r,     Colors::red.g,     Colors::red.b,     1,   // Fade from
      Colors::red.r,     Colors::red.g,     Colors::red.b,     1,
      Colors::paleRed.r, Colors::paleRed.g, Colors::paleRed.b, 1,   // Fade to
      Colors::paleRed.r, Colors::paleRed.g, Colors::paleRed.b, 1,
   };

   GaugeRenderer::render(health, 1, colors, GaugeBottomMargin, GaugeHeight);
}


} }      // Nested namespaces
