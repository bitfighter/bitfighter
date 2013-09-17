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

#ifndef _GAMELOADER_H_
#define _GAMELOADER_H_

#include "tnlVector.h"
#include "tnlTypes.h"
#include <string>


using namespace TNL;
using namespace std;

namespace Zap
{



////////////////////////////////////////
////////////////////////////////////////

// Thrown when the loader encounters a line that seems broken for some reason
struct LevelLoadException : public std::exception
{
   string msg;

   explicit LevelLoadException (string str);         // Constructor
   virtual ~LevelLoadException () throw();  // Destructor, required by gcc to avoid "looser throw" error
   const char* what() const throw();
};


};

#endif


