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

#ifndef _GAMECONNECTION_H_
#define _GAMECONNECTION_H_


#include "sfx.h"
#include "controlObjectConnection.h"
#include "shipItems.h"           // For EngineerBuildObjects enum
#include "dataConnection.h"      // For DataSendable interface
#include "statistics.h"

#include "tnlNetConnection.h"
#include "timer.h"
#include <time.h>



namespace Zap
{
using TNL::StringPtr;
using std::string;

static const char USED_EXTERNAL *gConnectStatesTable[] = {
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
      ""
};

////////////////////////////////////////
////////////////////////////////////////

struct ClientInfo
{
   string name;
   Nonce id;
   bool authenticated;
   F32 simulatedPacketLoss;
   U32 simulatedLag;
};



////////////////////////////////////////
////////////////////////////////////////

class ClientRef;
class ClientGame;
struct LevelInfo;


class GameConnection: public ControlObjectConnection, public DataSendable
{
private:
   typedef ControlObjectConnection Parent;

   void initialize();

   time_t joinTime;
   bool mAcheivedConnection;

   // For saving passwords
   std::string mLastEnteredLevelChangePassword;
   std::string mLastEnteredAdminPassword;       

   // The server maintains a linked list of clients...
   GameConnection *mNext;
   GameConnection *mPrev;
   static GameConnection gClientList;

   bool mInCommanderMap;
   bool mIsRobot;
   bool mIsAdmin;
   bool mIsLevelChanger;
   bool mWaitingForPermissionsReply;
   bool mGotPermissionsReply;
   bool mIsBusy;              // True when the player is off chatting or futzing with options or whatever, false when they are "active"

   StringTableEntry mClientName;
   StringTableEntry mClientNameNonUnique;    // For authentication, not unique name.
   Nonce mClientId;
   bool mClientClaimsToBeVerified;
   bool mClientNeedsToBeVerified;
   bool mIsVerified;          // True if the connection has a verified account confirmed by the master
   Timer mAuthenticationTimer;
   S32 mAuthenticationCounter;

   StringTableEntry mServerName;
   Vector<U32> mLoadout;
   SafePtr<ClientRef> mClientRef;


public:
   ClientGame *mClientGame;
   Vector<U32> mOldLoadout;   // Server: to respawn with old loadout  Client: to check if using same loadout configuration
   U16 switchedTeamCount;

   U8 mVote;  // 0 = not voted,  1 = vote yes,  2 = vote no
   U32 mVoteTime;
   bool mChatMute;

   void linkToClientList();
   Vector<LevelInfo> mLevelInfos;

   static const S32 MASTER_SERVER_FAILURE_RETRY = 10000;   // 10 secs

   enum MessageColors
   {
      ColorWhite,
      ColorRed,
      ColorGreen,
      ColorBlue,
      ColorAqua,
      ColorYellow,
      ColorNuclearGreen,
      ColorCount              // Must be last
   };

   enum ParamType             // Be careful changing the order of this list... c2sSetParam() expects this for message creation
   {
      LevelChangePassword = 0,
      AdminPassword = 1,
      ServerPassword = 2,
      ServerName = 3,
      ServerDescr = 4,
      DeleteLevel = 5,
      ParamTypeCount       // Must be last
   };

   static const S32 BanDuration = 30000;     // Players are banned for 30secs after being kicked

   GameConnection();                                  // Constructor
   GameConnection(const ClientInfo &clientInfo);      // Constructor
   ~GameConnection();                                 // Destructor


   // These from the DataSendable interface class
   TNL_DECLARE_RPC(s2rSendLine, (StringPtr line));
   TNL_DECLARE_RPC(s2rCommandComplete, (RangedU32<0,SENDER_STATUS_COUNT> status));


   S32 mScore;                // Total points scored my this connection
   S32 mTotalScore;           // Total points scored by anyone while this connection is alive
   U32 mGamesPlayed;          // Number of games played, obviously
   F32 mRating;               // Game-normalized rating
   Statistics mStatistics;    // Player statistics tracker

   Timer mSwitchTimer;        // Timer controlling when player can switch teams after an initial switch

   void setClientName(StringTableEntry name) { mClientName = name; }
   void setClientNameNonUnique(StringTableEntry name) { mClientNameNonUnique = name; }
   void setServerName(StringTableEntry name) { mServerName = name; }

   bool lostContact() { return getTimeSinceLastPacketReceived() > 2000 && mLastPacketRecvTime != 0; }     // No contact in 2000ms?  That's bad!

   std::string getServerName() { return mServerName.getString(); }
   static std::string makeUnique(std::string name);    // Make sure a given name is unique across all clients & bots

   void setClientRef(ClientRef *theRef);
   ClientRef *getClientRef();

   StringTableEntryRef getClientName() { return mClientName; }

   void submitAdminPassword(const char *password);
   void submitLevelChangePassword(std::string password);

   void suspendGame();
   void unsuspendGame();

   bool isRobot() { return mIsRobot; }
   void setIsRobot(bool robot) { mIsRobot = robot; }

   bool isAdmin() { return mIsAdmin; }
   void setIsAdmin(bool admin) { mIsAdmin = admin; }

   bool isBusy() { if(!this) return false; else return mIsBusy; }
   void setIsBusy(bool busy) { mIsBusy = busy; }

   bool isLevelChanger() { return mIsLevelChanger; }
   void setIsLevelChanger(bool levelChanger) { mIsLevelChanger = levelChanger; }

   // Tell UI we're waiting for password confirmation from server
   void setWaitingForPermissionsReply(bool waiting) { mWaitingForPermissionsReply = waiting; }
   bool waitingForPermissionsReply() { return mWaitingForPermissionsReply; }

   // Tell UI whether we've recieved password confirmation from server
   void setGotPermissionsReply(bool gotReply) { mGotPermissionsReply = gotReply; }
   bool gotPermissionsReply() { return mGotPermissionsReply; }

   // Suspend/unsuspend game
   TNL_DECLARE_RPC(c2sSuspendGame, (bool suspend));
   TNL_DECLARE_RPC(s2cUnsuspend, ());

   TNL_DECLARE_RPC(c2sEngineerDeployObject, (RangedU32<0,EngineeredObjectCount> type));      // Player using engineer module
   bool sEngineerDeployObject(U32 type);      // Player using engineer module, robots use this, bypassing the net interface. True if successful.

   // Chage passwords on the server
   void changeParam(const char *param, ParamType type);

   TNL_DECLARE_RPC(c2sAdminPassword, (StringPtr pass));
   TNL_DECLARE_RPC(c2sLevelChangePassword, (StringPtr pass));

   TNL_DECLARE_RPC(c2sSetAuthenticated, ());      // Tell server that the client is (or claims to be) authenticated

   TNL_DECLARE_RPC(c2sSetParam, (StringPtr param, RangedU32<0, ParamTypeCount> type));


   TNL_DECLARE_RPC(s2cSetIsAdmin, (bool granted));
   TNL_DECLARE_RPC(s2cSetIsLevelChanger, (bool granted, bool notify));

   TNL_DECLARE_RPC(s2cSetServerName, (StringTableEntry name));


   TNL_DECLARE_RPC(c2sAdminPlayerAction, (StringTableEntry playerName, U32 actionIndex, S32 team));

   bool isInCommanderMap() { return mInCommanderMap; }
   TNL_DECLARE_RPC(c2sRequestCommanderMap, ());
   TNL_DECLARE_RPC(c2sReleaseCommanderMap, ());

   TNL_DECLARE_RPC(c2sRequestLoadout, (Vector<U32> loadout));     // Client has changed his loadout configuration
   void sRequestLoadout(Vector<U32> &loadout);                     // Robot has changed his loadout configuration

   TNL_DECLARE_RPC(s2cDisplayMessageESI, (RangedU32<0, ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, 
                   StringTableEntry formatString, Vector<StringTableEntry> e, Vector<StringPtr> s, Vector<S32> i));
   TNL_DECLARE_RPC(s2cDisplayMessageE, (RangedU32<0, ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, 
                   StringTableEntry formatString, Vector<StringTableEntry> e));
   TNL_DECLARE_RPC(s2cTouchdownScored, (U32 sfx, S32 team, StringTableEntry formatString, Vector<StringTableEntry> e));

   TNL_DECLARE_RPC(s2cDisplayMessage, (RangedU32<0, ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString));
   TNL_DECLARE_RPC(s2cDisplayMessageBox, (StringTableEntry title, StringTableEntry instr, Vector<StringTableEntry> message));
   TNL_DECLARE_RPC(s2cAddLevel, (StringTableEntry name, StringTableEntry type));
   TNL_DECLARE_RPC(c2sRequestLevelChange, (S32 newLevelIndex, bool isRelative));
   void c2sRequestLevelChange2(S32 newLevelIndex, bool isRelative);
   TNL_DECLARE_RPC(c2sRequestShutdown, (U16 time, StringPtr reason));
   TNL_DECLARE_RPC(c2sRequestCancelShutdown, ());
   TNL_DECLARE_RPC(s2cInitiateShutdown, (U16 time, StringTableEntry name, StringPtr reason, bool originator));
   TNL_DECLARE_RPC(s2cCancelShutdown, ());

   TNL_DECLARE_RPC(c2sSetIsBusy, (bool busy));

   TNL_DECLARE_RPC(c2sSetServerAlertVolume, (S8 vol));
   TNL_DECLARE_RPC(c2sRenameClient, (StringTableEntry newName));

   TNL_DECLARE_RPC(c2sRequestCurrentLevel, ());

   bool mSoccerCollide;
   TNL_DECLARE_RPC(s2cSoccerCollide, (bool enable));

   U8 mSendableFlags;
   TNL::ByteBuffer *mDataBuffer;
   TNL_DECLARE_RPC(s2rSendableFlags, (U8 flags));
   TNL_DECLARE_RPC(s2rSendDataParts, (U8 type, ByteBufferPtr data));
   bool s2rUploadFile(const char *filename, U8 type);

   static GameConnection *getClientList();
   static S32 getClientCount();
   static bool onlyClientIs(GameConnection *client);
   //static GameConnection *findClient(const Nonce &clientId);   // Loop through the client list, return first match

   Nonce *getClientId() { return &mClientId; }

   void setAuthenticated(bool isVerified);    // Runs on server only, after receiving a message from the master, or on local connection
   void resetAuthenticationTimer() { mAuthenticationTimer.reset(MASTER_SERVER_FAILURE_RETRY + 1000); mAuthenticationCounter++;}
   S32 getAuthenticationCounter() { return mAuthenticationCounter; }
   bool isAuthenticated() { return mIsVerified; }
   void requestAuthenticationVerificationFromMaster();
   void updateAuthenticationTimer(U32 timeDelta);

   GameConnection *getNextClient();

   void displayMessageE(U32 color, U32 sfx, StringTableEntry formatString, Vector<StringTableEntry> e);

   const Vector<U32> &getLoadout() { return mLoadout; }
   void writeConnectRequest(BitStream *stream);
   bool readConnectRequest(BitStream *stream, NetConnection::TerminationReason &reason);

   void onConnectionEstablished();

   void onConnectTerminated(TerminationReason r, const char *notUsed);

   void onConnectionTerminated(TerminationReason r, const char *string);

   TNL_DECLARE_NETCONNECTION(GameConnection);
};

LevelInfo getLevelInfo(char *level, S32 size);


};

#endif

