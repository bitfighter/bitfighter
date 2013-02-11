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

using namespace TNL;
using namespace std;

namespace Zap
{

class BanList
{
private:
   struct BanItem
   {
      string address;
      string nickname;
      string startDateTime;
      string durationMinutes;
   };

   struct KickedHost {
      Address address;
      U32 kickTimeRemaining;
   };

   Vector<BanItem> serverBanList;
   Vector<KickedHost> serverKickList;

   string banListTokenDelimiter;
   string banListWildcardCharater;

   S32 defaultBanDurationMinutes;
   S32 kickDurationMilliseconds;

   bool processBanListLine(const string &line);
   string banItemToString(BanItem *banItem);

public:
   explicit BanList(const string &iniDir);
   virtual ~BanList();

   void addToBanList(const Address &address, S32 durationMinutes, bool nonAuthenticatedOnly = false);
   void addPlayerNameToBanList(const char *playerName, S32 durationMinutes);
   void removeFromBanList(const Address &address);

   bool isBanned(const Address &address, const string &nickname, bool isAuthenticated);

   string getDelimiter();
   string getWildcard();
   S32 getKickDuration();
   S32 getDefaultBanDuration();

   Vector<string> banListToString();
   void loadBanList(const Vector<string> &banItemList);

   void kickHost(const Address &address);       // Add an address to kick list
   bool isAddressKicked(const Address &address);   // Check if address is on the kick list
   void updateKickList(U32 timeElapsed);              // Check if kick time has expired and update the kick list
};

} /* namespace Zap */
#endif /* BANLIST_H_ */
