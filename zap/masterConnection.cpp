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

#include "masterConnection.h"
#include "UIQueryServers.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "UIChat.h"
#include "gameConnection.h"
#include "gameNetInterface.h"
#include "gameObject.h"

#include "../tnl/tnl.h"
#include "../tnl/tnlNetInterface.h"

namespace Zap
{

extern U32 gSimulatedLag;
extern F32 gSimulatedPacketLoss;
extern bool gQuit;

TNL_IMPLEMENT_NETCONNECTION(MasterServerConnection, NetClassGroupMaster, false);


// Try this...
void MasterServerConnection::startServerQuery()
{
   // Invalidate old queries
   mCurrentQueryId++;

   // And automatically do a server query as well - you may not want to do things
   // in this order in your own clients.
   c2mQueryServers(mCurrentQueryId, 0xFFFFFFFF, 0, 128, 0, 128, 0, "", "");
   //             (queryId, regionMask, minPlayers, maxPlayers, infoFlags, maxBots, minCPUSpeed, gameType, missionType);

}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cQueryServersResponse, (U32 queryId, Vector<IPAddress> ipList))
{
   // Only process results from current query, ignoring anything older...
   if(queryId != mCurrentQueryId)
      return;

   // The master server sends out an empty list to signify and "end of transmission".  We'll build up a
   // list of servers here until we get that final EOT list, and then send the entire list to the UI.
   // We need to do this because the UI will look for servers that it knows about that are not on the master 
   // list, and will remove them from the display.  If we send the list in parts, the UI will remove any known servers that
   // don't happen to be on that part.
   if(ipList.size() > 0)
   {
      // Store these intermediate results
      for(S32 i = 0; i < ipList.size(); i++)
         mServerList.push_back(ipList[i]);
   }
   else  // Empty list recieved, transmission complete, send whole list on to the UI
   {
      gQueryServersUserInterface.mRecievedListOfServersFromMaster = true;
      gQueryServersUserInterface.addPingServers(mServerList);
      mServerList.clear();
   }
}


void MasterServerConnection::requestArrangedConnection(const Address &remoteAddress)
{
   mCurrentQueryId++;

   c2mRequestArrangedConnection(mCurrentQueryId, remoteAddress.toIPAddress(),
      getInterface()->getFirstBoundInterfaceAddress().toIPAddress(),
      new ByteBuffer((U8 *) "ZAP!", 5));
}


TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cClientRequestedArrangedConnection, (U32 requestId, Vector<IPAddress> possibleAddresses,
   ByteBufferPtr connectionParameters))
{
   if(!mIsGameServer)   // We're not a server!
   {
      TNL::logprintf("Rejecting arranged connection from %s", Address(possibleAddresses[0]).toString());
      c2mRejectArrangedConnection(requestId, connectionParameters);
      return;
   }

   Vector<Address> fullPossibleAddresses;
   for(S32 i = 0; i < possibleAddresses.size(); i++)
      fullPossibleAddresses.push_back(Address(possibleAddresses[i]));

   // Check if the specified host is banned on this server
   if(gServerGame->getNetInterface()->isHostBanned(fullPossibleAddresses[0]))
   {
      TNL::logprintf("Blocking connection from banned address %s", fullPossibleAddresses[0].toString());
      c2mRejectArrangedConnection(requestId, connectionParameters);
      return;
   }

   // Ok, let's do the arranged connection!
   U8 data[Nonce::NonceSize * 2 + SymmetricCipher::KeySize * 2];

   TNL::Random::read(data, sizeof(data));
   IPAddress localAddress = getInterface()->getFirstBoundInterfaceAddress().toIPAddress();
   ByteBufferPtr b = new ByteBuffer(data, sizeof(data));
   b->takeOwnership();

   c2mAcceptArrangedConnection(requestId, localAddress, b);
   GameConnection *conn = new GameConnection();

   conn->setNetAddress(fullPossibleAddresses[0]);

   TNL::logprintf("Accepting arranged connection from %s", Address(fullPossibleAddresses[0]).toString());
   TNL::logprintf("  Generated shared secret data: %s", b->encodeBase64()->getBuffer());

   ByteBufferPtr theSharedData = new ByteBuffer(data + 2 * Nonce::NonceSize, sizeof(data) - 2 * Nonce::NonceSize);
   Nonce nonce(data);
   Nonce serverNonce(data + Nonce::NonceSize);
   theSharedData->takeOwnership();

   conn->connectArranged(getInterface(), fullPossibleAddresses,
      nonce, serverNonce, theSharedData,false);
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cArrangedConnectionAccepted, (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData))
{
   if(!mIsGameServer && requestId == mCurrentQueryId && connectionData->getBufferSize() >= Nonce::NonceSize * 2 + SymmetricCipher::KeySize * 2)
   {
      TNL::logprintf("Remote host accepted arranged connection.");
      TNL::logprintf("  Shared secret data: %s", connectionData->encodeBase64()->getBuffer());

      Vector<Address> fullPossibleAddresses;
      for(S32 i = 0; i < possibleAddresses.size(); i++)
         fullPossibleAddresses.push_back(Address(possibleAddresses[i]));

      ByteBufferPtr theSharedData =
                     new ByteBuffer(
                           (U8 *) connectionData->getBuffer() + Nonce::NonceSize * 2,
                           connectionData->getBufferSize() - Nonce::NonceSize * 2
                     );

      Nonce nonce(connectionData->getBuffer());
      Nonce serverNonce(connectionData->getBuffer() + Nonce::NonceSize);

      GameConnection *conn = new GameConnection();
      const char *name = gNameEntryUserInterface.getText();
      if(!name[0])
         name = "Chump";

      conn->setSimulatedNetParams(gSimulatedPacketLoss, gSimulatedLag);
      conn->setClientName(name);
      gClientGame->setConnectionToServer(conn);

      conn->connectArranged(getInterface(), fullPossibleAddresses,
         nonce, serverNonce, theSharedData,true);
   }
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData))
{
   if(!mIsGameServer && requestId == mCurrentQueryId)
   {
      TNL::logprintf("Remote host rejected arranged connection...");       // Perhaps because the player was kicked/banned?
      onConnectionTerminated(ReasonTimedOut, "Connection timed out");
      endGame();
      gMainMenuUserInterface.activate();
   } 
}

// Display the MOTD that is set by the master server
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cSetMOTD, (StringPtr masterName, StringPtr motdString))
{
   setMasterName(masterName.getString());
   gMainMenuUserInterface.setMOTD(motdString); 
}

// Alert user to the fact that their client is (or is not) out of date
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cSendUpdgradeStatus, (bool needToUpgrade))
{
   gMainMenuUserInterface.setNeedToUpgrade(needToUpgrade);
}


// Handle incoming chat message
// Runs on client only (but initiated by master)
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cSendChat, (StringTableEntry playerNick, bool isPrivate, StringPtr message))
{
   //if(!mIsGameServer)
      gChatInterface.newMessage(playerNick.getString(), message.getString(), isPrivate);
}


// Handle players joining or leaving chat session
// Runs on client only (but initiated by master)
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cPlayerJoinedGlobalChat, (StringTableEntry playerNick))
{
   gChatInterface.mPlayersInGlobalChat.push_back(playerNick);
}


// Handle players joining or leaving chat session
// Runs on client only (but initiated by master)
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cPlayersInGlobalChat, (Vector<StringTableEntry> playerNicks))
{
   gChatInterface.mPlayersInGlobalChat.clear();

   for(S32 i = 0; i < playerNicks.size(); i++)
      gChatInterface.mPlayersInGlobalChat.push_back(playerNicks[i]);
}


// Handle players joining or leaving chat session
// Runs on client only (but initiated by master)
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cPlayerLeftGlobalChat, (StringTableEntry playerNick))
{
   for(S32 i = 0; i < gChatInterface.mPlayersInGlobalChat.size(); i++)
      if(gChatInterface.mPlayersInGlobalChat[i] == playerNick)
      {
         gChatInterface.mPlayersInGlobalChat.erase_fast(i);
         break;
      }
}


// Set master server name
void MasterServerConnection::setMasterName(string name)
{
   mMasterName = name;
}

// Retrieve master server name
string MasterServerConnection::getMasterName()
{
   return mMasterName;
}

extern char gJoystickName[260];     // 260 should be MAX_PATH from windef.h...

// Send a connection request to the master server.  Also, tell them a little about ourselves.
// Note that most of these parameters are completely bogus...  but even if they're not currently used, we gotta send something.
void MasterServerConnection::writeConnectRequest(BitStream *bstream)
{
   Parent::writeConnectRequest(bstream);

   bstream->writeString("+");                // Just a dummy string to indicate we'll be using a different protocol than the original, version will be specified
   bstream->write(MASTER_PROTOCOL_VERSION);  // Version of the protocol we'll be using to communicate with the master
   bstream->write(CS_PROTOCOL_VERSION);      // Version of the Client-Server protocol we use (can only play with others using same version)
   bstream->write(BUILD_VERSION);            // Current build of this game
   bstream->writeString(gJoystickName);      // Controller's autodetect string (for research purposes!)


   if(bstream->writeFlag(mIsGameServer))     // We're a server
   {
      bstream->write((U32) 1000);                              // CPU speed  (dummy)
      bstream->write((U32) 0xFFFFFFFF);                        // region code (dummy) --> want to use this?
      bstream->write((U32) gServerGame->getRobotCount());      // number of bots
      bstream->write((U32) gServerGame->getPlayerCount());     // num players       --> will always be 0 or 1?
      bstream->write((U32) gServerGame->getMaxPlayers());      // max players
      bstream->write((U32) mInfoFlags);                        // info flags (1=>test host, i.e. from editor)

      bstream->writeString(gServerGame->getCurrentLevelName().getString());      // Level name
      bstream->writeString(gServerGame->getCurrentLevelType().getString());      // Level type

      bstream->writeString(gServerGame->getHostName());        // Server name
      bstream->writeString(gServerGame->getHostDescr());       // Server description
   }
   else     // We're a client
      bstream->writeString(gNameEntryUserInterface.getText());  // User's nickname
}

void MasterServerConnection::onConnectionEstablished()
{
   TNL::logprintf("%s established connection with Master Server", mIsGameServer ? "Server" : "Client");
}

};


