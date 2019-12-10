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
#include "OpenglUtils.h"

#include "gameObjectRender.h"    // For drawHorizLine


#ifdef SHOW_SERVER_SITUATION
#  include "GameManager.h"
#  include "ServerGame.h"
#endif


namespace Zap {
   namespace UI {

      void GaugeRenderer::render(S32 energy, F32 health)
      {
         S32 margin = 6;
         EnergyGaugeRenderer::render(energy, GaugeRenderer::getGaugeBottom() - HealthGaugeRenderer::GaugeHeight - EnergyGaugeRenderer::GaugeHeight - margin);
         HealthGaugeRenderer::render(health, GaugeRenderer::getGaugeBottom());

         // Bar ends
         glColor(Colors::white);
         S32 extension = 4;
         S32 top = getGaugeBottom() - EnergyGaugeRenderer::GaugeHeight - HealthGaugeRenderer::GaugeHeight - margin - extension;
         S32 bottom = getGaugeBottom() + 2 * extension;     // Why 2 * ???

         drawVertLine(GaugeLeftMargin,              bottom, top);
         drawVertLine(GaugeLeftMargin + GaugeWidth, bottom, top);
      }


      static void renderBar(const Color &color1, const Color &color2, F32 *vertices, S32 vertex_count)
      {
         // Create blue-cyan fade
         F32 colors[] = {
            color1.r, color1.g, color1.b, 1,   // Fade from
            color1.r, color1.g, color1.b, 1,
            color2.r, color2.g, color2.b, 1,   // Fade to
            color2.r, color2.g, color2.b, 1,
         };
         renderColorVertexArray(vertices, colors, vertex_count / 2, GL_TRIANGLE_FAN);
      }


      void EnergyGaugeRenderer::render(S32 energy, S32 gaugeBottom)
      {
         static const S32 SafetyLineExtend = 4;      // How far the safety line extends above/below the main bar
         //static const S32 GaugeBottomMargin = UserInterface::vertMargin + 10;


         const F32 xul = F32(GaugeLeftMargin);
         const F32 yul = F32(gaugeBottom);

         F32 portion = F32(energy) / F32(Ship::EnergyMax);
         F32 full = portion * GaugeWidth;

         // Main bar outline
         F32 vertices[] = {
            xul,        yul,
            xul,        yul + GaugeHeight,
            xul + full, yul + GaugeHeight,
            xul + full, yul,
         };

         renderBar(Colors::blue, Colors::cyan, vertices, ARRAYSIZE(vertices));


         // Show safety line
         S32 cutoffx = Ship::EnergyCooldownThreshold * GaugeWidth / Ship::EnergyMax;

         glColor(Colors::yellow);
         drawVertLine(xul + cutoffx, yul - SafetyLineExtend - 1, yul + GaugeHeight + SafetyLineExtend);

#ifdef SHOW_SERVER_SITUATION
         ServerGame *serverGame = GameManager::getServerGame();

         if((serverGame && serverGame->getClientInfo(0)->getConnection()->getControlObject()))
         {
            S32 actDiff = static_cast<Ship *>(serverGame->getClientInfo(0)->getConnection()->getControlObject())->getEnergy();
            S32 p = F32(actDiff) / Ship::EnergyMax * GuageWidth;
            glColor(Colors::magenta);
            drawVertLine(xul + p, yul - SafetyLineExtend - 1, yul + GaugeHeight + SafetyLineExtend);
         }
#endif
         //GaugeRenderer::render((F32)energy, Ship::EnergyMax, Colors::blue, Colors::cyan,
               //GaugeBottomMargin, GaugeHeight, Ship::EnergyCooldownThreshold);
      }


      void HealthGaugeRenderer::render(F32 health, S32 gaugeBottom)
      {
         static const S32 GaugeBottomMargin = UserInterface::vertMargin - 5;


         const F32 xul = F32(GaugeLeftMargin);
         const F32 yul = gaugeBottom;

         F32 full = health * GaugeWidth;

         // Main bar outline
         F32 vertices[] = {
            xul,        yul,
            xul,        yul + GaugeHeight,
            xul + full, yul + GaugeHeight,
            xul + full, yul,
         };

         renderBar(Colors::red, Colors::white, vertices, ARRAYSIZE(vertices));
      }


   }
}      // Nested namespaces