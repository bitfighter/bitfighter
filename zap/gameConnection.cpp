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

extern const char *gServerPassword;
extern const char *gAdminPassword;
extern const char *gLevelChangePassword;


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
   mCumScore = 0;    // Total points scored my this connection
   mTotalScore = 0;  // Total points scored by anyone while this connection is alive
   mAcheivedConnection = false;
}

// Destructor
GameConnection::~GameConnection()
{
   // Unlink ourselves if we're in the client list
   mPrev->mNext = mNext;
   mNext->mPrev = mPrev;

   // Log the disconnect...
   TNL::logprintf("%s - client \"%s\" disconnected.", getNetAddressString(), mClientName.getString());

   if(isConnectionToClient() && mAcheivedConnection)    // isConnectionToClient only true if we're the server
   {  
     // Compute time we were connected
     time_t quitTime;
     time(&quitTime);

     double elapsed = difftime (quitTime, joinTime);

     TNL::s_logprintf("%s [%s] quit :: %s (%.2lf secs)", mClientName.getString(), isLocalConnection() ? "Local Connection" : getNetAddressString(), getTimeStamp().c_str(), elapsed);
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

GameConnection *GameConnection::getNextClient()
{
   if(mNext == &gClientList)
      return NULL;
   return mNext;
}

void GameConnection::setClientRef(ClientRef *theRef)
{
   mClientRef = theRef;
}

ClientRef *GameConnection::getClientRef()
{
   return mClientRef;
}


extern md5wrapper md5;

void GameConnection::submitAdminPassword(const char *password)
{
   c2sAdminPassword(md5.getSaltedHashFromString(password).c_str());
   setGotPermissionsReply(false);           
   setWaitingForPermissionsReply(true);     
}


void GameConnection::submitLevelChangePassword(const char *password)
{
   c2sLevelChangePassword(md5.getSaltedHashFromString(password).c_str());
   setGotPermissionsReply(false);           
   setWaitingForPermissionsReply(true); 
}


TNL_IMPLEMENT_RPC(GameConnection, c2sAdminPassword, (StringPtr pass), (pass), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{

   if(gAdminPassword && !strcmp(md5.getSaltedHashFromString(gAdminPassword).c_str(), pass))
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
   if(gLevelChangePassword && !strcmp(md5.getSaltedHashFromString(gLevelChangePassword).c_str(), pass))
   {
      setIsLevelChanger(true);
      s2cSetIsLevelChanger(true);                                                 // Tell client they have been granted access
      gServerGame->getGameType()->s2cClientBecameLevelChanger(mClientRef->name);  // Announce change to world
   }
   else
      s2cSetIsLevelChanger(false);                                                // Tell client they have NOT been granted access
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

   StringTableEntry msg;
   static StringTableEntry kickMessage("%e0 was kicked from the game by %e1.");
   static StringTableEntry changeTeamMessage("%e0 had their team changed by %e1.");
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

TNL_IMPLEMENT_RPC(GameConnection, s2cSetServerName, (StringTableEntry name), (name),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   setServerName(name);
}


extern Color gCmdChatColor;

static const char *adminPassSuccessMsg = "You've been granted permission to manage players and change levels";
static const char *adminPassFailureMsg = "Incorrect password: Admin access denied";

TNL_IMPLEMENT_RPC(GameConnection, s2cSetIsAdmin, (bool granted), (granted),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   setIsAdmin(granted);
   if(granted)                      // Don't want to rescind level change permissions for entering a bad PW
         setIsLevelChanger(true);

   setGotPermissionsReply(true);

   // If we're not waiting, don't show us a message.  Supresses superflous messages on startup.
   if(!waitingForPermissionsReply())
      return;

   if(granted)
      // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
      if(UserInterface::current->getMenuID() == GameMenuUI)
         gGameMenuUserInterface.menuSubTitle = adminPassSuccessMsg;    
      else
         gGameUserInterface.displayMessage(gCmdChatColor, adminPassSuccessMsg);

   else
      if(UserInterface::current->getMenuID() == GameMenuUI)
         gGameMenuUserInterface.menuSubTitle = adminPassFailureMsg;     
      else
         gGameUserInterface.displayMessage(gCmdChatColor, adminPassFailureMsg);
}


static const char *levelPassSuccessMsg = "You've been granted permission to change levels";
static const char *levelPassFailureMsg = "Incorrect password: Level changing permissions denied";

TNL_IMPLEMENT_RPC(GameConnection, s2cSetIsLevelChanger, (bool granted), (granted),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   setIsLevelChanger(granted);

   setGotPermissionsReply(true);

   // If we're not waiting, don't show us a message.  Supresses superflous messages on startup.
   if(!waitingForPermissionsReply())
      return;

   if(granted)      
      // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
      if(UserInterface::current->getMenuID() == GameMenuUI)
         gGameMenuUserInterface.menuSubTitle = levelPassSuccessMsg;    
      else
         gGameUserInterface.displayMessage(gCmdChatColor, levelPassSuccessMsg);

   else
      if(UserInterface::current->getMenuID() == GameMenuUI)
         gGameMenuUserInterface.menuSubTitle = levelPassFailureMsg;     
      else
         gGameUserInterface.displayMessage(gCmdChatColor, levelPassFailureMsg);
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


static void displayMessage(U32 colorIndex, U32 sfxEnum, const char *message)
{
   static Color colors[] =
   {
      Color(1,1,1),				// ColorWhite
      Color(1,0,0),				// ColorRed
      Color(0,1,0),				// ColorGreen
      Color(0,0,1),				// ColorBlue
      Color(0,1,1),				// ColorAqua
      Color(1,1,0),				// ColorYellow
      Color(0.6f, 1, 0.8f),	// ColorNuclearGreen
   };
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


TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessage,
                  (RangedU32<0, GameConnection::ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString),
                  (color, sfx, formatString),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   char outputBuffer[256];
   S32 pos = 0;
   const char *src = formatString.getString();
   while(*src)
   {
      outputBuffer[pos++] = *src++;

      if(pos >= 255)
         break;
   }
   outputBuffer[pos] = 0;
   displayMessage(color, sfx, outputBuffer);
}


// Server sends the name and type of a level to the client (gets run repeatedly when client connects to the server)
TNL_IMPLEMENT_RPC(GameConnection, s2cAddLevel, (StringTableEntry name, StringTableEntry type), (name, type),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   mLevelNames.push_back(name);
   mLevelTypes.push_back(type);
}


TNL_IMPLEMENT_RPC(GameConnection, c2sRequestLevelChange, (S32 newLevelIndex, bool isRelative), (newLevelIndex, isRelative), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(mIsLevelChanger)
   {
      bool restart = false;

      if(isRelative)
         newLevelIndex = (gServerGame->getCurrentLevelIndex() + newLevelIndex ) % gServerGame->getLevelCount();
      else if(newLevelIndex == -2)
      {
         restart = true;
         newLevelIndex = gServerGame->getCurrentLevelIndex();
      }
         
      while(newLevelIndex < 0)
         newLevelIndex += gServerGame->getLevelCount();

      static StringTableEntry msg( restart ? "%e0 restarted the current level." : "%e0 changed the level to %e1." );
      Vector<StringTableEntry> e;
      e.push_back(getClientName());
      if(!restart)
         e.push_back(gServerGame->getLevelNameFromIndex(newLevelIndex));

      gServerGame->cycleLevel(newLevelIndex);
      for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
         walk->s2cDisplayMessageE(ColorYellow, SFXNone, msg, e);
   }
}


// Client tells server that they are busy chatting or futzing with menus or configuring ship... or not
TNL_IMPLEMENT_RPC(GameConnection, c2sSetIsBusy, (bool busy), (busy), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 2)
{
   setIsBusy(busy);
}


// Client tells server that they are busy chatting or futzing with menus or configuring ship... or not
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
      // Do we need a password?  Look for the name in our reserved names list.
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
   if(gServerPassword && stricmp(buf, md5.getSaltedHashFromString(gServerPassword).c_str()))
   {
      *errorString = "PASSWORD";
      return false;
   }

   // Now read the player name
   stream->readString(buf);
   size_t len = strlen(buf);

   if(len > 30)      // Make sure it isn't too long
      len = 30;

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

   // Now, back to our regularly scheduled program.
   // Make sure name is unique.  If it's not, make it so.  The problem is that then the client doesn't know their official name.
   U32 index = 0;
checkPlayerName:
   for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
   {
      if(!strcmp(walk->mClientName.getString(), name))
      {
         dSprintf(name + len, 3, ".%d", index);
         index++;
         if(index > 9 && len == 30 || index > 99 && len == 29)    // This little hack should cover us for more names than we can handle
         {
            len--;
            name[len] = 0;
         }
         goto checkPlayerName;
      }
   }

   mClientName = name;
   return true;
}

void GameConnection::onConnectionEstablished()
{
   Parent::onConnectionEstablished();

   if(isInitiator())    // Client-side
   {
      setGhostFrom(false);
      setGhostTo(true);
      TNL::logprintf("%s - connected to server.", getNetAddressString());
      setFixedRateParameters(50, 50, 2000, 2000);
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
         
      TNL::logprintf("%s - client \"%s\" connected.", getNetAddressString(), mClientName.getString());
      if(isLocalConnection())
         TNL::s_logprintf("%s [%s] joined :: %s", mClientName.getString(), "Local Connection", getTimeStamp().c_str());
      else
         TNL::s_logprintf("%s [%s] joined :: %s", mClientName.getString(), getNetAddressString(), getTimeStamp().c_str());
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
   else
      gServerGame->removeClient(this);
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

