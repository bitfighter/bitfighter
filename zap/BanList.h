//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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
