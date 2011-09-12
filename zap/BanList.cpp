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

#include <boost/date_time/posix_time/posix_time.hpp>

#include <fstream>
#include <iostream>

using namespace boost::posix_time;

namespace Zap
{

// Constructor
BanList::BanList(const string &iniDir)
{
   banListTokenDelimiter = "|";
   banListWildcardCharater = "*";
}


// Destructor
BanList::~BanList()
{
   // Do nothing
}


bool BanList::addToBanList(BanItem *banItem)
{
   // TODO call this from an admin command?
   return false;
}


bool BanList::removeFromBanList(BanItem *banItem)
{
   // TODO call this from an admin command?
   return false;
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
   string startDateTime = words[2];
   string durationMinutes = words[3];

   // Validate IP address string
   if (!(Address(ipAddress.c_str()).isValid()) && ipAddress.compare(banListWildcardCharater) != 0)
      return false;

   // nickname could be anything...

   // Validate date
   ptime tempDateTime;
   // If exception is thrown, then date didn't parse correctly
   try
   {
      tempDateTime = from_iso_string(startDateTime);
   }
   catch (...)
   {
      return false;
   }
   // If date time ends up equaling empty ptime, then it didn't parse right either
   if (tempDateTime == ptime())
      return false;

   // Validate duration
   if(stoi(durationMinutes) <= 0)
      return false;

   // Now finally add to banList
   BanItem banItem;
   banItem.ipAddress = ipAddress;
   banItem.nickname = nickname;
   banItem.startDateTime = startDateTime;
   banItem.durationMinutes = durationMinutes;

   serverBanList.push_back(banItem);

   // Phoew! we made it..
   return true;
}


string BanList::banItemToString(BanItem *banItem)
{
   // IP, nickname, startTime, duration <- in this order
   return
         banItem->ipAddress + banListTokenDelimiter +
         banItem->nickname + banListTokenDelimiter +
         banItem->startDateTime + banListTokenDelimiter +
         banItem->durationMinutes;
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
      if (from_iso_string(serverBanList[i].startDateTime) + minutes(stoi(serverBanList[i].durationMinutes)) < currentTime)
         continue;

      // If we get here, that means nickname and IP address matched and we are still in the
      // ban allotted time period
      return true;
   }

   return false;
}

string BanList::getDelimiter()
{
   return banListTokenDelimiter;
}


Vector<string> BanList::banListToString()
{
   Vector<string> banList;
   for(S32 i = 0; i < serverBanList.size(); i++)
   {
      banList.push_back(banItemToString(&serverBanList[i]));
   }

   return banList;
}


void BanList::loadBanList(const Vector<string> &banItemList)
{
   for(S32 i = 0; i < banItemList.size(); i++)
      if(!processBanListLine(banItemList[i]))
         logprintf("Ban list item on line %d is malformed: %s", i+1, banItemList[i].c_str());
      else
         logprintf("Loading ban: %s", banItemList[i].c_str());
}


} /* namespace Zap */
