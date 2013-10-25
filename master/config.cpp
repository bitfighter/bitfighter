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

#include "master.h"     

#include "../zap/SharedConstants.h"
#include "../zap/stringUtils.h"
#include "../zap/IniFile.h"

#include "tnl.h"
#include "tnlLog.h"

#include <stdio.h>
#include <string>
#include <map>

using namespace TNL;
using namespace std;
using namespace Zap;

extern Vector<string> master_admins;

namespace Zap {
   extern string gSqlite;
}

extern map <U32, string> gMOTDClientMap;



void getCurrentMOTDFromFile(const string &filename, string &fillMessage)
{
   char *file = strdup(filename.c_str());    // Message stored in this file

   FILE *f = fopen(file, "r");
   if(!f)
   {
      logprintf(LogConsumer::LogError, "Unable to open MOTD file \"%s\" -- using default MOTD.", file);
      return;
   }

   char message[MOTD_LEN];
   if(fgets(message, MOTD_LEN, f))
      fillMessage = message;
   else
      fillMessage = "Welcome to Bitfighter!";  // Some default

   fclose(f);
}


void loadSettingsFromINI(const CIniFile *ini, MasterSettings *masterSettings)
{
   // Read all settings defined in the new modern manner
   string sections[] = { "host", "phpbb", "stats", "motd" };

   for(S32 i = 0; i < ARRAYSIZE(sections); i++)
   {
      string section = sections[i];

      // Enumerate all settings we've defined for [section]
      Vector<AbstractSetting *> settings = masterSettings->mSettings.getSettingsInSection(section);   

      for(S32 j = 0; j < settings.size(); j++)
         settings[j]->setValFromString(ini->GetValue(section, settings[j]->getKey(), settings[j]->getDefaultValueString()));
   }


   // Got to do something about this!
   string str1 = ini->GetValue("host", "master_admin", "");
   parseString(str1.c_str(), master_admins, ',');



   // [stats] section --> most has been modernized
   Zap::gSqlite = ini->GetValue("stats", "sqlite_file_basename", Zap::gSqlite);


   // [motd_clients] section
   // This section holds each old client build number as a key.  This allows us to set
   // different messages for different versions
   string defaultMessage = "New version available at bitfighter.org";
   Vector<string> keys;
   ini->GetAllKeys("motd_clients", keys);

   gMOTDClientMap.clear();

   for(S32 i = 0; i < keys.size(); i++)
   {
      U32 build_version = (U32)Zap::stoi(keys[i]);    // Avoid conflicts with std::stoi() which is defined for VC++ 10
      string message = ini->GetValue("motd_clients", keys[i], defaultMessage);

      gMOTDClientMap.insert(pair<U32, string>(build_version, message));
   }


   // [motd] section
   // Here we just get the name of the file.  We use a file so the message can be updated
   // externally through the website
   string motdFilename = ini->GetValue("motd", "motd_file", "motd");  // Default 'motd' in current directory

   // Grab the current message
   string fillMessage;
   getCurrentMOTDFromFile(motdFilename, fillMessage);

   // Add it to the map as the most recently released build
   gMOTDClientMap[masterSettings->getVal<U32>("LatestReleasedBuildVersion")] = fillMessage;
}


void readConfigFile(CIniFile *ini, MasterSettings *masterSettings) 
{
   // First clear
   ini->Clear();

   // Now read
   ini->ReadFile();

   // Now set up variables -- copies data from ini to settings
   loadSettingsFromINI(ini, masterSettings);

   // Not sure if this should go here...
   if(masterSettings->getVal<U32>("LatestReleasedCSProtocol")   == 0 && 
      masterSettings->getVal<U32>("LatestReleasedBuildVersion") == 0)
       logprintf(LogConsumer::LogError, "Unable to find a valid protocol line or build_version in config file... disabling update checks!");
}

