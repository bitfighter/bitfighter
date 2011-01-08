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
#include "UIErrorMessage.h"
#include "gameConnection.h"
#include "gameNetInterface.h"
#include "gameObject.h"

#include "SharedConstants.h"    // For AuthenticationStatus enum

#include "../tnl/tnl.h"
#include "../tnl/tnlNetInterface.h"

namespace Zap
{

extern bool gQuit;
extern string gPlayerPassword;

TNL_IMPLEMENT_NETCONNECTION(MasterServerConnection, NetClassGroupMaster, false);


// Constructor
MasterServerConnection::MasterServerConnection(bool isGameServer, U32 infoFlags)   
{
   mIsGameServer = isGameServer;
   mInfoFlags = infoFlags;
   mCurrentQueryId = 0;
   setIsConnectionToServer();
   setIsAdaptive();
}


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


TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2sClientRequestedArrangedConnection, 
                              (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionParameters))
{
   if(!mIsGameServer)   // We're not a server!
   {
      logprintf(LogConsumer::LogConnection, "Rejecting arranged connection from %s", Address(possibleAddresses[0]).toString());
      s2mRejectArrangedConnection(requestId, connectionParameters);
      return;
   }

   // From here on, we're running on a game server that the master is trying to arrange a connection with

   Vector<Address> fullPossibleAddresses;
   for(S32 i = 0; i < possibleAddresses.size(); i++)
      fullPossibleAddresses.push_back(Address(possibleAddresses[i]));

   // Check if the requestor is banned on this server
   if(gServerGame->getNetInterface()->isAddressBanned(fullPossibleAddresses[0]))
   {
      logprintf(LogConsumer::LogConnection, "Blocking connection from banned address %s", fullPossibleAddresses[0].toString());
      s2mRejectArrangedConnection(requestId, connectionParameters);
      return;
   }

   // Ok, let's do the arranged connection!
   U8 data[Nonce::NonceSize * 2 + SymmetricCipher::KeySize * 2];

   TNL::Random::read(data, sizeof(data));
   IPAddress localAddress = getInterface()->getFirstBoundInterfaceAddress().toIPAddress();
   ByteBufferPtr b = new ByteBuffer(data, sizeof(data));
   b->takeOwnership();

   // Let the server know we're accepting the connection, and pass back our buffer of random data (b)
   s2mAcceptArrangedConnection(requestId, localAddress, b);    
   GameConnection *conn = new GameConnection();

   conn->setNetAddress(fullPossibleAddresses[0]);

   logprintf(LogConsumer::LogConnection, "Accepting arranged connection from %s", Address(fullPossibleAddresses[0]).toString());

   ByteBufferPtr theSharedData = new ByteBuffer(data + 2 * Nonce::NonceSize, sizeof(data) - 2 * Nonce::NonceSize);
   Nonce nonce(data);
   Nonce serverNonce(data + Nonce::NonceSize);
   theSharedData->takeOwnership();

   conn->connectArranged(getInterface(), fullPossibleAddresses, nonce, serverNonce, theSharedData, false);
}

         // TODO: Delete after 014 -- replaced with identical m2sClientRequestedArrangedConnection above
         TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cClientRequestedArrangedConnection, 
                                    (U32 requestId, Vector<IPAddress> possibleAddresses,
                                     ByteBufferPtr connectionParameters))
         {
            if(!mIsGameServer)   // We're not a server!
            {
               logprintf(LogConsumer::LogConnection, "Rejecting arranged connection from %s", Address(possibleAddresses[0]).toString());
               s2mRejectArrangedConnection(requestId, connectionParameters);
               return;
            }

            // Server only from here on down

            Vector<Address> fullPossibleAddresses;
            for(S32 i = 0; i < possibleAddresses.size(); i++)
               fullPossibleAddresses.push_back(Address(possibleAddresses[i]));

            // Check if the specified host is banned on this server
            if(gServerGame->getNetInterface()->isAddressBanned(fullPossibleAddresses[0]))
            {
               logprintf(LogConsumer::LogConnection, "Blocking connection from banned address %s", fullPossibleAddresses[0].toString());
               s2mRejectArrangedConnection(requestId, connectionParameters);
               return;
            }

            // Ok, let's do the arranged connection!
            U8 data[Nonce::NonceSize * 2 + SymmetricCipher::KeySize * 2];

            TNL::Random::read(data, sizeof(data));
            IPAddress localAddress = getInterface()->getFirstBoundInterfaceAddress().toIPAddress();
            ByteBufferPtr b = new ByteBuffer(data, sizeof(data));
            b->takeOwnership();

            s2mAcceptArrangedConnection(requestId, localAddress, b);
            GameConnection *conn = new GameConnection();

            conn->setNetAddress(fullPossibleAddresses[0]);

            logprintf(LogConsumer::LogConnection, "Accepting arranged connection from %s", 
                                                  Address(fullPossibleAddresses[0]).toString());

            ByteBufferPtr theSharedData = 
                        new ByteBuffer(data + 2 * Nonce::NonceSize, sizeof(data) - 2 * Nonce::NonceSize);
            Nonce nonce(data);
            Nonce serverNonce(data + Nonce::NonceSize);
            theSharedData->takeOwnership();

            conn->connectArranged(getInterface(), fullPossibleAddresses, nonce, serverNonce, theSharedData, false);
         }


extern ClientInfo gClientInfo;

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cArrangedConnectionAccepted, 
                           (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData))
{
   if(!mIsGameServer && requestId == mCurrentQueryId && 
                  connectionData->getBufferSize() >= Nonce::NonceSize * 2 + SymmetricCipher::KeySize * 2)
   {
      logprintf(LogConsumer::LogConnection, "Remote host accepted arranged connection.");

      Vector<Address> fullPossibleAddresses;
      for(S32 i = 0; i < possibleAddresses.size(); i++)
         fullPossibleAddresses.push_back(Address(possibleAddresses[i]));

      ByteBufferPtr theSharedData =
                     new ByteBuffer(
                           (U8 *) connectionData->getBuffer() + Nonce::NonceSize * 2,
                           connectionData->getBufferSize() - Nonce::NonceSize * 2 );

      Nonce nonce(connectionData->getBuffer());
      Nonce serverNonce(connectionData->getBuffer() + Nonce::NonceSize);

      // Client is creating new connection to game server
      GameConnection *gameConnection = new GameConnection(gClientInfo);
      gClientGame->setConnectionToServer(gameConnection);

      gameConnection->connectArranged(getInterface(), fullPossibleAddresses, nonce, serverNonce, theSharedData, true);
   }
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData))
{
   if(!mIsGameServer && requestId == mCurrentQueryId)
   {
      logprintf(LogConsumer::LogConnection, "Remote host rejected arranged connection...");    // Maybe player was kicked/banned?
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


// The master server has looked at our name and password, and determined if we're in the database properly.  Here's it's reply:
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cSetAuthenticated, 
                                    (RangedU32<0, AuthenticationStatusCount> authStatus, StringPtr correctedName))
{
   if((AuthenticationStatus)authStatus.value == AuthenticationStatusAuthenticatedName)
   {
      // Hmmm.... same info in two places...
      gClientInfo.name = correctedName.getString();
      gIniSettings.name = correctedName.getString();

      gClientInfo.authenticated = true;
      if(gClientGame->getConnectionToServer())
         gClientGame->getConnectionToServer()->c2sSetAuthenticated();
   }
   else 
      gClientInfo.authenticated = false;       
}


//  Tell all clients name is changed, and update server side name
// Game Server only
void updateClientChangedName(GameConnection *gc, StringTableEntry newName){
	GameType *gt = gServerGame->getGameType();
	ClientRef *cr = gc->getClientRef();
	logprintf(LogConsumer::LogConnection, "Name changed from %s to %s",gc->getClientName().getString(),newName.getString());
	if(gt)
	{
		gt->s2cRenameClient(gc->getClientName(), newName);
	}
	gc->setClientName(newName);
	cr->name = newName;
	Ship *ship = dynamic_cast<Ship *>(gc->getControlObject());
	if(ship)
	{
		ship->setName(newName);
		ship->setMaskBits(Ship::AuthenticationMask);  //ship names will update with this bit
	}
}

// Now we know that player with specified id has an approved name
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2sSetAuthenticated, (Vector<U8> id, StringTableEntry name,
         RangedU32<0,AuthenticationStatusCount> status))
{
   Nonce clientId(id);     // Reconstitute our id into a nonce

   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
      if(*walk->getClientId() == clientId)
      {
         if(status == AuthenticationStatusAuthenticatedName)
         {
            walk->setAuthenticated(true);
            //auto-rename other non-Authenticated clients to avoid stealing the authenticated name.
            for(GameConnection *walk2 = GameConnection::getClientList(); walk2; walk2 = walk2->getNextClient())
            {
               if(walk2->getClientName() == name && !walk2->isAuthenticated() )
					{
                  //walk2->setClientName(GameConnection::makeUnique(walk2->getClientName().getString()).c_str());
						updateClientChangedName(walk2, GameConnection::makeUnique(walk2->getClientName().getString()).c_str());
                          //makeUnique will think the name is in use by self, and rename it.

					}
            }
				StringTableEntry oldName = walk->getClientName();
				walk->setClientName(StringTableEntry(""));       //avoid unique self
				StringTableEntry uniqueName = GameConnection::makeUnique(name.getString()).c_str();  //new name
				walk->setClientName(oldName);                   //restore name to properly get it updated to clients.
            if(walk->getClientName() != uniqueName)
					updateClientChangedName(walk, uniqueName);
         }
         else if(status == AuthenticationStatusUnauthenticatedName)
         {  // braces needed
            if(walk->getAuthenticationCounter() > 1)     // Client gets two bites at the apple, to cover rare race condition
               walk->setAuthenticated(false);
            else
               walk->resetAuthenticationTimer();
         }
         else if(status == AuthenticationStatusTryAgainLater)
            walk->resetAuthenticationTimer();

         break;
      }
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
      gChatInterface.newMessage(playerNick.getString(), message.getString(), isPrivate, false);
}


// Handle players joining or leaving chat session
// Runs on client only (but initiated by master)
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cPlayerJoinedGlobalChat, (StringTableEntry playerNick))
{
   gChatInterface.playerJoinedGlobalChat(playerNick);
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
   gChatInterface.playerLeftGlobalChat(playerNick);
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

   bstream->writeString("+");                // Just a dummy string to indicate we'll be using a different protocol than the original, 
                                             // version will be specified
   bstream->write(MASTER_PROTOCOL_VERSION);  // Version of the protocol we'll be using to communicate with the master
   bstream->write(CS_PROTOCOL_VERSION);      // Version of the Client-Server protocol we use (can only play with others using same version)
   bstream->write(BUILD_VERSION);            // Current build of this game
   bstream->writeString(gJoystickName);      // Controller's autodetect string (for research purposes!)


   if(bstream->writeFlag(mIsGameServer))     // We're a server, tell the master a little about us
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
   {
      bstream->writeString(gClientInfo.name.c_str());   // User's nickname
      bstream->writeString(gPlayerPassword.c_str());    // and whatever password they supplied
      gClientInfo.id.write(bstream);  
   }
}


void MasterServerConnection::onConnectionEstablished()
{
   logprintf(LogConsumer::LogConnection, "%s established connection with Master Server", mIsGameServer ? "Server" : "Client");
}


// A still-being-established connection has been terminated
void MasterServerConnection::onConnectTerminated(TerminationReason reason, const char *reasonStr)   
{
   switch(reason)
   {
      case NetConnection::ReasonDuplicateId:
         gErrorMsgUserInterface.setMessage(2, "Your connection was rejected by the server");
         gErrorMsgUserInterface.setMessage(3, "because you sent a duplicate player id. Player ids are");
         gErrorMsgUserInterface.setMessage(4, "generated randomly, and collisions are extremely rare.");
         gErrorMsgUserInterface.setMessage(5, "Please restart Bitfighter and try again.  Statistically");
         gErrorMsgUserInterface.setMessage(6, "speaking, you should never see this message again!");
         gErrorMsgUserInterface.activate();

         if(gClientGame->getConnectionToServer())
            gClientGame->setReadyToConnectToMaster(false);  //New ID might cause Authentication (underline name) problems if connected to game server...
         else
            gClientInfo.id.getRandom();                     //get another ID, if not connected to game server
         break;

      case NetConnection::ReasonBadLogin:
         gErrorMsgUserInterface.setMessage(2, "Unable to log you in with the username/password you");
         gErrorMsgUserInterface.setMessage(3, "provided. If you have an account, please verify your");
         gErrorMsgUserInterface.setMessage(4, "password. Otherwise, you chose a reserved name; please");
         gErrorMsgUserInterface.setMessage(5, "try another.");
         gErrorMsgUserInterface.setMessage(7, "Please check your credentials and try again.");

         gNameEntryUserInterface.activate();
         gErrorMsgUserInterface.activate();
         break;

      case NetConnection::ReasonInvalidUsername:
         gErrorMsgUserInterface.setMessage(2, "Your connection was rejected by the server because");
         gErrorMsgUserInterface.setMessage(3, "you sent an username that contained illegal characters.");
         gErrorMsgUserInterface.setMessage(5, "Please try a different name.");

         gNameEntryUserInterface.activate();
         gErrorMsgUserInterface.activate();
         break;

      case NetConnection::ReasonError:
         gErrorMsgUserInterface.setMessage(2, "Unable to connect to the server.  Recieved message:");
         gErrorMsgUserInterface.setMessage(3, reasonStr);
         gErrorMsgUserInterface.setMessage(5, "Please try a different game server, or try again later.");
         gErrorMsgUserInterface.activate();
         break;
   }
}


void MasterServerConnection::requestAuthentication(StringTableEntry clientName, Nonce clientId)
{
   s2mRequestAuthentication(clientId.toVector(), clientName);
}

};


