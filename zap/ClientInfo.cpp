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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "ship.h"
#include "ClientInfo.h"
#include "gameConnection.h"
#include "playerInfo.h"
#include "EditorObject.h"        // For NO_TEAM def
#include "EngineeredItem.h"      // For EngineerModuleDeployer def

#include "voiceCodec.h"


class Game;

namespace Zap
{

// Constructor
ClientInfo::ClientInfo()
{
   mPlayerInfo = NULL;
   mGame = NULL;

   mScore = 0;
   mTotalScore = 0;
   mTeamIndex = (NO_TEAM + 0);
   mPing = 0;
   mIsAdmin = false;
   mIsLevelChanger = false;
   mIsRobot = false;
   mIsAuthenticated = false;
   mBadges = NO_BADGES;
   mNeedToCheckAuthenticationWithMaster = false;     // Does client report that they are verified
   mSpawnDelayed = false;
   mIsBusy = false;
   mIsEngineeringTeleporter = false;
   mShipSystemsDisabled = false;

   // After canceling /idle command, this is the delay penalty
   static const U32 SPAWN_UNDELAY_TIMER_DELAY = 5000;    // 5 secs

   mReturnToGameTimer.clear();
   mReturnToGameTimer.setPeriod(SPAWN_UNDELAY_TIMER_DELAY);
}


// Destructor
ClientInfo::~ClientInfo()
{
   delete mPlayerInfo;
}


void ClientInfo::setAuthenticated(bool isAuthenticated, Int<BADGE_COUNT> badges)
{
   mNeedToCheckAuthenticationWithMaster = false;     // Once we get here, we'll treat the ruling as definitive
   mIsAuthenticated = isAuthenticated; 
   mBadges = badges;
}


Int<BADGE_COUNT> ClientInfo::getBadges()
{
   return mBadges;
}


bool ClientInfo::hasBadge(MeritBadges badge)
{
   return mBadges & BIT(badge);
}


const StringTableEntry ClientInfo::getName()
{
   return mName;
}


// An 8 bit bitmask to send to master on connection
const U8 ClientInfo::getPlayerFlagstoSendToMaster() const
{
   U8 bitmask = 0;

#ifdef TNL_DEBUG
   bitmask |= ClientDebugModeFlag;
#endif

   return bitmask;
}

void ClientInfo::setName(const StringTableEntry &name)
{
   mName = name;
}


S32 ClientInfo::getScore()
{
   return mScore;
}


void ClientInfo::setScore(S32 score)
{
   mScore = score;
}


void ClientInfo::addScore(S32 score)
{
   mScore += score;
}


void ClientInfo::setShip(Ship *ship)
{
   mShip = ship;
}


Ship *ClientInfo::getShip()
{
   return mShip;
}


void ClientInfo::setNeedToCheckAuthenticationWithMaster(bool needToCheck)
{
   mNeedToCheckAuthenticationWithMaster = needToCheck;
}


bool ClientInfo::getNeedToCheckAuthenticationWithMaster()
{
   return mNeedToCheckAuthenticationWithMaster;
}


// Returns true if spawn has actually been delayed 
bool ClientInfo::isSpawnDelayed()
{
   return mSpawnDelayed;
}


void ClientInfo::setShipSystemsDisabled(bool disabled)
{
   mShipSystemsDisabled = disabled;
}


bool ClientInfo::isShipSystemsDisabled()
{
   return mShipSystemsDisabled;
}


// Returns true if spawn has actually been delayed 
bool ClientInfo::isBusy()
{
   return mIsBusy;
}


void ClientInfo::setIsBusy(bool isBusy)
{
   mIsBusy = isBusy;
}


void ClientInfo::resetLoadout(bool levelHasLoadoutZone)
{
   // Save current loadout to put on-deck
   LoadoutTracker loadout = getLoadout();

   resetLoadout();
   mOldLoadout.resetLoadout();

   // If the current level has a loadout zone, put last level's load-out on-deck
   if(levelHasLoadoutZone)
      requestLoadout(loadout);
}


void ClientInfo::resetLoadout()
{
   mLoadout.setLoadout(DefaultLoadout);
}


const LoadoutTracker &ClientInfo::getLoadout() const
{
   return mLoadout;
}


void ClientInfo::resetOldLoadout()
{
   mOldLoadout.resetLoadout();
}


void ClientInfo::setOldLoadout(const LoadoutTracker &loadout)
{
   mOldLoadout = loadout;
}


S32 ClientInfo::getPing()
{
   return mPing;
}


void ClientInfo::setPing(S32 ping)
{
   mPing = ping;
}


S32 ClientInfo::getTeamIndex()
{
   return mTeamIndex;
}


void ClientInfo::setTeamIndex(S32 teamIndex)
{
   mTeamIndex = teamIndex;
}


bool ClientInfo::isAuthenticated()
{
   return mIsAuthenticated;
}


bool ClientInfo::isLevelChanger()
{
   return mIsLevelChanger;
}


void ClientInfo::setIsLevelChanger(bool isLevelChanger)
{
   mIsLevelChanger = isLevelChanger;
}


bool ClientInfo::isAdmin()
{
   return mIsAdmin;
}


void ClientInfo::setIsAdmin(bool isAdmin)
{
   mIsAdmin = isAdmin;
}


bool ClientInfo::isRobot()
{
   return mIsRobot;
}


F32 ClientInfo::getCalculatedRating()
{
   return mStatistics.getCalculatedRating();
}


// Resets stats and the like
void ClientInfo::endOfGameScoringHandler()
{
   mStatistics.addGamePlayed();
   mStatistics.resetStatistics();
}


LuaPlayerInfo *ClientInfo::getPlayerInfo()
{
   // Lazily initialize
   if(!mPlayerInfo)
      mPlayerInfo = new PlayerInfo(this);   // Deleted in destructor

   return mPlayerInfo;
}


// Server only, robots can run this, bypassing the net interface. Return true if successfuly deployed.
// TODO: Move this elsewhere   <--  where?...  anybody?
bool ClientInfo::sEngineerDeployObject(U32 objectType)
{
   Ship *ship = getShip();
   if(!ship)                                    // Not a good sign...
      return false;                             // ...bail

   GameType *gameType = ship->getGame()->getGameType();

   if(!gameType->isEngineerEnabled())          // Something fishy going on here...
      return false;                            // ...bail

   EngineerModuleDeployer deployer;

   // Check if we can create the engineer object; if not, return false
   if(!deployer.canCreateObjectAtLocation(ship->getGame()->getGameObjDatabase(), ship, objectType))
   {
      if(!isRobot())
         getConnection()->s2cDisplayErrorMessage(deployer.getErrorMessage().c_str());

      return false;
   }

   // Now deploy the object
   if(deployer.deployEngineeredItem(ship->getClientInfo(), objectType))
   {
      // Announce the build
      StringTableEntry msg( "%e0 has engineered a %e1." );

      U32 responseEvent;

      Vector<StringTableEntry> e;
      e.push_back(getName());

      Statistics *stats = getStatistics();

      switch(objectType)
      {
         case EngineeredTurret:
            e.push_back("turret");
            responseEvent = EngineerEventTurretBuilt;
            stats->mTurretsEngineered++;
            break;

         case EngineeredForceField:
            e.push_back("force field");
            responseEvent = EngineerEventForceFieldBuilt;
            stats->mFFsEngineered++;
            break;

         case EngineeredTeleporterEntrance:
            e.push_back("teleport entrance");
            responseEvent = EngineerEventTeleporterEntranceBuilt;
            break;

         case EngineeredTeleporterExit:
            e.push_back("teleport exit");
            responseEvent = EngineerEventTeleporterExitBuilt;
            stats->mTeleportersEngineered++;
            break;

         default:
            TNLAssert(false, "Should never get here");
            return false;
      }

      // Send response to client that is doing the engineering
      if(!isRobot())
         getConnection()->s2cEngineerResponseEvent(responseEvent);

      gameType->broadcastMessage(GameConnection::ColorAqua, SFXNone, msg, e);

      // Finally, deduct energy cost
      S32 energyCost = Game::getModuleInfo(ModuleEngineer)->getPrimaryPerUseCost();
      ship->creditEnergy(-energyCost);    // Deduct energy from engineer

      return true;
   }

   // Else deployment failed and we need to credit some energy back to the client
   S32 energyCost = Game::getModuleInfo(ModuleEngineer)->getPrimaryPerUseCost();
   getConnection()->s2cCreditEnergy(energyCost);  

   // And depart quietly
   return false;
}


void ClientInfo::setEngineeringTeleporter(bool isEngineeringTeleporter1)
{
   if(isEngineeringTeleporter() == isEngineeringTeleporter1)
      return;

   setIsEngineeringTeleporter(isEngineeringTeleporter1);

   // Tell everyone that a particular client is engineering a teleport
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      GameType *gameType = mGame->getGameType();

      if(gameType)
         gameType->s2cSetPlayerEngineeringTeleporter(mName, isEngineeringTeleporter1);
   }
}


void ClientInfo::sDisableShipSystems(bool disable)
{
   // We only need to tell the one client
   if(!isRobot() && isShipSystemsDisabled() != disable)  // only send if different
      getConnection()->s2cDisableWeaponsAndModules(disable);

   // Server's ClientInfo
   setShipSystemsDisabled(disable);
}


void ClientInfo::sEngineerDeploymentInterrupted(U32 objectType)
{
   if(objectType == EngineeredTeleporterExit)
   {
      getShip()->destroyPartiallyDeployedTeleporter();
      sTeleporterCleanup();
   }
}


void ClientInfo::sTeleporterCleanup()
{
   getShip()->setEngineeredTeleporter(NULL);   // Clear out the attached teleporter
   sDisableShipSystems(false);
   setEngineeringTeleporter(false);
}


void ClientInfo::requestLoadout(const LoadoutTracker &loadout)
{
   if(!loadout.isValid())
      return;

   mLoadout = loadout;

   GameType *gt = mGame->getGameType();

   if(gt)
      gt->SRV_clientRequestLoadout(this, mLoadout);    // This will set loadout if ship is in loadout zone
}


const LoadoutTracker &ClientInfo::getOldLoadout() const
{
   return mOldLoadout;
}


// Return pointer to statistics tracker 
Statistics *ClientInfo::getStatistics()
{
   return &mStatistics;
}


Nonce *ClientInfo::getId()
{
   return &mId;
}


// Methods to provide access to mReturnToGameTimer -- this is used on the server to enforce a post /idle delay
// and used on the client to display the (approximate) time left in that delay.
U32  ClientInfo::getReturnToGameTime()                  { return mReturnToGameTimer.getCurrent();      }
void ClientInfo::setReturnToGameTimer(U32 timeDelta) { mReturnToGameTimer.reset(timeDelta, mReturnToGameTimer.getPeriod()); }
bool ClientInfo::updateReturnToGameTimer(U32 timeDelta) { return mReturnToGameTimer.update(timeDelta); }
void ClientInfo::resetReturnToGameTimer()               {        mReturnToGameTimer.reset();           }


////////////////////////////////////////
////////////////////////////////////////

// Constructor
FullClientInfo::FullClientInfo(Game *game, GameConnection *gameConnection, bool isRobot) : ClientInfo()
{
   mGame = game;
   mClientConnection = gameConnection;
   mIsRobot = isRobot;
}


// Destructor
FullClientInfo::~FullClientInfo()
{
   // Do nothing
}


void FullClientInfo::setAuthenticated(bool isAuthenticated, Int<BADGE_COUNT> badges)
{
   TNLAssert(isAuthenticated || badges == NO_BADGES, "Unauthenticated players should never have badges!");
   Parent::setAuthenticated(isAuthenticated, badges);

   // Broadcast new connection status to all clients, except the client that is authenticated.  Presumably they already know.  
   if(mClientConnection && mClientConnection->isConnectionToClient())      
      for(S32 i = 0; i < gServerGame->getClientCount(); i++)
         if(gServerGame->getClientInfo(i)->getName() != mName && gServerGame->getClientInfo(i)->getConnection())
            gServerGame->getClientInfo(i)->getConnection()->s2cSetAuthenticated(mName, isAuthenticated, badges);
}


F32 FullClientInfo::getRating()
{
   // Initial case: no one has scored
   if(mTotalScore == 0)      
      return .5;

   // Standard case: 
   else   
      return F32(mScore) / F32(mTotalScore);
}


// Check if player is "on hold" due to inactivity; bots are never on hold.  Server only!
bool FullClientInfo::shouldDelaySpawn()
{
   if(mIsRobot)         // Robots are never spawn-delayed
      return false;

   return getConnection()->getTimeSinceLastMove() > GameConnection::SPAWN_DELAY_TIME;    // 20 secs -- includes time between games
}


bool FullClientInfo::hasReturnToGamePenalty()
{
   return mReturnToGameTimer.getCurrent() != 0;
}


// Server only -- RemoteClientInfo has a client-side override
void FullClientInfo::setSpawnDelayed(bool spawnDelayed)
{
   if(spawnDelayed == mSpawnDelayed)                     // Already in requested state -- nothing to do
      return;

   //if(!spawnDelayed && mReturnToGameTimer.getCurrent())  // Already waiting to unspawn... hold your horses!
      //return;

   mSpawnDelayed = spawnDelayed;

   if(mGame->isServer())
   {
      if(spawnDelayed)                                      // Tell client their spawn has been delayed
         getConnection()->s2cPlayerSpawnDelayed((getReturnToGameTime() + 99) / 100); // add for round up divide
		else
			getConnection()->s2cPlayerSpawnUndelayed();
      mGame->getGameType()->s2cSetIsSpawnDelayed(mName, spawnDelayed);  // Notify other clients
   }
}


GameConnection *FullClientInfo::getConnection()
{
   return mClientConnection;
}


void FullClientInfo::setConnection(GameConnection *conn)
{
   mClientConnection = conn;
}


void FullClientInfo::setRating(F32 rating)
{
   TNLAssert(false, "Ratings can't be set for this class!");
}


SoundEffect *FullClientInfo::getVoiceSFX()
{
   TNLAssert(false, "Can't access VoiceSFX from this class!");
   return NULL;
}


VoiceDecoder *FullClientInfo::getVoiceDecoder()
{
   TNLAssert(false, "Can't access VoiceDecoder from this class!");
   return NULL;
}


bool FullClientInfo::isEngineeringTeleporter()
{
   return getShip()->getEngineeredTeleporter() != NULL;
}


void FullClientInfo::setIsEngineeringTeleporter(bool isEngineeringTeleporter)
{
   TNLAssert(false, "isEngineeringTeleporter shouldn't be set for this class!");
}


////////////////////////////////////////
////////////////////////////////////////


#ifndef ZAP_DEDICATED
// Constructor
RemoteClientInfo::RemoteClientInfo(Game *game, const StringTableEntry &name, bool isAuthenticated, Int<BADGE_COUNT> badges, 
                                   bool isRobot, bool isAdmin, bool isLevelChanger, bool isSpawnDelayed, bool isBusy) : ClientInfo()
{
   mGame = game;
   mName = name;
   mIsAuthenticated = isAuthenticated;
   mIsRobot = isRobot;
   mIsAdmin = isAdmin;
   mIsLevelChanger = isLevelChanger;
   mTeamIndex = NO_TEAM;
   mRating = 0;
   mBadges = badges;
   mSpawnDelayed = isSpawnDelayed;
   mIsBusy = isBusy;

   // Initialize speech stuff
   mDecoder = new SpeexVoiceDecoder();                                  // Deleted in destructor
   mVoiceSFX = new SoundEffect(SFXVoice, NULL, 1, Point(), Point());    // RefPtr, will self-delete
}


// Destructor
RemoteClientInfo::~RemoteClientInfo()
{
   delete mDecoder;
}


GameConnection *RemoteClientInfo::getConnection()
{
   TNLAssert(false, "Can't get a GameConnection from a RemoteClientInfo!");
   return NULL;
}


void RemoteClientInfo::setConnection(GameConnection *conn)
{
   TNLAssert(false, "Can't set a GameConnection on a RemoteClientInfo!");
}


void RemoteClientInfo::setSpawnDelayed(bool spawnDelayed)
{
   mSpawnDelayed = spawnDelayed;
}


F32 RemoteClientInfo::getRating()
{
   return mRating;
}


void RemoteClientInfo::setRating(F32 rating)
{
   mRating = rating;
}


// Voice chat stuff -- these will be invalid on the server side
SoundEffect *RemoteClientInfo::getVoiceSFX()
{
   return mVoiceSFX;
}


VoiceDecoder *RemoteClientInfo::getVoiceDecoder()
{
   return mDecoder;
}


bool RemoteClientInfo::isEngineeringTeleporter()
{
   return mIsEngineeringTeleporter;
}


void RemoteClientInfo::setIsEngineeringTeleporter(bool isEngineeringTeleporter)
{
   mIsEngineeringTeleporter = isEngineeringTeleporter;
}

#endif


};
