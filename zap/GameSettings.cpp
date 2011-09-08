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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "GameSettings.h"
#include "config.h"     // For IniSettings def

using namespace std;

namespace Zap
{

struct IniSettings;
extern IniSettings gIniSettings;    // For now...


// Helper for init functions below
static const string *choose(const string &firstChoice, const string &secondChoice)
{
   return firstChoice != "" ? &firstChoice : &secondChoice;
}


void GameSettings::setHostName(const string &hostName) 
{ 
   mHostName = hostName; 
   gIniSettings.hostname = hostName; 
}


void GameSettings::initHostName(const string &cmdLineVal, const string &iniVal)
{
   mHostName = *choose(cmdLineVal, iniVal);
}


void GameSettings::setHostDescr(const string &hostDescr) 
{ 
   mHostDescr = hostDescr; 
   gIniSettings.hostdescr = hostDescr; 
}


void GameSettings::initHostDescr(const string &cmdLineVal, const string &iniVal)
{
   mHostDescr = *choose(cmdLineVal, iniVal);
}


};