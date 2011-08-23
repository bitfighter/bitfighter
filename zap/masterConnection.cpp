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
//#include "UIQueryServers.h"
#include "gameConnection.h"
#include "gameNetInterface.h"
#include "gameObject.h"
#include "version.h"
#include "config.h"
#include "game.h"

#ifndef ZAP_DEDICATED
#include "Joystick.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "UIChat.h"
#include "UIErrorMessage.h"
#include "ClientGame.h"
#endif

#include "SharedConstants.h"    // For AuthenticationStatus enum

#include "tnl.h"
#include "../tnl/tnlNetInterface.h"

namespace Zap
{

extern string gPlayerPassword;

TNL_IMPLEMENT_NETCONNECTION(MasterServerConnection, NetClassGroupMaster, false);


// Constructor
MasterServerConnection::MasterServerConnection(Game *game)   
{
   mGame = game;

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

#ifndef ZAP_DEDICATED
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cQueryServersResponse, (U32 queryId, Vector<IPAddress> ipList))
{
   // Only process results from current query, ignoring anything older...
   if(queryId != mCurrentQueryId || mGame->isServer())
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
      dynamic_cast<ClientGame *>(mGame)->gotServerListFromMaster(mServerList);

      mServerList.clear();
   }
}
#endif


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
   if(!mGame->isServer())   // We're not a server!  Reject connection!
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
   if(mGame->getNetInterface()->isAddressBanned(fullPossibleAddresses[0]))
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


#ifndef ZAP_DEDICATED
extern ClientInfo gClientInfo;

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cArrangedConnectionAccepted, 
                           (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData))
{
   if(!mGame->isServer() && requestId == mCurrentQueryId && 
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
      dynamic_cast<ClientGame *>(mGame)->setConnectionToServer(gameConnection);

      gameConnection->connectArranged(getInterface(), fullPossibleAddresses, nonce, serverNonce, theSharedData, true);
   }
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData))
{
   if(!mGame->isServer() && requestId == mCurrentQueryId)
   {
      logprintf(LogConsumer::LogConnection, "Remote host rejected arranged connection...");    // Maybe player was kicked/banned?
      dynamic_cast<ClientGame *>(mGame)->connectionToServerRejected();
   }
}

// Display the MOTD that is set by the master server
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cSetMOTD, (StringPtr masterName, StringPtr motdString))
{
   if(mGame->isServer())
      return;

   setMasterName(masterName.getString());
   dynamic_cast<ClientGame *>(mGame)->setMOTD(motdString.getString());
}


// The master server has looked at our name and password, and determined if we're in the database properly.  Here's it's reply:
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cSetAuthenticated, 
                                    (RangedU32<0, AuthenticationStatusCount> authStatus, StringPtr correctedName))
{
   if(mGame->isServer())
      return;

   if((AuthenticationStatus)authStatus.value == AuthenticationStatusAuthenticatedName)
   {
      // Hmmm.... same info in two places...
      gClientInfo.name = correctedName.getString();
      gIniSettings.name = correctedName.getString();  

      gClientInfo.authenticated = true;
      GameConnection *gc = dynamic_cast<ClientGame *>(mGame)->getConnectionToServer();
      if(gc)
         gc->c2sSetAuthenticated();
   }
   else 
      gClientInfo.authenticated = false;
}
#endif


// Now we know that player with specified id has an approved name
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2sSetAuthenticated, (Vector<U8> id, StringTableEntry name,
         RangedU32<0,AuthenticationStatusCount> status))
{
   if(!mGame->isServer())
      return;

   Nonce clientId(id);     // Reconstitute our id into a nonce

   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
      if(walk->getClientId()->isValid() && *walk->getClientId() == clientId)  // Robots don't have valid clientID
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


#ifndef ZAP_DEDICATED
// Alert user to the fact that their client is (or is not) out of date
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cSendUpdgradeStatus, (bool needToUpgrade))
{
   if(mGame->isServer())
      return;

   dynamic_cast<ClientGame *>(mGame)->setNeedToUpgrade(needToUpgrade);
}


// Handle incoming chat message
// Runs on client only (but initiated by master)
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cSendChat, (StringTableEntry playerNick, bool isPrivate, StringPtr message))
{
   if(mGame->isServer())
      return;

   dynamic_cast<ClientGame *>(mGame)->gotChatMessage(playerNick.getString(), message.getString(), isPrivate, false);
}


// Set the list of players in chat, typically called when player joins chatroom and needs to know who's there
// Runs on client only (but initiated by master)
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cPlayersInGlobalChat, (Vector<StringTableEntry> playerNicks))
{
   if(mGame->isServer())
      return;

   dynamic_cast<ClientGame *>(mGame)->setPlayersInGlobalChat(playerNicks);
}


// Handle players joining or leaving chat session
// Runs on client only (but initiated by master)
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cPlayerJoinedGlobalChat, (StringTableEntry playerNick))
{
   if(mGame->isServer())
      return;

   dynamic_cast<ClientGame *>(mGame)->playerJoinedGlobalChat(playerNick);
}


// Handle players joining or leaving chat session
// Runs on client only (but initiated by master)
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cPlayerLeftGlobalChat, (StringTableEntry playerNick))
{
   if(mGame->isServer())
      return;

   dynamic_cast<ClientGame *>(mGame)->playerLeftGlobalChat(playerNick);
}
#endif


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

#ifdef ZAP_DEDICATED
	bstream->writeString("");  // empty controller string for dedicated.
#else
   // First controller's autodetect string (for research purposes!)
   bstream->writeString(Joystick::DetectedJoystickNameList.size() > 0 ? Joystick::DetectedJoystickNameList[0] : "");
#endif


   if(bstream->writeFlag(mGame->isServer()))     // We're a server, tell the master a little about us
   {
      ServerGame *serverGame = dynamic_cast<ServerGame *>(mGame);

      bstream->write((U32) 1000);                             // CPU speed  (dummy)
      bstream->write((U32) 0xFFFFFFFF);                       // region code (dummy) --> want to use this?
      bstream->write((U32) serverGame->getRobotCount());      // number of bots
      bstream->write((U32) serverGame->getPlayerCount());     // num players       --> will always be 0 or 1?
      bstream->write((U32) serverGame->getMaxPlayers());      // max players
      bstream->write((U32) serverGame->mInfoFlags);           // info flags (1=>test host, i.e. from editor)

      bstream->writeString(serverGame->getCurrentLevelName().getString());      // Level name
      bstream->writeString(serverGame->getCurrentLevelType().getString());      // Level type

      bstream->writeString(serverGame->getHostName());        // Server name
      bstream->writeString(serverGame->getHostDescr());       // Server description
   }
   else     // We're a client
   {
#ifndef ZAP_DEDICATED
      bstream->writeString(gClientInfo.name.c_str());   // User's nickname
      bstream->writeString(gPlayerPassword.c_str());    // and whatever password they supplied
      gClientInfo.id.write(bstream);
#endif
   }
}

extern CmdLineSettings gCmdLineSettings;

void MasterServerConnection::onConnectionEstablished()
{
   if(mGame->isServer())        // Might want ServerFilter ?
      logprintf(LogConsumer::ServerFilter, "Server established connection with Master Server");
   else
      logprintf(LogConsumer::LogConnection, "Client established connection with Master Server");

   if(gCmdLineSettings.masterAddress == "" && gMasterAddress.size() >= 2)
   {
      // If there is 2 or more master address, the first address is the one used to connect..
      string string1;
      for(S32 i = 0; i < gMasterAddress.size() - 1; i++)
      {
         string1 = string1 + gMasterAddress[i] + ",";
      }
      string1 += gMasterAddress[gMasterAddress.size()-1];
      gIniSettings.masterAddress = string1;
   }
}


// A still-being-established connection has been terminated
void MasterServerConnection::onConnectTerminated(TerminationReason reason, const char *reasonStr)   
{
   //gClientGame->onConnectionTerminated(getNetAddress(), reason, reasonStr);
	// Don't want to get Connection Terminated message interrupting the game,
	// Also, this may be called from dedicated server's connection to master which do not have gClientGame..

   // TODO: put something here or keep empty?
}


void MasterServerConnection::requestAuthentication(StringTableEntry clientName, Nonce clientId)
{
   s2mRequestAuthentication(clientId.toVector(), clientName);
}

};


