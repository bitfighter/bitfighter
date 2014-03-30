//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gameConnection.h"

#include "ServerGame.h"
#include "IniFile.h"             // For CIniFile def
#include "shipItems.h"           // For EngineerBuildObjects enum
#include "masterConnection.h"    // For MasterServerConnection def
#include "GameSettings.h"
#include "BanList.h"
#include "gameNetInterface.h"
#include "gameType.h"
#include "LevelSource.h"

#include "SoundSystemEnums.h"
#include "GameRecorder.h"

#ifndef ZAP_DEDICATED
#   include "ClientGame.h"
#   include "GameRecorderPlayback.h"
#   include "UIManager.h"
#endif

#include "Colors.h"
#include "Md5Utils.h"
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
   mDataBufferLevelGen = NULL;

   mWrongPasswordCount = 0;

   mVoiceChatEnabled = true;

   mPackUnpackShipEnergyMeter = false;

   resetConnectionStatus();
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

ServerGame *GameConnection::getServerGame()
{
   return mServerGame;
}

// Clears/initializes some things between levels
void GameConnection::resetConnectionStatus()
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
TNL_IMPLEMENT_RPC(GameConnection, s2cPlayerSpawnDelayed, (U8 waitTimeInOneTenthsSeconds), (waitTimeInOneTenthsSeconds), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   getClientInfo()->setReturnToGameTimer(waitTimeInOneTenthsSeconds * 100);

   getClientInfo()->setSpawnDelayed(true);
   mClientGame->setSpawnDelayed(true);
#endif
}


TNL_IMPLEMENT_RPC(GameConnection, s2cPlayerSpawnUndelayed, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   getClientInfo()->setSpawnDelayed(false);
   getClientInfo()->setShowLevelUpMessage(NONE);
   mClientGame->setSpawnDelayed(false);
#endif
}


// User has pressed a key or taken some action to undelay their spawn, or a msg has been reieved from server to undelay this client
// Server only, got here from c2sPlayerSpawnUndelayed(), or maybe when returnToGameTimer went off after being set here
void GameConnection::undelaySpawn()
{
    FullClientInfo *clientInfo = static_cast<FullClientInfo *>(getClientInfo());

   // Already spawn undelayed, ignore command
   if(!clientInfo->isSpawnDelayed())
      return;

   resetTimeSinceLastMove();

   if(mServerGame->isOrIsAboutToBeSuspended())
   {
      clientInfo->setReturnToGameTimer(0);    // No penalties when game is suspended
      clientInfo->requireReturnToGameTimer(false);
   }

   // Check if there is a penalty being applied to client (e.g. there is a 5 sec penalty for using the /idle command).
   // If so, start the timer clear the penalty flag, and leave.  We'll be back here again after the timer goes off.
   else if(clientInfo->hasReturnToGamePenalty())
   {
      clientInfo->setReturnToGameTimer(ClientInfo::SPAWN_UNDELAY_TIMER_DELAY);
      clientInfo->requireReturnToGameTimer(false);
      s2cPlayerSpawnDelayed(ClientInfo::SPAWN_UNDELAY_TIMER_DELAY / 100);
      return;
   }

   if(!clientInfo->getReturnToGameTime())
   {
      clientInfo->setSpawnDelayed(false);       // ClientInfo here is a FullClientInfo
      mServerGame->getGameType()->spawnShip(clientInfo);
      mServerGame->unsuspendIfActivePlayers(); // should be done after setSpawnDelayed(false)
   }
}


// Client has just woken up and is ready to play.  They have requested to be undelayed.
TNL_IMPLEMENT_RPC(GameConnection, c2sPlayerSpawnUndelayed, (), (), NetClassGroupGameMask, RPCGuaranteed, RPCDirClientToServer, 0)
{
   undelaySpawn();
}


// Client requests that the server to spawn delay them... only called from /idle command or when player levels up
TNL_IMPLEMENT_RPC(GameConnection, c2sPlayerRequestSpawnDelayed, (bool incursPenalty), (incursPenalty), 
                  NetClassGroupGameMask, RPCGuaranteed, RPCDirClientToServer, 0)
{
   ClientInfo *clientInfo = getClientInfo();
   
   // If we've just died, this will keep a second copy of ourselves from appearing
   clientInfo->respawnTimer.clear();

   // Now suicide!
   Ship *ship = clientInfo->getShip();
   if(ship)
   {
      if(incursPenalty)
         static_cast<FullClientInfo *>(clientInfo)->requireReturnToGameTimer(true);   // Client will have to wait to rejoin the game

      ship->kill();
   }

   clientInfo->setSpawnDelayed(true);           
   mServerGame->suspendIfNoActivePlayers(true);
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
      s2cDisplayErrorMessage("!!! Getmap command is disabled on this server");
      return;
   }


   string filename = mServerGame->getCurrentLevelFileName();
   filename = strictjoindir(mSettings->getFolderManager()->levelDir, filename);
   if(!TransferLevelFile(filename.c_str()))
      s2cDisplayErrorMessage("!!! Server Error, unable to download");
   return;
}


const U32 maxDataBufferSize = 1024*1024*8;  // 8 MB



void GameConnection::submitPassword(const char *password)
{
   string encrypted = Md5::getSaltedHashFromString(password);
   c2sSubmitPassword(encrypted.c_str());

   mLastEnteredPassword = password;

   setGotPermissionsReply(false);
   setWaitingForPermissionsReply(true);      // Means we'll show a reply from the server
}


TNL_IMPLEMENT_RPC(GameConnection, s2rSetSuspendGame, (bool isSuspend), (isSuspend),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   if(!isInitiator()) // server
   {
      if(mServerGame->clientCanSuspend(getClientInfo()))
      {
         if(isSuspend)
            mServerGame->suspendGame(this);
         else
            mServerGame->unsuspendGame(true);
      }
      else
         s2cDisplayErrorMessage("!!! Need admin");
   }
#ifndef ZAP_DEDICATED
   else // client
      mClientGame->setGameSuspended_FromServerMessage(isSuspend);
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
   prepareReplay();
   mClientGame->gotEngineerResponseEvent(EngineerResponseEvent(event.value));
#endif
}


TNL_IMPLEMENT_RPC(GameConnection, s2cDisableWeaponsAndModules, (bool disable), (disable),
                  NetClassGroupGameMask, RPCGuaranteed, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   // For whatever reason mClientInfo here isn't what is grabbed in the Ship:: class
   ClientInfo *clientInfo = mClientGame->findClientInfo(mClientInfo->getName()); // But this could be NULL on level change/restart
   if(!clientInfo)
      clientInfo = mClientInfo;
   TNLAssert(clientInfo, "NULL ClientInfo");
   if(clientInfo)
      clientInfo->setShipSystemsDisabled(disable);
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
TNL_IMPLEMENT_RPC(GameConnection, s2cSetAuthenticated, (StringTableEntry name, bool isAuthenticated, Int<BADGE_COUNT> badges, U16 gamesPlayed), 
                                                       (name, isAuthenticated, badges, gamesPlayed), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   ClientInfo *clientInfo = mClientGame->findClientInfo(name);

   if(clientInfo)
      clientInfo->setAuthenticated(isAuthenticated, badges, gamesPlayed);
   //else
      // This can happen if we're hosting locally when we first join the game.  Not sure why, but it seems harmless...
#endif
}


TNL_IMPLEMENT_RPC(GameConnection, c2sSubmitPassword, (StringPtr pass), (pass), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   string ownerPW = mSettings->getOwnerPassword();
   string adminPW = mSettings->getAdminPassword();
   string levChangePW = mSettings->getLevelChangePassword();

   GameType *gameType = mServerGame->getGameType();

   if(!mClientInfo->isOwner() && ownerPW != "" && !strcmp(Md5::getSaltedHashFromString(ownerPW).c_str(), pass))
   {
      logprintf(LogConsumer::ServerFilter, "User [%s] granted owner permissions", mClientInfo->getName().getString());
      mWrongPasswordCount = 0;

      // Send level list if not already LevelChanger
      if(!mClientInfo->isLevelChanger())
         sendLevelList();

      mClientInfo->setRole(ClientInfo::RoleOwner);
      s2cSetRole(ClientInfo::RoleOwner, true);                    // Tell client they have been granted access

      if(mSettings->getIniSettings()->allowAdminMapUpload)
      {
         mSendableFlags |= ServerFlagAllowUpload;                 // Enable level uploads
         s2rSendableFlags(mSendableFlags);
      }

      GameType *gameType = mServerGame->getGameType();

      // Announce admin access was granted
      if(gameType)
         gameType->s2cClientChangedRoles(mClientInfo->getName(), ClientInfo::RoleOwner);
   }

   // If admin password is blank, no one can get admin permissions except the local host, if there is one...
   else if(!mClientInfo->isAdmin() && adminPW != "" && !strcmp(Md5::getSaltedHashFromString(adminPW).c_str(), pass))
   {
      logprintf(LogConsumer::ServerFilter, "User [%s] granted admin permissions", mClientInfo->getName().getString());
      mWrongPasswordCount = 0;

      if(!mClientInfo->isLevelChanger())
         sendLevelList();
      
      mClientInfo->setRole(ClientInfo::RoleAdmin);               // Enter admin PW and...
      s2cSetRole(ClientInfo::RoleAdmin, true);                   // Tell client they have been granted access

      if(mSettings->getIniSettings()->allowAdminMapUpload)
      {
         mSendableFlags |= ServerFlagAllowUpload;                 // Enable level uploads
         s2rSendableFlags(mSendableFlags);
      }

      // Announce change to world
      if(gameType)
         gameType->s2cClientChangedRoles(mClientInfo->getName(), ClientInfo::RoleAdmin);
   }

   // If level change password is blank, it should already been granted to all clients
   else if(!mClientInfo->isLevelChanger() && !strcmp(Md5::getSaltedHashFromString(levChangePW).c_str(), pass)) 
   {
      logprintf(LogConsumer::ServerFilter, "User [%s] granted level change permissions", mClientInfo->getName().getString());
      mWrongPasswordCount = 0;

      mClientInfo->setRole(ClientInfo::RoleLevelChanger);
      s2cSetRole(ClientInfo::RoleLevelChanger, true);      // Tell client they have been granted access

      sendLevelList();                       // Send client the level list

      // Announce change to world
      if(gameType)
         gameType->s2cClientChangedRoles(mClientInfo->getName(), ClientInfo::RoleLevelChanger);
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


// Allow admins to change the passwords and other parameters on their systems
TNL_IMPLEMENT_RPC(GameConnection, c2sSetParam, 
                  (StringPtr param, RangedU32<0, GameConnection::ParamTypeCount> paramType), 
                  (param, paramType),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   ParamType type = (ParamType) paramType.value;

   if(!mClientInfo->isAdmin())   // Do nothing --> non-admins have no pull here.  Note that this should never happen; 
                                 // client should filter out non-admins before we get here, but we'll check anyway in 
                                 // case the client has been hacked.  But we have no obligation to notify client if 
                                 // this has happened.

   // Check for forbidden blank parameters -- the following commands require a value to be passed in param
   if( (type == AdminPassword || type == OwnerPassword || type == ServerName || type == ServerDescr || type == LevelDir) &&
                          !strcmp(param.getString(), ""))
      return;

   // Add a message to the server log
   if(type == DeleteLevel)
      logprintf(LogConsumer::ServerFilter, "User [%s] added level [%s] to server skip list", mClientInfo->getName().getString(), 
                                                mServerGame->getCurrentLevelFileName().c_str());
   else if(type == UndeleteLevel)
      logprintf(LogConsumer::ServerFilter, "User [%s] removed level [%s] from the server skip list", mClientInfo->getName().getString(), 
                                                mServerGame->getCurrentLevelFileName().c_str());   
   else
   {
      const char *paramName;
      switch(type)
      {
         case LevelChangePassword: paramName = "level change password"; break;
         case AdminPassword:       paramName = "admin password";        break;
         case OwnerPassword:       paramName = "owner password";        break;
         case ServerPassword:      paramName = "server password";       break;
         case ServerName:          paramName = "server name";           break;
         case ServerDescr:         paramName = "server description";    break;
         case LevelDir:            paramName = "leveldir param";        break;
         default:                  paramName = "unknown"; TNLAssert(false, "Fix unknown description"); break;
      }
      logprintf(LogConsumer::ServerFilter, "User [%s] %s to [%s]", mClientInfo->getName().getString(), 
                                                                   strcmp(param.getString(), "") ? "changed" : "cleared", paramName);
   }

   // Update our in-memory copies of the param, but do not save the new values to the INI
   if(type == LevelChangePassword)
      mSettings->setLevelChangePassword(param.getString(), false);
   
   else if(type == OwnerPassword && mClientInfo->isOwner())   // Need to be owner to change this
      mSettings->setOwnerPassword(param.getString(), false);

   else if(type == AdminPassword && mClientInfo->isOwner())   // Need to be owner to change this
      mSettings->setAdminPassword(param.getString(), false);
   
   else if(type == ServerPassword)
      mSettings->setServerPassword(param.getString(), false);
   
   else if(type == ServerName)
   {
      mSettings->setHostName(param.getString(), false);
      if(mServerGame->getConnectionToMaster())
         mServerGame->getConnectionToMaster()->s2mChangeName(StringTableEntry(param.getString()));   
   }
   else if(type == ServerDescr)
   {
      mSettings->setHostDescr(param.getString(), false);

      if(mServerGame->getConnectionToMaster())
         mServerGame->getConnectionToMaster()->s2mServerDescription(StringTableEntry(param.getString()));
   }

   // TODO: Add option here for type == Playlist

   else if(type == LevelDir)
   {
      FolderManager *folderManager = mSettings->getFolderManager();
      string folder = folderManager->resolveLevelDir(param.getString());

      if(folderManager->levelDir == folder)
      {
         s2cDisplayErrorMessage("!!! Specified folder is already the current level folder");
         return;
      }

      // Make sure the specified dir exists; hopefully it contains levels
      if(folder == "" || !fileExists(folder))
      {
         s2cDisplayErrorMessage("!!! Could not find specified folder");
         return;
      }

      Vector<string> levelList = LevelSource::findAllLevelFilesInFolder(folder);

      if(levelList.size() == 0)
      {
         s2cDisplayErrorMessage("!!! Specified folder contains no levels");
         return;
      }

      LevelSource *newLevelSource =  mSettings->chooseLevelSource(mServerGame);

      bool anyLoaded = newLevelSource->loadLevels(folderManager);    // Populates all our levelInfos by loading each file in turn

      if(!anyLoaded)
      {
         s2cDisplayErrorMessage("!!! Specified location contains no valid levels.  See server log for details.");
         return;
      }

      LevelSourcePtr levelSource = LevelSourcePtr(newLevelSource);

      // Folder contains some valid levels -- save it!
      folderManager->levelDir = folder;

      // Send the new list of levels to all levelchangers
      for(S32 i = 0; i < mServerGame->getClientCount(); i++)
      {
         ClientInfo *clientInfo = mServerGame->getClientInfo(i);
         GameConnection *conn = clientInfo->getConnection();

         if(clientInfo->isLevelChanger() && conn)
            conn->sendLevelList();
      }

      s2cDisplaySuccessMessage("Level folder changed");

   }  // end change leveldir


   else if(type == DeleteLevel)
      markCurrentLevelAsDeleted();

   else if(type == UndeleteLevel)
   {
      string level = undeleteMostRecentlyDeletedLevel();
      if(level == "")
         s2cDisplayErrorMessage("!!! No levels on delete list");
      else
      {
         Vector<StringTableEntry> e;
         e.push_back(level);

         s2cDisplayMessageE(ColorSuccess, SFXNone, "\"%e0\" removed from skip list", e);
      }

      return;
   }


   if(type != DeleteLevel && type != UndeleteLevel && type != LevelDir)
   {
      const char *paramName;
      switch(type)
      {
         case LevelChangePassword: paramName = "LevelChangePassword"; break;
         case AdminPassword:       paramName = "AdminPassword";       break;
         case OwnerPassword:       paramName = "OwnerPassword";       break;
         case ServerPassword:      paramName = "ServerPassword";      break;
         case ServerName:          paramName = "ServerName";          break;
         case ServerDescr:         paramName = "ServerDescription";   break;
         default:                  paramName = NULL; TNLAssert(false, "Fix unknown parameter to save"); break;
      }

      if(paramName != NULL)
      {
         // Update the INI file
         GameSettings::iniFile.SetValue("Host", paramName, param.getString(), true);
         GameSettings::iniFile.WriteFile();    // Save new INI settings to disk
      }
   }

   // Some messages we might show the user... should these just be inserted directly below?   Yes.  TODO: <== do that!
   static StringTableEntry levelPassChanged     = "Level change password changed";
   static StringTableEntry levelPassCleared     = "Level change password cleared -- anyone can change levels";
   static StringTableEntry adminPassChanged     = "Admin password changed";
   static StringTableEntry ownerPassChanged     = "Owner password changed";
   static StringTableEntry serverPassChanged    = "Server password changed -- only players with the password can connect";
   static StringTableEntry serverPassCleared    = "Server password cleared -- anyone can connect";
   static StringTableEntry serverNameChanged    = "Server name changed";
   static StringTableEntry serverDescrChanged   = "Server description changed";
   static StringTableEntry serverLevelDeleted   = "Level added to skip list; level will stay in rotation until server restarted";

   // Pick out just the right message
   StringTableEntry msg;

   if(type == LevelChangePassword)
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
               clientInfo->setRole(ClientInfo::RoleLevelChanger);
               if(conn)
               {
                  conn->sendLevelList();
                  conn->s2cSetRole(ClientInfo::RoleLevelChanger, false);     // Silently
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
               clientInfo->setRole(ClientInfo::RoleNone);
               if(conn)
                  conn->s2cSetRole(ClientInfo::RoleNone, false);

               // Announce the change
               mServerGame->getGameType()->s2cClientChangedRoles(clientInfo->getName(), ClientInfo::RoleNone);
            }
         }
      }
   }
   else if(type == AdminPassword)
   {
      msg = adminPassChanged;

      // Revoke all admin permissions upon password change (except Owner)
      if(mClientInfo->isOwner())
      {
         for(S32 i = 0; i < mServerGame->getClientCount(); i++)
         {
            ClientInfo *clientInfo = mServerGame->getClientInfo(i);
            GameConnection *conn = clientInfo->getConnection();

            if(clientInfo->isAdmin() && (!clientInfo->isOwner()))
            {
               clientInfo->setRole(ClientInfo::RoleNone);
               if(conn)
                  conn->s2cSetRole(ClientInfo::RoleNone, false);

               // Announce the change
               mServerGame->getGameType()->s2cClientChangedRoles(clientInfo->getName(), ClientInfo::RoleNone);
            }
         }
      }
   }
   else if(type == OwnerPassword)
      msg = ownerPassChanged;
   else if(type == ServerPassword)
      msg = strcmp(param.getString(), "") ? serverPassChanged : serverPassCleared;
   else if(type == ServerName)
   {
      msg = serverNameChanged;

      // If we've changed the server name, notify all the clients
      for(S32 i = 0; i < mServerGame->getClientCount(); i++)
         if(mServerGame->getClientInfo(i)->getConnection())
            mServerGame->getClientInfo(i)->getConnection()->s2cSetServerName(mSettings->getHostName());
   }
   else if(type == ServerDescr)
      msg = serverDescrChanged;
   else if(type == DeleteLevel)
      msg = serverLevelDeleted;

   s2cDisplaySuccessMessage(msg);      // Notify user their bidding has been done
}


void GameConnection::markCurrentLevelAsDeleted()
{
   string filename = mServerGame->getCurrentLevelFileName();

   // Avoid duplicates on skip list
   if(mSettings->isLevelOnSkipList(filename))
      return;

   // Add level to our skip list.  Deleting it from the active list of levels is more of a challenge...
   mSettings->addLevelToSkipList(filename);
}


string GameConnection::undeleteMostRecentlyDeletedLevel()
{
   Vector<string> *skipList = mSettings->getLevelSkipList();

   if(skipList->size() == 0)     // No deleted items to undelete
      return "";

   string name = skipList->last();
   skipList->erase(skipList->size() - 1);

   mSettings->saveSkipList();

   return name;
}


// This gets called under two circumstances; when it's a new game, or when the server's name is changed by an admin
TNL_IMPLEMENT_RPC(GameConnection, s2cSetServerName, (StringTableEntry name), (name),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
   setServerName(name);

   // If we know the level change password, apply for permissions if we don't already have them
   if(!mClientInfo->isLevelChanger())
   {
      string levelChangePassword = GameSettings::iniFile.GetValue("SavedLevelChangePasswords", getServerName());
      if(levelChangePassword != "")
      {
         c2sSubmitPassword(Md5::getSaltedHashFromString(levelChangePassword).c_str());
         setWaitingForPermissionsReply(false);     // Want to return silently
      }
   }

   // If we know the admin password, apply for permissions if we don't already have them
   if(!mClientInfo->isAdmin())
   {
      string adminPassword = GameSettings::iniFile.GetValue("SavedAdminPasswords", getServerName());
      if(adminPassword != "")
      {
         c2sSubmitPassword(Md5::getSaltedHashFromString(adminPassword).c_str());
         setWaitingForPermissionsReply(false);     // Want to return silently
      }
   }

   // If we know the owner password, apply for permissions if we don't already have them
   if(!mClientInfo->isOwner())
   {
      string ownerPassword = GameSettings::iniFile.GetValue("SavedOwnerPasswords", getServerName());
      if(ownerPassword != "")
      {
         c2sSubmitPassword(Md5::getSaltedHashFromString(ownerPassword).c_str());
         setWaitingForPermissionsReply(false);     // Want to return silently
      }
   }
}


TNL_IMPLEMENT_RPC(GameConnection, s2cSetRole, (RangedU32<0,ClientInfo::MaxRoles> role, bool notify), (role, notify),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   ClientInfo *clientInfo = mClientGame->getClientInfo();

   ClientInfo::ClientRole newRole = (ClientInfo::ClientRole) role.value;
   ClientInfo::ClientRole currentRole = clientInfo->getRole();

   // Role has stayed the same.  Do nothing
   if(newRole == currentRole)
      return;

   // We were demoted
   bool lostRole = newRole < currentRole;

   // Handle password saving in the INI
   static string ownerKey = "SavedOwnerPasswords";
   static string adminKey = "SavedAdminPasswords";
   static string levelChangeKey = "SavedLevelChangePasswords";

   // If we've got a new role, save our password
   if(newRole != ClientInfo::RoleNone && mLastEnteredPassword != "")
   {
      if(newRole == ClientInfo::RoleOwner)
         GameSettings::iniFile.SetValue(ownerKey, getServerName(), mLastEnteredPassword, true);
      else if(newRole == ClientInfo::RoleAdmin)
         GameSettings::iniFile.SetValue(adminKey, getServerName(), mLastEnteredPassword, true);
      else if(newRole == ClientInfo::RoleLevelChanger)
         GameSettings::iniFile.SetValue(levelChangeKey, getServerName(), mLastEnteredPassword, true);

      mLastEnteredPassword = "";
   }

   // If we're being demoted, get rid of our currently saved password
   if(lostRole)
   {
      if(currentRole == ClientInfo::RoleOwner)
         GameSettings::iniFile.deleteKey(ownerKey, getServerName());
      else if(currentRole == ClientInfo::RoleAdmin)
         GameSettings::iniFile.deleteKey(adminKey, getServerName());
      else if(currentRole == ClientInfo::RoleLevelChanger)
         GameSettings::iniFile.deleteKey(levelChangeKey, getServerName());
   }

   // Extra instructions upon demotion
   if(lostRole)
   {
      if(currentRole == ClientInfo::RoleLevelChanger)
         mClientGame->displayCmdChatMessage(
            "An admin has changed the level change password; you must enter the new password to change levels.");
      else if(currentRole == ClientInfo::RoleAdmin)
         mClientGame->displayCmdChatMessage(
            "An owner has changed the admin password; you must enter the new password to become an admin.");
   }

   // Finally set our new role
   clientInfo->setRole(newRole);

   // Notify UI of permissions update
   if(newRole != ClientInfo::RoleNone)
   {
      setGotPermissionsReply(true);

      // If we're not waiting, don't show us a message.  Supresses superflous messages on startup.
      if(waitingForPermissionsReply() && notify)
         mClientGame->gotPermissionsReply(newRole);
   }
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


// 18 bits == 262144, enough for -100000 to 100000  (Ship::EnergyMax is 100000)
// This way we can credit negative energy as well
TNL_IMPLEMENT_RPC(GameConnection, s2cCreditEnergy, (SignedInt<18> energy), (energy), NetClassGroupGameMask, RPCGuaranteed, RPCDirServerToClient, 0)
{
   Ship *ship = static_cast<Ship *>(getControlObject());

   if(ship)
   {
      prepareReplay();
      ship->creditEnergy(energy);
   }
}


TNL_IMPLEMENT_RPC(GameConnection, s2cSetFastRechargeTime, (U32 time), (time), NetClassGroupGameMask, RPCGuaranteed, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   U32 interval = /*getClientGame()->getGameType()->getTotalGamePlayedInMs() -*/ time;
   Ship *ship = static_cast<Ship *>(getControlObject());
   if(ship)
   {
      ship->resetFastRecharge();
      // manually set the time remaining, but keep the same period
      ship->mFastRechargeTimer.reset(interval, ship->mFastRechargeTimer.getPeriod());
   }
#endif
}


// Client has changed his loadout configuration.  This gets run on the server as soon as the loadout is entered.
TNL_IMPLEMENT_RPC(GameConnection, c2sRequestLoadout, (Vector<U8> loadout), (loadout), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   LoadoutTracker req(loadout);
   getClientInfo()->requestLoadout(req);
}


// TODO: make this an xmacro
Color colors[] =
{
   Colors::white,          // ColorWhite
   Colors::cmdChatColor,   // ColorRed  
   Colors::green,          // ColorGreen
   Colors::blue,           // ColorBlue
   Colors::cyan,           // ColorAqua
   Colors::yellow,         // ColorYellow
   Color(0.6f, 1, 0.8f),   // ColorNuclearGreen, SuccessColor
};


void GameConnection::displayMessage(U32 colorIndex, U32 sfxEnum, const char *message)
{
#ifndef ZAP_DEDICATED
   mClientGame->displayMessage(colors[colorIndex], "%s", message);
   if(sfxEnum != SFXNone)
      mClientGame->playSoundEffect(sfxEnum);
#endif
}


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
                  (RangedU32<0, NumSFXBuffers> sfx, S32 team, StringTableEntry formatString, Vector<StringTableEntry> e, Point scorePos),
                  (sfx, team, formatString, e, scorePos),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   if(formatString.getString()[0] != 0)
      displayMessageE(GameConnection::ColorNuclearGreen, sfx, formatString, e);

   GameType* gt = mClientGame->getGameType();
   if(gt)
   {
      gt->majorScoringEventOcurred(team);
      mClientGame->emitTextEffect("Touchdown!", *gt->getTeamColor(team), scorePos);
   }
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


TNL_IMPLEMENT_RPC(GameConnection, s2cDisplaySuccessMessage,
                  (StringTableEntry formatString),
                  (formatString),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
#ifndef ZAP_DEDICATED
   mClientGame->displaySuccessMessage(formatString.getString());
#endif
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
         mServerGame->getPlayerCount() > 1 && mServerGame->voteStart(mClientInfo, ServerGame::VoteLevelChange, newLevelIndex))
      return;

   // Don't let spawn delay kick in for caller.  This prevents a race condition with spawn undelay and becoming unbusy
   resetTimeSinceLastMove();

   bool restart = false;

   if(isRelative)
      newLevelIndex = (mServerGame->getCurrentLevelIndex() + newLevelIndex ) % mServerGame->getLevelCount();
   else if(newLevelIndex == REPLAY_LEVEL)
      restart = true;

   StringTableEntry msg( restart ? "%e0 restarted the current level." : "%e0 changed the level to %e1." );
   Vector<StringTableEntry> e;
   e.push_back(mClientInfo->getName()); 

   // resolve the index (which could be a meta-index) to an absolute index
   newLevelIndex = mServerGame->getAbsoluteLevelIndex(newLevelIndex);

   mServerGame->cycleLevel(newLevelIndex);
   if(!restart)
      e.push_back(mServerGame->getLevelNameFromIndex(newLevelIndex));

   mServerGame->getGameType()->broadcastMessage(ColorYellow, SFXNone, msg, e);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sShowNextLevel, (), (), NetClassGroupGameMask, RPCGuaranteed, RPCDirClientToServer, 0)
{
   Vector<StringTableEntry> e;
   e.push_back(mServerGame->getLevelNameFromIndex(NEXT_LEVEL)); 

   s2cDisplayMessageE(ColorInfo, SFXNone, "Next level will be \"%e0\"", e);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestShutdown, (U16 time, StringPtr reason), (time, reason), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   if(!mClientInfo->isOwner())
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

   mIsBusy = isBusy;

   for(S32 i = 0; i < mServerGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mServerGame->getClientInfo(i);

      if(clientInfo->isRobot())
         continue;

      // No need to notify player that they themselves are busy... is there?
      if(clientInfo == mClientInfo)
         continue;

      clientInfo->getConnection()->s2cSetIsBusy(mClientInfo->getName(), isBusy);
   }

   mClientInfo->setIsBusy(isBusy);

   // If we're busy, force spawndelay timer to run out
   if(isBusy)
      addTimeSinceLastMove(SPAWN_DELAY_TIME - 2000);
   else
      resetTimeSinceLastMove();
}


TNL_IMPLEMENT_RPC(GameConnection, c2sSetServerAlertVolume, (S8 vol), (vol), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   //setServerAlertVolume(vol);
}


// Client connects to master after joining a game, authentication fails,
// then client has changed name to non-reserved, or entered password.
TNL_IMPLEMENT_RPC(GameConnection, c2sRenameClient, (StringTableEntry newName), (newName), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   StringTableEntry oldName = mClientInfo->getName();
   mClientInfo->setName("");     

   StringTableEntry uniqueName = mServerGame->makeUnique(newName.getString()).c_str();    // Make sure new name is unique
   mClientInfo->setName(oldName);         // Restore name to properly get it updated to clients
   setClientNameNonUnique(newName);       // For correct authentication
   
   mClientInfo->setAuthenticated(false, NO_BADGES, 0);            // Prevents name from being underlined
   mClientInfo->setNeedToCheckAuthenticationWithMaster(false);    // Do not inquire with master

   if(oldName != uniqueName)              // Did the name actually change?
      mServerGame->getGameType()->updateClientChangedName(mClientInfo, uniqueName);
}


TNL_IMPLEMENT_RPC(GameConnection, s2rSendableFlags, (U8 flags), (flags), NetClassGroupGameMask, RPCGuaranteed, RPCDirAny, 0)
{
   mSendableFlags = flags;
}


void GameConnection::ReceivedLevelFile(const U8 *leveldata, U32 levelsize, const U8 *levelgendata, U32 levelgensize)
{
   bool isServer = !isInitiator();

   // Only server runs this part of code
   FolderManager *folderManager = mSettings->getFolderManager();

   if(isServer && levelgensize != 0 && !mSettings->getIniSettings()->allowLevelgenUpload)
   {
      s2cDisplayErrorMessage("!!! Server does not allow levelgen upload");
      return;
   }

   LevelInfo levelInfo;
   LevelSource::getLevelInfoFromCodeChunk(string((char *)leveldata, levelsize), levelInfo);

   if(isServer && levelgensize == 0 && levelInfo.mScriptFileName.length() != 0)
   {
      if(isServer)
      {
         s2cDisplayErrorMessage("!!! LevelGen not uploaded, does Script name end with .levelgen ?"); // for existing client release 019
         return;
      }
      else
         s2cDisplayErrorMessage_remote("!!! Levelgen not downloaded, downloaded level needs levelgen");
   }

   string titleName = makeFilenameFromString(levelInfo.mLevelName.getString());
   string filename = (isServer ? UploadPrefix : DownloadPrefix) + titleName + ".level";
   string filenameLevelgen = (isServer ? UploadPrefix : DownloadPrefix) + titleName + ".levelgen";

   string fullFilename = strictjoindir(folderManager->levelDir, filename);
   levelInfo.filename = filename;
   levelInfo.folder   = folderManager->levelDir;

   FILE *f = fopen(fullFilename.c_str(), "wb");
   if(f)
   {
      if(levelgensize != 0)
      {
         // Modify the "Script" line so it points to new uploaded script filename
         U32 c = 0;
         bool foundscript = false;
         // First, find a line that says "Script"
         while(c < levelsize - 10)
         {  // Make sure the previous char is a new line (c < 32), and compare "Script " with only 7 chars
            if((c == 0 || leveldata[c-1] < 32) && strncmp("Script ", (char*)&leveldata[c], 7) == 0)
            {
               foundscript = true;
               break;
            }
            c++;
         }
         if(foundscript)
         {  // Found a line, write a modified Script filename we will soon write to.
            // Could use more work here to better handle spaces and quotes
            if(c != 0)
               fwrite(leveldata, 1, c, f);
            fwrite("Script ", 1, 7, f); // Using quotation marks to handle filename with spaces
            fwrite(filenameLevelgen.c_str(), 1, strlen(filenameLevelgen.c_str()), f);
            //fputc('"', f);  // End with quotation mark
            c += 6;
            while(c < levelsize && leveldata[c] == 32) // First one or more spaces.
               c++;
            bool isInQuote = false; // to ignore spaces while in quotoation marks
            while(c < levelsize && leveldata[c] >= 32 && (isInQuote || leveldata[c] != 32)) // Go to end of second arg
            {
               isInQuote = isInQuote != (leveldata[c] == '"');
               c++;
            }
         }
         else
            c = 0;
         if(c < levelsize)
            fwrite(&leveldata[c], 1, levelsize - c, f); // Write the rest of level
      }
      else
         fwrite(leveldata, 1, levelsize, f);
      fclose(f);

      if(isServer)
         logprintf(LogConsumer::ServerFilter, "%s %s Uploaded %s", getNetAddressString(), mClientInfo->getName().getString(), filename.c_str());
      else
         // _remote works so well when we are already client and not sending through network, avoids messy #define ZAP_DEDICATED
         s2cDisplaySuccessMessage_remote("Level download completed");

      if(levelgensize != 0)  // next, write levelgen if we have one.
      {
         string str1 = strictjoindir(folderManager->levelDir, filenameLevelgen);
         f = fopen(str1.c_str(), "wb");
         if(f)
         {
            fwrite(levelgendata, 1, levelgensize, f);
            fclose(f);
         }
         else if(isServer)
            s2cDisplayErrorMessage("!!! Levelgen Upload failed -- server can't write file");
         else
            s2cDisplayErrorMessage_remote("!!! Unable to save levelgen");
      }

      if(isServer)
      {
         S32 index = mServerGame->addLevel(levelInfo);
         c2sRequestLevelChange_remote(index, false);  // Might as well switch to it after done with upload
      }
   }
   else if(isServer)
      s2cDisplayErrorMessage("!!! Upload failed -- server can't write file");
   else
      s2cDisplayErrorMessage_remote("!!! Unable to save level");
}


void GameConnection::ReceivedRecordedGameplay(const U8 *filedata, U32 filedatasize)
{
   if(!isInitiator())
   {
      TNLAssert(false, "ReceivedRecordedGameplay is Client only");
      return;
   }

#ifndef ZAP_DEDICATED

   const string &dir = mClientGame->getSettings()->getFolderManager()->recordDir;
   string filename = string(mServerName.getString()) + "_" + mFileName;
   filename = joindir(dir, makeFilenameFromString(filename.c_str(), true));
   FILE *f = fopen(filename.c_str(), "wb");
   if(f)
   {
      fwrite(f, 1, filedatasize, f);
      fclose(f);
   }
   else
      s2cDisplayErrorMessage_remote("!!! Unable to save");

#endif
}


TNL_IMPLEMENT_RPC(GameConnection, s2rSendDataParts, (U8 type, ByteBufferPtr data), (type, data), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   // Abort early if user can't upload
   if(!isInitiator() && !(mSettings->getIniSettings()->allowMapUpload || (mSettings->getIniSettings()->allowAdminMapUpload && mClientInfo->isAdmin())))
      return;

   ByteBuffer *&dataBuffer = (type & 2 ? mDataBufferLevelGen : mDataBuffer);

   if(dataBuffer)
   {
      if(dataBuffer->getBufferSize() < maxDataBufferSize || isInitiator())  // Limit memory consumption (no limit on clients due to how big game recordings can be)
         dataBuffer->appendBuffer(*data.getPointer());
   }
   else
   {
      dataBuffer = new ByteBuffer(*data.getPointer());
      dataBuffer->takeOwnership();
   }

   if((type & TransmissionDone) && mDataBuffer && mDataBuffer->getBufferSize() != 0)
   {
      if(type & TransmissionRecordedGame)
         ReceivedRecordedGameplay(mDataBuffer->getBuffer(), mDataBuffer->getBufferSize());
      else if(mDataBufferLevelGen)
         ReceivedLevelFile(mDataBuffer->getBuffer(), mDataBuffer->getBufferSize(), mDataBufferLevelGen->getBuffer(), mDataBufferLevelGen->getBufferSize());
      else
         ReceivedLevelFile(mDataBuffer->getBuffer(), mDataBuffer->getBufferSize(), NULL, 0);
   }

   if(type & TransmissionDone)
   {
      if(mDataBuffer)
         delete mDataBuffer;
      mDataBuffer = NULL;
      if(mDataBufferLevelGen)
         delete mDataBufferLevelGen;
      mDataBufferLevelGen = NULL;
   }
}


TNL_IMPLEMENT_RPC(GameConnection, s2rTransferFileSize, (U32 size), (size), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   mReceiveTotalSize = size;
}

static S32 QSORT_CALLBACK numberAlphaSort(string *a, string *b)
{
   int aNum = atoi(a->c_str());
   int bNum = atoi(b->c_str());
   if(aNum != bNum)
      return bNum - aNum;

   return stricmp(a->c_str(), b->c_str());        // Is there something analagous to stricmp for strings (as opposed to c_strs)?
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestRecordedGameplay, (StringPtr file), (file), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 2)
{
   if(file.getString()[0] != 0)
   {
      string filePath = joindir(mServerGame->getSettings()->getFolderManager()->recordDir, file.getString());
      TransferRecordedGameplay(filePath.c_str());
   }
   else
   {
      Vector<string> levels;
      const string &dir = mServerGame->getSettings()->getFolderManager()->recordDir;
      getFilesFromFolder(dir, levels);
		GameRecorderServer *g = mServerGame->getGameRecorder();
		if(g)
		{
			for(S32 i = 0; i < levels.size(); i++)
				if(levels[i] == g->mFileName)
				{
					levels.erase(i);
					break;
				}
		}
		if(levels.size())
			levels.sort(numberAlphaSort);
      s2cListRecordedGameplays(levels);
   }
}
TNL_IMPLEMENT_RPC(GameConnection, s2cListRecordedGameplays, (Vector<string> files), (files), 
                  NetClassGroupGameMask, RPCGuaranteedOrderedBigData, RPCDirServerToClient, 2)
{
#ifndef ZAP_DEDICATED
   mClientGame->getUIManager()->getUI<PlaybackServerDownloadUserInterface>()->receivedLevelList(files);
#endif
}

TNL_IMPLEMENT_RPC(GameConnection, s2cSetFilename, (string filename), (filename), 
                  NetClassGroupGameMask, RPCGuaranteedOrderedBigData, RPCDirServerToClient, 2)
{
   mFileName = filename;
}

bool GameConnection::TransferLevelFile(const char *filename)
{
   BitStream s;
   const U32 partsSize = 512;   // max 1023, limited by ByteBufferSizeBitSize value of 10

   FILE *f = fopen(filename, "rb");

   if(f)
   {
      U32 size = partsSize;
      const U32 DATAARRAYSIZE = 8192;
      U8 *data = new U8[DATAARRAYSIZE];
      U32 totalTransferSize = 0;
      size = fread(data, 1, DATAARRAYSIZE, f);

      mPendingTransferData.resize(0);

      if(size <= 0 || size > DATAARRAYSIZE)
      {
         fclose(f);
         delete[] data;
         return false;
      }

      LevelInfo levelInfo;
      LevelSource::getLevelInfoFromCodeChunk(string((char*)data, size), levelInfo);

      for(U32 i = 0; i < size; i += partsSize)
      {
         ByteBuffer *bytebuffer = new ByteBuffer(&data[i], min(partsSize, size - i));
         bytebuffer->takeOwnership();
         mPendingTransferData.push_back(bytebuffer);
         totalTransferSize += bytebuffer->getBufferSize();
      }
      delete[] data;

      size = (size == DATAARRAYSIZE ? partsSize : 0);
      while(size == partsSize)
      {
         ByteBuffer *bytebuffer = new ByteBuffer(512);

         size = (U32)fread(bytebuffer->getBuffer(), 1, bytebuffer->getBufferSize(), f);

         if(size != partsSize)
            bytebuffer->resize(size);

         mPendingTransferData.push_back(bytebuffer);
         totalTransferSize += size;
      }
      fclose(f);

      U32 pendingleveltransfer = mPendingTransferData.size();

      if(levelInfo.mScriptFileName.c_str()[0] != 0)
      {
         FolderManager *folderManager = mSettings->getFolderManager();
         string filename1 = strictjoindir(folderManager->levelDir, levelInfo.mScriptFileName);
         f = fopen(filename1.c_str(), "rb");

         if(!f)
         {
            filename1 += ".levelgen"; // Script line missing ".levelgen"?
            f = fopen(filename1.c_str(), "rb");
            if(!f)
            {
               if(isInitiator()) // isClient
               {
                  s2cDisplayErrorMessage_remote("Unable to find LevelGen");

                  // Vector deleteAndClear doesn't work for SafePtr on OSX, so we do it the
                  // old-fashioned way
                  for(S32 i = 0; i < mPendingTransferData.size(); i++)
                     delete mPendingTransferData[i].getPointer();
                  mPendingTransferData.clear();

                  return false;
               }
            }
         }
      }
      else
         f = NULL;

      if(f)
      {
         U32 size = partsSize;
         while(size == partsSize)
         {
            ByteBuffer *bytebuffer = new ByteBuffer(512);

            size = (U32)fread(bytebuffer->getBuffer(), 1, bytebuffer->getBufferSize(), f);

            if(size != partsSize)
               bytebuffer->resize(size);

            mPendingTransferData.push_back(bytebuffer);
            totalTransferSize += size;
         }
         fclose(f);
      }

      s2rTransferFileSize(totalTransferSize);
      for(U32 i=0; i < pendingleveltransfer; i++)
         s2rSendDataParts(TransmissionLevelFile, ByteBufferPtr(mPendingTransferData[i]));
      for(U32 i=pendingleveltransfer; i < U32(mPendingTransferData.size()); i++)
         s2rSendDataParts(TransmissionLevelGenFile, ByteBufferPtr(mPendingTransferData[i]));

      s2rSendDataParts(TransmissionDone, ByteBufferPtr(new ByteBuffer(0)));
      return true;
   }

   return false;
}

bool GameConnection::TransferRecordedGameplay(const char *filename)
{
   BitStream s;
   const U32 partsSize = 512;   // max 1023, limited by ByteBufferSizeBitSize value of 10

   FILE *f = fopen(filename, "rb");

   if(f)
   {
      mPendingTransferData.resize(0);
      U32 totalTransferSize = 0;

      U32 size = partsSize;
      while(size == partsSize)
      {
         ByteBuffer *bytebuffer = new ByteBuffer(512);

         size = (U32)fread(bytebuffer->getBuffer(), 1, bytebuffer->getBufferSize(), f);

         if(size != partsSize)
            bytebuffer->resize(size);

         mPendingTransferData.push_back(bytebuffer);
         totalTransferSize += size;
      }
      fclose(f);

      if(mPendingTransferData.size() == 0)
      {
         if(!isInitiator())
            s2cDisplayErrorMessage("Recorded file is empty");
         return false;
      }

      s2cSetFilename(filename);
      s2rTransferFileSize(totalTransferSize);
      for(U32 i=0; i < U32(mPendingTransferData.size()) - 1; i++)
         s2rSendDataParts(TransmissionRecordedGame, ByteBufferPtr(mPendingTransferData[i]));

      s2rSendDataParts(TransmissionRecordedGame | TransmissionDone, ByteBufferPtr(new ByteBuffer(0)));
      return true;
   }
   else
      if(!isInitiator())
         s2cDisplayErrorMessage("Unable to read recorded file");

   return false;
}





F32 GameConnection::getFileProgressMeter()
{
   if(mPendingTransferData.size())
   {
      // Sent data becomes NULL, which we can use to see the upload progress.
      S32 numberOfNull = 0;
      for(S32 i = 0; i < mPendingTransferData.size(); i++)
         if(mPendingTransferData[i].isNull())
            numberOfNull++;
      if(numberOfNull == mPendingTransferData.size())
         mPendingTransferData.resize(0); // Everything is now NULL, zero size the Vector.
      else
         return F32(numberOfNull+1) / (mPendingTransferData.size()+1);
   }
   if(mDataBuffer && mReceiveTotalSize != 0)
   {
      U32 size = mDataBuffer->getBufferSize();
      if(mDataBufferLevelGen)
         size += mDataBufferLevelGen->getBufferSize();
      return F32(size) / mReceiveTotalSize;
   }
   return 0;
}


TNL_IMPLEMENT_RPC(GameConnection, s2rVoiceChatEnable, (bool enable), (enable), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   mVoiceChatEnabled = enable;
}


static string serverPW;

// Send password, client's name, and version info to game server
void GameConnection::writeConnectRequest(BitStream *stream)
{
#ifndef ZAP_DEDICATED
   Parent::writeConnectRequest(stream);

   stream->write(CONNECT_VERSION);
   
   string lastServerName = mClientGame->getRequestedServerName();

   // If we're local, just use the password we already know because, you know, we're the server
   if(isLocalConnection())
      serverPW = mSettings->getServerPassword();

   // If we have a saved password for this server, use that
   else if(GameSettings::getServerPassword(lastServerName) != "")
      serverPW = GameSettings::getServerPassword(lastServerName); 

   // Otherwise, use whatever the user entered
   else 
      serverPW = mClientGame->getEnteredServerAccessPassword();

   // Write some info about the client... name, id, and verification status
   stream->writeString(Md5::getSaltedHashFromString(serverPW).c_str());
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
   mClientInfo = new FullClientInfo(mServerGame, this, "Remote Player", ClientInfo::ClassHuman);   // Deleted in destructor
   mSettings = mServerGame->getSettings();  // now that we got the server, set the settings.

   stream->read(&mConnectionVersion);

   stream->readString(buf);
   string serverPassword = mServerGame->getSettings()->getServerPassword();

   if(serverPassword != "" && stricmp(buf, Md5::getSaltedHashFromString(serverPassword).c_str()))
   {
      reason = ReasonNeedServerPassword;
      return false;
   }

   // Now read the player name, id, and verification status
   stream->readString(buf);
   std::size_t len = strlen(buf);

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
   for(std::size_t i = 0; i < len; i++)
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


// Client and Server
void GameConnection::updateTimers(U32 timeDelta)
{
   if(mAuthenticationTimer.update(timeDelta))
      requestAuthenticationVerificationFromMaster();

   if(mVoteTime <= timeDelta)
      mVoteTime = 0;
   else
      mVoteTime -= timeDelta;

   if(isInitiator())
      updateTimers_client(timeDelta);
   else
      updateTimers_server(timeDelta);
}


void GameConnection::updateTimers_client(U32 timeDelta)
{
   mClientInfo->updateReturnToGameTimer(timeDelta);   
}


void GameConnection::updateTimers_server(U32 timeDelta)
{
   addToTimeCredit(timeDelta);

   if(mClientInfo->updateReturnToGameTimer(timeDelta))     // Time to spawn a delayed player!
       undelaySpawn();     
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


// We've just established a local connection to a server running in the same process
void GameConnection::onLocalConnection()
{
   getClientInfo()->setRole(ClientInfo::RoleOwner);           // Set Owner role on server
   sendLevelList();

   s2cSetRole(ClientInfo::RoleOwner, false);                  // Set Owner role on the client
   setServerName(mServerGame->getSettings()->getHostName());  // Server name is whatever we've set locally

   // Tell local host if we're authenticated... no need to verify
   GameConnection *gc = static_cast<GameConnection *>(getRemoteConnectionObject());
   ClientInfo *clientInfo = gc->getClientInfo();
   getClientInfo()->setAuthenticated(clientInfo->isAuthenticated(), clientInfo->getBadges(),
                                     clientInfo->getGamesPlayed());  
}


bool GameConnection::lostContact()
{
   return isMovesFull() ||
      getTimeSinceLastPacketReceived() > (U32)TWO_SECONDS && mLastPacketRecvTime != 0;   // No contact for 2 secs?  That's bad!
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


// This gets run when we've connected to the game server
// See also void ClientGame::onGameUIActivated() for other game startup stuff
void GameConnection::onConnectionEstablished_client()
{
#ifndef ZAP_DEDICATED
   setConnectionSpeed(mClientGame->getSettings()->getIniSettings()->connectionSpeed);  // set speed depending on client
   setGhostFrom(false);
   setGhostTo(true);
   logprintf(LogConsumer::LogConnection, "%s - connected to server.", getNetAddressString());

   // This is a new connection, server is expecting the new client to not show idling message.
   getClientInfo()->setSpawnDelayed(false);

   string lastServerName = mClientGame->getRequestedServerName();

   // ServerPW is whatever we used to connect to the server.  If the server has no password, we can send any ol' junk
   // and it will be accepted; we have no way of knowing if the server password is blank aside from explicitly trying it.
   if(!isLocalConnection())
      GameSettings::saveServerPassword(lastServerName, serverPW);

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
   setConnectionSpeed(2);                 // High speed, most servers have sufficient bandwidth
   mServerGame->addClient(mClientInfo);   // This clientInfo was created by the server... it has no badge data yet
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
      mClientInfo->setRole(ClientInfo::RoleLevelChanger);
      s2cSetRole(ClientInfo::RoleLevelChanger, false);          // Tell client, but don't display notification
      sendLevelList();
   }

   const char *name =  mClientInfo->getName().getString();

   logprintf(LogConsumer::LogConnection, "%s - client \"%s\" connected.", getNetAddressString(), name);
   logprintf(LogConsumer::ServerFilter,  "%s [%s] joined [%s]", name, 
                                          isLocalConnection() ? "Local Connection" : getNetAddressString(), getTimeStamp().c_str());

   mSendableFlags = 0;
   if(mServerGame->getSettings()->getIniSettings()->allowMapUpload)
      mSendableFlags |= ServerFlagAllowUpload;
   if(mServerGame->getSettings()->getIniSettings()->enableGameRecording)
      mSendableFlags |= ServerFlagHasRecordedGameplayDownloads;

   s2rSendableFlags(mSendableFlags);

   // No team changing allowed
   if(!mServerGame->getSettings()->getIniSettings()->allowTeamChanging)
   {
      // Forever!
      mSwitchTimer.reset(U32_MAX, U32_MAX);
   }

   if(mServerGame->isSuspended())
      s2rSetSuspendGame(true);
}


// Established connection is terminated.  Compare to onConnectTerminate() below.
void GameConnection::onConnectionTerminated(NetConnection::TerminationReason reason, const char *reasonStr)
{
   TNLAssert(reason == NetConnection::ReasonSelfDisconnect || !isLocalConnection(), "Local connection should not be disconnected!");

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

//This is here to avoid kicking the hosting player in case of duplicated authenticated name player joins or similar.
void GameConnection::disconnect(TerminationReason reason, const char *reasonStr)
{
   if(reason == NetConnection::ReasonSelfDisconnect || !isLocalConnection())
      Parent::disconnect(reason, reasonStr);
   else
      (this->*(isInitiator() ? &GameConnection::s2cDisplayErrorMessage_remote : &GameConnection::s2cDisplayErrorMessage))("Can't kick hosting player");
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


// Gets run when game is just beginning, before objects are sent to client.
// Some keywords to help find this function again: start, onGameStart, onGameBegin
// Client only
void GameConnection::onStartGhosting()
{
   Parent::onStartGhosting();

#ifndef ZAP_DEDICATED
   mClientGame->onGameStarting();
#endif
}


// Gets run when game is really and truly over, after post-game scoreboard is displayed.  Over.
// Some keywords to help find this function again: level over, change level, game over, onGameOver
// Client only, isConnectionToServer() is always true
void GameConnection::onEndGhosting()
{
#ifndef ZAP_DEDICATED
   TNLAssert(isConnectionToServer() && mClientGame, "when else is this called?");
   
   Parent::onEndGhosting();
   mClientGame->onGameReallyAndTrulyOver();
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



};


