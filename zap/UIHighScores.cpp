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

#include "gameObjectRender.h"
#include "masterConnection.h"   
#include "ClientGame.h"
#include "UIErrorMessage.h"

namespace Zap
{


////////////////////////////////////
////////////////////////////////////


// Constructor
HighScoresUserInterface::HighScoresUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(HighScoresUI);
   mHaveHighScores = false;
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
   // XXX: Ugly ugly ugly - rewrite entire method to be pretty
   S32 y = vertMargin;
   S32 headerSize = 32;
   S32 titleSize = 20;
   S32 textSize = 16;
   S32 x = horizMargin;

   glColor(Colors::white);

   drawCenteredStringf(y, headerSize, "HIGH SCORES");
   y += headerSize + 20;

   for(S32 i = 0; i < mScoreGroups.size(); i++)
   {
      drawStringf(x, y, titleSize, mScoreGroups[i].title.c_str());
      y += titleSize + 5;

      // Draw line
      drawHorizLine(x, x + 280, y);
      y += 5;

      // Now draw names
      for(S32 j = 0; j < mScoreGroups[i].names.size(); j++)
      {
         drawStringf(x, y, textSize, "%d %s", mScoreGroups[i].scores[j], mScoreGroups[i].names[j].c_str());
         y += textSize + 2;
      }

      y += 20;
   }
}


void HighScoresUserInterface::renderWaitingForScores()
{
   MasterServerConnection *connToMaster = getGame()->getConnectionToMaster();

   string msg;

   ErrorMessageUserInterface *errUI = getGame()->getUIManager()->getErrorMsgUserInterface();
   errUI->reset();
   errUI->setInstr("");


   if(connToMaster && connToMaster->getConnectionState() == NetConnection::Connected)
   {
      errUI->setTitle("");

      errUI->setMessage(1, "Waiting for scores");
      errUI->setMessage(2, "to be sent");
      errUI->setMessage(3, "from Master Server...");   

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
   MasterServerConnection *conn = getGame()->getConnectionToMaster();

   if(conn)
      conn->c2mRequestHighScores();
}


void HighScoresUserInterface::onReactivate()
{
   quit();     // Got here from ErrorMessageUserInterface, which we "borrow" for rendering some of our messages
}


bool HighScoresUserInterface::onKeyDown(InputCode inputCode, char ascii) 
{
   if(!Parent::onKeyDown(inputCode, ascii))
   quit();            // Quit when any key is pressed...  any key at all.  Except a couple.

   return true;
}


void HighScoresUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();
}



};
