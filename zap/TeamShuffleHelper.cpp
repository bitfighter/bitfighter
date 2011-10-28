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

#include "TeamShuffleHelper.h"
#include "ClientGame.h"
#include "UIGame.h"
#include "UIMenus.h"

#include "SDL/SDL_opengl.h"


namespace Zap
{

// Constructor
TeamShuffleHelper::TeamShuffleHelper(ClientGame *clientGame) : Parent(clientGame)
{
   // Do nothing
}


// Gets called at the beginning of every game; available options may change based on level
void TeamShuffleHelper::initialize(bool includeEngineer)
{

}


void TeamShuffleHelper::onMenuShow()
{

}


void TeamShuffleHelper::render()
{
   UserInterface::drawFilledRect(100,100,500,500,Colors::red, Colors::yellow);
}


// Return true if key did something, false if key had no effect
// Runs on client
bool TeamShuffleHelper::processInputCode(InputCode inputCode)
{
   if(Parent::processInputCode(inputCode))    // Check for cancel keys
      return true;

   // We're interested in space, enter, and escape

   return true;
}


InputCode TeamShuffleHelper::getActivationKey() 
{ 
   return inputLOADOUT[getGame()->getSettings()->getIniSettings()->inputMode]; 
}


bool TeamShuffleHelper::isMovementDisabled()
{
   return true;
}


};

