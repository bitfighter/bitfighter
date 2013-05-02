#include "TimeLeftRenderer.h"
#include "gameType.h"
#include "CoreGame.h"
#include "ClientGame.h"
#include "RenderUtils.h"
#include "OpenglUtils.h"
#include "stringUtils.h"
#include "gameObjectRender.h"    // For renderFlag()
#include "ScreenInfo.h"
#include "ClientInfo.h"
#include "UI.h"                  // For vertMargin
#include "FontManager.h"

#include <math.h>

namespace Zap { 

extern ScreenInfo gScreenInfo;

namespace UI {

const U32 screenMargin = 7;
const U32 rightAlignCoord = gScreenInfo.getGameCanvasWidth() - screenMargin;

const S32 timeTextSize = 28;
const S32 bigScoreTextSize = 28;
const S32 bigScoreTextGap = 5;


void TimeLeftRenderer::render(const GameType *gameType, bool scoreboardVisible) const
{
   FontManager::pushFontContext(LoadoutIndicatorContext);

   S32 timeTop = renderTimeLeft(gameType);

   // Some game types *ahem* Nexus *ahem* require an extra line for the scoreboard... a "special" if you will
   timeTop -= gameType->renderTimeLeftSpecial(rightAlignCoord, timeTop);

   if(!scoreboardVisible)
   {
      Game *game = gameType->getGame();
      S32 teamCount = game->getTeamCount();

      if(teamCount > 1 && gameType->isTeamGame())
         renderTeamScores(gameType, timeTop);

      else if(teamCount > 0 && !gameType->isTeamGame())     // For single team games like rabbit and bitmatch
         renderIndividualScores(gameType, timeTop);
   }

   FontManager::popFontContext();
}


// Draw the scores for each team, with an adjacent flag
void TimeLeftRenderer::renderTeamScores(const GameType *gameType, S32 bottom) const
{
   Game *game = gameType->getGame();
//   bool core = gameType->getGameTypeId() == CoreGame;

   S32 ypos = bottom - bigScoreTextSize;      

   S32 maxWidth = renderHeadlineScores(game, ypos);   // Use max score width to vertically align symbols
   S32 xpos = rightAlignCoord - maxWidth - 18;

   S32 teamCount = game->getTeamCount();

   for(S32 i = teamCount - 1; i >= 0; i--)
   {
      gameType->renderScoreboardOrnament(i, xpos, ypos);
      ypos -= bigScoreTextSize + bigScoreTextGap;
   }
}


// ypos is the coordinate of the top of the bottom most score; because the position of the scores is anchored to the bottom,
// we'll render bottom to top
S32 TimeLeftRenderer::renderHeadlineScores(const Game *game, S32 ypos) const
{
   S32 teamCount = game->getTeamCount();
   S32 maxWidth = S32_MIN;

   glColor(Colors::white);

   for(S32 i = teamCount - 1; i >= 0; i--)
   {
      S32 score = static_cast<Team *>(game->getTeam(i))->getScore();
      //score = Platform::getRealMilliseconds() / 1000 % 20 * i;  // Uncomment to test display of various scores

      // This is a total hack based on visual inspection trying to get scores ending in 1 to align with others
      // in a way that is nice.  This is totally font dependent, sadly...
      S32 oneFixFactor = score % 10 == 1 ? 5 : 0;

      S32 width = drawStringfr(rightAlignCoord - oneFixFactor, ypos, bigScoreTextSize, "%d", score);
      maxWidth = max(maxWidth, width);

      ypos -= bigScoreTextSize + bigScoreTextGap;
   }

   return maxWidth;
}


// Try to mitigate some of the weirdness that comes from TTF hinting when trying to
// right-align text
static void drawStringDigitByDigit(S32 x, S32 y, S32 textsize, const string &s)
{
   for(S32 i = (S32)s.length() - 1; i >= 0; i--)
      x -= drawStringr(x, y, textsize, s.substr(i, 1).c_str());
}


// Render 1 or 2 scores: Either render the current client on the bottom (if only one player); 
// or renders player on top and the 2nd player on the bottom (if player is winning);
// or leader on top and player second (if player is losing)
void TimeLeftRenderer::renderIndividualScores(const GameType *gameType, S32 bottom) const
{
   Game *game = gameType->getGame();
   ClientGame *clientGame = static_cast<ClientGame *>(game);

   // We can get here before we get the first unpackUpdate packet arrives -- if so, return
   if(clientGame->getLocalRemoteClientInfo() == NULL)
      return;

   if(gameType->getLeadingPlayer() < 0)
      return;

   const S32 textsize = 12;
   const S32 textgap = 4;
   const S32 oneAdjFact = 2;

   S32 ypos = bottom - textsize; 

   /// Render player score
   bool hasSecondLeader = gameType->getSecondLeadingPlayer() >= 0;

   const StringTableEntry localClientName = clientGame->getClientInfo()->getName();

   // The player is the leader if a leader is detected and it matches his name
   bool localClientIsLeader = (localClientName == game->getClientInfo(gameType->getLeadingPlayer())->getName());

   const char *topName, *botName;
   string topScoreStr, botScoreStr;
   S32 topScore, botScore, topScoreLen = 0, botScoreLen = 0, topOneFixFactor = 0, botOneFixFactor = 0;

   const Color *winnerColor = &Colors::red;
   const Color *loserColor  = &Colors::red60;

   bool renderTwoNames = hasSecondLeader || !localClientIsLeader;

   // Slide the first entry up if there will be a second entry
   S32 firstNameOffset = renderTwoNames ? (textsize + textgap) : 0;    

   glColor(winnerColor);

   topName  = game->getClientInfo(gameType->getLeadingPlayer())->getName().getString();
   topScore = gameType->getLeadingPlayerScore();

   // This is a total hack based on visual inspection trying to get scores ending in 1 to align with others
   // in a way that is nice.  This is totally font dependent, sadly...
   topOneFixFactor = topScore % 10 == 1 ? oneAdjFact : 0;

   topScoreStr = itos(topScore);
   topScoreLen = getStringWidth(textsize, topScoreStr.c_str()) + topOneFixFactor;

   if(renderTwoNames)
   {
      // Should test if leader first
      if(!localClientIsLeader)
      {
         botName  = clientGame->getLocalRemoteClientInfo()->getName().getString();
         botScore = clientGame->getLocalRemoteClientInfo()->getScore();
      }
      // hasSecondLeader
      else
      {
         botName  = game->getClientInfo(gameType->getSecondLeadingPlayer())->getName().getString();
         botScore = gameType->getSecondLeadingPlayerScore();
      }

      //botScore = (Platform::getRealMilliseconds() / 500 % 10 )* 2;
      
      botOneFixFactor = botScore % 10 == 1 ? oneAdjFact : 0;
      botScoreStr = itos(botScore);
      botScoreLen = getStringWidth(textsize, botScoreStr.c_str()) + botOneFixFactor;
   }

   // 5 here is the gap between the names and the scores
   S32 maxWidth = max(topScoreLen, botScoreLen) + 5;

   drawStringDigitByDigit(rightAlignCoord - topOneFixFactor, ypos - firstNameOffset, textsize, topScoreStr);
   drawStringr           (rightAlignCoord - maxWidth,        ypos - firstNameOffset, textsize, topName);

   // Render bottom score if we have one
   if(renderTwoNames)
   {
      if(topScore == botScore)      // If players are tied, render both with winner's color
         glColor(winnerColor);
      else
         glColor(loserColor);

      drawStringDigitByDigit(rightAlignCoord - botOneFixFactor, ypos, textsize, botScoreStr);
      drawStringr           (rightAlignCoord - maxWidth,        ypos, textsize, botName);
   }
}


// Returns y-coord of top of display, which we can use to position other elements 
S32 TimeLeftRenderer::renderTimeLeft(const GameType *gameType) const
{
   const S32 siSize = 12;                 // Size of stacked indicators
   const S32 grayLineHorizPadding = 4;
   const S32 grayLineVertPadding = 3;

   // Precalc some widths we'll need from time to time
   static const U32 w0     = getStringWidth(timeTextSize, "0");
   static const U32 wUnlim = getStringWidth(timeTextSize, "Unlim.");

   U32 timeWidth;
   if(gameType->isTimeUnlimited())
      timeWidth = wUnlim;
   else
   {
      // Get the width of the minutes and 10 seconds digit(s)
      // By using the actual minutes, we get a better fit while not changing the positioning too often
      U32 minsRemaining = gameType->getRemainingGameTimeInMs() / (60 * 1000);
      const U32 tenSecsRemaining = gameType->getRemainingGameTimeInMs() / 1000 % 60 / 10;
      string timestr = itos(minsRemaining) + ":" + itos(tenSecsRemaining);
      timeWidth = getStringWidth(timeTextSize, timestr.c_str()) + w0;

      // Add a little extra for the leading 0 that's drawn for one digit times
      if(minsRemaining < 10)
         timeWidth += w0;
   }

   const S32 grayLinePos = rightAlignCoord - timeWidth - grayLineHorizPadding;  // Where the vertical gray line is drawn
   const S32 smallTextRPos = grayLinePos - grayLineHorizPadding;                // Right-align the stacked text here
   
   // Left and top coordinates of the time display
   const S32 timeLeft = rightAlignCoord - timeWidth;
   const S32 timeTop  = gScreenInfo.getGameCanvasHeight() - timeTextSize - screenMargin;

   S32 wt, wb;    // Width of top and bottom items respectively

   glColor(Colors::cyan);
   // Align with top of time, + 4 technically not needed, but just looks better
   wt = drawStringfr(smallTextRPos, timeTop + 2, siSize, gameType->getShortName());

   glColor(Colors::red);
   // Align with bottom of time
   S32 stwSizeBonus = 1;
   wb = drawStringfr(smallTextRPos, timeTop + timeTextSize - siSize - stwSizeBonus, siSize + stwSizeBonus, 
                     itos(gameType->getWinningScore()).c_str()); 

   glColor(Colors::white);
   if(gameType->isTimeUnlimited())  
      drawString(timeLeft, timeTop, timeTextSize, "Unlim.");
   else
      drawTime(timeLeft, timeTop, timeTextSize, gameType->getRemainingGameTimeInMs());

   const S32 leftLineOverhangAmount = 4;
   const S32 visualVerticalTextAlignmentHackyFacty = 3;
   glColor(Colors::gray40);
   drawHorizLine(smallTextRPos - max(wt, wb) - leftLineOverhangAmount, rightAlignCoord, timeTop - grayLineVertPadding);
   drawVertLine(grayLinePos, timeTop + visualVerticalTextAlignmentHackyFacty, timeTop + timeTextSize);

   return timeTop - 2 * grayLineVertPadding - gDefaultLineWidth - 3;
}


} }      // Nested namespaces
