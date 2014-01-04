//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CLIENT_INFO_H_
#define _CLIENT_INFO_H_

#include "statistics.h"          // For Statistics def
#include "SharedConstants.h"
#include "LoadoutTracker.h"

#include "Intervals.h"
#include "Timer.h"

#include "tnlNetBase.h"
#include "tnlNonce.h"


namespace Zap
{

class Game;
class GameConnection;
class Ship;
class SoundEffect;
class VoiceDecoder;

class LuaPlayerInfo;



// This object only concerns itself with things that one client tracks about another.  We use it for other purposes, of course, 
// as a convenient strucure for holding certain settings about the local client, or about remote clients when we are running on the server.
// But the general scope of what we track should be limited; other items should be stored directly on the GameConnection object itself.
// Note that this comment is probably out of date.
class ClientInfo : public SafePtrData, public RefPtrData
{
public:
   // Each role has all permissions a lesser one is granted
   // Note:  changing this will break network compatibility
   enum ClientRole {
      RoleNone,
      RoleLevelChanger,
      RoleAdmin,
      RoleOwner,
      MaxRoles
   };

   // Robots can be different classes... as we descend the following list, each class is a lower priority
   // and can be more readily removed
   enum ClientClass {
      ClassHuman,                   // Human player, obviously
      ClassRobotAddedByLevel,       // A bot, specified in the level file with Robot directive
      ClassRobotAddedByAddbots,     // A bot added with the /addbot or /addbots command
      ClassRobotAddedByAutoleveler, // A bot added by the autoleveling system
      ClassRobotWithUnknownSource,
      ClassUnknown,
      ClassCount
   };

   static const S32 MaxKillStreakLength = 4095;
   // After canceling /idle command, this is the delay penalty
   static const U32 SPAWN_UNDELAY_TIMER_DELAY = FIVE_SECONDS; 

private:
   LuaPlayerInfo *mPlayerInfo;      // Lua access to this class
   Statistics mStatistics;          // Statistics tracker
   SafePtr<Ship> mShip;             // SafePtr will return NULL if ship object is deleted
   LoadoutTracker mOnDeckLoadout;
   LoadoutTracker mActiveLoadout;   // Server: to respawn with old loadout  Client: to check if using same loadout configuration

   bool mNeedToCheckAuthenticationWithMaster;

protected:
   StringTableEntry mName;
   S32 mScore;
   S32 mTotalScore;
   Nonce mId;
   S32 mTeamIndex;               // <=== Does not get set on the client's LocalClientInfo!!!
   S32 mPing;
   ClientRole mRole;
   bool mIsAuthenticated;
   bool mSpawnDelayed;
   bool mIsBusy;
   bool mIsEngineeringTeleporter;
   bool mShipSystemsDisabled;
   Int<BADGE_COUNT> mBadges;
   U16 mGamesPlayed;

   U32 mCurrentKillStreak;
   Game *mGame;

   bool mNeedReturnToGameTimer;
   Timer mReturnToGameTimer;

public:
   ClientInfo();           // Constructor
   virtual ~ClientInfo();  // Destructor

   virtual GameConnection *getConnection() = 0;
   virtual void setConnection(GameConnection *conn) = 0;

   const StringTableEntry getName() const;
   void setName(const StringTableEntry &name);

   const U8 getPlayerFlagstoSendToMaster() const;

   S32 getScore();
   void setScore(S32 score);
   void addScore(S32 score);

   U16 getGamesPlayed() const;

   // Whole mess of loadout related functions
   const LoadoutTracker &getOnDeckLoadout() const;
   const LoadoutTracker &getOldLoadout() const;

   void resetActiveLoadout();
   void saveActiveLoadout(const LoadoutTracker &loadout);
   void updateLoadout(bool useOnDeck, bool engineerAllowed, bool silent = false);
   void resetLoadout(bool levelHasLoadoutZone);
   void resetLoadout();
   void requestLoadout(const LoadoutTracker &loadout);

   Timer respawnTimer;

   bool isLoadoutValid(const LoadoutTracker &loadout, bool engineerAllowed);

   void setNeedToCheckAuthenticationWithMaster(bool needToCheck);
   bool getNeedToCheckAuthenticationWithMaster();

   bool isSpawnDelayed();              // Returns true if spawn has actually been delayed   
   virtual void setSpawnDelayed(bool spawnDelayed) = 0;

   virtual bool isPlayerInactive();    // Server only

   bool isBusy();
   void setIsBusy(bool isBusy);

   Ship *getShip();
   void setShip(Ship *ship);

   virtual void setShowLevelUpMessage(S32 level);
   virtual S32 getShowLevelUpMessage() const;

   virtual void setRating(F32 rating) = 0;
   virtual F32 getRating() = 0;
   F32 getCalculatedRating();
   void endOfGameScoringHandler();     // Resets stats and the like

   void incrementKillStreak();
   void clearKillStreak();
   U32 getKillStreak() const;

   S32 getPing();
   void setPing(S32 ping);

   S32 getTeamIndex();
   void setTeamIndex(S32 teamIndex);

   virtual void setAuthenticated(bool isAuthenticated, Int<BADGE_COUNT> badges, U16 gamesPlayed);
   bool isAuthenticated();

   Int<BADGE_COUNT> getBadges();
   bool hasBadge(MeritBadges badge);

   void setRole(ClientRole role);
   ClientRole getRole();
   bool isLevelChanger();
   bool isAdmin();
   bool isOwner();

   virtual bool isRobot() const = 0;

   Statistics *getStatistics();      // Return pointer to statistics tracker 

   LuaPlayerInfo *getPlayerInfo();

   // Player using engineer module, robots use this, bypassing the net interface. True if successful.
   bool sEngineerDeployObject(U32 objectType);
   void setEngineeringTeleporter(bool isEngineeringTeleport);
   void sEngineerDeploymentInterrupted(U32 objectType);
   void sTeleporterCleanup();

   void sDisableShipSystems(bool disable);

   void setShipSystemsDisabled(bool disabled);
   bool isShipSystemsDisabled();

   void addKill();
   void addDeath();

   Nonce *getId();

   U32 getReturnToGameTime();
   void setReturnToGameTimer(U32 time);
   bool updateReturnToGameTimer(U32 timeDelta);
   void requireReturnToGameTimer(bool required);

   virtual SoundEffect *getVoiceSFX() = 0;
   virtual VoiceDecoder *getVoiceDecoder() = 0;
   virtual void playVoiceChat(const ByteBufferPtr &voiceBuffer) = 0;

   virtual bool isEngineeringTeleporter() = 0;
   virtual void setIsEngineeringTeleporter(bool isEngineeringTeleporter) = 0;
};

////////////////////////////////////////
////////////////////////////////////////

class GameConnection;

class FullClientInfo : public ClientInfo
{
   typedef ClientInfo Parent;

private:
   GameConnection *mClientConnection;
   S32 mShowLevelUpMessage;
   ClientClass mClientClass;

public:
   FullClientInfo(Game *game, GameConnection *clientConnection, const string &name, ClientClass clientClass); // Constructor
   virtual ~FullClientInfo();                                                                                 // Destructor

   // WARNING!! mClientConnection can be NULL on client and server's robots
   GameConnection *getConnection();
   void setConnection(GameConnection *conn);

   void setAuthenticated(bool isAuthenticated, Int<BADGE_COUNT> badges, U16 gamesPlayed);

   void setSpawnDelayed(bool spawnDelayed);
   bool isPlayerInactive();
   bool hasReturnToGamePenalty();

   void setRating(F32 rating);
   F32 getRating();

   bool isRobot() const;
   void setClientClass(ClientClass clientClass);
   ClientClass getClientClass();


   SoundEffect *getVoiceSFX();
   VoiceDecoder *getVoiceDecoder();
   void playVoiceChat(const ByteBufferPtr &voiceBuffer);

   bool isEngineeringTeleporter();
   void setIsEngineeringTeleporter(bool isEngineeringTeleporter);
   void setShowLevelUpMessage(S32 level);
   S32 getShowLevelUpMessage() const;
};


////////////////////////////////////////
////////////////////////////////////////
// RemoteClientInfo is used on the client side to track information about other players; these other players do not have a connection
// to us -- all the information we know about them is located on this RemoteClientInfo object.
#ifndef ZAP_DEDICATED
class RemoteClientInfo : public ClientInfo
{
   typedef ClientInfo Parent;

private:
   F32 mRating;      // Ratings are provided by the server and stored here

   // For voice chat
   RefPtr<SoundEffect> mVoiceSFX;
   VoiceDecoder *mDecoder;

   bool mIsRobot;

public:
   RemoteClientInfo(Game *game, const StringTableEntry &name, bool isAuthenticated, Int<BADGE_COUNT> badges,      // Constructor
                    U16 gamesPlayed, RangedU32<0, MaxKillStreakLength> killStreak, bool isRobot, ClientRole role, 
                    bool isSpawnDelayed, bool isBusy);
   virtual ~RemoteClientInfo();      
   // Destructor
   void initialize();

   GameConnection *getConnection();
   void setConnection(GameConnection *conn);

   F32 getRating();
   void setRating(F32 rating);
   bool isRobot() const;

   void setSpawnDelayed(bool spawnDelayed);

   // Voice chat stuff -- these will be invalid on the server side
   SoundEffect *getVoiceSFX();
   VoiceDecoder *getVoiceDecoder();
   void playVoiceChat(const ByteBufferPtr &voiceBuffer);

   bool isEngineeringTeleporter();
   void setIsEngineeringTeleporter(bool isEngineeringTeleporter);
};
#endif


};


#endif

