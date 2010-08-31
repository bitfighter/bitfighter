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
#include "game.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "config.h"        // For gIniSettings support
#include "IniFile.h"       // For CIniFile def
#include "playerInfo.h"
#include "shipItems.h"     // For EngineerBuildObjects enum

#include "UI.h"
#include "UIEditor.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "UIErrorMessage.h"
#include "UIQueryServers.h"
#include "md5wrapper.h"
  

namespace Zap
{
// Global list of clients (if we're a server).
GameConnection GameConnection::gClientList;

extern string gServerPassword;
extern string gAdminPassword;
extern string gLevelChangePassword;


TNL_IMPLEMENT_NETCONNECTION(GameConnection, NetClassGroupGame, true);

// Constructor
GameConnection::GameConnection()
{
   mNext = mPrev = this;
   setTranslatesStrings();
   mInCommanderMap = false;
   mIsAdmin = false;
   mIsLevelChanger = false;
   mIsBusy = false;
   mGotPermissionsReply = false;
   mWaitingForPermissionsReply = false;
   mSwitchTimer.reset(0);
   mScore = 0;        // Total points scored my this connection (this game)
   mTotalScore = 0;   // Total points scored by anyone while this connection is alive (this game)
   mGamesPlayed = 0;  // Overall
   mRating = 0;       // Overall
   mAcheivedConnection = false;
   mLastEnteredLevelChangePassword = "";
   mLastEnteredAdminPassword = "";
}

// Destructor
GameConnection::~GameConnection()
{
   // Unlink ourselves if we're in the client list
   mPrev->mNext = mNext;
   mNext->mPrev = mPrev;

   // Log the disconnect...
   logprintf(LogConsumer::LogConnection, "%s - client \"%s\" disconnected.", getNetAddressString(), mClientName.getString());

   if(isConnectionToClient() && gServerGame->getSuspendor() == this)     // isConnectionToClient only true if we're the server
      gServerGame->suspenderLeftGame();


   if(isConnectionToClient() && mAcheivedConnection)    // isConnectionToClient only true if we're the server
   {
     // Compute time we were connected
     time_t quitTime;
     time(&quitTime);

     double elapsed = difftime (quitTime, joinTime);

     logprintf(LogConsumer::ServerFilter, "%s [%s] quit [%s] (%.2lf secs)", mClientName.getString(), 
                                               isLocalConnection() ? "Local Connection" : getNetAddressString(), 
                                               getTimeStamp().c_str(), elapsed);
   }
}


/// Adds this connection to the doubly linked list of clients.
void GameConnection::linkToClientList()
{
   mNext = gClientList.mNext;
   mPrev = gClientList.mNext->mPrev;
   mNext->mPrev = this;
   mPrev->mNext = this;
}


GameConnection *GameConnection::getClientList()
{
   return gClientList.getNextClient();
}


S32 GameConnection::getClientCount()
{
   S32 count = 0;

   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
      count++;

   return count;
}


bool GameConnection::onlyClientIs(GameConnection *client)
{
   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
      if(walk != client)
         return false;

   return true;
}


GameConnection *GameConnection::getNextClient()
{
   if(mNext == &gClientList)
      return NULL;
   return mNext;
}


//  Runs on server, theRef should never be null; therefore mClientRef should never be null.
void GameConnection::setClientRef(ClientRef *theRef)
{
   TNLAssert(theRef, "NULL ClientRef!");
   mClientRef = theRef;
}


// See comment above about why mClientRef should never be NULL.  Actually, it can be null while server is quitting the game.
ClientRef *GameConnection::getClientRef()
{
   return mClientRef;
}


extern md5wrapper md5;

void GameConnection::submitAdminPassword(const char *password)
{
   string encrypted = md5.getSaltedHashFromString(password);
   c2sAdminPassword(encrypted.c_str());

   mLastEnteredAdminPassword = encrypted;

   setGotPermissionsReply(false);
   setWaitingForPermissionsReply(true);      // Means we'll show a reply from the server
}


void GameConnection::submitLevelChangePassword(string password)    // password here has not yet been encrypted
{
   string encrypted = md5.getSaltedHashFromString(password);
   c2sLevelChangePassword(encrypted.c_str());

   mLastEnteredLevelChangePassword = encrypted;

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
TNL_IMPLEMENT_RPC(GameConnection, c2sSuspendGame, (bool suspend), (suspend), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(suspend)
      gServerGame->suspendGame(this);
   else
      gServerGame->unsuspendGame(true);
}

  
// Here, the server has sent a message to a suspended client to wake up, action's coming in hot!
// We'll also play the playerJoined sfx to alert local client that the game is on again.
TNL_IMPLEMENT_RPC(GameConnection, s2cUnsuspend, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   gClientGame->unsuspendGame();       
   SFXObject::play(SFXPlayerJoined, 1);
}


void GameConnection::changeParam(const char *param, ParamType type)
{
   c2sSetParam(param, type);
}


extern bool engClientCreateObject(GameConnection *connection, U32 object);

TNL_IMPLEMENT_RPC(GameConnection, c2sEngineerDeployObject, (RangedU32<0,EngineeredObjectCount> type), (type), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(engClientCreateObject(this, type))
   {
      // Announce the build
      StringTableEntry msg( "%e0 has engineered a %e1." );
      Vector<StringTableEntry> e;
      e.push_back(getClientName());
      e.push_back(type == EngineeredTurret ? "turret" : "force field");
   
      for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
         walk->s2cDisplayMessageE(ColorAqua, SFXNone, msg, e);
   }
}


TNL_IMPLEMENT_RPC(GameConnection, c2sAdminPassword, (StringPtr pass), (pass), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   // If gAdminPassword is blank, no one can get admin permissions except the local host, if there is one...
   if(gAdminPassword != "" && !strcmp(md5.getSaltedHashFromString(gAdminPassword).c_str(), pass))
   {
      setIsAdmin(true);          // Enter admin PW and...
      setIsLevelChanger(true);   // ...get these permissions too!
      s2cSetIsAdmin(true);                                                 // Tell client they have been granted access
      gServerGame->getGameType()->s2cClientBecameAdmin(mClientRef->name);  // Announce change to world
   }
   else
      s2cSetIsAdmin(false);                                                // Tell client they have NOT been granted access
}


TNL_IMPLEMENT_RPC(GameConnection, c2sLevelChangePassword, (StringPtr pass), (pass), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   // If password is blank, permissions always granted
   if(gLevelChangePassword == "" || !strcmp(md5.getSaltedHashFromString(gLevelChangePassword).c_str(), pass))
   {
      setIsLevelChanger(true);
      s2cSetIsLevelChanger(true, true);                                           // Tell client they have been granted access
      gServerGame->getGameType()->s2cClientBecameLevelChanger(mClientRef->name);  // Announce change to world
   }
   else
      s2cSetIsLevelChanger(false, true);                                          // Tell client they have NOT been granted access
}


extern CIniFile gINI;
extern string gHostName;
extern string gHostDescr;
extern ServerGame *gServerGame;
extern Vector<StringTableEntry> gLevelSkipList;

// Allow admins to change the passwords on their systems
TNL_IMPLEMENT_RPC(GameConnection, c2sSetParam, (StringPtr param, RangedU32<0, GameConnection::ParamTypeCount> type), (param, type),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(!isAdmin())    // Do nothing --> non-admins have no pull here
      return;

   // Check for forbidden blank parameters
   if((type == (U32)AdminPassword || type == (U32)ServerName || type == (U32)ServerDescr) &&
                          !strcmp(param.getString(), ""))    // Some params can't be blank
      return;

   // Add a message to the server log
   if(type == (U32)DeleteLevel)
      logprintf(LogConsumer::ServerFilter, "User [%s] added level [%s] to server skip list", mClientRef->name.getString(), 
                                                gServerGame->getCurrentLevelFileName().getString());
   else
   {
      const char *types[] = { "level change password", "admin password", "server password", "server name", "server description" };
      logprintf(LogConsumer::ServerFilter, "User [%s] %s %s", mClientRef->name.getString(), 
                                                strcmp(param.getString(), "") ? "set" : "cleared", types[type]);
   }


   // Update our in-memory copies of the param
   if(type == (U32)LevelChangePassword)
      gLevelChangePassword = param.getString();
   else if(type == (U32)AdminPassword)
      gAdminPassword = param.getString();
   else if(type == (U32)ServerPassword)
      gServerPassword = param.getString();
   else if(type == (U32)ServerName)
   {
      gServerGame->setHostName(param.getString());
      gHostName = param.getString();      // Needed on local host?
   }
   else if(type == (U32)ServerDescr)
   {
      gServerGame->setHostDescr(param.getString());    // Do we also need to set gHost
      gHostDescr = param.getString();                  // Needed on local host?
   }
   else if(type == (U32)DeleteLevel)
   {
      // Avoid duplicates on skip list
      bool found = false;
      for(S32 i = 0; i < gLevelSkipList.size(); i++)
         if(gLevelSkipList[i] == gServerGame->getCurrentLevelFileName())
         {
            found = true;
            break;
         }

      if(!found)
      {
         // Add level to our skip list.  Deleting it from the active list of levels is more of a challenge...
         gLevelSkipList.push_back(gServerGame->getCurrentLevelFileName());
         writeSkipList();     // Write skipped levels to INI
         gINI.WriteFile();    // Save new INI settings to disk
      }
   }

   if(type != (U32)DeleteLevel)
   {
      const char *keys[] = { "LevelChangePassword", "AdminPassword", "ServerPassword", "ServerName", "ServerDescription" };

      // Update the INI file
      gINI.SetValue("Host", keys[type], param.getString(), true);
      gINI.WriteFile();    // Save new INI settings to disk
   }

   

   // Some messages we might show the user... should these just be inserted directly below?
   static StringTableEntry levelPassChanged("Level change password changed");
   static StringTableEntry levelPassCleared("Level change password cleared -- anyone can change levels");
   static StringTableEntry adminPassChanged("Admin password changed");
   static StringTableEntry serverPassChanged("Server password changed -- only players with the password can connect");
   static StringTableEntry serverPassCleared("Server password cleared -- anyone can connect");
   static StringTableEntry serverNameChanged("Server name changed");
   static StringTableEntry serverDescrChanged("Server description changed");
   static StringTableEntry serverLevelDeleted("Level added to skip list; level will stay in rotation until server restarted");

   // Pick out just the right message
   StringTableEntry msg;

   if(type == (U32)LevelChangePassword)
      msg = strcmp(param.getString(), "") ? levelPassChanged : levelPassCleared;
   else if(type == (U32)AdminPassword)
      msg = adminPassChanged;
   else if(type == (U32)ServerPassword)
      msg = strcmp(param.getString(), "") ? serverPassChanged : serverPassCleared;
   else if(type == (U32)ServerName)
      msg = serverNameChanged;
   else if(type == (U32)ServerDescr)
      msg = serverDescrChanged;
   else if(type == (U32)DeleteLevel)
      msg = serverLevelDeleted;

   s2cDisplayMessage(ColorRed, SFXNone, msg);      // Notify user their bidding has been done

   // If we're clearning the level change password, quietly grant access to anyone who doesn't already have it
   if(type == (U32)LevelChangePassword && !strcmp(param.getString(), ""))
   {
      for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
         if(!walk->isLevelChanger())
         {
            walk->setIsLevelChanger(true);
            walk->s2cSetIsLevelChanger(true, false);     // Silently
         }
   }

   // If we've changed the server name, notify all the clients
   else if(type == (U32)ServerName)
      for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
         walk->s2cSetServerName(gServerGame->getHostName());
}


// Kick player or change his team
TNL_IMPLEMENT_RPC(GameConnection, c2sAdminPlayerAction,
   (StringTableEntry playerName, U32 actionIndex, S32 team), (playerName, actionIndex, team),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(!isAdmin())
      return;              // do nothing --> non-admins have no pull here

   // else...

   GameConnection *theClient;
   for(theClient = getClientList(); theClient; theClient = theClient->getNextClient())
      if(theClient->getClientName() == playerName)
         break;

   if(!theClient)    // Hmmm... couldn't find him.  Maybe the dude disconnected?
      return;

   static StringTableEntry kickMessage("%e0 was kicked from the game by %e1.");
   static StringTableEntry changeTeamMessage("%e0 had their team changed by %e1.");

   StringTableEntry msg;
   Vector<StringTableEntry> e;
   e.push_back(theClient->getClientName());
   e.push_back(getClientName());

   switch(actionIndex)
   {
   case PlayerMenuUserInterface::ChangeTeam:
      msg = changeTeamMessage;
      {
         GameType *gt = gServerGame->getGameType();
         gt->changeClientTeam(theClient, team);
      }
      break;
   case PlayerMenuUserInterface::Kick:
      {
         msg = kickMessage;
         if(theClient->isAdmin())
         {
            static StringTableEntry nokick("Can't kick an administrator!");
            s2cDisplayMessage(ColorAqua, SFXNone, nokick);
            return;
         }
         ConnectionParameters &p = theClient->getConnectionParameters();
         if(p.mIsArranged)
            gServerGame->getNetInterface()->banHost(p.mPossibleAddresses[0], BanDuration);      // Banned for 30 seconds
         gServerGame->getNetInterface()->banHost(theClient->getNetAddress(), BanDuration);      // Banned for 30 seconds

         theClient->disconnect(ReasonKickedByAdmin, "");
         break;
      }
   default:
      return;
   }
   // Broadcast the message
   for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
      walk->s2cDisplayMessageE(ColorAqua, SFXIncomingMessage, msg, e);
}


//// Announce a new player has become an admin
//TNL_IMPLEMENT_RPC(GameConnection, s2cClientBecameAdmin, (StringTableEntry name), (name), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
//{
//   ClientRef *cl = findClientRef(name);
//   cl->clientConnection->isAdmin = true;
//   gGameUserInterface.displayMessage(Color(0,1,1), "%s has been granted administrator access.", name.getString());
//}


// This gets called under two circumstances; when it's a new game, or when the server's name is changed by an admin
TNL_IMPLEMENT_RPC(GameConnection, s2cSetServerName, (StringTableEntry name), (name),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   setServerName(name);

   // If we know the level change password, apply for permissions if we don't already have them
   if(!mIsLevelChanger)
   {
      string levelChangePassword = gINI.GetValue("SavedLevelChangePasswords", getServerName());
      if(levelChangePassword != "")
      {
         c2sLevelChangePassword(levelChangePassword.c_str());
         setWaitingForPermissionsReply(false);     // Want to return silently
      }
   }

   // If we know the admin password, apply for permissions if we don't already have them
   if(!mIsAdmin)
   {
      string adminPassword = gINI.GetValue("SavedAdminPasswords", getServerName());
      if(adminPassword != "")
      {
         c2sAdminPassword(adminPassword.c_str());
         setWaitingForPermissionsReply(false);     // Want to return silently
      }
   }
}


extern Color gCmdChatColor;

TNL_IMPLEMENT_RPC(GameConnection, s2cSetIsAdmin, (bool granted), (granted),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   static const char *adminPassSuccessMsg = "You've been granted permission to manage players and change levels";
   static const char *adminPassFailureMsg = "Incorrect password: Admin access denied";

   if(mClientRef)
   {
      if(granted)
         logprintf(LogConsumer::ServerFilter, "User [%s] granted admin permissions", mClientRef->name.getString());
      else
         logprintf(LogConsumer::ServerFilter, "User [%s] denied admin permissions", mClientRef->name.getString());
   }

   setIsAdmin(granted);

   // Admin permissions automatically give level change permission
   if(granted)                      // Don't rescind level change permissions for entering a bad admin PW
      setIsLevelChanger(true);


   // If we entered a password, and it worked, let's save it for next time
   if(granted && mLastEnteredAdminPassword != "")
   {
      gINI.SetValue("SavedAdminPasswords", getServerName(), mLastEnteredAdminPassword, true);
      mLastEnteredAdminPassword = "";
   }

   // We have the wrong password, let's make sure it's not saved
   if(!granted)
      gINI.DeleteValue("SavedAdminPasswords", getServerName());

   setGotPermissionsReply(true);

   // If we're not waiting, don't show us a message.  Supresses superflous messages on startup.
   if(waitingForPermissionsReply())
   {
      if(granted)
      {
         // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
         if(UserInterface::current->getMenuID() == GameMenuUI)
            gGameMenuUserInterface.menuSubTitle = adminPassSuccessMsg;
         else
            gGameUserInterface.displayMessage(gCmdChatColor, adminPassSuccessMsg);
      }
      else
      {
         if(UserInterface::current->getMenuID() == GameMenuUI)
            gGameMenuUserInterface.menuSubTitle = adminPassFailureMsg;
         else
            gGameUserInterface.displayMessage(gCmdChatColor, adminPassFailureMsg);
      }
   }
}


TNL_IMPLEMENT_RPC(GameConnection, s2cSetIsLevelChanger, (bool granted, bool notify), (granted, notify),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   static const char *levelPassSuccessMsg = "You've been granted permission to change levels";
   static const char *levelPassFailureMsg = "Incorrect password: Level changing permissions denied";

   if(mClientRef)
   {
      if(granted)
         logprintf(LogConsumer::ServerFilter, "User [%s] granted level change permissions", mClientRef->name.getString());
      else
         logprintf(LogConsumer::ServerFilter, "User [%s] denied level change permissions", mClientRef->name.getString());
   }

   // If we entered a password, and it worked, let's save it for next time
   if(granted && mLastEnteredLevelChangePassword != "")
   {
      gINI.SetValue("SavedLevelChangePasswords", getServerName(), mLastEnteredLevelChangePassword, true);
      mLastEnteredLevelChangePassword = "";
   }

   // We have the wrong password, let's make sure it's not saved
   if(!granted)
      gINI.DeleteValue("SavedLevelChangePasswords", getServerName());


   setIsLevelChanger(granted);

   setGotPermissionsReply(true);

   // If we're not waiting, don't show us a message.  Supresses superflous messages on startup.
   if(waitingForPermissionsReply() && notify)
   {
      if(granted)
      {
         // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
         if(UserInterface::current->getMenuID() == GameMenuUI)
            gGameMenuUserInterface.menuSubTitle = levelPassSuccessMsg;
         else
            gGameUserInterface.displayMessage(gCmdChatColor, levelPassSuccessMsg);
      }
      else
      {
         if(UserInterface::current->getMenuID() == GameMenuUI)
            gGameMenuUserInterface.menuSubTitle = levelPassFailureMsg;
         else
            gGameUserInterface.displayMessage(gCmdChatColor, levelPassFailureMsg);
      }
   }
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestCommanderMap, (), (),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   mInCommanderMap = true;
}

TNL_IMPLEMENT_RPC(GameConnection, c2sReleaseCommanderMap, (), (),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   mInCommanderMap = false;
}

// Client has changed his loadout configuration.  This gets run on the server as soon as the loadout is entered.
TNL_IMPLEMENT_RPC(GameConnection, c2sRequestLoadout, (Vector<U32> loadout), (loadout), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   mLoadout = loadout;
   GameType *gt = gServerGame->getGameType();
   if(gt)
      gt->clientRequestLoadout(this, mLoadout);    // Currently does nothing

   // Check if ship is in a loadout zone, in which case we'll make the loadout take effect immediately
   Ship *ship = dynamic_cast<Ship *>(this->getControlObject());

   if(ship && ship->isInZone(LoadoutZoneType))
      ship->setLoadout(loadout);
}

static Color colors[] =
{
   Color(1,1,1),           // ColorWhite
   Color(1,0,0),           // ColorRed    ==> also used for chat commands
   Color(0,1,0),           // ColorGreen
   Color(0,0,1),           // ColorBlue
   Color(0,1,1),           // ColorAqua
   Color(1,1,0),           // ColorYellow
   Color(0.6f, 1, 0.8f),   // ColorNuclearGreen
};

Color gCmdChatColor = colors[GameConnection::ColorRed];

static void displayMessage(U32 colorIndex, U32 sfxEnum, const char *message)
{

   gGameUserInterface.displayMessage(colors[colorIndex], "%s", message);
   if(sfxEnum != SFXNone)
      SFXObject::play(sfxEnum);
}

// I believe this is not used -CE
TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessageESI,
                  (RangedU32<0, GameConnection::ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString,
                  Vector<StringTableEntry> e, Vector<StringPtr> s, Vector<S32> i),
                  (color, sfx, formatString, e, s, i),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
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
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   displayMessageE(color, sfx, formatString, e);
}



TNL_IMPLEMENT_RPC(GameConnection, s2cTouchdownScored,
                  (U32 sfx, S32 team, StringTableEntry formatString, Vector<StringTableEntry> e),
                  (sfx, team, formatString, e),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   displayMessageE(GameConnection::ColorNuclearGreen, sfx, formatString, e);
   gClientGame->getGameType()->majorScoringEventOcurred(team);
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


//class RPC_GameConnection_s2cDisplayMessage : public TNL::RPCEvent { \
//public: \
//   TNL::FunctorDecl<void (GameConnection::*) args > mFunctorDecl;\
//   RPC_GameConnection_s2cDisplayMessage() : TNL::RPCEvent(guaranteeType, eventDirection), mFunctorDecl(&GameConnection::s2cDisplayMessage_remote) { mFunctor = &mFunctorDecl; } \
//   TNL_DECLARE_CLASS( RPC_GameConnection_s2cDisplayMessage ); \
//   bool checkClassType(TNL::Object *theObject) { return dynamic_cast<GameConnection *>(theObject) != NULL; } }; \
//   TNL_IMPLEMENT_NETEVENT( RPC_GameConnection_s2cDisplayMessage, groupMask, rpcVersion ); \
//   void GameConnection::name args { if(!canPostNetEvent()) return; RPC_GameConnection_s2cDisplayMessage *theEvent = new RPC_GameConnection_s2cDisplayMessage; theEvent->mFunctorDecl.set argNames ; postNetEvent(theEvent); } \
//   TNL::NetEvent * GameConnection::s2cDisplayMessage_construct args { RPC_GameConnection_s2cDisplayMessage *theEvent = new RPC_GameConnection_s2cDisplayMessage; theEvent->mFunctorDecl.set argNames ; return theEvent; } \
//   void GameConnection::s2cDisplayMessage_test args { RPC_GameConnection_s2cDisplayMessage *theEvent = new RPC_GameConnection_s2cDisplayMessage; theEvent->mFunctorDecl.set argNames ; TNL::PacketStream ps; theEvent->pack(this, &ps); ps.setBytePosition(0); theEvent->unpack(this, &ps); theEvent->process(this); } \
//   void GameConnection::s2cDisplayMessage_remote args


TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessage,
                  (RangedU32<0, GameConnection::ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString),
                  (color, sfx, formatString),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   static const S32 STRLEN = 256;
   char outputBuffer[STRLEN];

   strncpy(outputBuffer, formatString.getString(), STRLEN - 1);
   outputBuffer[STRLEN - 1] = '\0';    // Make sure we're null-terminated

   displayMessage(color, sfx, outputBuffer);
}


TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessageBox, (StringTableEntry title, StringTableEntry instr, Vector<StringTableEntry> message),
                  (title, instr, message), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   gErrorMsgUserInterface.reset();
   gErrorMsgUserInterface.setTitle(title.getString());
   gErrorMsgUserInterface.setInstr(instr.getString());

   for(S32 i = 0; i < message.size(); i++)
      gErrorMsgUserInterface.setMessage(i+1, message[i].getString());      // UIErrorMsgInterface ==> first line = 1

   gErrorMsgUserInterface.activate();
}


// Server sends the name and type of a level to the client (gets run repeatedly when client connects to the server)
TNL_IMPLEMENT_RPC(GameConnection, s2cAddLevel, (StringTableEntry name, StringTableEntry type), (name, type),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   mLevelInfos.push_back(LevelInfo(name, type));
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestLevelChange, (S32 newLevelIndex, bool isRelative), (newLevelIndex, isRelative), 
                              NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(!mIsLevelChanger)
      return;

   bool restart = false;

   if(isRelative)
      newLevelIndex = (gServerGame->getCurrentLevelIndex() + newLevelIndex ) % gServerGame->getLevelCount();
   else if(newLevelIndex == ServerGame::REPLAY_LEVEL)
         restart = true;

   StringTableEntry msg( restart ? "%e0 restarted the current level." : "%e0 changed the level to %e1." );
   Vector<StringTableEntry> e;
   e.push_back(getClientName());
   
   if(!restart)
      e.push_back(gServerGame->getLevelNameFromIndex(newLevelIndex));

   gServerGame->cycleLevel(newLevelIndex);

   for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
      walk->s2cDisplayMessageE(ColorYellow, SFXNone, msg, e);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestShutdown, (U16 time), (time), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(!mIsAdmin)
      return;

   logprintf(LogConsumer::ServerFilter, "User [%s] requested shutdown in %d seconds", mClientRef->name.getString(), time);

   gServerGame->setShuttingDown(true, time, mClientRef);

   for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
      walk->s2cInitiateShutdown(time, mClientRef->name, walk == this);
}


TNL_IMPLEMENT_RPC(GameConnection, s2cInitiateShutdown, (U16 time, StringTableEntry name, bool originator),
                  (time, name, originator), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   gGameUserInterface.shutdownInitiated(time, name, originator);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestCancelShutdown, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(!mIsAdmin)
      return;

   logprintf(LogConsumer::ServerFilter, "User %s canceled shutdown", mClientRef->name.getString());

   for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
      if(walk != this)     // Don't send message to cancellor!
         walk->s2cCancelShutdown();

   gServerGame->setShuttingDown(false, 0, NULL);
}


TNL_IMPLEMENT_RPC(GameConnection, s2cCancelShutdown, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   gGameUserInterface.shutdownCanceled();
}


// Client tells server that they are busy chatting or futzing with menus or configuring ship... or not
TNL_IMPLEMENT_RPC(GameConnection, c2sSetIsBusy, (bool busy), (busy), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 2)
{
   setIsBusy(busy);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sSetServerAlertVolume, (S8 vol), (vol), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 2)
{
   //setServerAlertVolume(vol);
}


extern IniSettings gIniSettings;

// Send password, client's name, and version info to game server
void GameConnection::writeConnectRequest(BitStream *stream)
{
   Parent::writeConnectRequest(stream);

   bool isLocal = gServerGame;      // Only way to have gServerGame defined is if we're also hosting... ergo, we must be local

   // If we're the local player, determine if we need a password to use our user name...  if so, supply it so the user doesn't need to enter it.
   string password = "";
   if(isLocal)
   {
      // Do we have a password?  Look for the name in our reserved names list.
      for(S32 i = 0; i < gIniSettings.reservedNames.size(); i++)
         if(!stricmp(gIniSettings.reservedNames[i].c_str(), mClientName.getString()))
         {
            password = gIniSettings.reservedPWs[i];
            break;
         }
   }

   stream->writeString(isLocal ? md5.getSaltedHashFromString(gServerPassword).c_str() : md5.getSaltedHashFromString(gPasswordEntryUserInterface.getText()).c_str());
   stream->writeString(mClientName.getString());
   stream->writeString(isLocal ? md5.getSaltedHashFromString(password).c_str() : md5.getSaltedHashFromString(gReservedNamePasswordEntryUserInterface.getText()).c_str());
}


// On the server side of things, read the connection request, and return if everything looks legit.  If not, provide an error string
// to help diagnose the problem, or prompt further data from client (such as a password).
bool GameConnection::readConnectRequest(BitStream *stream, const char **errorString)
{
   if(!Parent::readConnectRequest(stream, errorString))
      return false;

   if(gServerGame->isFull())
   {
      *errorString = "Server Full.";
      return false;
   }

   char buf[256];
   char pwbuf[256];

   stream->readString(buf);
   if(gServerPassword != "" && stricmp(buf, md5.getSaltedHashFromString(gServerPassword).c_str()))
   {
      *errorString = "PASSWORD";
      return false;
   }

   // Now read the player name
   stream->readString(buf);
   size_t len = strlen(buf);

   if(len > MAX_SHORT_TEXT_LEN)      // Make sure it isn't too long
      len = MAX_SHORT_TEXT_LEN;

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

   // Remove invisible chars
   for(size_t i = 0; i < len; i++)
      if(!(name[i] >= ' ' && name[i] <= '~'))
         name[i] = 'X';

   name[len] = 0;    // Terminate string properly

   // Take a break from our name cleanup to check the reserved-name-
   // password (use the cleaned up but not-yet-uniqued name for this)
   stream->readString(pwbuf);

   string password = "";
   // Do we need a password?  Look for the name in our reserved names list.
   for(S32 i = 0; i < gIniSettings.reservedNames.size(); i++)
      if(!stricmp(gIniSettings.reservedNames[i].c_str(), name))
      {
         password = gIniSettings.reservedPWs[i];
         break;
      }

   if(password != "" && stricmp(pwbuf, md5.getSaltedHashFromString(password).c_str()))
   {
      *errorString = "RESERVEDNAME";
      return false;
   }

   mClientName = name;
   return true;
}


// Make sure name is unique.  If it's not, make it so.  The problem is that then the client doesn't know their official name.
// This makes the assumption that we'll find a unique name before numstr runs out of space (allowing us to try 999,999,999 or so combinations)
std::string GameConnection::makeUnique(string name)
{
   U32 index = 0;
   string proposedName = name;

   bool unique = false;

   while(!unique)
   {
      unique = true;
      for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
      {
         // TODO:  How to combine these blocks?
         if(proposedName == walk->mClientName.getString())          // Collision detected!
         {
            unique = false;

            char numstr[10];
            sprintf(numstr, ".%d", index);
			// Max length name can be such that when number is appended, it's still less than MAX_SHORT_TEXT_LEN
            S32 maxNamePos = MAX_SHORT_TEXT_LEN - (S32)strlen(numstr); 
            name = name.substr(0, maxNamePos);                         // Make sure name won't grow too long
            proposedName = name + numstr;

            index++;
            break;
         }
      }

      for(S32 i = 0; i < Robot::robots.size(); i++)
      {
         if(proposedName == Robot::robots[i]->getName().getString())
         {
            unique = false;

            char numstr[10];
            sprintf(numstr, ".%d", index);

			// Max length name can be such that when number is appended, it's still less than MAX_SHORT_TEXT_LEN
            S32 maxNamePos = MAX_SHORT_TEXT_LEN - (S32)strlen(numstr);   
            name = name.substr(0, maxNamePos);                      // Make sure name won't grow too long
            proposedName = name + numstr;

            index++;
            break;
         }
      }
   }

   return proposedName;
}


void GameConnection::onConnectionEstablished()
{
   Parent::onConnectionEstablished();

   if(isInitiator())    // Client-side
   {
      setGhostFrom(false);
      setGhostTo(true);
      logprintf(LogConsumer::LogConnection, "%s - connected to server.", getNetAddressString());

      setFixedRateParameters(50, 50, 2000, 2000);       // U32 minPacketSendPeriod, U32 minPacketRecvPeriod, U32 maxSendBandwidth, U32 maxRecvBandwidth
   }
   else                 // Server-side
   {
      linkToClientList();              // Add to list of clients
      gServerGame->addClient(this);
      setGhostFrom(true);
      setGhostTo(false);
      activateGhosting();
      setFixedRateParameters(50, 50, 2000, 2000);        // U32 minPacketSendPeriod, U32 minPacketRecvPeriod, U32 maxSendBandwidth, U32 maxRecvBandwidth

      s2cSetServerName(gServerGame->getHostName());      // Ideally, this would be part of the connection handshake, but this should work fine

      time(&joinTime);
      mAcheivedConnection = true;

      Robot::getEventManager().fireEvent(NULL, EventManager::PlayerJoinedEvent, getClientRef()->getPlayerInfo());

      if(gLevelChangePassword == "")                // Grant level change permissions if level change PW is blank
      {
         setIsLevelChanger(true);
         s2cSetIsLevelChanger(true, false);         // Tell client, but don't display notification
      }

      logprintf(LogConsumer::LogConnection, "%s - client \"%s\" connected.", getNetAddressString(), mClientName.getString());

      if(isLocalConnection())
         logprintf(LogConsumer::ServerFilter, "%s [%s] joined [%s]", mClientName.getString(), "Local Connection", getTimeStamp().c_str());
      else
         logprintf(LogConsumer::ServerFilter, "%s [%s] joined [%s]", mClientName.getString(), getNetAddressString(), getTimeStamp().c_str());
   }
}

void GameConnection::onConnectionTerminated(NetConnection::TerminationReason reason, const char *reasonStr)
{
   if(isInitiator())    // i.e. this is a client that connected to the server
   {
      if (UserInterface::cameFromEditor())
         UserInterface::reactivateMenu(gEditorUserInterface);
      else
         UserInterface::reactivateMenu(gMainMenuUserInterface);

      gClientGame->unsuspendGame();

      // Display a context-appropriate error message
      gErrorMsgUserInterface.reset();
      gErrorMsgUserInterface.setTitle("Connection Terminated");

      switch(reason)
      {
         case NetConnection::ReasonTimedOut:
            gErrorMsgUserInterface.setMessage(2, "Your connection timed out.  Please try again later.");
            gErrorMsgUserInterface.activate();
            break;

         // We should never see this one...
         case NetConnection::ReasonNeedPassword:
            gErrorMsgUserInterface.setMessage(2, "Your connection was rejected by the server.");
            gErrorMsgUserInterface.setMessage(4, "You need to supply a password!");
            gErrorMsgUserInterface.activate();
            break;

         case NetConnection::ReasonPuzzle:
            gErrorMsgUserInterface.setMessage(2, "Unable to connect to the server.  Recieved message:");
            gErrorMsgUserInterface.setMessage(3, "Invalid puzzle solution");
            gErrorMsgUserInterface.setMessage(5, "Please try a different game server, or try again later.");
            gErrorMsgUserInterface.activate();
            break;

         case NetConnection::ReasonKickedByAdmin:
            gErrorMsgUserInterface.setMessage(2, "You were kicked off the server by an admin,");
            gErrorMsgUserInterface.setMessage(3, "and have been temporarily banned.");
            gErrorMsgUserInterface.setMessage(5, "You can try another server, host your own,");
            gErrorMsgUserInterface.setMessage(6, "or try the server that kicked you again later.");
            gErrorMsgUserInterface.activate();

            // Add this server to our list of servers not to display for a spell...
            gQueryServersUserInterface.addHiddenServer(getNetAddress(), Platform::getRealMilliseconds() + BanDuration);
            break;

         case NetConnection::ReasonFloodControl:
            gErrorMsgUserInterface.setMessage(2, "Your connection was rejected by the server");
            gErrorMsgUserInterface.setMessage(3, "because you sent too many connection requests.");
            gErrorMsgUserInterface.setMessage(5, "Please try a different game server, or try again later.");
            gErrorMsgUserInterface.activate();
            break;

         case NetConnection::ReasonError:
            gErrorMsgUserInterface.setMessage(2, "Unable to connect to the server.  Recieved message:");
            gErrorMsgUserInterface.setMessage(3, reasonStr);
            gErrorMsgUserInterface.setMessage(5, "Please try a different game server, or try again later.");
            gErrorMsgUserInterface.activate();
            break;

         case NetConnection::ReasonShutdown:
            gErrorMsgUserInterface.setMessage(2, "Remote server shut down.");
            gErrorMsgUserInterface.setMessage(4, "Please try a different server,");
            gErrorMsgUserInterface.setMessage(5, "or host a game of your own!");
            gErrorMsgUserInterface.activate();
            break;

         case NetConnection::ReasonSelfDisconnect:
               // We get this when we terminate our own connection.  Since this is intentional behavior,
               // we don't want to display any message to the user.
            break;

         default:
            gErrorMsgUserInterface.setMessage(1, "Unable to connect to the server for reasons unknown.");
            gErrorMsgUserInterface.setMessage(3, "Please try a different game server, or try again later.");
            gErrorMsgUserInterface.activate();
      }
   }
   else     // Server
   {
      if(getClientRef() != NULL)    // ClientRef might be NULL if the server is quitting the game, in which case we don't need to fire these events anyway
      {
         getClientRef()->getPlayerInfo()->setDefunct();
         Robot::getEventManager().fireEvent(NULL, EventManager::PlayerLeftEvent, getClientRef()->getPlayerInfo());
      }

      gServerGame->removeClient(this);
   }

}


// I'm thinking that this function only gets called while the player is trying to connect to a server.
void GameConnection::onConnectTerminated(TerminationReason r, const char *string)
{
   if(isInitiator())
   {
      if(!strcmp(string, "PASSWORD"))
      {
         gPasswordEntryUserInterface.setConnectServer(getNetAddress());
         gPasswordEntryUserInterface.activate();
      }
      else if(!strcmp(string, "RESERVEDNAME"))
      {
         gReservedNamePasswordEntryUserInterface.setConnectServer(getNetAddress());
         gReservedNamePasswordEntryUserInterface.activate();
      }
      else  // Looks like the connection failed for some unknown reason.  Server died?
      {
         UserInterface::reactivateMenu(gMainMenuUserInterface);

         // Display a context-appropriate error message
         gErrorMsgUserInterface.reset();
         gErrorMsgUserInterface.setTitle("Connection Terminated");

         gMainMenuUserInterface.activate();
         gErrorMsgUserInterface.setMessage(2, "Lost connection with the server.");
         gErrorMsgUserInterface.setMessage(3, "Unable to join game.  Please try again.");
         gErrorMsgUserInterface.activate();
      }
   }
}

};


