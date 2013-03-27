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

#include "UILevelInfoDisplayer.h"      // Header
#include "ScreenInfo.h"
#include "Colors.h"
#include "OpenglUtils.h"               
#include "RenderUtils.h"
#include "UI.h"                        // For renderFancyBox()
#include "GameTypesEnum.h"
#include "gameType.h"



namespace Zap
{


void LevelInfoDisplayer::resetTimer()
{
   mDisplayTimer.reset(6000);  // 6 seconds
}

void LevelInfoDisplayer::idle(U32 timeDelta)
{
   mDisplayTimer.update(timeDelta);
}


S32 LevelInfoDisplayer::getDisplayTime()
{
   return mDisplayTimer.getCurrent();
}


void LevelInfoDisplayer::clearTimer()
{
   mDisplayTimer.clear();
}


extern ScreenInfo gScreenInfo;

void LevelInfoDisplayer::render(const GameType *gameType, S32 teamCount, const char *activationKey, bool userActivated)
{
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();
   static const S32 yStart = 50;  // 50 from the top

   // Fade message out
   F32 alpha = 1;
   if(mDisplayTimer.getCurrent() < 1000 && !userActivated)
      alpha = mDisplayTimer.getCurrent() * 0.001f;

   // Draw top info box
   UserInterface::renderFancyBox(yStart, 210, 30, Colors::blue, alpha * 0.70f);

   glColor(Colors::white, alpha);
   drawCenteredStringf(yStart + 5, 30, "Level: %s", gameType->getLevelName()->getString());

   // Prefix game type with "Team" if they are typically individual games, but are being played in team mode
   const char *gtPrefix = (gameType->canBeIndividualGame() && gameType->getGameTypeId() != SoccerGame && 
                           teamCount > 1) ? "Team " : "";

   drawCenteredStringf(yStart + 45, 30, "Game Type: %s%s", gtPrefix, gameType->getGameTypeName());

   glColor(Colors::cyan, alpha);
   drawCenteredString(yStart + 85, 20, gameType->getInstructionString());

   glColor(Colors::magenta, alpha);
   drawCenteredString(yStart + 110, 20, gameType->getLevelDescription()->getString());

   glColor(Colors::yellow, alpha);
   drawCenteredStringf(yStart + 135, 20, "Score to Win: %d", gameType->getWinningScore());

   if(gameType->getLevelCredits()->isNotNull())    // Only render credits string if it's is not empty
   {
      glColor(Colors::red, alpha);
      drawCenteredStringf(yStart + 175, 20, "%s", gameType->getLevelCredits()->getString());
   }

   // Draw bottom info box
   UserInterface::renderFancyBox(canvasHeight - 105, 35, 155, Colors::blue, alpha * 0.70f);

   glColor(Colors::menuHelpColor, alpha);
   drawCenteredStringf(canvasHeight - 100, 20, "Press [%s] to see this information again", activationKey);
}


};