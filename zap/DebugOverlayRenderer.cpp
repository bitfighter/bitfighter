//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "DebugOverlayRenderer.h"

#include "BfObject.h"
#include "BotNavMeshZone.h"
#include "ClientGame.h"
#include "Colors.h"
#include "DisplayManager.h"
#include "EventManager.h"
#include "Rect.h"
#include "RenderUtils.h"
#include "Renderer.h"
#include "ServerGame.h"
#include "UI.h"
#include "game.h"
#include "robot.h"

namespace Zap
{

void DebugOverlayRenderer::toggleShowingShipCoords() { mDebugShowShipCoords = !mDebugShowShipCoords; }
void DebugOverlayRenderer::toggleShowingObjectIds()  { mDebugShowObjectIds  = !mDebugShowObjectIds;  }
void DebugOverlayRenderer::toggleShowingMeshZones()  { mDebugShowMeshZones  = !mDebugShowMeshZones;  }
void DebugOverlayRenderer::toggleShowDebugBots()     { mShowDebugBots       = !mShowDebugBots;       }

bool DebugOverlayRenderer::isShowingDebugShipCoords() const { return mDebugShowShipCoords; }
bool DebugOverlayRenderer::renderingObjectIds() const    { return mDebugShowObjectIds; }
bool DebugOverlayRenderer::renderingMeshZones() const    { return mDebugShowMeshZones; }
bool DebugOverlayRenderer::renderingBotPaths() const     { return mShowDebugBots; }


void DebugOverlayRenderer::appendBotPaths(ClientGame *game, Vector<BfObject *> &renderObjects) const
{
   ServerGame *serverGame = game->getServerGame();

   if(serverGame)
      for(S32 i = 0; i < serverGame->getBotCount(); i++)
         renderObjects.push_back(serverGame->getBot(i));
}


void DebugOverlayRenderer::populateRenderZones(ClientGame *game, const Rect *extentRect)
{
   mRawRenderObjects.clear();

   if(extentRect)
      game->getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, mRawRenderObjects, *extentRect);
   else
      game->getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, mRawRenderObjects);

   mRenderZones.clear();
   for(S32 i = 0; i < mRawRenderObjects.size(); i++)
      mRenderZones.push_back(static_cast<BotNavMeshZone *>(mRawRenderObjects[i]));
}


void DebugOverlayRenderer::renderDebugStatus() const
{
   // When bots are frozen, render large pause icon in lower left
   if(EventManager::get()->isPaused())
   {
      Renderer::get().setColor(Colors::white);

      const S32 PAUSE_HEIGHT = 30;
      const S32 PAUSE_WIDTH = 10;
      const S32 PAUSE_GAP = 6;
      const S32 BOX_INSET = 5;

      const S32 TEXT_SIZE = 15;
      const char *TEXT = "STEP: Alt-], Ctrl-]";

      S32 x, y;

      // Draw box
      x = DisplayManager::getScreenInfo()->getGameCanvasWidth() - UserInterface::horizMargin - 2 * (PAUSE_WIDTH + PAUSE_GAP) - BOX_INSET - getStringWidth(TEXT_SIZE, TEXT);
      y = UserInterface::vertMargin + PAUSE_HEIGHT;

      // Draw Pause symbol
      drawFilledRect(x, y, x + PAUSE_WIDTH, y - PAUSE_HEIGHT, Colors::black, Colors::white);

      x += PAUSE_WIDTH + PAUSE_GAP;
      drawFilledRect(x, y, x + PAUSE_WIDTH, y - PAUSE_HEIGHT, Colors::black, Colors::white);

      x += PAUSE_WIDTH + PAUSE_GAP + BOX_INSET;

      y -= TEXT_SIZE + (PAUSE_HEIGHT - TEXT_SIZE) / 2 + 1;
      drawString(x, y, TEXT_SIZE, TEXT);
   }
}

void DebugOverlayRenderer::renderObjectIds(const ClientGame *game) const
{
   TNLAssert(game->isTestServer(), "Will crash on non server!");
   if(!game->isTestServer())
      return;

   const Vector<DatabaseObject *> *objects = Game::getServerGameObjectDatabase()->findObjects_fast();

   for(S32 i = 0; i < objects->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objects->get(i));
      static const S32 height = 13;

      // ForceFields don't have a geometry.  When I gave them one, they just rendered the ID at the
      // exact same location as their owning projector - so we'll just skip them
      if(obj->getObjectTypeNumber() == ForceFieldTypeNumber)
         continue;

      S32 id = obj->getUserAssignedId();     // Ids assigned in the editor are positive, auto-assigned ones are negative
      S32 width = getStringWidthf(height, "[%d]", id);

      F32 x = obj->getPos().x;
      F32 y = obj->getPos().y;

      Renderer::get().setColor(Colors::black);
      drawFilledRect(x - 1, y - 1, x + width + 1, y + height + 1);

      Renderer::get().setColor(Colors::gray70);
      drawStringf(x, y, height, "[%d]", id);
   }
}


void DebugOverlayRenderer::renderMeshZones(S32 layer) const
{
   for(S32 i = 0; i < mRenderZones.size(); i++)
      mRenderZones[i]->renderLayer(layer);
}

}
