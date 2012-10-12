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

#include "gameConnection.h"
//#include "game.h"
#include "ServerGame.h"
#include "soccerGame.h"          // For checking if pick up soccer is allowed
#include "IniFile.h"             // For CIniFile def
#include "playerInfo.h"
#include "shipItems.h"           // For EngineerBuildObjects enum
#include "masterConnection.h"    // For MasterServerConnection def
#include "EngineeredItem.h"      // For EngineerModuleDeployer
#include "BanList.h"
#include "gameNetInterface.h"
#include "ClientInfo.h"

#ifndef ZAP_DEDICATED
#   include "ClientGame.h"
#endif

#include "md5wrapper.h"

#include "Colors.h"
#include "stringUtils.h"         // For strictjoindir()


namespace Zap
{

TNL_IMPLEMENT_NETCONNECTION(GameConnection, NetClassGroupGame, true);

const U8 GameConnection::CONNECT_VERSION = 0;  // GameConnection's version, for possible future use with changes on compatible versions

// Constructor -- used on Server by TNL, not called directly, used when a new client connects to the server
GameConnection::GameConnection()
{
   initialize();

   mSettings = NULL; // mServerGame->getSettings();      // will be set on ReadConnectRequest

   mVote = 0;
   mVoteTime = 0;

   // Might be a tad more efficient to put this in the initializer, but the (legitimate, in this case) use of this
   // in the arguments makes VC++ nervous, which in turn makes me nervous.
   mClientInfo = NULL;    /// FullClientInfo created when we know what the ServerGame is, in readConnectRequest

#ifndef ZAP_DEDICATED
   mClientGame = NULL;
#endif

}


#ifndef ZAP_DEDICATED
// Constructor on client side
GameConnection::GameConnection(ClientGame *clientGame)
{
   initialize();

   mSettings = clientGame->getSettings();
   mClientInfo = clientGame->getClientInfo();      // Now have a FullClientInfo representing the local player

   TNLAssert(mClientInfo->getName() != "", "Client has invalid name!");
   if(mClientInfo->getName() == "")
      mClientInfo->setName("Chump");


   setSimulatedNetParams(mSettings->getSimulatedLoss(), mSettings->getSimulatedLag());
}
#endif


void GameConnection::initialize()
{
   mServerGame = NULL;
   setTranslatesStrings();
   mInCommanderMap = false;
   mGotPermissionsReply = false;
   mWaitingForPermissionsReply = false;
   mSwitchTimer.reset(0);

   mAcheivedConnection = false;
   mLastEnteredPassword = "";

   // Things related to verification
   mAuthenticationCounter = 0;            // Counts number of retries

   switchedTeamCount = 0;
   mSendableFlags = 0;
   mDataBuffer = NULL;

   mWrongPasswordCount = 0;

   mVoiceChatEnabled = true;

   reset();
}

// Destructor
GameConnection::~GameConnection()
{
   // Log the disconnect...
   if(mClientInfo && !mClientInfo->isRobot())        // Avoid cluttering log with useless messages like "IP:any:0 - client quickbot disconnected."
      logprintf(LogConsumer::LogConnection, "%s - client \"%s\" disconnected.", getNetAddressString(), mClientInfo->getName().getString());

   if(isConnectionToClient())    // Only true if we're the server
   {
      if(mServerGame->getSuspendor() == this)     
         mServerGame->suspenderLeftGame();

      if(mAcheivedConnection)         
      {
        // Compute time we were connected
        time_t quitTime;
        time(&quitTime);

        double elapsed = difftime (quitTime, joinTime);

        logprintf(LogConsumer::ServerFilter, "%s [%s] quit [%s] (%.2lf secs)", mClientInfo->getName().getString(), 
                                                  isLocalConnection() ? "Local Connection" : getNetAddressString(), 
                                                  getTimeStamp().c_str(), elapsed);
      }
   }

   delete mDataBuffer;

}


#ifndef ZAP_DEDICATED
ClientGame *GameConnection::getClientGame()
{
   return mClientGame;
}


void GameConnection::setClientGame(ClientGame *game)
{
   mClientGame = game;
}
#endif


// Clears/initializes some things between levels
void GameConnection::reset()
{  
   mReadyForRegularGhosts = false;
   mWantsScoreboardUpdates = false;
}


const char *GameConnection::getConnectionStateString(S32 i)
{
   static const char *connectStatesTable[] = {
      "Not connected...",
      "Sending challenge request...",
      "Punching through firewalls...",
      "Computing puzzle solution...",
      "Sent connect request...",
      "Connection timed out",
      "Connection rejected",
      "Connected",
      "Disconnected",
      "Connection timed out",
   };

   TNLAssert(i < S32(ARRAYSIZE(connectStatesTable)), "Invalid index!");

   return connectStatesTable[i];
}


// Player appears to be away, spawn is on hold until he returns
TNL_IMPLEMENT_RPC(GameConnection, s2cPlayerSpawnDelayed, (), (), NetClassGroupGameMask, RPCGuaranteed, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED  
   mClientGame->setSpawnDelayed(true);
#endif
}


void GameConnection::undelaySpawn()
{
   resetTimeSinceLastMove();

   ClientInfo *clientInfo = getClientInfo();

   clientInfo->setSpawnDelayed(false);
   mServerGame->unsuspendGame(false);     // Does nothing if game isn't suspended

   mServerGame->getGameType()->spawnShip(clientInfo);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sPlayerSpawnUndelayed, (), (), NetClassGroupGameMask, RPCGuaranteed, RPCDirClientToServer, 0)
{
   undelaySpawn();
}


TNL_IMPLEMENT_RPC(GameConnection, c2sPlayerRequestSpawnDelayed, (), (), NetClassGroupGameMask, RPCGuaranteed, RPCDirClientToServer, 0)
{
   ClientInfo *clientInfo = getClientInfo();

   clientInfo->setSpawnDelayed(true);

   // If we've just died, this will keep a second copy of ourselves from appearing
   clientInfo->respawnTimer.clear();

   // Now suicide!
   Ship *ship = clientInfo->getShip();
   if(ship)
      ship->kill();
}


// Old server side /getmap command, now unused, may be removed
// 1. client send /getmap command
// 2. server send map if allowed
// 3. When client get all the level map data parts, it create file and save the map

// This new client side /getmap command
// 1. client create file to write
// 2. client requent current level
// 3. server send data and client writes to file, What if sendmap not allowed?
// 4. server send CommandComplete
TNL_IMPLEMENT_RPC(GameConnection, c2sRequestCurrentLevel, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   if(!mSettings->getIniSettings()->allowGetMap)
   {
      s2rCommandComplete(COMMAND_NOT_ALLOWED);  
      return;
   }

   const char *filename = mServerGame->getCurrentLevelFileName().getString();
   
   // Initialize on the server to start sending requested file -- will return OK if everything is set up right
   FolderManager *folderManager = mSettings->getFolderManager();
   SenderStatus stat = mServerGame->dataSender.initialize(this, folderManager, filename, LEVEL_TYPE);

   if(stat != STATUS_OK)
   {
      const char *msg = DataConnection::getErrorMessage(stat, filename).c_str();

      logprintf(LogConsumer::LogError, "%s", msg);
      s2rCommandComplete(COULD_NOT_OPEN_FILE);
      return;
   }
}


const U32 maxDataBufferSize = 1024*256;

// << DataSendable >>
// Send a chunk of the file -- this gets run on the receiving end       
TNL_IMPLEMENT_RPC(GameConnection, s2rSendLine, (StringPtr line), (line), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
//void s2rSendLine2(StringPtr line)
{
   if(!isInitiator()) // make it client only.
      return;
   //// server might need mOutputFile, if the server were to receive files. Currently, server don't receive files in-game.
   //TNLAssert(mClientGame != NULL, "trying to get mOutputFile, mClientGame is NULL");

   //if(mClientGame && mClientGame->getUserInterface()->mOutputFile)
   //   fwrite(line.getString(), 1, strlen(line.getString()), mClientGame->getUserInterface()->mOutputFile);
      //mOutputFile.write(line.getString(), strlen(line.getString()));
   // else... what?
   if(mDataBuffer)
   {
      // Limit memory consumption:
      if(mDataBuffer->getBufferSize() < maxDataBufferSize)                                   
         mDataBuffer->appendBuffer((U8 *)line.getString(), (U32)strlen(line.getString()));
   }
   else
   {
      mDataBuffer = new ByteBuffer((U8 *)line.getString(), (U32)strlen(line.getString()));
      mDataBuffer->takeOwnership();
   }
}


// << DataSendable >>
// When sender is finished, it sends a commandComplete message
TNL_IMPLEMENT_RPC(GameConnection, s2rCommandComplete, (RangedU32<0,SENDER_STATUS_COUNT> status), (status), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   if(!isInitiator()) // Make it client only
      return;

#ifndef ZAP_DEDICATED
   // Server might need mOutputFile, if the server were to receive files.  Currently, server doesn't receive files in-game.
   TNLAssert(mClientGame != NULL, "We need a clientGame to proceed...");

   if(mClientGame)
   {
      string levelDir = mSettings->getFolderManager()->levelDir;
      string outputFilenameString = strictjoindir(levelDir, mClientGame->getRemoteLevelDownloadFilename());
      const char *outputFilename = outputFilenameString.c_str(); // when outputFilenameString goes out of scope or deleted, then pointer to outputFilename becomes invalid

      if(strcmp(outputFilename, ""))
      {
         if(status.value == STATUS_OK && mDataBuffer)
         {
            FILE *outputFile = fopen(outputFilename, "wb");

            if(!outputFile)
            {
               logprintf("Problem opening file %s for writing", outputFilename);
               mClientGame->displayErrorMessage("!!! Problem opening file %s for writing", outputFilename);
            }
            else
            {
               fwrite((char *)mDataBuffer->getBuffer(), 1, mDataBuffer->getBufferSize(), outputFile);
               fclose(outputFile);

               mClientGame->displaySuccessMessage("Level downloaded to %s", mClientGame->getRemoteLevelDownloadFilename().c_str());
            }
         }
         else if(status.value == COMMAND_NOT_ALLOWED)
            mClientGame->displayErrorMessage("!!! Getmap command is disabled on this server");
         else
            mClientGame->displayErrorMessage("Error downloading level");

         // mClientGame->setOutputFilename("");   <=== what is this for??  
      }
   }
#endif
   if(mDataBuffer)
   {
      delete mDataBuffer;
      mDataBuffer = NULL;
   }
}


extern md5wrapper md5;

void GameConnection::submitPassword(const char *password)
{
   string encrypted = md5.getSaltedHashFromString(password);
   c2sSubmitPassword(encrypted.c_str());

   mLastEnteredPassword = password;

   setGotPermissionsReply(false);
   setWaitingForPermissionsReply(true);      // Means we'll show a reply from the server
}


void GameConnection::suspendGame()
{
   c2sSuspendGame(true);
}


void GameConnection::unsuspendGame()
{
   c2sSuspendGame(false);
}

// Client requests that the game be suspended while he waits for other players.  This runs on the server.
TNL_IMPLEMENT_RPC(GameConnection, c2sSuspendGame, (bool suspend), (suspend), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   if(suspend)
      mServerGame->suspendGame(this);
   else
      mServerGame->unsuspendGame(true);
}


// If the server thinks everyone is alseep, it will suspend the game
TNL_IMPLEMENT_RPC(GameConnection, s2cSuspendGame, (bool gameIsRunning), (gameIsRunning), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   mClientGame->suspendGame(gameIsRunning);       
#endif
}
  

// Here, the server has sent a message to a suspended client to wake up, action's coming in hot!
// We'll also play the playerJoined sfx to alert local client that the game is on again.
TNL_IMPLEMENT_RPC(GameConnection, s2cUnsuspend, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   mClientGame->unsuspendGame();       
   SoundSystem::playSoundEffect(SFXPlayerJoined, 1);
#endif
}


void GameConnection::changeParam(const char *param, ParamType type)
{
   c2sSetParam(param, type);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sEngineerDeployObject, (RangedU32<0,EngineeredItemCount> objectType), (objectType),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   mClientInfo->sEngineerDeployObject(objectType);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sEngineerInterrupted, (RangedU32<0,EngineeredItemCount> objectType), (objectType),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   mClientInfo->sEngineerDeploymentInterrupted(objectType);
}


TNL_IMPLEMENT_RPC(GameConnection, s2cEngineerResponseEvent, (RangedU32<0,EngineerEventCount> event), (event),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   switch(event)
   {
      case EngineerEventTurretBuilt:
      case EngineerEventForceFieldBuilt:
         mClientGame->enterMode(PlayMode);  // Equivalent to HelperMenu::exitHelper();
         break;

      case EngineerEventTeleporterEntranceBuilt:
         mClientGame->setSelectedEngineeredObject(EngineeredTeleporterExit);
         break;

      case EngineerEventTeleporterExitBuilt:
         mClientGame->enterMode(PlayMode);  // Finally exit helper menu
         break;

      default:
         break;
   }
#endif
}


TNL_IMPLEMENT_RPC(GameConnection, s2cDisableWeaponsAndModules, (bool disable), (disable),
                  NetClassGroupGameMask, RPCGuaranteed, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   // For whatever reason mClientInfo here isn't what is grabbed in the Ship:: class
   mClientGame->findClientInfo(mClientInfo->getName())->setShipSystemsDisabled(disable);
#endif
}


// Client tells the server that they claim to be authenticated.  Of course, we need to verify this ourselves.
TNL_IMPLEMENT_RPC(GameConnection, c2sSetAuthenticated, (), (), 
                  NetClassGroupGameMask, RPCGuaranteed, RPCDirClientToServer, 0)
{
#ifndef ZAP_DEDICATED
   mClientInfo->setNeedToCheckAuthenticationWithMaster(true);

   requestAuthenticationVerificationFromMaster();
#endif
}


// A client has changed it's authentication status -- Only fired when game::setAuthenticated() is run on the server
TNL_IMPLEMENT_RPC(GameConnection, s2cSetAuthenticated, (StringTableEntry name, bool isAuthenticated, Int<BADGE_COUNT> badges), 
                                                       (name, isAuthenticated, badges), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   ClientInfo *clientInfo = mClientGame->findClientInfo(name);

   if(clientInfo)
      clientInfo->setAuthenticated(isAuthenticated, badges);
   //else
      // This can happen if we're hosting locally when we first join the game.  Not sure why, but it seems harmless...
#endif
}


TNL_IMPLEMENT_RPC(GameConnection, c2sSubmitPassword, (StringPtr pass), (pass), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   string adminPW = mSettings->getAdminPassword();
   string levChangePW = mSettings->getLevelChangePassword();

   // If admin password is blank, no one can get admin permissions except the local host, if there is one...
   if(!mClientInfo->isAdmin() && adminPW != "" && !strcmp(md5.getSaltedHashFromString(adminPW).c_str(), pass))
   {
      logprintf(LogConsumer::ServerFilter, "User [%s] granted admin permissions", mClientInfo->getName().getString());
      mWrongPasswordCount = 0;

      mClientInfo->setIsAdmin(true);               // Enter admin PW and...

      if(!mClientInfo->isLevelChanger())
      {
         mClientInfo->setIsLevelChanger(true);     // ...get these permissions too!
         sendLevelList();
      }
      
      s2cSetIsAdmin(true);                         // Tell client they have been granted access

      if(mSettings->getIniSettings()->allowAdminMapUpload)
         s2rSendableFlags(ServerFlagAllowUpload);                      // Enable level uploads

      GameType *gameType = mServerGame->getGameType();

      if(gameType)
         gameType->s2cClientBecameAdmin(mClientInfo->getName());  // Announce change to world
   }
   else if(!mClientInfo->isLevelChanger() && !strcmp(md5.getSaltedHashFromString(levChangePW).c_str(), pass)) // If level change password is blank, it should already been granted to all clients
   {
      logprintf(LogConsumer::ServerFilter, "User [%s] granted level change permissions", mClientInfo->getName().getString());
      mWrongPasswordCount = 0;

      mClientInfo->setIsLevelChanger(true);

      s2cSetIsLevelChanger(true, true);      // Tell client they have been granted access
      sendLevelList();                       // Send client the level list

      // Announce change to world
      mServerGame->getGameType()->s2cClientBecameLevelChanger(mClientInfo->getName());  
   }
   else
   {
      s2cWrongPassword();      // Tell client they have NOT been granted access

      logprintf(LogConsumer::LogConnection, "%s - client \"%s\" provided incorrect password.", 
                                               getNetAddressString(), mClientInfo->getName().getString());
      mWrongPasswordCount++;
      if(mWrongPasswordCount > MAX_WRONG_PASSWORD)
         disconnect(NetConnection::ReasonError, "Too many wrong password");
   }
}



// Allow admins to change the passwords on their systems
TNL_IMPLEMENT_RPC(GameConnection, c2sSetParam, (StringPtr param, RangedU32<0, GameConnection::ParamTypeCount> type), (param, type),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   if(!mClientInfo->isAdmin())   // Do nothing --> non-admins have no pull here.  Note that this should never happen; client should filter out
      return;                    // non-admins before we get here, but we'll check anyway in case the client has been hacked.

   // Check for forbidden blank parameters -- the following commands require a value to be passed in param
   if( (type == (U32)AdminPassword || type == (U32)ServerName || type == (U32)ServerDescr || type == (U32)LevelDir) &&
                          !strcmp(param.getString(), ""))
      return;

   // Add a message to the server log
   if(type == (U32)DeleteLevel)
      logprintf(LogConsumer::ServerFilter, "User [%s] added level [%s] to server skip list", mClientInfo->getName().getString(), 
                                                mServerGame->getCurrentLevelFileName().getString());
   else
   {
      const char *types[] = { "level change password", "admin password", "server password", "server name", "server description", "leveldir param" };
      logprintf(LogConsumer::ServerFilter, "User [%s] %s to [%s]", mClientInfo->getName().getString(), 
                                                                   strcmp(param.getString(), "") ? "changed" : "cleared", types[type]);
   }

   // Update our in-memory copies of the param, but do not save the new values to the INI
   if(type == (U32)LevelChangePassword)
      mSettings->setLevelChangePassword(param.getString(), false);
   
   else if(type == (U32)AdminPassword)
      mSettings->setAdminPassword(param.getString(), false);
   
   else if(type == (U32)ServerPassword)
      mSettings->setServerPassword(param.getString(), false);
   
   else if(type == (U32)ServerName)
   {
      mSettings->setHostName(param.getString(), false);
      if(mServerGame->getConnectionToMaster())
         mServerGame->getConnectionToMaster()->s2mChangeName(StringTableEntry(param.getString()));   
   }
   else if(type == (U32)ServerDescr)
   {
      mSettings->setHostDescr(param.getString(), false);

      if(mServerGame->getConnectionToMaster())
         mServerGame->getConnectionToMaster()->s2mServerDescription(StringTableEntry(param.getString()));
   }

   else if(type == (U32)LevelDir)
   {
      FolderManager *folderManager = mSettings->getFolderManager();
      string candidate = folderManager->resolveLevelDir(param.getString());

      if(folderManager->levelDir == candidate)
      {
         s2cDisplayErrorMessage("!!! Specified folder is already the current level folder");
         return;
      }

      // Make sure the specified dir exists; hopefully it contains levels
      if(candidate == "" || !fileExists(candidate))
      {
         s2cDisplayErrorMessage("!!! Could not find specified folder");
         return;
      }

      Vector<string> newLevels = mSettings->getLevelList(candidate);

      if(newLevels.size() == 0)
      {
         s2cDisplayErrorMessage("!!! Specified folder contains no levels");
         return;
      }

      mServerGame->buildBasicLevelInfoList(newLevels);      // Populates mLevelInfos on mServerGame with nearly empty LevelInfos 

      bool anyLoaded = false;

      for(S32 i = 0; i < newLevels.size(); i++)
      {
         string levelFile = folderManager->findLevelFile(candidate, newLevels[i]);

         LevelInfo levelInfo(newLevels[i]);
         if(mServerGame->getLevelInfo(levelFile, levelInfo))
         {
            if(!anyLoaded)    // i.e. we just found the first valid level; safe to clear out old list
               mServerGame->clearLevelInfos();

            anyLoaded = true;

            mServerGame->addLevelInfo(levelInfo);
         }
      }

      if(!anyLoaded)
      {
         s2cDisplayErrorMessage("!!! Specified folder contains no valid levels.  See server log for details.");
         return;
      }

      folderManager->levelDir = candidate;

      // Send the new list of levels to all levelchangers
      for(S32 i = 0; i < mServerGame->getClientCount(); i++)
      {
         ClientInfo *clientInfo = mServerGame->getClientInfo(i);
         GameConnection *conn = clientInfo->getConnection();

         if(clientInfo->isLevelChanger() && conn)
            conn->sendLevelList();
      }

      s2cDisplayMessage(ColorAqua, SFXNone, "Level folder changed");

   }  // end change leveldir

   else if(type == (U32)DeleteLevel)
   {
      // Avoid duplicates on skip list
      bool found = false;
      Vector<string> *skipList = mSettings->getLevelSkipList();

      for(S32 i = 0; i < skipList->size(); i++)
         if(skipList->get(i) == mServerGame->getCurrentLevelFileName().getString())    // Already on our list!
         {
            found = true;
            break;
         }

      if(!found)
      {
         // Add level to our skip list.  Deleting it from the active list of levels is more of a challenge...
         skipList->push_back(mServerGame->getCurrentLevelFileName().getString());
         writeSkipList(&gINI, skipList);           // Write skipped levels to INI
         gINI.WriteFile();                         // Save new INI settings to disk
      }
   }

   if(type != (U32)DeleteLevel && type != (U32)LevelDir)
   {
      const char *keys[] = { "LevelChangePassword", "AdminPassword", "ServerPassword", "ServerName", "ServerDescription" };

      // Update the INI file
      gINI.SetValue("Host", keys[type], param.getString(), true);
      gINI.WriteFile();    // Save new INI settings to disk
   }

   // Some messages we might show the user... should these just be inserted directly below?
   static StringTableEntry levelPassChanged   = "Level change password changed";
   static StringTableEntry levelPassCleared   = "Level change password cleared -- anyone can change levels";
   static StringTableEntry adminPassChanged   = "Admin password changed";
   static StringTableEntry serverPassChanged  = "Server password changed -- only players with the password can connect";
   static StringTableEntry serverPassCleared  = "Server password cleared -- anyone can connect";
   static StringTableEntry serverNameChanged  = "Server name changed";
   static StringTableEntry serverDescrChanged = "Server description changed";
   static StringTableEntry serverLevelDeleted = "Level added to skip list; level will stay in rotation until server restarted";

   // Pick out just the right message
   StringTableEntry msg;

   if(type == (U32)LevelChangePassword)
   {
      msg = strcmp(param.getString(), "") ? levelPassChanged : levelPassCleared;

      // If we're clearning the level change password, quietly grant access to anyone who doesn't already have it
      if(!strcmp(param.getString(), ""))
      {
         for(S32 i = 0; i < mServerGame->getClientCount(); i++)
         {
            ClientInfo *clientInfo = mServerGame->getClientInfo(i);
            GameConnection *conn = clientInfo->getConnection();

            if(!clientInfo->isLevelChanger())
            {
               clientInfo->setIsLevelChanger(true);
               if(conn)
               {
                  conn->sendLevelList();
                  conn->s2cSetIsLevelChanger(true, false);     // Silently
               }
            }
         }
      }
      else  // If setting a password, remove everyone's permissions (except admins)
      { 
         for(S32 i = 0; i < mServerGame->getClientCount(); i++)
         {
            ClientInfo *clientInfo = mServerGame->getClientInfo(i);
            GameConnection *conn = clientInfo->getConnection();

            if(clientInfo->isLevelChanger() && (!clientInfo->isAdmin()))
            {
               clientInfo->setIsLevelChanger(false);
               if(conn)
                  conn->s2cSetIsLevelChanger(false, false);
            }
         }
      }
   }
   else if(type == (U32)AdminPassword)
      msg = adminPassChanged;
   else if(type == (U32)ServerPassword)
      msg = strcmp(param.getString(), "") ? serverPassChanged : serverPassCleared;
   else if(type == (U32)ServerName)
   {
      msg = serverNameChanged;

      // If we've changed the server name, notify all the clients
      for(S32 i = 0; i < mServerGame->getClientCount(); i++)
         if(mServerGame->getClientInfo(i)->getConnection())
            mServerGame->getClientInfo(i)->getConnection()->s2cSetServerName(mSettings->getHostName());
   }
   else if(type == (U32)ServerDescr)
      msg = serverDescrChanged;
   else if(type == (U32)DeleteLevel)
      msg = serverLevelDeleted;

   s2cDisplayMessage(ColorRed, SFXNone, msg);      // Notify user their bidding has been done
}


//// Announce a new player has become an admin
//TNL_IMPLEMENT_RPC(GameConnection, s2cClientBecameAdmin, (StringTableEntry name), (name), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
//{
//   ClientRef *cl = findClientConnection(name);
//   cl->clientConnection->isAdmin = true;
//   getUserInterface().displayMessage(Color(0,1,1), "%s has been granted administrator access.", name.getString());
//}


// This gets called under two circumstances; when it's a new game, or when the server's name is changed by an admin
TNL_IMPLEMENT_RPC(GameConnection, s2cSetServerName, (StringTableEntry name), (name),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
   setServerName(name);

   // If we know the level change password, apply for permissions if we don't already have them
   if(!mClientInfo->isLevelChanger())
   {
      string levelChangePassword = gINI.GetValue("SavedLevelChangePasswords", getServerName());
      if(levelChangePassword != "")
      {
         c2sSubmitPassword(md5.getSaltedHashFromString(levelChangePassword).c_str());
         setWaitingForPermissionsReply(false);     // Want to return silently
      }
   }

   // If we know the admin password, apply for permissions if we don't already have them
   if(!mClientInfo->isAdmin())
   {
      string adminPassword = gINI.GetValue("SavedAdminPasswords", getServerName());
      if(adminPassword != "")
      {
         c2sSubmitPassword(md5.getSaltedHashFromString(adminPassword).c_str());
         setWaitingForPermissionsReply(false);     // Want to return silently
      }
   }
}


extern Color gCmdChatColor;

TNL_IMPLEMENT_RPC(GameConnection, s2cSetIsAdmin, (bool granted), (granted),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED

   ClientInfo *clientInfo = mClientGame->getClientInfo();

   clientInfo->setIsAdmin(granted);

   // Admin permissions automatically give level change permission
   if(granted)                      // Don't rescind level change permissions for entering a bad admin PW
   {
      //if(!clientInfo->isLevelChanger())
      //   sendLevelList();  // error, client send level list to server?

      clientInfo->setIsLevelChanger(true);
   }


   // If we entered a password, and it worked, let's save it for next time
   if(granted && mLastEnteredPassword != "")
   {
      gINI.SetValue("SavedAdminPasswords", getServerName(), mLastEnteredPassword, true);
      mLastEnteredPassword = "";
   }

   // We have the wrong password, let's make sure it's not saved
   if(!granted)
      gINI.deleteKey("SavedAdminPasswords", getServerName());

   setGotPermissionsReply(true);

   // If we're not waiting, don't do anything.  Supresses superflous messages on startup.
   if(waitingForPermissionsReply())
      mClientGame->gotAdminPermissionsReply(granted);
#endif
}


TNL_IMPLEMENT_RPC(GameConnection, s2cSetIsLevelChanger, (bool granted, bool notify), (granted, notify),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED

   // If we entered a password, and it worked, let's save it for next time
   if(granted && mLastEnteredPassword != "")
   {
      gINI.SetValue("SavedLevelChangePasswords", getServerName(), mLastEnteredPassword, true);
      mLastEnteredPassword = "";
   }

   // We have the wrong password, let's make sure it's not saved
   if(!granted)
      gINI.deleteKey("SavedLevelChangePasswords", getServerName());

   ClientInfo *clientInfo = mClientGame->getClientInfo();

   // Check for permissions being rescinded by server, will happen if admin changes level change pw
   if(clientInfo->isLevelChanger() && !granted)
      mClientGame->displayMessage(gCmdChatColor, "An admin has changed the level change password; you must enter the new password to change levels.");

   clientInfo->setIsLevelChanger(granted);

   setGotPermissionsReply(true);

   // If we're not waiting, don't show us a message.  Supresses superflous messages on startup.
   if(waitingForPermissionsReply() && notify)
      mClientGame->gotLevelChangePermissionsReply(granted);
#endif
}

TNL_IMPLEMENT_RPC(GameConnection, s2cWrongPassword, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   if(waitingForPermissionsReply())
      mClientGame->gotWrongPassword();
#endif
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestCommanderMap, (), (),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   mInCommanderMap = true;
}


TNL_IMPLEMENT_RPC(GameConnection, c2sReleaseCommanderMap, (), (),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   mInCommanderMap = false;
}


// Client has changed his loadout configuration.  This gets run on the server as soon as the loadout is entered.
TNL_IMPLEMENT_RPC(GameConnection, c2sRequestLoadout, (Vector<U8> loadout), (loadout), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   getClientInfo()->sRequestLoadout(loadout);
}


// TODO: make this an xmacro
Color colors[] =
{
   Colors::white,          // ColorWhite
   Colors::red,            // ColorRed    ==> also used for chat commands
   Colors::green,          // ColorGreen
   Colors::blue,           // ColorBlue
   Colors::cyan,           // ColorAqua
   Colors::yellow,         // ColorYellow
   Color(0.6f, 1, 0.8f),   // ColorNuclearGreen
};


Color gCmdChatColor = colors[GameConnection::ColorRed];

void GameConnection::displayMessage(U32 colorIndex, U32 sfxEnum, const char *message)
{
#ifndef ZAP_DEDICATED
   mClientGame->displayMessage(colors[colorIndex], "%s", message);
   if(sfxEnum != SFXNone)
      SoundSystem::playSoundEffect(sfxEnum);
#endif
}


// I believe this is not used -CE
TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessageESI,
                  (RangedU32<0, GameConnection::ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString,
                  Vector<StringTableEntry> e, Vector<StringPtr> s, Vector<S32> i),
                  (color, sfx, formatString, e, s, i),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
   char outputBuffer[256];
   S32 pos = 0;
   const char *src = formatString.getString();
   while(*src)
   {
      if(src[0] == '%' && (src[1] == 'e' || src[1] == 's' || src[1] == 'i') && (src[2] >= '0' && src[2] <= '9'))
      {
         S32 index = src[2] - '0';
         switch(src[1])
         {
            case 'e':
               if(index < e.size())
                  pos += dSprintf(outputBuffer + pos, 256 - pos, "%s", e[index].getString());
               break;
            case 's':
               if(index < s.size())
                  pos += dSprintf(outputBuffer + pos, 256 - pos, "%s", s[index].getString());
               break;
            case 'i':
               if(index < i.size())
                  pos += dSprintf(outputBuffer + pos, 256 - pos, "%d", i[index]);
               break;
         }
         src += 3;
      }
      else
         outputBuffer[pos++] = *src++;

      if(pos >= 255)
         break;
   }
   outputBuffer[pos] = 0;
   displayMessage(color, sfx, outputBuffer);
}


TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessageE,
                  (RangedU32<0, GameConnection::ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString,
                  Vector<StringTableEntry> e), (color, sfx, formatString, e),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
   displayMessageE(color, sfx, formatString, e);
}


TNL_IMPLEMENT_RPC(GameConnection, s2cTouchdownScored,
                  (RangedU32<0, NumSFXBuffers> sfx, S32 team, StringTableEntry formatString, Vector<StringTableEntry> e),
                  (sfx, team, formatString, e),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   if(formatString.getString()[0] != 0)
      displayMessageE(GameConnection::ColorNuclearGreen, sfx, formatString, e);
   if(mClientGame->getGameType())
      mClientGame->getGameType()->majorScoringEventOcurred(team);
#endif
}


void GameConnection::displayMessageE(U32 color, U32 sfx, StringTableEntry formatString, Vector<StringTableEntry> e)
{
   char outputBuffer[256];
   S32 pos = 0;
   const char *src = formatString.getString();
   while(*src)
   {
      if(src[0] == '%' && (src[1] == 'e') && (src[2] >= '0' && src[2] <= '9'))
      {
         S32 index = src[2] - '0';
         switch(src[1])
         {
            case 'e':
               if(index < e.size())
                  pos += dSprintf(outputBuffer + pos, 256 - pos, "%s", e[index].getString());
               break;
         }
         src += 3;
      }
      else
         outputBuffer[pos++] = *src++;

      if(pos >= 255)
         break;
   }
   outputBuffer[pos] = 0;
   displayMessage(color, sfx, outputBuffer);
}


void GameConnection::sendLevelList()
{
   // Send blank entry to clear the remote list
   s2cAddLevel("", NoGameType);    

   // Build list remotely by sending level names one-by-one
   for(S32 i = 0; i < mServerGame->getLevelCount(); i++)
   {
      LevelInfo levelInfo = mServerGame->getLevelInfo(i);
      s2cAddLevel(levelInfo.mLevelName, levelInfo.mLevelType);
   }
}


TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessage,
                  (RangedU32<0, GameConnection::ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString),
                  (color, sfx, formatString),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
   displayMessage(color, sfx, formatString.getString());
}


TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayErrorMessage,
                  (StringTableEntry formatString),
                  (formatString),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   mClientGame->displayErrorMessage(formatString.getString());
#endif
}


TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessageBox, (StringTableEntry title, StringTableEntry instr, Vector<StringTableEntry> message),
                  (title, instr, message), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   mClientGame->displayMessageBox(title, instr, message);
#endif
}


// Server sends the name and type of a level to the client (gets run repeatedly when client connects to the server). 
// Sending a blank name and type will clear the list.
TNL_IMPLEMENT_RPC(GameConnection, s2cAddLevel, (StringTableEntry name, RangedU32<0, GameTypesCount> type), (name, type),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
   // Sending a blank name and type will clear the list.  Type should never be blank except in this use case, so check it first.
   if(type == NoGameType)
      mLevelInfos.clear();
   else
      mLevelInfos.push_back(LevelInfo(name, (GameTypeId)type.value));
}


// Server sends the level that got removed, or removes all levels from list when index is -1
// Unused??
TNL_IMPLEMENT_RPC(GameConnection, s2cRemoveLevel, (S32 index), (index),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
   if(index < 0)
      mLevelInfos.clear();
   else if(index < mLevelInfos.size())
      mLevelInfos.erase(index);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestLevelChange, (S32 newLevelIndex, bool isRelative), (newLevelIndex, isRelative), 
                              NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   if(!mClientInfo->isLevelChanger())
   {
      s2cDisplayErrorMessage("!!! Need level change permission");  // currently can come from GameType::processServerCommand "/random"
      return;
   }

   // Use voting when there is no level change password and there is more then 1 player (unless changer is an admin)
   if(!mClientInfo->isAdmin() && mSettings->getLevelChangePassword().length() == 0 && 
         mServerGame->getPlayerCount() > 1 && mServerGame->voteStart(mClientInfo, 0, newLevelIndex))
      return;

   bool restart = false;

   if(isRelative)
      newLevelIndex = (mServerGame->getCurrentLevelIndex() + newLevelIndex ) % mServerGame->getLevelCount();
   else if(newLevelIndex == ServerGame::REPLAY_LEVEL)
      restart = true;

   StringTableEntry msg( restart ? "%e0 restarted the current level." : "%e0 changed the level to %e1." );
   Vector<StringTableEntry> e;
   e.push_back(mClientInfo->getName()); 
   
   if(!restart)
      e.push_back(mServerGame->getLevelNameFromIndex(newLevelIndex));

   mServerGame->cycleLevel(newLevelIndex);

   mServerGame->getGameType()->broadcastMessage(ColorYellow, SFXNone, msg, e);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestShutdown, (U16 time, StringPtr reason), (time, reason), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   if(!mClientInfo->isAdmin())
      return;

   logprintf(LogConsumer::ServerFilter, "User [%s] requested shutdown in %d seconds [%s]", 
                                        mClientInfo->getName().getString(), time, reason.getString());

   mServerGame->setShuttingDown(true, time, this, reason.getString());

   // Broadcast the message
   for(S32 i = 0; i < mServerGame->getClientCount(); i++)
   {
      GameConnection *conn = mServerGame->getClientInfo(i)->getConnection();
      if(conn)
         conn->s2cInitiateShutdown(time, mClientInfo->getName(), reason, conn == this);
   }
}


TNL_IMPLEMENT_RPC(GameConnection, s2cInitiateShutdown, (U16 time, StringTableEntry name, StringPtr reason, bool originator),
                  (time, name, reason, originator), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   mClientGame->shutdownInitiated(time, name, reason, originator);
#endif
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestCancelShutdown, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   if(!mClientInfo->isAdmin())
      return;

   logprintf(LogConsumer::ServerFilter, "User %s canceled shutdown", mClientInfo->getName().getString());

   // Broadcast the message
   for(S32 i = 0; i < mServerGame->getClientCount(); i++)
   {
      GameConnection *conn = mServerGame->getClientInfo(i)->getConnection();

      if(conn != this)     // Don't send message to cancellor!
         conn->s2cCancelShutdown();
   }

   mServerGame->setShuttingDown(false, 0, NULL, "");
}


TNL_IMPLEMENT_RPC(GameConnection, s2cCancelShutdown, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   mClientGame->cancelShutdown();
#endif
}


// Server tells clients that another player is idle and will not be joining us for the moment
TNL_IMPLEMENT_RPC(GameConnection, s2cSetIsBusy, (StringTableEntry name, bool isBusy), (name, isBusy), 
                  NetClassGroupGameMask, RPCGuaranteed, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   ClientInfo *clientInfo = mClientGame->findClientInfo(name);

   // Might not find clientInfo if level just cycled and players haven't been re-sent to client yet.  In which case,
   // this is ok, since busy status will be sent with s2cAddClient().

   if(!clientInfo)      
      return;

   clientInfo->setIsBusy(isBusy);     
#endif
}


// Client tells server that they are busy chatting or futzing with menus or configuring ship... or not
TNL_IMPLEMENT_RPC(GameConnection, c2sSetIsBusy, (bool isBusy), (isBusy), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   if(mIsBusy == isBusy)
      return;

   for(S32 i = 0; i < mServerGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mServerGame->getClientInfo(i);

      if(clientInfo->isRobot())
         continue;

      clientInfo->getConnection()->s2cSetIsBusy(mClientInfo->getName(), isBusy);
   }

   mIsBusy = isBusy;
   mClientInfo->setIsBusy(isBusy);

   // If we're busy, force spawndelay timer to run out
   addTimeSinceLastMove(SPAWN_DELAY_TIME);

   // Respawn if we were spawn delayed
   if(!isBusy && mClientInfo->isSpawnDelayed())
      undelaySpawn();
}


TNL_IMPLEMENT_RPC(GameConnection, c2sSetServerAlertVolume, (S8 vol), (vol), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   //setServerAlertVolume(vol);
}


// Client connects to master after joining a game, authentication fails,
// then client has changed name to non-reserved, or entered password.
TNL_IMPLEMENT_RPC(GameConnection, c2sRenameClient, (StringTableEntry newName), (newName), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   StringTableEntry oldName = mClientInfo->getName();
   mClientInfo->setName("");     

   StringTableEntry uniqueName = mServerGame->makeUnique(newName.getString()).c_str();    // Make sure new name is unique
   mClientInfo->setName(oldName);         // Restore name to properly get it updated to clients
   setClientNameNonUnique(newName);       // For correct authentication
   
   mClientInfo->setAuthenticated(false, NO_BADGES);               // Prevents name from being underlined
   mClientInfo->setNeedToCheckAuthenticationWithMaster(false);    // Do not inquire with master

   if(oldName != uniqueName)              // Did the name actually change?
      mServerGame->getGameType()->updateClientChangedName(mClientInfo, uniqueName);
}


TNL_IMPLEMENT_RPC(GameConnection, s2rSendableFlags, (U8 flags), (flags), NetClassGroupGameMask, RPCGuaranteed, RPCDirAny, 0)
{
   mSendableFlags = flags;
}


extern LevelInfo getLevelInfoFromFileChunk(char *chunk, S32 size, LevelInfo &levelInfo);

TNL_IMPLEMENT_RPC(GameConnection, s2rSendDataParts, (U8 type, ByteBufferPtr data), (type, data), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   if(!mSettings->getIniSettings()->allowMapUpload && !mClientInfo->isAdmin())  // Don't need it when not enabled, saves some memory. May remove this, it is checked again leter.
      return;

   if(mDataBuffer)
   {
      if(mDataBuffer->getBufferSize() < maxDataBufferSize)  // Limit memory consumption
         mDataBuffer->appendBuffer(*data.getPointer());
   }
   else
   {
      mDataBuffer = new ByteBuffer(*data.getPointer());
      mDataBuffer->takeOwnership();
   }

   if(type == LevelFileTransmissionInProgress &&
         (mSettings->getIniSettings()->allowMapUpload || (mSettings->getIniSettings()->allowAdminMapUpload && mClientInfo->isAdmin())) &&
         !isInitiator() && mDataBuffer->getBufferSize() != 0)
   {
      // Only server runs this part of code
      FolderManager *folderManager = mSettings->getFolderManager();

      LevelInfo levelInfo("Transmitted Level");
      getLevelInfoFromFileChunk((char *)mDataBuffer->getBuffer(), mDataBuffer->getBufferSize(), levelInfo);

      //BitStream s(mDataBuffer.getBuffer(), mDataBuffer.getBufferSize());
      char filename[128];

      string titleName = makeFilenameFromString(levelInfo.mLevelName.getString());
      dSprintf(filename, sizeof(filename), "upload_%s.level", titleName.c_str());

      string fullFilename = strictjoindir(folderManager->levelDir, filename);

      FILE *f = fopen(fullFilename.c_str(), "wb");
      if(f)
      {
         fwrite(mDataBuffer->getBuffer(), 1, mDataBuffer->getBufferSize(), f);
         fclose(f);
         logprintf(LogConsumer::ServerFilter, "%s %s Uploaded %s", getNetAddressString(), mClientInfo->getName().getString(), filename);
         S32 id = mServerGame->addUploadedLevelInfo(filename, levelInfo);
         c2sRequestLevelChange_remote(id, false);  // we are server (switching to it after fully uploaded)
      }
      else
         s2cDisplayErrorMessage("!!! Upload failed -- server can't write file");
   }

   if(type != LevelFileTransmissionComplete)
   {
      delete mDataBuffer;
      mDataBuffer = NULL;
   }
}


bool GameConnection::s2rUploadFile(const char *filename, U8 type)
{
   BitStream s;
   const U32 partsSize = 512;   // max 1023, limited by ByteBufferSizeBitSize value of 10

   FILE *f = fopen(filename, "rb");

   if(f)
   {
      U32 size = partsSize;
      while(size == partsSize)
      {
         ByteBuffer *bytebuffer = new ByteBuffer(512);

         size = (U32)fread(bytebuffer->getBuffer(), 1, bytebuffer->getBufferSize(), f);

         if(size != partsSize)
            bytebuffer->resize(size);

         s2rSendDataParts(size == partsSize ? LevelFileTransmissionComplete : type, ByteBufferPtr(bytebuffer));
      }
      fclose(f);
      return true;
   }

   return false;
}



TNL_IMPLEMENT_RPC(GameConnection, s2rVoiceChatEnable, (bool enable), (enable), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   mVoiceChatEnabled = enable;
}


// Send password, client's name, and version info to game server
void GameConnection::writeConnectRequest(BitStream *stream)
{
#ifndef ZAP_DEDICATED
   Parent::writeConnectRequest(stream);

   //bool isLocal = isLocalConnection();
   TNLAssert((gServerGame != NULL) == isLocalConnection(), "If this triggers, make a note, and delete this assert.  If not, we can get replace isLocal with isLocalConnection().");

   stream->write(CONNECT_VERSION);

   string serverPW;
   string lastServerName = mClientGame->getRequestedServerName();

   // If we're local, just use the password we already know because, you know, we're the server
   if(isLocalConnection())
      serverPW = md5.getSaltedHashFromString(mSettings->getServerPassword());

   // If we have a saved password for this server, use that
   else if(gINI.GetValue("SavedServerPasswords", lastServerName) != "")
      serverPW = md5.getSaltedHashFromString(gINI.GetValue("SavedServerPasswords", lastServerName)); 

   // Otherwise, use whatever's in the interface entry box
   else 
      serverPW = mClientGame->getHashedServerPassword();

   // Write some info about the client... name, id, and verification status
   stream->writeString(serverPW.c_str());
   stream->writeString(mClientInfo->getName().getString());

    mClientInfo->getId()->write(stream);

    stream->writeFlag(mClientInfo->isAuthenticated());    // Tell server whether we (the client) claim to be authenticated
#endif
}


// On the server side of things, read the connection request, and return if everything looks legit.  If not, provide an error string
// to help diagnose the problem, or prompt further data from client (such as a password).
// Note that we'll always go through this, even if the client is running on in the same process as the server.
// Note also that mSettings will be NULL here.
bool GameConnection::readConnectRequest(BitStream *stream, NetConnection::TerminationReason &reason)
{
   if(!Parent::readConnectRequest(stream, reason))
      return false;

   char buf[256];

   GameNetInterface *gameNetInterface = dynamic_cast<GameNetInterface *>(getInterface());
   if(!gameNetInterface)
      return false;   // need a GameNetInterface

   mServerGame = gameNetInterface->getGame()->isServer() ? (ServerGame *)gameNetInterface->getGame() : NULL;
   if(!mServerGame)
      return false;  // need a ServerGame

   TNLAssert(!mClientInfo, "mClientInfo should be NULL");
   mClientInfo = new FullClientInfo(mServerGame, this, false);         // Deleted in destructor
   mSettings = mServerGame->getSettings();  // now that we got the server, set the settings.

   stream->read(&mConnectionVersion);

   stream->readString(buf);
   string serverPassword = mServerGame->getSettings()->getServerPassword();

   if(serverPassword != "" && stricmp(buf, md5.getSaltedHashFromString(serverPassword).c_str()))
   {
      reason = ReasonNeedServerPassword;
      return false;
   }

   // Now read the player name, id, and verification status
   stream->readString(buf);
   size_t len = strlen(buf);

   if(len > MAX_PLAYER_NAME_LENGTH)      // Make sure it isn't too long
      len = MAX_PLAYER_NAME_LENGTH;

   // Clean up name, render it safe

   // Strip leading and trailing spaces...
   char *name = buf;
   while(len && *name == ' ')
   {
      name++;
      len--;
   }
   while(len && name[len-1] == ' ')
      len--;

   // Remove invisible chars and quotes
   for(size_t i = 0; i < len; i++)
      if(name[i] < ' ' || name[i] > '~' || name[i] == '"')
         name[i] = 'X';

   name[len] = 0;    // Terminate string properly

   // Change name to be unique - i.e. if we have multiples of 'ChumpChange'
   mClientInfo->setName(mServerGame->makeUnique(name));   // Unique name
   mClientNameNonUnique = name;              // For authentication non-unique name

   mClientInfo->getId()->read(stream);
   bool needToCheckAuthentication = stream->readFlag();
   mClientInfo->setNeedToCheckAuthenticationWithMaster(needToCheckAuthentication);

   if(!isLocalConnection())  // don't disconnect local host making "Host game" unusable, which could be a result of ban yourself..
   {
      // Now that we have the name, check if the client is banned,
      // can't use isAuthenticated() until after waiting for m2sSetAuthenticated, using needToCheckAuthentication instead.
      if(mServerGame->getSettings()->getBanList()->isBanned(getNetAddress().toString(), string(name), needToCheckAuthentication))
      {
         reason = ReasonBanned;
         return false;
      }

      // Was the client kicked temporarily?
      if(mServerGame->getSettings()->getBanList()->isAddressKicked(getNetAddress()))
      {
         reason = ReasonKickedByAdmin;
         return false;
      }

      // Is the server Full?
      if(mServerGame->isFull())
      {
         reason = ReasonServerFull;
         return false;
      }
   }

   requestAuthenticationVerificationFromMaster();    

   return true;
}


// Server side writes ConnectAccept
void GameConnection::writeConnectAccept(BitStream *stream)
{
   Parent::writeConnectAccept(stream);
   stream->write(CONNECT_VERSION);

   stream->writeFlag(mServerGame->getSettings()->getIniSettings()->enableServerVoiceChat);
}


// Client side reads ConnectAccept
bool GameConnection::readConnectAccept(BitStream *stream, NetConnection::TerminationReason &reason)
{
   if(!Parent::readConnectAccept(stream, reason))
      return false;
   stream->read(&mConnectionVersion);

   mVoiceChatEnabled = stream->readFlag();
   return true;
}


void GameConnection::resetAuthenticationTimer()
{
   mAuthenticationTimer.reset(MASTER_SERVER_FAILURE_RETRY_TIME + 1000);
   mAuthenticationCounter++;
}


S32 GameConnection::getAuthenticationCounter()
{
   return mAuthenticationCounter;
}


void GameConnection::updateTimers(U32 timeDelta)
{
   if(mAuthenticationTimer.update(timeDelta))
      requestAuthenticationVerificationFromMaster();

   if(!isInitiator())
      addToTimeCredit(timeDelta);

   if(mVoteTime <= timeDelta)
      mVoteTime = 0;
   else
      mVoteTime -= timeDelta;
}


void GameConnection::requestAuthenticationVerificationFromMaster()
{
   MasterServerConnection *masterConn = mServerGame->getConnectionToMaster();

   // Ask master if client name/id match and the client is authenticated; don't bother if they're already authenticated, or
   // if they don't claim they are
   if(!mClientInfo->isAuthenticated() && masterConn && masterConn->isEstablished() && mClientInfo->getNeedToCheckAuthenticationWithMaster())
      masterConn->requestAuthentication(mClientNameNonUnique, *mClientInfo->getId());   
}


void GameConnection::setClientNameNonUnique(StringTableEntry name)
{
   mClientNameNonUnique = name;
}


void GameConnection::setServerName(StringTableEntry name)
{
   mServerName = name;
}


ClientInfo *GameConnection::getClientInfo()
{
   return mClientInfo;
}


void GameConnection::setClientInfo(ClientInfo *clientInfo)
{
   mClientInfo = clientInfo;
}


bool GameConnection::lostContact()
{
   return getTimeSinceLastPacketReceived() > 2000 && mLastPacketRecvTime != 0;   // No contact in 2000ms?  That's bad!
}


string GameConnection::getServerName()
{
   return mServerName.getString();
}


void GameConnection::setConnectionSpeed(S32 speed)
{
   U32 minPacketSendPeriod;
   U32 minPacketRecvPeriod;
   U32 maxSendBandwidth;
   U32 maxRecvBandwidth;
   if(speed <= -2)
   {
      minPacketSendPeriod = 80;
      minPacketRecvPeriod = 80;
      maxSendBandwidth = 800;
      maxRecvBandwidth = 800;
   }
   else if(speed == -1)
   {
      minPacketSendPeriod = 50; //  <== original zap settings
      minPacketRecvPeriod = 50;
      maxSendBandwidth = 2000;
      maxRecvBandwidth = 2000;
   }
   else if(speed == 0)
   {
      minPacketSendPeriod = 45;
      minPacketRecvPeriod = 45;
      maxSendBandwidth = 8000;
      maxRecvBandwidth = 8000;
   }
   else if(speed == 1)
   {
      minPacketSendPeriod = 30;
      minPacketRecvPeriod = 30;
      maxSendBandwidth = 20000;
      maxRecvBandwidth = 20000;
   }
   else if(speed >= 2)
   {
      minPacketSendPeriod = 20;
      minPacketRecvPeriod = 20;
      maxSendBandwidth = 65535;
      maxRecvBandwidth = 65535;
   }

   //if(this->isLocalConnection())    // Local connections don't use network, maximum bandwidth
   //{
   //   minPacketSendPeriod = 15;
   //   minPacketRecvPeriod = 15;
   //   maxSendBandwidth = 65535;     // Error when higher than 65535
   //   maxRecvBandwidth = 65535;
   //}

   setFixedRateParameters(minPacketSendPeriod, minPacketRecvPeriod, maxSendBandwidth, maxRecvBandwidth);
}

// Runs on client and server
void GameConnection::onConnectionEstablished()
{
   Parent::onConnectionEstablished();

   if(isInitiator())    
      onConnectionEstablished_client();
   else                 
      onConnectionEstablished_server();
}


void GameConnection::onConnectionEstablished_client()
{
#ifndef ZAP_DEDICATED
   setConnectionSpeed(mClientGame->getSettings()->getIniSettings()->connectionSpeed);  // set speed depending on client
   mClientGame->setInCommanderMap(false);       // Start game in regular mode
   mClientGame->clearZoomDelta();               // No in zoom effect
   setGhostFrom(false);
   setGhostTo(true);
   logprintf(LogConsumer::LogConnection, "%s - connected to server.", getNetAddressString());


   // If we arrive here and the saved password is empty it means that the user entered a good password.  
   // So let's save it for next time.

   //bool isLocal = gServerGame;
   TNLAssert((gServerGame != NULL) == isLocalConnection(), "If this triggers, make a note, and delete this assert.  If not, we can get replace isLocal with isLocalConnection().");
      
   string lastServerName = mClientGame->getRequestedServerName();

   if(!isLocalConnection() && gINI.GetValue("SavedServerPasswords", lastServerName) == "")
      gINI.SetValue("SavedServerPasswords", lastServerName, mClientGame->getServerPassword(), true);


   if(!isLocalConnection())    // Might use /connect, want to add to list after successfully connected. Does nothing while connected to master.
   {         
      string addr = getNetAddressString();
      bool found = false;

      for(S32 i = 0; i < mSettings->getIniSettings()->prevServerListFromMaster.size(); i++)
         if(mSettings->getIniSettings()->prevServerListFromMaster[i].compare(addr) == 0) 
         {
            found = true;
            break;
         }

      if(!found) 
         mSettings->getIniSettings()->prevServerListFromMaster.push_back(addr);
   }

   if(mSettings->getIniSettings()->voiceChatVolLevel == 0)
      s2rVoiceChatEnable(false);
#endif
}


void GameConnection::onConnectionEstablished_server()
{
   setConnectionSpeed(2);  // high speed, most servers often have a lot of bandwidth available.
   mServerGame->addClient(mClientInfo);
   setGhostFrom(true);
   setGhostTo(false);
   activateGhosting();
   //setFixedRateParameters(minPacketSendPeriod, minPacketRecvPeriod, maxSendBandwidth, maxRecvBandwidth);  // make this client only?

   // Ideally, the server name would be part of the connection handshake, but this will work as well
   s2cSetServerName(mServerGame->getSettings()->getHostName());   // Note: mSettings is NULL here

   time(&joinTime);
   mAcheivedConnection = true;
      
   // Notify the bots that a new player has joined
   EventManager::get()->fireEvent(NULL, EventManager::PlayerJoinedEvent, getClientInfo()->getPlayerInfo());

   if(mServerGame->getSettings()->getLevelChangePassword() == "")   // Grant level change permissions if level change PW is blank
   {
      mClientInfo->setIsLevelChanger(true);
      s2cSetIsLevelChanger(true, false);          // Tell client, but don't display notification
      sendLevelList();
   }

   const char *name =  mClientInfo->getName().getString();

   logprintf(LogConsumer::LogConnection, "%s - client \"%s\" connected.", getNetAddressString(), name);
   logprintf(LogConsumer::ServerFilter,  "%s [%s] joined [%s]", name, 
                                          isLocalConnection() ? "Local Connection" : getNetAddressString(), getTimeStamp().c_str());

   if(mServerGame->getSettings()->getIniSettings()->allowMapUpload)
      s2rSendableFlags(ServerFlagAllowUpload);
}


// Established connection is terminated.  Compare to onConnectTerminate() below.
void GameConnection::onConnectionTerminated(NetConnection::TerminationReason reason, const char *reasonStr)
{
   if(isInitiator())    // i.e. this is a client that connected to the server
   {
#ifndef ZAP_DEDICATED
      TNLAssert(mClientGame, "onConnectionTerminated: mClientGame is NULL");

      if(mClientGame)
         mClientGame->onConnectionTerminated(getNetAddress(), reason, reasonStr, true);
#endif
   }
   else     // Server
   {
      LuaPlayerInfo *playerInfo = getClientInfo()->getPlayerInfo();
      playerInfo->setDefunct();
      EventManager::get()->fireEvent(NULL, EventManager::PlayerLeftEvent, playerInfo);

      mServerGame->removeClient(mClientInfo);
   }
}


// This function only gets called while the player is trying to connect to a server.  Connection has not yet been established.
// Compare to onConnectIONTerminated()
void GameConnection::onConnectTerminated(TerminationReason reason, const char *reasonStr)
{
   if(isInitiator())
   {
#ifndef ZAP_DEDICATED
      TNLAssert(mClientGame, "onConnectTerminated: mClientGame is NULL");
      if(!mClientGame)
         return;

      mClientGame->onConnectionTerminated(getNetAddress(), reason, reasonStr, false);
#endif

   }
}


bool GameConnection::isReadyForRegularGhosts()
{
   return mReadyForRegularGhosts;
}


void GameConnection::setReadyForRegularGhosts(bool ready)
{
   mReadyForRegularGhosts = ready;
}


bool GameConnection::wantsScoreboardUpdates()
{
   return mWantsScoreboardUpdates;
}


void GameConnection::setWantsScoreboardUpdates(bool wantsUpdates)
{
   mWantsScoreboardUpdates = wantsUpdates;
}


// Gets run when game is really and truly over, after post-game scoreboard is displayed.  Over.
// Some keywords to help find this function again: level over, change level, game over
// Client only
void GameConnection::onEndGhosting()
{
#ifndef ZAP_DEDICATED
   TNLAssert(isConnectionToServer() && mClientGame, "when else is this called?");
   
   Parent::onEndGhosting();
   mClientGame->clearClientList();                   // Erase all info we have about fellow clients

   if(mClientGame->getUIMode() == TeamShuffleMode)   // Exit Shuffle helper to keep things from getting too crashy
      mClientGame->enterMode(PlayMode);             

#endif
}


void GameConnection::setWaitingForPermissionsReply(bool waiting)
{
   mWaitingForPermissionsReply = waiting;
}


bool GameConnection::waitingForPermissionsReply()
{
   return mWaitingForPermissionsReply;
}


void GameConnection::setGotPermissionsReply(bool gotReply)
{
   mGotPermissionsReply = gotReply;
}


bool GameConnection::gotPermissionsReply()
{
   return mGotPermissionsReply;
}


bool GameConnection::isInCommanderMap()
{
   return mInCommanderMap;
}


const Vector<U8> &GameConnection::getLoadout()
{
   return mLoadout;
}


};


