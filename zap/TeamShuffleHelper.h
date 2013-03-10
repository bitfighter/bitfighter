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

#ifndef _TEAMSHUFFLE_H_
#define _TEAMSHUFFLE_H_

#include "helperMenu.h"

using namespace TNL;

namespace Zap
{

class ClientInfo;

class TeamShuffleHelper : public HelperMenu
{
   typedef HelperMenu Parent;

private:
   const char *getCancelMessage();
   InputCode getActivationKey();

   S32 columnWidth;
   S32 maxColumnWidth;
   S32 rowHeight;
   S32 rows, cols;
   S32 teamCount;
   S32 playersPerTeam;

   S32 topMargin, leftMargin;

   static const S32 vpad = 10, hpad = 14;    // Padding inside the boxes
   static const S32 TEXT_SIZE = 22;
   static const S32 margin = 10;

   Vector<Vector<ClientInfo *> > mTeams;
   void shuffle();
   void calculateRenderSizes();

public:
   explicit TeamShuffleHelper();             // Constructor

   HelperMenuType getType();

   void render();                
   void onMenuShow();  
   bool processInputCode(InputCode inputCode);   

   bool isMovementDisabled();

   void onPlayerJoined();
   void onPlayerQuit();
};

};

#endif


