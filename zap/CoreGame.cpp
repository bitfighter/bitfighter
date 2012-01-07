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

#include "CoreGame.h"
#include "item.h"

#ifndef ZAP_DEDICATED
#include "ClientGame.h"
#endif


namespace Zap {


CoreGameType::CoreGameType()
{
   // TODO Auto-generated constructor stub
}

CoreGameType::~CoreGameType()
{
   // TODO Auto-generated destructor stub
}



// Runs on client
void CoreGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
#ifndef ZAP_DEDICATED
   Parent::renderInterfaceOverlay(scoreboardVisible);
   Ship *ship = dynamic_cast<Ship *>(dynamic_cast<ClientGame *>(getGame())->getConnectionToServer()->getControlObject());

   if(!ship)
      return;

   for(S32 i = 0; i < mCores.size(); i++)
      if(mCores[i]->getTeam() != ship->getTeam())
         renderObjectiveArrow(mCores[i], getTeamColor(mCores[i]->getTeam()));
#endif
}


S32 CoreGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
{
   // TODO Auto-generated destructor stub
   return 0;
}



GameTypes CoreGameType::getGameType() const
{
   return CoreGame;
}


const char *CoreGameType::getShortName() const
{
   return "C";
}


const char *CoreGameType::getInstructionString()
{
   return "Destroy all of the opposing team's Cores";
}


bool CoreGameType::canBeTeamGame() const
{
   return true;
}


bool CoreGameType::canBeIndividualGame() const
{
   return false;
}


TNL_IMPLEMENT_NETOBJECT(CoreGameType);

} /* namespace Zap */
