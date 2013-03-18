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


#include "controlObjectConnection.h"
#include "dataConnection.h"            // For DataSendable interface
#include "statistics.h"
#include "SoundSystem.h"               // For NumSFXBuffers
#include "SharedConstants.h"           // For BADGE_COUNT constant

#include "ship.h"                      // For Ship::EnergyMax

#include "tnlNetConnection.h"
#include "Timer.h"
#include <time.h>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "GameTypesEnum.h"
#include "ServerGame.h"
#include "Engineerable.h"
#include "ChatCheck.h"

using namespace TNL;
using namespace std;

namespace Zap
{

////////////////////////////////////////
////////////////////////////////////////

class ClientGame;
struct LevelInfo;
class GameSettings;
class LuaPlayerInfo;
class ClientInfo;
class FullClientInfo;


class GameConnection: public ControlObjectConnection, public DataSendable, public ChatCheck
{
private:
   typedef ControlObjectConnection Parent;

   void initialize();

   time_t joinTime;
   bool mAcheivedConnection;

   // For saving passwords
   string mLastEnteredPassword;

   RefPtr<ClientInfo> mClientInfo;               // This could be either a FullClientInfo or a RemoteClientInfo

#ifndef ZAP_DEDICATED
   ClientGame *mClientGame;         // NULL on server side, not available for dedicated build
#endif
   ServerGame *mServerGame;         // NULL on client side

   bool mInCommanderMap;
   bool mWaitingForPermissionsReply;
   bool mGotPermissionsReply;

   bool mWantsScoreboardUpdates;    // Indicates if client has requested scoreboard streaming (e.g. pressing Tab key)
   bool mReadyForRegularGhosts;

   StringTableEntry mClientNameNonUnique; // For authentication, not unique name

   Timer mAuthenticationTimer;
   S32 mAuthenticationCounter;

   void displayMessage(U32 colorIndex, U32 sfxEnum, const char *message);    // Helper function

   StringTableEntry mServerName;
   Vector<U8> mLoadout;
   GameSettings *mSettings;

public:
   U16 switchedTeamCount;

   U8 mVote;                     // 0 = not voted,  1 = vote yes,  2 = vote no    TODO: Make 
   U32 mVoteTime;

   U32 mWrongPasswordCount;
   static const U32 MAX_WRONG_PASSWORD = 20;  // too many wrong password, and client get disconnect

   Vector<LevelInfo> mLevelInfos;

   static const S32 MASTER_SERVER_FAILURE_RETRY_TIME = 10000;   // 10 secs
   static const U32 SPAWN_DELAY_TIME = 20000;                   // 20 seconds until eligible for being spawn delayed

   static const char *getConnectionStateString(S32 i);

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
      AdminPassword,
      ServerPassword,
      ServerName,
      ServerDescr,

      // Items not listed in c2sSetParam()::*keys[] should be added here
      LevelDir, 

      // Items not listed in c2sSetParam()::*types[] should be added here
      DeleteLevel,            

      ParamTypeCount       // Must be last
   };


#ifndef ZAP_DEDICATED
   explicit GameConnection(ClientGame *game);      // Constructor for ClientGame
#endif
   GameConnection();                      // Constructor for ServerGame
   virtual ~GameConnection();             // Destructor


   // These from the DataSendable interface class
   TNL_DECLARE_RPC(s2rSendLine, (StringPtr line));
   TNL_DECLARE_RPC(s2rCommandComplete, (RangedU32<0,SENDER_STATUS_COUNT> status));


#ifndef ZAP_DEDICATED
   ClientGame *getClientGame();
   void setClientGame(ClientGame *game);
#endif

   Timer mSwitchTimer;           // Timer controlling when player can switch teams after an initial switch

   void setClientNameNonUnique(StringTableEntry name);
   void setServerName(StringTableEntry name);

   ClientInfo *getClientInfo();
   void setClientInfo(ClientInfo *clientInfo);

   bool lostContact();

   string getServerName();

   void reset();        // Clears/initializes some things between levels

   void submitPassword(const char *password);

   void unsuspendGame();

   void sendLevelList();

   bool isReadyForRegularGhosts();
   void setReadyForRegularGhosts(bool ready);

   bool wantsScoreboardUpdates();
   void setWantsScoreboardUpdates(bool wantsUpdates);

   virtual void onStartGhosting();  // Gets run when game starts
   virtual void onEndGhosting();    // Gets run when game is over


   // Tell UI we're waiting for password confirmation from server
   void setWaitingForPermissionsReply(bool waiting);
   bool waitingForPermissionsReply();

   // Tell UI whether we've received password confirmation from server
   void setGotPermissionsReply(bool gotReply);
   bool gotPermissionsReply();

   // Suspend/unsuspend game
   TNL_DECLARE_RPC(s2cSuspendGame, (bool gameIsRunning));
   TNL_DECLARE_RPC(s2cUnsuspend, ());

   void undelaySpawn();

   // Delay/undelay spawn
   TNL_DECLARE_RPC(s2cPlayerSpawnDelayed, ());
   TNL_DECLARE_RPC(c2sPlayerSpawnUndelayed, ());
   TNL_DECLARE_RPC(c2sPlayerRequestSpawnDelayed, ());


   // Player using engineer module
   TNL_DECLARE_RPC(c2sEngineerDeployObject,  (RangedU32<0,EngineeredItemCount> objectType));
   TNL_DECLARE_RPC(c2sEngineerInterrupted,   (RangedU32<0,EngineeredItemCount> objectType));
   TNL_DECLARE_RPC(s2cEngineerResponseEvent, (RangedU32<0,EngineerEventCount>  event));

   TNL_DECLARE_RPC(s2cDisableWeaponsAndModules, (bool disable));

   // Chage passwords on the server
   void changeParam(const char *param, ParamType type);

   TNL_DECLARE_RPC(c2sSubmitPassword, (StringPtr pass));

   // Tell server that the client is (or claims to be) authenticated
   TNL_DECLARE_RPC(c2sSetAuthenticated, ());       
   // Tell clients a player is authenticated, and pass on some badge info while we're on the phone
   TNL_DECLARE_RPC(s2cSetAuthenticated, (StringTableEntry, bool, Int<BADGE_COUNT>));   

   TNL_DECLARE_RPC(c2sSetParam, (StringPtr param, RangedU32<0, ParamTypeCount> type));


   TNL_DECLARE_RPC(s2cSetIsAdmin, (bool granted));
   TNL_DECLARE_RPC(s2cSetIsLevelChanger, (bool granted, bool notify));
   TNL_DECLARE_RPC(s2cWrongPassword, ());

   TNL_DECLARE_RPC(s2cSetServerName, (StringTableEntry name));

   bool isInCommanderMap();

   TNL_DECLARE_RPC(c2sRequestCommanderMap, ());
   TNL_DECLARE_RPC(c2sReleaseCommanderMap, ());

   TNL_DECLARE_RPC(c2sDeploySpybug, ());                       // Client requests a spybug be placed at ship's current location
   TNL_DECLARE_RPC(s2cCreditEnergy, (RangedU32<0, Ship::EnergyMax> energy));

   TNL_DECLARE_RPC(c2sRequestLoadout, (Vector<U8> loadout));   // Client has changed his loadout configuration

   TNL_DECLARE_RPC(s2cDisplayMessageESI, (RangedU32<0, ColorCount> color, RangedU32<0, NumSFXBuffers> sfx,
                   StringTableEntry formatString, Vector<StringTableEntry> e, Vector<StringPtr> s, Vector<S32> i));
   TNL_DECLARE_RPC(s2cDisplayMessageE, (RangedU32<0, ColorCount> color, RangedU32<0, NumSFXBuffers> sfx,
                   StringTableEntry formatString, Vector<StringTableEntry> e));
   TNL_DECLARE_RPC(s2cTouchdownScored, (RangedU32<0, NumSFXBuffers> sfx, S32 team, StringTableEntry formatString, Vector<StringTableEntry> e));

   TNL_DECLARE_RPC(s2cDisplayMessage, (RangedU32<0, ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString));
   TNL_DECLARE_RPC(s2cDisplayErrorMessage, (StringTableEntry formatString));    

   TNL_DECLARE_RPC(s2cDisplayMessageBox, (StringTableEntry title, StringTableEntry instr, Vector<StringTableEntry> message));

   TNL_DECLARE_RPC(s2cAddLevel, (StringTableEntry name, RangedU32<0, GameTypesCount> type));
   TNL_DECLARE_RPC(s2cRemoveLevel, (S32 index));

   TNL_DECLARE_RPC(c2sRequestLevelChange, (S32 newLevelIndex, bool isRelative));
   TNL_DECLARE_RPC(c2sRequestShutdown, (U16 time, StringPtr reason));
   TNL_DECLARE_RPC(c2sRequestCancelShutdown, ());
   TNL_DECLARE_RPC(s2cInitiateShutdown, (U16 time, StringTableEntry name, StringPtr reason, bool originator));
   TNL_DECLARE_RPC(s2cCancelShutdown, ());

   TNL_DECLARE_RPC(s2cSetIsBusy, (StringTableEntry name, bool isBusy));

   TNL_DECLARE_RPC(c2sSetIsBusy, (bool isBusy));

   TNL_DECLARE_RPC(c2sSetServerAlertVolume, (S8 vol));
   TNL_DECLARE_RPC(c2sRenameClient, (StringTableEntry newName));

   TNL_DECLARE_RPC(c2sRequestCurrentLevel, ());

   enum ServerFlags {
      ServerFlagAllowUpload = BIT(0),
      // U8 max!
   };

   enum LevelFileTransmissionStage {
      LevelFileTransmissionInProgress,
      LevelFileTransmissionComplete
   };

   U8 mSendableFlags;
   ByteBuffer *mDataBuffer;

   TNL_DECLARE_RPC(s2rSendableFlags, (U8 flags));
   TNL_DECLARE_RPC(s2rSendDataParts, (U8 type, ByteBufferPtr data));
   bool s2rUploadFile(const char *filename, U8 type);

   bool mVoiceChatEnabled;  // server side: false when this client have set the voice volume to zero, which means don't send voice to this client
                            // client side: this can allow or disallow sending voice to server
   TNL_DECLARE_RPC(s2rVoiceChatEnable, (bool enabled));

   void resetAuthenticationTimer();
   S32 getAuthenticationCounter();

   void requestAuthenticationVerificationFromMaster();
   void updateTimers(U32 timeDelta);

   void displayMessageE(U32 color, U32 sfx, StringTableEntry formatString, Vector<StringTableEntry> e);

   const Vector<U8> &getLoadout();

   static const U8 CONNECT_VERSION;  // may be useful in future version with same CS protocol number
   U8 mConnectionVersion;  // the CONNECT_VERSION of the other side of this connection

   void writeConnectRequest(BitStream *stream);
   bool readConnectRequest(BitStream *stream, NetConnection::TerminationReason &reason);
   void writeConnectAccept(BitStream *stream);
   bool readConnectAccept(BitStream *stream, NetConnection::TerminationReason &reason);

   void setConnectionSpeed(S32 speed);

   void onConnectionEstablished();
   void onConnectionEstablished_client();
   void onConnectionEstablished_server();

   void onConnectTerminated(TerminationReason r, const char *notUsed);

   void onConnectionTerminated(TerminationReason r, const char *string);



   TNL_DECLARE_NETCONNECTION(GameConnection);
};

};

#endif

