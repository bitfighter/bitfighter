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

#ifdef _MSC_VER
#  pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#include "gameLoader.h"
#include "stringUtils.h"

#include "tnl.h"
#include "tnlLog.h"

#include <fstream>
#include <sstream>

#include <stdio.h>

#  ifdef TNL_OS_WIN32
#     include "../other/dirent.h"        // Need local copy for Windows builds
#  else
#     include <dirent.h>        // Need standard copy for *NIXes
#  endif


using namespace TNL;


namespace Zap
{


using namespace std;



// Constructor
LevelLoadException::LevelLoadException(string str) : msg(str)
{
   // Do nothing
}


// Destructor, required by gcc to avoid "looser throw" error
LevelLoadException::~LevelLoadException() throw ()
{
   // Do nothing
}


const char *LevelLoadException::what() const throw ()
{
   return msg.c_str();
}


};

