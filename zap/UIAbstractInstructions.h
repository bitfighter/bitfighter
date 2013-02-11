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


#ifndef _UIABSTRACTINSTRUCTIONS_H_
#define _UIABSTRACTINSTRUCTIONS_H_


#include "UI.h"


namespace Zap {

struct ControlStringsEditor
{
   string command;
   string descr;
};


////////////////////////////////////////
////////////////////////////////////////

class AbstractInstructionsUserInterface : public UserInterface
{
   typedef UserInterface Parent;

protected:
   void renderConsoleCommands(const char *activationCommand, const ControlStringsEditor *cmdList);

public:
   explicit AbstractInstructionsUserInterface(ClientGame *clientGame);      // Constructor
};

}

#endif


