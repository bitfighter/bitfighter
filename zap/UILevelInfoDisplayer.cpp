//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UILevelInfoDisplayer.h"   // Header

#include "DisplayManager.h"
#include "game.h"
#include "ClientGame.h"
#include "UI.h"                     // Only here for the margins
#include "GameTypesEnum.h"
#include "gameType.h"               // Can get rid of with a bunch of passthroughs
#include "FontManager.h"
#include "SymbolShape.h"
#include "UIGame.h"

#include "Colors.h"
#include "Intervals.h"

#include "stringUtils.h"
#include "OpenglUtils.h"               
#include "RenderUtils.h"
#include "gameObjectRender.h"


namespace Zap { 
   
namespace UI {

// Constructor
LevelInfoDisplayer::LevelInfoDisplayer()
{
   mDisplayTimer.setPeriod(SIX_SECONDS);
}

// Destructor
LevelInfoDisplayer::~LevelInfoDisplayer()
{
   // Do nothing
}


void LevelInfoDisplayer::resetDisplayTimer()
{
   mDisplayTimer.reset();
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


void LevelInfoDisplayer::render(const GameType *gameType, S32 teamCount, bool isInDatabase) const
{
   glPushMatrix();
   glTranslate(0, getInsideEdge(), 0);

   FontManager::pushFontContext(LevelInfoContext);

    // Only render these when they are not empty
   bool showCredits = gameType->getLevelCredits()->isNotNull();    
   bool showDescr   = strcmp(gameType->getLevelDescription(), "") != 0;

   const char *title   = gameType->getLevelName().c_str();
   const S32 titleSize = 30;
   const S32 titleGap  = 10;

   Vector<SymbolShapePtr> symbols;
   symbols.push_back(SymbolString::getSymbolText(title, titleSize, LevelInfoHeadlineContext));

   // Find the unicode in Character Map or similar utility,
   // then convert it here: http://www.ltg.ed.ac.uk/~richard/utf-8.html
   // Use Hex UTF-8 bytes, represent in string like this: \xE2\x99\xA6

   // These variables are for rendering the gray legend below
   static const F32 RatingSize = 12;

   F32 totalSignlessRatingWidth = 0;
   F32 totalSignWidth = 0;
   F32 mySignlessRatingWidth = 0;

   static const string divider = " / ";
   static const F32 dividerWidth = getStringWidth(LevelInfoContext, RatingSize, divider.c_str());

   ClientGame *clientGame = static_cast<ClientGame *>(gameType->getGame());

   PersonalRating myRating    = clientGame->getPersonalLevelRating();
   S16            totalRating = clientGame->getTotalLevelRating();

   if(isInDatabase)
   {
      symbols.push_back(SymbolString::getBlankSymbol(10));
      symbols.push_back(SymbolString::getSymbolText("\xEF\x80\x8B", 15, WebDingContext));  // Little database icon

      S32 pos;
      
      symbols.push_back(SymbolString::getBlankSymbol(8));  // Padding -- more symbols will be added below in symbolParse

      if(totalRating == UnknownRating)
      {
         SymbolString::symbolParse(NULL, "Loading Rating ", symbols, LevelInfoContext, (S32)RatingSize,     true, &Colors::red);
         SymbolString::symbolParse(NULL, "[[SPINNER]]",     symbols, LevelInfoContext, (S32)RatingSize + 6, true, &Colors::red);
      }
      else
      {
         string myRatingStr = GameUserInterface::getPersonalRatingString(myRating);
         pos = (myRatingStr[0] == '+' || myRatingStr[0] == '-') ? 1 : 0;         // Used to skip past sign below
         mySignlessRatingWidth = getStringWidth(LevelInfoContext, RatingSize, myRatingStr.c_str() + pos);

         string totalRatingStr = GameUserInterface::getTotalRatingString(totalRating);
         pos = (totalRatingStr[0] == '+' || totalRatingStr[0] == '-') ? 1 : 0;   // Used to skip past sign below
         totalSignlessRatingWidth = getStringWidth(LevelInfoContext, RatingSize, totalRatingStr.c_str() + pos);
         totalSignWidth = getStringWidth(LevelInfoContext, RatingSize, totalRatingStr.substr(0, pos).c_str());

         SymbolString::symbolParse(NULL, myRatingStr + divider + totalRatingStr, symbols, 
                                   LevelInfoContext, (S32)RatingSize, true, &Colors::red);
      }
   }

   SymbolString titleSymbolString(symbols);

   if(isInDatabase && totalRating != UnknownRating)      // No legend for unknown rating
   {
      // Figure out where the ratings will be rendered
      const F32 rightEdge = F32(DisplayManager::getScreenInfo()->getGameCanvasWidth() + titleSymbolString.getWidth()) / 2;

      const F32 x1 = rightEdge - totalSignlessRatingWidth - totalSignWidth - dividerWidth - mySignlessRatingWidth / 2;
      const F32 y1 = 10;
      const F32 ybot = 30;
      const F32 xright = x1 + 35;
      const F32 legendSize = 10;
      const F32 textleft = xright + 5;

      // Draw a legend for the ratings
      glColor(Colors::gray70);

      drawVertLine(x1, y1, ybot);
      drawHorizLine(x1, xright, y1);
      drawString_fixed(textleft, y1 + legendSize / 2, legendSize, "Your rating");

      const F32 x2 = rightEdge - totalSignlessRatingWidth / 2;
      const F32 y2 = 22;

      drawVertLine(x2, y2, ybot);
      drawHorizLine(x2, xright, y2);
      drawString_fixed(textleft, y2 + legendSize / 2, legendSize, "Total rating");
   }



   const char *descr           = gameType->getLevelDescription();
   const S32 descriptionSize   = 20;
   const S32 descriptionHeight = showDescr ? descriptionSize + 8 : 0;

   const char *designedBy      = "Designed By:";
   const char *credits         = gameType->getLevelCredits()->getString();
   const S32 creditsSize       = 20;
   const S32 creditsHeight     = showCredits ? creditsSize + 8 : 0;
   
   const S32 frameMargin       = UserInterface::vertMargin;

   const S32 totalHeight = frameMargin + titleSize + titleGap + descriptionHeight + creditsHeight + frameMargin;
   const S32 totalWidth = DisplayManager::getScreenInfo()->getGameCanvasWidth() - 60;
   S32 yPos = frameMargin + titleSize;

   // Draw top info box
   renderSlideoutWidgetFrame((DisplayManager::getScreenInfo()->getGameCanvasWidth() - totalWidth) / 2, 0, totalWidth, totalHeight, Colors::blue);

   glColor(Colors::white);
   titleSymbolString.render(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, yPos, AlignmentCenter);

   yPos += titleGap;

   if(showDescr)
   {
      glColor(Colors::magenta);
      drawCenteredString(yPos, descriptionSize, descr);
      yPos += descriptionHeight;
   }

   if(showCredits)
   {
      drawCenteredStringPair(yPos, creditsSize, Colors::cyan, Colors::red, designedBy, credits);
      yPos += creditsHeight;
   }

   glPopMatrix();

   /////
   // Auxilliary side panel

   glPushMatrix();
   glTranslate(-getInsideEdge(), 0, 0);

   bool showTwoLinesOfInstructions = gameType->getInstructionString()[1];     // Show 'em if we got 'em

   const S32 sideBoxY = 275;     // Top edge of side box
   const S32 sideMargin = UserInterface::horizMargin;

   const S32 gameTypeTextSize  = 20;
   const S32 gameTypeMargin    =  8;
   const S32 gameTypeHeight    = gameTypeTextSize + gameTypeMargin;

   const S32 instructionSize   = 13;
   const S32 instructionMargin =  5;
   const S32 postInstructionMargin = 8 - instructionMargin;
   const S32 instructionHeight = instructionSize + instructionMargin;

   const S32 scoreToWinSize    = 20;
   const S32 scoreToWinMargin  =  6;
   const S32 scoreToWinHeight  = scoreToWinSize  + scoreToWinMargin;

   const S32 sideBoxTotalHeight = frameMargin + gameTypeHeight + instructionHeight * (showTwoLinesOfInstructions ? 2 : 1) + 
                                  postInstructionMargin + scoreToWinHeight + frameMargin;

   const S32 instrWidth = max(getStringWidth(instructionSize, gameType->getInstructionString()[0]), 
                              showTwoLinesOfInstructions ? getStringWidth(instructionSize, gameType->getInstructionString()[1]) : 0); 

   // Prefix game type with "Team" if they are typically individual games, but are being played in team mode
   bool team = gameType->canBeIndividualGame() && gameType->getGameTypeId() != SoccerGame && teamCount > 1;
   string gt  = string(team ? "Team " : "") + gameType->getGameTypeName();
   string sgt = string("[") + gameType->getShortName() + "]";

   const S32 gameTypeWidth = getStringPairWidth(gameTypeTextSize, LevelInfoHeadlineContext, 
                                                LevelInfoHeadlineContext, gt.c_str(), sgt.c_str());


   static const char *scoreToWinStr = "Score to Win:";
   
   FontManager::pushFontContext(LevelInfoHeadlineContext);
   const S32 scoreToWinWidth = getStringWidthf(scoreToWinSize, "%s%d", scoreToWinStr, gameType->getWinningScore()) + 5;
   FontManager::popFontContext();

   const S32 sideBoxWidth = max(instrWidth, max(gameTypeWidth, scoreToWinWidth)) + sideMargin * 2;
   const S32 sideBoxCen   = DisplayManager::getScreenInfo()->getGameCanvasWidth() - sideBoxWidth / 2;
   
   renderSlideoutWidgetFrame(DisplayManager::getScreenInfo()->getGameCanvasWidth() - sideBoxWidth, sideBoxY, sideBoxWidth, sideBoxTotalHeight, Colors::blue);

   yPos = sideBoxY + frameMargin;

   drawCenteredStringPair(sideBoxCen, yPos, gameTypeTextSize, LevelInfoHeadlineContext, LevelInfoHeadlineContext,
                          Colors::white, Colors::cyan, gt.c_str(), sgt.c_str());

   yPos += gameTypeHeight;

   glColor(Colors::yellow);
   drawCenteredString(sideBoxCen, yPos, instructionSize, gameType->getInstructionString()[0]);
   yPos += instructionHeight;

   // Add a second line of instructions if there is one...
   if(showTwoLinesOfInstructions)
   {
      drawCenteredString(sideBoxCen, yPos, instructionSize, gameType->getInstructionString()[1]);
      yPos += instructionHeight;
   }

   yPos += postInstructionMargin;

   drawCenteredStringPair(sideBoxCen, yPos, scoreToWinSize, LevelInfoHeadlineContext, LevelInfoHeadlineContext, 
                          Colors::cyan, Colors::red, scoreToWinStr, itos(gameType->getWinningScore()).c_str());
   yPos += scoreToWinHeight;

   glPopMatrix();

   FontManager::popFontContext();
}


bool LevelInfoDisplayer::isActive() const
{
   return Parent::isActive() || mDisplayTimer.getCurrent() > 0;
}


bool LevelInfoDisplayer::isDisplayTimerActive() const
{
   return mDisplayTimer.getCurrent() > 0;
}



} } // Nested namespace
