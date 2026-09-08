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
#include "MapTile.h"
#include "Rect.h"
#include "RenderUtils.h"
#include "Renderer.h"
#include "ServerGame.h"
#include "UI.h"
#include "game.h"
#include "gameType.h"
#include "robot.h"

#include <cmath>
#include <map>
#include <set>

namespace Zap
{

void DebugOverlayRenderer::toggleShowingShipCoords() { mDebugShowShipCoords = !mDebugShowShipCoords; }
void DebugOverlayRenderer::toggleShowingObjectIds()  { mDebugShowObjectIds  = !mDebugShowObjectIds;  }
void DebugOverlayRenderer::toggleShowingMeshZones()  { mDebugShowMeshZones  = !mDebugShowMeshZones;  }
void DebugOverlayRenderer::toggleShowDebugBots()     { mShowDebugBots       = !mShowDebugBots;       }
void DebugOverlayRenderer::toggleShowingMapTiles()   { mDebugShowMapTiles   = !mDebugShowMapTiles;   }
void DebugOverlayRenderer::toggleShowingEdgeIds()    { mDebugShowEdgeIds    = !mDebugShowEdgeIds;    }

bool DebugOverlayRenderer::isShowingDebugShipCoords() const { return mDebugShowShipCoords; }
bool DebugOverlayRenderer::renderingObjectIds() const    { return mDebugShowObjectIds; }
bool DebugOverlayRenderer::renderingMeshZones() const    { return mDebugShowMeshZones; }
bool DebugOverlayRenderer::renderingBotPaths() const     { return mShowDebugBots; }
bool DebugOverlayRenderer::renderingMapTiles() const     { return mDebugShowMapTiles; }
bool DebugOverlayRenderer::renderingEdgeIds() const      { return mDebugShowEdgeIds; }


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
   // When bots are fr        ozen, render large pause icon in lower left
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


void DebugOverlayRenderer::renderMapTiles(const ClientGame *game) const
{
   // Access the server's GameType directly (local test server only)
   ServerGame *serverGame = game->getServerGame();
   if(!serverGame)
      return;
   GameType *gt = serverGame->getGameType();
   if(!gt)
      return;

   const Vector<MapTile> &tiles = gt->getMapTiles();
   if(tiles.size() == 0)
      return;

   Renderer &r = Renderer::get();

   // Collect unique grid coordinates from all tile bounds.
   // All tiles are the same size, so sorting and deduping gives the grid lines.
   Vector<F32> gridX, gridY;
   for(S32 i = 0; i < tiles.size(); i++)
   {
      gridX.push_back(tiles[i].bounds.min.x);
      gridX.push_back(tiles[i].bounds.max.x);
      gridY.push_back(tiles[i].bounds.min.y);
      gridY.push_back(tiles[i].bounds.max.y);
   }
   gridX.sort([](const F32 &a, const F32 &b) { return a < b; });
   gridY.sort([](const F32 &a, const F32 &b) { return a < b; });

   // Dedup within a small epsilon
   Vector<F32> uniqX, uniqY;
   for(S32 i = 0; i < gridX.size(); i++)
      if(i == 0 || gridX[i] > gridX[i-1] + 0.1f)
         uniqX.push_back(gridX[i]);

   for(S32 i = 0; i < gridY.size(); i++)
      if(i == 0 || gridY[i] > gridY[i-1] + 0.1f)
         uniqY.push_back(gridY[i]);

   // Draw the grid as horizontal and vertical lines (each edge once)
   r.setColor(Colors::cyan, 0.35f);
   r.setLineWidth(1);
   for(S32 i = 0; i < uniqX.size(); i++)
   {
      Vector<Point> line;
      line.push_back(Point(uniqX[i], uniqY[0]));
      line.push_back(Point(uniqX[i], uniqY[uniqY.size()-1]));
      r.renderPointVector(&line, RenderType::Lines);
   }
   for(S32 i = 0; i < uniqY.size(); i++)
   {
      Vector<Point> line;
      line.push_back(Point(uniqX[0], uniqY[i]));
      line.push_back(Point(uniqX[uniqX.size()-1], uniqY[i]));
      r.renderPointVector(&line, RenderType::Lines);
   }

   // Draw wall poly edges within each tile:
   //   green  = original wall edge (outline == true)
   //   red    = clip-introduced edge (outline == false, normally hidden)
   // Batch all edges into two vectors for fewer draw calls
   Vector<Point> origEdges, clipEdges;
   for(S32 i = 0; i < tiles.size(); i++)
   {
      const MapTile &tile = tiles[i];
      for(S32 j = 0; j < tile.polys.size(); j++)
      {
         const WallPoly &wp = tile.polys[j];
         const U32 numVerts = wp.numVerts();
         if(numVerts < 3)
            continue;

         for(U32 k = 0; k < numVerts; k++)
         {
            const U32 v0 = k * 2;
            const U32 v1 = ((k + 1) % numVerts) * 2;
            const bool isOrig = (k < wp.edges.size() && wp.edges[k] != EdgeStyle::None);

            Vector<Point> &batch = isOrig ? origEdges : clipEdges;

            batch.push_back(Point(wp.verts[v0], wp.verts[v0 + 1]));
            batch.push_back(Point(wp.verts[v1], wp.verts[v1 + 1]));
         }
      }
   }
   r.setColor(Colors::green, 0.8f);
   if(origEdges.size() > 0)
      r.renderPointVector(&origEdges, RenderType::Lines);
   r.setColor(Colors::red, 0.6f);
   if(clipEdges.size() > 0)
      r.renderPointVector(&clipEdges, RenderType::Lines);

   // Draw tile ID labels centered in each tile
   for(S32 i = 0; i < tiles.size(); i++)
   {
      const MapTile &tile = tiles[i];
      F32 cx = (tile.bounds.min.x + tile.bounds.max.x) / 2;
      F32 cy = (tile.bounds.min.y + tile.bounds.max.y) / 2;
      r.setColor(Colors::white);
      drawStringf(cx, cy, 12, "%d", tile.tileId);
   }
}


void DebugOverlayRenderer::renderEdgeIds(const ClientGame *game) const
{
   // Access the server's GameType directly (local test server only)
   ServerGame *serverGame = game->getServerGame();
   if(!serverGame)
      return;
   GameType *gt = serverGame->getGameType();
   if(!gt)
      return;

   const Vector<MapTile> &tiles = gt->getMapTiles();
   if(tiles.size() == 0)
      return;

   Renderer &r = Renderer::get();

   // Build the same canonical edge→ID map used by the test harness
   // (see buildEdgeIdMaps in bitfighter_test/TestMapTiling.cpp).
   static const F32 EDGE_SNAP_TOL = 0.1f;

   struct EdgeKey
   {
      F32 x1, y1, x2, y2;

      EdgeKey(F32 ax, F32 ay, F32 bx, F32 by)
      {
         F32 rx1 = roundf(ax / EDGE_SNAP_TOL) * EDGE_SNAP_TOL;
         F32 ry1 = roundf(ay / EDGE_SNAP_TOL) * EDGE_SNAP_TOL;
         F32 rx2 = roundf(bx / EDGE_SNAP_TOL) * EDGE_SNAP_TOL;
         F32 ry2 = roundf(by / EDGE_SNAP_TOL) * EDGE_SNAP_TOL;

         if(rx1 < rx2 || (rx1 == rx2 && ry1 < ry2))
            { x1 = rx1; y1 = ry1; x2 = rx2; y2 = ry2; }
         else
            { x1 = rx2; y1 = ry2; x2 = rx1; y2 = ry1; }
      }

      bool operator<(const EdgeKey &o) const
      {
         if(x1 != o.x1) return x1 < o.x1;
         if(y1 != o.y1) return y1 < o.y1;
         if(x2 != o.x2) return x2 < o.x2;
         return y2 < o.y2;
      }
   };

   // Gather unique canonical keys for visible edges
   std::set<EdgeKey> keys;
   std::map<EdgeKey, Point> keyMidpoints;  // midpoint for label placement

   for(S32 t = 0; t < tiles.size(); t++)
   {
      for(S32 p = 0; p < tiles[t].polys.size(); p++)
      {
         const WallPoly &wp = tiles[t].polys[p];
         U32 nv = wp.numVerts();

         for(U32 e = 0; e < nv; e++)
         {
            // Include ALL edges — including EdgeStyle::None (tile boundary /
            // interior overlap edges).  This is so /showedgeids shows every
            // edge the tile builder produced, helping the author identify which
            // edges are incorrectly hidden.  If an EdgeStyle::None edge should
            // actually be visible, its ID will appear on the offending edge.
            U32 v0 = e * 2;
            U32 v1 = ((e + 1) % nv) * 2;
            F32 ax = wp.verts[v0], ay = wp.verts[v0 + 1];
            F32 bx = wp.verts[v1], by = wp.verts[v1 + 1];
            EdgeKey key(ax, ay, bx, by);
            keys.insert(key);
            keyMidpoints[key] = Point((ax + bx) / 2, (ay + by) / 2);
         }
      }
   }

   // Assign IDs in sorted order and render.
   // Labels are jittered by a few pixels, colored, and size-varied so that
   // edges sharing the same midpoint (e.g. collinear seams from adjacent
   // tiles) remain individually readable.
   static const Color labelColors[] = {
      Colors::yellow,
      Colors::cyan,
      Colors::magenta,
      Colors::orange67,
      Colors::paleGreen,
      Colors::gold,
      Colors::red80,
      Colors::blue80,
      Colors::green65,
      Colors::gray70,
   };
   static const S32 NUM_COLORS = sizeof(labelColors) / sizeof(labelColors[0]);

   // Deterministic jitter offsets (world units).  A few units of offset is
   // enough to separate labels that land on the same midpoint while still
   // keeping each label visually associated with its edge.
   static const F32 JITTER_PX = 8.0f;
   static const F32 jitterAngles[] = {
      0.3f, 2.0f, -2.0f, 0.9f, -0.5f,  2.4f, -1.9f, 0.1f,  1.4f, -2.6f,
      0.6f, 2.8f, -1.1f, 1.8f, -0.8f, -3.0f, 2.2f, -2.2f, 0.4f,  1.1f,
   };
   static const S32 NUM_JITTER = sizeof(jitterAngles) / sizeof(jitterAngles[0]);

   int nextId = 0;
   for(const auto &key : keys)
   {
      Point mid = keyMidpoints[key];

      // Angular jitter: offset the label from the edge midpoint by a small,
      // deterministic amount.  Using the edge ID keeps the pattern stable
      // across frames and between the in-game display and test expectations.
      const S32 ji = nextId % NUM_JITTER;
      const F32 ang = jitterAngles[ji];
      const F32 dx = cosf(ang) * JITTER_PX;
      const F32 dy = sinf(ang) * JITTER_PX;

      const Color &col = labelColors[nextId % NUM_COLORS];
      const F32 fontSize = 12.0f + (nextId % 3);   // 12, 13, or 14

      // Draw a short leader line from the jittered label position back to the
      // edge midpoint, in the same color as the number, so it's clear which
      // segment each ID refers to.
      const F32 labelX = mid.x + dx;
      const F32 labelY = mid.y + dy;

      Vector<Point> leader;
      leader.push_back(Point(labelX, labelY));
      leader.push_back(Point(mid.x, mid.y));
      r.setColor(col, 0.85f);
      r.setLineWidth(1);
      r.renderPointVector(&leader, RenderType::Lines);

      r.setColor(col);
      drawStringf(labelX, labelY, fontSize, "%d", nextId);
      nextId++;
   }
}

}