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

   errUI->activate();
}


void HighScoresUserInterface::idle(U32 timeDelta)
{

}


void HighScoresUserInterface::onActivate()
{
   //c2mRequestHighScores();
   mHaveHighScores = false;
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
