//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _MASTERCONNECTION_H_
#define _MASTERCONNECTION_H_

#include "../master/masterInterface.h"
#include "MasterTypes.h"

#include <string>

using namespace std;
using namespace Master;

namespace Zap
{

class Game;


class MasterServerConnection : public MasterServerInterface
{
   typedef MasterServerInterface Parent;

private:
   U32 mCurrentQueryId;    // ID of our current query

   Game *mGame;
   string mMasterName;

   S32 mClientId;

   void terminateIfAnonymous();

protected:
   MasterConnectionType mConnectionType;

public:
   explicit MasterServerConnection(Game *game);    // Constructor
   MasterServerConnection();              // Default Constructor required by TNL
   virtual ~MasterServerConnection();     // Destructor

   void setConnectionType(MasterConnectionType type);
   MasterConnectionType getConnectionType();

   void setMasterName(string name);
   string getMasterName();

   void startServerQuery();

   Vector<ServerAddr> mServerList;

   void cancelArrangedConnectionAttempt();
   void requestArrangedConnection(const Address &remoteAddress);
   void updateServerStatus(StringTableEntry levelName, StringTableEntry levelType, U32 botCount, 
                           U32 playerCount, U32 maxPlayers, U32 infoFlags);

#ifndef ZAP_DEDICATED
   TNL_DECLARE_RPC_OVERRIDE(m2cQueryServersResponse,      (U32 queryId, Vector<IPAddress> ipList));
   TNL_DECLARE_RPC_OVERRIDE(m2cQueryServersResponse_019a, (U32 queryId, Vector<IPAddress> ipList, Vector<S32> clientIdList));
#endif

   TNL_DECLARE_RPC_OVERRIDE(m2sClientRequestedArrangedConnection, (U32 requestId, Vector<IPAddress> possibleAddresses,
      ByteBufferPtr connectionParameters));

#ifndef ZAP_DEDICATED
   TNL_DECLARE_RPC_OVERRIDE(m2cArrangedConnectionAccepted, 
               (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData));
   TNL_DECLARE_RPC_OVERRIDE(m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData));

   TNL_DECLARE_RPC_OVERRIDE(m2cSetAuthenticated_019, (RangedU32<0, AuthenticationStatusCount> authStatus, 
                                                     Int<BADGE_COUNT> badges, U16 gamesPlayed, StringPtr correctedName));

   TNL_DECLARE_RPC_OVERRIDE(m2cSetMOTD, (StringPtr masterName, StringPtr motdString));
   TNL_DECLARE_RPC_OVERRIDE(m2cSendUpdgradeStatus, (bool needToUpgrade));

   // Incoming out-of-game chat message from master
   TNL_DECLARE_RPC_OVERRIDE(m2cSendChat, (StringTableEntry clientName, bool isPrivate, StringPtr message));      
   
   // For managing list of players in global chat
   TNL_DECLARE_RPC_OVERRIDE(m2cPlayerJoinedGlobalChat, (StringTableEntry playerNick));
   TNL_DECLARE_RPC_OVERRIDE(m2cPlayerLeftGlobalChat, (StringTableEntry playerNick));
   TNL_DECLARE_RPC_OVERRIDE(m2cPlayersInGlobalChat, (Vector<StringTableEntry> playerNicks));
   TNL_DECLARE_RPC_OVERRIDE(m2cSendHighScores, (Vector<StringTableEntry> groupNames, Vector<string> names, Vector<string> scores));
   TNL_DECLARE_RPC_OVERRIDE(m2cSendPlayerLevelRating,  (U32 databaseId, RangedU32<0, 2> rating));
   TNL_DECLARE_RPC_OVERRIDE(m2cSendTotalLevelRating, (U32 databaseId, S16 rating));

   // Some helpers
   void sendTotalLevelRating(S32 databaseId, S16 rating);
   void sendPlayerLevelRating(S32 databaseId, S32 rating);
#endif

   void requestAuthentication(StringTableEntry mClientName, Nonce mClientId);

   TNL_DECLARE_RPC_OVERRIDE(m2sSetAuthenticated_019, (Vector<U8> id, StringTableEntry name, 
                                                      RangedU32<0,AuthenticationStatusCount> status, Int<BADGE_COUNT> badges,
                                                      U16 gamesPlayed));
   void writeConnectRequest(BitStream *bstream);
   bool readConnectAccept(BitStream *stream, NetConnection::TerminationReason &reason);

   virtual void onConnectionEstablished();
   void onConnectionTerminated(TerminationReason r, const char *string); // An existing connection has been terminated
   void onConnectTerminated(TerminationReason r, const char *string);    // A still-being-established connection has been terminated

   TNL_DECLARE_NETCONNECTION(MasterServerConnection);
};


typedef void (MasterServerConnection::*MasterConnectionCallback)();

class AnonymousMasterServerConnection : public MasterServerConnection
{
   typedef MasterServerConnection Parent;

private:
   MasterConnectionCallback mConnectionCallback;

public:
   explicit AnonymousMasterServerConnection(Game *game);    // Constructor
   virtual ~AnonymousMasterServerConnection();

   void setConnectionCallback(MasterConnectionCallback callback);
   MasterConnectionCallback getConnectionCallback();

   void onConnectionEstablished();
};

};

#endif

