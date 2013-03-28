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


void LevelInfoDisplayer::resetDisplayTimer()
{
   mDisplayTimer.reset(6000);  // 6 seconds
}


void LevelInfoDisplayer::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);
   if(mDisplayTimer.update(timeDelta))
      onDeactivated();
}


void LevelInfoDisplayer::clearDisplayTimer()
{
   mDisplayTimer.clear();
}


extern ScreenInfo gScreenInfo;

void LevelInfoDisplayer::render(const GameType *gameType, S32 teamCount, const char *activationKey, bool userActivated)
{
   glPushMatrix();
   glTranslate(0, getInsideEdge(), 0);

   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   bool showCredits = gameType->getLevelCredits()->isNotNull();    // Only render credits string if it's is not empty


   const S32 titleSize = 30;
   const S32 titleMargin = 10;
   const S32 instructionSize = 20;
   const S32 instructionMargin = 8;
   const S32 descriptionSize = 20;
   const S32 descriptionMargin = 8;
   const S32 scoreToWinSize = 20;
   const S32 scoreToWinMargin = 8;
   const S32 creditsSize = 20;
   const S32 creditsMargin = 8;
   const S32 helpSize = 15;

   const S32 topMargin = UserInterface::vertMargin;
   const S32 bottomMargin = topMargin;

   const S32 titleHeight = titleSize + titleMargin;
   const S32 instructionHeight = instructionSize + instructionMargin;
   const S32 descriptionHeight = descriptionSize + descriptionMargin;
   const S32 scoreToWinHeight = scoreToWinSize + scoreToWinMargin;
   const S32 creditsHeight = creditsSize + creditsMargin;
   const S32 helpHeight = helpSize + bottomMargin;

   const S32 totalHeight = topMargin + titleHeight + instructionHeight + descriptionHeight + scoreToWinHeight + creditsHeight + helpHeight;

   S32 yPos = topMargin;

   // Draw top info box
   UserInterface::renderFancyBox(0, totalHeight, 30, Colors::blue, 0.70f);

   FontManager::pushFontContext(FontManager::LevelInfoContext);

   // Prefix game type with "Team" if they are typically individual games, but are being played in team mode
   bool team = gameType->canBeIndividualGame() && gameType->getGameTypeId() != SoccerGame && teamCount > 1;
   string gt = string(" [") + (team ? "Team " : "") + gameType->getGameTypeName() + "]";

   drawCenteredStringPair(yPos, titleSize, Colors::white, Colors::green, gameType->getLevelName()->getString(), gt.c_str());


   yPos += titleHeight;

   glColor(Colors::cyan);
   drawCenteredString(yPos, instructionSize, gameType->getInstructionString());
   yPos += instructionHeight;

   glColor(Colors::magenta);
   drawCenteredString(yPos, descriptionSize, gameType->getLevelDescription()->getString());
   yPos += descriptionHeight;

   drawCenteredStringPair(yPos, scoreToWinSize, Colors::yellow, Colors::red, "Score to Win: ", itos(gameType->getWinningScore()).c_str());
   yPos += scoreToWinHeight;

   if(showCredits)
   {
      glColor(Colors::red);
      drawCenteredStringf(yPos, creditsSize, "%s", gameType->getLevelCredits()->getString());
      yPos += creditsHeight;
   }

   glColor(Colors::menuHelpColor);
   drawCenteredStringf(yPos, helpSize, "Press [%s] to see this information again", activationKey);

   FontManager::popFontContext();

   glPopMatrix();
}


bool LevelInfoDisplayer::isActive() const
{
   return Parent::isActive() || mDisplayTimer.getCurrent() > 0;
}


bool LevelInfoDisplayer::isDisplayTimerActive() const
{
   return mDisplayTimer.getCurrent() > 0;
}

};