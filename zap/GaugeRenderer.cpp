//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GaugeRenderer.h"
#include "DisplayManager.h"
#include "UI.h"
#include "ship.h"

#include "Colors.h"
#include "RenderUtils.h"

#ifdef SHOW_SERVER_SITUATION
#  include "GameManager.h"
#endif


namespace Zap {   namespace UI {

void GaugeRenderer::render(F32 ether, F32 maxEther, const Color &fromColor, const Color &toColor,
      S32 bottomMargin, S32 height, F32 safetyThresh)
{
   // Coordinates of upper left corner of main guage bar
   const F32 xul = F32(GaugeLeftMargin);
   const F32 yul = F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - bottomMargin - height);

   F32 full = ether / maxEther * GaugeWidth;

   // Main bar outline
   RenderUtils::drawRectHorizGradient(xul, yul, full, height, fromColor, 1.0f, toColor, 1.0f);

   // Gauge outline
   RenderUtils::drawVertLine(xul, yul, yul + height, Colors::white);
   RenderUtils::drawVertLine(xul + GaugeWidth, yul, yul + height, Colors::white);

   // Show safety line... or not as the case may be
   if(safetyThresh == 0)
      return;

   F32 cutoffx = safetyThresh * GaugeWidth / maxEther;
   RenderUtils::drawVertLine(xul + cutoffx, yul - SafetyLineExtend - 1, yul + height + SafetyLineExtend, Colors::yellow);
}


void EnergyGaugeRenderer::render(S32 energy)
{
   GaugeRenderer::render((F32)energy, Ship::EnergyMax, Colors::blue, Colors::cyan,
         GaugeBottomMargin, GaugeHeight, Ship::EnergyCooldownThreshold);

#ifdef SHOW_SERVER_SITUATION
   ServerGame *serverGame = GameManager::getServerGame();

   if((serverGame && serverGame->getClientInfo(0)->getConnection()->getControlObject()))
   {
      S32 actDiff = static_cast<Ship *>(serverGame->getClientInfo(0)->getConnection()->getControlObject())->getEnergy();
      S32 p = F32(actDiff) / Ship::EnergyMax * GaugeWidth;
      drawVertLine(xul + p, yul - SafetyLineExtend - 1, yul + GaugeHeight + SafetyLineExtend, Colors::magenta);

      //Or, perhaps, just this:
      //renderGauge(energy, Ship::EnergyMax, Colors::blue, Colors::cyan, GaugeBottomMargin, GaugeHeight);
   }
#endif
}


void HealthGaugeRenderer::render(F32 health)
{
   static const S32 GaugeHeight = 5;
   static const S32 GaugeBottomMargin = UserInterface::vertMargin;

   GaugeRenderer::render(health, 1, Colors::red, Colors::paleRed, GaugeBottomMargin, GaugeHeight);
}


} }      // Nested namespaces
