//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

//#define USE_DUMMY_PLAYER_SCORES

#include "ScoreboardRenderer.h"

#include "UIGame.h"

#include "ClientGame.h"
#include "ClientInfo.h"
#include "DisplayManager.h"
#include "FontManager.h"
#include "gameType.h"
#include "Renderer.h"
#include "RenderUtils.h"
#include "SymbolShape.h"
#include "Point.h"
#include "Colors.h"
#include "stringUtils.h"
#include "teamInfo.h"
#include "gameObjectRender.h"

#include <algorithm>

namespace Zap
{
namespace
{

using std::string;
using std::max;
using std::min;

static const char *botSymbol = "B";
static const char *levelChangerSymbol = "+";
static const char *adminSymbol = "@";

static const S32 ScoreOff = 160;
static const S32 KdOff   = 85;
static const S32 PingOff = 60;
static const U32 Gap = 3;
static const S32 ColHeaderTextSize = 10;

#ifdef USE_DUMMY_PLAYER_SCORES
S32 getDummyTeamCount() { return 2; }
S32 getDummyMaxPlayers() { return 5; }

void getDummyPlayerScores(ClientGame *game, Vector<ClientInfo *> &scores)
{
   ClientInfo *clientInfo;
   const S32 teams = getDummyTeamCount();

   for(S32 i = 0; i < getDummyMaxPlayers(); i++)
   {
      string name = "PlayerName-" + itos(i);

      clientInfo = new RemoteClientInfo(game, name, false, 0, ((i + 1) % 4) > 0, i, i % 3, ClientInfo::ClientRole(i % 4), false, false);

      clientInfo->setScore(i * 3);
      clientInfo->setAuthenticated((i % 2), 0, (i % 3) > 0);
      clientInfo->setPing(100 * i + 10);
      clientInfo->setTeamIndex(i % teams);

      scores.push_back(clientInfo);
   }
}
#endif

void renderScoreboardLegend(S32 humans, U32 scoreboardTop, U32 totalHeight)
{
   const S32 LegendSize = 12;
   const S32 LegendGap  = 3;
   const S32 legendPos  = scoreboardTop + totalHeight + LegendGap + LegendSize;

   static Vector<SymbolShapePtr> symbols;
   static S32 lastHumans = S32_MIN;

   if(symbols.size() == 0)
   {
      string legend = " | " + string(adminSymbol) + " = Admin | " +
                      levelChangerSymbol + " = Can Change Levels | " + botSymbol + " = Bot |";

      symbols.push_back(SymbolShapePtr());
      symbols.push_back(SymbolShapePtr(new SymbolText(legend, LegendSize, ScoreboardContext, &Colors::standardPlayerNameColor)));
      symbols.push_back(SymbolShapePtr(new SymbolText(" Idle Player", LegendSize, ScoreboardContext, &Colors::idlePlayerNameColor)));
      symbols.push_back(SymbolShapePtr(new SymbolText(" | ", LegendSize, ScoreboardContext, &Colors::standardPlayerNameColor)));
      symbols.push_back(SymbolShapePtr(new SymbolText("Player on Rampage", LegendSize, ScoreboardContext, &Colors::streakPlayerNameColor)));
   }

   if(humans != lastHumans)
   {
      const string humanStr = itos(humans) + " Human" + (humans != 1 ? "s" : "");
      symbols[0] = SymbolShapePtr(new SymbolText(humanStr, LegendSize, ScoreboardContext, &Colors::standardPlayerNameColor));
      lastHumans = humans;
   }

   SymbolString symbolString(symbols);
   symbolString.render(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, legendPos, AlignmentCenter);
}

void renderPlayerSymbolAndSetColor(ClientInfo *player, S32 x, S32 y, S32 size)
{
   Renderer& r = Renderer::get();

   x -= getStringWidth(size, adminSymbol) + Gap;

   FontManager::pushFontContext(OldSkoolContext);
   static const S32 levelSize = 7;
   r.setColor(Colors::green);
   drawStringf(x - 8, y + 7 , levelSize, "%d", ClientGame::getExpLevel(player->getGamesPlayed()));
   FontManager::popFontContext();

   if(player->isSpawnDelayed())
      r.setColor(Colors::idlePlayerNameColor);
   else if(player->getKillStreak() >= UserInterface::StreakingThreshold)
      r.setColor(Colors::streakPlayerNameColor);
   else
      r.setColor(Colors::standardPlayerNameColor);

   if(player->isRobot())
      drawString(x, y, size, botSymbol);
   else if(player->isAdmin())
      drawString(x, y, size, adminSymbol);
   else if(player->isLevelChanger())
      drawString(x, y, size, levelChangerSymbol);
}

enum ColIndex {
   KdIndex,
   PingIndex,
   ScoreIndex,
   ColIndexCount
};

}

ScoreboardRenderer::ScoreboardRenderer(GameUserInterface *ui) : mUI(ui)
{
   // Do nothing
}


void ScoreboardRenderer::render()
{
   ClientGame *game = mUI->getGame();
   GameType *gameType = game->getGameType();
   TNLAssert(gameType, "Expected gametype when rendering scoreboard");
   if(!gameType)
      return;

   const bool isTeamGame = gameType->isTeamGame();

#ifdef USE_DUMMY_PLAYER_SCORES
   S32 maxTeamPlayers = getDummyMaxPlayers();
   S32 teams = isTeamGame ? getDummyTeamCount() : 1;
#else
   game->countTeamPlayers();

   const S32 teams = isTeamGame ? game->getTeamCount() : 1;
   S32 maxTeamPlayers = 0;

   for(S32 i = 0; i < teams; i++)
   {
      Team *team = static_cast<Team *>(game->getTeam(i));

      if(!isTeamGame)
         maxTeamPlayers += team->getPlayerBotCount();
      else if(team->getPlayerBotCount() > maxTeamPlayers)
         maxTeamPlayers = team->getPlayerBotCount();
   }
#endif

   if(maxTeamPlayers == 0)
      return;

   const S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();
   const S32 canvasWidth  = DisplayManager::getScreenInfo()->getGameCanvasWidth();

   const S32 teamHeaderHeight = isTeamGame ? 40 : 2;
   const S32 numTeamRows = (teams + 1) >> 1;
   const S32 desiredHeight = (canvasHeight - UserInterface::vertMargin * 2) / numTeamRows;
   const S32 lineHeight    = std::min<S32>(30, (desiredHeight - teamHeaderHeight) / maxTeamPlayers);

   const S32 sectionHeight = teamHeaderHeight + (lineHeight * maxTeamPlayers) + (2 * Gap) + 10;
   const S32 totalHeight   = sectionHeight * numTeamRows - 10 + (isTeamGame ? 0 : 4);

   const S32 scoreboardTop = (canvasHeight - totalHeight) / 2;

   drawFilledFancyBox(UserInterface::horizMargin - Gap, scoreboardTop - (2 * Gap),
                          (canvasWidth - UserInterface::horizMargin) + Gap, scoreboardTop + totalHeight + 23,
                          13, Colors::black, 0.85f, Colors::blue);

   FontManager::pushFontContext(ScoreboardContext);

   for(S32 i = 0; i < teams; i++)
      renderTeamScoreboard(i, teams, isTeamGame, scoreboardTop, sectionHeight, teamHeaderHeight, lineHeight);

   renderScoreboardLegend(game->getPlayerCount(), scoreboardTop, totalHeight);

   FontManager::popFontContext();
}


void ScoreboardRenderer::renderTeamScoreboard(S32 index, S32 teams, bool isTeamGame,
                                              S32 scoreboardTop, S32 sectionHeight, S32 teamHeaderHeight, S32 lineHeight) const
{
   const S32 canvasWidth  = DisplayManager::getScreenInfo()->getGameCanvasWidth();
   const S32 drawableWidth = canvasWidth - UserInterface::horizMargin * 2;

   const S32 columnCount = min(teams, 2);
   const S32 teamWidth = drawableWidth / columnCount;

   const S32 xl = UserInterface::horizMargin + Gap + (index & 1) * teamWidth;
   const S32 xr = (xl + teamWidth) - (2 * Gap);
   const S32 yt = scoreboardTop + (index >> 1) * sectionHeight;

   if(isTeamGame)
      renderTeamName(index, xl, xr, yt);

   Vector<ClientInfo *> playerInfos;

#ifdef USE_DUMMY_PLAYER_SCORES
   getDummyPlayerScores(mUI->getGame(), playerInfos);
#else
   mUI->getGame()->getGameType()->getSortedPlayersByScore(index, playerInfos);
#endif

   S32 curRowY = yt + teamHeaderHeight + 1;

   const S32 x = xl + 40;
   const S32 colHeaderYPos = isTeamGame ? curRowY + 3 : curRowY + 8;

   if(playerInfos.size() > 0)
   {
      const S32 colHeaderHeight = isTeamGame ? ColHeaderTextSize - 3 : ColHeaderTextSize + 2;
      curRowY += colHeaderHeight;
   }

   S32 colIndexWidths[ColIndexCount];
   S32 maxColIndexWidths[ColIndexCount] = {0};

   for(S32 i = 0; i < playerInfos.size(); i++)
   {
      renderScoreboardLine(playerInfos, isTeamGame, i, x, curRowY, lineHeight, xr, colIndexWidths);
      curRowY += lineHeight;

      for(S32 j = 0; j < ColIndexCount; j++)
         maxColIndexWidths[j] = max(colIndexWidths[j], maxColIndexWidths[j]);
   }

   if(playerInfos.size() > 0)
      renderScoreboardColumnHeaders(x, xr, colHeaderYPos, maxColIndexWidths, isTeamGame);

#ifdef USE_DUMMY_PLAYER_SCORES
   playerInfos.deleteAndClear();
#endif
}


void ScoreboardRenderer::renderTeamName(S32 index, S32 left, S32 right, S32 top) const
{
   Renderer& r = Renderer::get();
   static const S32 teamFontSize = 24;

   const Color *teamColor = mUI->getGame()->getTeamColor(index);
   const S32 headerBoxHeight = teamFontSize + 2 * Gap;

   drawFilledFancyBox(left, top, right, top + headerBoxHeight, 10, *teamColor, 0.6f, *teamColor);

   FontManager::pushFontContext(ScoreboardHeadlineContext);
   r.setColor(Colors::white);

   const S32 minRight = right - 50;
   const S32 maxRight = right - 10;
   const S32 minLeft = left + 50;
   const S32 maxLeft = left + 10;

   const S32 gap = 20;

   string scoreStr = itos(((Team *)(mUI->getGame()->getTeam(index)))->getScore());
   string origTeamName = mUI->getGame()->getTeamName(index).getString();

   const S32 teamWidth  = getStringWidth(teamFontSize, origTeamName.c_str());
   const S32 scoreWidth = getStringWidth(teamFontSize, scoreStr.c_str());

   const S32 deficit = (teamWidth + scoreWidth + gap) - (minRight - minLeft);
   S32 offset = (deficit > 0) ? deficit / 2 : 0;
   S32 leftPos  = max(minLeft  - offset, maxLeft);
   S32 rightPos = min(minRight + offset, maxRight);

   const S32 maxLen = maxRight - maxLeft - scoreWidth - gap;

   S32 fontSize = teamFontSize;
   S32 vertAdjustment = 0;
   if(teamWidth + scoreWidth + gap > maxLen)
   {
      F32 ratio = F32(maxLen) / teamWidth;
      fontSize = S32(ratio * teamFontSize);
      vertAdjustment = (teamFontSize - fontSize + 1) / 2;
   }

   drawString(leftPos,  top + 2 + vertAdjustment, fontSize, origTeamName.c_str());
   drawStringr(rightPos, top + 2, teamFontSize, scoreStr.c_str());

   FontManager::popFontContext();
}


void ScoreboardRenderer::renderScoreboardColumnHeaders(S32 leftEdge, S32 rightEdge, S32 y, const S32 *colIndexWidths, bool isTeamGame) const
{
   Renderer::get().setColor(Colors::gray50);

   drawString_fixed(leftEdge,                                                 y, ColHeaderTextSize, "Name");
   drawStringc     (rightEdge -  (KdOff   + colIndexWidths[KdIndex]    / 2),  y, ColHeaderTextSize, "Kill/Death");
   drawStringc     (rightEdge -  (PingOff - colIndexWidths[PingIndex]  / 2),  y, ColHeaderTextSize, "Ping");

   if(!isTeamGame)
      drawStringc(rightEdge - (ScoreOff + colIndexWidths[ScoreIndex] / 2), y, ColHeaderTextSize, "Score");
}


void ScoreboardRenderer::renderScoreboardLine(const Vector<ClientInfo *> &playerInfos, bool isTeamGame, S32 row,
                                              S32 x, S32 y, U32 lineHeight, S32 rightEdge, S32 *colWidths) const
{
   const S32 playerFontSize = S32(lineHeight * 0.75f);
   const S32 symbolFontSize = S32(lineHeight * 0.75f * 0.75f);
   const S32 dataFontSize   = S32(lineHeight * 0.75f * 0.75f);

   static const S32 symbolVertAdjustFact = (playerFontSize - symbolFontSize) / 2 - 1;
   static const S32 dataVertAdjustFact   = (playerFontSize - dataFontSize) / 2;

   renderPlayerSymbolAndSetColor(playerInfos[row], x, y + symbolVertAdjustFact + 2, symbolFontSize);

   S32 nameWidth = drawStringAndGetWidth(x, y, playerFontSize, playerInfos[row]->getName().getString());

   colWidths[KdIndex]   = drawStringfr          (rightEdge - KdOff,   y + dataVertAdjustFact, dataFontSize,
         "%d/%d", playerInfos[row]->getKills(), playerInfos[row]->getDeaths());
   colWidths[PingIndex] = drawStringAndGetWidthf(rightEdge - PingOff, y + dataVertAdjustFact, dataFontSize,
         "%d", playerInfos[row]->getPing());

   if(!isTeamGame)
      colWidths[ScoreIndex] = drawStringfr(rightEdge - ScoreOff, y, playerFontSize, "%d", playerInfos[row]->getScore());

   const F32 scaleRatio = F32(lineHeight) / 30.f;

   renderBadges(playerInfos[row], x + nameWidth + 10 + Gap, y + (lineHeight / 2), scaleRatio);
}


void ScoreboardRenderer::renderBadges(ClientInfo *clientInfo, S32 x, S32 y, F32 scaleRatio) const
{
   FontManager::pushFontContext(OldSkoolContext);

   F32 badgeRadius = 10.f * scaleRatio;
   S32 badgeOffset = S32(2 * badgeRadius) + 5;
   F32 badgeBackgroundEdgeSize = 2 * badgeRadius + 2.f;

   bool hasBBBBadge = false;

   for(S32 i = 0; i < BADGE_COUNT; i++)
   {
      MeritBadges badge = MeritBadges(i);

      if(clientInfo->hasBadge(badge))
      {
         if(badge == BADGE_BBB_GOLD || badge == BADGE_BBB_SILVER || badge == BADGE_BBB_BRONZE || badge == BADGE_BBB_PARTICIPATION)
         {
            if(hasBBBBadge)
               continue;

            hasBBBBadge = true;
         }

         Renderer::get().setColor(Colors::gray20);
         drawRoundedRect(Point(x, y), badgeBackgroundEdgeSize, badgeBackgroundEdgeSize, 3.f);

         renderBadge(F32(x), F32(y), badgeRadius, badge);
         x += badgeOffset;
      }
   }

   FontManager::popFontContext();
}

}
