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

#ifndef BANLIST_H_
#define BANLIST_H_

#include "tnlTypes.h"
#include "tnlUDP.h"

#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

using namespace TNL;
using namespace std;
using namespace boost::posix_time;

namespace Zap
{


struct BanItem
{
   string ipAddress;
   string nickname;
   ptime startTime;
   S32 durationMinutes;
};


class BanList
{
private:
   Vector<BanItem> serverBanList;
   string banListFileName;
   string banListFilePath;
   string banListTokenDelimiter;
   string banListWildcardCharater;

   bool processBanListLine(const string &line);
   string banItemToString(BanItem banItem);
   string ptimeToIsoString(ptime time);

public:
   BanList(const string &iniDir);
   virtual ~BanList();

   bool addToBanList(BanItem banItem);
   bool removeFromBanList(BanItem banItem);
   bool isBanned(Address ipAddress, string nickname);

   bool writeToFile();
   void readFromFile();
};

} /* namespace Zap */
#endif /* BANLIST_H_ */
