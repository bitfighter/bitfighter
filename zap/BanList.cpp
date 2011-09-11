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

#include "BanList.h"
#include "stringUtils.h"
#include "config.h"

#include "tnlLog.h"

#include <fstream>
#include <iostream>

namespace Zap
{

// Constructor
BanList::BanList(const string &iniDir)
{
   banListTokenDelimiter = "|";
   banListWildcardCharater = "*";
   banListFileName = "ban_list.txt";

   // Ban list file should be in iniDir
   banListFilePath = joindir(iniDir, banListFileName);
   //serverBanList = Vector<BanItem>();      ==> not needed... happens automatically!  raptor: Delete this when you've read it!
}


// Destructor
BanList::~BanList()
{
   // Do nothing
}


bool BanList::addToBanList(BanItem banItem)
{
   // TODO call this from an admin command?
   return false;
}


bool BanList::removeFromBanList(BanItem banItem)
{
   // TODO call this from an admin command?
   return false;
}

// Make ban list always have windows line endings, like INI.  Why??  For consistency, I think..
#if defined(WIN32)
#define EOL endl
#else
#define EOL '\r' << endl
#endif

bool BanList::writeToFile()
{
   ofstream file(banListFilePath.c_str());

   if(file.fail())
   {
      file.close();
      return false;
   }

   // Write BanItems to file
   string line;

   for(S32 i = 0; i < serverBanList.size(); i++) {
      line = banItemToString(serverBanList[i]);

      file << line << EOL;
   }

   file.close();

   return true;
}

void BanList::readFromFile()
{
   ifstream file(banListFilePath.c_str());

   Vector<string> banListFileLines;

   // If file didn't fail to load, load each line into a Vector
   if(!file.fail())
   {
      string line;
      while(getline(file, line))
         banListFileLines.push_back(line);
   }

   file.close();

   // Process each line of the ban list file
   for(S32 i = 0; i < banListFileLines.size(); i++)
      if(!processBanListLine(banListFileLines[i]))
         logprintf("Ban list item on line %d is malformed: %s", i+1, banListFileLines[i].c_str());
}


bool BanList::processBanListLine(const string &line)
{
   // Tokenize the line
   Vector<string> words;
   parseString(line.c_str(), words, banListTokenDelimiter[0]);

   // Check for incorrect number of tokens => 4, which is the member count of the BanItem struct
   if (words.size() != 4)
      return false;

   // Check to make sure there is at lease one character in each token
   for (S32 i = 0; i < 4; i++)
      if(words[i].length() < 1)
         return false;

   // IP, nickname, startTime, duration <- in this order
   string ipAddress = words[0];
   string nickname = words[1];
   string dateTimeString = words[2];
   string durationMinutesString = words[3];

   // Validate IP address string
   if (!(Address(ipAddress.c_str()).isValid()) && ipAddress.compare(banListWildcardCharater) != 0)
      return false;

   // nickname could be anything...

   // Validate date
   ptime startTime;
   // If exception is thrown, then date didn't parse correctly
   try
   {
      startTime = from_iso_string(dateTimeString);
   }
   catch (...)
   {
      return false;
   }
   // If date time ends up equaling empty ptime, then it didn't parse right either
   if (startTime == ptime())
      return false;

   // Validate duration
   S32 durationMinutes = stoi(durationMinutesString);
   if(durationMinutes <= 0)
      return false;

   // Now finally add to banList
   BanItem banItem;
   banItem.ipAddress = ipAddress;
   banItem.nickname = nickname;
   banItem.startTime = startTime;
   banItem.durationMinutes = durationMinutes;

   serverBanList.push_back(banItem);

   // Phoew! we made it..
   return true;
}


string BanList::banItemToString(BanItem banItem)
{
   // IP, nickname, startTime, duration <- in this order
   return
         banItem.ipAddress + banListTokenDelimiter +
         banItem.nickname + banListTokenDelimiter +
         ptimeToIsoString(banItem.startTime) + banListTokenDelimiter +
         itos(banItem.durationMinutes);
}


// Custom toString method so we don't have to compile extra boost sources
// This is super slow because of using streams...  not sure how to fix
string BanList::ptimeToIsoString(ptime time)
{
   ostringstream formatter;
   formatter.imbue(locale(cout.getloc(), new boost::posix_time::time_facet("%Y%m%dT%H%M%S")));
   formatter << time;

   return formatter.str();
}


bool BanList::isBanned(Address address, string nickname)
{
   // Build proper IP Address string
   char addressBuffer[16];
   dSprintf(addressBuffer, 16, "%d.%d.%d.%d", U8(address.netNum[0] >> 24 ),
         U8 (address.netNum[0] >> 16 ), U8 (address.netNum[0] >> 8), U8(address.netNum[0]));

   string ipAddress(addressBuffer);
   ptime currentTime = second_clock::local_time();

   for (S32 i = 0; i < serverBanList.size(); i++)
   {
      // Check IP
      if (ipAddress.compare(serverBanList[i].ipAddress) != 0 && serverBanList[i].ipAddress.compare("*") != 0)
         continue;

      // Check nickname
      if (nickname.compare(serverBanList[i].nickname) != 0 && serverBanList[i].nickname.compare("*") != 0)
         continue;

      // Check time
      if (serverBanList[i].startTime + minutes(serverBanList[i].durationMinutes) < currentTime)
         continue;

      // If we get here, that means nickname and IP address matched and we are still in the
      // ban allotted time period
      return true;
   }

   return false;
}


} /* namespace Zap */
