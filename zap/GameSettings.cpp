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
#include "SharedConstants.h"  // For MAX_PLAYERS
#include "config.h"           // For IniSettings, CmdLineSettings defs
#include "BanList.h"
#include "stringUtils.h"      // For itos

using namespace std;

namespace Zap
{

// For now...  soon all these things will be contained herein
extern CIniFile gINI;

// Constructor
GameSettings::GameSettings()
{
   mBanList = new BanList(mFolderManager.iniDir);
}

// Destructor
GameSettings::~GameSettings()
{
   delete mBanList;
}


// Helpers for init functions below
static const string *choose(const string &firstChoice, const string &secondChoice)
{
   return firstChoice != "" ? &firstChoice : &secondChoice;
}


static const string *choose(const string &firstChoice, const string &secondChoice, const string &thirdChoice)
{
   return choose(firstChoice, *choose(secondChoice, thirdChoice));
}


void GameSettings::setHostName(const string &hostName, bool updateINI) 
{ 
   mHostName = hostName; 

   if(updateINI)
      mIniSettings.hostname = hostName; 
}


void GameSettings::initHostName(const string &cmdLineVal, const string &iniVal)
{
   mHostName = *choose(cmdLineVal, iniVal);
}


void GameSettings::setHostDescr(const string &hostDescr, bool updateINI) 
{ 
   mHostDescr = hostDescr;
   
   if(updateINI)
      mIniSettings.hostdescr = hostDescr; 
}


void GameSettings::initHostDescr(const string &cmdLineVal, const string &iniVal)
{
   mHostDescr = *choose(cmdLineVal, iniVal);
}


void GameSettings::setServerPassword(const string &serverPassword, bool updateINI) 
{ 
   mServerPassword = serverPassword; 

   if(updateINI)
      mIniSettings.serverPassword = serverPassword; 
}


void GameSettings::initServerPassword(const string &cmdLineVal, const string &iniVal)
{
   mServerPassword = *choose(cmdLineVal, iniVal, "");
}


void GameSettings::setAdminPassword(const string &adminPassword, bool updateINI) 
{ 
   mAdminPassword = adminPassword; 

   if(updateINI)
      mIniSettings.adminPassword = adminPassword; 
}


void GameSettings::initAdminPassword(const string &cmdLineVal, const string &iniVal)
{
   mAdminPassword = *choose(cmdLineVal, iniVal, "");
}


void GameSettings::setLevelChangePassword(const string &levelChangePassword, bool updateINI) 
{ 
   mLevelChangePassword = levelChangePassword; 

   if(updateINI)
      mIniSettings.levelChangePassword = levelChangePassword; 
}


void GameSettings::initLevelChangePassword(const string &cmdLineVal, const string &iniVal)
{
   mLevelChangePassword = *choose(cmdLineVal, iniVal, "");
}


extern U16 DEFAULT_GAME_PORT;

// Will be cleaned up when gINI is integrated
// NOTE: I believe that this accurately replicates how things were set before, but I'm not sure this makes sense.  There seem to be many
// ways to set the address.  Do we need all of them?  Is the order of precedence logical?  I think we can get rid of everything except the two
// hostaddr options.
string GameSettings::getHostAddress()
{
   if(mCmdLineSettings.hostaddr != "")
      return mCmdLineSettings.hostaddr;

   if(mIniSettings.hostaddr != "")
      return mIniSettings.hostaddr;

   // Should we get rid of this option?
   if(mCmdLineSettings.dedicated != "")     // Should only be true if dedicated server
      return mCmdLineSettings.dedicated;

   // Should we get rid of this option?
   if(mCmdLineSettings.server != "")
      return mCmdLineSettings.server;

   return "IP:Any:" + itos(DEFAULT_GAME_PORT);
}


U32 GameSettings::getMaxPlayers()
{
   U32 maxplayers;

   if(mCmdLineSettings.maxPlayers > 0)
      maxplayers = mCmdLineSettings.maxPlayers;
   else
      maxplayers = mIniSettings.maxPlayers;

   if(maxplayers > MAX_PLAYERS)
      maxplayers = MAX_PLAYERS;

   return maxplayers;
}


extern void saveWindowMode(CIniFile *ini, IniSettings *iniSettings);

// Write all our settings to bitfighter.ini
void GameSettings::save()
{
   //   BanList *bl = settings->getBanList();
   //   bl->writeToFile();      // Writes ban list back to file XXX enable this when admin functionality is built in

   saveWindowMode(&gINI, &mIniSettings);              
   saveSettingsToINI(&gINI, this);        // Writes settings to gINI, then writes it to disk
}



};