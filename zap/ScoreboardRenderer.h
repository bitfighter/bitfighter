//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SCOREBOARD_RENDERER_H_
#define _SCOREBOARD_RENDERER_H_

#include "UI.h"
#include "UILevelInfoDisplayer.h"
#include "HelperManager.h"   
#include "LoadoutIndicator.h"
#include "TimeLeftRenderer.h"
#include "FpsRenderer.h"
#include "ConnectionStatsRenderer.h"
#include "HelpItemManager.h"
#include "move.h"
#include "config.h"     // For UserSettings def
#include "ClientGame.h"

#include "SoundEffect.h"
#include "sparkManager.h"

#include "Color.h"
#include "Rect.h"

#include "tnlString.h"
#include "tnlTypes.h"


namespace Zap
{


using namespace Zap::UI;

class ScoreboardRenderer
{

public:
   static void renderScoreboard(ClientGame *clientGame);

};


}

#endif

