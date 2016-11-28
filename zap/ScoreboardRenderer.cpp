//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Use this for testing the scoreboard
//#define USE_DUMMY_PLAYER_SCORES

#include "ScoreboardRenderer.h"

#include "ClientGame.h"
#include "Colors.h"
#include "DisplayManager.h"
#include "FontManager.h"
#include "GameObjectRender.h"
#include "gameType.h"
#include "RenderUtils.h"

//#include <cmath>     // Needed to compile under Linux, OSX


static const S32 vertMargin  = UserInterface::vertMargin;
static const S32 horizMargin = UserInterface::horizMargin;

enum ColIndex {
   KdIndex,        // = 0
   PingIndex,      // = 1
   ScoreIndex,     // = 2
   ColIndexCount
};

static const S32 ColHeaderTextSize = 10;

// Horiz offsets from the right for rendering score components
static const S32 ScoreOff = 160;    // Solo game only
static const S32 KdOff   = 85;
static const S32 PingOff = 60;
static const U32 Gap = 3;        // Small gap for use between various UI elements TODO: Get this from UIGame


namespace Zap
{


static const char *botSymbol = "B";
static const char *levelChangerSymbol = "+";
static const char *adminSymbol = "@";


#ifdef USE_DUMMY_PLAYER_SCORES

S32 getDummyTeamCount()  { return 2; }    // Teams
S32 getDummyMaxPlayers() { return 5; }    // Players per team

// Create a set of fake player scores for testing the scoreboard -- fill scores
void getDummyPlayerScores(ClientGame *game, Vector<ClientInfo *> &scores)
{
   ClientInfo *clientInfo;

   S32 teams = getDummyTeamCount();

   for(S32 i = 0; i < getDummyMaxPlayers(); i++)
   {
      string name = "PlayerName-" + itos(i);

      clientInfo = new RemoteClientInfo(game, name, false, 0, ((i+1) % 4) > 0, i, i % 3, ClientInfo::ClientRole(i % 4), false, false);

      clientInfo->setScore(i * 6);
      clientInfo->setAuthenticated((i % 2), 0, (i % 3) > 0);
      clientInfo->setPing(10 * i + 80);
      clientInfo->setTeamIndex(i % teams);

      scores.push_back(clientInfo);
   }
}
#endif


static S32 getMaxPlayersOnAnyTeam(ClientGame *clientGame, S32 teams, bool isTeamGame)
{
   S32 maxTeamPlayers = 0;

   // Check to make sure at least one team has at least one player...
   for(S32 i = 0; i < teams; i++)
   {
      Team *team = (Team *)clientGame->getTeam(i);
      S32 teamPlayers = team->getPlayerBotCount();

      if(!isTeamGame)
         maxTeamPlayers += teamPlayers;

      else if(teamPlayers > maxTeamPlayers)
         maxTeamPlayers = teamPlayers;
   }

   return maxTeamPlayers;
}


static void renderScoreboardLegend(S32 humans, U32 scoreboardTop, U32 totalHeight)
{
   const S32 LegendSize = 12;     
   const S32 LegendGap  =  3;    // Space between scoreboard and legend
   const S32 legendPos  = scoreboardTop + totalHeight + LegendGap + LegendSize;

   // Create a standard legend; only need to swap out the Humans count, which is the first chunk -- this should work even if
   // there are multiple players running in the same session -- the humans count should be the same regardless!
   static Vector<SymbolShapePtr> symbols;
   static S32 lastHumans = S32_MIN;
   if(symbols.size() == 0)
   {
      string legend = " | " + string(adminSymbol) + " = Admin | " + 
                      levelChangerSymbol + " = Can Change Levels | " + botSymbol + " = Bot |";

      symbols.push_back(SymbolShapePtr());    // Placeholder, will be replaced with humans count below
      symbols.push_back(SymbolShapePtr(new SymbolText(legend, LegendSize, ScoreboardContext, Colors::standardPlayerNameColor)));
      symbols.push_back(SymbolShapePtr(new SymbolText(" Idle Player", LegendSize, ScoreboardContext, Colors::idlePlayerNameColor)));
      symbols.push_back(SymbolShapePtr(new SymbolText(" | ", LegendSize, ScoreboardContext, Colors::standardPlayerNameColor)));
      symbols.push_back(SymbolShapePtr(new SymbolText("Player on Rampage", LegendSize, ScoreboardContext, Colors::streakPlayerNameColor)));
   }

   // Rebuild the humans symbol, if the number of humans has changed
   if(humans != lastHumans)
   {
      const string humanStr = itos(humans) + " Human" + (humans != 1 ? "s" : "");
      symbols[0] = SymbolShapePtr(new SymbolText(humanStr, LegendSize, ScoreboardContext, Colors::standardPlayerNameColor));
      lastHumans = humans;
   }

   UI::SymbolString symbolString(symbols);
   symbolString.render(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, legendPos, AlignmentCenter);
}


// Figure out what color to use to render player name
static const Color &getPlayerNameColor(ClientInfo *player)
{
   if(player->isSpawnDelayed())
      return Colors::idlePlayerNameColor;

   if(player->getKillStreak() >= UserInterface::StreakingThreshold)
      return Colors::streakPlayerNameColor;
                                                                     
   return Colors::standardPlayerNameColor;
}


static void renderPlayerSymbol(ClientInfo *player, S32 x, S32 y, S32 size, const Color &color)
{
   // Figure out how much room we need to leave for our player symbol (@, +, etc.)
   x -= RenderUtils::getStringWidth(size, adminSymbol) + Gap;  // Use admin symbol as it's the widest

   // Draw the player's experience level before we set the color
   FontManager::pushFontContext(OldSkoolContext);

   static const S32 fontSize = 7;

   RenderUtils::drawStringf_fixed(x - 8, y + (fontSize - size) + 7, fontSize, Colors::green, "%d", ClientGame::getExpLevel(player->getGamesPlayed()));
   FontManager::popFontContext();

   // Mark of the bot
   if(player->isRobot())
      RenderUtils::drawString_fixed(x, y, size, color, botSymbol);

   // Admin mark
   else if(player->isAdmin())
      RenderUtils::drawString_fixed(x, y, size, color, adminSymbol);

   // Level changer mark
   else if(player->isLevelChanger())
      RenderUtils::drawString_fixed(x, y, size, color, levelChangerSymbol);
}


static void renderTeamName(ClientGame *clientGame, S32 index, bool isWinningTeam, S32 left, S32 right, S32 top)
{
   static const S32 teamFontSize = 24;

   // First the box
   const Color &teamColor = clientGame->getTeamColor(index);
   const Color &borderColor = isWinningTeam ? Colors::white : teamColor;
   const S32 headerBoxHeight = teamFontSize + 2 * Gap + 1;

   if(index == 1)    // Box in UR corner needs to be fancy to nest in outer box
      RenderUtils::drawFilledFancyBox(left, top, right, top + headerBoxHeight, 10, RenderUtils::UR, teamColor, 0.6f, borderColor);
   else
      RenderUtils::drawFilledRect(left, top, right, top + headerBoxHeight, teamColor, 0.6f, borderColor, 1.0f);

   // Then the team name & score
   FontManager::pushFontContext(ScoreboardHeadlineContext);

   // Figure out where we should draw the teamname and score -- we can nudge things apart a little to 
   // accomodate long names or high scores

   // First, set some limits about where the score can go...
   const S32 minRight = right - 50;    // Ideal score location
   const S32 maxRight = right - 10;    // Score can't go further right than this
   const S32 minLeft = left + 50;
   const S32 maxLeft = left + 10;

   const S32 gap = 20;                 // Min gap between team name and score

   string scoreStr = itos(clientGame->getTeam(index)->getScore());

   const S32 teamWidth  = RenderUtils::getStringWidth(teamFontSize, clientGame->getTeamName(index).getString());
   const S32 scoreWidth = RenderUtils::getStringWidth(teamFontSize, scoreStr.c_str());

   const S32 deficit = (teamWidth + scoreWidth + gap) - (minRight - minLeft);
   S32 offset = (deficit > 0) ? deficit / 2 : 0;
   S32 leftPos  = max(minLeft  - offset, maxLeft);
   S32 rightPos = min(minRight + offset, maxRight);

   string teamName;

   // If name is still too long, truncate it
   S32 maxLen = maxRight - maxLeft - scoreWidth - gap;

   if(teamWidth + scoreWidth + gap > maxLen)
   {
      S32 len = 0;
      S32 i;

      for(i = 0; len < maxLen; i++)
         len += RenderUtils::getStringWidthf(teamFontSize, "%c", clientGame->getTeamName(index).getString()[i]);

      teamName = string(clientGame->getTeamName(index).getString()).substr(0, i - 1);
   }
   else 
      teamName = clientGame->getTeamName(index).getString();

   FontManager::setFontColor(Colors::white);
   RenderUtils::drawString (leftPos,  top + 2, teamFontSize, teamName.c_str());
   RenderUtils::drawStringr(rightPos, top + 2, teamFontSize, scoreStr.c_str());

   FontManager::popFontContext();
}


static void renderScoreboardColumnHeaders(S32 leftEdge, S32 rightEdge, S32 y,
                                          const S32 *colIndexWidths, bool isTeamGame)
{
   FontManager::setFontColor(Colors::gray50);

   RenderUtils::drawString_fixed(leftEdge,                                                 y, ColHeaderTextSize, "Name");
   RenderUtils::drawStringc     (rightEdge -  (KdOff   + colIndexWidths[KdIndex]   / 2),   y, ColHeaderTextSize, "Threat Level");
   RenderUtils::drawStringc     (rightEdge -  (PingOff - colIndexWidths[PingIndex]  / 2),  y, ColHeaderTextSize, "Ping");
   // Solo games need one more header
   if(!isTeamGame)
      RenderUtils::drawStringc   (rightEdge - (ScoreOff + colIndexWidths[ScoreIndex] / 2), y, ColHeaderTextSize, "Score");
}


// Static method
static void renderBadges(ClientInfo *clientInfo, S32 x, S32 y, F32 scaleRatio)
{
   // Default to vector font for badges
   FontManager::pushFontContext(OldSkoolContext);

   F32 badgeRadius = 10.f * scaleRatio;
   S32 badgeOffset = S32(2 * badgeRadius) + 5;
   F32 badgeBackgroundEdgeSize = 2 * badgeRadius + 2.f;

   bool hasBBBBadge = false;

   for(S32 i = 0; i < BADGE_COUNT; i++)
   {
      MeritBadges badge = MeritBadges(i);    // C++ enums can be rather tedious...

      if(clientInfo->hasBadge(badge))
      {
         // Test for BBB badges.  We're only going to show the most valued one
         if(badge == BADGE_BBB_GOLD || badge == BADGE_BBB_SILVER || badge == BADGE_BBB_BRONZE || badge == BADGE_BBB_PARTICIPATION)
         {
            // If we've already got one, don't draw this badge.  This assumes the value of the badges decrease
            // with each iteration
            if(hasBBBBadge)
               continue;

            hasBBBBadge = true;
         }

         // Draw badge border
         RenderUtils::drawRoundedRect(Point(x, y), badgeBackgroundEdgeSize, badgeBackgroundEdgeSize, 3.0f, Colors::gray20);

         GameObjectRender::renderBadge((F32)x, (F32)y, badgeRadius, badge);
         x += badgeOffset;
      }
   }

   FontManager::popFontContext();
}


static S32 getMaxPingWidth(const Vector<ClientInfo *> &playerScores, S32 fontSize)
{
   S32 maxWidth = -1;

   for(S32 i = 0; i < playerScores.size(); i++)
   {
      S32 width = RenderUtils::getStringWidthf(fontSize, "%d", playerScores[i]->getPing());
      if(width > maxWidth)
         maxWidth = width;
   }
   
   return maxWidth;
}


// Renders a line on the scoreboard, and returns the widths of the rendered items in colWidths
static void renderScoreboardLine(const Vector<ClientInfo *> &playerScores, bool isTeamGame, S32 row, S32 playerFontSize,
                                 S32 x, S32 y, U32 lineHeight, S32 rightEdge, S32 *colWidths, S32 maxPingWidth)
{
   const S32 symbolFontSize = S32(lineHeight * 0.75f * 0.75f);

   static const S32 vertAdjustFact = (playerFontSize - symbolFontSize) / 2 - 1;

   const Color &color = getPlayerNameColor(playerScores[row]);
   renderPlayerSymbol(playerScores[row], x, y + vertAdjustFact + 2 + symbolFontSize, symbolFontSize, color);

   y += playerFontSize;
   S32 nameWidth = RenderUtils::drawStringAndGetWidth_fixed(x, y, playerFontSize, color, playerScores[row]->getName().getString());

   colWidths[KdIndex]   = RenderUtils::drawStringfr_fixed(rightEdge - KdOff,                  y, playerFontSize, color, "%2.2f", playerScores[row]->getRating());
   colWidths[PingIndex] = RenderUtils::drawStringfr_fixed(rightEdge - PingOff + maxPingWidth, y, playerFontSize, color, "%d",    playerScores[row]->getPing());

   if(!isTeamGame)
      colWidths[ScoreIndex] = RenderUtils::drawStringfr_fixed(rightEdge - ScoreOff, y, playerFontSize, color, "%d", playerScores[row]->getScore());

   // Vertical scale ratio to maximum line height
   const F32 scaleRatio = lineHeight / 30.f;

   // Circle back and render the badges now that all the rendering with the name color is finished
   renderBadges(playerScores[row], x + nameWidth + 10 + Gap, y + (lineHeight / 2), scaleRatio);
}


static void renderTeamScoreboard(ClientGame *clientGame, S32 index, S32 teams, bool isTeamGame, bool isWinningTeam,
                                 S32 scoreboardTop, S32 sectionHeight, S32 teamHeaderHeight, S32 lineHeight)
{
   static const S32 canvasWidth  = DisplayManager::getScreenInfo()->getGameCanvasWidth();

   static const S32 drawableWidth = canvasWidth - UserInterface::horizMargin * 2;

   const S32 columnCount = min(teams, 2);
   const S32 teamWidth = drawableWidth / columnCount;

   const S32 xl = horizMargin + Gap + (index & 1) * teamWidth;    // Left edge of team render area
   const S32 xr = (xl + teamWidth) - (2 * Gap);                   // Right edge of team render area
   const S32 yt = scoreboardTop + (index >> 1) * sectionHeight;   // Top edge of team render area

   // Team header
   if(isTeamGame)
      renderTeamName(clientGame, index, isWinningTeam, xl, xr, yt);

   // Now for player scores.  First build a list.  Then sort it.  Then display it.  Then reuse it.
   static Vector<ClientInfo *> playerScores;
   playerScores.clear();

#ifdef USE_DUMMY_PLAYER_SCORES      // For testing purposes only!
   getDummyPlayerScores(clientGame, playerScores);
#else
   clientGame->getGameType()->getSortedPlayerScores(index, playerScores);     // Fills playerScores for team index
#endif

   S32 curRowY = yt + teamHeaderHeight + 1;                          // Advance y coord to below team display, if there is one

   const S32 x = xl + 40;                                            // + 40 to align with team name in team game
   const S32 colHeaderYPos = isTeamGame ? curRowY + 3 : curRowY + 8; // Calc this before we change curRowY

   // Leave a gap for the colHeader... not sure yet of the exact xpos... will figure that out and render in this slot later
   if(playerScores.size() > 0)
   {
      const S32 colHeaderHeight = isTeamGame ? ColHeaderTextSize - 3: ColHeaderTextSize + 2;
      curRowY += colHeaderHeight;
   }

   S32 colIndexWidths[ColIndexCount];     
   S32 maxColIndexWidths[ColIndexCount] = {0};     // Inits every element of array to 0

   const S32 playerFontSize = S32(lineHeight * 0.75f);
   const S32 maxPingWidth = getMaxPingWidth(playerScores, playerFontSize);

   for(S32 i = 0; i < playerScores.size(); i++)
   {
      renderScoreboardLine(playerScores, isTeamGame, i, playerFontSize, x, curRowY, lineHeight, xr, colIndexWidths, maxPingWidth);
      curRowY += lineHeight;

      for(S32 j = 0; j < ColIndexCount; j++)
         maxColIndexWidths[j] = max(colIndexWidths[j], maxColIndexWidths[j]);
   }

   // Go back and render the column headers, now that we know the widths.  These will be different for team and solo games.
   if(playerScores.size() > 0)
      renderScoreboardColumnHeaders(x, xr, colHeaderYPos, maxColIndexWidths, isTeamGame);

#ifdef USE_DUMMY_PLAYER_SCORES
   playerScores.deleteAndClear();      // Clean up
#endif

}


void ScoreboardRenderer::renderScoreboard(ClientGame *clientGame)
{
   GameType *gameType = clientGame->getGameType();

   const bool isTeamGame = gameType->isTeamGame();

#ifdef USE_DUMMY_PLAYER_SCORES
   S32 teams = isTeamGame ? getDummyTeamCount() : 1;
   S32 maxTeamPlayers = getDummyMaxPlayers();
#else
   clientGame->countTeamPlayers();
   const S32 teams = isTeamGame ? clientGame->getTeamCount() : 1;
   S32 maxTeamPlayers = getMaxPlayersOnAnyTeam(clientGame, teams, isTeamGame);
#endif

   if(maxTeamPlayers == 0)
      return;

   static const S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();
   static const S32 canvasWidth  = DisplayManager::getScreenInfo()->getGameCanvasWidth();

   const S32 teamHeaderHeight = isTeamGame ? 40 : 2;

   const S32 numTeamRows = (teams + 1) >> 1;

   const S32 desiredHeight = (canvasHeight - vertMargin * 2) / numTeamRows;
   const S32 lineHeight    = MIN(30, (desiredHeight - teamHeaderHeight) / maxTeamPlayers);

   const S32 sectionHeight = teamHeaderHeight + (lineHeight * maxTeamPlayers) + (2 * Gap) + 10;
   const S32 totalHeight   = sectionHeight * numTeamRows - 10  + (isTeamGame ? 0 : 4);    // 4 provides a gap btwn bottom name and legend

   const S32 scoreboardTop = (canvasHeight - totalHeight) / 2;    // Center vertically

   const S32 winStatus = clientGame->getTeamBasedGameWinner().first;
   bool hasWinner = winStatus == HasWinner;
   bool isWinningTeam;

   // Outer scoreboard box
   RenderUtils::drawFilledFancyBox(horizMargin - Gap, scoreboardTop - (2 * Gap),
                     (canvasWidth - horizMargin) + Gap, scoreboardTop + totalHeight + 23,
                     13, RenderUtils::LL|RenderUtils::UR, Colors::black, 0.85f, Colors::blue);

   FontManager::pushFontContext(ScoreboardContext);
   
   for(S32 i = 0; i < teams; i++)
   {
      if(clientGame->isGameOver() && hasWinner && i == clientGame->getTeamBasedGameWinner().second)
         isWinningTeam = true;
      else
         isWinningTeam = false;
      
      renderTeamScoreboard(clientGame, i, teams, isTeamGame, isWinningTeam, scoreboardTop, sectionHeight, teamHeaderHeight, lineHeight);
   }

   renderScoreboardLegend(clientGame->getPlayerCount(), scoreboardTop, totalHeight);

   FontManager::popFontContext();
}

}
