//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIHighScores.h"

#include "UIErrorMessage.h"
#include "UIManager.h"

#include "ClientGame.h"
#include "gameObjectRender.h"
#include "masterConnection.h"   
#include "DisplayManager.h"          // For canvas dimensions

#include "FontManager.h"
#include "Colors.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"

namespace Zap
{


////////////////////////////////////
////////////////////////////////////


// Constructor
HighScoresUserInterface::HighScoresUserInterface(ClientGame *game) : Parent(game)
{
   mHaveHighScores = false;
}

// Destructor
HighScoresUserInterface::~HighScoresUserInterface()
{
   // Do nothing
}


void HighScoresUserInterface::render()
{
   if(mHaveHighScores)
      renderScores();
   else
      renderWaitingForScores();
}


void HighScoresUserInterface::renderScores()
{
   FontManager::pushFontContext(HelpContext);

   S32 y = vertMargin;
   S32 headerSize = 32;
   S32 titleSize = 20;
   S32 textSize = 16;
   S32 gapBetweenNames = textSize / 3;
   S32 gapAfterTitle = 80;
   S32 gapBetweenGroups = 40;
   S32 scoreIndent = 10;

   glColor(Colors::green);

   drawCenteredUnderlinedString(y, headerSize, "BITFIGHTER HIGH SCORES");
   y += gapAfterTitle;

   S32 col = 0;   // 0 = left col, 1 = right col
   S32 yStart;

   for(S32 i = 0; i < mScoreGroups.size(); i++)
   {
      yStart = y;    // For future reference

      S32 x = col == 0 ? horizMargin : DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2;

      glColor(Colors::palePurple);

      drawString(x, y, titleSize, mScoreGroups[i].title.c_str());
      y += titleSize + 5;

      // Draw line
      glColor(Colors::gray70);
      drawHorizLine(x, x + DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2 - 2 * horizMargin, y);
      y += 5;


      S32 w = -1;

      // Now draw names
      for(S32 j = 0; j < mScoreGroups[i].names.size(); j++)
      {
         glColor(Colors::cyan);

         // First gap will always be largest if scores are descending...
         if(w == -1)
            w = getStringWidth(textSize, mScoreGroups[i].scores[j].c_str());

         drawStringr(x + scoreIndent + w, y, textSize, mScoreGroups[i].scores[j].c_str());

         glColor(Colors::yellow);
         drawStringAndGetWidth(x + scoreIndent + w + 15, y, textSize, mScoreGroups[i].names[j].c_str());

         y += textSize + gapBetweenNames;
      }

      y += gapBetweenGroups;

      col = 1 - col;    // Toggle col between 0 and 1
      if(col == 1)
         y = yStart;
   }

   glColor(Colors::red80);

   drawCenteredString(DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin - titleSize, titleSize, "The week ends Sunday/Monday at 0:00:00 UTC Time");

   FontManager::popFontContext();
}


void HighScoresUserInterface::renderWaitingForScores()
{
   MasterServerConnection *masterConn = getGame()->getConnectionToMaster();

   string msg;

   ErrorMessageUserInterface *errUI = getGame()->getUIManager()->getUI<ErrorMessageUserInterface>();
   errUI->reset();
   errUI->setInstr("");


   if(masterConn && masterConn->isEstablished())
   {
      SymbolStringSet symbolSet(10);

      Vector<string> lines;
      wrapString("Retrieving scores from Master Server", UIManager::MessageBoxWrapWidth, 18, ErrorMsgContext, lines);

      for(S32 i = 0; i < lines.size(); i++)
         symbolSet.add(SymbolString::getSymbolText(lines[i], 30, ErrorMsgContext, &Colors::blue));

      symbolSet.add(SymbolString(SymbolString::getBlankSymbol(0, 10)));   
      symbolSet.add(SymbolString(SymbolString::getSymbolSpinner(18, &Colors::cyan)));   

      symbolSet.render(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, (DisplayManager::getScreenInfo()->getGameCanvasHeight() - symbolSet.getHeight()) / 2, AlignmentCenter);
   }
   else     // Let the user know they are not connected to master and shouldn't wait
   {
      errUI->setTitle("NO CONNECTION TO MASTER");
      string msg = "High Scores are currently unavailable because there is no connection to the Bitfighter Master Server.\n\n"
                   "Firewall issues?  Do you have the latest version?";

      errUI->setMessage(msg);

      // Only render, don't activate so we don't have to deactivate when we get the high scores
      errUI->render();
   }      
}


void HighScoresUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);
}


void HighScoresUserInterface::setHighScores(Vector<StringTableEntry> groupNames, Vector<string> names, Vector<string> scores)
{
   mScoreGroups.clear();

   S32 scoresPerGroup = names.size() / groupNames.size();

   for(S32 i = 0; i < groupNames.size(); i++)
   {
      ScoreGroup scoreGroup;
      Vector<string> currNames;
      Vector<string> currScores;


      scoreGroup.title = string(groupNames[i].getString());

      for(S32 j = 0; j < scoresPerGroup; j++)    
      {
         currNames .push_back(names [i * scoresPerGroup + j]);    
         currScores.push_back(scores[i * scoresPerGroup + j]);                      
      }

      scoreGroup.names = currNames;
      scoreGroup.scores = currScores;

      mScoreGroups.push_back(scoreGroup);
   }

   if(mScoreGroups.size() > 0)
      mHaveHighScores = true;
}


void HighScoresUserInterface::onActivate()
{
   mHaveHighScores = false;

   MasterServerConnection *masterConn = getGame()->getConnectionToMaster();

   if(masterConn && masterConn->isEstablished())
      masterConn->c2mRequestHighScores();
}


void HighScoresUserInterface::onReactivate()
{
   quit();     // Got here from ErrorMessageUserInterface, which we "borrow" for rendering some of our messages
}


bool HighScoresUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;
   else
      quit();            // Quit when any key is pressed...  any key at all.  Except a couple.

   return false;
}


void HighScoresUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();
}



};
