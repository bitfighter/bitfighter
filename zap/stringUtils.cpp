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

#include "tnl.h"     // For types and dSprintf
#include <string>

using namespace TNL;

namespace Zap
{

// Collection of useful string things

std::string ExtractDirectory( const std::string& path )
{
  return path.substr( 0, path.find_last_of( '\\' ) + 1 );
}

std::string ExtractFilename( const std::string& path )
{
  return path.substr( path.find_last_of( '\\' ) + 1 );
}


std::string itos(S32 i)
{
   char outString[100];
   dSprintf(outString, sizeof(outString), "%d", i);
   return outString;
}

};

