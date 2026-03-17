//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _DEBUG_OVERLAY_RENDERER_H_
#define _DEBUG_OVERLAY_RENDERER_H_

#include "tnlVector.h"

using namespace TNL;

namespace Zap
{

class ClientGame;
class BfObject;
class BotNavMeshZone;
class DatabaseObject;
class Rect;

class DebugOverlayRenderer
{
private:
   bool mDebugShowShipCoords = false;
   bool mDebugShowObjectIds = false;
   bool mDebugShowMeshZones = false;
   bool mShowDebugBots = false;

   Vector<DatabaseObject *> mRawRenderObjects;
   Vector<BotNavMeshZone *> mRenderZones;

public:
   void toggleShowingShipCoords();
   void toggleShowingObjectIds();
   void toggleShowingMeshZones();
   void toggleShowDebugBots();

   bool isShowingDebugShipCoords() const;
   bool renderingObjectIds() const;
   bool renderingMeshZones() const;
   bool renderingBotPaths() const;

   void appendBotPaths(ClientGame *game, Vector<BfObject *> &renderObjects) const;
   void populateRenderZones(ClientGame *game, const Rect *extentRect = NULL);

   void renderDebugStatus() const;
   void renderObjectIds(const ClientGame *game) const;
   void renderMeshZones(S32 layer) const;
};

}

#endif
