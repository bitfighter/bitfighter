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

#include "UIHighScores.h"

#include "UIErrorMessage.h"
#include "UIManager.h"

#include "ClientGame.h"
#include "gameObjectRender.h"
#include "masterConnection.h"   
#include "ScreenInfo.h"          // For canvas dimensions

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


extern ScreenInfo gScreenInfo;

void HighScoresUserInterface::renderScores()
{
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

      S32 x = col == 0 ? horizMargin : gScreenInfo.getGameCanvasWidth() / 2;

      glColor(Colors::palePurple);

      drawString(x, y, titleSize, mScoreGroups[i].title.c_str());
      y += titleSize + 5;

      // Draw line
      glColor(Colors::gray70);
      drawHorizLine(x, x + gScreenInfo.getGameCanvasWidth() / 2 - 2 * horizMargin, y);
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

   drawCenteredString(gScreenInfo.getGameCanvasHeight() - vertMargin - titleSize, titleSize, "The week ends Sunday/Monday at 0:00:00 UTC Time");
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
      errUI->setTitle("");

      errUI->setMessage(1, "Retrieving scores");
      errUI->setMessage(2, "from Master Server...");   

      errUI->setPresentation(1);

   }
   else     // Let the user know they are not connected to master and shouldn't wait
   {
      errUI->setTitle("NO CONNECTION TO MASTER");

      errUI->setMessage(1, "");
      errUI->setMessage(2, "High Scores are currently unavailable");
      errUI->setMessage(3, "because there is no connection");
      errUI->setMessage(4, "to the Bitfighter Master Server.");
      errUI->setMessage(6, "");
      errUI->setMessage(7, "Firewall issues?  Do you have the latest version?");
      errUI->setMessage(8, "");
      errUI->setMessage(9, "");

      errUI->setPresentation(0);
   }      

   // Only render, don't activate so we don't have to deactivate when we get the high scores
   errUI->render();
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
         currScores.push_back(scores[i  *scoresPerGroup + j]);                      
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
